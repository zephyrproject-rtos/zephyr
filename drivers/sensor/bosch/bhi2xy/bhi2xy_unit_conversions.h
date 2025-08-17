/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_UNIT_CONVERSIONS_H_
#define ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_UNIT_CONVERSIONS_H_

#include <zephyr/drivers/sensor.h>

#include <stdint.h>

void bhi2xy_accel_to_ms2(struct sensor_value *val, int16_t raw_val, uint16_t range);
void bhi2xy_gyro_to_rads(struct sensor_value *val, int16_t raw_val, uint16_t range);
void bhi2xy_mag_to_gauss(struct sensor_value *val, int16_t raw_val);
void bhi2xy_pres_to_kpa(struct sensor_value *val, uint32_t raw_val);
void bhi2xy_rv_grv_to_quat(struct sensor_value *val, int16_t raw_val);
void bhi2xy_ori_to_deg(struct sensor_value *val, int16_t raw_val);

#endif /*ZEPHYR_DRIVERS_SENSOR_BHI2XY_BHI2XY_UNIT_CONVERSIONS_H_ */
