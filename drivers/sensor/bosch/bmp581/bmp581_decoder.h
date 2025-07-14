/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP581_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP581_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "bmp581.h"

struct bmp581_encoded_data {
	struct {
		uint8_t channels;
		uint8_t events;
		uint64_t timestamp;
		uint8_t press_en;
	} header;
	uint8_t payload[6]; /* 3 bytes temp + 3 bytes press */
};

int bmp581_encode(const struct device *dev,
		   const struct sensor_read_config *read_config,
		   uint8_t trigger_status,
		   uint8_t *buf);

int bmp581_get_decoder(const struct device *dev,
		       const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP581_DECODER_H_ */
