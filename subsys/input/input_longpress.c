/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_input_longpress

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_longpress, CONFIG_INPUT_LOG_LEVEL);

struct longpress_config {
	const struct device *input_dev;
	struct longpress_data_entry *entries;
	const uint16_t *input_codes;
	const uint16_t *short_codes;
	const uint16_t *long_codes;
	uint32_t long_delays_ms;
	uint8_t num_codes;
};

struct longpress_data_entry {
	const struct device *dev;
	struct k_work_delayable work;
	uint8_t index;
	bool long_fired;
};

static void longpress_deferred(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct longpress_data_entry *entry = CONTAINER_OF(
			dwork, struct longpress_data_entry, work);
	const struct device *dev = entry->dev;
	const struct longpress_config *cfg = dev->config;
	uint16_t code;

	code = cfg->long_codes[entry->index];

	input_report_key(dev, code, 1, true, K_FOREVER);

	entry->long_fired = true;
}

static void longpress_cb(struct input_event *evt, void *user_data)
{
	const struct device *dev = user_data;
	const struct longpress_config *cfg = dev->config;
	struct longpress_data_entry *entry;
	int i;

	if (evt->type != INPUT_EV_KEY) {
		return;
	}

	for (i = 0; i < cfg->num_codes; i++) {
		if (evt->code == cfg->input_codes[i]) {
			break;
		}
	}
	if (i == cfg->num_codes) {
		LOG_DBG("ignored code %d", evt->code);
		return;
	}

	entry = &cfg->entries[i];

	if (evt->value) {
		entry->long_fired = false;
		k_work_schedule(&entry->work, K_MSEC(cfg->long_delays_ms));
	} else {
		k_work_cancel_delayable(&entry->work);
		if (entry->long_fired) {
			input_report_key(dev, cfg->long_codes[i], 0, true, K_FOREVER);
		} else if (cfg->short_codes != NULL) {
			input_report_key(dev, cfg->short_codes[i], 1, true, K_FOREVER);
			input_report_key(dev, cfg->short_codes[i], 0, true, K_FOREVER);
		}
	}
}

static int longpress_init(const struct device *dev)
{
	const struct longpress_config *cfg = dev->config;

	if (cfg->input_dev && !device_is_ready(cfg->input_dev)) {
		LOG_ERR("input device not ready");
		return -ENODEV;
	}

	for (int i = 0; i < cfg->num_codes; i++) {
		struct longpress_data_entry *entry = &cfg->entries[i];

		entry->dev = dev;
		entry->index = i;
		k_work_init_delayable(&entry->work, longpress_deferred);
	}

	return 0;
}

#define INPUT_LONGPRESS_DEFINE(inst)                                                               \
	BUILD_ASSERT((DT_INST_PROP_LEN(inst, input_codes) ==                                       \
		      DT_INST_PROP_LEN_OR(inst, short_codes, 0)) ||                                \
		      !DT_INST_NODE_HAS_PROP(inst, short_codes));                                  \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, input_codes) == DT_INST_PROP_LEN(inst, long_codes));   \
	                                                                                           \
	INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, input)),           \
				    longpress_cb, (void *)DEVICE_DT_INST_GET(inst),                \
				    longpress_cb_##inst);                                          \
	                                                                                           \
	static const uint16_t longpress_input_codes_##inst[] = DT_INST_PROP(inst, input_codes);    \
	                                                                                           \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, short_codes), (                                     \
	static const uint16_t longpress_short_codes_##inst[] = DT_INST_PROP(inst, short_codes);    \
	));                                                                                        \
	                                                                                           \
	static const uint16_t longpress_long_codes_##inst[] = DT_INST_PROP(inst, long_codes);      \
	                                                                                           \
	static struct longpress_data_entry longpress_data_entries_##inst[DT_INST_PROP_LEN(         \
			inst, input_codes)];                                                       \
	                                                                                           \
	static const struct longpress_config longpress_config_##inst = {                           \
		.input_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, input)),                  \
		.entries = longpress_data_entries_##inst,                                          \
		.input_codes = longpress_input_codes_##inst,                                       \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, short_codes), (                             \
		.short_codes = longpress_short_codes_##inst,                                       \
		))                                                                                 \
		.long_codes = longpress_long_codes_##inst,                                         \
		.num_codes = DT_INST_PROP_LEN(inst, input_codes),                                  \
		.long_delays_ms = DT_INST_PROP(inst, long_delay_ms),                               \
	};                                                                                         \
	                                                                                           \
	DEVICE_DT_INST_DEFINE(inst, longpress_init, NULL,                                          \
			      NULL, &longpress_config_##inst,                                      \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_LONGPRESS_DEFINE)
