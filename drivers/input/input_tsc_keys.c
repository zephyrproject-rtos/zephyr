/*
 * Copyright (c) 2024 Arif Balik <arifbalik@outlook.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tsc_keys

#include <stdlib.h>
#include <stdbool.h>
#include <autoconf.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/misc/stm32_tsc/stm32_tsc.h>

LOG_MODULE_REGISTER(tsc_keys, CONFIG_INPUT_LOG_LEVEL);

struct input_tsc_keys_child_data {
	uint32_t buffer[CONFIG_INPUT_STM32_TSC_KEYS_BUFFER_WORD_SIZE];
	struct ring_buf rb;
	bool expect_release;
};

struct input_tsc_keys_child_config {
	uint8_t group_index;
	int32_t noise_threshold;
	int zephyr_code;
};

struct input_tsc_keys_data {
	struct k_timer sampling_timer;
};

struct input_tsc_keys_config {
	const struct device *tsc_dev;
	uint32_t sampling_interval_ms;
	const struct input_tsc_keys_child_config *children_cfg;
	struct input_tsc_keys_child_data *children_data;
	uint8_t child_count;
};

static void input_tsc_sampling_timer_callback(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);

	stm32_tsc_start(dev);
}

static inline int input_tsc_child_index_get(const struct device *dev, uint8_t group)
{
	const struct input_tsc_keys_config *config = dev->config;

	for (int i = 0; i < config->child_count; i++) {
		const struct input_tsc_keys_child_config *child = &config->children_cfg[i];

		if (child->group_index == group) {
			return i;
		}
	}

	return -ENODEV;
}

static void input_tsc_callback_handler(uint8_t group, uint32_t value, void *user_data)
{
	const struct device *dev = (const struct device *)user_data;
	const struct input_tsc_keys_config *config = dev->config;

	const int child_index = input_tsc_child_index_get(dev, group);

	if (child_index < 0) {
		LOG_ERR("%s: No child config for group %d", config->tsc_dev->name, group);
		return;
	}

	const struct input_tsc_keys_child_config *child_config = &config->children_cfg[child_index];
	struct input_tsc_keys_child_data *child_data = &config->children_data[child_index];

	if (ring_buf_item_space_get(&child_data->rb) == 0) {
		uint32_t oldest_point;
		int32_t slope;

		(void)ring_buf_get(&child_data->rb, (uint8_t *)&oldest_point, sizeof(oldest_point));

		slope = value - oldest_point;
		if (slope < -child_config->noise_threshold && !child_data->expect_release) {
			child_data->expect_release = true;
			input_report_key(dev, child_config->zephyr_code, 1, false, K_NO_WAIT);
		} else if (slope > child_config->noise_threshold && child_data->expect_release) {
			child_data->expect_release = false;
			input_report_key(dev, child_config->zephyr_code, 0, false, K_NO_WAIT);
		}
	}

	(void)ring_buf_put(&child_data->rb, (uint8_t *)&value, sizeof(value));
}

static int input_tsc_keys_init(const struct device *dev)
{
	const struct input_tsc_keys_config *config = dev->config;
	struct input_tsc_keys_data *data = dev->data;

	if (!device_is_ready(config->tsc_dev)) {
		LOG_ERR("%s: TSC device not ready", config->tsc_dev->name);
		return -ENODEV;
	}

	for (uint8_t i = 0; i < config->child_count; i++) {
		struct input_tsc_keys_child_data *child_data = &config->children_data[i];

		ring_buf_item_init(&child_data->rb, CONFIG_INPUT_STM32_TSC_KEYS_BUFFER_WORD_SIZE,
				   child_data->buffer);
	}

	stm32_tsc_register_callback(config->tsc_dev, input_tsc_callback_handler, (void *)dev);

	k_timer_init(&data->sampling_timer, input_tsc_sampling_timer_callback, NULL);
	k_timer_user_data_set(&data->sampling_timer, (void *)config->tsc_dev);
	k_timer_start(&data->sampling_timer, K_NO_WAIT, K_MSEC(config->sampling_interval_ms));

	return 0;
}

#define TSC_KEYS_CHILD(node_id)                                                                    \
	{                                                                                          \
		.group_index = DT_PROP(DT_PHANDLE(node_id, group), group),                         \
		.zephyr_code = DT_PROP(node_id, zephyr_code),                                      \
		.noise_threshold = DT_PROP(node_id, noise_threshold),                              \
	},

#define TSC_KEYS_INIT(node_id)                                                                     \
	static struct input_tsc_keys_child_data                                                    \
		tsc_keys_child_data_##node_id[DT_INST_CHILD_NUM_STATUS_OKAY(node_id)];             \
	static const struct input_tsc_keys_child_config tsc_keys_children_##node_id[] = {          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(node_id, TSC_KEYS_CHILD)};                       \
                                                                                                   \
	static struct input_tsc_keys_data tsc_keys_data_##node_id;                                 \
                                                                                                   \
	static const struct input_tsc_keys_config tsc_keys_config_##node_id = {                    \
		.tsc_dev = DEVICE_DT_GET(DT_INST_PHANDLE(node_id, tsc_controller)),                \
		.sampling_interval_ms = DT_INST_PROP(node_id, sampling_interval_ms),               \
		.child_count = DT_INST_CHILD_NUM_STATUS_OKAY(node_id),                             \
		.children_cfg = tsc_keys_children_##node_id,                                       \
		.children_data = tsc_keys_child_data_##node_id,                                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(node_id, input_tsc_keys_init, NULL, &tsc_keys_data_##node_id,        \
			      &tsc_keys_config_##node_id, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(TSC_KEYS_INIT);
