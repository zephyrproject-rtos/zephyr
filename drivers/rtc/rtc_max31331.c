/*
 * Copyright (c) 2025 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/rtc.h>
#include <errno.h>
#include "rtc_max31331.h"
#include "rtc_utils.h"
#ifdef CONFIG_RTC_MAX31331_TIMESTAMPING
#include <zephyr/drivers/rtc/rtc_max31331.h>
#endif

#define DT_DRV_COMPAT adi_max31331

LOG_MODULE_REGISTER(rtc_max31331, CONFIG_RTC_LOG_LEVEL);

#ifdef CONFIG_RTC_ALARM
struct rtc_max31331_alarm {
	rtc_alarm_callback callback;
	void *user_data;
};
#endif
struct rtc_max31331_data {
#ifdef CONFIG_RTC_ALARM
	struct rtc_max31331_alarm alarms[ALARM_COUNT];
#endif

#ifdef CONFIG_RTC_MAX31331_TIMESTAMPING
	struct rtc_time timestamp_buffer[4];
	rtc_max31331_timestamp_callback ts_callback;
	void *ts_user_data;
#endif
	struct gpio_callback int_callback;
#if defined(CONFIG_RTC_MAX31331_INTERRUPT_GLOBAL_THREAD)
	struct k_work work;
#elif defined(CONFIG_RTC_MAX31331_INTERRUPT_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_RTC_MAX31331_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#endif
	const struct device *dev;
};

struct rtc_max31331_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec inta_gpios;
	bool ts_enable;
	bool ts_vbat_enable;
	bool ts_din;
	bool ts_overwrite;
	bool ts_power_supply_switch;
	bool din_polarity;
	bool din_en_io;
};

/**
 * @brief Generic register access function for MAX31331
 *
 * @param dev Device Descriptor
 * @param addr_reg Register Address
 * @param data Data buffer pointer
 * @param read Read (true) or Write (false) operation
 * @param length Number of bytes to read/write
 * @return int 0 on success, negative error code on failure
 */
static int max31331_reg_access(const struct device *dev, uint8_t addr_reg, uint8_t *data, bool read,
			       uint8_t length)
{

	const struct rtc_max31331_config *config = dev->config;

	if (read) {
		return i2c_burst_read_dt(&config->i2c, addr_reg, data, length);
	} else {
		return i2c_burst_write_dt(&config->i2c, addr_reg, data, length);
	}
}

/**
 * @brief Read register(s) from MAX31331
 *
 * @param dev Device Descriptor
 * @param reg_addr Register Address
 * @param val Data buffer pointer
 * @param length Number of bytes to read
 * @return int 0 on success, negative error code on failure
 */
static int max31331_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *val,
			     uint8_t length)
{
	return max31331_reg_access(dev, reg_addr, val, true, length);
}

/**
 * @brief Single Byte write to MAX31331 register
 *
 * @param dev Device Descriptor
 * @param reg_addr Register Address
 * @param val Value to write
 * @return int 0 on success, negative error code on failure
 */
static int max31331_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t val)
{
	return max31331_reg_access(dev, reg_addr, &val, false, 1);
}

/**
 * @brief Multiple Byte write to MAX31331 register
 *
 * @param dev Device Descriptor
 * @param reg_addr Register Address
 * @param val Data buffer pointer
 * @param length Number of bytes to write
 * @return int 0 on success, negative error code on failure
 */
static int max31331_reg_write_multiple(const struct device *dev, uint8_t reg_addr, uint8_t *val,
				       uint8_t length)
{
	if (length < 2) {
		return -EINVAL;
	}
	return max31331_reg_access(dev, reg_addr, val, false, length);
}

/**
 * @brief Update specific bits in a MAX31331 register
 *
 * @param dev Device Descriptor
 * @param reg_addr Register Address
 * @param mask Bit mask to update
 * @param val New value for the specified bits
 * @return int 0 on success, negative error code on failure
 */
static int max31331_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			       uint8_t val)
{
	uint8_t reg_val = 0;
	int ret;

	ret = max31331_reg_read(dev, reg_addr, &reg_val, 1);
	if (ret < 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, val);

	return max31331_reg_write(dev, reg_addr, reg_val);
}

/**
 * @brief Get the current time from MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param timeptr Pointer to rtc_time structure to store the retrieved time
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	int ret;
	uint8_t raw_time[7];

	ret = max31331_reg_read(dev, MAX31331_SECONDS, raw_time, ARRAY_SIZE(raw_time));
	if (ret) {
		LOG_ERR("Unable to get time. Err: %i", ret);
		return ret;
	}

	timeptr->tm_sec = bcd2bin(raw_time[0] & SECONDS_FIELD_MASK);
	timeptr->tm_min = bcd2bin(raw_time[1] & MINUTES_FIELD_MASK);
	timeptr->tm_hour = bcd2bin(raw_time[2] & HOURS_FIELD_MASK);
	timeptr->tm_wday = bcd2bin(raw_time[3] & DAY_FIELD_MASK) + MAX31331_DAY_OFFSET;
	timeptr->tm_mday = bcd2bin(raw_time[4] & DATE_FIELD_MASK);
	timeptr->tm_mon = bcd2bin(raw_time[5] & MONTH_FIELD_MASK) - 1;
	if (raw_time[5] & CENTURY_MASK) {
		timeptr->tm_year = bcd2bin(raw_time[6] & YEAR_FIELD_MASK) + MAX31331_YEAR_2100;
	} else {
		timeptr->tm_year = bcd2bin(raw_time[6] & YEAR_FIELD_MASK);
	}

	LOG_DBG("Get time: year: %d, month: %d, month day: %d, week day: %d, hour: %d, "
		"minute: %d, second: %d",
		timeptr->tm_year + 1900, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

/**
 * @brief Set the current time on MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param timeptr Pointer to rtc_time structure containing the time to set
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	int ret;
	uint8_t raw_time[7];

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, MAX31331_RTC_TIME_MASK)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	raw_time[0] = bin2bcd(timeptr->tm_sec) & SECONDS_FIELD_MASK;
	raw_time[1] = bin2bcd(timeptr->tm_min) & MINUTES_FIELD_MASK;
	raw_time[2] = bin2bcd(timeptr->tm_hour) & HOURS_FIELD_MASK;
	raw_time[3] = bin2bcd(timeptr->tm_wday - MAX31331_DAY_OFFSET) & DAY_FIELD_MASK;
	raw_time[4] = bin2bcd(timeptr->tm_mday) & DATE_FIELD_MASK;
	raw_time[5] = bin2bcd(timeptr->tm_mon + 1) & MONTH_FIELD_MASK;

	if (timeptr->tm_year >= MAX31331_YEAR_2100) {
		raw_time[5] |= BIT(7);
		raw_time[6] = bin2bcd(timeptr->tm_year % 100) & YEAR_FIELD_MASK;
	} else {
		raw_time[5] &= ~BIT(7);
		raw_time[6] = bin2bcd(timeptr->tm_year % 100) & YEAR_FIELD_MASK;
	}

	ret = max31331_reg_write_multiple(dev, MAX31331_SECONDS, raw_time, ARRAY_SIZE(raw_time));
	if (ret) {
		LOG_ERR("Error when setting time: %i", ret);
		return ret;
	}
	return 0;
}

#ifdef CONFIG_RTC_ALARM

static inline int validate_alarm_1_time_mask(uint16_t mask)
{
	if (mask & RTC_ALARM_TIME_MASK_YEARDAY) {
		LOG_ERR("Alarm 1 does not support yearday field");
		return -EINVAL;
	}

	if (mask == 0) {
		LOG_ERR("Alarm 1 time mask not set");
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Helper to Write time for Alarm 1
 *
 * @param dev Device Pointer
 * @param mask RTC Alarm Time Mask
 * @param timeptr Pointer to rtc_time structure
 * @return int 0 on success, negative error code on failure
 */
static int set_alarm_time_1(const struct device *dev, uint16_t mask, const struct rtc_time *timeptr)
{
	int ret;
	uint8_t raw_time[6];

	ret = validate_alarm_1_time_mask(mask);
	if (ret) {
		LOG_ERR("Invalid alarm 1 time mask: %i", ret);
		return ret;
	}

	raw_time[0] = (mask & RTC_ALARM_TIME_MASK_SECOND)
			      ? (bin2bcd(timeptr->tm_sec) & ALARM_1_SECONDS_FIELD_MASK) &
					~ALARM_1_SECONDS_ENABLE_MASK
			      : (bin2bcd(timeptr->tm_sec) & ALARM_1_SECONDS_FIELD_MASK) |
					ALARM_1_SECONDS_ENABLE_MASK;
	raw_time[1] = (mask & RTC_ALARM_TIME_MASK_MINUTE)
			      ? (bin2bcd(timeptr->tm_min) & ALARM_1_MINUTES_FIELD_MASK) &
					~ALARM_1_MINUTES_ENABLE_MASK
			      : (bin2bcd(timeptr->tm_min) & ALARM_1_MINUTES_FIELD_MASK) |
					ALARM_1_MINUTES_ENABLE_MASK;
	raw_time[2] = (mask & RTC_ALARM_TIME_MASK_HOUR)
			      ? (bin2bcd(timeptr->tm_hour) & ALARM_1_HOURS_FIELD_MASK) &
					~ALARM_1_HOURS_ENABLE_MASK
			      : (bin2bcd(timeptr->tm_hour) & ALARM_1_HOURS_FIELD_MASK) |
					ALARM_1_HOURS_ENABLE_MASK;

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
	    (timeptr->tm_wday >= 0 && timeptr->tm_wday <= 6)) {

		raw_time[3] =
			((bin2bcd(timeptr->tm_wday + 1) & ALARM_1_DAY_DATE_MASK) |
			 ALARM_1_DAY_DATE_OP_MASK |
			 ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) ? 0 : ALARM_1_DAY_DATE_ENABLE_MASK));

	} else if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
		   (timeptr->tm_mday >= 1 && timeptr->tm_mday <= 31)) {

		raw_time[3] =
			(((bin2bcd(timeptr->tm_mday) & ALARM_1_DAY_DATE_FIELD_MASK) &
			  ~ALARM_1_DAY_DATE_OP_MASK) |
			 ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) ? 0
								: ALARM_1_DAY_DATE_ENABLE_MASK));

	} else {
		/* Disable day/date alarm if neither field is valid */
		raw_time[3] = 0x80;
	}
	raw_time[4] = (mask & RTC_ALARM_TIME_MASK_MONTH)
			      ? ((bin2bcd(timeptr->tm_mon + 1) & ALARM_1_MONTH_FIELD_MASK) &
				 ~ALARM_1_MONTH_ENABLE_MASK)
			      : ((bin2bcd(timeptr->tm_mon + 1) & ALARM_1_MONTH_FIELD_MASK) |
				 ALARM_1_MONTH_ENABLE_MASK);

	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		raw_time[5] = (bin2bcd(timeptr->tm_year % 100) & ALARM_1_YEAR_FIELD_MASK);
	} else {
		raw_time[4] |= ALARM_1_YEAR_ENABLE_MASK;
		raw_time[5] = (bin2bcd(timeptr->tm_year % 100) & ALARM_1_YEAR_FIELD_MASK);
	}
	ret = max31331_reg_write_multiple(dev, MAX31331_ALARM_1_SECONDS, raw_time,
					  ARRAY_SIZE(raw_time));
	if (ret) {
		LOG_ERR("Error when setting alarm: %i", ret);
		return ret;
	}
	return 0;
}

/**
 * @brief Helper to validate Alarm 2 time mask
 *
 * @param mask RTC Alarm Time Mask
 * @return int 0 on success, negative error code on failure
 */
static inline int validate_alarm_2_time_mask(uint16_t mask)
{
	if (mask == 0) {
		LOG_ERR("Alarm 2 time mask not set");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		LOG_ERR("Alarm 2 does not support seconds field");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		LOG_ERR("Alarm 2 does not support year field");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		LOG_ERR("Alarm 2 does not support month field");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Helper to Write time for Alarm 2
 *
 * @param dev Device Pointer
 * @param mask RTC Alarm Time Mask
 * @param timeptr Pointer to rtc_time structure
 * @return int 0 on success, negative error code on failure
 */
static int set_alarm_time_2(const struct device *dev, uint16_t mask, const struct rtc_time *timeptr)
{
	int ret;
	uint8_t raw_time[3];

	ret = validate_alarm_2_time_mask(mask);
	if (ret) {
		LOG_ERR("Invalid alarm 2 time mask: %i", ret);
		return ret;
	}

	raw_time[0] = (mask & RTC_ALARM_TIME_MASK_MINUTE)
			      ? (bin2bcd(timeptr->tm_min) & ALARM_2_MINUTES_FIELD_MASK) &
					~ALARM_2_MINUTES_ENABLE_MASK
			      : (bin2bcd(timeptr->tm_min) & ALARM_2_MINUTES_FIELD_MASK) |
					ALARM_2_MINUTES_ENABLE_MASK;
	raw_time[1] = (mask & RTC_ALARM_TIME_MASK_HOUR)
			      ? (bin2bcd(timeptr->tm_hour) & ALARM_2_HOURS_FIELD_MASK) &
					~ALARM_2_HOURS_ENABLE_MASK
			      : (bin2bcd(timeptr->tm_hour) & ALARM_2_HOURS_FIELD_MASK) |
					ALARM_2_HOURS_ENABLE_MASK;

	if (timeptr->tm_wday >= 0 && timeptr->tm_wday <= 6) {
		/* Alarm based on weekday */
		if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
			raw_time[2] = ((bin2bcd(timeptr->tm_wday + 1) & ALARM_2_DAY_DATE_MASK) |
				       ALARM_2_DAY_DATE_OP_MASK) &
				      ~ALARM_2_DAY_DATE_ENABLE_MASK;
		} else {
			raw_time[2] = ((bin2bcd(timeptr->tm_wday + 1) & ALARM_2_DAY_DATE_MASK) |
				       ALARM_2_DAY_DATE_OP_MASK | ALARM_2_DAY_DATE_ENABLE_MASK);
		}
	} else if (timeptr->tm_mday >= 1 && timeptr->tm_mday <= 31) {
		/* Alarm based on day of month */
		if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
			raw_time[2] = ((bin2bcd(timeptr->tm_mday) &
					(ALARM_2_DAY_DATE_FIELD_MASK & ~ALARM_2_DAY_DATE_OP_MASK)) &
				       ~ALARM_2_DAY_DATE_ENABLE_MASK);
		} else {
			raw_time[2] = ((bin2bcd(timeptr->tm_mday) &
					(ALARM_2_DAY_DATE_FIELD_MASK & ~ALARM_2_DAY_DATE_OP_MASK)) |
				       ALARM_2_DAY_DATE_ENABLE_MASK);
		}
	} else {
		raw_time[2] = 0x80;
	}

	ret = max31331_reg_write_multiple(dev, MAX31331_ALARM_2_MINUTES, raw_time,
					  ARRAY_SIZE(raw_time));
	if (ret) {
		LOG_ERR("Error when setting alarm: %i", ret);
		return ret;
	}
	return 0;
}

/**
 * @brief Enable or disable alarm interrupt on MAX31331 RTC
 *
 * @param dev Device Pointer
 * @param id Alarm ID
 * @param mask RTC Alarm Time Mask
 * @return int 0 on success, negative error code on failure
 */
static int enable_alarm_interrupt(const struct device *dev, uint16_t id, uint16_t mask)
{
	int ret;
	bool enable;

	if (mask != 0) {
		enable = true;
	} else {
		enable = false;
	}

	if (id == 1) {
		ret = max31331_reg_update(dev, MAX31331_INTERRUPT_ENABLE,
					  ALARM_1_INTERRUPT_ENABLE_MASK, enable);
		if (ret) {
			LOG_ERR("Error setting alarm interrupt: %i", ret);
			return ret;
		}
	} else if (id == 2) {
		ret = max31331_reg_update(dev, MAX31331_INTERRUPT_ENABLE,
					  ALARM_2_INTERRUPT_ENABLE_MASK, enable);
		if (ret) {
			LOG_ERR("Error setting alarm interrupt: %i", ret);
			return ret;
		}
	} else {
		LOG_ERR("Invalid Alarm ID: %d", id);
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Set alarm time helper for MAX31331 RTC
 *
 * @param dev Device Pointer
 * @param mask RTC Alarm Time Mask
 * @param timeptr Pointer to rtc_time structure
 * @param id Alarm ID
 * @return int 0 on success, negative error code on failure
 */
static int set_alarm_time(const struct device *dev, uint16_t mask, const struct rtc_time *timeptr,
			  uint16_t id)
{
	int ret;

	if (id == 1) {
		ret = set_alarm_time_1(dev, mask, timeptr);
		if (ret) {
			return ret;
		}
	} else if (id == 2) {
		ret = set_alarm_time_2(dev, mask, timeptr);
		if (ret) {
			return ret;
		}

	} else {
		LOG_ERR("Invalid Alarm ID: %d", id);
		return -EINVAL;
	}
	return 0;
}
static int validate_mask_month_week_day(uint16_t mask)
{
	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) && (mask & RTC_ALARM_TIME_MASK_WEEKDAY)) {
		LOG_ERR("Both day and date are set. Not Supported");
		return -EINVAL;
	}
	return 0;
}
/**
 * @brief Set alarm time on MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param id Alarm ID
 * @param mask Alarm time fields mask
 * @param timeptr Pointer to rtc_time structure containing the alarm time to set
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				       const struct rtc_time *timeptr)
{
	int ret = 0;

	ret = validate_mask_month_week_day(mask);
	if (ret) {
		return ret;
	}

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("invalid alarm time");
		return -EINVAL;
	}
	ret = set_alarm_time(dev, mask, timeptr, id);
	if (ret) {
		LOG_ERR("Error when setting alarm time: %i", ret);
		return ret;
	}
	ret = enable_alarm_interrupt(dev, id, mask);
	if (ret) {
		LOG_ERR("Error when enabling alarm interrupt: %i", ret);
		return ret;
	}
	return 0;
}

static void process_mask_alarm_1(uint16_t *mask, const uint8_t *raw_time)
{
	*mask = 0;
	if (!(raw_time[0] & ALARM_1_SECONDS_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}
	if (!(raw_time[1] & ALARM_1_MINUTES_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (!(raw_time[2] & ALARM_1_HOURS_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (!(raw_time[3] & ALARM_1_DAY_DATE_ENABLE_MASK)) {
		if (raw_time[3] & ALARM_1_DAY_DATE_OP_MASK) {
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		} else {
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}
	}
	if (!(raw_time[4] & ALARM_1_MONTH_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_MONTH;
	}
	if (!(raw_time[4] & ALARM_1_YEAR_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_YEAR;
	}
}

static void process_mask_alarm_2(uint16_t *mask, const uint8_t *raw_time)
{
	*mask = 0;
	if (!(raw_time[0] & ALARM_2_MINUTES_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (!(raw_time[1] & ALARM_2_HOURS_ENABLE_MASK)) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (!(raw_time[2] & ALARM_2_DAY_DATE_ENABLE_MASK)) {
		if (raw_time[2] & ALARM_2_DAY_DATE_OP_MASK) {
			*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		} else {
			*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}
	}
}

/**
 * @brief Get alarm time from MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param id Alarm ID
 * @param mask Alarm time fields mask
 * @param timeptr Pointer to rtc_time structure to store the retrieved alarm time
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				       struct rtc_time *timeptr)
{
	int ret;
	uint8_t raw_time[6];

	switch (id) {
	case 1:
		ret = max31331_reg_read(dev, MAX31331_ALARM_1_SECONDS, raw_time,
					ARRAY_SIZE(raw_time));
		if (ret) {
			LOG_ERR("Error when getting alarm time: %i", ret);
			return ret;
		}
		*mask = 0;
		process_mask_alarm_1(mask, raw_time);
		return 0;
	case 2:
		ret = max31331_reg_read(dev, MAX31331_ALARM_2_MINUTES, raw_time, 3);
		if (ret) {
			LOG_ERR("Error when getting alarm time: %i", ret);
			return ret;
		}
		*mask = 0;
		process_mask_alarm_2(mask, raw_time);
		return 0;
	default:
		LOG_ERR("Invalid Alarm ID: %d", id);
		return -EINVAL;
	}
}

/**
 * @brief Set alarm callback for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param id Alarm ID
 * @param callback Alarm callback function
 * @param user_data User data pointer to be passed to the callback
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_alarm_set_callback(const struct device *dev, uint16_t id,
					   rtc_alarm_callback callback, void *user_data)
{
	struct rtc_max31331_data *data = dev->data;

	if (id <= 0 || id > 2) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}
	data->alarms[id - 1].callback = callback;
	data->alarms[id - 1].user_data = user_data;
	return 0;
}

/**
 * @brief Get supported alarm time fields for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param id Alarm ID
 * @param mask Pointer to store the supported alarm time fields mask
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						   uint16_t *mask)
{
	*mask = RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY |
		RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MINUTE;

	switch (id) {
	case 1:
		*mask |= RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MONTH |
			 RTC_ALARM_TIME_MASK_YEAR;
		break;
	case 2:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Check if an alarm is pending on MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param id Alarm ID
 * @return int 1 if alarm is pending, 0 if not, negative error code on failure
 */
static int rtc_max31331_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int ret;
	uint8_t int_status;

	if (id <= 0 || id > 2) {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}

	ret = max31331_reg_read(dev, MAX31331_STATUS_REG, &int_status, 1);
	if (ret) {
		LOG_ERR("Failed to read interrupt status");
		return ret;
	}

	if (id == 1) {
		return (int_status & ALARM_1_FLAG_MASK) ? 1 : 0;
	} else if (id == 2) {
		return (int_status & ALARM_2_FLAG_MASK) ? 1 : 0;
	}

	return 0;
}

/**
 * @brief Initialize alarm structures for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_init_alarms(const struct device *dev)
{
	struct rtc_max31331_data *data = dev->data;

	for (int i = 0; i < ALARM_COUNT; i++) {
		data->alarms[i].callback = NULL;
		data->alarms[i].user_data = NULL;
	}
	return 0;
}

/**
 * @brief Main callback function for handling MAX31331 RTC interrupts
 *
 * @param dev Device Descriptor
 */
static void rtc_max31331_main_cb(const struct device *dev)
{
	const struct rtc_max31331_config *config = dev->config;
	struct rtc_max31331_data *data = dev->data;

	int ret = 0;
	uint8_t int_status;

	/* Read Status and Clear Flags */
	ret = max31331_reg_read(dev, MAX31331_STATUS_REG, &int_status, 1);
	if (ret) {
		LOG_ERR("Failed to read interrupt status");
		return;
	}

	if ((int_status & ALARM_1_FLAG_MASK) && data->alarms[0].callback) {
		data->alarms[0].callback(dev, 1, data->alarms[0].user_data);
	}

	if ((int_status & ALARM_2_FLAG_MASK) && data->alarms[1].callback) {
		data->alarms[1].callback(dev, 2, data->alarms[1].user_data);
	}

#if defined(CONFIG_RTC_MAX31331_TIMESTAMPING)
	if ((int_status & (DIGITAL_INTERRUPT_MASK | VBATLOW_MASK)) && data->ts_callback) {
		data->ts_callback(dev, data->ts_user_data);
	}
#endif

	ret = gpio_pin_interrupt_configure_dt(&config->inta_gpios, GPIO_INT_EDGE_FALLING);
	if (ret) {
		LOG_ERR("Failed to enable INT GPIO interrupt");
		return;
	}
	__ASSERT(ret == 0, "Interrupt Configuration Failed");
}

/**
 * @brief GPIO callback function for MAX31331 RTC interrupt
 *
 * @param dev Device Descriptor
 * @param cb GPIO callback structure
 * @param pins Pin mask that triggered the interrupt
 */
static void rtc_max31331_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				       uint32_t pins)
{
	struct rtc_max31331_data *data = CONTAINER_OF(cb, struct rtc_max31331_data, int_callback);
	const struct rtc_max31331_config *config = data->dev->config;
	int ret = 0;

	ret = gpio_pin_interrupt_configure_dt(&config->inta_gpios, GPIO_INT_DISABLE);
	if (ret) {
		LOG_ERR("Failed to disable INT GPIO interrupt");
		return;
	}
#if defined(CONFIG_RTC_MAX31331_INTERRUPT_GLOBAL_THREAD)
	k_work_submit(&data->work);
#elif defined(CONFIG_RTC_MAX31331_INTERRUPT_OWN_THREAD)
	k_sem_give(&data->sem);
#endif
}

#if defined(CONFIG_RTC_MAX31331_INTERRUPT_OWN_THREAD)
/**
 * @brief Thread function for handling MAX31331 RTC interrupts
 *
 * @param p1 Pointer to rtc_max31331_data structure
 * @param p2 Unused
 * @param p3 Unused
 */
static void max31331_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	struct rtc_max31331_data *data = p1;
	const struct device *dev = data->dev;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		rtc_max31331_main_cb(dev);
	}
}
#elif defined(CONFIG_RTC_MAX31331_INTERRUPT_GLOBAL_THREAD)
/**
 * @brief Work callback function for handling MAX31331 RTC interrupts
 *
 * @param work Pointer to k_work structure
 */
static void max31331_work_cb(struct k_work *work)
{
	struct rtc_max31331_data *data = CONTAINER_OF(work, struct rtc_max31331_data, work);
	const struct device *dev = data->dev;

	rtc_max31331_main_cb(dev);
}
#endif
/**
 * @brief Initialize alarm functionality for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_alarm_init(const struct device *dev)
{
	const struct rtc_max31331_config *config = dev->config;
	struct rtc_max31331_data *data = dev->data;
	int ret = 0;

	if (!gpio_is_ready_dt(&config->inta_gpios)) {
		LOG_ERR("INT GPIO not ready");
		return -ENODEV;
	}

	ret = rtc_max31331_init_alarms(dev);
	if (ret) {
		LOG_ERR("Failed to initialize alarms");
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->inta_gpios, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Failed to configure INT GPIO");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->inta_gpios, GPIO_INT_EDGE_FALLING);
	if (ret) {
		LOG_ERR("Failed to configure INT GPIO interrupt");
		return ret;
	}

	gpio_init_callback(&data->int_callback, rtc_max31331_gpio_callback,
			   BIT(config->inta_gpios.pin));
	ret = gpio_add_callback(config->inta_gpios.port, &data->int_callback);
	if (ret) {
		LOG_ERR("Failed to add INT GPIO callback");
		return ret;
	}

	data->dev = dev;
#if defined(CONFIG_RTC_MAX31331_INTERRUPT_GLOBAL_THREAD)
	k_work_init(&data->work, max31331_work_cb);
#elif defined(CONFIG_RTC_MAX31331_INTERRUPT_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_RTC_MAX31331_THREAD_STACK_SIZE,
			(k_thread_entry_t)max31331_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_RTC_MAX31331_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#endif

	return 0;
}

#endif

#ifdef CONFIG_RTC_MAX31331_TIMESTAMPING

/**
 * @brief Initialize timestamp callback for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_timestamp_callback_init(const struct device *dev)
{
	struct rtc_max31331_data *data = dev->data;

	data->ts_callback = NULL;
	data->ts_user_data = NULL;
	return 0;
}

/**
 * @brief Initialize timestamping functionality for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_timestamping_init(const struct device *dev)
{
	const struct rtc_max31331_config *config = dev->config;
	struct rtc_max31331_data *data = dev->data;
	int ret = 0;

	memset(data->timestamp_buffer, 0, sizeof(data->timestamp_buffer));

	ret = max31331_reg_write(
		dev, MAX31331_TIMESTAMP_CONFIG,
		(config->ts_enable ? TS_ENABLE_MASK : 0) |
			(config->ts_vbat_enable ? TS_VBAT_LOW_EN_MASK : 0) |
			(config->ts_din ? TS_DIN_MASK : 0) |
			(config->ts_overwrite ? TS_OVERWRITE_MASK : 0) |
			(config->ts_power_supply_switch ? TS_POWER_SUPPLY_SWITCH_MASK : 0));

	if (ret) {
		LOG_ERR("Failed to configure timestamping");
		return ret;
	}

	ret = max31331_reg_update(dev, MAX31331_RTC_CONFIG1, EN_IOUTPUT_MASK, config->din_en_io);
	if (ret) {
		LOG_ERR("Failed to configure timestamping I/O");
		return ret;
	}

	ret = max31331_reg_update(dev, MAX31331_RTC_CONFIG1, DIGITAL_INPUT_POLARITY_MASK,
				  config->din_polarity);
	if (ret) {
		LOG_ERR("Failed to configure timestamping DIN polarity");
		return ret;
	}

	ret = rtc_max31331_timestamp_callback_init(dev);
	if (ret) {
		LOG_ERR("Failed to initialize timestamp callback");
		return ret;
	}

	return 0;
}

/**
 * @brief Set timestamp callback for MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param cb Timestamp callback function
 * @param user_data User data pointer to be passed to the callback
 * @return int 0 on success, negative error code on failure
 */
int rtc_max31331_set_timestamp_callback(const struct device *dev,
					rtc_max31331_timestamp_callback cb, void *user_data)
{
	int ret;
	struct rtc_max31331_data *data = dev->data;

	data->ts_callback = cb;
	data->ts_user_data = user_data;

	ret = max31331_reg_update(dev, MAX31331_INTERRUPT_ENABLE, DIGITAL_INTERRUPT_ENABLE_MASK,
				  (cb != NULL) ? 1 : 0);
	if (ret) {
		LOG_ERR("Failed to %s timestamp interrupt", (cb != NULL) ? "enable" : "disable");
		return ret;
	}
	return 0;
}

/**
 * @brief Get timestamp from MAX31331 RTC
 *
 * @param dev Device Descriptor
 * @param timeptr Pointer to rtc_time structure to store the retrieved timestamp
 * @param index Timestamp index (0-3)
 * @param flags Pointer to store timestamp flags
 * @return int 0 on success, negative error code on failure
 */
int rtc_max31331_get_timestamps(const struct device *dev, struct rtc_time *timeptr, uint8_t index,
				uint8_t *flags)
{
	int ret = 0;
	uint8_t start_addr = 0;
	uint8_t reg_buf[7];

	switch (index) {
	case 0:
		start_addr = MAX31331_TS0_SEC;
		break;
	case 1:
		start_addr = MAX31331_TS1_SEC;
		break;
	case 2:
		start_addr = MAX31331_TS2_SEC;
		break;
	case 3:
		start_addr = MAX31331_TS3_SEC;
		break;
	default:
		LOG_ERR("Invalid timestamp index");
		return -EINVAL;
	}

	ret = max31331_reg_read(dev, start_addr, reg_buf, ARRAY_SIZE(reg_buf));
	if (ret) {
		LOG_ERR("Failed to read timestamp count");
		return ret;
	}

	timeptr->tm_sec = bcd2bin(reg_buf[0] & SECONDS_FIELD_MASK);
	timeptr->tm_min = bcd2bin(reg_buf[1] & MINUTES_FIELD_MASK);
	timeptr->tm_hour = bcd2bin(reg_buf[2] & HOURS_FIELD_MASK);
	timeptr->tm_mday = bcd2bin(reg_buf[3] & DATE_FIELD_MASK);
	timeptr->tm_mon = bcd2bin(reg_buf[4] & MONTH_FIELD_MASK) - 1;
	if (reg_buf[4] & CENTURY_MASK) {
		timeptr->tm_year = bcd2bin(reg_buf[5] & YEAR_FIELD_MASK) + MAX31331_YEAR_2100;
	} else {
		timeptr->tm_year = bcd2bin(reg_buf[5] & YEAR_FIELD_MASK);
	}

	*flags = reg_buf[6];

	return 0;
}

#endif

/**
 * @brief Initialize MAX31331 RTC device
 *
 * @param dev Device Descriptor
 * @return int 0 on success, negative error code on failure
 */
static int rtc_max31331_init(const struct device *dev)
{
	const struct rtc_max31331_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		return -ENODEV;
	}

	ret = max31331_reg_write(dev, MAX31331_RTC_RESET, SW_RESET_MASK);
	if (ret) {
		LOG_ERR("Failed to reset the device");
		return ret;
	}

	ret = max31331_reg_write(dev, MAX31331_RTC_RESET, 0);
	if (ret) {
		LOG_ERR("Failed to reset the device");
		return ret;
	}

	ret = max31331_reg_update(dev, MAX31331_RTC_CONFIG2, CLKOUT_ENABLE_MASK, 1);

	if (ret) {
		LOG_ERR("Failed to enable CLKOUT");
		return ret;
	}

	ret = max31331_reg_update(dev, MAX31331_TIMESTAMP_CONFIG, TS_REG_RESET_MASK, 1);
	if (ret) {
		LOG_ERR("Failed to reset timestamp registers");
		return ret;
	}

#if defined(CONFIG_RTC_MAX31331_TIMESTAMPING)
	ret = rtc_max31331_timestamping_init(dev);
	if (ret) {
		LOG_ERR("Failed to initialize timestamping");
		return ret;
	}
#endif

#ifdef CONFIG_RTC_ALARM
	if (config->inta_gpios.port) {
		ret = rtc_max31331_alarm_init(dev);
		if (ret) {
			LOG_ERR("Failed to initialize alarms");
			return ret;
		}
	}
#endif

	ret = max31331_reg_update(dev, MAX31331_RTC_CONFIG1, ENABLE_OSCILLATOR_MASK, 1);
	if (ret) {
		LOG_ERR("Failed to enable oscillator");
		return ret;
	}

	return 0;
}

static DEVICE_API(rtc, rtc_max31331) = {
	.set_time = rtc_max31331_set_time,
	.get_time = rtc_max31331_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_set_time = rtc_max31331_alarm_set_time,
	.alarm_get_time = rtc_max31331_alarm_get_time,
	.alarm_is_pending = rtc_max31331_alarm_is_pending,
	.alarm_set_callback = rtc_max31331_alarm_set_callback,
	.alarm_get_supported_fields = rtc_max31331_alarm_get_supported_fields,
#endif
};

#define RTC_MAX31331_CONFIG(inst)                                                                  \
	.ts_enable = DT_INST_PROP(inst, ts_enable),                                                \
	.ts_vbat_enable = DT_INST_PROP(inst, ts_vbat_enable),                                      \
	.ts_din = DT_INST_PROP(inst, ts_din), .ts_overwrite = DT_INST_PROP(inst, ts_overwrite),    \
	.ts_power_supply_switch = DT_INST_PROP(inst, ts_power_supply_switch),                      \
	.din_en_io = DT_INST_PROP(inst, din_en_io),                                                \
	.din_polarity = DT_INST_PROP(inst, din_polarity)

#define RTC_MAX31331_DEFINE(inst)                                                                  \
	static struct rtc_max31331_data rtc_max31331_prv_data_##inst;                              \
	static const struct rtc_max31331_config rtc_max31331_config##inst = {                      \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		RTC_MAX31331_CONFIG(inst),                                                         \
		IF_ENABLED(CONFIG_RTC_ALARM,                                    \
			   (.inta_gpios =                                      \
				    GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, \
								   {0}))) };    \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rtc_max31331_init, NULL, &rtc_max31331_prv_data_##inst,        \
			      &rtc_max31331_config##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,   \
			      &rtc_max31331);

DT_INST_FOREACH_STATUS_OKAY(RTC_MAX31331_DEFINE)
