/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_fake_encoder

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>

#include "fake_encoder.h"

struct fake_encoder_data {
	struct sensor_value rotation;
};

static int fake_encoder_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan);
	return 0;
}

static int fake_encoder_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct fake_encoder_data *data = dev->data;

	if (chan != SENSOR_CHAN_ROTATION && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	*val = data->rotation;
	return 0;
}

void fake_encoder_set_rotation(const struct device *dev, const struct sensor_value *val)
{
	struct fake_encoder_data *data = dev->data;

	data->rotation = *val;
}

void fake_encoder_set_rotation_degrees(const struct device *dev, int32_t degrees)
{
	struct sensor_value val = {
		.val1 = degrees,
		.val2 = 0,
	};

	fake_encoder_set_rotation(dev, &val);
}

static const struct sensor_driver_api fake_encoder_api = {
	.sample_fetch = fake_encoder_sample_fetch,
	.channel_get = fake_encoder_channel_get,
};

static int fake_encoder_init(const struct device *dev)
{
	struct fake_encoder_data *data = dev->data;

	data->rotation.val1 = 0;
	data->rotation.val2 = 0;

	return 0;
}

#define FAKE_ENCODER_INIT(inst)                                                                    \
                                                                                                   \
	static struct fake_encoder_data fake_encoder_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, fake_encoder_init, NULL, &fake_encoder_data_##inst, NULL,      \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &fake_encoder_api);

DT_INST_FOREACH_STATUS_OKAY(FAKE_ENCODER_INIT)
