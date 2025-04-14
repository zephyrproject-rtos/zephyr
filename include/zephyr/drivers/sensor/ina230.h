/*
 * Copyright (c) 2024, Tomas Jurena
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for TI INA230/INA236 current-shunt and power monitor
 *
 * This exposes two triggers for the INA230/INA236 which can be used for
 * setting trigger channel being above or bellow defined alert limit.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA230_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA230_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** @brief sensor_trigger_type_ina230 enum
 *
 * Selects if IRQ line is asserted when selected source is above or below
 * configured threshold.
 */
enum sensor_trigger_type_ina230 {
	/** Alert over limit trigger. */
	SENSOR_TRIG_INA230_OVER = SENSOR_TRIG_PRIV_START,
	/** Alert bellow trigger. */
	SENSOR_TRIG_INA230_UNDER,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_INA230_H_ */
