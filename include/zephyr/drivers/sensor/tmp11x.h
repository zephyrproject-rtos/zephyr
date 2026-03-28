/*
 * Copyright (c) 2021 Innoseis B.V
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of TMP11X sensors
 * @ingroup tmp11x_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP11X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP11X_H_

/**
 * @brief TMP11X temperature sensors
 * @defgroup tmp11x_interface TMP11X
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <sys/types.h>

/**
 * @brief Custom sensor attributes for TMP11X
 */
enum sensor_attribute_tmp_11x {
	/** Turn on power saving/one shot mode */
	SENSOR_ATTR_TMP11X_ONE_SHOT_MODE = SENSOR_ATTR_PRIV_START,
	/** Shutdown the sensor */
	SENSOR_ATTR_TMP11X_SHUTDOWN_MODE,
	/** Turn on continuous conversion */
	SENSOR_ATTR_TMP11X_CONTINUOUS_CONVERSION_MODE,
	/** Configure alert pin polarity */
	SENSOR_ATTR_TMP11X_ALERT_PIN_POLARITY,
	/** Configure alert mode */
	SENSOR_ATTR_TMP11X_ALERT_MODE,
	/** Configure alert pin mode for alert or DR*/
	SENSOR_ATTR_TMP11X_ALERT_PIN_SELECT,
};

/**
 * @name Alert pin support macros
 * @{
 */
#define TMP11X_ALERT_PIN_ACTIVE_LOW  0 /**< Alert pin is active low */
#define TMP11X_ALERT_PIN_ACTIVE_HIGH 1 /**< Alert pin is active high */
#define TMP11X_ALERT_ALERT_MODE      0 /**< Alert mode */
#define TMP11X_ALERT_THERM_MODE      1 /**< Therm mode */
#define TMP11X_ALERT_PIN_ALERT_SEL   0 /**< Alert pin reflects the status of the alert flags */
#define TMP11X_ALERT_PIN_DR_SEL      1 /**< Alert pin reflects the status of the data ready flag */
/** @} */

/**
 * @brief EEPROM size for TMP11X
 */
#define EEPROM_TMP11X_SIZE (4 * sizeof(uint16_t))

/**
 * @brief Read from EEPROM
 * @param dev Pointer to a tmp11x device
 * @param offset Offset in the EEPROM
 * @param data Pointer to a buffer to store the read data
 * @param len Length of the data to read
 * @return 0 on success, negative error code on failure
 */
int tmp11x_eeprom_read(const struct device *dev, off_t offset, void *data, size_t len);

/**
 * @brief Write to EEPROM
 * @param dev Pointer to a tmp11x device
 * @param offset Offset in the EEPROM
 * @param data Pointer to the data to write
 * @param len Length of the data to write
 * @return 0 on success, negative error code on failure
 */
int tmp11x_eeprom_write(const struct device *dev, off_t offset, const void *data, size_t len);

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TMP11X_H_ */
