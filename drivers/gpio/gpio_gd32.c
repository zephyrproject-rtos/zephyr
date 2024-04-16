/*
 * Copyright (c) 2021 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/interrupt_controller/gd32_exti.h>
#include <zephyr/drivers/reset.h>

#include <gd32_gpio.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#ifdef CONFIG_GD32_HAS_AF_PINMUX
/** SYSCFG DT node */
#define SYSCFG_NODE DT_NODELABEL(syscfg)
#else
/** AFIO DT node */
#define AFIO_NODE DT_NODELABEL(afio)

/** GPIO mode: analog (CTL bits) */
#define CTL_MODE_ANALOG 0x0U
/** GPIO mode: input floating (CTL bits) */
#define CTL_MODE_INP_FLOAT 0x4U
/** GPIO mode: input with pull-up/down (CTL bits) */
#define CTL_MODE_INP_PUPD 0x8U
/** GPIO mode: output push-pull @ 2MHz (CTL bits) */
#define CTL_MODE_OUT_PP 0x2U
/** GPIO mode: output open-drain @ 2MHz (CTL bits) */
#define CTL_MODE_OUT_OD 0x6U
#endif /* CONFIG_GD32_HAS_AF_PINMUX */

/** EXTISS mask */
#define EXTISS_MSK 0xFU
/** EXTISS line step size */
#define EXTISS_STEP 4U
/** EXTISS line shift */
#define EXTISS_LINE_SHIFT(pin) (EXTISS_STEP * ((pin) % EXTISS_STEP))

struct gpio_gd32_config {
	struct gpio_driver_config common;
	uint32_t reg;
	uint16_t clkid;
	uint16_t clkid_exti;
	struct reset_dt_spec reset;
};

struct gpio_gd32_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

/**
 * @brief EXTI ISR callback.
 *
 * @param line EXTI line (equals to GPIO pin number).
 * @param arg GPIO port instance.
 */
static void gpio_gd32_isr(uint8_t line, void *arg)
{
	const struct device *dev = arg;
	struct gpio_gd32_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(line));
}

/**
 * @brief Configure EXTI source selection register.
 *
 * @param port GPIO port instance.
 * @param pin GPIO pin number.
 *
 * @retval 0 on success.
 * @retval -EINVAL if pin is not valid.
 */
static int gpio_gd32_configure_extiss(const struct device *port,
				      gpio_pin_t pin)
{
	const struct gpio_gd32_config *config = port->config;
	uint8_t port_index, shift;
	volatile uint32_t *extiss;

	switch (pin / EXTISS_STEP) {
#ifdef CONFIG_GD32_HAS_AF_PINMUX
	case 0U:
		extiss = &SYSCFG_EXTISS0;
		break;
	case 1U:
		extiss = &SYSCFG_EXTISS1;
		break;
	case 2U:
		extiss = &SYSCFG_EXTISS2;
		break;
	case 3U:
		extiss = &SYSCFG_EXTISS3;
		break;
#else
	case 0U:
		extiss = &AFIO_EXTISS0;
		break;
	case 1U:
		extiss = &AFIO_EXTISS1;
		break;
	case 2U:
		extiss = &AFIO_EXTISS2;
		break;
	case 3U:
		extiss = &AFIO_EXTISS3;
		break;
#endif /* CONFIG_GD32_HAS_AF_PINMUX */
	default:
		return -EINVAL;
	}

	port_index = (config->reg - GPIOA) / (GPIOB - GPIOA);
	shift = EXTISS_LINE_SHIFT(pin);

	*extiss &= ~(EXTISS_MSK << shift);
	*extiss |= port_index << shift;

	return 0;
}

static inline int gpio_gd32_configure(const struct device *port, gpio_pin_t pin,
				      gpio_flags_t flags)
{
	const struct gpio_gd32_config *config = port->config;

#ifdef CONFIG_GD32_HAS_AF_PINMUX
	uint32_t ctl, pupd;

	ctl = GPIO_CTL(config->reg);
	ctl &= ~GPIO_MODE_MASK(pin);

	pupd = GPIO_PUD(config->reg);
	pupd &= ~GPIO_PUPD_MASK(pin);

	if ((flags & GPIO_OUTPUT) != 0U) {
		ctl |= GPIO_MODE_SET(pin, GPIO_MODE_OUTPUT);

		if ((flags & GPIO_SINGLE_ENDED) != 0U) {
			if ((flags & GPIO_LINE_OPEN_DRAIN) != 0U) {
				GPIO_OMODE(config->reg) |= BIT(pin);
			} else {
				return -ENOTSUP;
			}
		} else {
			GPIO_OMODE(config->reg) &= ~BIT(pin);
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			GPIO_BOP(config->reg) = BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			GPIO_BC(config->reg) = BIT(pin);
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		ctl |= GPIO_MODE_SET(pin, GPIO_MODE_INPUT);
	} else {
		ctl |= GPIO_MODE_SET(pin, GPIO_MODE_ANALOG);
	}

	if ((flags & GPIO_PULL_UP) != 0U) {
		pupd |= GPIO_PUPD_SET(pin, GPIO_PUPD_PULLUP);
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		pupd |= GPIO_PUPD_SET(pin, GPIO_PUPD_PULLDOWN);
	} else {
		pupd |= GPIO_PUPD_SET(pin, GPIO_PUPD_NONE);
	}

	GPIO_PUD(config->reg) = pupd;
	GPIO_CTL(config->reg) = ctl;
#else
	volatile uint32_t *ctl_reg;
	uint32_t ctl, pin_bit;

	pin_bit = BIT(pin);

	if (pin < 8U) {
		ctl_reg = &GPIO_CTL0(config->reg);
	} else {
		ctl_reg = &GPIO_CTL1(config->reg);
		pin -= 8U;
	}

	ctl = *ctl_reg;
	ctl &= ~GPIO_MODE_MASK(pin);

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_SINGLE_ENDED) != 0U) {
			if ((flags & GPIO_LINE_OPEN_DRAIN) != 0U) {
				ctl |= GPIO_MODE_SET(pin, CTL_MODE_OUT_OD);
			} else {
				return -ENOTSUP;
			}
		} else {
			ctl |= GPIO_MODE_SET(pin, CTL_MODE_OUT_PP);
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			GPIO_BOP(config->reg) = pin_bit;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			GPIO_BC(config->reg) = pin_bit;
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		if ((flags & GPIO_PULL_UP) != 0U) {
			ctl |= GPIO_MODE_SET(pin, CTL_MODE_INP_PUPD);
			GPIO_BOP(config->reg) = pin_bit;
		} else if ((flags & GPIO_PULL_DOWN) != 0U) {
			ctl |= GPIO_MODE_SET(pin, CTL_MODE_INP_PUPD);
			GPIO_BC(config->reg) = pin_bit;
		} else {
			ctl |= GPIO_MODE_SET(pin, CTL_MODE_INP_FLOAT);
		}
	} else {
		ctl |= GPIO_MODE_SET(pin, CTL_MODE_ANALOG);
	}

	*ctl_reg = ctl;
#endif /* CONFIG_GD32_HAS_AF_PINMUX */

	return 0;
}

static int gpio_gd32_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_gd32_config *config = port->config;

	*value = GPIO_ISTAT(config->reg);

	return 0;
}

static int gpio_gd32_port_set_masked_raw(const struct device *port,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_gd32_config *config = port->config;

	GPIO_OCTL(config->reg) =
		(GPIO_OCTL(config->reg) & ~mask) | (value & mask);

	return 0;
}

static int gpio_gd32_port_set_bits_raw(const struct device *port,
				       gpio_port_pins_t pins)
{
	const struct gpio_gd32_config *config = port->config;

	GPIO_BOP(config->reg) = pins;

	return 0;
}

static int gpio_gd32_port_clear_bits_raw(const struct device *port,
					 gpio_port_pins_t pins)
{
	const struct gpio_gd32_config *config = port->config;

	GPIO_BC(config->reg) = pins;

	return 0;
}

static int gpio_gd32_port_toggle_bits(const struct device *port,
				      gpio_port_pins_t pins)
{
	const struct gpio_gd32_config *config = port->config;

#ifdef CONFIG_GD32_HAS_AF_PINMUX
	GPIO_TG(config->reg) = pins;
#else
	GPIO_OCTL(config->reg) ^= pins;
#endif /* CONFIG_GD32_HAS_AF_PINMUX */

	return 0;
}

static int gpio_gd32_pin_interrupt_configure(const struct device *port,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	if (mode == GPIO_INT_MODE_DISABLED) {
		gd32_exti_disable(pin);
		(void)gd32_exti_configure(pin, NULL, NULL);
		gd32_exti_trigger(pin, GD32_EXTI_TRIG_NONE);
	} else if (mode == GPIO_INT_MODE_EDGE) {
		int ret;

		ret = gd32_exti_configure(pin, gpio_gd32_isr, (void *)port);
		if (ret < 0) {
			return ret;
		}

		ret = gpio_gd32_configure_extiss(port, pin);
		if (ret < 0) {
			return ret;
		}

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			gd32_exti_trigger(pin, GD32_EXTI_TRIG_FALLING);
			break;
		case GPIO_INT_TRIG_HIGH:
			gd32_exti_trigger(pin, GD32_EXTI_TRIG_RISING);
			break;
		case GPIO_INT_TRIG_BOTH:
			gd32_exti_trigger(pin, GD32_EXTI_TRIG_BOTH);
			break;
		default:
			return -ENOTSUP;
		}

		gd32_exti_enable(pin);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_gd32_manage_callback(const struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct gpio_gd32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static const struct gpio_driver_api gpio_gd32_api = {
	.pin_configure = gpio_gd32_configure,
	.port_get_raw = gpio_gd32_port_get_raw,
	.port_set_masked_raw = gpio_gd32_port_set_masked_raw,
	.port_set_bits_raw = gpio_gd32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_gd32_port_clear_bits_raw,
	.port_toggle_bits = gpio_gd32_port_toggle_bits,
	.pin_interrupt_configure = gpio_gd32_pin_interrupt_configure,
	.manage_callback = gpio_gd32_manage_callback,
};

static int gpio_gd32_init(const struct device *port)
{
	const struct gpio_gd32_config *config = port->config;

	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&config->clkid);
	(void)clock_control_on(GD32_CLOCK_CONTROLLER,
			       (clock_control_subsys_t)&config->clkid_exti);

	(void)reset_line_toggle_dt(&config->reset);

	return 0;
}

#define GPIO_GD32_DEFINE(n)						       \
	static const struct gpio_gd32_config gpio_gd32_config##n = {	       \
		.common = {						       \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),   \
		},							       \
		.reg = DT_INST_REG_ADDR(n),				       \
		.clkid = DT_INST_CLOCKS_CELL(n, id),			       \
		COND_CODE_1(DT_NODE_HAS_STATUS(SYSCFG_NODE, okay),	       \
			    (.clkid_exti = DT_CLOCKS_CELL(SYSCFG_NODE, id),),  \
			    (.clkid_exti = DT_CLOCKS_CELL(AFIO_NODE, id),))    \
		.reset = RESET_DT_SPEC_INST_GET(n),			       \
	};								       \
									       \
	static struct gpio_gd32_data gpio_gd32_data##n;			       \
									       \
	DEVICE_DT_INST_DEFINE(n, &gpio_gd32_init, NULL, &gpio_gd32_data##n,    \
			      &gpio_gd32_config##n, PRE_KERNEL_1,	       \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_gd32_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_GD32_DEFINE)
