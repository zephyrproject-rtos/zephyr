/*
 * Copyright 2024 Kelly Helmut Lord
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_input_double_tap

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_double_tap, CONFIG_INPUT_LOG_LEVEL);

/* Values of the first_tap atomic. */
enum first_tap_state {
	FIRST_TAP_IDLE = 0,
	FIRST_TAP_PRESSED,
	FIRST_TAP_RELEASED,
};

struct double_tap_config {
	const struct device *input_dev;
	struct double_tap_data_entry *entries;
	const uint16_t *input_codes;
	const uint16_t *single_tap_codes;
	const uint16_t *double_tap_codes;
	uint32_t double_tap_delay_ms;
	uint8_t num_codes;
};

struct double_tap_data_entry {
	const struct device *dev;
	struct k_work_delayable work;
	uint8_t index;
	atomic_t first_tap;
};

static void double_tap_deferred(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct double_tap_data_entry *entry =
		CONTAINER_OF(dwork, struct double_tap_data_entry, work);
	const struct device *dev = entry->dev;
	const struct double_tap_config *cfg = dev->config;

	if (atomic_set(&entry->first_tap, FIRST_TAP_IDLE) != FIRST_TAP_RELEASED) {
		return;
	}

	if (cfg->single_tap_codes == NULL) {
		return;
	}

	input_report_key(dev, cfg->single_tap_codes[entry->index], 1, true, K_FOREVER);
	input_report_key(dev, cfg->single_tap_codes[entry->index], 0, true, K_FOREVER);
}

static void double_tap_cb(struct input_event *evt, void *user_data)
{
	const struct device *dev = user_data;
	const struct double_tap_config *cfg = dev->config;
	struct double_tap_data_entry *entry;
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
		if (atomic_set(&entry->first_tap, FIRST_TAP_IDLE) != FIRST_TAP_IDLE) {
			k_work_cancel_delayable(&entry->work);
			input_report_key(dev, cfg->double_tap_codes[i], 1, true, K_FOREVER);
			input_report_key(dev, cfg->double_tap_codes[i], 0, true, K_FOREVER);
		} else {
			atomic_set(&entry->first_tap, FIRST_TAP_PRESSED);
			k_work_schedule(&entry->work, K_MSEC(cfg->double_tap_delay_ms));
		}
	} else {
		(void)atomic_cas(&entry->first_tap, FIRST_TAP_PRESSED, FIRST_TAP_RELEASED);
	}
}

static int double_tap_init(const struct device *dev)
{
	const struct double_tap_config *cfg = dev->config;

	if (cfg->input_dev && !device_is_ready(cfg->input_dev)) {
		LOG_ERR("input device not ready");
		return -ENODEV;
	}

	for (int i = 0; i < cfg->num_codes; i++) {
		struct double_tap_data_entry *entry = &cfg->entries[i];

		entry->dev = dev;
		entry->index = i;
		atomic_set(&entry->first_tap, FIRST_TAP_IDLE);
		k_work_init_delayable(&entry->work, double_tap_deferred);
	}

	return 0;
}

#define INPUT_DOUBLE_TAP_DEFINE(inst)                                                              \
	BUILD_ASSERT((DT_INST_PROP_LEN(inst, input_codes) ==                                       \
		      DT_INST_PROP_LEN_OR(inst, single_tap_codes, 0)) ||                           \
		      !DT_INST_NODE_HAS_PROP(inst, single_tap_codes));                             \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, input_codes) ==                                        \
		     DT_INST_PROP_LEN(inst, double_tap_codes));                                    \
                                                                                                   \
	INPUT_CALLBACK_DEFINE_NAMED(DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, input)),           \
				    double_tap_cb, (void *)DEVICE_DT_INST_GET(inst),               \
				    double_tap_cb_##inst);                                         \
                                                                                                   \
	static const uint16_t double_tap_input_codes_##inst[] = DT_INST_PROP(inst, input_codes);   \
                                                                                                   \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, single_tap_codes), (                                \
	static const uint16_t single_tap_codes_##inst[] = DT_INST_PROP(inst, single_tap_codes);    \
	));                                                                                        \
                                                                                                   \
	static const uint16_t double_tap_codes_##inst[] = DT_INST_PROP(inst, double_tap_codes);    \
                                                                                                   \
	static struct double_tap_data_entry                                                        \
		double_tap_data_entries_##inst[DT_INST_PROP_LEN(inst, input_codes)];               \
                                                                                                   \
	static const struct double_tap_config double_tap_config_##inst = {                         \
		.input_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, input)),                  \
		.entries = double_tap_data_entries_##inst,                                         \
		.input_codes = double_tap_input_codes_##inst,                                      \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, single_tap_codes), (                        \
		.single_tap_codes = single_tap_codes_##inst,                                       \
		))                                                                                 \
		.double_tap_codes = double_tap_codes_##inst,                                       \
		.num_codes = DT_INST_PROP_LEN(inst, input_codes),                                  \
		.double_tap_delay_ms = DT_INST_PROP(inst, double_tap_delay_ms),                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, double_tap_init, NULL, NULL, &double_tap_config_##inst,        \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_DOUBLE_TAP_DEFINE)
