/* ST Microelectronics LPS28DFW pressure and temperature sensor
 *
 * Copyright (c) 2023 STMicroelectronics
 * Copyright (c) 2023 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stmemsc.h>

#include <zephyr/drivers/sensor.h>

#include "lps28dfw_reg.h"

#ifndef ZEPHYR_DRIVERS_SENSOR_LPS28DFW_LPS28DFW_H_
#define ZEPHYR_DRIVERS_SENSOR_LPS28DFW_LPS28DFW_H_

extern const struct lps2xdf_chip_api st_lps28dfw_chip_api;

int st_lps28dfw_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_LPS28DFW_H_ */
