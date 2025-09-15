/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM4268X_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM4268X_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "icm4268x.h"

struct icm4268x_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t gyro_fs: 3;
	uint8_t accel_fs: 3;
	uint8_t variant: 1;
	struct alignment axis_align[3];
};

struct icm4268x_fifo_data {
	struct icm4268x_decoder_header header;
	uint8_t int_status;
	uint8_t gyro_odr: 4;
	uint8_t accel_odr: 4;
	uint16_t fifo_count: 11;
	uint16_t padding1: 5;
	uint16_t rtc_freq;
};

struct icm4268x_encoded_data {
	struct icm4268x_decoder_header header;
	struct {
		uint8_t channels: 7;
		uint8_t reserved: 1;
	}  __attribute__((__packed__));
	int16_t readings[7];
};

int icm4268x_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		    const size_t num_channels, uint8_t *buf);

int icm4268x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM4268X_DECODER_H_ */
