/* si7021.c - Driver for the SI7021 Relative Humidity and Temperature Sensor */

/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "si7021.h"

#include <gpio.h>
#include <i2c.h>
#include <sensor.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(SI7021)



/********************************************
 ******************* API ********************
 ********************************************/

static int si7021_sample_fetch(struct device *dev, enum sensor_channel chan)
{
    return 0;
}

static int si7021_channel_get(struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
    return 0;
}

static const struct sensor_driver_api si7021_api_funcs = {

    .sample_fetch = si7021_sample_fetch,
    .channel_get = si7021_channel_get
};

/********************************************
 ****************** INIT ********************
 ********************************************/

static int si7021_init(struct device *dev)
{
    return 0;
}

static struct si7021_data si7021_data;

DEVICE_AND_API_INIT(si7021, CONFIG_SI7021_DEV_NAME, si7021_init, &si7021_data,
        NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7021_api_funcs);