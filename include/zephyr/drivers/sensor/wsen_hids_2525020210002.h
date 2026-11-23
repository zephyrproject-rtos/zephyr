/*
 * Copyright (c) 2024 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for WSEN-HIDS-2525020210002 Sensor
 * @ingroup wsen_hids_2525020210002_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Würth Elektronik WSEN-HIDS humidity sensor
 * @defgroup wsen_hids_2525020210002_interface WSEN-HIDS-2525020210002
 * @ingroup sensor_interface_ext_we
 * @{
 */

/** @brief Custom sensor attributes for WSEN-HIDS-2525020210002 */
enum sensor_attribute_wsen_hids_2525020210002 {
	/** Measurement precision, one of @ref hids_2525020210002_precision_t */
	SENSOR_ATTR_WSEN_HIDS_2525020210002_PRECISION = SENSOR_ATTR_PRIV_START,
	/**
	 * Heater setting, one of @ref hids_2525020210002_heater_t. Only applies with
	 * @ref hids_2525020210002_precision_High.
	 */
	SENSOR_ATTR_WSEN_HIDS_2525020210002_HEATER
};

/** @brief Measurement precision */
typedef enum {
	hids_2525020210002_precision_Low = 0x0,    /**< Low precision */
	hids_2525020210002_precision_Medium = 0x1, /**< Medium precision */
	hids_2525020210002_precision_High = 0x2    /**< High precision */
} hids_2525020210002_precision_t;

/** @brief Heater setting */
typedef enum {
	hids_2525020210002_heater_Off = 0x0,            /**< Heater off */
	hids_2525020210002_heater_On_200mW_1s = 0x1,    /**< 200 mW for 1 s */
	hids_2525020210002_heater_On_200mW_100ms = 0x2, /**< 200 mW for 100 ms */
	hids_2525020210002_heater_On_110mW_1s = 0x3,    /**< 110 mW for 1 s */
	hids_2525020210002_heater_On_110mW_100ms = 0x4, /**< 110 mW for 100 ms */
	hids_2525020210002_heater_On_20mW_1s = 0x5,     /**< 20 mW for 1 s */
	hids_2525020210002_heater_On_20mW_100ms = 0x6,  /**< 20 mW for 100 ms */
} hids_2525020210002_heater_t;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_WSEN_HIDS_2525020210002_H_ */
