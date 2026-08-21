/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI270_BMI270_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI270_BMI270_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

/** Encoded FIFO buffer header for BMI270 */
struct bmi270_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t is_headerless: 1; /* 1 = FIFO had no per-frame headers; fixed 12 B/frame */
	uint8_t acc_range: 2;     /* 0=2G, 1=4G, 2=8G, 3=16G */
	uint8_t gyr_range_idx: 3; /* 0=2000dps .. 4=125dps */
	uint8_t reserved: 1;
	uint16_t acc_odr_hz;
	uint16_t gyr_odr_hz;
} __attribute__((__packed__));

/** Encoded FIFO read result: header + raw FIFO bytes */
struct bmi270_fifo_encoded_data {
	struct bmi270_decoder_header header;
	uint16_t fifo_byte_count;
	uint8_t fifo_data[];
} __attribute__((__packed__));

int bmi270_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI270_BMI270_DECODER_H_ */
