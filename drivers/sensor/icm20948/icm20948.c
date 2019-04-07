/* bmp280.c - Driver for Bosch BMP280 temperature and pressure sensor */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include "icm20948.h"

#ifdef DT_ICM20498_BUS_I2C
#include <i2c.h>
#elif defined DT_ICM20498_BUS_SPI
#include <spi.h>
#endif

static int icm20948_sample_fetch(struct device *dev, enum sensor_channel chan){
    struct icm20948_data *drv_data = dev->driver_data;

    return 0;
}

static const struct sensor_driver_api icm20498_driver_api = {
};

int icm20948_init(struct device *dev)
{
    return 0;
}

struct icm20948_data icm20948_data;

DEVICE_AND_API_INIT(icm20948,CONFIG_ICM20948_NAME,icm20948_init,&icm20948_data,
                NULL,POST_KERNEL,CONFIG_SENSOR_INIT_PRIORITY,
                &icm20498_driver_api);


