/*
 * Copyright (c) 2022 Schlumberger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/intc_xmc4xxx.h>
#include <zephyr/dt-bindings/gpio/infineon-xmc4xxx-gpio.h>
#include <xmc_gpio.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#define PORT_TO_PORT_ID(port)      ((int)(port) >> 8 & 0xf)

struct gpio_xmc4xxx_config {
	/* gpio_driver_config needs to be first, required by Zephyr */
	struct gpio_driver_config common;
	XMC_GPIO_PORT_t *port;
};

struct gpio_xmc4xxx_data {
	/* gpio_driver_data needs to be first, required by Zephyr */
	struct gpio_driver_data common;
#if defined(CONFIG_XMC4XXX_INTC)
	sys_slist_t callbacks;
#endif
};

static int gpio_xmc4xxx_convert_flags(XMC_GPIO_CONFIG_t *pin_config, gpio_flags_t flags)
{
	bool is_input  = flags & GPIO_INPUT;
	bool is_output = flags & GPIO_OUTPUT;
	int ds;

	/* GPIO_DISCONNECTED */
	if (!is_input && !is_output) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OPEN_SOURCE) {
		return -ENOTSUP;
	}

	if (is_input) {
		pin_config->mode = XMC_GPIO_MODE_INPUT_TRISTATE;
		if (flags & GPIO_PULL_DOWN) {
			pin_config->mode = XMC_GPIO_MODE_INPUT_PULL_DOWN;
		}
		if (flags & GPIO_PULL_UP) {
			pin_config->mode = XMC_GPIO_MODE_INPUT_PULL_UP;
		}
	}

	ds = XMC4XXX_GPIO_GET_DS(flags);
	if ((!is_output && ds) || ds > XMC4XXX_GPIO_DS_WEAK) {
		return -EINVAL;
	}

	if (is_output) {
		pin_config->mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL;
		if (flags & GPIO_OPEN_DRAIN) {
			pin_config->mode = XMC_GPIO_MODE_OUTPUT_OPEN_DRAIN;
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			pin_config->output_level = XMC_GPIO_OUTPUT_LEVEL_LOW;
		}
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			pin_config->output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH;
		}
		/* Strong medium edge is default */
		pin_config->output_strength = XMC_GPIO_OUTPUT_STRENGTH_STRONG_MEDIUM_EDGE;
		if (ds > 0) {
			pin_config->output_strength = ds - 1;
		}
	}

	return 0;
}

static int gpio_xmc4xxx_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	XMC_GPIO_PORT_t *port = config->port;
	XMC_GPIO_CONFIG_t pin_config = {0};
	gpio_port_pins_t pin_mask = config->common.port_pin_mask;
	int ret;

	if ((BIT(pin) & pin_mask) == 0) {
		return -EINVAL;
	}

	if ((port == (XMC_GPIO_PORT_t *)PORT14_BASE || port == (XMC_GPIO_PORT_t *)PORT15_BASE) &&
	    (flags & GPIO_OUTPUT)) {
		return -EINVAL;
	}

	ret = gpio_xmc4xxx_convert_flags(&pin_config, flags);
	if (ret) {
		return ret;
	}

	XMC_GPIO_Init(port, pin, &pin_config);
	return 0;
}

#if defined(CONFIG_XMC4XXX_INTC)
static void gpio_xmc4xxx_isr(const struct device *dev, int pin)
{
	struct gpio_xmc4xxx_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}
#endif

#ifdef CONFIG_XMC4XXX_INTC
static int gpio_xmc4xxx_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	int port_id = PORT_TO_PORT_ID(config->port);

	if (mode & GPIO_INT_ENABLE) {
		return intc_xmc4xxx_gpio_enable_interrupt(port_id, pin, mode, trig,
							  gpio_xmc4xxx_isr, (void *)dev);
	} else if (mode & GPIO_INT_DISABLE) {
		return intc_xmc4xxx_gpio_disable_interrupt(port_id, pin);
	} else {
		return -EINVAL;
	}
}
#endif

static int gpio_xmc4xxx_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	XMC_GPIO_PORT_t *port = config->port;
	gpio_port_pins_t pin_mask = config->common.port_pin_mask;

	*value = port->IN & pin_mask;
	return 0;
}

#if defined(CONFIG_XMC4XXX_INTC)
static int gpio_xmc4xxx_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct gpio_xmc4xxx_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}
#endif

static int gpio_xmc4xxx_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	XMC_GPIO_PORT_t *port = config->port;
	gpio_port_pins_t pin_mask = config->common.port_pin_mask;

	mask &= pin_mask;

	/* OMR - output modification register. Upper 16 bits is used to clear pins */
	port->OMR = (value & mask) | (~value & mask) << 16;
	return 0;
}

static int gpio_xmc4xxx_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	XMC_GPIO_PORT_t *port = config->port;
	gpio_port_pins_t pin_mask = config->common.port_pin_mask;

	port->OMR = pins & pin_mask;
	return 0;
}

static int gpio_xmc4xxx_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	XMC_GPIO_PORT_t *port = config->port;

	port->OMR = pins << 16;
	return 0;
}

static int gpio_xmc4xxx_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_xmc4xxx_config *config = dev->config;
	XMC_GPIO_PORT_t *port = config->port;
	gpio_port_pins_t pin_mask = config->common.port_pin_mask;

	pins &= pin_mask;
	port->OMR = pins | pins << 16;
	return 0;
}

static int gpio_xmc4xxx_init(const struct device *dev) { return 0; }

static const struct gpio_driver_api gpio_xmc4xxx_driver_api = {
	.pin_configure = gpio_xmc4xxx_pin_configure,
	.port_get_raw = gpio_xmc4xxx_get_raw,
	.port_set_masked_raw = gpio_xmc4xxx_set_masked_raw,
	.port_set_bits_raw = gpio_xmc4xxx_set_bits_raw,
	.port_clear_bits_raw = gpio_xmc4xxx_clear_bits_raw,
	.port_toggle_bits = gpio_xmc4xxx_toggle_bits,
#ifdef CONFIG_XMC4XXX_INTC
	.pin_interrupt_configure = gpio_xmc4xxx_pin_interrupt_configure,
	.manage_callback = gpio_xmc4xxx_manage_callback,
#endif
};

#define GPIO_XMC4XXX_INIT(index)                                                                   \
	static struct gpio_xmc4xxx_data xmc4xxx_data_##index;                                      \
                                                                                                   \
	static const struct gpio_xmc4xxx_config xmc4xxx_config_##index = {                         \
		.port = (XMC_GPIO_PORT_t *)DT_INST_REG_ADDR(index),                                \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(index)}};              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, gpio_xmc4xxx_init, NULL, &xmc4xxx_data_##index,               \
			      &xmc4xxx_config_##index, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,     \
			      &gpio_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_XMC4XXX_INIT)
