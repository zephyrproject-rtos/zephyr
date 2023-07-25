/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

struct icm42688_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t gyro_fs: 3;
	uint8_t accel_fs: 2;
	uint8_t reserved: 2;
} __attribute__((__packed__));

struct icm42688_encoded_data {
	struct icm42688_decoder_header header;
	struct {
		uint8_t channels: 7;
		uint8_t reserved: 1;
	}  __attribute__((__packed__));
	int16_t readings[7];
};

int icm42688_encode(const struct device *dev, const enum sensor_channel *const channels,
		    const size_t num_channels, uint8_t *buf);

int icm42688_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_DECODER_H_ */
