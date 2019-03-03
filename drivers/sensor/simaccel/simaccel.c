/* simaccel.c - Simulated driver for a 3-axis accelerometer. */

/*
 * Copyright (c) 2019 Kevin Townsend.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sensor.h>
#include <init.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include "simaccel.h"

/**
 * @brief Retrieves a set of data samples from the sensor.
 *
 * @param dev   Pointer to the `device` used when executing the read attempt.
 * @param chan  The `sensor_channel` to request data from. This should
 *              normally be `SENSOR_CHAN_ALL`.
 *
 * @return 0 on success, otherwise an appropriate error code.
 */
static int simaccel_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct simaccel_data *data = dev->driver_data;
	u8_t buf[6] = { 0 };
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

    /* TODO: Generate sensible random data or use a known lookup table. */
    data->range = 2;
    data->xdata = (s16_t)buf[1] << 8 | buf[0];
    data->ydata = (s16_t)buf[3] << 8 | buf[2];
    data->zdata = (s16_t)buf[5] << 8 | buf[4];

	return 0;
}

static int simaccel_channel_get(struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct simaccel_data *data = dev->driver_data;

	switch (chan) {
    	default:
    		return -EINVAL;
	}

	return 0;
}

static const struct sensor_driver_api simaccel_api_funcs = {
	.sample_fetch = simaccel_sample_fetch,
	.channel_get = simaccel_channel_get,
};

/**
 *  Attempts to initialise a simaccel device.
 *
 *  @param dev The 'device' to bind the driver instance to.
 *
 *  @return 0 on success, otherwise an appropriate error code.
 */
int simaccel_init(struct device *dev)
{
	struct simaccel_data *data = dev->driver_data;

    return 0;
}

static struct simaccel_data simaccel_data;

DEVICE_AND_API_INIT(simaccel, "simaccel",
            simaccel_init, &simaccel_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &simaccel_api_funcs);
