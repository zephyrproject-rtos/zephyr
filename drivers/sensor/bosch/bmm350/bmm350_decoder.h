/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_DECODER_H_

#include "bmm350.h"

void bmm350_decoder_compensate_raw_data(const struct bmm350_raw_mag_data *raw_data,
					const struct mag_compensate *comp,
					struct bmm350_mag_temp_data *out);

int bmm350_encode(const struct device *dev,
		  const struct sensor_read_config *read_config,
		  bool is_trigger,
		  uint8_t *buf);

int bmm350_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMM350_BMM350_DECODER_H_ */
