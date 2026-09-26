/*
 * Copyright 2026 Ahmed Ashraf NourEldeen <a.programmer55559@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42605_ICM42605_BUS_H_

#include "icm42605.h"

int icm42605_reg_write(const struct icm42605_config *cfg, uint8_t reg, uint8_t *data);
int icm42605_reg_read(const struct icm42605_config *cfg, uint8_t reg, uint8_t *data, size_t len);

#endif /* __SENSOR_ICM42605_ICM42605_BUS__ */
