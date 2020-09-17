/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_SETUP_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_SETUP_H_

#include <device.h>

int icm42605_sensor_init(struct device *dev);
int icm42605_turn_on_fifo(struct device *dev);
int icm42605_turn_off_fifo(struct device *dev);
int icm42605_turn_off_sensor(struct device *dev);
int icm42605_set_odr(struct device *dev, int a_rate, int g_rate);

#endif /* __SENSOR_ICM42605_ICM42605_SETUP__ */
