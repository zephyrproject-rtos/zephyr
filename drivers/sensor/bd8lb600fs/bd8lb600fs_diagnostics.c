/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bd8lb600fs_diagnostics

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/mfd/bd8lb600fs.h>
#include <zephyr/drivers/sensor/bd8lb600fs.h>
#include <zephyr/logging/log.h>

#include "bd8lb600fs_diagnostics.h"

LOG_MODULE_REGISTER(BD8LB600FS_DIAGNOSTICS, CONFIG_SENSOR_LOG_LEVEL);

static int bd8lb600fs_diagnostics_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct bd8lb600fs_diagnostics_config *config = dev->config;
	struct bd8lb600fs_diagnostics_data *data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	return mfd_bd8lb600fs_get_output_diagnostics(config->parent_dev, &data->old,
						     &data->ocp_or_tsd);
}

static int bd8lb600fs_diagnostics_channel_get(const struct device *dev, enum sensor_channel chan,
					      struct sensor_value *val)
{
	struct bd8lb600fs_diagnostics_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_BD8LB600FS_OPEN_LOAD:
		val->val1 = data->old;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_BD8LB600FS_OVER_CURRENT_OR_THERMAL_SHUTDOWN:
		val->val1 = data->ocp_or_tsd;
		val->val2 = 0;
		return 0;
	default:
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api bd8lb600fs_diagnostics_driver_api = {
	.sample_fetch = bd8lb600fs_diagnostics_sample_fetch,
	.channel_get = bd8lb600fs_diagnostics_channel_get,
};

static int bd8lb600fs_diagnostics_init(const struct device *dev)
{
	const struct bd8lb600fs_diagnostics_config *config = dev->config;

	if (!device_is_ready(config->parent_dev)) {
		LOG_ERR("%s: parent device is not ready", dev->name);
		return -ENODEV;
	}

	return 0;
}

#define BD8LB600FS_DIAGNOSTICS_DEFINE(inst)                                                        \
	static struct bd8lb600fs_diagnostics_data bd8lb600fs_diagnostics_data_##inst;              \
                                                                                                   \
	static const struct bd8lb600fs_diagnostics_config bd8lb600fs_diagnostics_config_##inst = { \
		.parent_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		inst, bd8lb600fs_diagnostics_init, NULL, &bd8lb600fs_diagnostics_data_##inst,      \
		&bd8lb600fs_diagnostics_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,   \
		&bd8lb600fs_diagnostics_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BD8LB600FS_DIAGNOSTICS_DEFINE)
