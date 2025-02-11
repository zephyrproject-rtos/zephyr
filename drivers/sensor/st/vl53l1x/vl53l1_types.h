/* vl53l1_types.h - Zephyr customization of ST vl53l1x library, basic type definition. */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_TYPES_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_TYPES_H_


/* Zephyr provides stdint.h and stddef.h, so this is enough to include it.
 * If it was not the case, we would defined here all signed and unsigned
 * basic types...
 */
#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#error "Error NULL definition should be done. Please add required include "
#endif

/** use where fractional values are expected
 *
 * Given a floating point value f it's .16 bit point is (int)(f*(1<<16))
 */
typedef uint32_t FixPoint1616_t;

#endif /* ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_TYPES_H_ */
