/*
 * Bosch BMA400 3-axis accelerometer driver
 * SPDX-FileCopyrightText: Copyright 2026 Luca Gessi lucagessi90@gmail.com
 * SPDX-FileCopyrightText: Copyright 2026 The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA400_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA400_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

/*
 * RTIO types
 */

struct bma400_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t accel_fs: 2;
	uint8_t has_motion_trigger: 1;
	uint8_t has_fifo_wm_trigger: 1;
	uint8_t has_fifo_full_trigger: 1;
	uint8_t reserved: 2;
} __attribute__((__packed__));

struct bma400_fifo_data {
	struct bma400_decoder_header header;
	uint8_t int_status;
	uint16_t accel_odr: 4;
	uint16_t fifo_count: 10;
	uint16_t reserved: 1;
} __attribute__((__packed__));

struct bma400_encoded_data {
	struct bma400_decoder_header header;
	uint8_t accel_xyz_raw_data[7]; /* For SPI dummy byte after register address + 6 bytes */
} __attribute__((__packed__));

int bma400_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

/* BMA400 FIFO Length definition */
#define BMA400_FIFO_HEADER_LENGTH UINT8_C(1)
#define BMA400_FIFO_A_LENGTH      UINT8_C(6)
#define BMA400_FIFO_M_LENGTH      UINT8_C(8)
#define BMA400_FIFO_MA_LENGTH     UINT8_C(14)
#define BMA400_FIFO_ST_LENGTH     UINT8_C(3)
#define BMA400_FIFO_CF_LENGTH     UINT8_C(1)
#define BMA400_FIFO_DATA_LENGTH   UINT8_C(2)

/* Each bit count is 3.9mG or 3900uG */
#define BMA400_OFFSET_MICROG_PER_BIT (3900)
#define BMA400_OFFSET_MICROG_MIN     (INT8_MIN * BMA400_OFFSET_MICROG_PER_BIT)
#define BMA400_OFFSET_MICROG_MAX     (INT8_MAX * BMA400_OFFSET_MICROG_PER_BIT)

/* Range is -104C to 150C. Use shift of 8 (+/-256) */
#define BMA400_TEMP_SHIFT (8)

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA400_DECODER_H_ */
