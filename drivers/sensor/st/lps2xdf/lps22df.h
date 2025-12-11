/* ST Microelectronics LPS22DF pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stmemsc.h>

#include "lps22df_reg.h"

#include <zephyr/drivers/sensor.h>

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS22DF_LPS22DF_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS22DF_LPS22DF_H_

extern const struct lps2xdf_chip_api st_lps22df_chip_api;

int st_lps22df_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS22DF_H_ */
