/*
 * Copyright 2024 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT input_keymap

#include <zephyr/device.h>
#include <zephyr/dt-bindings/input/keymap.h>
#include <zephyr/input/input.h>
#include <zephyr/input/input_keymap.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_keymap, CONFIG_INPUT_LOG_LEVEL);

struct keymap_config {
	const struct device *input_dev;
	const uint16_t *codes;
	uint32_t num_codes;
	uint8_t row_size;
	uint8_t col_size;
};

struct keymap_data {
	uint32_t row;
	uint32_t col;
	bool pressed;
};

static void keymap_cb(const struct device *dev, struct input_event *evt)
{
	const struct keymap_config *cfg = dev->config;
	struct keymap_data *data = dev->data;
	const uint16_t *codes = cfg->codes;
	uint32_t offset;

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

	if (!evt->sync) {
		return;
	}

	if (data->row >= cfg->row_size ||
	    data->col >= cfg->col_size) {
		LOG_WRN("keymap event out of range: row=%u col=%u", data->row, data->col);
		return;
	}

	offset = (data->row * cfg->col_size) + data->col;

	if (offset >= cfg->num_codes || codes[offset] == 0) {
		LOG_DBG("keymap event undefined: row=%u col=%u", data->row, data->col);
		return;
	}

	LOG_DBG("input event: %3u %3u %d", data->row, data->col, data->pressed);

	input_report_key(dev, codes[offset], data->pressed, true, K_FOREVER);
}

static int keymap_init(const struct device *dev)
{
	const struct keymap_config *cfg = dev->config;

	if (!device_is_ready(cfg->input_dev)) {
		LOG_ERR("input device not ready");
		return -ENODEV;
	}

	return 0;
}

#define KEYMAP_ENTRY_OFFSET(keymap_entry, col_size) \
	(MATRIX_ROW(keymap_entry) * col_size + MATRIX_COL(keymap_entry))

#define KEYMAP_ENTRY_CODE(keymap_entry) (keymap_entry & 0xffff)

#define KEYMAP_ENTRY_VALIDATE(node_id, prop, idx)			\
	BUILD_ASSERT(MATRIX_ROW(DT_PROP_BY_IDX(node_id, prop, idx)) <	\
		     DT_PROP(node_id, row_size), "invalid row");	\
	BUILD_ASSERT(MATRIX_COL(DT_PROP_BY_IDX(node_id, prop, idx)) <	\
		     DT_PROP(node_id, col_size), "invalid col");

#define CODES_INIT(node_id, prop, idx) \
	[KEYMAP_ENTRY_OFFSET(DT_PROP_BY_IDX(node_id, prop, idx), DT_PROP(node_id, col_size))] = \
		KEYMAP_ENTRY_CODE(DT_PROP_BY_IDX(node_id, prop, idx)),

#define INPUT_KEYMAP_DEFINE(inst)								\
	static void keymap_cb_##inst(struct input_event *evt)					\
	{											\
		keymap_cb(DEVICE_DT_INST_GET(inst), evt);					\
	}											\
	INPUT_CALLBACK_DEFINE(DEVICE_DT_GET_OR_NULL(DT_INST_PARENT(inst)), keymap_cb_##inst);	\
												\
	DT_INST_FOREACH_PROP_ELEM(inst, keymap, KEYMAP_ENTRY_VALIDATE)				\
												\
	static const uint16_t keymap_codes_##inst[] = {						\
		DT_INST_FOREACH_PROP_ELEM(inst, keymap, CODES_INIT)				\
	};											\
												\
	static const struct keymap_config keymap_config_##inst = {				\
		.input_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),				\
		.codes = keymap_codes_##inst,							\
		.num_codes = ARRAY_SIZE(keymap_codes_##inst),					\
		.row_size = DT_INST_PROP(inst, row_size),					\
		.col_size = DT_INST_PROP(inst, col_size),					\
	};											\
												\
	static struct keymap_data keymap_data_##inst;						\
												\
	DEVICE_DT_INST_DEFINE(inst, keymap_init, NULL,						\
			      &keymap_data_##inst, &keymap_config_##inst,			\
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_KEYMAP_DEFINE)
