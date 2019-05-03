/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _VL53L1_PLATFORM_H_
#define _VL53L1_PLATFORM_H_

#include "vl53l1_ll_def.h"
#include "vl53l1_platform_log.h"

#define VL53L1_IPP_API
#include "vl53l1_platform_user_data.h"

#ifdef __cplusplus
extern "C"
{
#endif

VL53L1_Error VL53L1_CommsInitialise(
	VL53L1_Dev_t *pdev,
	uint8_t       comms_type,
	uint16_t      comms_speed_khz);

VL53L1_Error VL53L1_CommsClose(
	VL53L1_Dev_t *pdev);

VL53L1_Error VL53L1_WriteMulti(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint8_t      *pdata,
		uint32_t      count);

VL53L1_Error VL53L1_ReadMulti(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint8_t      *pdata,
		uint32_t      count);

VL53L1_Error VL53L1_WrByte(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint8_t       VL53L1_PRM_00005);

VL53L1_Error VL53L1_WrWord(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint16_t      VL53L1_PRM_00005);

VL53L1_Error VL53L1_WrDWord(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint32_t      VL53L1_PRM_00005);

VL53L1_Error VL53L1_RdByte(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint8_t      *pdata);

VL53L1_Error VL53L1_RdWord(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint16_t     *pdata);

VL53L1_Error VL53L1_RdDWord(
		VL53L1_Dev_t *pdev,
		uint16_t      index,
		uint32_t     *pdata);

VL53L1_Error VL53L1_WaitUs(
		VL53L1_Dev_t *pdev,
		int32_t       wait_us);

VL53L1_Error VL53L1_WaitMs(
		VL53L1_Dev_t *pdev,
		int32_t       wait_ms);

VL53L1_Error VL53L1_GetTimerFrequency(int32_t *ptimer_freq_hz);

VL53L1_Error VL53L1_GetTimerValue(int32_t *ptimer_count);

VL53L1_Error VL53L1_GpioSetMode(uint8_t pin, uint8_t mode);

VL53L1_Error VL53L1_GpioSetValue(uint8_t pin, uint8_t value);

VL53L1_Error VL53L1_GpioGetValue(uint8_t pin, uint8_t *pvalue);

VL53L1_Error VL53L1_GpioXshutdown(uint8_t value);

VL53L1_Error VL53L1_GpioCommsSelect(uint8_t value);

VL53L1_Error VL53L1_GpioPowerEnable(uint8_t value);

VL53L1_Error  VL53L1_GpioInterruptEnable(void (*function)(void), uint8_t edge_type);

VL53L1_Error  VL53L1_GpioInterruptDisable(void);

VL53L1_Error VL53L1_GetTickCount(
	uint32_t *ptime_ms);

VL53L1_Error VL53L1_WaitValueMaskEx(
		VL53L1_Dev_t *pdev,
		uint32_t      timeout_ms,
		uint16_t      index,
		uint8_t       value,
		uint8_t       mask,
		uint32_t      poll_delay_ms);

#ifdef __cplusplus
}
#endif

#endif
