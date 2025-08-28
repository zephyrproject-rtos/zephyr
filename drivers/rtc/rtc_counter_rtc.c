/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_counter_rtc

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <time.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(counter_rtc, CONFIG_RTC_LOG_LEVEL);

struct counter_rtc_config {
	const struct device *counter_dev;
};

struct counter_rtc_data {
	/* Unix seconds offset from raw counter ticks */
	int64_t epoch_offset;
#ifdef CONFIG_RTC_ALARM
	const struct device *rtc_dev;
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	uint16_t alarm_mask;
	bool alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

/*
 * Generic RTC time to ticks conversion using time.h and counter API.
 * Returns 0 on success, -EINVAL on invalid time or overflow.
 */
static int counter_rtc_time_to_ticks(const struct rtc_time *timeptr, uint32_t *ticks_out)
{
	if (timeptr == NULL || ticks_out == NULL) {
		return -EINVAL;
	}

	struct tm tm_val = {
		.tm_sec = timeptr->tm_sec,
		.tm_min = timeptr->tm_min,
		.tm_hour = timeptr->tm_hour,
		.tm_mday = timeptr->tm_mday,
		.tm_mon = timeptr->tm_mon,
		.tm_year = timeptr->tm_year,
		.tm_isdst = -1,
	};

	time_t seconds = mktime(&tm_val);

	/* Reject invalid or negative epoch times */
	if (seconds == (time_t)-1 || seconds < 0) {
		return -EINVAL;
	}

	/* With 1 Hz counter (asserted in init), ticks == seconds. Check 32-bit overflow. */
	uint64_t s64 = (uint64_t)seconds;

	if (s64 > 0xFFFFFFFFULL) {
		return -EINVAL;
	}

	*ticks_out = (uint32_t)s64;
	return 0;
}

/* Generic RTC ticks to time conversion using time.h and counter API */
static void counter_rtc_ticks_to_time(uint32_t ticks, struct rtc_time *timeptr)
{
	time_t seconds = (time_t)ticks;
	struct tm tm_val;

	if (!gmtime_r(&seconds, &tm_val)) {
		memset(timeptr, 0, sizeof(struct rtc_time));
		return;
	}

	timeptr->tm_sec = tm_val.tm_sec;
	timeptr->tm_min = tm_val.tm_min;
	timeptr->tm_hour = tm_val.tm_hour;
	timeptr->tm_mday = tm_val.tm_mday;
	timeptr->tm_mon = tm_val.tm_mon;
	timeptr->tm_year = tm_val.tm_year;
	timeptr->tm_wday = tm_val.tm_wday;
	timeptr->tm_yday = tm_val.tm_yday;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;
}

static int counter_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	if (!timeptr) {
		return -EINVAL;
	}

	const struct counter_rtc_config *config = dev->config;
	struct counter_rtc_data *data = dev->data;

	uint32_t desired_ticks = 0;
	uint32_t now_ticks = 0;

	int ret = counter_rtc_time_to_ticks(timeptr, &desired_ticks);

	/* -EINVAL on overflow/invalid time */
	if (ret < 0) {
		return ret;
	}

	/* Stop counter */
	ret = counter_stop(config->counter_dev);

	if (ret < 0) {
		return ret;
	}

	ret = counter_get_value(config->counter_dev, &now_ticks);
	if (ret < 0) {
		return ret;
	}

	/* Update the software offset: offset = desired_time - current_raw_ticks */
	data->epoch_offset = (int64_t)desired_ticks - (int64_t)now_ticks;

	/* Restart counter */
	return counter_start(config->counter_dev);
}

static int counter_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	if (!timeptr) {
		return -EINVAL;
	}

	const struct counter_rtc_config *config = dev->config;
	struct counter_rtc_data *data = dev->data;
	uint32_t now_ticks;

	int ret = counter_get_value(config->counter_dev, &now_ticks);

	if (ret < 0) {
		return ret;
	}

	/* Calculate current time: unix_time = current_raw_ticks + offset */
	int64_t current_seconds = (int64_t)now_ticks + data->epoch_offset;

	if (current_seconds < 0 || current_seconds > UINT32_MAX) {
		return -ERANGE;
	}

	memset(timeptr, 0, sizeof(struct rtc_time));
	counter_rtc_ticks_to_time((uint32_t)current_seconds, timeptr);

	return 0;
}

#if defined(CONFIG_RTC_ALARM)

static int counter_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |
		 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR |
		 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_YEARDAY);

	return 0;
}

static void counter_rtc_alarm_callback(const struct device *counter_dev, uint8_t chan_id,
				       uint32_t ticks, void *user_data)
{
	struct counter_rtc_data *data = (struct counter_rtc_data *)user_data;
	const struct device *rtc_dev = data->rtc_dev;

	if (data->alarm_callback) {
		data->alarm_callback(rtc_dev, 0, data->alarm_user_data);
		data->alarm_pending = false;
	} else {
		data->alarm_pending = true;
	}
}

static int counter_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	if (id != 0 || (mask && (timeptr == 0)) ||
	    (timeptr && !rtc_utils_validate_rtc_time(timeptr, mask))) {
		return -EINVAL;
	}

	const struct counter_rtc_config *config = dev->config;
	struct counter_rtc_data *data = dev->data;

	if (mask == 0) {
		/* Disable alarm */
		return counter_cancel_channel_alarm(config->counter_dev, 0);
	}

	uint32_t desired_ticks;
	int ret = counter_rtc_time_to_ticks(timeptr, &desired_ticks);

	/* -EINVAL on overflow/invalid time */
	if (ret < 0) {
		return ret;
	}

	/* Convert desired absolute Unix time to a raw tick value for the counter */
	int64_t raw_alarm_ticks_64 = (int64_t)desired_ticks - data->epoch_offset;
	/* Reject wraparound: must be within current window and not in the past */
	uint32_t top = counter_get_top_value(config->counter_dev);

	/* would require wrap */
	if (raw_alarm_ticks_64 < 0 || raw_alarm_ticks_64 > (int64_t)top) {
		return -ERANGE;
	}

	uint32_t alarm_ticks = (uint32_t)raw_alarm_ticks_64;
	uint32_t now_raw;

	ret = counter_get_value(config->counter_dev, &now_raw);
	if (ret < 0) {
		return ret;
	}

	/* target already passed in this window */
	if (alarm_ticks < now_raw) {
		return -ERANGE;
	}

	struct counter_alarm_cfg alarm_cfg = {
		.callback = counter_rtc_alarm_callback,
		.ticks = alarm_ticks,
		.user_data = data,
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
	};

	data->alarm_mask = mask;
	data->alarm_pending = false;

	return counter_set_channel_alarm(config->counter_dev, 0, &alarm_cfg);
}

static int counter_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);
	ARG_UNUSED(mask);
	ARG_UNUSED(timeptr);

	return -ENOTSUP;
}

static int counter_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	if (id != 0) {
		return -EINVAL;
	}

	struct counter_rtc_data *data = dev->data;
	int ret = 0;

	uint32_t key = irq_lock();

	ret = data->alarm_pending ? 1 : 0;
	data->alarm_pending = false;
	irq_unlock(key);

	return ret;
}

static int counter_rtc_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	if (id != 0) {
		return -EINVAL;
	}

	struct counter_rtc_data *data = dev->data;
	uint32_t key = irq_lock();

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	irq_unlock(key);

	return 0;
}

#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)

static int counter_rtc_update_set_callback(const struct device *dev, rtc_update_callback callback,
					   void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

#endif /* CONFIG_RTC_UPDATE */

#if defined(CONFIG_RTC_CALIBRATION)

static int counter_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(calibration);
	return -ENOTSUP;
}

static int counter_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(calibration);
	return -ENOTSUP;
}

#endif /* CONFIG_RTC_CALIBRATION */

static int counter_rtc_init(const struct device *dev)
{
	const struct counter_rtc_config *config = dev->config;
	struct counter_rtc_data *data = dev->data;

	if (!device_is_ready(config->counter_dev)) {
		LOG_ERR("Counter device %s not ready", config->counter_dev->name);
		return -ENODEV;
	}

	/* Require a 1 Hz counter frequency */
	uint32_t freq = counter_get_frequency(config->counter_dev);

	if (freq != 1U) {
		LOG_ERR("Unsupported counter frequency: %u Hz (expected 1 Hz)", freq);
		return -ENOTSUP;
	}

	/* Start with zero offset until rtc_set_time is called */
	data->epoch_offset = 0;

#ifdef CONFIG_RTC_ALARM
	data->rtc_dev = dev;
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_mask = 0;
	data->alarm_pending = false;
#endif /* CONFIG_RTC_ALARM */

	return 0;
}

static DEVICE_API(rtc, rtc_counter_rtc_driver_api) = {
	.set_time = counter_rtc_set_time,
	.get_time = counter_rtc_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_supported_fields = counter_rtc_alarm_get_supported_fields,
	.alarm_set_time = counter_rtc_alarm_set_time,
	.alarm_get_time = counter_rtc_alarm_get_time,
	.alarm_is_pending = counter_rtc_alarm_is_pending,
	.alarm_set_callback = counter_rtc_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = counter_rtc_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION)
	.set_calibration = counter_rtc_set_calibration,
	.get_calibration = counter_rtc_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

/* Ensure RTC init priority is bigger than counter */
BUILD_ASSERT(CONFIG_RTC_INIT_PRIORITY > CONFIG_COUNTER_INIT_PRIORITY,
	     "RTC init priority must be bigger than counter");

#define COUNTER_RTC_DEVICE_INIT(n)                                                \
	static const struct counter_rtc_config counter_rtc_config_##n = {             \
		.counter_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                          \
	};                                                                            \
	static struct counter_rtc_data counter_rtc_data_##n;                          \
	DEVICE_DT_INST_DEFINE(n, counter_rtc_init, NULL, &counter_rtc_data_##n,       \
			      &counter_rtc_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, \
			      &rtc_counter_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RTC_DEVICE_INIT)
