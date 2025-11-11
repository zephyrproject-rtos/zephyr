/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "icm45686.h"

int icm45686_encode(const struct device *dev,
		    const struct sensor_chan_spec *const channels,
		    const size_t num_channels,
		    uint8_t *buf);

int icm45686_get_decoder(const struct device *dev,
			 const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_DECODER_H_ */
