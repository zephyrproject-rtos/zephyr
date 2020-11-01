/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_

#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <zephyr/types.h>
#include <drivers/sensor.h>

typedef enum {
  LOW=0,
  MID,
  HIGH
} lsm9ds1_perform ;

typedef void (*lsm9ds1_sample_fetch_t)(struct device *device);
typedef void (*lsm9ds1_channel_get_t)(struct device *device, enum sensor_channel chan, float *val);
typedef void (*lsm9ds1_performance_t)(struct device *device, lsm9ds1_perform perform);
typedef bool (*lsm9ds1_initDone_t)(struct device *device);

struct lsm9ds1_api {
    lsm9ds1_sample_fetch_t sample_fetch;
    lsm9ds1_channel_get_t  channel_get;
    lsm9ds1_performance_t  sensor_performance;
    lsm9ds1_initDone_t     init_done;
};

struct lsm9ds1_data {
    struct device *i2c;

    float accel_x;
    float accel_y;
    float accel_z;
    
    float gyro_x;
    float gyro_y;
    float gyro_z;
    
    float magn_x;
    float magn_y;
    float magn_z;
    
    float temperature_c;

};


#endif /* ZEPHYR_DRIVERS_SENSOR_LSM9DS1_LSM9DS1_H_ */
