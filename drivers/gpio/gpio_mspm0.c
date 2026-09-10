/*
 * Copyright (c) 2025 Texas Instruments
 * Copyright (c) 2025 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_gpio

/* Zephyr includes */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/mspm0-pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>

#define GPIO_PWREN         0x800  /* GPRCM power enable */
#define GPIO_RSTCTL        0x804  /* GPRCM reset control */
#define GPIO_IIDX          0x1020 /* CPU_INT interrupt index */
#define GPIO_IMASK         0x1028 /* CPU_INT interrupt mask */
#define GPIO_MIS           0x1038 /* CPU_INT masked interrupt status */
#define GPIO_ICLR          0x1048 /* CPU_INT interrupt clear */
#define GPIO_DOUT31_0      0x1280 /* Data output 31 to 0 */
#define GPIO_DOUTSET31_0   0x1290 /* Data output set 31 to 0 */
#define GPIO_DOUTCLR31_0   0x12A0 /* Data output clear 31 to 0 */
#define GPIO_DOUTTGL31_0   0x12B0 /* Data output toggle 31 to 0 */
#define GPIO_DOE31_0       0x12C0 /* Data output enable 31 to 0 */
#define GPIO_DOESET31_0    0x12D0 /* Data output enable set 31 to 0 */
#define GPIO_DOECLR31_0    0x12E0 /* Data output enable clear 31 to 0 */
#define GPIO_DIN31_0       0x1380 /* Data input 31 to 0 */
#define GPIO_POLARITY15_0  0x1390 /* Polarity 15 to 0 */
#define GPIO_POLARITY31_16 0x13A0 /* Polarity 31 to 16 */
#define GPIO_FASTWAKE      0x1404 /* Fast wake enable */

#define GPIO_MSPM0_PWREN_KEY_UNLOCK_W        0x26000000U
#define GPIO_MSPM0_PWREN_ENABLE_ENABLE       0x00000001U
#define GPIO_MSPM0_RSTCTL_KEY_UNLOCK_W       0xB1000000U
#define GPIO_MSPM0_RSTCTL_RESETSTKYCLR_CLR   0x00000002U
#define GPIO_MSPM0_RSTCTL_RESETASSERT_ASSERT 0x00000001U

struct gpio_mspm0_config {
	/* gpio_mspm0_config needs to be first (doesn't actually get used) */
	struct gpio_driver_config common;
	/* port base address */
	mem_addr_t base;
	/*
	 * Per-pin pinmux values (MSP_PINMUX(pincm, MSPM0_PIN_FUNCTION_GPIO)),
	 * dereferenced from the DT "pinmux" phandle list at build time.
	 * A value of 0 marks a non-bonded (package-absent) pin.
	 */
	const uint32_t *pinmux;
	/* number of entries in pinmux[] */
	uint8_t pin_count;
};

struct gpio_mspm0_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks; /* List of interrupt callbacks */
};

/* Pin split: POLARITY15_0 handles pins 0-15, POLARITY31_16 handles pins 16-31 */
#define MSPM0_PINS_LOW_GROUP 16

/* GPIO defines */
#define GPIOA_NODE DT_NODELABEL(gpioa)
#define GPIOB_NODE DT_NODELABEL(gpiob)
#define GPIOC_NODE DT_NODELABEL(gpioc)

static int gpio_mspm0_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_mspm0_config *config = port->config;

	/* Read entire port */
	*value = sys_read32(config->base + GPIO_DIN31_0);

	return 0;
}

static int gpio_mspm0_port_set_masked_raw(const struct device *port, uint32_t mask, uint32_t value)
{
	const struct gpio_mspm0_config *config = port->config;
	uint32_t out = sys_read32(config->base + GPIO_DOUT31_0);

	sys_write32((out & ~mask) | (value & mask), config->base + GPIO_DOUT31_0);

	return 0;
}

static int gpio_mspm0_port_set_bits_raw(const struct device *port, uint32_t mask)
{
	const struct gpio_mspm0_config *config = port->config;

	sys_write32(mask, config->base + GPIO_DOUTSET31_0);

	return 0;
}

static int gpio_mspm0_port_clear_bits_raw(const struct device *port, uint32_t mask)
{
	const struct gpio_mspm0_config *config = port->config;

	sys_write32(mask, config->base + GPIO_DOUTCLR31_0);

	return 0;
}

static int gpio_mspm0_port_toggle_bits(const struct device *port, uint32_t mask)
{
	const struct gpio_mspm0_config *config = port->config;

	sys_write32(mask, config->base + GPIO_DOUTTGL31_0);

	return 0;
}

static int gpio_mspm0_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mspm0_config *cfg = port->config;

	if (pin >= cfg->pin_count) {
		return -EINVAL;
	}

	uint32_t pinmux = cfg->pinmux[pin];

	if (pinmux == 0U) {
		return -EINVAL;
	}

	pinctrl_soc_pin_t p = {.pinmux = pinmux, .iomux = 0};

	if (flags & GPIO_PULL_UP) {
		p.iomux |= BIT(MSP_GPIO_RESISTOR_PULL_UP);
	}
	if (flags & GPIO_PULL_DOWN) {
		p.iomux |= BIT(MSP_GPIO_RESISTOR_PULL_DOWN);
	}

	/* Config pin based on flags */
	switch (flags & (GPIO_INPUT | GPIO_OUTPUT)) {
	case GPIO_INPUT:
		p.iomux |= BIT(MSP_GPIO_INPUT_ENABLE);

		if (flags & GPIO_INT_WAKEUP) {
			sys_write32(sys_read32(cfg->base + GPIO_FASTWAKE) | BIT(pin),
				    cfg->base + GPIO_FASTWAKE);
			p.iomux |= BIT(MSP_GPIO_WAKEUP_ENABLE);
			if (!(flags & GPIO_ACTIVE_LOW)) {
				p.iomux |= BIT(MSP_GPIO_WAKEUP_COMPARE);
			}
		}

		if (pinctrl_configure_pins(&p, 1, PINCTRL_REG_NONE) < 0) {
			return -EINVAL;
		}
		sys_write32(BIT(pin), cfg->base + GPIO_DOECLR31_0);
		break;

	case GPIO_OUTPUT:
		if (flags & GPIO_OPEN_DRAIN) {
			p.iomux |= BIT(MSP_GPIO_OPEN_DRAIN_OUTPUT);
		}

		/* Set initial state */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_mspm0_port_set_bits_raw(port, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_mspm0_port_clear_bits_raw(port, BIT(pin));
		}

		/*
		 * Enable the output driver before connecting the pad via
		 * pinctrl, so the pin reaches its level in a single transition.
		 * Applying the pad config (pull resistor) first, while the driver
		 * is still disabled, would drive an extra edge on the line.
		 */
		sys_write32(BIT(pin), cfg->base + GPIO_DOESET31_0);

		if (pinctrl_configure_pins(&p, 1, PINCTRL_REG_NONE) < 0) {
			return -EINVAL;
		}
		break;

	case GPIO_DISCONNECTED:
		if (pinctrl_configure_pins(&p, 1, PINCTRL_REG_NONE) < 0) {
			return -EINVAL;
		}
		sys_write32(BIT(pin), cfg->base + GPIO_DOECLR31_0);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_mspm0_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_mspm0_config *config = port->config;

	/* Config interrupt */
	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		sys_write32(BIT(pin), config->base + GPIO_ICLR);
		sys_write32(sys_read32(config->base + GPIO_IMASK) & ~BIT(pin),
			    config->base + GPIO_IMASK);
		break;
	case GPIO_INT_MODE_EDGE:
		uint32_t polarity = 0x00;
		uint32_t pol_shift;
		uint32_t pol_val;
		mem_addr_t pol_reg;

		if (trig & GPIO_INT_TRIG_LOW) {
			polarity |= BIT(1);
		}

		if (trig & GPIO_INT_TRIG_HIGH) {
			polarity |= BIT(0);
		}

		if (pin < MSPM0_PINS_LOW_GROUP) {
			pol_reg = config->base + GPIO_POLARITY15_0;
			pol_shift = 2 * pin;
		} else {
			pol_reg = config->base + GPIO_POLARITY31_16;
			pol_shift = 2 * (pin - MSPM0_PINS_LOW_GROUP);
		}

		pol_val = sys_read32(pol_reg) & ~(0x3U << pol_shift);
		sys_write32(pol_val | (polarity << pol_shift), pol_reg);

		sys_write32(BIT(pin), config->base + GPIO_ICLR);
		sys_write32(sys_read32(config->base + GPIO_IMASK) | BIT(pin),
			    config->base + GPIO_IMASK);
		break;
	case GPIO_INT_MODE_LEVEL:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_mspm0_manage_callback(const struct device *port, struct gpio_callback *callback,
				      bool set)
{
	struct gpio_mspm0_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_mspm0_get_pending_int(const struct device *port)
{
	const struct gpio_mspm0_config *config = port->config;

	return sys_read32(config->base + GPIO_IIDX);
}

static void gpio_mspm0_isr(const struct device *port)
{
	struct gpio_mspm0_data *data;
	const struct gpio_mspm0_config *config;
	const struct device *dev_list[] = {
		DEVICE_DT_GET_OR_NULL(GPIOA_NODE),
		DEVICE_DT_GET_OR_NULL(GPIOB_NODE),
		DEVICE_DT_GET_OR_NULL(GPIOC_NODE),
	};

	for (uint8_t i = 0; i < ARRAY_SIZE(dev_list); i++) {
		uint32_t status;

		if (dev_list[i] == NULL) {
			continue;
		}

		data = dev_list[i]->data;
		config = dev_list[i]->config;

		status = sys_read32(config->base + GPIO_MIS);

		sys_write32(status, config->base + GPIO_ICLR);
		if (status != 0) {
			gpio_fire_callbacks(&data->callbacks, dev_list[i], status);
		}
	}
}

static int gpio_mspm0_init(const struct device *dev)
{
	const struct gpio_mspm0_config *cfg = dev->config;
	static bool init_irq = true;

	/* Reset and enable GPIO banks */
	sys_write32(GPIO_MSPM0_RSTCTL_KEY_UNLOCK_W | GPIO_MSPM0_RSTCTL_RESETSTKYCLR_CLR |
			    GPIO_MSPM0_RSTCTL_RESETASSERT_ASSERT,
		    cfg->base + GPIO_RSTCTL);
	sys_write32(GPIO_MSPM0_PWREN_KEY_UNLOCK_W | GPIO_MSPM0_PWREN_ENABLE_ENABLE,
		    cfg->base + GPIO_PWREN);

	/* All the interrupt port share the same irq number, do it once */
	if (init_irq) {
		init_irq = false;

		IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), gpio_mspm0_isr,
			    DEVICE_DT_INST_GET(0), 0);
		irq_enable(DT_INST_IRQN(0));
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_mspm0_pin_get_config(const struct device *port, gpio_pin_t pin,
				     gpio_flags_t *out_flags)
{
	const struct gpio_mspm0_config *config = port->config;

	/* Currently only returns current state and not actually all flags */
	if (BIT(pin) & sys_read32(config->base + GPIO_DOE31_0)) {
		*out_flags = BIT(pin) & sys_read32(config->base + GPIO_DOUT31_0) ? GPIO_OUTPUT_HIGH
										 : GPIO_OUTPUT_LOW;
	} else {
		*out_flags = GPIO_INPUT;
	}

	return 0;
}
#endif

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_mspm0_port_get_direction(const struct device *port, gpio_port_pins_t map,
					 gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_mspm0_config *config = port->config;
	uint32_t doe = sys_read32(config->base + GPIO_DOE31_0);

	map &= config->common.port_pin_mask;
	*inputs = map & ~doe;
	*outputs = map & doe;

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static DEVICE_API(gpio, gpio_mspm0_driver_api) = {
	.pin_configure = gpio_mspm0_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_mspm0_pin_get_config,
#endif
	.port_get_raw = gpio_mspm0_port_get_raw,
	.port_set_masked_raw = gpio_mspm0_port_set_masked_raw,
	.port_set_bits_raw = gpio_mspm0_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mspm0_port_clear_bits_raw,
	.port_toggle_bits = gpio_mspm0_port_toggle_bits,
	.pin_interrupt_configure = gpio_mspm0_pin_interrupt_configure,
	.manage_callback = gpio_mspm0_manage_callback,
	.get_pending_int = gpio_mspm0_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_mspm0_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

#define GPIO_MSPM0_PINMUX_ELEM(node_id, prop, idx)                                                 \
	DT_PROP_BY_PHANDLE_IDX(node_id, prop, idx, pinmux),

#define GPIO_DEVICE_INIT(n, __suffix, __base_addr)                                                 \
	static const uint32_t gpio##__suffix##_pinmux[] = {                                        \
		DT_FOREACH_PROP_ELEM(n, pinmux, GPIO_MSPM0_PINMUX_ELEM)};                          \
	static const struct gpio_mspm0_config gpio_mspm0_cfg_##__suffix = {                        \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask =                                                   \
					GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_PROP_LEN(n, pinmux)),    \
			},                                                                         \
		.base = (mem_addr_t)__base_addr,                                                   \
		.pinmux = gpio##__suffix##_pinmux,                                                 \
		.pin_count = DT_PROP_LEN(n, pinmux),                                               \
	};                                                                                         \
	static struct gpio_mspm0_data gpio_mspm0_data_##__suffix;                                  \
	DEVICE_DT_DEFINE(n, gpio_mspm0_init, NULL, &gpio_mspm0_data_##__suffix,                    \
			 &gpio_mspm0_cfg_##__suffix, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,      \
			 &gpio_mspm0_driver_api)

#define GPIO_DEVICE_INIT_MSPM0(__suffix)                                                           \
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix), __suffix,                                   \
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
GPIO_DEVICE_INIT_MSPM0(a);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
GPIO_DEVICE_INIT_MSPM0(b);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioc), okay)
GPIO_DEVICE_INIT_MSPM0(c);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioc), okay) */
