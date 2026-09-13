/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup sensor_interface
 * @brief TI VTM (Voltage and Thermal Manager) extended sensor API
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TI_VTM_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TI_VTM_H_

/**
 * @brief TI VTM sensor driver API extension
 * @defgroup ti_vtm_sensor_interface TI VTM temperature sensor
 * @since 4.5.0
 * @version 0.1.0
 * @ingroup sensor_interface
 * @{
 */

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief VTM-specific thermal trip trigger types
 *
 * Three independent temperature trip thresholds, each triggering an interrupt
 * when the die temperature crosses the configured boundary. Configure thresholds
 * via sensor_attr_set() with vtm_sensor_attribute, then register handlers via
 * sensor_trigger_set().
 */
enum ti_vtm_sensor_trigger_type {
	/** Low temperature trip (LT_TH0): fires when temp falls below threshold */
	TI_VTM_TRIG_TH0 = SENSOR_TRIG_PRIV_START + 0,
	/** High temperature trip (GT_TH1): fires when temp rises above threshold */
	TI_VTM_TRIG_TH1 = SENSOR_TRIG_PRIV_START + 1,
	/** Critical temperature trip (GT_TH2): fires when temp rises above threshold */
	TI_VTM_TRIG_TH2 = SENSOR_TRIG_PRIV_START + 2,
};

/**
 * @brief VTM-specific sensor attributes
 *
 * VTM-specific attributes for sensor_attr_set() / sensor_attr_get().
 *
 */
enum vtm_sensor_attribute {
	/** Cold threshold in milli-Celsius (LT_TH0) */
	TI_VTM_ATTR_TH0_THRESH = SENSOR_ATTR_PRIV_START + 0,
	/** Hot threshold in milli-Celsius (GT_TH1) */
	TI_VTM_ATTR_TH1_THRESH = SENSOR_ATTR_PRIV_START + 1,
	/** Critical threshold in milli-Celsius (GT_TH2) */
	TI_VTM_ATTR_TH2_THRESH = SENSOR_ATTR_PRIV_START + 2,
	/**
	 * Read-only: MAXT_OUTRG_ALERT status.
	 * val1 = 1 while temperature exceeds maxt-outrg-thresh.
	 * val1 = 0 once temperature drops below maxt-outrg-thresh0.
	 */
	TI_VTM_ATTR_OUTRG_ALERT = SENSOR_ATTR_PRIV_START + 3,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TI_VTM_H_ */
