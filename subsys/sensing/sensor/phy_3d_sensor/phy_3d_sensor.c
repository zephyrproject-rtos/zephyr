/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#define DT_DRV_COMPAT zephyr_sensing_phy_3d_sensor

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sensing/sensing.h>
#include <zephyr/sensing/sensing_sensor.h>
#include <zephyr/sys/util.h>

#include "phy_3d_sensor.h"

LOG_MODULE_REGISTER(phy_3d_sensor, CONFIG_SENSING_LOG_LEVEL);

#define SENSING_ACCEL_Q31_SHIFT 6
#define SENSING_GYRO_Q31_SHIFT 15

static int64_t shifted_q31_to_scaled_int64(q31_t q, int8_t shift, int64_t scale)
{
	int64_t scaled_value;
	int64_t shifted_value;

	shifted_value = (int64_t)q << shift;
	shifted_value = llabs(shifted_value);

	scaled_value =
		FIELD_GET(GENMASK64(31 + shift, 31), shifted_value) * scale +
		(FIELD_GET(GENMASK64(30, 0), shifted_value) * scale / BIT(31));

	if (q < 0) {
		scaled_value = -scaled_value;
	}

	return scaled_value;
}

static q31_t scaled_int64_to_shifted_q31(int64_t val, int64_t scale,
		int8_t shift)
{
	return (q31_t)((val * BIT(31 - shift) / scale));
}

static q31_t accel_sensor_value_to_q31(struct sensor_value *val)
{
	int64_t micro_ms2 = sensor_value_to_micro(val);
	int64_t micro_g = micro_ms2 * 1000000LL / SENSOR_G;

	return scaled_int64_to_shifted_q31(micro_g, 1000000LL,
			SENSING_ACCEL_Q31_SHIFT);
}

static void accel_q31_to_sensor_value(q31_t q, struct sensor_value *val)
{
	int64_t micro_g = shifted_q31_to_scaled_int64(q,
			SENSING_ACCEL_Q31_SHIFT, 1000000LL);
	int64_t micro_ms2 = micro_g * SENSOR_G / 1000000LL;

	sensor_value_from_micro(val, micro_ms2);
}

static const struct phy_3d_sensor_custom custom_accel = {
	.chan_all = SENSOR_CHAN_ACCEL_XYZ,
	.shift = SENSING_ACCEL_Q31_SHIFT,
	.q31_to_sensor_value = accel_q31_to_sensor_value,
	.sensor_value_to_q31 = accel_sensor_value_to_q31,
};

static q31_t gyro_sensor_value_to_q31(struct sensor_value *val)
{
	int64_t micro_rad = (int64_t)sensor_value_to_micro(val);
	int64_t micro_deg = micro_rad * 180000000LL / SENSOR_PI;

	return scaled_int64_to_shifted_q31(micro_deg, 1000000LL,
			SENSING_GYRO_Q31_SHIFT);
}

static void gyro_q31_to_sensor_value(q31_t q, struct sensor_value *val)
{
	int64_t micro_deg = shifted_q31_to_scaled_int64(q,
			SENSING_GYRO_Q31_SHIFT, 1000000LL);
	int64_t micro_rad = micro_deg * SENSOR_PI / 180000000LL;

	sensor_value_from_micro(val, micro_rad);
}

static const struct phy_3d_sensor_custom custom_gyro = {
	.chan_all = SENSOR_CHAN_GYRO_XYZ,
	.shift = SENSING_GYRO_Q31_SHIFT,
	.q31_to_sensor_value = gyro_q31_to_sensor_value,
	.sensor_value_to_q31 = gyro_sensor_value_to_q31,
};

static int phy_3d_sensor_init(const struct device *dev)
{
	const struct phy_3d_sensor_config *cfg = dev->config;
	struct phy_3d_sensor_data *data = dev->data;
	int i;

	for (i = 0; i < cfg->sensor_num; i++) {
		switch (cfg->sensor_types[i]) {
		case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
			data->customs[i] = &custom_accel;
		break;
		case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
			data->customs[i] = &custom_gyro;
		break;
		default:
			LOG_ERR("phy_3d_sensor doesn't support sensor type %d",
			cfg->sensor_types[i]);
			return -ENOTSUP;
		}
	}

	LOG_INF("%s: Underlying device: %s", dev->name, cfg->hw_dev->name);

	return 0;
}

static int phy_3d_sensor_attr_set_hyst(const struct device *dev,
		enum sensor_channel chan,
		const struct sensor_value *val)
{
	const struct phy_3d_sensor_config *cfg = dev->config;

	if (chan == SENSOR_CHAN_ACCEL_XYZ || chan == SENSOR_CHAN_GYRO_XYZ) {
		return sensor_attr_set(cfg->hw_dev, chan, SENSOR_ATTR_SLOPE_TH, val);
	} else {
		return sensor_attr_set(cfg->hw_dev, chan, SENSOR_ATTR_HYSTERESIS, val);
	}
}

static int phy_3d_sensor_attr_set(const struct device *dev,
		enum sensor_channel chan,
		enum sensor_attribute attr,
		const struct sensor_value *val)
{
	const struct phy_3d_sensor_config *cfg = dev->config;
	int ret = 0;

	switch (attr) {
	case SENSOR_ATTR_HYSTERESIS:
		ret = phy_3d_sensor_attr_set_hyst(dev, chan, val);
		break;

	default:
		ret = sensor_attr_set(cfg->hw_dev, chan, attr, val);
		break;
	}

	LOG_INF("%s:%s attr:%d ret:%d", __func__, dev->name, attr, ret);
	return ret;
}

static void phy_3d_sensor_submit(const struct device *dev,
		struct rtio_iodev_sqe *sqe)
{
	struct sensing_submit_config *config = (struct sensing_submit_config *)sqe->sqe.iodev->data;
	const struct phy_3d_sensor_config *cfg = dev->config;
	struct phy_3d_sensor_data *data = dev->data;
	const struct phy_3d_sensor_custom *custom = data->customs[config->info_index];
	struct sensing_sensor_value_3d_q31 *sample;
	struct sensor_value value[PHY_3D_SENSOR_CHANNEL_NUM];
	uint32_t buffer_len;
	int i, ret;

	ret = rtio_sqe_rx_buf(sqe, sizeof(*sample), sizeof(*sample),
			(uint8_t **)&sample, &buffer_len);
	if (ret) {
		rtio_iodev_sqe_err(sqe, ret);
		return;
	}

	ret = sensor_sample_fetch_chan(cfg->hw_dev, custom->chan_all);
	if (ret) {
		LOG_ERR("%s: sample fetch failed: %d", dev->name, ret);
		rtio_iodev_sqe_err(sqe, ret);
		return;
	}

	ret = sensor_channel_get(cfg->hw_dev, custom->chan_all, value);
	if (ret) {
		LOG_ERR("%s: channel get failed: %d", dev->name, ret);
		rtio_iodev_sqe_err(sqe, ret);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(value); ++i) {
		sample->readings[0].v[i] = custom->sensor_value_to_q31(&value[i]);
	}

	sample->header.reading_count = 1;
	sample->shift = custom->shift;

	LOG_DBG("%s: Sample data:\t x: %d, y: %d, z: %d",
			dev->name,
			sample->readings[0].x,
			sample->readings[0].y,
			sample->readings[0].z);

	rtio_iodev_sqe_ok(sqe, 0);
	return;
}

static DEVICE_API(sensor, phy_3d_sensor_api) = {
	.attr_set = phy_3d_sensor_attr_set,
	.submit = phy_3d_sensor_submit,
};

/* To be removed */
static const struct sensing_sensor_register_info phy_3d_sensor_reg = {
	.flags = SENSING_SENSOR_FLAG_REPORT_ON_CHANGE,
	.sample_size = sizeof(struct sensing_sensor_value_3d_q31),
	.sensitivity_count = PHY_3D_SENSOR_CHANNEL_NUM,
	.version.value = SENSING_SENSOR_VERSION(0, 8, 0, 0),
};

#define SENSING_PHY_3D_SENSOR_DT_DEFINE(_inst)						\
	static struct rtio_iodev_sqe _CONCAT(sqes, _inst)[DT_INST_PROP_LEN(_inst,	\
			sensor_types)];							\
	static const struct phy_3d_sensor_custom *_CONCAT(customs, _inst)		\
			[DT_INST_PROP_LEN(_inst, sensor_types)];			\
	static struct phy_3d_sensor_data _CONCAT(data, _inst) = {			\
		.sqes = _CONCAT(sqes, _inst),						\
		.customs = _CONCAT(customs, _inst),					\
	};										\
	static const struct phy_3d_sensor_config _CONCAT(cfg, _inst) = {		\
		.hw_dev = DEVICE_DT_GET(						\
				DT_PHANDLE(DT_DRV_INST(_inst),				\
				underlying_device)),					\
		.sensor_num = DT_INST_PROP_LEN(_inst, sensor_types),			\
		.sensor_types = DT_PROP(DT_DRV_INST(_inst), sensor_types),		\
	};										\
	SENSING_SENSORS_DT_INST_DEFINE(_inst, &phy_3d_sensor_reg, NULL,			\
		&phy_3d_sensor_init, NULL,						\
		&_CONCAT(data, _inst), &_CONCAT(cfg, _inst),				\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
		&phy_3d_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PHY_3D_SENSOR_DT_DEFINE);
