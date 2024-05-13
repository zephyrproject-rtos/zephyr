/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/retention/retention.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/device_state.h>

LOG_MODULE_REGISTER(device_state, CONFIG_RETENTION_LOG_LEVEL);

struct device_state_retention_config {
	const struct device *parent;
	size_t offset;
	size_t size;
	size_t device_count;
};

struct device_state_retention_data {
	bool *device_init_done;
};

#define DT_DRV_COMPAT zephyr_device_state_retention

static int device_state_retention_load(const struct device *dev)
{
	const struct device_state_retention_config *config = dev->config;
	struct device_state_retention_data *data = dev->data;
	uint8_t value;
	const size_t block_size = sizeof(value) * 8;
	size_t block_count = config->device_count / block_size;
	int result;
	size_t block_end;

	if (config->device_count % block_size != 0) {
		block_count++;
	}

	for (size_t block = 0; block < block_count; ++block) {
		value = 0;
		block_end = block_size;

		if (block * block_size + block_end > config->device_count) {
			block_end = config->device_count % block_size;
		}

		result = retention_read(config->parent, config->offset, &value, sizeof(value));
		if (result != 0) {
			LOG_ERR("%s: unable to read from retention, error %i", dev->name, result);
			return result;
		}

		for (size_t i = 0; i < block_end; ++i) {
			if ((value & BIT(i)) != 0) {
				data->device_init_done[block * block_size + i] = true;
			}
		}
	}

	return 0;
}

static int device_state_retention_write(const struct device *dev)
{
	const struct device_state_retention_config *config = dev->config;
	struct device_state_retention_data *data = dev->data;
	uint8_t value;
	const size_t block_size = sizeof(value) * 8;
	size_t block_count = config->device_count / block_size;
	int result;
	size_t block_end;

	if (config->device_count % block_size != 0) {
		block_count++;
	}

	for (size_t block = 0; block < block_count; ++block) {
		value = 0;
		block_end = block_size;

		if (block * block_size + block_end > config->device_count) {
			block_end = config->device_count % block_size;
		}

		for (size_t i = 0; i < block_end; ++i) {
			if (data->device_init_done[block * block_size + i]) {
				WRITE_BIT(value, i, true);
			}
		}

		result = retention_write(config->parent, config->offset, &value, sizeof(value));
		if (result != 0) {
			LOG_ERR("%s: unable to write to retention, error %i", dev->name, result);
			return result;
		}
	}

	return 0;
}

static int device_state_retention_init(const struct device *dev)
{
	const struct device_state_retention_config *config = dev->config;
	struct device_state_retention_data *data = dev->data;
	int result;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Parent device is not ready");
		return -ENODEV;
	}

	for (size_t i = 0; i < config->device_count; ++i) {
		data->device_init_done[i] = false;
	}

	if (retention_is_valid(config->parent)) {
		LOG_DBG("%s: found valid content in retention area", dev->name);

		result = device_state_retention_load(dev);
		if (result != 0) {
			return result;
		}
	} else {
		LOG_DBG("%s: found invalid content in retention area", dev->name);

		result = device_state_retention_write(dev);
		if (result != 0) {
			return result;
		}
	}

	return 0;
}

bool device_state_retention_check_reinit(const struct device *dev, size_t index)
{
	struct device_state_retention_data *data = dev->data;

	__ASSERT(
		index < ((const struct device_state_retention_config *)(dev->config))->device_count,
		"invalid index");
	return data->device_init_done[index];
}

void device_state_retention_set_init_done(const struct device *dev, size_t index, bool value)
{
	struct device_state_retention_data *data = dev->data;

	__ASSERT(
		index < ((const struct device_state_retention_config *)(dev->config))->device_count,
		"invalid index");
	data->device_init_done[index] = value;
	device_state_retention_write(dev);
}

static const struct device_state_retention_api device_state_retention_api = {
	.check_reinit = device_state_retention_check_reinit,
	.set_init_done = device_state_retention_set_init_done,
};

#define DEVICE_STATE_RETENTION_DEVICE(inst)                                                        \
	static bool device_state_retention_init_done_##inst[DT_INST_PROP(inst, device_count)];     \
	static struct device_state_retention_data device_state_retention_data_##inst = {           \
		.device_init_done = device_state_retention_init_done_##inst,                       \
	};                                                                                         \
	static const struct device_state_retention_config device_state_retention_config_##inst = { \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),                  \
		.offset = DT_INST_REG_ADDR(inst),                                                  \
		.size = DT_INST_REG_SIZE(inst),                                                    \
		.device_count = DT_INST_PROP(inst, device_count),                                  \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(                                                                     \
		inst, &device_state_retention_init, NULL, &device_state_retention_data_##inst,     \
		&device_state_retention_config_##inst, POST_KERNEL,                                \
		CONFIG_RETENTION_DEVICE_STATE_MODULE_INIT_PRIORITY, &device_state_retention_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_STATE_RETENTION_DEVICE)
