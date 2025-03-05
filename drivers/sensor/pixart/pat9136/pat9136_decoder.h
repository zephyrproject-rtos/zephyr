/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAT9136_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_PAT9136_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "pat9136.h"

int pat9136_encode(const struct device *dev,
		   const struct sensor_chan_spec *const channels,
		   size_t num_channels,
		   uint8_t *buf);

uint8_t pat9136_encode_channel(uint16_t chan);

int pat9136_get_decoder(const struct device *dev,
			const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_PAT9136_DECODER_H_ */
