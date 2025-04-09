/* ST Microelectronics LIS2DUX12 6-axis IMU sensor driver
 *
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LIS2DUX12_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_LIS2DUX12_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

/*
 * This macro converts the Accelerometer full-scale range value (which should be a power of 2) to
 * an index value used by the decoder. Note: this index is not the same as the RAW register value.
 */
#define LIS2DUX12_ACCEL_FS_VAL_TO_FS_IDX(x) (__builtin_clz(x) - 1)

struct lis2dux12_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t range: 2;
	uint8_t reserved: 5;
	uint8_t int_status;
} __attribute__((__packed__));

struct lis2dux12_fifo_data {
	struct lis2dux12_decoder_header header;
	uint32_t accel_odr: 4;
	uint32_t fifo_count: 7;
	uint32_t reserved_1: 5;
	uint32_t accel_batch_odr: 3;
	uint32_t ts_batch_odr: 2;
	uint32_t reserved: 11;
} __attribute__((__packed__));

struct lis2dux12_rtio_data {
	struct lis2dux12_decoder_header header;
	struct {
		uint8_t has_accel: 1; /* set if accel channel has data */
		uint8_t has_temp: 1;  /* set if temp channel has data */
		uint8_t reserved: 6;
	}  __attribute__((__packed__));
	int16_t acc[3];
	int16_t temp;
};

int lis2dux12_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		      const size_t num_channels, uint8_t *buf);

int lis2dux12_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_LIS2DUX12_DECODER_H_ */
