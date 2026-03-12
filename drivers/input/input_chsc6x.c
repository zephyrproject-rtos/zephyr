/*
 * Copyright (c) 2024 Nicolas Goualard <nicolas.goualard@sfr.fr>
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(chsc6x, CONFIG_INPUT_LOG_LEVEL);

typedef int (*chsc6x_read_data_fn)(const struct i2c_dt_spec *i2c, uint8_t *is_pressed,
				   uint16_t *x_axis, uint16_t *y_axis);

struct chsc6x_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpio;
	chsc6x_read_data_fn read_data;
};

struct chsc6x_data {
	const struct device *dev;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
};

#define CHSC6X_READ_ADDR 0
#define CHSC6X_READ_LEN  5
#define CHSC6X_PRESS_POS 0
#define CHSC6X_X_POS     2
#define CHSC6X_Y_POS     4

static int __maybe_unused chsc6x_read_data(const struct i2c_dt_spec *i2c, uint8_t *is_pressed,
					   uint16_t *x_axis, uint16_t *y_axis)
{
	uint8_t output[CHSC6X_READ_LEN];
	int ret;

	ret = i2c_burst_read_dt(i2c, CHSC6X_READ_ADDR, output, CHSC6X_READ_LEN);
	if (ret < 0) {
		return -ENODATA;
	}

	*is_pressed = output[CHSC6X_PRESS_POS];
	*x_axis = output[CHSC6X_X_POS];
	*y_axis = output[CHSC6X_Y_POS];

	return 0;
}

#define CHSC6540_READ_ADDR 0
#define CHSC6540_READ_LEN  7
#define CHSC6540_PRESS_POS 2
#define CHSC6540_X_POS     3
#define CHSC6540_Y_POS     5

static int __maybe_unused chsc6540_read_data(const struct i2c_dt_spec *i2c, uint8_t *is_pressed,
					     uint16_t *x_axis, uint16_t *y_axis)
{
	uint8_t output[CHSC6540_READ_LEN];
	int ret;

	ret = i2c_burst_read_dt(i2c, CHSC6540_READ_ADDR, output, CHSC6540_READ_LEN);
	if (ret < 0) {
		return -ENODATA;
	}

	*is_pressed = output[CHSC6540_PRESS_POS];
	*x_axis = sys_get_be16(&output[CHSC6540_X_POS]) & 0x0FFF;
	*y_axis = sys_get_be16(&output[CHSC6540_Y_POS]) & 0x0FFF;

	return 0;
}

static int chsc6x_process(const struct device *dev)
{
	uint16_t y_axis, x_axis;
	uint8_t is_pressed;
	int ret;

	const struct chsc6x_config *cfg = dev->config;

	ret = cfg->read_data(&cfg->i2c, &is_pressed, &x_axis, &y_axis);
	if (ret < 0) {
		LOG_ERR("Could not read data: %d", ret);
		return ret;
	}

	LOG_DBG("is_pressed=%u, x_axis=%u, y_axis=%u", is_pressed, x_axis, y_axis);

	if (is_pressed) {
		input_report_abs(dev, INPUT_ABS_X, x_axis, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, y_axis, false, K_FOREVER);
		input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	} else {
		input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
	}

	return 0;
}

static void chsc6x_work_handler(struct k_work *work)
{
	struct chsc6x_data *data = CONTAINER_OF(work, struct chsc6x_data, work);

	chsc6x_process(data->dev);
}

static void chsc6x_isr_handler(const struct device *dev, struct gpio_callback *cb, uint32_t mask)
{
	struct chsc6x_data *data = CONTAINER_OF(cb, struct chsc6x_data, int_gpio_cb);

	k_work_submit(&data->work);
}

static int chsc6x_chip_init(const struct device *dev)
{
	const struct chsc6x_config *cfg = dev->config;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	return 0;
}

static int chsc6x_init(const struct device *dev)
{
	struct chsc6x_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_work_init(&data->work, chsc6x_work_handler);

	const struct chsc6x_config *config = dev->config;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("GPIO port %s not ready", config->int_gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO pin: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO interrupt: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, chsc6x_isr_handler, BIT(config->int_gpio.pin));

	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not set gpio callback: %d", ret);
		return ret;
	}

	return chsc6x_chip_init(dev);
};

#define CHSC6_DEFINE(node_id, _read_data)                                                          \
	static const struct chsc6x_config chsc6x_config_##node_id = {                              \
		.i2c = I2C_DT_SPEC_GET(node_id),                                                   \
		.int_gpio = GPIO_DT_SPEC_GET(node_id, irq_gpios),                                  \
		.read_data = _read_data,                                                           \
	};                                                                                         \
	static struct chsc6x_data chsc6x_data_##node_id;                                           \
	DEVICE_DT_DEFINE(node_id, chsc6x_init, NULL, &chsc6x_data_##node_id,                       \
			 &chsc6x_config_##node_id, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

#define CHSC6X_DEFINE(node_id) \
	CHSC6_DEFINE(node_id, chsc6x_read_data)

#define CHSC6540_DEFINE(node_id) \
	CHSC6_DEFINE(node_id, chsc6540_read_data)

DT_FOREACH_STATUS_OKAY(chipsemi_chsc6x, CHSC6X_DEFINE)
DT_FOREACH_STATUS_OKAY(chipsemi_chsc6540, CHSC6540_DEFINE)
