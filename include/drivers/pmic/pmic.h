/* pmic.h - PMIC (battery charger etc.) driver interface */

/*
 * Copyright (c) 2018 Makaio GmbH.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PMIC device controller APIs
 *
 * This file contains the PMIC device APIs.
 * Depending on the actual capabilities of the specific device
 * drivers should implement the corresponding APIs described
 * in this file.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_PMIC_PMIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_PMIC_PMIC_H_

#include <zephyr.h>

typedef int (*pmic_gauge_voltage_get)(void);
typedef float (*pmic_gauge_soc_get)(void);

/** @brief Fuel gauge specific API **/
struct pmic_gauge_api {
	pmic_gauge_voltage_get voltage_get;
	pmic_gauge_soc_get soc_get;
};

typedef int (*pmic_charger_voltage_set)(u16_t millivolts);
typedef u16_t (*pmic_charger_voltage_get)(void);

#ifdef CONFIG_PMIC_ENABLE_SENSOR_DRIVER
void pmic_gauge_work_cb(struct k_work *work);

int pmic_gauge_attr_set(struct device *dev,
		      enum sensor_channel chan,
		      enum sensor_attribute attr,
		      const struct sensor_value *val);

int pmic_gauge_trigger_set(struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif

/** @brief Battery charger specific API **/
struct pmic_charger_api {
	pmic_charger_voltage_set charge_voltage_set;
	pmic_charger_voltage_set charge_voltage_get;
};

typedef int (*pmic_boost_disable)(void);
typedef int (*pmic_boost_enable)(void);
typedef int (*pmic_boost_voltage_set)(u16_t millivolts);
typedef u16_t (*pmic_boost_voltage_get)(void);

/** @brief API for PMICs including boost capabilities **/
struct pmic_boost_api {
	pmic_boost_disable boost_disable;
	pmic_boost_enable boost_enable;
	pmic_boost_voltage_set boost_voltage_set;
	pmic_boost_voltage_get boost_voltage_get;
};

struct pmic_api {
	const struct pmic_gauge_api *gauge_api;
	const struct pmic_charger_api *charger_api;
	const struct pmic_boost_api *boost_api;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_PMIC_PMIC_H_ */
