/*
 * Copyright (c) 2023, Vitrolife A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EXPLORIR_M_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EXPLORIR_M_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_explorir_m {
	/* Sensor integrated low-pass filter. Values 16, 32, 64, and 128 is allowed */
	SENSOR_ATTR_EXPLORIR_M_FILTER = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EXPLORIR_M_H_ */
