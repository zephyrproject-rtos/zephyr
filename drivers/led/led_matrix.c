/*
 * Copyright (c) 2026 Hubert Miś
 * SPDX-License-Identifier: Apache-2.0
 *
 * LED Matrix Driver Implementation
 */

#define DT_DRV_COMPAT led_matrix

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/led/led_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(led_matrix, CONFIG_LED_LOG_LEVEL);

struct led_matrix_led_config {
	uint8_t row;
	uint8_t col;
};

struct led_matrix_config {
	const struct device *row_driver;
	const struct device *col_driver;
	const struct led_matrix_led_config *leds;
	uint32_t refresh_rate_ms;
	uint32_t num_rows;
	uint32_t num_cols;
	uint32_t num_leds;
};

struct led_matrix_data {
	uint8_t brightness_levels[CONFIG_LED_MATRIX_ROWS * CONFIG_LED_MATRIX_COLS];
	/* Protects brightness_levels[] updates together with lit_count. */
	struct k_spinlock lock;
	size_t lit_count;

#ifdef CONFIG_LED_MATRIX_REFRESH_METHOD_THREAD

	struct k_thread refresh_thread;
	struct k_sem any_led_on_sem;

	K_KERNEL_STACK_MEMBER(refresh_thread_stack, CONFIG_LED_MATRIX_THREAD_STACK_SIZE);

#elif defined(CONFIG_LED_MATRIX_REFRESH_METHOD_KTIMER)

	struct k_timer refresh_timer;
	int prev_row;

#endif
};

static size_t led_fb_index(const struct led_matrix_config *config, uint32_t led)
{
	const struct led_matrix_led_config *led_cfg = &config->leds[led];

	return (size_t)led_cfg->row * config->num_cols + led_cfg->col;
}

/**
 * @brief Disable a row of the LED matrix
 *
 * @param dev The LED matrix device
 * @param row The row to disable or -1 if no row is to be disabled.
 *
 * @return 0 on success or negative error code on failure
 */
static int disable_row(const struct device *dev, int row)
{
	const struct led_matrix_config *config = dev->config;

	int ret = 0;

	if (row >= 0) {
		ret = led_off(config->row_driver, row);

		if (ret < 0) {
			LOG_ERR("Failed to turn off LED row %d: %d", row, ret);
		}
	}

	return ret;
}

/**
 * @brief Enable a row of the LED matrix
 *
 * @param dev The LED matrix device
 * @param row The row to enable.
 *
 * @return 0 on success or negative error code on failure
 */
static int enable_row(const struct device *dev, int row)
{
	const struct led_matrix_config *config = dev->config;

	int ret = 0;

	ret = led_on(config->row_driver, row);

	if (ret < 0) {
		LOG_ERR("Failed to turn on LED row %d: %d", row, ret);
	}

	return ret;
}

/**
 * @brief Set the brightness levels of all columns in a row
 *
 * @param dev The LED matrix device
 * @param row The row to set the columns for
 *
 * @return 0 on success or negative error code on failure
 */
static int set_cols(const struct device *dev, int row)
{
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;
	int ret = 0;

#if CONFIG_LED_MATRIX_USE_LED_WRITE_CHANNELS
	ret = led_write_channels(config->col_driver, 0,
		config->num_cols,
		&data->brightness_levels[row * config->num_cols]);

	if (ret < 0) {
		LOG_ERR("Failed to set column brightness levels: %d", ret);
		return ret;
	}
#else /* CONFIG_LED_MATRIX_USE_LED_WRITE_CHANNELS */
	for (size_t col = 0; col < config->num_cols; col++) {
		ret = led_set_brightness(config->col_driver, col,
			data->brightness_levels[row * config->num_cols + col]);

		if (ret < 0) {
			LOG_ERR("Failed to set brightness level for column %zu: %d", col, ret);
			return ret;
		}
	}
#endif /* CONFIG_LED_MATRIX_USE_LED_WRITE_CHANNELS */

	return ret;
}

/**
 * @brief Refresh a single row of the LED matrix
 *
 * @param dev The LED matrix device
 * @param prev_row The previous row to disable or -1 if no row was refreshed yet.
 *
 * @retval 0..num_rows-1 The currently active row
 * @retval LED_MATRIX_REFRESH_SUSPEND Refreshing can be suspended (all LEDs off)
 * @retval negative error code on failure
 */
static int refresh_row(const struct device *dev, int prev_row)
{
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;
	int row = prev_row < 0 ? 0 : (prev_row + 1) % config->num_rows;
	int ret;

	ret = disable_row(dev, prev_row);

	if (ret < 0) {
		return ret;
	}

	ret = set_cols(dev, row);

	if (ret < 0) {
		return ret;
	}

	/*
	 * lit_count is updated under data->lock; this read is lock-free.
	 * A stale zero may return SUSPEND while an LED was just turned on:
	 * - THREAD: k_sem_give() in set_brightness resumes the waiter
	 * - EXTERNAL: caller must keep polling (or track activity itself)
	 * - KTIMER: start/stop uses locked counts
	 */
	if (data->lit_count == 0) {
		LOG_DBG("No LEDs are on, save power");
		return LED_MATRIX_REFRESH_SUSPEND;
	}

	/* Enable the current row */
	ret = enable_row(dev, row);

	if (ret < 0) {
		return ret;
	}

	return row;
}

#if defined(CONFIG_LED_MATRIX_REFRESH_METHOD_THREAD)

static void led_matrix_refresh_thread(void *p1, void *p2, void *p3)
{
	const struct device *dev = (const struct device *)p1;
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;

	while (true) {
		int row = -1;

		while (true) {
			int refresh_result = refresh_row(dev, row);

			if (refresh_result < 0) {
				k_sleep(K_MSEC(config->refresh_rate_ms));
				break;
			}

			if (refresh_result == LED_MATRIX_REFRESH_SUSPEND) {
				LOG_DBG("No LEDs are on, waiting on semaphore");
				k_sem_take(&data->any_led_on_sem, K_FOREVER);
				break;
			}

			row = refresh_result;

			/* Sleep for the refresh rate duration */
			k_sleep(K_MSEC(config->refresh_rate_ms));
		}
	}
}

#elif defined(CONFIG_LED_MATRIX_REFRESH_METHOD_KTIMER)

static void led_matrix_refresh_timer(struct k_timer *timer_id)
{
	const struct device *dev = k_timer_user_data_get(timer_id);
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;
	int ret;

	ret = refresh_row(dev, data->prev_row);

	if (ret >= 0 && ret < config->num_rows) {
		data->prev_row = ret;
	}
}

static void led_matrix_stop_timer(struct k_timer *timer_id)
{
	const struct device *dev = k_timer_user_data_get(timer_id);
	struct led_matrix_data *data = dev->data;

	disable_row(dev, data->prev_row);
	/* Set columns of any row, all are expected to be zeroed now. */
	set_cols(dev, 0);
	data->prev_row = -1;
}

static void start_timer(const struct device *dev)
{
	const struct led_matrix_config *config = dev->config;
	struct led_matrix_data *data = dev->data;

	k_timer_start(&data->refresh_timer, K_NO_WAIT, K_MSEC(config->refresh_rate_ms));
}

static void stop_timer(const struct device *dev)
{
	struct led_matrix_data *data = dev->data;

	k_timer_stop(&data->refresh_timer);
}


#elif defined(CONFIG_LED_MATRIX_REFRESH_METHOD_EXTERNAL)

int led_matrix_refresh(const struct device *dev, int prev_row)
{
	return refresh_row(dev, prev_row);
}

#endif

static int led_matrix_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	struct led_matrix_data *data = dev->data;
	const struct led_matrix_config *config = dev->config;
	k_spinlock_key_t key;
	size_t index;
	uint8_t old_value;
	size_t old_lit_count;
	size_t new_lit_count;

	if (led >= config->num_leds) {
		LOG_ERR("Invalid LED index: %d", led);
		return -EINVAL;
	}

	index = led_fb_index(config, led);

	/* Keep framebuffer cell and lit_count consistent across concurrent callers. */
	key = k_spin_lock(&data->lock);
	old_lit_count = data->lit_count;
	old_value = data->brightness_levels[index];
	data->brightness_levels[index] = value;
	if (old_value == 0U && value > 0U) {
		data->lit_count++;
	} else if (old_value > 0U && value == 0U) {
		data->lit_count--;
	}
	new_lit_count = data->lit_count;
	k_spin_unlock(&data->lock, key);

#if defined(CONFIG_LED_MATRIX_REFRESH_METHOD_THREAD)
	if (value > 0) {
		k_sem_give(&data->any_led_on_sem);
	}
#elif defined(CONFIG_LED_MATRIX_REFRESH_METHOD_KTIMER)
	/* Timer start/stop is not reentrant; see Kconfig. */
	if (old_lit_count == 0 && new_lit_count > 0) {
		start_timer(dev);
	}
	if (old_lit_count > 0 && new_lit_count == 0) {
		stop_timer(dev);
	}
#endif

	return 0;
}

static DEVICE_API(led, led_matrix_api) = {
	.set_brightness = led_matrix_set_brightness,
};

static int led_matrix_init(const struct device *dev)
{
	const struct led_matrix_config *config = dev->config;
#if defined(CONFIG_LED_MATRIX_REFRESH_METHOD_THREAD) || \
	defined(CONFIG_LED_MATRIX_REFRESH_METHOD_KTIMER)
	struct led_matrix_data *data = dev->data;
#endif

	if (config->num_rows == 0) {
		LOG_ERR("Configured number of rows for LED matrix %s is zero",
			dev->name);
		return -EINVAL;
	}

	if (config->num_rows > CONFIG_LED_MATRIX_ROWS) {
		LOG_ERR("Configured number of rows (%d) exceeds maximum allowed (%d)",
			config->num_rows, CONFIG_LED_MATRIX_ROWS);
		return -EINVAL;
	}

	if (config->num_cols == 0) {
		LOG_ERR("Configured number of columns for LED matrix %s is zero",
			dev->name);
		return -EINVAL;
	}

	if (config->num_cols > CONFIG_LED_MATRIX_COLS) {
		LOG_ERR("Configured number of columns (%d) exceeds maximum allowed (%d)",
			config->num_cols, CONFIG_LED_MATRIX_COLS);
		return -EINVAL;
	}

	if (config->num_leds == 0) {
		LOG_ERR("No LEDs found (DT child nodes missing)");
		return -ENODEV;
	}

	for (uint32_t i = 0; i < config->num_leds; i++) {
		const struct led_matrix_led_config *led = &config->leds[i];

		if (led->row >= config->num_rows || led->col >= config->num_cols) {
			LOG_ERR("LED %u position (%u, %u) out of range (%u x %u)", i, led->row,
				led->col, config->num_rows, config->num_cols);
			return -EINVAL;
		}

		for (uint32_t j = 0; j < i; j++) {
			if (config->leds[j].row == led->row && config->leds[j].col == led->col) {
				LOG_ERR("LED %u and %u share position (%u, %u)", j, i, led->row,
					led->col);
				return -EINVAL;
			}
		}
	}

	if (!device_is_ready(config->row_driver)) {
		LOG_ERR("LED Matrix %s row driver %s is not ready", dev->name,
			config->row_driver->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->col_driver)) {
		LOG_ERR("LED Matrix %s col driver %s is not ready", dev->name,
			config->col_driver->name);
		return -ENODEV;
	}

#ifdef CONFIG_LED_MATRIX_REFRESH_METHOD_THREAD
	k_sem_init(&data->any_led_on_sem, 0, 1);
	k_thread_create(&data->refresh_thread, data->refresh_thread_stack,
		CONFIG_LED_MATRIX_THREAD_STACK_SIZE, led_matrix_refresh_thread,
		(void *)dev, NULL, NULL, K_PRIO_PREEMPT(CONFIG_LED_MATRIX_THREAD_PRIORITY),
		0, K_NO_WAIT);
#elif defined(CONFIG_LED_MATRIX_REFRESH_METHOD_KTIMER)
	k_timer_init(&data->refresh_timer, led_matrix_refresh_timer, led_matrix_stop_timer);
	k_timer_user_data_set(&data->refresh_timer, (void *)dev);
	data->prev_row = -1;
#endif

	LOG_INF("LED Matrix initialized");
	return 0;
}

#define LED_MATRIX_LED_CONFIG(node_id)                                                             \
	{                                                                                          \
		.row = DT_PROP(node_id, row),                                                      \
		.col = DT_PROP(node_id, col),                                                      \
	},

#define LED_MATRIX_INIT(inst)                                                                      \
	static const struct led_matrix_led_config led_matrix_leds_##inst[] = {                     \
		DT_INST_FOREACH_CHILD(inst, LED_MATRIX_LED_CONFIG)                                 \
	};                                                                                         \
	static struct led_matrix_data led_matrix_data_##inst;                                      \
	static const struct led_matrix_config led_matrix_config_##inst = {                         \
		.row_driver = DEVICE_DT_GET(DT_INST_PHANDLE(inst, rows)),                          \
		.col_driver = DEVICE_DT_GET(DT_INST_PHANDLE(inst, columns)),                       \
		.leds = led_matrix_leds_##inst,                                                    \
		.refresh_rate_ms = DT_INST_PROP(inst, refresh_rate_ms),                            \
		.num_rows = DT_INST_PROP(inst, num_rows),                                          \
		.num_cols = DT_INST_PROP(inst, num_cols),                                          \
		.num_leds = ARRAY_SIZE(led_matrix_leds_##inst),                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, led_matrix_init, NULL, &led_matrix_data_##inst,                \
			      &led_matrix_config_##inst, POST_KERNEL,                              \
			      CONFIG_LED_MATRIX_INIT_PRIORITY, &led_matrix_api);

DT_INST_FOREACH_STATUS_OKAY(LED_MATRIX_INIT)
