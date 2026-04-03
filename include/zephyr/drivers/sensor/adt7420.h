/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /**
  * @file
  * @brief Header file for extended sensor API of ADT7420 sensor
  * @ingroup adt7420_interface
  */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADT7420_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADT7420_H_

/**
 * @brief Analog Devices ADT7420 Temperature Sensor
 * @defgroup adt7420_interface ADT7420
 * @ingroup sensor_interface_ext
 * @{
 */

#include <zephyr/drivers/sensor.h>

/** Critical temperature trigger channel for use with sensor_trigger_set() */
#define SENSOR_CHAN_ADT7420_CRIT_TEMP SENSOR_CHAN_PRIV_START

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ADT7420_H_ */
