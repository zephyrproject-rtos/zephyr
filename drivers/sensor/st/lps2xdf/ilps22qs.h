/* ST Microelectronics ILPS22QS pressure and temperature sensor
 *
 * Copyright (c) 2023-2024 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stmemsc.h>

#include "ilps22qs_reg.h"

#include <zephyr/drivers/sensor.h>

#ifndef ZEPHYR_DRIVERS_SENSOR_ILPS22QS_ILPS22QS_H_
#define ZEPHYR_DRIVERS_SENSOR_ILPS22QS_ILPS22QS_H_

extern const struct lps2xdf_chip_api st_ilps22qs_chip_api;

int st_ilps22qs_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_ILPS22QS_H_ */
