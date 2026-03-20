/*
 * Copyright (c) 2026 Google, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief Header file for extended sensor API of PAC194x/PAC195x sensor
  */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSORS_PAC194X_H
#define ZEPHYR_INCLUDE_DRIVERS_SENSORS_PAC194X_H

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/** Additional attributes supported by the PAC194x/PAC195x */
enum pac194x_sensor_attribute {
	/** Sampling frequency in RAW format. */
	SENSOR_ATTR_SAMPLING_FREQUENCY_RAW = SENSOR_ATTR_PRIV_START,
	/** Enable/disable specific channel. */
	SENSOR_ATTR_CHANNEL_ENABLED,
	/** PAC refresh mode. */
	SENSOR_ATTR_REFRESH_MODE,
	/** Command: force refresh PAC accumulated data */
	SENSOR_ATTR_FORCE_REFRESH_CMD,
};

/** Refresh modes for PAC194x/PAC195x */
enum pac194x_sensor_attr_refresh_mode {
	/**
	 * sample_fetch is non-blocking. Refresh is being
	 * called after samples are collected from ACC registers.
	 */
	PAC194X_SENSOR_ATTR_REFRESH_MODE_AUTO_NOWAIT = 0,
	/**
	 * sample_fetch is blocking. Refresh is being
	 * called before samples are collected from ACC registers and
	 * function waits at least 1ms for data to be accessible.
	 */
	PAC194X_SENSOR_ATTR_REFRESH_MODE_AUTO_WAIT,
	/**
	 * sample_fetch is non-blocking and refresh is not being
	 * called automatically. The user is responsible for
	 * sending an appropriate REFRESH command.
	 */
	PAC194X_SENSOR_ATTR_REFRESH_MODE_MANUAL,
};

/** Refresh commands for PAC194x/PAC195x */
enum pac194x_sensor_attr_force_refresh_cmd {
	/** Refresh only a singe PAC device. */
	PAC194X_SENSOR_ATTR_FORCE_REFRESH_CMD_SINGLE = 0,
	/** Refresh all PACs on the I2C bus at once. */
	PAC194X_SENSOR_ATTR_FORCE_REFRESH_CMD_ALL,
};

/** Additional channels supported by the PAC194x/PAC195x */
enum pac194x_sensor_channel {
	/** Channel 1: voltage sensor */
	PAC194X_CHAN_VBUS1 = SENSOR_CHAN_PRIV_START,
	/** Channel 2: voltage sensor */
	PAC194X_CHAN_VBUS2,
	/** Channel 3: voltage sensor */
	PAC194X_CHAN_VBUS3,
	/** Channel 4: voltage sensor */
	PAC194X_CHAN_VBUS4,
	/** Channel 1: current sensor */
	PAC194X_CHAN_CURR1,
	/** Channel 2: current sensor */
	PAC194X_CHAN_CURR2,
	/** Channel 3: current sensor */
	PAC194X_CHAN_CURR3,
	/** Channel 4: current sensor */
	PAC194X_CHAN_CURR4,
	/** Channel 1: accumulator RAW value */
	PAC194X_CHAN_ACC1,
	/** Channel 2: accumulator RAW value */
	PAC194X_CHAN_ACC2,
	/** Channel 3: accumulator RAW value */
	PAC194X_CHAN_ACC3,
	/** Channel 4: accumulator RAW value */
	PAC194X_CHAN_ACC4,
	/** Channel 1: accumulator AVG value over SAMPLE_COUNT */
	PAC194X_CHAN_ACC1_AVG,
	/** Channel 2: accumulator AVG value over SAMPLE_COUNT */
	PAC194X_CHAN_ACC2_AVG,
	/** Channel 3: accumulator AVG value over SAMPLE_COUNT */
	PAC194X_CHAN_ACC3_AVG,
	/** Channel 4: accumulator AVG value over SAMPLE_COUNT */
	PAC194X_CHAN_ACC4_AVG,
	/** Accumulator SAMPLE_COUNT since last REFRESH */
	PAC194X_CHAN_ACC_COUNT,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSORS_PAC194X_H */
