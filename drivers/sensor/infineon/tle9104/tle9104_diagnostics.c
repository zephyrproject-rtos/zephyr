/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_tle9104_diagnostics

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/tle9104.h>
#include <zephyr/logging/log.h>

#include "tle9104_diagnostics.h"

LOG_MODULE_REGISTER(TLE9104_DIAGNOSTICS, CONFIG_SENSOR_LOG_LEVEL);

static int tle9104_diagnostics_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tle9104_diagnostics_config *config = dev->config;
	struct tle9104_diagnostics_data *data = dev->data;
	int result;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);
	result = tle9104_get_diagnostics(config->parent, data->values);
	if (result != 0) {
		return result;
	}

	result = tle9104_clear_diagnostics(config->parent);
	if (result != 0) {
		return result;
	}

	return 0;
}

static int tle9104_diagnostics_channel_get(const struct device *dev, enum sensor_channel chan,
					   struct sensor_value *val)
{
	struct tle9104_diagnostics_data *data = dev->data;

	val->val1 = 0;
	val->val2 = 0;

	switch (chan) {
	case SENSOR_CHAN_TLE9104_OPEN_LOAD:
		for (size_t i = 0; i < ARRAY_SIZE(data->values); ++i) {
			if (data->values[i].off == TLE9104_OFFDIAG_OL) {
				val->val1 |= BIT(i);
			}
		}
		return 0;
	case SENSOR_CHAN_TLE9104_OVER_CURRENT:
		for (size_t i = 0; i < ARRAY_SIZE(data->values); ++i) {
			if (data->values[i].on == TLE9104_ONDIAG_OCTIME ||
			    data->values[i].on == TLE9104_ONDIAG_OCOT) {
				val->val1 |= BIT(i);
			}
		}
		return 0;
	default:
		LOG_ERR("%s: requesting unsupported channel %i", dev->name, chan);
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, tle9104_diagnostics_driver_api) = {
	.sample_fetch = tle9104_diagnostics_sample_fetch,
	.channel_get = tle9104_diagnostics_channel_get,
};

int tle9104_diagnostics_init(const struct device *dev)
{
	const struct tle9104_diagnostics_config *config = dev->config;

	if (!device_is_ready(config->parent)) {
		LOG_ERR("%s: parent device is not ready", dev->name);
		return -ENODEV;
	}

	return 0;
}

#define TLE9104_DIAGNOSTICS_DEFINE(inst)                                                           \
	static struct tle9104_diagnostics_data tle9104_diagnostics_data_##inst;                    \
                                                                                                   \
	static const struct tle9104_diagnostics_config tle9104_diagnostics_config##inst = {        \
		.parent = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                             \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(                                                              \
		inst, tle9104_diagnostics_init, NULL, &tle9104_diagnostics_data_##inst,            \
		&tle9104_diagnostics_config##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,       \
		&tle9104_diagnostics_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TLE9104_DIAGNOSTICS_DEFINE)
