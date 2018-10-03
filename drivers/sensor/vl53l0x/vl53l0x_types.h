/* vl53l0x_types.h - Zephyr customization of ST vl53l0x library,
 * basic type definition.
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L0X_VL53L0X_TYPES_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L0X_VL53L0X_TYPES_H_


/* Zephyr provides stdint.h and stddef.h, so this is enough to include it.
 * If it was not the case, we would defined here all signed and unsigned
 * basic types...
 */
#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#error "Error NULL definition should be done. Please add required include "
#endif

#if !defined(STDINT_H) && !defined(_GCC_STDINT_H) && !defined(__STDINT_DECLS) \
	&& !defined(_GCC_WRAP_STDINT_H) && !defined(_STDINT_H)                \
	&& !defined(__INC_stdint_h__)
 #pragma message("Review type definition of STDINT define for your platform")
#endif /* _STDINT_H */

/** use where fractional values are expected
 *
 * Given a floating point value f it's .16 bit point is (int)(f*(1<<16))
 */
typedef uint32_t FixPoint1616_t;

#endif /* ZEPHYR_DRIVERS_SENSOR_VL53L0X_VL53L0X_TYPES_H_ */
