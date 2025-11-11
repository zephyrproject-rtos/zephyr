/*
 * Copyright (c) 2025 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of FCX-MLDX5 sensor
 * @ingroup fcx_mldx5_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_FCX_MLDX5_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_FCX_MLDX5_H_

/**
 * @brief Angst+Pfister FCX-MLDX5 O<sub>2</sub> sensor
 * @defgroup fcx_mldx5_interface FCX-MLDX5
 * @ingroup sensor_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Custom sensor attributes for FCX-MLDX5
 */
enum sensor_attribute_fcx_mldx5 {
	/**
	 * Status of the sensor
	 *
	 * sensor_value.val1 is the status of the sensor, see fcx_mldx5_status enum
	 */
	SENSOR_ATTR_FCX_MLDX5_STATUS = SENSOR_ATTR_PRIV_START,
	/**
	 * Reset the sensor
	 */
	SENSOR_ATTR_FCX_MLDX5_RESET,
};

/**
 * @brief Status of the sensor
 */
enum fcx_mldx5_status {
	/**
	 *  @brief Standby
	 *  The heating of the O2 sensor is on standby, no O2 measurement is possible.
	 */
	FCX_MLDX5_STATUS_STANDBY = 2,
	/**
	 *  @brief Ramp-up
	 *  The O2 sensor is in the heating phase, no O2 measurement is possible.
	 */
	FCX_MLDX5_STATUS_RAMP_UP = 3,
	/**
	 *  @brief Run
	 *  The O2 module is in normal operation.
	 */
	FCX_MLDX5_STATUS_RUN = 4,
	/**
	 *  @brief Error
	 *  The O2 module has detected a system error, no O2 measurement is possible.
	 */
	FCX_MLDX5_STATUS_ERROR = 5,
	/**
	 *  @brief Unknown status
	 *  Unknown is not sent by the sensor but used in the driver to indicate an error case.
	 */
	FCX_MLDX5_STATUS_UNKNOWN,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_FCX_MLDX5_H_ */
