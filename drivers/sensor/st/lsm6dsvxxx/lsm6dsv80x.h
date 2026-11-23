/* ST Microelectronics LSM6DSVXXX family IMU sensor
 *
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dsv80x.pdf
 */
#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSV80X_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSV80X_H_

#include <stdint.h>
#include <stmemsc.h>

#include "lsm6dsvxxx.h"
#include "lsm6dsv80x_reg.h"

#include <zephyr/drivers/sensor.h>

extern const struct lsm6dsvxxx_chip_api st_lsm6dsv80x_chip_api;
extern const int8_t st_lsm6dsv80x_accel_bit_shift[];
extern const int32_t st_lsm6dsv80x_accel_scaler[];

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSV80X_H_ */
