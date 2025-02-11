/*
 * Copyright (c) 2024 Nicolas Goualard <nicolas.goualard@sfr.fr>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT chipsemi_chsc6x

#include <zephyr/sys/byteorder.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

struct chsc6x_config {
	struct i2c_dt_spec i2c;
	const struct gpio_dt_spec int_gpio;
};

struct chsc6x_data {
	const struct device *dev;
	struct k_work work;
	struct gpio_callback int_gpio_cb;
};

#define CHSC6X_READ_ADDR             0
#define CHSC6X_READ_LENGTH           5
#define CHSC6X_OUTPUT_POINTS_PRESSED 0
#define CHSC6X_OUTPUT_COL            2
#define CHSC6X_OUTPUT_ROW            4

LOG_MODULE_REGISTER(chsc6x, CONFIG_INPUT_LOG_LEVEL);

static int chsc6x_process(const struct device *dev)
{
	uint8_t output[CHSC6X_READ_LENGTH];
	uint8_t row, col;
	bool is_pressed;
	int ret;

	const struct chsc6x_config *cfg = dev->config;

	ret = i2c_burst_read_dt(&cfg->i2c, CHSC6X_READ_ADDR, output, CHSC6X_READ_LENGTH);
	if (ret < 0) {
		LOG_ERR("Could not read data: %i", ret);
		return -ENODATA;
	}

	is_pressed = output[CHSC6X_OUTPUT_POINTS_PRESSED];
	col = output[CHSC6X_OUTPUT_COL];
	row = output[CHSC6X_OUTPUT_ROW];

	if (is_pressed) {
		input_report_abs(dev, INPUT_ABS_X, col, false, K_FOREVER);
		input_report_abs(dev, INPUT_ABS_Y, row, false, K_FOREVER);
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

#define CHSC6X_DEFINE(index)                                                                       \
	static const struct chsc6x_config chsc6x_config_##index = {                                \
		.i2c = I2C_DT_SPEC_INST_GET(index),                                                \
		.int_gpio = GPIO_DT_SPEC_INST_GET(index, irq_gpios),                               \
	};                                                                                         \
	static struct chsc6x_data chsc6x_data_##index;                                             \
	DEVICE_DT_INST_DEFINE(index, chsc6x_init, NULL, &chsc6x_data_##index,                      \
			      &chsc6x_config_##index, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,     \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(CHSC6X_DEFINE)
