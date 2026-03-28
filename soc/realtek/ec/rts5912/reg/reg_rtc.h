/*
 * Copyright (c) 2025 Realtek, SIBG-SD7
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_REALTEK_RTS5912_REG_RTC_H
#define ZEPHYR_SOC_REALTEK_RTS5912_REG_RTC_H

#include <zephyr/kernel.h>

/*
 * @brief RTC Controller (RTC)
 */

typedef struct {
	volatile uint8_t SEC;
	volatile uint8_t SECALARM;
	volatile uint8_t MIN;
	volatile uint8_t MINALARM;
	volatile uint8_t HR;
	volatile uint8_t HRALARM;
	volatile uint8_t DAYWEEK;
	volatile uint8_t DAYMONTH;
	volatile uint8_t MONTH;
	volatile uint8_t YEAR;
	volatile uint8_t CTRL0;
	volatile uint8_t CTRL1;
	volatile uint8_t CTRL2;
	volatile uint8_t DAYWEEKALARM;
	volatile uint8_t DAYMONTHALARM;
	volatile const uint8_t RESERVED;
	volatile const uint32_t RESERVED1[2];
	volatile uint32_t DLSFW;
	volatile uint32_t DLSBW;
	volatile uint32_t WEEK;
} RTC_Type;

/* HR */
#define RTC_HR_VAL_Pos         (0UL)
#define RTC_HR_VAL_Msk         GENMASK(5, 0)
#define RTC_HR_AMPM_Pos        (7UL)
#define RTC_HR_AMPM_Msk        BIT(RTC_HR_AMPM_Pos)
/* DAYWEEK */
#define RTC_DAYWEEK_Pos        (0UL)
#define RTC_DAYWEEK_Msk        GENMASK(3, 0)
/* DAYMONTH */
#define RTC_DAYMONTH_Pos       (0UL)
#define RTC_DAYMONTH_Msk       GENMASK(5, 0)
/* CTRL0 */
#define RTC_CTRL0_DIVCTL_Pos   (4UL)
#define RTC_CTRL0_DIVCTL_Msk   GENMASK(6, 4)
/* CTRL1 */
#define RTC_CTRL1_HRMODE_Pos   (1UL)
#define RTC_CTRL1_HRMODE_Msk   BIT(RTC_CTRL1_HRMODE_Pos)
#define RTC_CTRL1_DATEMODE_Pos (2UL)
#define RTC_CTRL1_DATEMODE_Msk BIT(RTC_CTRL1_DATEMODE_Pos)
#define RTC_CTRL1_SETMODE_Pos  (7UL)
#define RTC_CTRL1_SETMODE_Msk  BIT(RTC_CTRL1_SETMODE_Pos)
/* WEEK */
#define RTC_WEEK_NUM_Pos       (0UL)
#define RTC_WEEK_NUM_Msk       GENMASK(7, 0)

#endif /* ZEPHYR_SOC_REALTEK_RTS5912_REG_RTC_H */
