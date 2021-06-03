/*
 * Copyright (c) 2020 Innoseis BV
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tca6408a

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <logging/log.h>
#include <stdint.h>
#include "gpio_utils.h"

LOG_MODULE_REGISTER(tca6408a, CONFIG_GPIO_LOG_LEVEL);

enum {
	TCA6408A_INPUT_PORT_REG         = 0x00,
	TCA6408A_OUTPUT_PORT_REG        = 0x01,
	TCA6408A_POLARITY_INV_REG       = 0x02,
	TCA6408A_CONFIG_REG             = 0x03,
};

struct gpio_tca6408a_config {
	struct gpio_driver_config gpio_config;
	const struct device *bus;
	uint16_t slave_addr;
};

struct gpio_tca6408a_data {
	struct gpio_driver_data gpio_data;
	int pm_state;
	uint8_t config_reg;
	uint8_t output_reg;
	struct k_mutex mutex;
};

static inline const struct gpio_tca6408a_config *get_config(
	const struct device *dev)
{
	return dev->config;
}

static inline struct gpio_tca6408a_data *get_data(const struct device *dev)
{
	return dev->data;
}

static int gpio_tca6408a_write_reg(const struct device *dev, uint8_t reg_addr,
				   uint8_t reg_value)
{
	const struct gpio_tca6408a_config *config = get_config(dev);

	return i2c_reg_write_byte(config->bus, config->slave_addr, reg_addr,
				  reg_value);
}

static int gpio_tca6408a_write_output_reg(const struct device *dev,
					  uint8_t reg_value)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	rc = gpio_tca6408a_write_reg(dev, TCA6408A_OUTPUT_PORT_REG, reg_value);
	if (rc == 0) {
		gpio_tca6408a->output_reg = reg_value;
	}

	return rc;
}

static int gpio_tca6408a_write_config_reg(const struct device *dev,
					  uint8_t reg_value)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	rc = gpio_tca6408a_write_reg(dev, TCA6408A_CONFIG_REG, reg_value);
	if (rc == 0) {
		gpio_tca6408a->config_reg = reg_value;
	}

	return rc;
}

static int gpio_tca6408a_pin_configure(const struct device *dev, gpio_pin_t pin,
				       gpio_flags_t flags)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	rc = (flags & (GPIO_DS_ALT_LOW | GPIO_DS_ALT_HIGH)) == 0 ? 0 : -ENOTSUP;
	if (rc) {
		return rc;
	}

	rc = (flags & GPIO_SINGLE_ENDED) == 0 ? 0 : -ENOTSUP;
	if (rc) {
		return rc;
	}

	rc = (flags & (GPIO_PULL_DOWN | GPIO_PULL_UP)) == 0 ? 0 : -ENOTSUP;
	if (rc) {
		return rc;
	}

	rc = (flags & (GPIO_INPUT | GPIO_OUTPUT)) != GPIO_DISCONNECTED ? 0
	     : -ENOTSUP;
	if (rc) {
		return rc;
	}

	k_mutex_lock(&gpio_tca6408a->mutex, K_FOREVER);
	uint8_t config_reg = gpio_tca6408a->config_reg;
	uint8_t output_reg = gpio_tca6408a->output_reg;

	WRITE_BIT(config_reg, pin, (flags & GPIO_OUTPUT) ? 0 : 1);
	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			WRITE_BIT(output_reg, pin, 0);
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			WRITE_BIT(output_reg, pin, 1);
		}
	}

	rc = gpio_tca6408a_write_output_reg(dev, output_reg);
	if (rc) {
		goto i2c_gpio6408a_configure_end;
	}

	rc = gpio_tca6408a_write_config_reg(dev, config_reg);

i2c_gpio6408a_configure_end:
	k_mutex_unlock(&gpio_tca6408a->mutex);
	return rc;
}

static int gpio_tca6408a_port_get_raw(const struct device *dev,
				      gpio_port_value_t *value)
{
	uint8_t reg_input;
	int rc;
	const struct gpio_tca6408a_config *config = get_config(dev);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	rc = i2c_reg_read_byte(config->bus, config->slave_addr,
			       TCA6408A_INPUT_PORT_REG, &reg_input);

	if (rc == 0) {
		*value = reg_input;
	}

	return rc;
}

static int gpio_tca6408a_port_set_masked_raw(const struct device *dev,
					     gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);
	const uint8_t reg_val = mask & (uint8_t)value;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&gpio_tca6408a->mutex, K_FOREVER);
	rc = gpio_tca6408a_write_output_reg(dev, reg_val);
	k_mutex_unlock(&gpio_tca6408a->mutex);

	return rc;
}

static int gpio_tca6408a_port_set_bits_raw(const struct device *dev,
					   gpio_port_pins_t pins)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&gpio_tca6408a->mutex, K_FOREVER);
	const uint8_t reg_val = gpio_tca6408a->output_reg | (uint8_t)pins;

	rc = gpio_tca6408a_write_output_reg(dev, reg_val);
	k_mutex_unlock(&gpio_tca6408a->mutex);

	return rc;
}

static int gpio_tca6408a_port_clear_bits_raw(const struct device *dev,
					     gpio_port_pins_t pins)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&gpio_tca6408a->mutex, K_FOREVER);
	const uint8_t reg_val = gpio_tca6408a->config_reg & ~pins;

	rc = gpio_tca6408a_write_output_reg(dev, reg_val);
	k_mutex_unlock(&gpio_tca6408a->mutex);

	return rc;
}

static int gpio_tca6408a_port_toggle_bits(const struct device *dev,
					  gpio_port_pins_t pins)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_mutex_lock(&gpio_tca6408a->mutex, K_FOREVER);
	const uint8_t reg_val = gpio_tca6408a->output_reg ^ pins;

	rc = gpio_tca6408a_write_output_reg(dev, reg_val);
	k_mutex_unlock(&gpio_tca6408a->mutex);

	return rc;
}

static int gpio_tca608a_apply_config(const struct device *dev)
{
	int rc;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	k_mutex_lock(&gpio_tca6408a->mutex, K_FOREVER);

	rc = gpio_tca6408a_write_output_reg(dev, gpio_tca6408a->output_reg);

	if (rc == 0) {
		rc = gpio_tca6408a_write_config_reg(dev,
						    gpio_tca6408a->config_reg);
	}

	k_mutex_unlock(&gpio_tca6408a->mutex);

	return rc;
}

static int gpio_tca6408a_pin_interrupt_configure(const struct device *port,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	return -ENOTSUP;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static int gpio_tca6408_pm_control(const struct device *dev,
				   uint32_t command,
				   uint32_t *context,
				   pm_device_cb cb,
				   void *arg)
{
	int ret = 0;
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);

	switch (command) {
	case PM_DEVICE_STATE_SET:
		if (*context == PM_DEVICE_STATE_ACTIVE) {
			ret = gpio_tca608a_apply_config(dev);
			gpio_tca6408a->pm_state = PM_DEVICE_STATE_ACTIVE;
		} else if (*context == PM_DEVICE_STATE_SUSPEND) {
			gpio_tca6408a->pm_state = PM_DEVICE_STATE_SUSPEND;
		} else {
			ret = -EINVAL;
		}
		break;
	case PM_DEVICE_STATE_GET:
		*context = gpio_tca6408a->pm_state;
		break;
	default:
		ret = -EINVAL;
	}

	if (cb) {
		cb(dev, ret, context, arg);
	}

	return ret;
}
#endif

static int gpio_tca6408a_init(const struct device *dev)
{
	struct gpio_tca6408a_data *gpio_tca6408a = get_data(dev);
	const struct gpio_tca6408a_config *config = get_config(dev);

	if (!device_is_ready(config->bus)) {
		LOG_ERR("Could not find i2c device %s", config->bus->name);
		return -1;
	}

	gpio_tca6408a->config_reg = 0xff;
	gpio_tca6408a->output_reg = 0x0;
	k_mutex_init(&gpio_tca6408a->mutex);

	return 0;
}

struct gpio_driver_api gpio_tca6408a_drv_api_funcs = {
	.pin_configure = gpio_tca6408a_pin_configure,
	.port_get_raw = gpio_tca6408a_port_get_raw,
	.port_set_masked_raw = gpio_tca6408a_port_set_masked_raw,
	.port_set_bits_raw = gpio_tca6408a_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tca6408a_port_clear_bits_raw,
	.port_toggle_bits = gpio_tca6408a_port_toggle_bits,
	.pin_interrupt_configure = gpio_tca6408a_pin_interrupt_configure,
	.manage_callback = NULL,
	.get_pending_int = NULL,
};

#define TCA6408A_DEV_DEFINE(inst)					       \
	static struct gpio_tca6408a_data gpio_tca6408a_drvdata_##inst;	       \
									       \
	static const struct gpio_tca6408a_config gpio_tca6408a_cfg_##inst = {  \
		.gpio_config = {					       \
			.port_pin_mask =				       \
				GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)	       \
		},							       \
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),		       \
		.slave_addr = DT_INST_REG_ADDR(inst),			       \
	};								       \
									       \
	DEVICE_DT_INST_DEFINE(inst,					       \
			      gpio_tca6408a_init,			       \
			      gpio_tca6408_pm_control,			       \
			      &gpio_tca6408a_drvdata_##inst,		       \
			      &gpio_tca6408a_cfg_##inst,		       \
			      POST_KERNEL, CONFIG_GPIO_TCA6408A_INIT_PRIORITY, \
			      &gpio_tca6408a_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(TCA6408A_DEV_DEFINE)
