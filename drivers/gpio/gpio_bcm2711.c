/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT brcm_bcm2711_gpio

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#define GPIO_REG_GROUP(n, cnt)       (n / cnt)
#define GPIO_REG_SHIFT(n, cnt, bits) ((n % cnt) * bits)

#define GPFSEL(base, n) (base + 0x00 + 0x04 * n)
#define GPSET(base, n)  (base + 0x1C + 0x04 * n)
#define GPCLR(base, n)  (base + 0x28 + 0x04 * n)
#define GPLEV(base, n)  (base + 0x34 + 0x04 * n)
#define GPEDS(base, n)  (base + 0x40 + 0x04 * n)
#define GPREN(base, n)  (base + 0x4C + 0x04 * n)
#define GPFEN(base, n)  (base + 0x58 + 0x04 * n)
#define GPHEN(base, n)  (base + 0x64 + 0x04 * n)
#define GPLEN(base, n)  (base + 0x70 + 0x04 * n)
#define GPAREN(base, n) (base + 0x7C + 0x04 * n)
#define GPAFEN(base, n) (base + 0x88 + 0x04 * n)
#define GPPULL(base, n) (base + 0xE4 + 0x04 * n)

/* Legacy pull-control regs (BCM2835 / 2836 / 2837 / 2710) -- the
 * modern GPPULL register at 0xE4 is reserved on these chips.
 */
#define GPPUD(base)        ((base) + 0x94)
#define GPPUDCLK(base, n)  ((base) + 0x98 + 0x04 * (n))

#define LEGACY_PULL_OFF	0x0
#define LEGACY_PULL_DOWN	0x1
#define LEGACY_PULL_UP	0x2

#define FSEL_GROUPS (10)
#define FSEL_BITS   (3)
#define FSEL_OUTPUT (0x1)

#define IO_GROUPS (32)
#define IO_BITS   (1)

#define PULL_GROUPS (16)
#define PULL_BITS   (2)
#define PULL_UP     (0x1)
#define PULL_DOWN   (0x2)

#define DEV_CFG(dev)  ((const struct gpio_bcm2711_config *const)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_bcm2711_data *const)(dev)->data)

#define RPI_PIN_NUM(dev, n) (DEV_CFG(dev)->offset + n)

#define FROM_U64(val, idx) ((uint32_t)((val >> (idx * 32)) & UINT32_MAX))

struct gpio_bcm2711_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);

	void (*irq_config_func)(void);

	uint8_t offset;
	uint8_t ngpios;
	bool legacy_pull;
};

struct gpio_bcm2711_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	mem_addr_t base;

	sys_slist_t cb;
};

static int gpio_bcm2711_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint32_t group;
	uint32_t shift;
	uint32_t regval;

	if (flags & GPIO_OPEN_DRAIN) {
		return -ENOTSUP;
	}

	/* Set direction */
	{
		group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), FSEL_GROUPS);
		shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), FSEL_GROUPS, FSEL_BITS);

		regval = sys_read32(GPFSEL(data->base, group));
		regval &= ~(BIT_MASK(FSEL_BITS) << shift);
		if (flags & GPIO_OUTPUT) {
			regval |= (FSEL_OUTPUT << shift);
		}
		sys_write32(regval, GPFSEL(data->base, group));
	}

	/* Set output level */
	if (flags & GPIO_OUTPUT) {
		group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), IO_GROUPS);
		shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), IO_GROUPS, IO_BITS);

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			regval = sys_read32(GPSET(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPSET(data->base, group));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			regval = sys_read32(GPCLR(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPCLR(data->base, group));
		}
	}

	/* Set pull. Two register layouts -- BCM2711 has a single
	 * GPPULL/PUP_PDN_CNTRL_REG at offset 0xE4 (2 bits per pin); the
	 * BCM2835 family (used in Pi 1..3 / Pi Zero / Pi Zero 2 W) has
	 * a legacy GPPUD/GPPUDCLK0/GPPUDCLK1 sequence at 0x94/0x98/0x9C.
	 * Branch by the binding's legacy-pull-control flag.
	 */
	if (DEV_CFG(port)->legacy_pull) {
		uint32_t pin_num = RPI_PIN_NUM(port, pin);
		uint32_t pud;

		if (flags & GPIO_PULL_UP) {
			pud = LEGACY_PULL_UP;
		} else if (flags & GPIO_PULL_DOWN) {
			pud = LEGACY_PULL_DOWN;
		} else {
			pud = LEGACY_PULL_OFF;
		}

		/* The BCM2835 datasheet's recommended sequence:
		 *   1. write GPPUD with the pull mode
		 *   2. wait >= 150 cycles for the value to set up
		 *   3. write GPPUDCLK with a 1 in the bit for the target pin
		 *   4. wait >= 150 cycles for the clock pulse to register
		 *   5. write GPPUD = 0 and GPPUDCLK = 0
		 *
		 * 150 cycles at the (up to) 250 MHz core clock is < 1 us;
		 * k_busy_wait(1) is the smallest available wait and rounds
		 * up to ~1 us. Plenty.
		 */
		sys_write32(pud, GPPUD(data->base));
		k_busy_wait(1);
		sys_write32(BIT(pin_num & 0x1F),
			    GPPUDCLK(data->base, pin_num >> 5));
		k_busy_wait(1);
		sys_write32(0, GPPUD(data->base));
		sys_write32(0, GPPUDCLK(data->base, pin_num >> 5));
	} else {
		group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), PULL_GROUPS);
		shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), PULL_GROUPS,
				       PULL_BITS);

		regval = sys_read32(GPPULL(data->base, group));
		regval &= ~(BIT_MASK(PULL_BITS) << shift);
		if (flags & GPIO_PULL_UP) {
			regval |= (PULL_UP << shift);
		} else if (flags & GPIO_PULL_DOWN) {
			regval |= (PULL_DOWN << shift);
		}
		sys_write32(regval, GPPULL(data->base, group));
	}

	return 0;
}

static int gpio_bcm2711_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint64_t regval;

	regval = ((uint64_t)sys_read32(GPLEV(data->base, 0))) |
		 ((uint64_t)sys_read32(GPLEV(data->base, 1)) << 32);

	*value = (regval >> cfg->offset) & BIT_MASK(cfg->ngpios);

	return 0;
}

static int gpio_bcm2711_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint64_t regval, regmask;
	uint64_t set, clr;

	value &= BIT_MASK(cfg->ngpios);
	mask &= BIT_MASK(cfg->ngpios);

	regval = (uint64_t)value << cfg->offset;
	regmask = (uint64_t)mask << cfg->offset;

	set = regval & regmask;
	clr = regval ^ regmask;

	sys_write32(FROM_U64(set, 0), GPSET(data->base, 0));
	sys_write32(FROM_U64(clr, 0), GPCLR(data->base, 0));
	sys_write32(FROM_U64(set, 1), GPSET(data->base, 1));
	sys_write32(FROM_U64(clr, 1), GPCLR(data->base, 1));

	return 0;
}

static int gpio_bcm2711_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint64_t regval;

	regval = ((uint64_t)pins & BIT_MASK(cfg->ngpios)) << cfg->offset;

	sys_write32(FROM_U64(regval, 0), GPSET(data->base, 0));
	sys_write32(FROM_U64(regval, 1), GPSET(data->base, 1));

	return 0;
}

static int gpio_bcm2711_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint64_t regval;

	regval = ((uint64_t)pins & BIT_MASK(cfg->ngpios)) << cfg->offset;

	sys_write32(FROM_U64(regval, 0), GPCLR(data->base, 0));
	sys_write32(FROM_U64(regval, 1), GPCLR(data->base, 1));

	return 0;
}

static int gpio_bcm2711_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint64_t regval, regmask;
	uint64_t set, clr;

	regval = ((uint64_t)sys_read32(GPLEV(data->base, 0))) |
		 ((uint64_t)sys_read32(GPLEV(data->base, 1)) << 32);

	regmask = ((uint64_t)pins & BIT_MASK(cfg->ngpios)) << cfg->offset;

	set = regval ^ regmask;
	clr = regval & regmask;

	sys_write32(FROM_U64(set, 0), GPSET(data->base, 0));
	sys_write32(FROM_U64(clr, 0), GPCLR(data->base, 0));
	sys_write32(FROM_U64(set, 1), GPSET(data->base, 1));
	sys_write32(FROM_U64(clr, 1), GPCLR(data->base, 1));

	return 0;
}

static int gpio_bcm2711_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint32_t group;
	uint32_t shift;
	uint32_t regval;

	group = GPIO_REG_GROUP(RPI_PIN_NUM(port, pin), IO_GROUPS);
	shift = GPIO_REG_SHIFT(RPI_PIN_NUM(port, pin), IO_GROUPS, IO_BITS);

	/* Clear all detections first */

	regval = sys_read32(GPREN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPREN(data->base, group));

	regval = sys_read32(GPFEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPFEN(data->base, group));

	regval = sys_read32(GPHEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPHEN(data->base, group));

	regval = sys_read32(GPLEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPLEN(data->base, group));

	regval = sys_read32(GPAREN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPAREN(data->base, group));

	regval = sys_read32(GPAFEN(data->base, group));
	regval &= ~BIT(shift);
	sys_write32(regval, GPAFEN(data->base, group));

	/* Clear any pending event left from a prior detector mode, so the
	 * first ISR after reconfigure reflects a real edge -- not a stale
	 * one (especially important for EDGE_BOTH, where a stale latch
	 * would consume one of the two edges the caller expects to see).
	 */
	sys_write32(BIT(shift), GPEDS(data->base, group));

	if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig & GPIO_INT_LOW_0) {
			regval = sys_read32(GPLEN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPLEN(data->base, group));
		}
		if (trig & GPIO_INT_HIGH_1) {
			regval = sys_read32(GPHEN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPHEN(data->base, group));
		}
	} else if (mode == GPIO_INT_MODE_EDGE) {
		if (trig & GPIO_INT_LOW_0) {
			regval = sys_read32(GPAFEN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPAFEN(data->base, group));
		}
		if (trig & GPIO_INT_HIGH_1) {
			regval = sys_read32(GPAREN(data->base, group));
			regval |= BIT(shift);
			sys_write32(regval, GPAREN(data->base, group));
		}
	}

	return 0;
}

static int gpio_bcm2711_manage_callback(const struct device *port, struct gpio_callback *cb,
					bool set)
{
	struct gpio_bcm2711_data *data = DEV_DATA(port);

	return gpio_manage_callback(&data->cb, cb, set);
}

static void gpio_bcm2711_isr(const struct device *port)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);
	uint64_t regval;
	uint32_t pins;

	regval = ((uint64_t)sys_read32(GPEDS(data->base, 0))) |
		 ((uint64_t)sys_read32(GPEDS(data->base, 1)) << 32);

	regval &= BIT_MASK(cfg->ngpios) << cfg->offset;

	/* Clear the latched events before running callbacks: a callback may
	 * itself generate a new edge on a watched pin (e.g. EDGE_BOTH where
	 * the handler toggles the output), and a post-callback W1C write
	 * would race with that new edge.
	 */
	sys_write32(FROM_U64(regval, 0), GPEDS(data->base, 0));
	sys_write32(FROM_U64(regval, 1), GPEDS(data->base, 1));

	pins = (uint32_t)(regval >> cfg->offset);
	gpio_fire_callbacks(&data->cb, port, pins);
}

int gpio_bcm2711_init(const struct device *port)
{
	const struct gpio_bcm2711_config *cfg = DEV_CFG(port);
	struct gpio_bcm2711_data *data = DEV_DATA(port);

	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	data->base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	cfg->irq_config_func();

	return 0;
}

static DEVICE_API(gpio, gpio_bcm2711_api) = {
	.pin_configure = gpio_bcm2711_pin_configure,
	.port_get_raw = gpio_bcm2711_port_get_raw,
	.port_set_masked_raw = gpio_bcm2711_port_set_masked_raw,
	.port_set_bits_raw = gpio_bcm2711_port_set_bits_raw,
	.port_clear_bits_raw = gpio_bcm2711_port_clear_bits_raw,
	.port_toggle_bits = gpio_bcm2711_port_toggle_bits,
	.pin_interrupt_configure = gpio_bcm2711_pin_interrupt_configure,
	.manage_callback = gpio_bcm2711_manage_callback,
};

#define GPIO_BCM2711_INST(n)                                                                       \
	static struct gpio_bcm2711_data gpio_bcm2711_data_##n;                                     \
                                                                                                   \
	static void gpio_bcm2711_irq_config_func_##n(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_bcm2711_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct gpio_bcm2711_config gpio_bcm2711_cfg_##n = {                           \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_INST_PARENT(n)),                           \
		.irq_config_func = gpio_bcm2711_irq_config_func_##n,                               \
		.offset = DT_INST_REG_ADDR(n),                                                     \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
		.legacy_pull = DT_INST_PROP(n, legacy_pull_control),                               \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_bcm2711_init, NULL, &gpio_bcm2711_data_##n,                  \
			      &gpio_bcm2711_cfg_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,      \
			      &gpio_bcm2711_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_BCM2711_INST)
