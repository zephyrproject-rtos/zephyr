/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_STRIPPED_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_STRIPPED_

#include <stdint.h>
#include <zephyr/sys/util_macro.h>

#ifndef SENSOR_G
#define SENSOR_G 9806650LL
#endif

/* Define struct sensor_value here to avoid including include/zepyhr/drivers/sensor.h */
struct sensor_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

#endif // ZEPHYR_INCLUDE_DRIVERS_SENSOR_H_STRIPPED_
