/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of STS4X sensor
 * @ingroup sts4x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_STS4x_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_STS4x_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sensirion STS4x temperature sensor
 * @defgroup sts4x_interface STS4X
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/**
 * @brief Set the repeatability for the next measurement.
 *
 *
 * * 0 -> low repeatability
 * * 1 -> medium repeatability
 * * 2 -> high repeatability
 *
 * @param[in] rep
 *
 */
void sts4x_set_repeatability(uint8_t rep);

/**
 * @brief Read the unique serial number from OTP memory.
 *
 * @param[out] serial_number
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int sts4x_read_serial_number(uint32_t *serial_number);

/**
 * @brief Perform a soft reset and return the sensor to idle state.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int sts4x_soft_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_STS4x_H_ */
