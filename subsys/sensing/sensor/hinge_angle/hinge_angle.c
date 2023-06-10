/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sensing/sensing_sensor.h>

LOG_MODULE_REGISTER(hinge_angle, CONFIG_SENSING_LOG_LEVEL);

static struct sensing_sensor_register_info hinge_reg = {
	.flags = SENSING_SENSOR_FLAG_REPORT_ON_CHANGE,
	.sample_size = sizeof(struct sensing_sensor_value_q31),
	.sensitivity_count = 1,
	.version.value = SENSING_SENSOR_VERSION(1, 0, 0, 0),
};

struct hinge_angle_context {
	uint32_t interval;
	uint32_t sensitivity;
	int32_t base_acc_handle;
	int32_t lid_acc_handle;
	void *algo_handle;
};

static int hinge_init(const struct device *dev,
	const struct sensing_sensor_info *info, const sensing_sensor_handle_t *reporter_handles,
	int32_t reporters_count)
{
	return 0;
}

static int hinge_set_interval(const struct device *dev, uint32_t value)
{
	return 0;
}

static int hinge_get_interval(const struct device *dev, uint32_t *value)
{
	return 0;
}

static int hinge_set_sensitivity(const struct device *dev, int index,
	uint32_t value)
{
	return 0;
}

static int hinge_get_sensitivity(const struct device *dev, int index,
	uint32_t *value)
{
	return 0;
}

static int hinge_process(const struct device *dev,
			 const sensing_sensor_handle_t reporter_handle,
			 void *buf,
			 int32_t size)
{
	return 0;
}

static int hinge_sensitivity_test(const struct device *dev, int index,
				  uint32_t sensitivity, void *last_sample_buf,
				  int last_sample_size, void *current_sample_buf,
				  int current_sample_size)
{
	return 0;
}

static const struct sensing_sensor_api hinge_api = {
	.init = hinge_init,
	.get_interval = hinge_get_interval,
	.set_interval = hinge_set_interval,
	.get_sensitivity = hinge_get_sensitivity,
	.set_sensitivity = hinge_set_sensitivity,
	.process = hinge_process,
	.sensitivity_test = hinge_sensitivity_test,
};

#define DT_DRV_COMPAT zephyr_sensing_hinge_angle
#define SENSING_HINGE_ANGLE_DT_DEFINE(_inst) \
	static struct hinge_angle_context _CONCAT(hinge_ctx, _inst) = { 0 }; \
	SENSING_SENSOR_DT_DEFINE(DT_DRV_INST(_inst), &hinge_reg, \
		&_CONCAT(hinge_ctx, _inst), &hinge_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_HINGE_ANGLE_DT_DEFINE);
