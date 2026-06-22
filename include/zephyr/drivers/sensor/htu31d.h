/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of HTU31D sensor
 * @ingroup htu31d_interface
 *
 * This exposes APIs for the on-chip heater, diagnostics register, and
 * factory-programmed serial number, which are outside the scope of the
 * generic Zephyr sensor driver abstraction.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_HTU31D_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_HTU31D_H_

/**
 * @defgroup htu31d_interface HTU31D
 * @ingroup sensor_interface_ext
 * @brief TE Connectivity HTU31D temperature and humidity sensor
 * @{
 */

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Diagnostics register bit flags
 *
 * Bit positions of the diagnostics byte returned by htu31d_read_diagnostics().
 * @{
 */
#define HTU31D_DIAG_HEATER_ON     BIT(0) /**< Heater is currently enabled */
#define HTU31D_DIAG_TEMP_LOW_ERR  BIT(1) /**< Temperature below minimum */
#define HTU31D_DIAG_TEMP_HIGH_ERR BIT(2) /**< Temperature above maximum */
#define HTU31D_DIAG_TEMP_OVER_ERR BIT(3) /**< Temperature under/over-run */
#define HTU31D_DIAG_HUMI_LOW_ERR  BIT(4) /**< Humidity below minimum */
#define HTU31D_DIAG_HUMI_HIGH_ERR BIT(5) /**< Humidity above maximum */
#define HTU31D_DIAG_HUMI_OVER_ERR BIT(6) /**< Humidity under/over-run */
#define HTU31D_DIAG_NVM_ERR       BIT(7) /**< NVM checksum error */
/** @} */

/**
 * @brief Enable or disable the on-chip heater.
 *
 * The heater can be used to burn off condensation or perform sensor
 * self-diagnostics. It increases the measured temperature by several
 * degrees while enabled and should not be left on continuously.
 *
 * @param dev     Pointer to the sensor device.
 * @param enable  True to enable the heater, false to disable it.
 *
 * @return 0 on success, negative errno on I2C error.
 */
int htu31d_heater_set(const struct device *dev, bool enable);

/**
 * @brief Read the sensor diagnostics register.
 *
 * The diagnostics byte reports persistent fault flags; decode the
 * returned value using the HTU31D_DIAG_* bit macros.
 *
 * @param dev    Pointer to the sensor device.
 * @param diag   Output, populated with the 8-bit diagnostics word.
 *
 * @return 0 on success, -EIO on CRC failure, or another negative errno.
 */
int htu31d_read_diagnostics(const struct device *dev, uint8_t *diag);

/**
 * @brief Read the factory-programmed 24-bit serial number.
 *
 * @param dev     Pointer to the sensor device.
 * @param serial  Output, populated with the 24-bit serial number in
 *                the low three bytes (upper byte is zero).
 *
 * @return 0 on success, -EIO on CRC failure, or another negative errno.
 */
int htu31d_read_serial(const struct device *dev, uint32_t *serial);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_HTU31D_H_ */
