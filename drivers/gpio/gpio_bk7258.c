/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO driver for the BK7258.
 *
 * Every register semantic in this file was cross-checked against the vendor
 * SoC headers, referenced in the comment that uses it. Do not guess a value
 * here: a wrong guess builds fine and then does nothing on hardware.
 *
 * Sources used:
 * ap/middleware/soc/bk7258_ap/soc/gpio_struct.h pin config word layout
 * ap/middleware/soc/bk7258_ap/hal/gpio_ll.h vendor register access
 * ap/middleware/driver/gpio/gpio_driver_base.c vendor init / ISR flow
 * ap/middleware/soc/bk7258_ap/soc/icu_map.h INT_SRC_GPIO -> NVIC 55
 * ap/middleware/soc/bk7258_ap/hal/sys_types.h GPIO_INTERRUPT_CTRL_BIT
 * ap/include/driver/hal/hal_gpio_types.h gpio_int_type_t enum
 *
 * Two traps that are easy to miss even when copying the vendor driver
 * (measured during )
 * -------------------------------------------------------------------
 * 1. The second-level interrupt enable. Interrupts are two-level on the
 * BK7258: the NVIC (level 1, IRQ 55) plus cpu1_int_32_63_en in the SYSTEM
 * domain (level 2, bit 23). The vendor gpio_driver_init() ends with
 * sys_drv_int_group2_enable(GPIO_INTERRUPT_CTRL_BIT)
 * precisely for this. Calling IRQ_CONNECT without setting that bit means the
 * interrupt never reaches the CPU, with no error at all (the NVIC ICSR looks
 * perfectly clean), so it presents as "configured but no callback".
 *
 * 2. The interrupt type is 2 bits wide and has no both-edges mode:
 * 0 = low level, 1 = high level, 2 = rising edge, 3 = falling edge, and
 * that is all. GPIO_INT_TRIG_BOTH therefore has to return -ENOTSUP rather
 * than pretend to work, which would silently drop half the edges.
 *
 * Boundary against the resources shared with the system controller (worth remembering)
 * -----------------------------------------------------------------
 * GPIO lives in the AON power domain, so this core and the system controller look at the *same*
 * register file. The vendor driver arbitrates across cores with
 * amp_res_init(AMP_RES_ID_GPIO); we run single-core and have no arbitration,
 * so:
 * - the driver only writes pins that were explicitly configured, set or
 * interrupt_configure'd. It never does an "initialise every pin" sweep,
 * which would step on pins the system controller is using;
 * - the devicetree marks UART0's GPIO10/11 as reserved with
 * gpio-reserved-ranges, so a stray configuration cannot kill the system controller's console
 * (driving that UART takes the whole board down).
 */

#define DT_DRV_COMPAT beken_bk7258_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <soc.h>

/*
 * io mode field of the pin configuration word, bit[3:2]
 * bit2 = input_en (active high) bit3 = output_en (active LOW)
 *
 * The three values of the vendor gpio_ll_set_io_mode():
 * GPIO_OUTPUT_ENABLE -> 0 => bit[3:2] = 00 (output_en=0 on, input_en=0)
 * GPIO_INPUT_ENABLE -> 3 => bit[3:2] = 11 (output_en=1 off, input_en=1)
 * GPIO_IO_DISABLE -> 2 => bit[3:2] = 10 (output_en=1 off, input_en=0)
 *
 * For output we deliberately do NOT copy the vendor's 00 but use 01 instead.
 * This was forced by hardware measurements on and the reasoning has
 * to stay documented:
 *
 * bit0 (gpio_input, "current pin level") only tracks the pad when input_en=1
 * (measured in section [1] of the GPIO self test):
 * - bit[3:2] = 01 (output on + input_en on): write 0 reads 0, write 1
 * reads 1
 * - bit[3:2] = 00 (vendor setting, input_en=0): driving 0 still leaves
 * bit0 reading 1
 *
 * The consequence is concrete: Zephyr's gpio_port_get_raw() would read a
 * stale value in output mode, and every "drive then read back" usage --
 * our own self test, the shell's gpio get -- would report a false result.
 * The vendor FreeRTOS code never uses it that way, which is why it never
 * surfaced there.
 *
 * Output therefore uses bit[3:2] = 01: the output driver is on and the input
 * sampler is on as well. Measurement shows the two coexist (drive 0 reads 0,
 * drive 1 reads 1).
 */
#define BK_GPIO_IO_MASK    (SOC_GPIO_CFG_INPUT_EN_BIT | SOC_GPIO_CFG_OUTPUT_EN_BIT)
#define BK_GPIO_IO_OUTPUT  SOC_GPIO_CFG_INPUT_EN_BIT  /* bit[3:2] = 01 */
#define BK_GPIO_IO_INPUT   BK_GPIO_IO_MASK            /* bit[3:2] = 11 */
#define BK_GPIO_IO_DISABLE SOC_GPIO_CFG_OUTPUT_EN_BIT /* bit[3:2] = 10 */

#define BK_GPIO_PULL_MASK (SOC_GPIO_CFG_PULL_MODE_BIT | SOC_GPIO_CFG_PULL_MODE_EN_BIT)

struct gpio_bk7258_config {
	/* gpio_driver_config must be the first member */
	struct gpio_driver_config common;
	uint8_t ngpios;
	void (*irq_cfg_func)(void);
};

struct gpio_bk7258_data {
	/* gpio_driver_data must be the first member */
	struct gpio_driver_data common;
	sys_slist_t cb;
	struct k_spinlock lock;
};

/*
 * Basic register access
 */

static inline uint32_t pin_reg_read(gpio_pin_t pin)
{
	return sys_read32(SOC_GPIO_PIN_CFG_ADDR(pin));
}

static inline void pin_reg_write(gpio_pin_t pin, uint32_t v)
{
	sys_write32(v, SOC_GPIO_PIN_CFG_ADDR(pin));
}

/*
 * pin_configure
 */

static int gpio_bk7258_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	struct gpio_bk7258_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t v;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	/* Open drain / open source: the pin configuration word has no such bit,
	 * so this is explicitly unsupported.
	 */
	if ((flags & (GPIO_OPEN_DRAIN | GPIO_OPEN_SOURCE)) != 0) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	v = pin_reg_read(pin);

	/*
	 * Anything coming through the Zephyr GPIO API is treated as "use this
	 * as a plain GPIO":
	 * - clear the secondary-function master switch (bit 6), which is set
	 * by default for most pins in the mux table
	 * - clear the interrupt enable (bit 12); pin_interrupt_configure
	 * turns it on separately
	 */
	v &= ~(SOC_GPIO_CFG_2_FUNC_EN_BIT | SOC_GPIO_CFG_INT_EN_BIT);
	v &= ~BK_GPIO_IO_MASK;

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		v |= BK_GPIO_IO_INPUT;
		break;
	case GPIO_OUTPUT:
		/* Set the level before enabling the driver, to avoid a glitch */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			v |= SOC_GPIO_CFG_OUTPUT_BIT;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			v &= ~SOC_GPIO_CFG_OUTPUT_BIT;
		}
		v |= BK_GPIO_IO_OUTPUT;
		break;
	default: /* GPIO_DISCONNECTED */
		v |= BK_GPIO_IO_DISABLE;
		break;
	}

	v &= ~BK_GPIO_PULL_MASK;
	if ((flags & GPIO_PULL_UP) != 0) {
		v |= BK_GPIO_PULL_MASK; /* pull_en=1, pull_mode=1 (up) */
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		v |= SOC_GPIO_CFG_PULL_MODE_EN_BIT; /* pull_en=1, pull_mode=0 (down) */
	}

	pin_reg_write(pin, v);
	k_spin_unlock(&data->lock, key);

	/*
	 * GPIO_ACTIVE_LOW is ignored: the hardware has no polarity inversion
	 * bit. Deliberately not reported as -ENOTSUP, because samples such as
	 * blinky would then fail outright.
	 */
	return 0;
}

/*
 * Port access
 *
 * gpio_port_value_t and gpio_port_pins_t are both uint32_t and Zephyr has no
 * 64-bit port width option, so one controller can only expose 32 pins. The
 * BK7258 has 56; GPIO0..31 are exposed here (see ngpios in the SoC dtsi).
 */

static int gpio_bk7258_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	uint32_t v = 0U;
	gpio_pin_t pin;

	for (pin = 0; pin < cfg->ngpios; pin++) {
		if ((pin_reg_read(pin) & SOC_GPIO_CFG_INPUT_BIT) != 0U) {
			v |= BIT(pin);
		}
	}

	*value = v;
	return 0;
}

static int gpio_bk7258_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	gpio_pin_t pin;

	for (pin = 0; pin < cfg->ngpios; pin++) {
		uint32_t v;

		if ((mask & BIT(pin)) == 0U) {
			continue;
		}

		v = pin_reg_read(pin);
		if ((value & BIT(pin)) != 0U) {
			v |= SOC_GPIO_CFG_OUTPUT_BIT;
		} else {
			v &= ~SOC_GPIO_CFG_OUTPUT_BIT;
		}
		pin_reg_write(pin, v);
	}

	return 0;
}

static int gpio_bk7258_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	gpio_pin_t pin;

	for (pin = 0; pin < cfg->ngpios; pin++) {
		if ((pins & BIT(pin)) == 0U) {
			continue;
		}
		pin_reg_write(pin, pin_reg_read(pin) | SOC_GPIO_CFG_OUTPUT_BIT);
	}

	return 0;
}

static int gpio_bk7258_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	gpio_pin_t pin;

	for (pin = 0; pin < cfg->ngpios; pin++) {
		if ((pins & BIT(pin)) == 0U) {
			continue;
		}
		pin_reg_write(pin, pin_reg_read(pin) & ~SOC_GPIO_CFG_OUTPUT_BIT);
	}

	return 0;
}

static int gpio_bk7258_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	gpio_pin_t pin;

	for (pin = 0; pin < cfg->ngpios; pin++) {
		if ((pins & BIT(pin)) == 0U) {
			continue;
		}
		pin_reg_write(pin, pin_reg_read(pin) ^ SOC_GPIO_CFG_OUTPUT_BIT);
	}

	return 0;
}

/*
 * Interrupts
 */

static int gpio_bk7258_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	struct gpio_bk7258_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t type;
	uint32_t v;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		key = k_spin_lock(&data->lock);
		v = pin_reg_read(pin) & ~SOC_GPIO_CFG_INT_EN_BIT;
		pin_reg_write(pin, v);
		k_spin_unlock(&data->lock, key);
		/* Clear the leftover status too, or re-enabling fires once at once */
		sys_write32(BIT(pin), SOC_GPIO_INT_ST_LO_ADDR);
		return 0;
	}

	/*
	 * Zephyr 4.4's gpio_int_trig has no RISING or FALLING member. Rising and
	 * falling are combinations of mode and trig:
	 * EDGE + HIGH = rising edge EDGE + LOW = falling edge
	 * LEVEL + HIGH = high level LEVEL + LOW = low level
	 * (enum gpio_int_trig in include/zephyr/drivers/gpio.h:786 only has
	 * LOW / HIGH / BOTH / WAKE*. Writing RISING by intuition does not even
	 * compile.)
	 */
	if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig == GPIO_INT_TRIG_LOW) {
			type = SOC_GPIO_INT_TYPE_LOW_LEVEL;
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			type = SOC_GPIO_INT_TYPE_HIGH_LEVEL;
		} else {
			return -ENOTSUP;
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_HIGH) {
			type = SOC_GPIO_INT_TYPE_RISING_EDGE;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			type = SOC_GPIO_INT_TYPE_FALLING_EDGE;
		} else {
			/* 2-bit type field: both edges is out of reach */
			return -ENOTSUP;
		}
	} else {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	v = pin_reg_read(pin);
	v &= ~(0x3U << SOC_GPIO_CFG_INT_TYPE_SHIFT);
	v |= (type << SOC_GPIO_CFG_INT_TYPE_SHIFT);
	/*
	 * Turn input_en on as well: edge detection depends on the input sampler.
	 * This is the other face of the same hardware fact behind "output mode
	 * needs input_en=1 too" above: without sampling there is neither a
	 * readable level nor an edge.
	 */
	v |= SOC_GPIO_CFG_INPUT_EN_BIT;
	pin_reg_write(pin, v);

	/* Clear the leftover first, then enable: the other order yields a
	 * spurious interrupt.
	 */
	sys_write32(BIT(pin), SOC_GPIO_INT_ST_LO_ADDR);

	v = pin_reg_read(pin) | SOC_GPIO_CFG_INT_EN_BIT;
	pin_reg_write(pin, v);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_bk7258_manage_callback(const struct device *dev, struct gpio_callback *cb, bool set)
{
	struct gpio_bk7258_data *data = dev->data;

	return gpio_manage_callback(&data->cb, cb, set);
}

static uint32_t gpio_bk7258_get_pending_int(const struct device *dev)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	uint32_t st = sys_read32(SOC_GPIO_INT_ST_LO_ADDR);

	return st & ((cfg->ngpios >= 32U) ? 0xFFFFFFFFU : (BIT(cfg->ngpios) - 1U));
}

/*
 * ISR, structured like the vendor gpio_isr(): read status, clear status, then
 * fire the callbacks pin by pin.
 *
 * Why clear before calling back: the vendor comment says
 * "clear IRQ status and doesn't disable IRQ, then it causes a new ISR"
 * For a level-triggered interrupt, calling back first and clearing afterwards
 * leaves the level asserted when the callback returns, so the NVIC re-enters
 * immediately and the handler storms. Clearing first shrinks that window.
 */
static void gpio_bk7258_isr(const struct device *dev)
{
	struct gpio_bk7258_data *data = dev->data;
	uint32_t st = sys_read32(SOC_GPIO_INT_ST_LO_ADDR);
	gpio_pin_t pin;

	if (st == 0U) {
		return; /* shared line or glitch: ignore */
	}

	/* (1) status register is write-1-to-clear */
	sys_write32(st, SOC_GPIO_INT_ST_LO_ADDR);

	/* (2) clear again per pin (what the vendor bk_gpio_clear_interrupt does) */
	for (pin = 0U; (st != 0U) && (pin < 32U); pin++) {
		if ((st & BIT(pin)) != 0U) {
			pin_reg_write(pin, pin_reg_read(pin) | SOC_GPIO_CFG_INT_CLEAR_BIT);
		}
	}

	gpio_fire_callbacks(&data->cb, dev, st);
}

/*
 * Initialisation
 */

static void gpio_bk7258_irq_cfg_0(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gpio_bk7258_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}

static int gpio_bk7258_init(const struct device *dev)
{
	const struct gpio_bk7258_config *cfg = dev->config;
	uint32_t v;

	/*
	 * (1) The second-level interrupt enable. Without it a GPIO interrupt
	 * never reaches the CPU, with no error at all. The SYSTEM domain is
	 * shared with the system controller, so this has to be a read-modify-write.
	 * Bit number = IRQ - 32 = 55 - 32 = 23, matching
	 * SYS_CPU0_INT_32_63_EN_CPU0_GPIO_INT_EN_POS(23) in sys_types.h.
	 */
	v = sys_read32(SOC_SYS_CPU1_INT_32_63_EN_ADDR);
	v |= BIT(DT_INST_IRQN(0) - 32U);
	sys_write32(v, SOC_SYS_CPU1_INT_32_63_EN_ADDR);

	/* (2) Drop the interrupt status left over from power-up or a deep-sleep
	 * wake-up. The AON domain keeps those bits across reset, so without
	 * this the ISR reports a false interrupt on entry.
	 */
	sys_write32(0xFFFFFFFFU, SOC_GPIO_INT_ST_LO_ADDR);
	sys_write32(0xFFFFFFFFU, SOC_GPIO_INT_ST_HI_ADDR);

	/* (3) Connect the NVIC last, same reason as the vendor's
	 * "Move ISR to last" comment. Only the instance that owns the interrupt
	 * configures it; the second controller over the same register file has
	 * none and leaves the handler alone.
	 */
	if (cfg->irq_cfg_func != NULL) {
		cfg->irq_cfg_func();
	}

	return 0;
}

/*
 * Driver instance
 */

static DEVICE_API(gpio, gpio_bk7258_driver) = {
	.pin_configure = gpio_bk7258_pin_configure,
	.port_get_raw = gpio_bk7258_port_get_raw,
	.port_set_masked_raw = gpio_bk7258_port_set_masked_raw,
	.port_set_bits_raw = gpio_bk7258_port_set_bits_raw,
	.port_clear_bits_raw = gpio_bk7258_port_clear_bits_raw,
	.port_toggle_bits = gpio_bk7258_port_toggle_bits,
	.pin_interrupt_configure = gpio_bk7258_pin_interrupt_configure,
	.manage_callback = gpio_bk7258_manage_callback,
	.get_pending_int = gpio_bk7258_get_pending_int,
};

/*
 * One instance per controller node. The chip has one interrupt for all of its
 * pins and two devices cannot register the same handler, so only the first
 * instance configures the interrupt; the second shares the register file and
 * is used for pin-level work on the pins the first one does not own.
 */
#define GPIO_BK7258_INIT(inst)                                                                     \
	static const struct gpio_bk7258_config gpio_bk7258_cfg##inst = {                           \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.ngpios = DT_INST_PROP(inst, ngpios),                                              \
		.irq_cfg_func = (inst == 0) ? gpio_bk7258_irq_cfg_0 : NULL,                        \
	};                                                                                         \
	static struct gpio_bk7258_data gpio_bk7258_data##inst;                                     \
	DEVICE_DT_INST_DEFINE(inst, gpio_bk7258_init, NULL, &gpio_bk7258_data##inst,               \
			      &gpio_bk7258_cfg##inst, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,     \
			      &gpio_bk7258_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_BK7258_INIT)
