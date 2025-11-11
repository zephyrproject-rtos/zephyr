/*
 * Copyright (c) 2024 Joel Jaldemark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ilitek_ili2132a

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ili2132a, CONFIG_INPUT_LOG_LEVEL);

#define IS_TOUCHED_BIT 0x40
#define TIP            1
#define X_COORD        2
#define Y_COORD        4

struct ili2132a_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work work;
};

struct ili2132a_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec rst;
	struct gpio_dt_spec irq;
};

static void gpio_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pin)
{
	struct ili2132a_data *data = CONTAINER_OF(cb, struct ili2132a_data, gpio_cb);

	k_work_submit(&data->work);
}

static void ili2132a_process(const struct device *dev)
{
	const struct ili2132a_config *dev_cfg = dev->config;
	uint8_t buf[8];
	uint16_t x, y;
	int ret;

	ret = i2c_read_dt(&dev_cfg->i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("Failed to read data: %d", ret);
		return;
	}

	if (buf[TIP] & IS_TOUCHED_BIT) {
		x = sys_get_le16(&buf[X_COORD]);
		y = sys_get_le16(&buf[Y_COORD]);
		input_report_abs(dev, INPUT_ABS_X, x, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, y, false, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}
}

static void ili2132a_work_handler(struct k_work *work_item)
{
	struct ili2132a_data *data = CONTAINER_OF(work_item, struct ili2132a_data, work);

	ili2132a_process(data->dev);
}

static int ili2132a_init(const struct device *dev)
{
	struct ili2132a_data *data = dev->data;
	const struct ili2132a_config *dev_cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&dev_cfg->i2c)) {
		LOG_ERR("%s is not ready", dev_cfg->i2c.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&dev_cfg->rst)) {
		LOG_ERR("Reset GPIO controller device not ready");
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&dev_cfg->irq)) {
		LOG_ERR("Interrupt GPIO controller device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	ret = gpio_pin_configure_dt(&dev_cfg->irq, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt gpio");
		return ret;
	}

	ret = gpio_pin_configure_dt(&dev_cfg->rst, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure reset gpio");
		return ret;
	}

	ret = gpio_pin_set_dt(&dev_cfg->rst, 0);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, gpio_isr, BIT(dev_cfg->irq.pin));
	ret = gpio_add_callback(dev_cfg->irq.port, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&dev_cfg->irq, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt");
		return ret;
	}

	k_work_init(&data->work, ili2132a_work_handler);

	return 0;
}
#define ILI2132A_INIT(index)                                                                       \
	static const struct ili2132a_config ili2132a_config_##index = {                            \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.rst = GPIO_DT_SPEC_INST_GET(index, rst_gpios),                                    \
		.irq = GPIO_DT_SPEC_INST_GET(index, irq_gpios),                                    \
	};                                                                                         \
	static struct ili2132a_data ili2132a_data_##index;                                         \
	DEVICE_DT_INST_DEFINE(index, ili2132a_init, NULL, &ili2132a_data_##index,                  \
			      &ili2132a_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,   \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(ILI2132A_INIT)
