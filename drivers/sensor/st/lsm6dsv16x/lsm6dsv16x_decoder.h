/* ST Microelectronics LSM6DSV16X 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LSM6DSV16X_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_LSM6DSV16X_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

struct lsm6dsv16x_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t gyro_fs: 4;
	uint8_t accel_fs: 2;
	uint8_t reserved: 1;
} __attribute__((__packed__));

struct lsm6dsv16x_fifo_data {
	struct lsm6dsv16x_decoder_header header;
	uint8_t int_status;
	uint16_t gyro_odr: 4;
	uint16_t accel_odr: 4;
	uint16_t fifo_count: 11;
	uint16_t reserved_1: 5;
	uint16_t gyro_batch_odr: 4;
	uint16_t accel_batch_odr: 4;
	uint16_t temp_batch_odr: 4;
	uint16_t sflp_batch_odr: 3;
	uint16_t reserved_2: 1;
} __attribute__((__packed__));

struct lsm6dsv16x_rtio_data {
	struct lsm6dsv16x_decoder_header header;
	struct {
		uint8_t has_accel: 1; /* set if accel channel has data */
		uint8_t has_gyro: 1;  /* set if gyro channel has data */
		uint8_t has_temp: 1;  /* set if temp channel has data */
		uint8_t reserved: 5;
	}  __attribute__((__packed__));
	int16_t acc[3];
	int16_t gyro[3];
	int16_t temp;
};

int lsm6dsv16x_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		      const size_t num_channels, uint8_t *buf);

int lsm6dsv16x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_LSM6DSV16X_DECODER_H_ */
