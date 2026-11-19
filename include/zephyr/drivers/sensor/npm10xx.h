/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM10XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM10XX_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @file npm10xx.h
 * @brief Nordic's nPM10 Series PMIC Sensor driver custom channels and attributes
 * @defgroup sensor_interface_npm10xx nPM10xx Sensor interface
 * @ingroup sensor_interface_ext_nordic
 * @{
 */

/** nPM10xx PMIC Sensor custom channels. Extends enum sensor_channel */
enum npm10xx_sensor_chan {
	/** VSYS voltage, in volts */
	NPM10XX_SENSOR_CHAN_VSYS = SENSOR_CHAN_PRIV_START,
	/** VBUS voltage, in volts */
	NPM10XX_SENSOR_CHAN_VBUS,
	/** ADC offset value */
	NPM10XX_SENSOR_CHAN_OFFSET,
	/** Digital charge current (not ADC result), in ampere */
	NPM10XX_SENSOR_CHAN_GAUGE_DIG_CURRENT,
};

/** nPM10xx PMIC Sensor custom attributes. Extends enum sensor_attribute */
enum npm10xx_sensor_attr {
	/**
	 * Configure PMIC supply selection during measurement.
	 * Choose one of @ref supply_select "NPM10XX_SUPPLY_SELECT_*" values.
	 */
	NPM10XX_SENSOR_ATTR_SUPPLY_SELECT = SENSOR_ATTR_PRIV_START,
};

/**
 * @name PMIC supply selection during measurement
 * @anchor supply_select
 * @{
 */
/** Disable supply selection */
#define NPM10XX_SUPPLY_SELECT_DISABLE   0
/** Select VBAT source but return back after the measurement */
#define NPM10XX_SUPPLY_SELECT_VBAT      BIT(0)
/** Select VSYS source but return back after the measurement */
#define NPM10XX_SUPPLY_SELECT_VSYS      BIT_MASK(2)
/** Select VBAT source and keep it after the measurement */
#define NPM10XX_SUPPLY_SELECT_VBAT_KEEP (BIT(0) | BIT(2))
/** Select VSYS source and keep it after the measurement */
#define NPM10XX_SUPPLY_SELECT_VSYS_KEEP BIT_MASK(3)
/**
 * @}
 */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM10XX_H_ */
