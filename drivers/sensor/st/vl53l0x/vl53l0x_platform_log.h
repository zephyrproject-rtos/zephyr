/* vl53l0x_platform_log.h - Zephyr customization of ST vl53l0x library,
 * logging functions, not implemented
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L0X_VL53L0X_PLATFORM_LOG_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L0X_VL53L0X_PLATFORM_LOG_H_

#include <stdio.h>
#include <string.h>
/* LOG Functions */

#ifdef __cplusplus
extern "C" {
#endif

enum {
	TRACE_LEVEL_NONE,
	TRACE_LEVEL_ERRORS,
	TRACE_LEVEL_WARNING,
	TRACE_LEVEL_INFO,
	TRACE_LEVEL_DEBUG,
	TRACE_LEVEL_ALL,
	TRACE_LEVEL_IGNORE
};

enum {
	TRACE_FUNCTION_NONE = 0,
	TRACE_FUNCTION_I2C  = 1,
	TRACE_FUNCTION_ALL  = 0x7fffffff /* all bits except sign */
};

enum {
	TRACE_MODULE_NONE              = 0x0,
	TRACE_MODULE_API               = 0x1,
	TRACE_MODULE_PLATFORM          = 0x2,
	TRACE_MODULE_ALL               = 0x7fffffff /* all bits except sign */
};

#define _LOG_FUNCTION_START(module, fmt, ...) (void)0
#define _LOG_FUNCTION_END(module, status, ...) (void)0
#define _LOG_FUNCTION_END_FMT(module, status, fmt, ...) (void)0

#define VL53L0X_COPYSTRING(str, ...) strcpy(str, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_DRIVERS_SENSOR_VL53L0X_VL53L0X_PLATFORM_LOG_H_ */
