/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT zephyr_kscan_input

#include <zephyr/drivers/kscan.h>
#include <zephyr/input/input.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(kscan_input, CONFIG_KSCAN_LOG_LEVEL);

struct kscan_input_config {
	const struct device *input_dev;
};

struct kscan_input_data {
	bool enabled;
	kscan_callback_t callback;
	int row;
	int col;
	bool pressed;
};

static void kscan_input_cb(const struct device *dev, struct input_event *evt)
{
	struct kscan_input_data *data = dev->data;

	switch (evt->code) {
	case INPUT_ABS_X:
		data->col = evt->value;
		break;
	case INPUT_ABS_Y:
		data->row = evt->value;
		break;
	case INPUT_BTN_TOUCH:
		data->pressed = evt->value;
		break;
	}

	if (evt->sync) {
		LOG_DBG("input event: %3d %3d %d",
			data->row, data->col, data->pressed);
		if (data->callback) {
			data->callback(dev, data->row, data->col, data->pressed);
		}
	}
}

static int kscan_input_configure(const struct device *dev,
				      kscan_callback_t callback)
{
	struct kscan_input_data *data = dev->data;

	if (!callback) {
		LOG_ERR("Invalid callback (NULL)");
		return -EINVAL;
	}

	data->callback = callback;

	return 0;
}

static int kscan_input_enable_callback(const struct device *dev)
{
	struct kscan_input_data *data = dev->data;

	data->enabled = true;

	return 0;
}

static int kscan_input_disable_callback(const struct device *dev)
{
	struct kscan_input_data *data = dev->data;

	data->enabled = false;

	return 0;
}

static int kscan_input_init(const struct device *dev)
{
	const struct kscan_input_config *cfg = dev->config;

	if (!device_is_ready(cfg->input_dev)) {
		LOG_ERR("Input device not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct kscan_driver_api kscan_input_driver_api = {
	.config = kscan_input_configure,
	.enable_callback = kscan_input_enable_callback,
	.disable_callback = kscan_input_disable_callback,
};

#define KSCAN_INPUT_INIT(index) \
	static void kscan_input_cb_##index(struct input_event *evt)            \
	{                                                                      \
		kscan_input_cb(DEVICE_DT_GET(DT_INST(index, DT_DRV_COMPAT)),   \
			       evt);                                           \
	}                                                                      \
	INPUT_CALLBACK_DEFINE(DEVICE_DT_GET(DT_INST_PARENT(index)),            \
			      kscan_input_cb_##index);                         \
	static const struct kscan_input_config kscan_input_config_##index = {  \
		.input_dev = DEVICE_DT_GET(DT_INST_PARENT(index)),             \
	};                                                                     \
	static struct kscan_input_data kscan_input_data_##index;               \
	DEVICE_DT_INST_DEFINE(index, kscan_input_init, NULL,                   \
			      &kscan_input_data_##index,                       \
			      &kscan_input_config_##index,                     \
			      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY,         \
			      &kscan_input_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KSCAN_INPUT_INIT)
