/*
 * Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for WSEN-HIDS-2525020210002 Sensor
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_wsen_hids_2525020210002 {
	SENSOR_ATTR_WSEN_HIDS_2525020210002_PRECISION = SENSOR_ATTR_PRIV_START,
	SENSOR_ATTR_WSEN_HIDS_2525020210002_HEATER
};

typedef enum {
	hids_2525020210002_precision_Low = 0x0,
	hids_2525020210002_precision_Medium = 0x1,
	hids_2525020210002_precision_High = 0x2
} hids_2525020210002_precision_t;

typedef enum {
	hids_2525020210002_heater_Off = 0x0,
	hids_2525020210002_heater_On_200mW_1s = 0x1,
	hids_2525020210002_heater_On_200mW_100ms = 0x2,
	hids_2525020210002_heater_On_110mW_1s = 0x3,
	hids_2525020210002_heater_On_110mW_100ms = 0x4,
	hids_2525020210002_heater_On_20mW_1s = 0x5,
	hids_2525020210002_heater_On_20mW_100ms = 0x6,
} hids_2525020210002_heater_t;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_H_ */
