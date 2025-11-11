/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA4XX_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA4XX_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

/*
 * RTIO types
 */

struct bma4xx_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t accel_fs: 2;
	uint8_t reserved: 5;
} __attribute__((__packed__));

struct bma4xx_fifo_data {
	struct bma4xx_decoder_header header;
	uint8_t int_status;
	uint16_t accel_odr: 4;
	uint16_t fifo_count: 10;
	uint16_t reserved: 1;
} __attribute__((__packed__));

struct bma4xx_encoded_data {
	struct bma4xx_decoder_header header;
	uint8_t accel_xyz_raw_data[6];
#ifdef CONFIG_BMA4XX_TEMPERATURE
	int8_t temp;
#endif /* CONFIG_BMA4XX_TEMPERATURE */
};

int bma4xx_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_DECODER_H_ */
