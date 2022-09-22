/* vl6180_platform_log.h - Zephyr customization of ST vl6180 library,
 * logging functions, not implemented
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_SENSOR_VL6180X_VL6180X_PLATFORM_LOG_H_
#define ZEPHYR_DRIVERS_SENSOR_VL6180X_VL6180X_PLATFORM_LOG_H_

void OnErrLog(void);
#define LOG_FUNCTION_START(...) (void)0
#define LOG_FUNCTION_END(...) (void)0
#define LOG_FUNCTION_END_FMT(...) (void)0
#define VL6180_ErrLog(...) (void)0

#endif  /* ZEPHYR_DRIVERS_SENSOR_VL6180X_VL6180X_PLATFORM_LOG_H_ */

