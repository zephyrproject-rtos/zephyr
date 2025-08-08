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
#include <zephyr/drivers/interrupt_controller/intc_extint_hc32.h>

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
 * @brief Calculate the on/off status of the exit interrupt by the given flags
 *
 */
static int gpio_hc32_cfg2ll_intr_on(gpio_flags_t flags)
{
	int ext_int = PIN_EXTINT_OFF;

	if ((flags & GPIO_INT_MASK) != 0) {
		if ((flags & GPIO_INT_ENABLE) != 0) {
			ext_int = PIN_EXTINT_ON;
		}
	}

	return ext_int;
}

/**
 * @brief Calculate interrupt trigger type by the given flags
 *
 */
static int gpio_hc32_cfg2ll_intr_trig(gpio_flags_t flags)
{
	int trigger = HC32_EXTINT_TRIG_NOT_SUPPT;

	if ((flags & GPIO_INT_EDGE_BOTH) == GPIO_INT_EDGE_BOTH) {
		trigger = HC32_EXTINT_TRIG_BOTH;
	} else if ((flags & GPIO_INT_EDGE_RISING) == GPIO_INT_EDGE_RISING) {
		trigger = HC32_EXTINT_TRIG_RISING;
	} else if ((flags & GPIO_INT_EDGE_FALLING) == GPIO_INT_EDGE_FALLING) {
		trigger = HC32_EXTINT_TRIG_FALLING;
	} else if ((flags & GPIO_INT_LEVEL_LOW) == GPIO_INT_LEVEL_LOW) {
		trigger = HC32_EXTINT_TRIG_LOW_LVL;
	} else {
		/* Nothing to do */
	}

	return trigger;
}

/**
 * @brief Calculate gpio output type
 *
 */
static int gpio_hc32_cfg2ll_output_type(gpio_flags_t flags, uint16_t *out_type)
{
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		if (flags & GPIO_LINE_OPEN_DRAIN) {
			*out_type = PIN_OUT_TYPE_NMOS;
		} else {
			/* Output can't be open source */
			return -ENOTSUP;
		}
	} else {
		*out_type = PIN_OUT_TYPE_CMOS;
	}

	return 0;
}

/**
 * @brief Calculate gpio output status
 *
 */
static int gpio_hc32_cfg2ll_output_status(gpio_flags_t flags, uint16_t *out_status)
{
	int ret = 0;

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		*out_status = PIN_STAT_SET;
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		*out_status = PIN_STAT_RST;
	} else {
		if ((flags & GPIO_OUTPUT_INIT_LOGICAL) != 0U) {
			/* Don't support logical set */
			ret = -ENOTSUP;
		}
	}

	return ret;
}

/**
 * @brief Configure pin or port
 */
static int gpio_hc32_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	int intr_trigger;
	const struct gpio_hc32_config *cfg = dev->config;
	const struct gpio_hc32_data *data = (struct gpio_hc32_data *)dev->data;
	const struct device *intc_dev = data->intc_dev;
	const struct hc32_extint_driver_api *extint_api = intc_dev->api;
	uint8_t hc32_port = cfg->port;
	stc_gpio_init_t stc_gpio_init;

	GPIO_StructInit(&stc_gpio_init);

	/* GPIO input/output configuration flags */
	if ((flags & GPIO_OUTPUT) != 0U) {
		/* output */
		stc_gpio_init.u16PinDir = PIN_DIR_OUT;

		if (0 != gpio_hc32_cfg2ll_output_type(flags, &stc_gpio_init.u16PinOutputType)) {
			return -ENOTSUP;
		}

		if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
			/* No pull up or down in out mode*/
			return -ENOTSUP;
		}

		if (0 != gpio_hc32_cfg2ll_output_status(flags, &stc_gpio_init.u16PinState)) {
			/* Don't support logical set */
			return -ENOTSUP;
		}
	} else if ((flags & GPIO_INPUT) != 0) {
		/* Input */
		stc_gpio_init.u16PinDir = PIN_DIR_IN;

		if ((flags & GPIO_PULL_UP) != 0) {
			stc_gpio_init.u16PullUp = PIN_PU_ON;
		} else {
			if ((flags & GPIO_PULL_DOWN) != 0) {
				/* No pull down */
				return -ENOTSUP;
			}
		}
	} else {
		/* Deactivated: Analog */
		stc_gpio_init.u16PinAttr = PIN_ATTR_ANALOG;
	}

	/* GPIO interrupt configuration flags */
	stc_gpio_init.u16ExtInt = gpio_hc32_cfg2ll_intr_on(flags);
	if (PIN_EXTINT_ON == stc_gpio_init.u16ExtInt) {
		intr_trigger = gpio_hc32_cfg2ll_intr_trig(flags);
		if (HC32_EXTINT_TRIG_NOT_SUPPT == intr_trigger) {
			stc_gpio_init.u16ExtInt = PIN_EXTINT_OFF;
			return -ENOTSUP;
		}
		extint_api->extint_set_trigger(intc_dev, pin, intr_trigger);
	}

	return GPIO_Init(hc32_port, BIT(pin), &stc_gpio_init);
}

#if defined(CONFIG_GPIO_GET_CONFIG)
static int gpio_hc32_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint16_t PCR_value = *(cfg->base + pin * 4);
	gpio_flags_t hc32_flag = 0;

	/* only support input/output cfg */
	if ((PCR_value & GPIO_PCR_DDIS) == 0U) {
		if (((PCR_value)&GPIO_PCR_POUTE) != 0U) {
			hc32_flag |= GPIO_OUTPUT;
			if ((PCR_value & GPIO_PCR_POUT) != 0U) {
				hc32_flag |= GPIO_OUTPUT_INIT_HIGH;
			} else {
				hc32_flag |= GPIO_OUTPUT_INIT_LOW;
			}
		} else {
			hc32_flag |= GPIO_INPUT;
			if (((PCR_value)&GPIO_PCR_PUU) != 0U) {
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

static int gpio_hc32_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;

	GPIO_WritePort(port, mask);

	return 0;
}

static int gpio_hc32_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;

	GPIO_SetPins(port, pins);

	return 0;
}

static int gpio_hc32_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;

	GPIO_ResetPins(port, pins);

	return 0;
}

static int gpio_hc32_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_hc32_config *cfg = dev->config;
	uint8_t port = cfg->port;

	GPIO_TogglePins(port, pins);

	return 0;
}

static int gpio_hc32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_hc32_config *cfg = dev->config;
	struct gpio_hc32_data *data = (struct gpio_hc32_data *)dev->data;
	const struct device *intc_dev = data->intc_dev;
	const struct hc32_extint_driver_api *extint_api = intc_dev->api;
	uint8_t port = cfg->port;
	int trigger = 0;
	int err = 0;

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY) {
		extint_api->extint_disable(port, pin);
		return 0;
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		extint_api->extint_enable(port, pin);
		return 0;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	switch (mode) {
	case GPIO_INT_DISABLE:
		extint_api->extint_disable(intc_dev, port, pin);
		extint_api->extint_unset_cb(intc_dev, pin);
		extint_api->extint_set_trigger(intc_dev, pin, HC32_EXTINT_TRIG_FALLING);
		return 0;

	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_LOW) {
			trigger = HC32_EXTINT_TRIG_LOW_LVL;
		} else {
			return -ENOTSUP;
		}
		break;

	case GPIO_INT_MODE_EDGE:
		switch (trig) {
		case GPIO_INT_TRIG_BOTH:
			trigger = HC32_EXTINT_TRIG_BOTH;
			break;
		case GPIO_INT_TRIG_HIGH:
			trigger = HC32_EXTINT_TRIG_RISING;
			break;
		case GPIO_INT_TRIG_LOW:
			/* Default trigger is falling edge */
		default:
			trigger = HC32_EXTINT_TRIG_FALLING;
			break;
		}
		break;

	default:
		break;
	}

	extint_api->extint_set_trigger(intc_dev, pin, trigger);
	err = extint_api->extint_set_cb(intc_dev, pin, gpio_hc32_isr, (void *)data);
	extint_api->extint_enable(intc_dev, port, pin);

	return err;
}

static int gpio_hc32_manage_callback(const struct device *dev, struct gpio_callback *callback,
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

#define GPIO_HC32_DEFINE(n)                                                                        \
	static const struct gpio_hc32_config gpio_hc32_cfg_##n = {                                 \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = (uint16_t *)DT_INST_REG_ADDR(n),                                           \
		.port = n,                                                                         \
	};                                                                                         \
	static struct gpio_hc32_data gpio_hc32_data_##n = {                                        \
		.intc_dev = DEVICE_DT_GET(DT_INST(0, xhsc_hc32_extint)),                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, gpio_hc32_init, NULL, &gpio_hc32_data_##n, &gpio_hc32_cfg_##n,    \
			      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY, &gpio_hc32_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_HC32_DEFINE)
