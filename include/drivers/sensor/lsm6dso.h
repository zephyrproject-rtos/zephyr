/*
 * Copyright (c) 2022, Linus Reitmayr
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/lsm6dso.pdf
 */

/**
 * @file
 * @brief Extended public API for the lsm6dso driver
 *
 * This exposes one attribute for the lsm6dso which can be used for
 * setting the low pass filter cut-off frequency.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSO_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <drivers/sensor.h>

/** @brief Sensor specific attributes of LSM6DSO */
enum sensor_attribute_lsm6dso {
	/** Sensor CPI for both X and Y axes. */
	SENSOR_ATTR_LPF_FREQ = SENSOR_ATTR_PRIV_START,
};

#if defined(CONFIG_LSM6DSO_ENABLE_FIFO)
enum sensor_trigger_type_lsm6dso {
	/** Counter batch data rate trigger */
	SENSOR_TRIG_LSM6DSO_CNT_BDR = SENSOR_TRIG_PRIV_START,
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_LSM6DSO_H_ */
