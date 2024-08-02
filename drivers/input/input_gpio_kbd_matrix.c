/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_kbd_matrix

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_gpio_kbd_matrix, CONFIG_INPUT_LOG_LEVEL);

struct gpio_kbd_matrix_config {
	struct input_kbd_matrix_common_config common;
	const struct gpio_dt_spec *row_gpio;
	const struct gpio_dt_spec *col_gpio;

	struct gpio_callback *gpio_cb;
	gpio_callback_handler_t gpio_cb_handler;

	struct k_work_delayable *idle_poll_dwork;
	k_work_handler_t idle_poll_handler;

	bool col_drive_inactive;
};

struct gpio_kbd_matrix_data {
	struct input_kbd_matrix_common_data common;
	uint32_t last_col_state;
	bool direct_read;
	bool direct_write;
};

INPUT_KBD_STRUCT_CHECK(struct gpio_kbd_matrix_config,
		       struct gpio_kbd_matrix_data);

static void gpio_kbd_matrix_drive_column(const struct device *dev, int col)
{
	const struct gpio_kbd_matrix_config *cfg = dev->config;
	const struct input_kbd_matrix_common_config *common = &cfg->common;
	struct gpio_kbd_matrix_data *data = dev->data;
	uint32_t state;

	if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_NONE) {
		state = 0;
	} else if (col == INPUT_KBD_MATRIX_COLUMN_DRIVE_ALL) {
		state = BIT_MASK(common->col_size);
	} else {
		state = BIT(col);
	}

	if (data->direct_write) {
		const struct gpio_dt_spec *gpio0 = &cfg->col_gpio[0];
		gpio_port_pins_t gpio_mask;
		gpio_port_value_t gpio_val;

		gpio_mask = BIT_MASK(common->col_size) << gpio0->pin;
		gpio_val = state << gpio0->pin;

		gpio_port_set_masked(gpio0->port, gpio_mask, gpio_val);

		return;
	}

	for (int i = 0; i < common->col_size; i++) {
		const struct gpio_dt_spec *gpio = &cfg->col_gpio[i];

		if ((data->last_col_state ^ state) & BIT(i)) {
			if (cfg->col_drive_inactive) {
				gpio_pin_set_dt(gpio, state & BIT(i));
			} else if (state & BIT(i)) {
				gpio_pin_configure_dt(gpio, GPIO_OUTPUT_ACTIVE);
			} else {
				gpio_pin_configure_dt(gpio, GPIO_INPUT);
			}
		}
	}

	data->last_col_state = state;
}

static kbd_row_t gpio_kbd_matrix_read_row(const struct device *dev)
{
	const struct gpio_kbd_matrix_config *cfg = dev->config;
	const struct input_kbd_matrix_common_config *common = &cfg->common;
	struct gpio_kbd_matrix_data *data = dev->data;
	kbd_row_t val = 0;

	if (data->direct_read) {
		const struct gpio_dt_spec *gpio0 = &cfg->row_gpio[0];
		gpio_port_value_t gpio_val;

		gpio_port_get(gpio0->port, &gpio_val);

		return (gpio_val >> gpio0->pin) & BIT_MASK(common->row_size);
	}

	for (int i = 0; i < common->row_size; i++) {
		const struct gpio_dt_spec *gpio = &cfg->row_gpio[i];

		if (gpio_pin_get_dt(gpio)) {
			val |= BIT(i);
		}
	}

	return val;
}

static __maybe_unused void gpio_kbd_matrix_idle_poll_handler(const struct device *dev)
{
	const struct gpio_kbd_matrix_config *cfg = dev->config;
	const struct input_kbd_matrix_common_config *common = &cfg->common;

	if (gpio_kbd_matrix_read_row(dev) == 0) {
		k_work_reschedule(cfg->idle_poll_dwork,
				  K_USEC(common->poll_period_us));
		return;
	}

	input_kbd_matrix_poll_start(dev);
}

static void gpio_kbd_matrix_set_detect_mode(const struct device *dev, bool enabled)
{
	const struct gpio_kbd_matrix_config *cfg = dev->config;
	const struct input_kbd_matrix_common_config *common = &cfg->common;
	int ret;

	if (cfg->idle_poll_dwork != NULL) {
		if (enabled) {
			k_work_reschedule(cfg->idle_poll_dwork,
					  K_USEC(common->poll_period_us));
		}
		return;
	}

	if (cfg->gpio_cb == NULL) {
		return;
	}

	for (int i = 0; i < common->row_size; i++) {
		const struct gpio_dt_spec *gpio = &cfg->row_gpio[i];
		gpio_flags_t flags = enabled ? GPIO_INT_EDGE_TO_ACTIVE : GPIO_INT_DISABLE;

		ret = gpio_pin_interrupt_configure_dt(gpio, flags);
		if (ret != 0) {
			LOG_ERR("Pin %d interrupt configuration failed: %d", i, ret);
			return;
		}
	}
}

static bool gpio_kbd_matrix_is_gpio_coherent(
		const struct gpio_dt_spec *gpio, int gpio_count)
{
	const struct gpio_dt_spec *gpio0 = &gpio[0];

	for (int i = 1; i < gpio_count; i++) {
		if (gpio[i].port != gpio0->port ||
		    gpio[i].dt_flags != gpio0->dt_flags ||
		    gpio[i].pin != gpio0->pin + i) {
			return false;
		}
	}

	return true;
}

static bool gpio_kbd_continuous_scan_mode(const struct device *dev)
{
	const struct gpio_kbd_matrix_config *cfg = dev->config;

	if (cfg->gpio_cb == NULL && cfg->idle_poll_dwork == NULL) {
		return true;
	}

	return false;
}

static int gpio_kbd_matrix_init(const struct device *dev)
{
	const struct gpio_kbd_matrix_config *cfg = dev->config;
	const struct input_kbd_matrix_common_config *common = &cfg->common;
	struct gpio_kbd_matrix_data *data = dev->data;
	int ret;
	int i;

	for (i = 0; i < common->col_size; i++) {
		const struct gpio_dt_spec *gpio = &cfg->col_gpio[i];

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("%s is not ready", gpio->port->name);
			return -ENODEV;
		}

		if (cfg->col_drive_inactive) {
			ret = gpio_pin_configure_dt(gpio, GPIO_OUTPUT_INACTIVE);
		} else {
			ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
		}
		if (ret != 0) {
			LOG_ERR("Pin %d configuration failed: %d", i, ret);
			return ret;
		}
	}

	for (i = 0; i < common->row_size; i++) {
		const struct gpio_dt_spec *gpio = &cfg->row_gpio[i];
		struct gpio_callback *gpio_cb;

		if (!gpio_is_ready_dt(gpio)) {
			LOG_ERR("%s is not ready", gpio->port->name);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
		if (ret != 0) {
			LOG_ERR("Pin %d configuration failed: %d", i, ret);
			return ret;
		}

		if (cfg->gpio_cb == NULL) {
			continue;
		}
		gpio_cb = &cfg->gpio_cb[i];

		gpio_init_callback(gpio_cb, cfg->gpio_cb_handler,
				   BIT(gpio->pin));

		ret = gpio_add_callback_dt(gpio, gpio_cb);
		if (ret < 0) {
			LOG_ERR("Could not set gpio callback");
			return ret;
		}
	}

	if (cfg->idle_poll_dwork != NULL) {
		k_work_init_delayable(cfg->idle_poll_dwork,
				      cfg->idle_poll_handler);
	}

	data->direct_read = gpio_kbd_matrix_is_gpio_coherent(
			cfg->row_gpio, common->row_size);

	if (cfg->col_drive_inactive) {
		data->direct_write = gpio_kbd_matrix_is_gpio_coherent(
				cfg->col_gpio, common->col_size);
	}

	LOG_DBG("direct_read: %d direct_write: %d",
		data->direct_read, data->direct_write);

	ret = input_kbd_matrix_common_init(dev);
	if (ret != 0) {
		return ret;
	}

	if (gpio_kbd_continuous_scan_mode(dev)) {
		input_kbd_matrix_poll_start(dev);
	}

	return 0;
}

static const struct input_kbd_matrix_api gpio_kbd_matrix_api = {
	.drive_column = gpio_kbd_matrix_drive_column,
	.read_row = gpio_kbd_matrix_read_row,
	.set_detect_mode = gpio_kbd_matrix_set_detect_mode,
};

#define INPUT_GPIO_KBD_MATRIX_INIT(n)								\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, col_gpios) <= 32, "invalid col-size");			\
												\
	INPUT_KBD_MATRIX_DT_INST_DEFINE_ROW_COL(						\
		n, DT_INST_PROP_LEN(n, row_gpios), DT_INST_PROP_LEN(n, col_gpios));		\
												\
	static const struct gpio_dt_spec gpio_kbd_matrix_row_gpio_##n[DT_INST_PROP_LEN(		\
			n, row_gpios)] = {							\
		DT_INST_FOREACH_PROP_ELEM_SEP(n, row_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))	\
	};											\
	static const struct gpio_dt_spec gpio_kbd_matrix_col_gpio_##n[DT_INST_PROP_LEN(		\
			n, col_gpios)] = {							\
		DT_INST_FOREACH_PROP_ELEM_SEP(n, col_gpios, GPIO_DT_SPEC_GET_BY_IDX, (,))	\
	};											\
												\
	IF_ENABLED(DT_INST_ENUM_HAS_VALUE(n, idle_mode, interrupt), (				\
	static struct gpio_callback gpio_kbd_matrix_gpio_cb_##n[DT_INST_PROP_LEN(n, row_gpios)];\
	static void gpio_kbd_matrix_cb_##n(const struct device *gpio_dev,			\
					   struct gpio_callback *cb, uint32_t pins)		\
	{											\
		input_kbd_matrix_poll_start(DEVICE_DT_INST_GET(n));				\
	}											\
	))											\
	IF_ENABLED(DT_INST_ENUM_HAS_VALUE(n, idle_mode, poll), (				\
	static struct k_work_delayable gpio_kbd_matrix_idle_poll_dwork_##n;			\
	static void gpio_kbd_matrix_idle_poll_handler_##n(struct k_work *work)			\
	{											\
		gpio_kbd_matrix_idle_poll_handler(DEVICE_DT_INST_GET(n));			\
	}											\
	))											\
	IF_ENABLED(DT_INST_ENUM_HAS_VALUE(n, idle_mode, scan), (				\
	BUILD_ASSERT(DT_INST_PROP(n, poll_timeout_ms) == 0,					\
		     "poll-timeout-ms must be set to 0 for scan mode to work correctly");	\
	))											\
												\
	static const struct gpio_kbd_matrix_config gpio_kbd_matrix_cfg_##n = {			\
		.common = INPUT_KBD_MATRIX_DT_INST_COMMON_CONFIG_INIT_ROW_COL(			\
			n, &gpio_kbd_matrix_api,						\
			DT_INST_PROP_LEN(n, row_gpios), DT_INST_PROP_LEN(n, col_gpios)),	\
		.row_gpio = gpio_kbd_matrix_row_gpio_##n,					\
		.col_gpio = gpio_kbd_matrix_col_gpio_##n,					\
		IF_ENABLED(DT_INST_ENUM_HAS_VALUE(n, idle_mode, interrupt), (			\
		.gpio_cb = gpio_kbd_matrix_gpio_cb_##n,						\
		.gpio_cb_handler = gpio_kbd_matrix_cb_##n,					\
		))										\
		IF_ENABLED(DT_INST_ENUM_HAS_VALUE(n, idle_mode, poll), (			\
		.idle_poll_dwork = &gpio_kbd_matrix_idle_poll_dwork_##n,			\
		.idle_poll_handler = gpio_kbd_matrix_idle_poll_handler_##n,			\
		))										\
		.col_drive_inactive = DT_INST_PROP(n, col_drive_inactive),			\
	};											\
												\
	static struct gpio_kbd_matrix_data gpio_kbd_matrix_data_##n;				\
												\
	DEVICE_DT_INST_DEFINE(n, gpio_kbd_matrix_init, NULL,					\
			      &gpio_kbd_matrix_data_##n, &gpio_kbd_matrix_cfg_##n,		\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,				\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_GPIO_KBD_MATRIX_INIT)
