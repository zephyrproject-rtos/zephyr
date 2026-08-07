/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAA3905_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_PAA3905_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "paa3905.h"

int paa3905_encode(const struct device *dev,
		   const struct sensor_chan_spec *const channels,
		   size_t num_channels,
		   uint8_t *buf);

uint8_t paa3905_encode_channel(enum sensor_channel chan);

int paa3905_get_decoder(const struct device *dev,
			const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_PAA3905_DECODER_H_ */
