/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __XLNX_RTC_H__
#define __XLNX_RTC_H__

#include "rtc_utils.h"

/* Bit map for calibration register */
#define XLNX_RTC_CALIB_WR_OFFSET	BIT(3)
#define XLNX_RTC_CALIB_RD_OFFSET	BIT(3) | BIT(2)

/* Bit map for control register */
#define XLNX_RTC_CTL_OFFSET	BIT(6)

/* Bit map for oscillator and Battery enable */
#define XLNX_RTC_OSC_EN		BIT(24)
#define XLNX_RTC_BATTERY_EN	BIT(31)
#define RTC_OSCILLATOR_FREQ	BIT(15)

/* Bit map for Interrupt Status Register */
#define XLNX_RTC_INT_STS_OFFSET	0x20
#define XLNX_RTC_SECS_MASK	BIT(0)
#define XLNX_RTC_ALARM_MASK	BIT(1)

/* Bit map for Interrupt Registers */
#define XLNX_RTC_INT_DIS_OFFSET	0x2c
#define XLNX_RTC_INT_ENA_OFFSET	0x28

/* Bit map for Current time register */
#define XLNX_RTC_CUR_TIM_OFFSET	0x10

/* Bit map for Set time register */
#define XLNX_RTC_SET_TIM_OFFSET	0x00
#define XLNX_RTC_SET_TIM_READ_OFFSET	0x04

/* Bit map for Calibration read register */
#define XLNX_RTC_CALIB_READ_OFFSET	0x0c

/* Bit map for Calibration write register */
#define XLNX_RTC_CALIB_WRITE_OFFSET	0x08

/* Bit map for Alarm register */
#define XLNX_RTC_ALARM_OFFSET	0x18

#define XLNX_RTC_ALARM_TIME_MASK								\
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |	\
	RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR)

/* Mask to get the lower 16 bits of the calibration tick value */
#define RTC_TICK_MASK	0xFFFF

/* Maximum number of fractional ticks supported by the hardware */
#define RTC_FR_MAX_TICKS	16

/* Bit shift value to extract fractional tick data from calibration register */
#define RTC_FR_DATSHIFT	16

/* Mask to extract fractional tick bits from calibration register */
#define RTC_FR_MASK	0xF0000

/* Default value for calibration offset */
#define RTC_CALIB_DEF	0x7FFF

/* RTC timing resolution in parts-per-billion (ppb) */
#define RTC_PPB	1000000000ULL

/* Minimum allowed calibration offset in ppb */
#define RTC_MIN_OFFSET	-32768000

/* Maximum allowed calibration offset in ppb */
#define RTC_MAX_OFFSET	32767000

/* Bit mask to check if fractional tick compensation is enabled */
#define RTC_FR_EN	BIT(20)

#endif
