/*
 * Copyright (c) 2026 Hubert Miś
 * SPDX-License-Identifier: Apache-2.0
 *
 * LED Matrix Driver Implementation
 */

#define DT_DRV_COMPAT led_matrix

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(led_matrix, CONFIG_LED_MATRIX_LOG_LEVEL);

struct led_matrix_config {
	const struct device *row_driver;
	const struct device *col_driver;
	uint32_t refresh_rate_ms;
	uint32_t num_rows;
	uint32_t num_cols;
};

struct led_matrix_data {
	uint8_t brightness_levels[CONFIG_LED_MATRIX_ROWS * CONFIG_LED_MATRIX_COLS];
	struct k_thread refresh_thread;
	struct k_sem any_led_on_sem;

	K_KERNEL_STACK_MEMBER(refresh_thread_stack, CONFIG_LED_MATRIX_THREAD_STACK_SIZE);
};

static bool is_any_led_on(const struct device *dev)
{
	const struct led_matrix_config *config = dev->config;
	const struct led_matrix_data *data = dev->data;

	for (size_t row = 0; row < config->num_rows; row++) {
		for (size_t col = 0; col < config->num_cols; col++) {
			size_t index = row * config->num_cols + col;

			if (data->brightness_levels[index] > 0) {
				return true;
			}
		}
	}

	return false;
}

static void led_matrix_refresh(void *p1, void *p2, void *p3)
{
	const struct device *dev = (const struct device *)p1;
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;

	static size_t previous_row;

	while (true) {
		for (size_t row = 0; row < config->num_rows; row++) {
			led_off(config->row_driver, previous_row);

			/* Set the column brightness levels for the current row */
#if CONFIG_LED_MATRIX_USE_LED_WRITE_CHANNELS
			led_write_channels(config->col_driver, 0,
				config->num_cols,
				&data->brightness_levels[row * config->num_cols]);
#else
			for (size_t col = 0; col < config->num_cols; col++) {
				led_set_brightness(config->col_driver, col,
					data->brightness_levels[row * config->num_cols + col]);
			}
#endif

			/* Check if no LEDs are on and exit the loop early to save power */
			if (!is_any_led_on(dev)) {
				LOG_DBG("No LEDs are on, exiting loop early to save power");
				break;
			}

			/* Enable the current row */
			led_on(config->row_driver, row);
			previous_row = row;

			/* Sleep for the refresh rate duration */
			k_sleep(K_MSEC(config->refresh_rate_ms));
		}

		/* Check if any LED is on after iterating all rows */
		if (!is_any_led_on(dev)) {
			LOG_DBG("No LEDs are on, waiting on semaphore");
			k_sem_take(&data->any_led_on_sem, K_FOREVER);
		}
	}
}

static int led_matrix_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	struct led_matrix_data *data = dev->data;
	const struct led_matrix_config *config = dev->config;

	if (led >= config->num_rows * config->num_cols) {
		LOG_ERR("Invalid LED index: %d", led);
		return -EINVAL;
	}

	data->brightness_levels[led] = value;

	if (value > 0) {
		k_sem_give(&data->any_led_on_sem);
	}

	return 0;
}

static const struct led_driver_api led_matrix_api = {
	.set_brightness = led_matrix_set_brightness,
};

static int led_matrix_init(const struct device *dev)
{
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;

	if (config->num_rows > CONFIG_LED_MATRIX_ROWS) {
		LOG_ERR("Configured number of rows (%d) exceeds maximum allowed (%d)",
			config->num_rows, CONFIG_LED_MATRIX_ROWS);
		return -EINVAL;
	}

	if (config->num_cols > CONFIG_LED_MATRIX_COLS) {
		LOG_ERR("Configured number of columns (%d) exceeds maximum allowed (%d)",
			config->num_cols, CONFIG_LED_MATRIX_COLS);
		return -EINVAL;
	}

	memset(data->brightness_levels, 0, sizeof(data->brightness_levels));
	k_sem_init(&data->any_led_on_sem, 0, 1);
	k_thread_create(&data->refresh_thread, data->refresh_thread_stack,
		CONFIG_LED_MATRIX_THREAD_STACK_SIZE, led_matrix_refresh,
		(void *)dev, NULL, NULL, K_PRIO_PREEMPT(CONFIG_LED_MATRIX_THREAD_PRIORITY),
		0, K_NO_WAIT);

	LOG_INF("LED Matrix initialized");
	return 0;
}

#define LED_MATRIX_INIT(inst)                                                                      \
	static struct led_matrix_data led_matrix_data_##inst;                                      \
	static const struct led_matrix_config led_matrix_config_##inst = {                         \
		.row_driver = DEVICE_DT_GET(DT_INST_PHANDLE(inst, rows)),                          \
		.col_driver = DEVICE_DT_GET(DT_INST_PHANDLE(inst, columns)),                       \
		.refresh_rate_ms = DT_INST_PROP(inst, refresh_rate_ms),                            \
		.num_rows = DT_INST_PROP(inst, num_rows),                                          \
		.num_cols = DT_INST_PROP(inst, num_cols),                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, led_matrix_init, NULL, &led_matrix_data_##inst,                \
			      &led_matrix_config_##inst, POST_KERNEL,                              \
			      CONFIG_LED_MATRIX_INIT_PRIORITY, &led_matrix_api);

DT_INST_FOREACH_STATUS_OKAY(LED_MATRIX_INIT)
