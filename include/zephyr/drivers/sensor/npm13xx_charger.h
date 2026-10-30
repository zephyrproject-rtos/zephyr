/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of nPM13xx charger
 * @ingroup npm13xx_charger_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM13XX_CHARGER_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM13XX_CHARGER_H_

#include <zephyr/drivers/sensor.h>

/**
 * @brief Nordic nPM13xx PMIC battery charger
 * @defgroup npm13xx_charger_interface nPM13xx charger
 * @ingroup sensor_interface_ext_nordic
 * @{
 */

/** @brief Custom sensor channels for the nPM13xx charger */
enum sensor_channel_npm13xx_charger {
	/** Charger status register value */
	SENSOR_CHAN_NPM13XX_CHARGER_STATUS = SENSOR_CHAN_PRIV_START,
	/** Charger error reason register value */
	SENSOR_CHAN_NPM13XX_CHARGER_ERROR,
	/**
	 * VBUS status register value. Read individual flags with sensor_attr_get() and the
	 * @ref sensor_attribute_npm13xx_charger attributes.
	 */
	SENSOR_CHAN_NPM13XX_CHARGER_VBUS_STATUS,
};

/**
 * @brief Custom sensor attributes for the nPM13xx charger
 *
 * VBUS status flags, read with sensor_attr_get() on
 * @ref SENSOR_CHAN_NPM13XX_CHARGER_VBUS_STATUS. @c sensor_value.val1 is 1 when the flag is set.
 */
enum sensor_attribute_npm13xx_charger {
	/** VBUS is present */
	SENSOR_ATTR_NPM13XX_CHARGER_VBUS_PRESENT = SENSOR_ATTR_PRIV_START,
	/** VBUS current limit is active */
	SENSOR_ATTR_NPM13XX_CHARGER_VBUS_CUR_LIMIT,
	/** VBUS overvoltage protection is active */
	SENSOR_ATTR_NPM13XX_CHARGER_VBUS_OVERVLT_PROT,
	/** VBUS undervoltage is detected */
	SENSOR_ATTR_NPM13XX_CHARGER_VBUS_UNDERVLT,
	/** VBUS is suspended */
	SENSOR_ATTR_NPM13XX_CHARGER_VBUS_SUSPENDED,
	/** VBUSOUT is active */
	SENSOR_ATTR_NPM13XX_CHARGER_VBUS_BUSOUT,
};

/** @} */

#endif
