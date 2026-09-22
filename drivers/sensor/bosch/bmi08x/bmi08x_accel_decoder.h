/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_ACCEL_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_ACCEL_DECODER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "bmi08x.h"

void bmi08x_accel_encode_header(const struct device *dev, struct bmi08x_accel_encoded_data *edata,
				bool is_streaming, uint16_t buf_len);
int bmi08x_accel_decoder_get(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_ACCEL_DECODER_H_ */
