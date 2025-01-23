/*
 * Copyright (c) 2024 Michal Piekos
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Custom channels and values for VL53L0X ToF Sensor
 *
 * These channels provide additional sensor data not covered by the standard
 * Zephyr sensor channels. Application must include vl53l0x.h file to gain
 * access to these channels.
 *
 * Example usage:
 * @code{c}
 * #include <zephyr/drivers/sensor/vl53l0x.h>
 *
 * if (sensor_channel_get(dev, SENSOR_CHAN_VL53L0X_RANGE_STATUS, &value)) {
 *	printk("Status: %d\n", value.val1);
 * }
 * @endcode
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_

#include <zephyr/drivers/sensor.h>

/* VL53L0x specific channels */
enum sensor_channel_vl53l0x {
	SENSOR_CHAN_VL53L0X_RANGE_DMAX = SENSOR_CHAN_PRIV_START,
	SENSOR_CHAN_VL53L0X_SIGNAL_RATE_RTN_CPS,
	SENSOR_CHAN_VL53L0X_AMBIENT_RATE_RTN_CPS,
	SENSOR_CHAN_VL53L0X_EFFECTIVE_SPAD_RTN_COUNT,
	SENSOR_CHAN_VL53L0X_RANGE_STATUS,
};

/* VL53L0x meas status values */
#define VL53L0X_RANGE_STATUS_RANGE_VALID    (0)
#define VL53L0X_RANGE_STATUS_SIGMA_FAIL	    (1)
#define VL53L0X_RANGE_STATUS_SIGNAL_FAIL    (2)
#define VL53L0X_RANGE_STATUS_MIN_RANGE_FAIL (3)
#define VL53L0X_RANGE_STATUS_PHASE_FAIL	    (4)
#define VL53L0X_RANGE_STATUS_HARDWARE_FAIL  (5)
#define VL53L0X_RANGE_STATUS_NO_UPDATE      (255)

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_VL53L0X_H_ */
