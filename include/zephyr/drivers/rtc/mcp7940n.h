/*
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public header for MCP7940N RTC driver.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_

#include <zephyr/sys/timeutil.h>
#include <time.h>

/**
 * @brief RTC MCP7940N Driver-Specific API
 * @defgroup rtc_mcp7940n_interface RTC MCP7940N Interface
 * @ingroup rtc_interface
 * @{
 */

/** @brief RTC seconds register structure
 *
 * Contains the seconds value in BCD format with oscillator start bit.
 */
struct mcp7940n_rtc_sec {
	uint8_t sec_one: 4;   /**< Seconds ones digit (0-9) */
	uint8_t sec_ten: 3;   /**< Seconds tens digit (0-5) */
	uint8_t start_osc: 1; /**< Oscillator start bit */
} __packed;

/** @brief RTC minutes register structure
 *
 * Contains the minutes value in BCD format.
 */
struct mcp7940n_rtc_min {
	uint8_t min_one: 4; /**< Minutes ones digit (0-9) */
	uint8_t min_ten: 3; /**< Minutes tens digit (0-5) */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 1;
	/** @endcond */
} __packed;

/** @brief RTC hours register structure
 *
 * Contains the hours value in BCD format with 12/24 hour mode bit.
 */
struct mcp7940n_rtc_hours {
	uint8_t hr_one: 4;    /**< Hours ones digit (0-9) */
	uint8_t hr_ten: 2;    /**< Hours tens digit (0-2) */
	uint8_t twelve_hr: 1; /**< 12-hour mode bit */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 1;
	/** @endcond */
} __packed;

/** @brief RTC weekday register structure
 *
 * Contains the weekday value and status bits for battery backup and oscillator.
 */
struct mcp7940n_rtc_weekday {
	uint8_t weekday: 3; /**< Day of the week (1-7) */
	uint8_t vbaten: 1;  /**< Battery backup enable bit */
	uint8_t pwrfail: 1; /**< Power fail bit */
	uint8_t oscrun: 1;  /**< Oscillator running bit */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 2;
	/** @endcond */
} __packed;

/** @brief RTC date register structure
 *
 * Contains the date value in BCD format.
 */
struct mcp7940n_rtc_date {
	uint8_t date_one: 4; /**< Date ones digit (0-9) */
	uint8_t date_ten: 2; /**< Date tens digit (0-3) */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 2;
	/** @endcond */
} __packed;

/** @brief RTC month register structure
 *
 * Contains the month value in BCD format with leap year bit.
 */
struct mcp7940n_rtc_month {
	uint8_t month_one: 4; /**< Month ones digit (0-9) */
	uint8_t month_ten: 1; /**< Month tens digit (0-1) */
	uint8_t lpyr: 1;      /**< Leap year bit */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 2;
	/** @endcond */
} __packed;

/** @brief RTC year register structure
 *
 * Contains the year value in BCD format.
 */
struct mcp7940n_rtc_year {
	uint8_t year_one: 4; /**< Year ones digit (0-9) */
	uint8_t year_ten: 4; /**< Year tens digit (0-9) */
} __packed;

/** @brief RTC control register structure
 *
 * Contains control bits for square wave output, alarms, and oscillator settings.
 */
struct mcp7940n_rtc_control {
	uint8_t sqwfs: 2;    /**< Square wave frequency select */
	uint8_t crs_trim: 1; /**< Coarse trim bit */
	uint8_t ext_osc: 1;  /**< External oscillator bit */
	uint8_t alm0_en: 1;  /**< Alarm 0 enable bit */
	uint8_t alm1_en: 1;  /**< Alarm 1 enable bit */
	uint8_t sqw_en: 1;   /**< Square wave enable bit */
	uint8_t out: 1;      /**< Output control bit */
} __packed;

/** @brief RTC oscillator trim register structure
 *
 * Contains oscillator trim value and sign bit for frequency adjustment.
 */
struct mcp7940n_rtc_osctrim {
	uint8_t trim_val: 7; /**< Trim value (0-127) */
	uint8_t sign: 1;     /**< Trim sign bit (0=positive, 1=negative) */
} __packed;

/** @brief Alarm seconds register structure
 *
 * Contains the alarm seconds value in BCD format.
 */
struct mcp7940n_alm_sec {
	uint8_t sec_one: 4; /**< Seconds ones digit (0-9) */
	uint8_t sec_ten: 3; /**< Seconds tens digit (0-5) */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 1;
	/** @endcond */
} __packed;

/** @brief Alarm minutes register structure
 *
 * Contains the alarm minutes value in BCD format.
 */
struct mcp7940n_alm_min {
	uint8_t min_one: 4; /**< Minutes ones digit (0-9) */
	uint8_t min_ten: 3; /**< Minutes tens digit (0-5) */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 1;
	/** @endcond */
} __packed;

/** @brief Alarm hours register structure
 *
 * Contains the alarm hours value in BCD format with 12/24 hour mode bit.
 */
struct mcp7940n_alm_hours {
	uint8_t hr_one: 4;    /**< Hours ones digit (0-9) */
	uint8_t hr_ten: 2;    /**< Hours tens digit (0-2) */
	uint8_t twelve_hr: 1; /**< 12-hour mode bit */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 1;
	/** @endcond */
} __packed;

/** @brief Alarm weekday register structure
 *
 * Contains the alarm weekday value and alarm configuration bits.
 */
struct mcp7940n_alm_weekday {
	uint8_t weekday: 3; /**< Day of the week (1-7) */
	uint8_t alm_if: 1;  /**< Alarm interrupt flag */
	uint8_t alm_msk: 3; /**< Alarm mask bits */
	uint8_t alm_pol: 1; /**< Alarm polarity bit */
} __packed;

/** @brief Alarm date register structure
 *
 * Contains the alarm date value in BCD format.
 */
struct mcp7940n_alm_date {
	uint8_t date_one: 4; /**< Date ones digit (0-9) */
	uint8_t date_ten: 2; /**< Date tens digit (0-3) */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 2;
	/** @endcond */
} __packed;

/** @brief Alarm month register structure
 *
 * Contains the alarm month value in BCD format.
 */
struct mcp7940n_alm_month {
	uint8_t month_one: 4; /**< Month ones digit (0-9) */
	uint8_t month_ten: 1; /**< Month tens digit (0-1) */
	/** @cond INTERNAL_HIDDEN */
	uint8_t nimp: 3;
	/** @endcond */
} __packed;

/** @brief Complete RTC time registers structure
 *
 * Contains all time-related registers for the MCP7940N RTC.
 */
struct mcp7940n_time_registers {
	struct mcp7940n_rtc_sec rtc_sec;         /**< Seconds register */
	struct mcp7940n_rtc_min rtc_min;         /**< Minutes register */
	struct mcp7940n_rtc_hours rtc_hours;     /**< Hours register */
	struct mcp7940n_rtc_weekday rtc_weekday; /**< Weekday register */
	struct mcp7940n_rtc_date rtc_date;       /**< Date register */
	struct mcp7940n_rtc_month rtc_month;     /**< Month register */
	struct mcp7940n_rtc_year rtc_year;       /**< Year register */
	struct mcp7940n_rtc_control rtc_control; /**< Control register */
	struct mcp7940n_rtc_osctrim rtc_osctrim; /**< Oscillator trim register */
} __packed;

/** @brief Complete alarm registers structure
 *
 * Contains all alarm-related registers for the MCP7940N RTC.
 */
struct mcp7940n_alarm_registers {
	struct mcp7940n_alm_sec alm_sec;         /**< Alarm seconds register */
	struct mcp7940n_alm_min alm_min;         /**< Alarm minutes register */
	struct mcp7940n_alm_hours alm_hours;     /**< Alarm hours register */
	struct mcp7940n_alm_weekday alm_weekday; /**< Alarm weekday register */
	struct mcp7940n_alm_date alm_date;       /**< Alarm date register */
	struct mcp7940n_alm_month alm_month;     /**< Alarm month register */
} __packed;

/** @brief MCP7940N register addresses
 *
 * Contains the register addresses for the MCP7940N RTC.
 */
enum mcp7940n_register {
	REG_RTC_SEC = 0x0,     /**< Time keeping seconds value register */
	REG_RTC_MIN = 0x1,     /**< Time keeping minutes value register */
	REG_RTC_HOUR = 0x2,    /**< Time keeping hours value register */
	REG_RTC_WDAY = 0x3,    /**< Time keeping weekday value register */
	REG_RTC_DATE = 0x4,    /**< Time keeping date value register */
	REG_RTC_MONTH = 0x5,   /**< Time keeping month value register */
	REG_RTC_YEAR = 0x6,    /**< Time keeping year value register */
	REG_RTC_CONTROL = 0x7, /**< Time keeping control register */
	REG_RTC_OSCTRIM = 0x8, /**< Time keeping oscillator digital trim register */
	/* 0x9 not implemented */
	REG_ALM0_SEC = 0xA,   /**< Alarm 0 seconds value register */
	REG_ALM0_MIN = 0xB,   /**< Alarm 0 minutes value register */
	REG_ALM0_HOUR = 0xC,  /**< Alarm 0 hours value register */
	REG_ALM0_WDAY = 0xD,  /**< Alarm 0 weekday value register */
	REG_ALM0_DATE = 0xE,  /**< Alarm 0 date value register */
	REG_ALM0_MONTH = 0xF, /**< Alarm 0 month value register */
	/* 0x10 not implemented */
	REG_ALM1_SEC = 0x11,   /**< Alarm 1 seconds value register */
	REG_ALM1_MIN = 0x12,   /**< Alarm 1 minutes value register */
	REG_ALM1_HOUR = 0x13,  /**< Alarm 1 hours value register */
	REG_ALM1_WDAY = 0x14,  /**< Alarm 1 weekday value register */
	REG_ALM1_DATE = 0x15,  /**< Alarm 1 date value register */
	REG_ALM1_MONTH = 0x16, /**< Alarm 1 month value register */
	/* 0x17 not implemented */
	REG_PWR_DWN_MIN = 0x18,   /**< Power down timestamp minutes value register */
	REG_PWR_DWN_HOUR = 0x19,  /**< Power down timestamp hours value register */
	REG_PWR_DWN_DATE = 0x1A,  /**< Power down timestamp date value register */
	REG_PWR_DWN_MONTH = 0x1B, /**< Power down timestamp month value register */
	REG_PWR_UP_MIN = 0x1C,    /**< Power up timestamp minutes value register */
	REG_PWR_UP_HOUR = 0x1D,   /**< Power up timestamp hours value register */
	REG_PWR_UP_DATE = 0x1E,   /**< Power up timestamp date value register */
	REG_PWR_UP_MONTH = 0x1F,  /**< Power up timestamp month value register */
	SRAM_MIN = 0x20,          /**< SRAM first register */
	SRAM_MAX = 0x5F,          /**< SRAM last register */
	REG_INVAL = 0x60,         /**< Invalid register */
};

/** @brief MCP7940N alarm trigger settings
 *
 * Mutually exclusive alarm trigger settings.
 */
enum mcp7940n_alarm_trigger {
	MCP7940N_ALARM_TRIGGER_SECONDS = 0x0, /**< Alarm asserts on seconds */
	MCP7940N_ALARM_TRIGGER_MINUTES = 0x1, /**< Alarm asserts on minutes */
	MCP7940N_ALARM_TRIGGER_HOURS = 0x2,   /**< Alarm asserts on hours */
	MCP7940N_ALARM_TRIGGER_WDAY = 0x3,    /**< Alarm asserts on weekday */
	MCP7940N_ALARM_TRIGGER_DATE = 0x4,    /**< Alarm asserts on date */
	/* TRIGGER_ALL matches seconds, minutes, hours, weekday, date and month */
	MCP7940N_ALARM_TRIGGER_ALL = 0x7, /**< Alarm asserts on all */
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
 * @retval 0 success
 * @retval -EINVAL invalid parameter
 * @retval -errno I2C transaction error code
 */
int mcp7940n_rtc_set_time(const struct device *dev, time_t unix_time);

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_MCP7940N_H_ */
