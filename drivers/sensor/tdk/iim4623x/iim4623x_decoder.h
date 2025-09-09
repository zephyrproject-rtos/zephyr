/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_IIM4623X_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_IIM4623X_DECODER_H_

#include <zephyr/drivers/sensor.h>
#include "iim4623x.h"

int iim4623x_encode(const struct device *dev, struct iim4623x_encoded_data *edata);

int iim4623x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#endif /* ZEPHYR_DRIVERS_SENSOR_IIM4623X_DECODER_H_ */
