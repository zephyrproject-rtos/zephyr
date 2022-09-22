/* vl180_types.h - Zephyr customization of ST vl53l0x library,
 * basic type definition.
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL6180X_VL6180X_TYPES_H_
#define ZEPHYR_DRIVERS_SENSOR_VL6180X_VL6180X_TYPES_H_


/* Zephyr provides stdint.h and stddef.h, so this is enough to include it.
 * If it was not the case, we would defined here all signed and unsigned
 * basic types...
 */
#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#error "Error NULL definition should be done. Please add required include "
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_VL6180X_VL6180X_TYPES_H_ */
