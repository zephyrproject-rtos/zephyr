/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

typedef int16_t icm42688_sample_t;

struct icm42688_encoded_data {
	uint16_t channels: 7;
	uint16_t accelerometer_scale: 2;
	uint16_t gyroscope_scale: 3;
	uint16_t reserved: 4;
	int16_t readings[7];
};

int icm42688_encode(const struct device *dev, const enum sensor_channel *const channels,
		    const size_t num_channels, uint8_t *buf);

int icm42688_get_decoder(const struct device *dev, struct sensor_decoder_api *decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_DECODER_H_ */
