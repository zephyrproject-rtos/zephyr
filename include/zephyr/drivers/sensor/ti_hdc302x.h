/*
 * Copyright (c) 2025 Psicontrol N.V.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Extended public API for HDC302X Temperature Sensors
 *
 * This exposes attributes for the HDC302X which can be used for
 * setting the Low power parameters.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_HDC302X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_HDC302X_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_hdc302x {
	/* Sensor low power Mode
	 * Rather than set this value directly, can only be set to operate in one of four modes:
	 *
	 * HDC302X_SENSOR_POWER_MODE_0
	 * HDC302X_SENSOR_POWER_MODE_1
	 * HDC302X_SENSOR_POWER_MODE_2
	 * HDC302X_SENSOR_POWER_MODE_3
	 *
	 * See datasheet for more info on different modes.
	 */
	SENSOR_ATTR_POWER_MODE = SENSOR_ATTR_PRIV_START + 1,

	/* Sensor Automatic Measurement Mode
	 * Can only be set to one of the following values:
	 *
	 * HDC302X_SENSOR_MEAS_INTERVAL_MANUAL,
	 * HDC302X_SENSOR_MEAS_INTERVAL_0_5,
	 * HDC302X_SENSOR_MEAS_INTERVAL_1,
	 * HDC302X_SENSOR_MEAS_INTERVAL_2,
	 * HDC302X_SENSOR_MEAS_INTERVAL_4,
	 * HDC302X_SENSOR_MEAS_INTERVAL_10,
	 */
	SENSOR_ATTR_INTEGRATION_TIME,
	/* Sensor status register */
	SENSOR_ATTR_STATUS_REGISTER,
	/* Sensor heater level */
	SENSOR_ATTR_HEATER_LEVEL, /* Heater level (0-14) */
};

enum sensor_power_mode_hdc302x {
	HDC302X_SENSOR_POWER_MODE_0, /* (lowest noise) */
	HDC302X_SENSOR_POWER_MODE_1,
	HDC302X_SENSOR_POWER_MODE_2,
	HDC302X_SENSOR_POWER_MODE_3, /* (lowest power) */

	HDC302X_SENSOR_POWER_MODE_MAX
};

enum sensor_measurement_interval_hdc302x {
	HDC302X_SENSOR_MEAS_INTERVAL_MANUAL, /*  Manual Mode                 */
	HDC302X_SENSOR_MEAS_INTERVAL_0_5,    /*  1 Measurement per 2 Seconds */
	HDC302X_SENSOR_MEAS_INTERVAL_1,      /*  1 Measurement per Second    */
	HDC302X_SENSOR_MEAS_INTERVAL_2,      /*  2 Measurements per Second   */
	HDC302X_SENSOR_MEAS_INTERVAL_4,      /*  4 Measurements per Second   */
	HDC302X_SENSOR_MEAS_INTERVAL_10,     /* 10 Measurements per Second   */

	HDC302X_SENSOR_MEAS_INTERVAL_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_HDC302X_H_ */
