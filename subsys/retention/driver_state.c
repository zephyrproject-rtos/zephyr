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
#include <zephyr/retention/driver_state.h>

LOG_MODULE_REGISTER(driver_state, CONFIG_RETENTION_LOG_LEVEL);

struct retention_driver_state_config {
	const struct device *parent;
	size_t offset;
	size_t size;
};

struct retention_driver_state_data {
	bool valid;
};

#define DT_DRV_COMPAT zephyr_retention_driver_state

static int retention_driver_state_init(const struct device *dev)
{
	const struct retention_driver_state_config *config = dev->config;
	struct retention_driver_state_data *data = dev->data;
	struct retention_driver_state_header header = {
		.initialized = 0,
	};
	int result;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("Parent device is not ready");
		return -ENODEV;
	}

	data->valid = retention_is_valid(config->parent);

	if (data->valid) {
		LOG_DBG("%s: found valid content in retention area", dev->name);

		result = retention_read(config->parent, config->offset, (uint8_t *)&header,
					sizeof(header));
		if (result != 0) {
			LOG_ERR("%s: unable to read from retention area, error %i", dev->name,
				result);
			return result;
		}

		data->valid = header.initialized;
	} else {
		LOG_DBG("%s: found invalid content in retention area", dev->name);

		result = retention_write(config->parent, config->offset, (uint8_t *)&header,
					 sizeof(header));
		if (result != 0) {
			LOG_ERR("%s: unable to write to retention area, error %i", dev->name,
				result);
			return result;
		}
	}

	if (data->valid) {
		LOG_DBG("%s: retention driver state is valid", dev->name);
	} else {
		LOG_DBG("%s: retention driver state is invalid", dev->name);
	}

	return 0;
}

bool retention_driver_state_is_valid(const struct device *dev)
{
	struct retention_driver_state_data *data = dev->data;

	return data->valid;
}

int retention_driver_state_write(const struct device *dev, const uint8_t *buffer, size_t size)
{
	const struct retention_driver_state_config *config = dev->config;
	struct retention_driver_state_data *data = dev->data;
	struct retention_driver_state_header header = {
		.initialized = 1,
	};
	int result;

	result = retention_write(config->parent,
				 config->offset + sizeof(struct retention_driver_state_header),
				 buffer, size);
	if (result != 0) {
		LOG_ERR("%s: unable to write to retention area, error %i", dev->name, result);
		return result;
	}

	result =
		retention_write(config->parent, config->offset, (uint8_t *)&header, sizeof(header));
	if (result != 0) {
		LOG_ERR("%s: unable to write to retention area, error %i", dev->name, result);
		return result;
	}

	data->valid = true;
	return 0;
}

int retention_driver_state_read(const struct device *dev, uint8_t *buffer, size_t size)
{
	const struct retention_driver_state_config *config = dev->config;
	struct retention_driver_state_data *data = dev->data;
	int result;

	if (!data->valid) {
		LOG_ERR("%s: retention driver state is invalid", dev->name);
		return -EIO;
	}

	result = retention_read(config->parent,
				config->offset + sizeof(struct retention_driver_state_header),
				buffer, size);
	if (result != 0) {
		LOG_ERR("%s: unable to read from retention area, %i", dev->name, result);
		return result;
	}

	return 0;
}

static const struct retention_driver_state_api retention_driver_state_api = {
	.is_valid = retention_driver_state_is_valid,
	.read = retention_driver_state_read,
	.write = retention_driver_state_write,
};

#define RETENTION_DRIVER_STATE_DEVICE(inst)                                                        \
	static struct retention_driver_state_data retention_driver_state_data_##inst = {           \
		.valid = false,                                                                    \
	};                                                                                         \
	static const struct retention_driver_state_config retention_driver_state_config_##inst = { \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_INST(inst, DT_DRV_COMPAT))),                  \
		.offset = DT_INST_REG_ADDR(inst),                                                  \
		.size = DT_INST_REG_SIZE(inst),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(                                                                     \
		inst, &retention_driver_state_init, NULL, &retention_driver_state_data_##inst,     \
		&retention_driver_state_config_##inst, POST_KERNEL,                                \
		CONFIG_RETENTION_DRIVER_STATE_INIT_PRIORITY, &retention_driver_state_api);

DT_INST_FOREACH_STATUS_OKAY(RETENTION_DRIVER_STATE_DEVICE)
