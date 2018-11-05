/* si1133.c - Driver for SI1133 UV index and ambient light sensor */

/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "si1133.h"

#include <gpio.h>
#include <i2c.h>


/********************************************
 ******************* API ********************
 ********************************************/

static int si1133_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	/* TODO */

	return 0;
}

static int si1133_channel_get(struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	/* TODO */

	return 0;
}

static const struct sensor_driver_api si1133_api_funcs = {
	.sample_fetch = si1133_sample_fetch,
	.channel_get = si1133_channel_get,
#ifdef CONFIG_SI1133_TRIGGER
	.trigger_set = si1133_trigger_set,
#endif
};

/********************************************
 ****************** INIT ********************
 ********************************************/

static int si1133_init(struct device *dev)
{
	/* TODO */

	return 0;
}

struct si1133_data si1133_data;

DEVICE_AND_API_INIT(si1133 CONFIG_SI1133_DEV_NAME, si1133_init, &si1133_data,
		NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si1133_api_funcs);
