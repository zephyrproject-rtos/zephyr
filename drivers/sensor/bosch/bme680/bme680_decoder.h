/*
 * Copyright (c) 2025 Nordic Semiconductors ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BME680_BME680_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_BME680_BME680_DECODER_H_

#include "bme680.h"

void bme680_compensate_raw_data(enum sensor_channel chan,
							const struct bme680_raw_data *raw_data,
							const struct bme680_comp_param *comp_param,
							struct bme680_compensated_data *data);

int bme680_encode(const struct device *dev,
		  const struct sensor_read_config *read_config,
		  uint8_t *buf);

int bme680_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_BME680_BME680_DECODER_H_ */
