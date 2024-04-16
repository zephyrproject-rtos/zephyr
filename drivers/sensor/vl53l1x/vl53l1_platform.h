/* vl53l1_platform.h - Zephyr customization of ST vl53l1x library. */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_PLATFORM_H_
#define ZEPHYR_DRIVERS_SENSOR_VL53L1X_VL53L1_PLATFORM_H_

#include <string.h>
#include "vl53l1_ll_def.h"
#include "vl53l1_platform_log.h"
#include "vl53l1_platform_user_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Writes the supplied byte buffer to the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param pdata Pointer to uint8_t (byte) buffer containing the data to be written
 * @param count Number of bytes in the supplied byte buffer
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WriteMulti(VL53L1_Dev_t *pdev, uint16_t index,
		 uint8_t *pdata, uint32_t count);

/**
 * Reads the requested number of bytes from the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param pdata Pointer to the uint8_t (byte) buffer to store read data
 * @param count Number of bytes to read
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_ReadMulti(VL53L1_Dev_t *pdev, uint16_t index,
		uint8_t *pdata, uint32_t count);

/**
 * Writes a single byte to the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param data  Data value to write
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WrByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t data);

/**
 * Writes a single word (16-bit unsigned) to the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param data  Data value write
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WrWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t data);

/**
 * Writes a single dword (32-bit unsigned) to the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param data  Data value to write
 * @return VL53L1_ERROR_NONE Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WrDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t data);

/**
 * Reads a single byte from the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index
 * @param pdata Pointer to uint8_t data value
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_RdByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata);

/**
 * Reads a single word (16-bit unsigned) from the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param pdata Pointer to uint16_t data value
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_RdWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t *pdata);

/**
 * Reads a single double word (32-bit unsigned) from the device
 * @param pdev  Pointer to device structure (device handle)
 * @param index Register index value
 * @param pdata Pointer to uint32_t data value
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_RdDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t *pdata);

/**
 * Implements a programmable wait in us
 * @param pdev Pointer to device structure (device handle)
 * @param wait_us Integer wait in micro seconds
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, int32_t wait_us);

/**
 * Implements a programmable wait in ms
 * @param pdev Pointer to device structure (device handle)
 * @param wait_ms Integer wait in milliseconds
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, int32_t wait_ms);

/**
 * Gets current system tick count in [ms]
 * @param Pointer to current time in [ms]
 * @return VL53L1_ERROR_NONE Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_GetTickCount(uint32_t *ptime_ms);

/**
 * Register "wait for value" polling routine
 * @param dev		Pointer to device structure (device handle)
 * @param timeout	Timeout in [ms]
 * @param i			Register index value
 * @param val		Value to wait for
 * @param mask		Mask to be applied before comparison with value
 * @param delay		Polling delay between each read transaction in [ms]
 * @return VL53L1_ERROR_NONE  Success
 * @return "Other error code" See ::VL53L1_Error
 */
VL53L1_Error VL53L1_WaitValueMaskEx(VL53L1_Dev_t *dev, uint32_t timeout, uint16_t i, uint8_t val,
	uint8_t mask, uint32_t delay);

#ifdef __cplusplus
}
#endif

#endif
