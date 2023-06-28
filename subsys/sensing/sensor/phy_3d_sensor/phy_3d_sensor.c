/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sensing_phy_3d_sensor

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
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

static void phy_3d_sensor_data_ready_handler(const struct device *dev,
		const struct sensor_trigger *trig)
{
	struct phy_3d_sensor_context *ctx = CONTAINER_OF(trig,
			struct phy_3d_sensor_context,
			trig);
	int ret;

	LOG_DBG("%s: trigger type:%d", dev->name, trig->type);

	ret = sensor_sample_fetch_chan(ctx->hw_dev, ctx->custom->chan_all);
	if (ret) {
		LOG_ERR("%s: sample fetch failed: %d", dev->name, ret);
		return;
	}

	sensing_sensor_notify_data_ready(ctx->dev);
}

static int phy_3d_sensor_enable_data_ready(struct phy_3d_sensor_context *ctx,
		bool enable)
{
	sensor_trigger_handler_t handler;
	int ret = 0;

	ctx->trig.type = SENSOR_TRIG_DATA_READY;
	ctx->trig.chan = ctx->custom->chan_all;

	if (enable != ctx->data_ready_enabled) {
		handler = enable ? phy_3d_sensor_data_ready_handler : NULL;

		ret = sensing_sensor_set_data_ready(ctx->dev, enable);
		if (!ret) {
			if (!sensor_trigger_set(ctx->hw_dev, &ctx->trig,
						handler)) {
				ctx->data_ready_enabled = enable;
			} else {
				sensing_sensor_set_data_ready(ctx->dev,
						!enable);
				LOG_INF("%s: set data ready trigger not successfully",
						ctx->dev->name);
			}
		}
	}

	LOG_DBG("%s: trigger data ready enabled:%d", ctx->dev->name,
			ctx->data_ready_enabled);

	return ret;
}

static int phy_3d_sensor_init(const struct device *dev,
		const struct sensing_sensor_info *info,
		const sensing_sensor_handle_t *reporter_handles,
		int reporters_count)
{
	struct phy_3d_sensor_context *ctx;
	int ret;

	ARG_UNUSED(reporter_handles);
	ARG_UNUSED(reporters_count);

	ctx = sensing_sensor_get_ctx_data(dev);
	ctx->dev = dev;

	switch (ctx->sensor_type) {
	case SENSING_SENSOR_TYPE_MOTION_ACCELEROMETER_3D:
		ctx->custom = &custom_accel;
		break;
	case SENSING_SENSOR_TYPE_MOTION_GYROMETER_3D:
		ctx->custom = &custom_gyro;
		break;
	default:
		LOG_ERR("phy_3d_sensor doesn't support sensor type %d",
				ctx->sensor_type);
		return -ENOTSUP;
	}

	LOG_INF("%s: Underlying device: %s", dev->name, ctx->hw_dev->name);

	/* Check underlying device supports data ready or not */
	ret = phy_3d_sensor_enable_data_ready(ctx, true);
	ctx->data_ready_support = ctx->data_ready_enabled;
	if (!ret && ctx->data_ready_enabled) {
		ret = phy_3d_sensor_enable_data_ready(ctx, false);
	}

	return ret;
}

static int phy_3d_sensor_read_sample(const struct device *dev,
		void *buf, int size)
{
	struct phy_3d_sensor_context *ctx;
	struct sensing_sensor_value_3d_q31 *sample = buf;
	struct sensor_value value[PHY_3D_SENSOR_CHANNEL_NUM];
	int i, ret;

	ctx = sensing_sensor_get_ctx_data(dev);

	if (!ctx->data_ready_enabled) {
		ret = sensor_sample_fetch_chan(ctx->hw_dev,
				ctx->custom->chan_all);
		if (ret) {
			LOG_ERR("%s: sample fetch failed: %d", dev->name, ret);
			return ret;
		}
	}

	ret = sensor_channel_get(ctx->hw_dev, ctx->custom->chan_all, value);
	if (ret) {
		LOG_ERR("%s: channel get failed: %d", dev->name, ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(value); ++i) {
		sample->readings[0].v[i] =
			ctx->custom->sensor_value_to_q31(&value[i]);
	}

	sample->header.reading_count = 1;
	sample->shift = ctx->custom->shift;

	LOG_DBG("%s: Sample data:\t x: %d, y: %d, z: %d",
			dev->name,
			sample->readings[0].x,
			sample->readings[0].y,
			sample->readings[0].z);

	return ret;
}

static int phy_3d_sensor_sensitivity_test(const struct device *dev,
		int index, uint32_t sensitivity,
		void *last_sample_buf, int last_sample_size,
		void *current_sample_buf, int current_sample_size)
{
	struct sensing_sensor_value_3d_q31 *last = last_sample_buf;
	struct sensing_sensor_value_3d_q31 *curr = current_sample_buf;
	q31_t sensi = sensitivity;
	struct phy_3d_sensor_context *ctx;
	int reached = 0;
	int i;

	ctx = sensing_sensor_get_ctx_data(dev);
	if (index >= 0 && index < ARRAY_SIZE(ctx->sensitivities)) {
		reached = abs(curr->readings[0].v[index]
				- last->readings[0].v[index])
			>= sensi;
	} else if (index == SENSING_SENSITIVITY_INDEX_ALL) {
		for (i = 0; i < ARRAY_SIZE(ctx->sensitivities); ++i) {
			reached |= abs(curr->readings[0].v[i]
					- last->readings[0].v[i])
				>= sensi;
		}
	} else {
		LOG_ERR("%s: test sensitivity: invalid index: %d", dev->name,
				index);
		return -EINVAL;
	}

	return !reached;
}

static int phy_3d_sensor_set_interval(const struct device *dev, uint32_t value)
{
	struct phy_3d_sensor_context *ctx;
	struct sensor_value odr;
	int ret;

	LOG_DBG("%s: set report interval %u us", dev->name, value);

	ctx = sensing_sensor_get_ctx_data(dev);

	if (ctx->interval == value) {
		return 0;
	}

	if (value) {
		if (ctx->data_ready_support) {
			phy_3d_sensor_enable_data_ready(ctx, true);
		}

		odr.val1 = USEC_PER_SEC / value;
		odr.val2 = (uint64_t)USEC_PER_SEC * 1000000 / value % 1000000;

		ret = sensor_attr_set(ctx->hw_dev, ctx->custom->chan_all,
				SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
		if (ret) {
			LOG_ERR("%s: Cannot set sampling frequency %d.%06d Hz. ret:%d",
					dev->name, odr.val1, odr.val2, ret);
		} else {
			LOG_DBG("%s: Set sampling frequency %d.%06d Hz.",
					dev->name, odr.val1, odr.val2);
		}
	} else {
		if (ctx->data_ready_support) {
			phy_3d_sensor_enable_data_ready(ctx, false);
		}
	}

	ctx->interval = value;

	return 0;
}

static int phy_3d_sensor_get_interval(const struct device *dev,
		uint32_t *value)
{
	struct phy_3d_sensor_context *ctx;
	struct sensor_value odr;
	uint64_t micro_freq;

	ctx = sensing_sensor_get_ctx_data(dev);

	if (!sensor_attr_get(ctx->hw_dev, ctx->custom->chan_all,
				SENSOR_ATTR_SAMPLING_FREQUENCY, &odr)) {
		micro_freq = sensor_value_to_micro(&odr);
		if (micro_freq) {
			*value = (uint32_t)(USEC_PER_SEC * 1000000 / micro_freq);
		} else {
			*value = 0;
		}
	} else {
		*value = ctx->interval;
	}

	LOG_DBG("%s: get report interval %u us", dev->name, *value);

	return 0;
}

static int phy_3d_sensor_set_slope(struct phy_3d_sensor_context *ctx,
		enum sensor_channel chan, uint32_t value)
{
	struct sensor_value attr_value;
	enum sensor_attribute attr;
	int ret = 0;

	ctx->custom->q31_to_sensor_value((q31_t)value, &attr_value);
	attr = SENSOR_ATTR_SLOPE_TH;

	ret = sensor_attr_set(ctx->hw_dev, chan, attr, &attr_value);
	if (!ret) {
		/* set slope duration */
		attr_value.val1 = PHY_3D_SENSOR_SLOPE_DURATION;
		attr_value.val2 = 0;
		attr = SENSOR_ATTR_SLOPE_DUR;

		ret = sensor_attr_set(ctx->hw_dev, chan, attr, &attr_value);
		if (!ret) {
			ctx->trig.type = SENSOR_TRIG_DELTA;
			ctx->trig.chan = chan;

			if (value) {
				ret = sensor_trigger_set(ctx->hw_dev,
						&ctx->trig,
						phy_3d_sensor_data_ready_handler);
			} else {
				ret = sensor_trigger_set(ctx->hw_dev,
						&ctx->trig,
						NULL);
			}
		}
	}

	if (ret) {
		LOG_DBG("%s: set slope failed! attr:%d chan:%d ret:%d",
				ctx->hw_dev->name, attr, chan, ret);
	}

	return ret;
}

static int phy_3d_sensor_set_sensitivity(const struct device *dev,
		int index, uint32_t value)
{
	struct phy_3d_sensor_context *ctx;
	uint32_t sensi;
	bool enabled;
	int ret = 0;
	int i;

	ctx = sensing_sensor_get_ctx_data(dev);

	if (index >= 0 && index < ARRAY_SIZE(ctx->sensitivities)) {
		ctx->sensitivities[index] = value;
	} else if (index == SENSING_SENSITIVITY_INDEX_ALL) {
		for (i = 0; i < ARRAY_SIZE(ctx->sensitivities); ++i) {
			ctx->sensitivities[i] = value;
		}
	} else {
		LOG_ERR("%s: set sensitivity: invalid index: %d",
				dev->name, index);
		return -EINVAL;
	}

	LOG_DBG("%s: set sensitivity index: %d value: %d", dev->name,
			index, value);

	sensi = ctx->sensitivities[0];
	for (i = 1; i < ARRAY_SIZE(ctx->sensitivities); ++i) {
		sensi = MIN(ctx->sensitivities[i], sensi);
	}

	/* Disable data ready before enable any-motion */
	enabled = ctx->data_ready_enabled;
	if (ctx->data_ready_support && enabled) {
		phy_3d_sensor_enable_data_ready(ctx, false);
	}

	ret = phy_3d_sensor_set_slope(ctx, ctx->custom->chan_all, sensi);
	if (ret) {
		/* Try to enable data ready if enable any-motion failed */
		if (ctx->data_ready_support && enabled) {
			phy_3d_sensor_enable_data_ready(ctx, true);
		}
	}

	return 0;
}

static const struct sensing_sensor_api phy_3d_sensor_api = {
	.init = phy_3d_sensor_init,
	.set_interval = phy_3d_sensor_set_interval,
	.get_interval = phy_3d_sensor_get_interval,
	.set_sensitivity = phy_3d_sensor_set_sensitivity,
	.read_sample = phy_3d_sensor_read_sample,
	.sensitivity_test = phy_3d_sensor_sensitivity_test,
};

static const struct sensing_sensor_register_info phy_3d_sensor_reg = {
	.flags = SENSING_SENSOR_FLAG_REPORT_ON_CHANGE,
	.sample_size = sizeof(struct sensing_sensor_value_3d_q31),
	.sensitivity_count = PHY_3D_SENSOR_CHANNEL_NUM,
	.version.value = SENSING_SENSOR_VERSION(0, 8, 0, 0),
};

#define SENSING_PHY_3D_SENSOR_DT_DEFINE(_inst)				\
	static struct phy_3d_sensor_context _CONCAT(ctx, _inst) = {	\
		.hw_dev = DEVICE_DT_GET(				\
				DT_PHANDLE(DT_DRV_INST(_inst),		\
				underlying_device)),			\
		.sensor_type = DT_PROP(DT_DRV_INST(_inst), sensor_type),\
	};								\
	SENSING_SENSOR_DT_DEFINE(DT_DRV_INST(_inst),			\
		&phy_3d_sensor_reg, &_CONCAT(ctx, _inst),		\
		&phy_3d_sensor_api);

DT_INST_FOREACH_STATUS_OKAY(SENSING_PHY_3D_SENSOR_DT_DEFINE);
