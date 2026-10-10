/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT goodix_gt1151

#include <string.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_touch.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gt1151, CONFIG_INPUT_LOG_LEVEL);

#define GT1151_REG_STATUS 0x814EU
#define GT1151_POINT_SIZE 8U

/* REG_STATUS: buffer ready flag and number of touch points */
#define GT1151_STATUS_READY      BIT(7)
#define GT1151_STATUS_POINTS_MSK 0x0FU

struct gt1151_touch_point {
	uint8_t id;
	uint16_t x;
	uint16_t y;
	uint16_t strength;
} __packed;

struct gt1151_config {
	struct input_touchscreen_common_config common;
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;
#ifdef CONFIG_INPUT_GT1151_INTERRUPT
	struct gpio_dt_spec int_gpio;
#endif
};

struct gt1151_data {
	const struct device *dev;
	struct k_work work;
#ifdef CONFIG_INPUT_GT1151_INTERRUPT
	struct gpio_callback int_gpio_cb;
#else
	struct k_timer timer;
#endif
	bool pressed;
	uint16_t last_x;
	uint16_t last_y;
};

INPUT_TOUCH_STRUCT_CHECK(struct gt1151_config);

static int gt1151_reg_read(const struct i2c_dt_spec *bus, uint16_t reg, void *buf, size_t len)
{
	uint8_t reg_be[2] = { reg >> 8, reg & 0xFF };

	return i2c_write_read_dt(bus, reg_be, sizeof(reg_be), buf, len);
}

static int gt1151_reg_write_u8(const struct i2c_dt_spec *bus, uint16_t reg, uint8_t value)
{
	uint8_t buf[3] = { reg >> 8, reg & 0xFF, value };

	return i2c_write_dt(bus, buf, sizeof(buf));
}

static int gt1151_process(const struct device *dev)
{
	const struct gt1151_config *config = dev->config;
	struct gt1151_data *data = dev->data;
	uint8_t status;
	uint8_t touch_cnt;
	int ret;
	uint8_t raw[1U + GT1151_POINT_SIZE * CONFIG_INPUT_GT1151_MAX_TOUCH_POINTS];

	ret = gt1151_reg_read(&config->bus, GT1151_REG_STATUS, &status, sizeof(status));
	if (ret < 0) {
		return ret;
	}

	if ((status & GT1151_STATUS_READY) == 0U) {
		return 0;
	}

	touch_cnt = MIN(status & GT1151_STATUS_POINTS_MSK, CONFIG_INPUT_GT1151_MAX_TOUCH_POINTS);
	if (touch_cnt == 0U) {
		(void)gt1151_reg_write_u8(&config->bus, GT1151_REG_STATUS, 0U);
		if (data->pressed) {
			input_touchscreen_report_pos(dev, data->last_x, data->last_y, K_FOREVER);
			input_report_key(dev, INPUT_BTN_TOUCH, 0, true, K_FOREVER);
			data->pressed = false;
		}
		return 0;
	}

	ret = gt1151_reg_read(&config->bus, GT1151_REG_STATUS, raw,
			      1U + touch_cnt * GT1151_POINT_SIZE);

	(void)gt1151_reg_write_u8(&config->bus, GT1151_REG_STATUS, 0U);

	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0; i < touch_cnt; i++) {
		const struct gt1151_touch_point *point =
			(const struct gt1151_touch_point *)&raw[1U + i * GT1151_POINT_SIZE];

		if (CONFIG_INPUT_GT1151_MAX_TOUCH_POINTS > 1) {
			input_report_abs(dev, INPUT_ABS_MT_SLOT, point->id & 0x0FU, true, K_FOREVER);
		}

		data->last_x = sys_le16_to_cpu(point->x);
		data->last_y = sys_le16_to_cpu(point->y);
		input_touchscreen_report_pos(dev, data->last_x, data->last_y, K_FOREVER);
	}

	input_report_key(dev, INPUT_BTN_TOUCH, 1, true, K_FOREVER);
	data->pressed = true;

	return 0;
}

static void gt1151_work_handler(struct k_work *work)
{
	struct gt1151_data *data = CONTAINER_OF(work, struct gt1151_data, work);

	(void)gt1151_process(data->dev);
}

#ifdef CONFIG_INPUT_GT1151_INTERRUPT
static void gt1151_isr_handler(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct gt1151_data *data = CONTAINER_OF(cb, struct gt1151_data, int_gpio_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_work_submit(&data->work);
}
#else
static void gt1151_timer_handler(struct k_timer *timer)
{
	struct gt1151_data *data = CONTAINER_OF(timer, struct gt1151_data, timer);

	k_work_submit(&data->work);
}
#endif

static int gt1151_init(const struct device *dev)
{
	const struct gt1151_config *config = dev->config;
	struct gt1151_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	data->dev = dev;
	k_work_init(&data->work, gt1151_work_handler);

	if (config->reset_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			return ret;
		}

		k_msleep(10);
		ret = gpio_pin_set_dt(&config->reset_gpio, 0);
		if (ret < 0) {
			return ret;
		}
		k_msleep(10);
	}

#ifdef CONFIG_INPUT_GT1151_INTERRUPT
	if (!gpio_is_ready_dt(&config->int_gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	gpio_init_callback(&data->int_gpio_cb, gt1151_isr_handler, BIT(config->int_gpio.pin));
	ret = gpio_add_callback(config->int_gpio.port, &data->int_gpio_cb);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		return ret;
	}
#else
	k_timer_init(&data->timer, gt1151_timer_handler, NULL);
	k_timer_start(&data->timer, K_MSEC(CONFIG_INPUT_GT1151_PERIOD_MS),
		      K_MSEC(CONFIG_INPUT_GT1151_PERIOD_MS));
#endif

	return 0;
}

#define GT1151_INIT(n)                                                                             \
	static const struct gt1151_config gt1151_config_##n = {                                    \
		.common = INPUT_TOUCH_DT_INST_COMMON_CONFIG_INIT(n),                               \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {0}),                        \
		IF_ENABLED(CONFIG_INPUT_GT1151_INTERRUPT,                                          \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET(n, irq_gpios),))                     \
	};                                                                                         \
	static struct gt1151_data gt1151_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, gt1151_init, NULL, &gt1151_data_##n, &gt1151_config_##n,          \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(GT1151_INIT)
