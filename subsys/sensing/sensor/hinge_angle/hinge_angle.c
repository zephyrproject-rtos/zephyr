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

#define HINGE_REPORTER_NUM 2

static struct sensing_sensor_register_info hinge_reg = {
	.flags = SENSING_SENSOR_FLAG_REPORT_ON_CHANGE,
	.sample_size = sizeof(struct sensing_sensor_value_q31),
	.sensitivity_count = 1,
	.version.value = SENSING_SENSOR_VERSION(1, 0, 0, 0),
};

struct hinge_angle_context {
	struct rtio_iodev_sqe *sqe;
	sensing_sensor_handle_t reporters[HINGE_REPORTER_NUM];
	struct sensing_sensor_value_3d_q31 sample[HINGE_REPORTER_NUM];
	int has_sample[HINGE_REPORTER_NUM];
};

static int hinge_init(const struct device *dev)
{
	struct hinge_angle_context *data = dev->data;
	int ret;

	ret = sensing_sensor_get_reporters(dev,
			SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D,
			data->reporters, HINGE_REPORTER_NUM);
	if (ret != HINGE_REPORTER_NUM) {
		LOG_ERR("%s: reporter mismatch:%d", dev->name, ret);
		return -ENODEV;
	}

	LOG_INF("%s:Found reporter 0: %s", dev->name,
			sensing_get_sensor_info(data->reporters[0])->name);
	LOG_INF("%s:Found reporter 1: %s", dev->name,
			sensing_get_sensor_info(data->reporters[1])->name);

	return 0;
}

static int hinge_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	struct sensing_sensor_config config = {0};
	struct hinge_angle_context *data = dev->data;
	int ret = 0;

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		config.attri = SENSING_SENSOR_ATTRIBUTE_INTERVAL;
		config.interval = (uint32_t)(USEC_PER_SEC * 1000LL /
				sensor_value_to_milli(val));
		ret = sensing_set_config(data->reporters[0], &config, 1);
		ret |= sensing_set_config(data->reporters[1], &config, 1);
		break;

	case SENSOR_ATTR_HYSTERESIS:
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	LOG_INF("%s set attr:%d ret:%d", dev->name, attr, ret);
	return ret;
}

static void hinge_submit(const struct device *dev,
		struct rtio_iodev_sqe *sqe)
{
	struct hinge_angle_context *data = dev->data;

	if (data->sqe) {
		rtio_iodev_sqe_err(sqe, -EBUSY);
	} else {
		data->sqe = sqe;
	}
}

static DEVICE_API(sensor, hinge_api) = {
	.attr_set = hinge_attr_set,
	.submit = hinge_submit,
};

static q31_t calc_hinge_angle(struct hinge_angle_context *data)
{
	q31_t val;

	LOG_INF("Acc 0: x:%08x y:%08x z:%08x",
			data->sample[0].readings[0].x,
			data->sample[0].readings[0].y,
			data->sample[0].readings[0].z);
	LOG_INF("Acc 1: x:%08x y:%08x z:%08x",
			data->sample[1].readings[0].x,
			data->sample[1].readings[0].y,
			data->sample[1].readings[0].z);

	/* Todo: calc hinge angle base on data->sample[0] and data->sample[1] */
	val = 0;

	return val;
}

static void hinge_reporter_on_data_event(sensing_sensor_handle_t handle,
		const void *buf, void *context)
{
	struct hinge_angle_context *data = context;
	struct sensing_sensor_value_q31 *sample;
	uint32_t buffer_len = 0;
	int both = 0;
	int i, ret;

	for (i = 0; i < HINGE_REPORTER_NUM; ++i) {
		if (handle == data->reporters[i]) {
			memcpy(&data->sample[i], buf, sizeof(data->sample[i]));
			data->has_sample[i] = 1;
		}
		both += data->has_sample[i];
	}

	if (both == HINGE_REPORTER_NUM) {
		data->has_sample[0] = 0;
		data->has_sample[1] = 0;

		ret = rtio_sqe_rx_buf(data->sqe, sizeof(*sample), sizeof(*sample),
				(uint8_t **)&sample, &buffer_len);
		if (ret) {
			rtio_iodev_sqe_err(data->sqe, ret);
			return;
		}

		sample->readings[0].v = calc_hinge_angle(data);

		struct rtio_iodev_sqe *sqe = data->sqe;

		data->sqe = NULL;
		rtio_iodev_sqe_ok(sqe, 0);
	}
}

#define DT_DRV_COMPAT zephyr_sensing_hinge_angle
#define SENSING_HINGE_ANGLE_DT_DEFINE(_inst)					\
	static struct hinge_angle_context _CONCAT(hinge_ctx, _inst);		\
	static struct sensing_callback_list _CONCAT(hinge_cb, _inst) = {	\
		.on_data_event = hinge_reporter_on_data_event,			\
		.context = &_CONCAT(hinge_ctx, _inst),				\
	};									\
	SENSING_SENSORS_DT_INST_DEFINE(_inst, &hinge_reg,			\
		&_CONCAT(hinge_cb, _inst),					\
		&hinge_init, NULL,						\
		&_CONCAT(hinge_ctx, _inst), NULL,				\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
		&hinge_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_HINGE_ANGLE_DT_DEFINE);
