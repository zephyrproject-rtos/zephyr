/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max7321

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(max7321, CONFIG_GPIO_LOG_LEVEL);

/* Valid pin mask */
#define MAX7321_ALL_PINS(dev) \
	((uint8_t)((const struct max7321_drv_cfg *)(dev)->config)->common.port_pin_mask)

struct max7321_drv_data {
	struct gpio_driver_data common;

	uint8_t output_latch;   /* Software latch image */
	uint8_t output_mask;    /* 1 = output, 0 = input */
	uint8_t input_cache;    /* Last read input snapshot */

	sys_slist_t callbacks;

	struct k_sem lock;
	struct k_work work;

	const struct device *dev;
	struct gpio_callback int_gpio_cb;
};

struct max7321_drv_cfg {
	struct gpio_driver_config common;
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec gpio_int;
};

static int max7321_read_port(const struct device *dev, uint8_t *val)
{
	const struct max7321_drv_cfg *cfg = dev->config;
	int ret = i2c_read_dt(&cfg->i2c, val, 1);

	return ret ? -EIO : 0;
}

static int max7321_write_port(const struct device *dev, uint8_t val)
{
	const struct max7321_drv_cfg *cfg = dev->config;
	int ret = i2c_write_dt(&cfg->i2c, &val, 1);

	return ret ? -EIO : 0;
}

static int max7321_write_effective(const struct device *dev)
{
	struct max7321_drv_data *data = dev->data;
	const struct max7321_drv_cfg *cfg = dev->config;

	uint8_t mask = cfg->common.port_pin_mask;

	uint8_t out =
		(data->output_latch & data->output_mask) |
		(~data->output_mask & mask);

	return max7321_write_port(dev, out);
}

static void max7321_work_handler(struct k_work *work)
{
	struct max7321_drv_data *data =
		CONTAINER_OF(work, struct max7321_drv_data, work);

	uint8_t now;

	k_sem_take(&data->lock, K_FOREVER);

	if (max7321_read_port(data->dev, &now) == 0) {
		gpio_port_pins_t changed =
			(now ^ data->input_cache) & ~data->output_mask;

		data->input_cache = now;
		k_sem_give(&data->lock);

		if (changed) {
			gpio_fire_callbacks(&data->callbacks,
					    data->dev, changed);
		}
	} else {
		k_sem_give(&data->lock);
	}
}

static void max7321_int_gpio_handler(const struct device *port,
				     struct gpio_callback *cb,
				     uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct max7321_drv_data *data =
		CONTAINER_OF(cb, struct max7321_drv_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static int max7321_pin_configure(const struct device *dev,
				 gpio_pin_t pin,
				 gpio_flags_t flags)
{
	struct max7321_drv_data *data = dev->data;
	const struct max7321_drv_cfg *cfg = dev->config;

	if (!(BIT(pin) & cfg->common.port_pin_mask)) {
		return -EINVAL;
	}

	if (flags & GPIO_PULL_DOWN) {
		return -ENOTSUP;
	}

	if (flags & GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INPUT) && (flags & GPIO_OUTPUT)) {
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (flags & GPIO_INPUT) {
		data->output_mask &= ~BIT(pin);
		data->output_latch |= BIT(pin);
	}

	if (flags & GPIO_OUTPUT) {
		data->output_mask |= BIT(pin);

		if (flags & GPIO_OUTPUT_INIT_LOW) {
			data->output_latch &= ~BIT(pin);
		} else {
			data->output_latch |= BIT(pin);
		}
	}

	int ret = max7321_write_effective(dev);

	k_sem_give(&data->lock);
	return ret;
}

static int max7321_port_get_raw(const struct device *dev,
				gpio_port_value_t *value)
{
	struct max7321_drv_data *data = dev->data;
	uint8_t v;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&data->lock, K_FOREVER);

	if (max7321_read_port(dev, &v) == 0) {
		data->input_cache = v;
		*value = v;
		k_sem_give(&data->lock);
		return 0;
	}

	k_sem_give(&data->lock);
	return -EIO;
}

static int max7321_port_set_masked_raw(const struct device *dev,
				       gpio_port_pins_t mask,
				       gpio_port_value_t value)
{
	struct max7321_drv_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&data->lock, K_FOREVER);

	data->output_latch =
		(data->output_latch & ~mask) | (value & mask);

	int ret = max7321_write_effective(dev);

	k_sem_give(&data->lock);
	return ret;
}

static int max7321_port_set_bits_raw(const struct device *dev,
				     gpio_port_pins_t pins)
{
	return max7321_port_set_masked_raw(dev, pins, pins);
}

static int max7321_port_clear_bits_raw(const struct device *dev,
				       gpio_port_pins_t pins)
{
	return max7321_port_set_masked_raw(dev, pins, 0);
}

static int max7321_port_toggle_bits(const struct device *dev,
				    gpio_port_pins_t pins)
{
	struct max7321_drv_data *data = dev->data;

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&data->lock, K_FOREVER);

	data->output_latch ^= pins;

	int ret = max7321_write_effective(dev);

	k_sem_give(&data->lock);
	return ret;
}

static int max7321_pin_interrupt_configure(const struct device *dev,
					   gpio_pin_t pin,
					   enum gpio_int_mode mode,
					   enum gpio_int_trig trig)
{
	const struct max7321_drv_cfg *cfg = dev->config;

	ARG_UNUSED(pin);
	ARG_UNUSED(trig);

	if (mode != GPIO_INT_MODE_DISABLED && !cfg->gpio_int.port) {
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	return 0;
}

static int max7321_manage_callback(const struct device *dev,
				   struct gpio_callback *cb,
				   bool set)
{
	struct max7321_drv_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static int max7321_init(const struct device *dev)
{
	const struct max7321_drv_cfg *cfg = dev->config;
	struct max7321_drv_data *data = dev->data;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		return -ENODEV;
	}

	uint8_t all_pins = MAX7321_ALL_PINS(dev);

	data->dev = dev;
	data->output_latch = all_pins;
	data->output_mask = 0x00;

	k_sem_init(&data->lock, 1, 1);
	k_work_init(&data->work, max7321_work_handler);

	if (max7321_write_port(dev, all_pins)) {
		LOG_ERR("%s: failed to init port", dev->name);
		return -EIO;
	}

	if (max7321_read_port(dev, &data->input_cache)) {
		LOG_ERR("%s: failed to read initial state", dev->name);
		return -EIO;
	}

	if (cfg->gpio_int.port) {
		if (!gpio_is_ready_dt(&cfg->gpio_int)) {
			LOG_ERR("%s: INT GPIO not ready", dev->name);
			return -ENODEV;
		}

		int ret;

		ret = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
		if (ret) {
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(
			&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
		if (ret) {
			return ret;
		}

		gpio_init_callback(&data->int_gpio_cb,
				   max7321_int_gpio_handler,
				   BIT(cfg->gpio_int.pin));

		ret = gpio_add_callback(cfg->gpio_int.port,
					&data->int_gpio_cb);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static DEVICE_API(gpio, max7321_api) = {
	.pin_configure = max7321_pin_configure,
	.port_get_raw = max7321_port_get_raw,
	.port_set_masked_raw = max7321_port_set_masked_raw,
	.port_set_bits_raw = max7321_port_set_bits_raw,
	.port_clear_bits_raw = max7321_port_clear_bits_raw,
	.port_toggle_bits = max7321_port_toggle_bits,
	.pin_interrupt_configure = max7321_pin_interrupt_configure,
	.manage_callback = max7321_manage_callback,
};

#define GPIO_MAX7321_INST(idx)						      \
	static const struct max7321_drv_cfg max7321_cfg_##idx = {	      \
		.common = {						      \
			.port_pin_mask =				      \
				GPIO_PORT_PIN_MASK_FROM_DT_INST(idx),	      \
		},							      \
		.i2c = I2C_DT_SPEC_INST_GET(idx),			      \
		.gpio_int =						      \
			GPIO_DT_SPEC_INST_GET_OR(idx, int_gpios, {0}),       \
	};								      \
	static struct max7321_drv_data max7321_data_##idx;		      \
	DEVICE_DT_INST_DEFINE(idx, max7321_init, NULL,			      \
			      &max7321_data_##idx,			      \
			      &max7321_cfg_##idx,			      \
			      POST_KERNEL,				      \
			      CONFIG_GPIO_MAX7321_INIT_PRIORITY,	      \
			      &max7321_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MAX7321_INST);
