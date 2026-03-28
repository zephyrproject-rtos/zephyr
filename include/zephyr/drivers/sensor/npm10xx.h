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
 * @ingroup sensor_interface_ext
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
};

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_NPM10XX_H_ */
