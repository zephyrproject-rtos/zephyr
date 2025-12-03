/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_GYRO_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_GYRO_DECODER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "bmi08x.h"

void bmi08x_gyro_encode_header(const struct device *dev, struct bmi08x_gyro_encoded_data *edata,
			       bool is_streaming);
int bmi08x_gyro_decoder_get(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_GYRO_DECODER_H_ */
