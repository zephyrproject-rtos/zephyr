/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SEN6X_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_SEN6X_DECODER_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

#include "sen6x.h"

int sen6x_encode(const struct device *dev, const struct sensor_chan_spec *const channels,
		 const size_t num_channels, uint8_t *buf);

int sen6x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_SEN6X_DECODER_H_ */
