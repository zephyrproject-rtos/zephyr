/* vl53l1_platform_user_data.h - Zephyr customization of ST vl53l1x library. */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_PLATFORM_USER_DATA_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_PLATFORM_USER_DATA_H_

#include "vl53l1_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct VL53L1_Dev_t
 * @brief  Generic PAL device type that does link between API and platform
 * abstraction layer
 *
 */
typedef struct {
	/*!< Low Level Driver data structure */
	VL53L1_DevData_t Data;
	/*!< New data ready poll duration in ms - for debug */
	uint32_t new_data_ready_poll_duration_ms;
	/*!< I2C device handle */
	const struct i2c_dt_spec *i2c;
} VL53L1_Dev_t;


/**
 * @brief   Declare the device Handle as a pointer of the structure @a VL53L1_Dev_t.
 *
 */
typedef VL53L1_Dev_t *VL53L1_DEV;

/**
 * @def VL53L1PALDevDataGet
 * @brief Get ST private structure @a VL53L1_DevData_t data access
 *
 * @param Dev       Device Handle
 * @param field     ST structure field name
 * It maybe used and as real data "ref" not just as "get" for sub-structure item
 * like PALDevDataGet(FilterData.field)[i] or
 * PALDevDataGet(FilterData.MeasurementIndex)++
 */
#define VL53L1DevDataGet(Dev, field) (Dev->Data.field)


/**
 * @def VL53L1PALDevDataSet(Dev, field, data)
 * @brief  Set ST private structure @a VL53L1_DevData_t data field
 * @param Dev       Device Handle
 * @param field     ST structure field name
 * @param data      Data to be set
 */
#define VL53L1DevDataSet(Dev, field, data) ((Dev->Data.field) = (data))


/**
 * @def VL53L1DevStructGetLLDriverHandle
 * @brief Get LL Driver handle @a VL53L0_Dev_t data access
 *
 * @param Dev      Device Handle
 */
#define VL53L1DevStructGetLLDriverHandle(Dev) (&Dev->Data.LLData)

/**
 * @def VL53L1DevStructGetLLResultsHandle
 * @brief Get LL Results handle @a VL53L0_Dev_t data access
 *
 * @param Dev      Device Handle
 */
#define VL53L1DevStructGetLLResultsHandle(Dev) (&Dev->Data.llresults)

#ifdef __cplusplus
}
#endif

#endif /*ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_PLATFORM_USER_DATA_H_*/
