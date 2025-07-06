/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_creg_gpio

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(creg_gpio, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

/** Runtime driver data */
struct creg_gpio_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	uint32_t pin_val;
	uint32_t base_addr;
};

/** Configuration data */
struct creg_gpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t ngpios;
	uint8_t bit_per_gpio;
	uint8_t off_val;
	uint8_t on_val;
};

static int port_get(const struct device *dev,
		    gpio_port_value_t *value)
{
	const struct creg_gpio_config *cfg = dev->config;
	struct creg_gpio_drv_data *drv_data = dev->data;
	uint32_t in = sys_read32(drv_data->base_addr);
	uint32_t tmp = 0;
	uint32_t val = 0;

	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		tmp = (in & cfg->on_val << i * cfg->bit_per_gpio) ? 1 : 0;
		val |= tmp << i;
	}
	*value = drv_data->pin_val = val;

	return 0;
}

static int port_write(const struct device *dev,
		      gpio_port_pins_t mask,
		      gpio_port_value_t value,
		      gpio_port_value_t toggle)
{
	const struct creg_gpio_config *cfg = dev->config;
	struct creg_gpio_drv_data *drv_data = dev->data;
	uint32_t *pin_val = &drv_data->pin_val;
	uint32_t out = 0;
	uint32_t tmp = 0;

	*pin_val = ((*pin_val & ~mask) | (value & mask)) ^ toggle;

	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		tmp = (*pin_val & 1 << i) ? cfg->on_val : cfg->off_val;
		out |= tmp << i * cfg->bit_per_gpio;
	}
	sys_write32(out, drv_data->base_addr);

	return 0;
}

static int port_set_masked(const struct device *dev,
			   gpio_port_pins_t mask,
			   gpio_port_value_t value)
{
	return port_write(dev, mask, value, 0);
}

static int port_set_bits(const struct device *dev,
			 gpio_port_pins_t pins)
{
	return port_write(dev, pins, pins, 0);
}

static int port_clear_bits(const struct device *dev,
			   gpio_port_pins_t pins)
{
	return port_write(dev, pins, 0, 0);
}

static int port_toggle_bits(const struct device *dev,
			    gpio_port_pins_t pins)
{
	return port_write(dev, 0, 0, pins);
}

static int pin_config(const struct device *dev,
		       gpio_pin_t pin,
		       gpio_flags_t flags)
{
	const struct creg_gpio_config *cfg = dev->config;
	uint32_t io_flags;
	bool pin_is_output;

	/* Check for invalid pin number */
	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	/* Does not support disconnected pin, and
	 * not supporting both input/output at same time.
	 */
	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if ((io_flags == GPIO_DISCONNECTED)
	    || (io_flags == (GPIO_INPUT | GPIO_OUTPUT))) {
		return -ENOTSUP;
	}

	/* No open-drain support */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	/* Does not support pull-up/pull-down */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	pin_is_output = (flags & GPIO_OUTPUT) != 0U;
	if (pin_is_output) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			return port_set_bits(dev, BIT(pin));
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			return port_clear_bits(dev, BIT(pin));
		}
	}

	return -ENOTSUP;
}

static DEVICE_API(gpio, api_table) = {
	.pin_configure = pin_config,
	.port_get_raw = port_get,
	.port_set_masked_raw = port_set_masked,
	.port_set_bits_raw = port_set_bits,
	.port_clear_bits_raw = port_clear_bits,
	.port_toggle_bits = port_toggle_bits,
};

static const struct creg_gpio_config creg_gpio_cfg = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	},
	.ngpios = DT_INST_PROP(0, ngpios),
	.bit_per_gpio = DT_INST_PROP(0, bit_per_gpio),
	.off_val = DT_INST_PROP(0, off_val),
	.on_val = DT_INST_PROP(0, on_val),
};

static struct creg_gpio_drv_data creg_gpio_drvdata = {
	.base_addr = DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &creg_gpio_drvdata, &creg_gpio_cfg,
		      POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,
		      &api_table);
