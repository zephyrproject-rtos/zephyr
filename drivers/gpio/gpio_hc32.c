/*
 * Copyright (C) 2024-2025, Xiaohua Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT xhsc_hc32_gpio

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include "gpio_hc32.h"
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/intc_hc32.h>

/**
 * @brief Common GPIO driver for HC32 MCUs.
 */

/**
 * @brief EXTI interrupt callback
 */
static void gpio_hc32_isr(int line, void *arg)
{
	struct gpio_hc32_data *data = arg;
	EXTINT_ClearExtIntStatus(BIT(line));
	gpio_fire_callbacks(&data->cb, data->dev, BIT(line));
}

/**
 * @brief Configure pin or port
 */
static int gpio_hc32_configure(const struct device *port, gpio_pin_t pin,
			       gpio_flags_t flags)
{
	const struct gpio_hc32_config *cfg = port->config;;
	uint8_t hc32_port = cfg->port;
	stc_gpio_init_t stc_gpio_init;

	GPIO_StructInit(&stc_gpio_init);

	/* GPIO input/output configuration flags */
	if ((flags & GPIO_OUTPUT) != 0U) {
		/* ouput */
		stc_gpio_init.u16PinDir = PIN_DIR_OUT;
		if ((flags & GPIO_SINGLE_ENDED) != 0) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				stc_gpio_init.u16PinOutputType = PIN_OUT_TYPE_NMOS;
			} else  {
				/* Output can't be open source */
				return -ENOTSUP;
			}
		} else {
			stc_gpio_init.u16PinOutputType = PIN_OUT_TYPE_CMOS;
		}

		if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
			/* No pull up or down in out mode*/
			return -ENOTSUP;
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			stc_gpio_init.u16PinState = PIN_STAT_SET;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			stc_gpio_init.u16PinState = PIN_STAT_RST;
		} else if ((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0U) {
			/* Don't support logical set */
			return -ENOTSUP;
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		/* Input */
		stc_gpio_init.u16PinDir = PIN_DIR_IN;

		if ((flags & GPIO_PULL_UP) != 0) {
			stc_gpio_init.u16PullUp = PIN_PU_ON;
		} else if ((flags & GPIO_PULL_DOWN) != 0) {
			/* No pull down */
			return -ENOTSUP;
		}
	} else {
		/* Deactivated: Analog */
		stc_gpio_init.u16PinAttr = PIN_ATTR_ANALOG;
	}

	/* GPIO interrupt configuration flags */
	if ((flags & GPIO_INT_MASK) != 0) {
		if ((flags & GPIO_INT_ENABLE) != 0) {
			stc_gpio_init.u16ExtInt = PIN_EXTINT_ON;
		} else if ((flags & GPIO_INT_DISABLE) != 0) {
			stc_gpio_init.u16ExtInt = PIN_EXTINT_OFF;
		}

		if ((flags & GPIO_INT_EDGE_BOTH) == GPIO_INT_EDGE_BOTH) {
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_BOTH);
		} else if ((flags & GPIO_INT_EDGE_RISING) == GPIO_INT_EDGE_RISING) {
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_RISING);
		} else if ((flags & GPIO_INT_EDGE_FALLING) == GPIO_INT_EDGE_FALLING) {
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_FALLING);
		} else if ((flags & GPIO_INT_LEVEL_LOW) == GPIO_INT_LEVEL_LOW) {
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_LOW_LVL);
		} else if (((flags & GPIO_INT_LEVEL_HIGH) == GPIO_INT_LEVEL_HIGH)
			   || ((flags & GPIO_INT_LEVELS_LOGICAL) != 0)) {
			/* Not support hight level and logical level set for int */
			stc_gpio_init.u16ExtInt = PIN_EXTINT_OFF;
			return -ENOTSUP;
		}
	}

	return GPIO_Init(hc32_port, BIT(pin), &stc_gpio_init);
}

#if defined(CONFIG_GPIO_GET_CONFIG)
static int gpio_hc32_get_config(const struct device *port, gpio_pin_t pin,
				gpio_flags_t *flags)
{
	const struct gpio_hc32_config *cfg = port->config;;
	uint16_t PCR_value = *(cfg->base + pin * 4);
	gpio_flags_t hc32_flag = 0;

	/* only support input/output cfg */
	if ((PCR_value & GPIO_PCR_DDIS) == 0U) {
		if (((PCR_value) & GPIO_PCR_POUTE) != 0U) {
			hc32_flag |= GPIO_OUTPUT;
			if ((PCR_value & GPIO_PCR_POUT) != 0U) {
				hc32_flag |= GPIO_OUTPUT_INIT_HIGH;
			} else {
				hc32_flag |= GPIO_OUTPUT_INIT_LOW;
			}
		} else {
			hc32_flag |= GPIO_INPUT;
			if (((PCR_value) & GPIO_PCR_PUU) != 0U) {
				hc32_flag |= GPIO_PULL_UP;
			}
		}
	} else {
		hc32_flag &= ~(GPIO_OUTPUT | GPIO_INPUT);
	}

	if ((PCR_value & GPIO_PCR_NOD) != 0U) {
		hc32_flag |= GPIO_OPEN_DRAIN;
	}

	*flags = hc32_flag;
	return 0;
}
#endif

static int gpio_hc32_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;

	*value = (uint32_t)GPIO_ReadInputPort(port);

	return 0;
}

static int gpio_hc32_port_set_masked_raw(const struct device *dev,
					 gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;
	GPIO_WritePort(port, mask);
	return 0;
}

static int gpio_hc32_port_set_bits_raw(const struct device *dev,
				       gpio_port_pins_t pins)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;
	GPIO_SetPins(port, pins);
	return 0;
}

static int gpio_hc32_port_clear_bits_raw(const struct device *dev,
					 gpio_port_pins_t pins)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;
	GPIO_ResetPins(port, pins);
	return 0;
}

static int gpio_hc32_port_toggle_bits(const struct device *dev,
				      gpio_port_pins_t pins)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;
	GPIO_TogglePins(port, pins);
	return 0;
}

static int gpio_hc32_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;
	struct gpio_hc32_data *data = dev->data;
	int err = 0;

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		hc32_extint_disable(port, pin);
		return 0;
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		hc32_extint_enable(port, pin);
		return 0;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	switch (mode) {
	case GPIO_INT_DISABLE:
		hc32_extint_disable(port, pin);
		hc32_extint_unset_callback(pin);
		hc32_extint_trigger(pin, HC32_EXTINT_TRIG_FALLING);
		return 0;
		break;

	case GPIO_INT_MODE_LEVEL:
		if (GPIO_INT_TRIG_LOW == trig) {
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_LOW_LVL);
		} else {
			return -ENOTSUP;
		}
		break;

	case GPIO_INT_MODE_EDGE:
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_FALLING);
			break;
		case GPIO_INT_TRIG_HIGH:
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_RISING);
			break;
		case GPIO_INT_TRIG_BOTH:
			hc32_extint_trigger(pin, HC32_EXTINT_TRIG_BOTH);
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	err = hc32_extint_set_callback(pin, gpio_hc32_isr, data);
	hc32_extint_enable(port, pin);
	return err;
}

static int gpio_hc32_manage_callback(const struct device *dev,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_hc32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static uint32_t gpio_hc32_get_pending_int(const struct device *dev)
{
	return 0;
}

static const struct gpio_driver_api gpio_hc32_driver = {
	.pin_configure = gpio_hc32_configure,
#if defined(CONFIG_GPIO_GET_CONFIG)
	.pin_get_config = gpio_hc32_get_config,
#endif /* CONFIG_GPIO_GET_CONFIG */
	.port_get_raw = gpio_hc32_port_get_raw,
	.port_set_masked_raw = gpio_hc32_port_set_masked_raw,
	.port_set_bits_raw = gpio_hc32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_hc32_port_clear_bits_raw,
	.port_toggle_bits = gpio_hc32_port_toggle_bits,
	.pin_interrupt_configure = gpio_hc32_pin_interrupt_configure,
	.manage_callback = gpio_hc32_manage_callback,
	.get_pending_int = gpio_hc32_get_pending_int,
};


static int gpio_hc32_init(const struct device *dev)
{
	struct gpio_hc32_data *data = dev->data;

	data->dev = dev;
	return 0;
}

#define GPIO_HC32_DEFINE(n) \
	static const struct gpio_hc32_config gpio_hc32_cfg_##n = {	\
		.common = {						     \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),\
		},														\
		.base =	(uint16_t*)DT_INST_REG_ADDR(n),						\
		.port = n, 			\
	};\
	static struct gpio_hc32_data gpio_hc32_data_##n;	\
	DEVICE_DT_INST_DEFINE(n, gpio_hc32_init, NULL,     \
	        &gpio_hc32_data_##n, &gpio_hc32_cfg_##n,     \
	        POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,     \
	        &gpio_hc32_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_HC32_DEFINE)
