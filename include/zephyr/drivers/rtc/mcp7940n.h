/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_

#include <zephyr/sys/timeutil.h>
#include <time.h>

struct mcp7940n_rtc_sec {
	uint8_t sec_one : 4;
	uint8_t sec_ten : 3;
	uint8_t start_osc : 1;
} __packed;

struct mcp7940n_rtc_min {
	uint8_t min_one : 4;
	uint8_t min_ten : 3;
	uint8_t nimp : 1;
} __packed;

struct mcp7940n_rtc_hours {
	uint8_t hr_one : 4;
	uint8_t hr_ten : 2;
	uint8_t twelve_hr : 1;
	uint8_t nimp : 1;
} __packed;

struct mcp7940n_rtc_weekday {
	uint8_t weekday : 3;
	uint8_t vbaten : 1;
	uint8_t pwrfail : 1;
	uint8_t oscrun : 1;
	uint8_t nimp : 2;
} __packed;

struct mcp7940n_rtc_date {
	uint8_t date_one : 4;
	uint8_t date_ten : 2;
	uint8_t nimp : 2;
} __packed;

struct mcp7940n_rtc_month {
	uint8_t month_one : 4;
	uint8_t month_ten : 1;
	uint8_t lpyr : 1;
	uint8_t nimp : 2;
} __packed;

struct mcp7940n_rtc_year {
	uint8_t year_one : 4;
	uint8_t year_ten : 4;
} __packed;

struct mcp7940n_rtc_control {
	uint8_t sqwfs : 2;
	uint8_t crs_trim : 1;
	uint8_t ext_osc : 1;
	uint8_t alm0_en : 1;
	uint8_t alm1_en : 1;
	uint8_t sqw_en : 1;
	uint8_t out : 1;
} __packed;

struct mcp7940n_rtc_osctrim {
	uint8_t trim_val : 7;
	uint8_t sign : 1;
} __packed;

struct mcp7940n_alm_sec {
	uint8_t sec_one : 4;
	uint8_t sec_ten : 3;
	uint8_t nimp : 1;
} __packed;

struct mcp7940n_alm_min {
	uint8_t min_one : 4;
	uint8_t min_ten : 3;
	uint8_t nimp : 1;
} __packed;

struct mcp7940n_alm_hours {
	uint8_t hr_one : 4;
	uint8_t hr_ten : 2;
	uint8_t twelve_hr : 1;
	uint8_t nimp : 1;
} __packed;

struct mcp7940n_alm_weekday {
	uint8_t weekday : 3;
	uint8_t alm_if : 1;
	uint8_t alm_msk : 3;
	uint8_t alm_pol : 1;
} __packed;

struct mcp7940n_alm_date {
	uint8_t date_one : 4;
	uint8_t date_ten : 2;
	uint8_t nimp : 2;
} __packed;

struct mcp7940n_alm_month {
	uint8_t month_one : 4;
	uint8_t month_ten : 1;
	uint8_t nimp : 3;
} __packed;

struct mcp7940n_time_registers {
	struct mcp7940n_rtc_sec rtc_sec;
	struct mcp7940n_rtc_min rtc_min;
	struct mcp7940n_rtc_hours rtc_hours;
	struct mcp7940n_rtc_weekday rtc_weekday;
	struct mcp7940n_rtc_date rtc_date;
	struct mcp7940n_rtc_month rtc_month;
	struct mcp7940n_rtc_year rtc_year;
	struct mcp7940n_rtc_control rtc_control;
	struct mcp7940n_rtc_osctrim rtc_osctrim;
} __packed;

struct mcp7940n_alarm_registers {
	struct mcp7940n_alm_sec alm_sec;
	struct mcp7940n_alm_min alm_min;
	struct mcp7940n_alm_hours alm_hours;
	struct mcp7940n_alm_weekday alm_weekday;
	struct mcp7940n_alm_date alm_date;
	struct mcp7940n_alm_month alm_month;
} __packed;

enum mcp7940n_register {
	REG_RTC_SEC		= 0x0,
	REG_RTC_MIN		= 0x1,
	REG_RTC_HOUR		= 0x2,
	REG_RTC_WDAY		= 0x3,
	REG_RTC_DATE		= 0x4,
	REG_RTC_MONTH		= 0x5,
	REG_RTC_YEAR		= 0x6,
	REG_RTC_CONTROL		= 0x7,
	REG_RTC_OSCTRIM		= 0x8,
	/* 0x9 not implemented */
	REG_ALM0_SEC		= 0xA,
	REG_ALM0_MIN		= 0xB,
	REG_ALM0_HOUR		= 0xC,
	REG_ALM0_WDAY		= 0xD,
	REG_ALM0_DATE		= 0xE,
	REG_ALM0_MONTH		= 0xF,
	/* 0x10 not implemented */
	REG_ALM1_SEC		= 0x11,
	REG_ALM1_MIN		= 0x12,
	REG_ALM1_HOUR		= 0x13,
	REG_ALM1_WDAY		= 0x14,
	REG_ALM1_DATE		= 0x15,
	REG_ALM1_MONTH		= 0x16,
	/* 0x17 not implemented */
	REG_PWR_DWN_MIN		= 0x18,
	REG_PWR_DWN_HOUR	= 0x19,
	REG_PWR_DWN_DATE	= 0x1A,
	REG_PWR_DWN_MONTH	= 0x1B,
	REG_PWR_UP_MIN		= 0x1C,
	REG_PWR_UP_HOUR		= 0x1D,
	REG_PWR_UP_DATE		= 0x1E,
	REG_PWR_UP_MONTH	= 0x1F,
	SRAM_MIN		= 0x20,
	SRAM_MAX		= 0x5F,
	REG_INVAL		= 0x60,
};

/* Mutually exclusive alarm trigger settings */
enum mcp7940n_alarm_trigger {
	MCP7940N_ALARM_TRIGGER_SECONDS	= 0x0,
	MCP7940N_ALARM_TRIGGER_MINUTES	= 0x1,
	MCP7940N_ALARM_TRIGGER_HOURS	= 0x2,
	MCP7940N_ALARM_TRIGGER_WDAY	= 0x3,
	MCP7940N_ALARM_TRIGGER_DATE	= 0x4,
	/* TRIGGER_ALL matches seconds, minutes, hours, weekday, date and month */
	MCP7940N_ALARM_TRIGGER_ALL	= 0x7,
};

/** @brief Set the RTC to a given Unix time
 *
 * The RTC advances one tick per second with no access to sub-second
 * precision. This function will convert the given unix_time into seconds,
 * minutes, hours, day of the week, day of the month, month and year.
 * A Unix time of '0' means a timestamp of 00:00:00 UTC on Thursday 1st January
 * 1970.
 *
 * @param dev the MCP7940N device pointer.
 * @param unix_time Unix time to set the rtc to.
 *
 * @retval return 0 on success, or a negative error code from an I2C
 * transaction or invalid parameter.
 */
int mcp7940n_rtc_set_time(const struct device *dev, time_t unix_time);

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_ */
