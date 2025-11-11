/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rtc_counter

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(rtc_counter, CONFIG_RTC_LOG_LEVEL);

struct rtc_counter_config {
	const struct device *counter_dev;
	/* Number of alarm channels */
	uint8_t alarms_count;
};

struct rtc_counter_data {
	/* Unix seconds offset from raw counter ticks */
	int64_t epoch_offset;
	/* protects epoch_offset */
	struct k_spinlock lock;
#ifdef CONFIG_RTC_ALARM
	/* false if counter has no alarm channels */
	bool alarm_capable;
	/* number of alarm channels exposed (<= underlying counter channels) */
	uint8_t num_alarm_chans;
	const struct device *rtc_dev;
	/* Per-instance arrays provided by init macro */
	rtc_alarm_callback *alarm_callback;
	void **alarm_user_data;
	uint16_t *alarm_mask;
	struct rtc_time *alarm_time;
	bool *alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

/*
 * Generic RTC time to ticks conversion using time.h and counter API.
 * Returns 0 on success, -EINVAL on invalid time or overflow.
 */
static int rtc_counter_time_to_ticks(const struct rtc_time *timeptr, uint32_t *ticks_out)
{
	struct tm tm_val;
	int64_t seconds64;

	if (timeptr == NULL || ticks_out == NULL) {
		return -EINVAL;
	}

	/* Populate broken-down time structure */
	memset(&tm_val, 0, sizeof(tm_val));
	tm_val.tm_sec = timeptr->tm_sec;
	tm_val.tm_min = timeptr->tm_min;
	tm_val.tm_hour = timeptr->tm_hour;
	tm_val.tm_mday = timeptr->tm_mday;
	tm_val.tm_mon = timeptr->tm_mon;
	tm_val.tm_year = timeptr->tm_year;
	tm_val.tm_isdst = -1;

	/* UTC, 64-bit, no DST/timezone ambiguity */
	seconds64 = timeutil_timegm64(&tm_val);

	/* Reject invalid/negative/out-of-range for 32-bit tick domain */
	if (seconds64 < 0 || seconds64 > (int64_t)UINT32_MAX) {
		return -EINVAL;
	}

	*ticks_out = (uint32_t)seconds64;
	return 0;
}

/* Generic RTC ticks to time conversion using time.h and counter API */
static void rtc_counter_ticks_to_time(uint32_t ticks, struct rtc_time *timeptr)
{
	time_t seconds;
	struct tm tm_val;

	seconds = (time_t)ticks;

	if (gmtime_r(&seconds, &tm_val) == NULL) {
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

#if defined(CONFIG_RTC_ALARM)

static void rtc_counter_alarm_callback(const struct device *counter_dev, uint8_t chan_id,
				       uint32_t ticks, void *user_data)
{
	struct rtc_counter_data *data = (struct rtc_counter_data *)user_data;
	const struct device *rtc_dev = data->rtc_dev;

	if (chan_id < data->num_alarm_chans && data->alarm_callback[chan_id] != NULL) {
		data->alarm_callback[chan_id](rtc_dev, chan_id, data->alarm_user_data[chan_id]);
		data->alarm_pending[chan_id] = false;
	} else if (chan_id < data->num_alarm_chans) {
		data->alarm_pending[chan_id] = true;
	} else {
		LOG_DBG("Spurious alarm callback on channel %u (max %u)", chan_id,
			data->num_alarm_chans ? (data->num_alarm_chans - 1U) : 0U);
	}
}

static int rtc_counter_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	struct rtc_counter_data *data = dev->data;

	if (!data->alarm_capable) {
		return -ENOTSUP;
	}

	if (id >= data->num_alarm_chans) {
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |
		 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR |
		 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_YEARDAY);

	return 0;
}

static int rtc_counter_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	const struct rtc_counter_config *config = dev->config;
	struct rtc_counter_data *data = dev->data;
	uint32_t desired_ticks;
	int ret;
	int64_t epoch;
	int64_t raw_alarm_ticks_64;
	uint32_t top;
	uint32_t alarm_ticks;
	uint32_t now_raw;
	struct counter_alarm_cfg alarm_cfg;

	if (!data->alarm_capable) {
		return -ENOTSUP;
	}

	if (id >= data->num_alarm_chans) {
		return -EINVAL;
	}

	if (mask == 0U) {
		/* Disable alarm on selected channel */
		ret = counter_cancel_channel_alarm(config->counter_dev, (uint8_t)id);
		if (ret == 0) {
			K_SPINLOCK(&data->lock) {
				data->alarm_mask[id] = 0U;
				data->alarm_pending[id] = false;
				memset(&data->alarm_time[id], 0, sizeof(struct rtc_time));
			}
		}
		return ret;
	}

	if (timeptr == NULL) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		return -EINVAL;
	}

	ret = rtc_counter_time_to_ticks(timeptr, &desired_ticks);

	/* -EINVAL on overflow/invalid time */
	if (ret < 0) {
		return ret;
	}

	/* Convert desired absolute Unix time to a raw tick value for the counter */
	K_SPINLOCK(&data->lock) {
		epoch = data->epoch_offset;
	}

	raw_alarm_ticks_64 = (int64_t)desired_ticks - epoch;
	/* Reject wraparound: must be within current window and not in the past */
	top = counter_get_top_value(config->counter_dev);

	/* would require wrap */
	if (raw_alarm_ticks_64 < 0 || raw_alarm_ticks_64 > (int64_t)top) {
		return -ERANGE;
	}

	alarm_ticks = (uint32_t)raw_alarm_ticks_64;

	ret = counter_get_value(config->counter_dev, &now_raw);
	if (ret < 0) {
		return ret;
	}

	/* target already passed in this window or equals 'now' */
	if (alarm_ticks <= now_raw) {
		/* 1-tick guard, but cannot wrap */
		if (now_raw == top) {
			return -ERANGE;
		}
		alarm_ticks = now_raw + 1U;
	}

	alarm_cfg.callback = rtc_counter_alarm_callback;
	alarm_cfg.ticks = alarm_ticks;
	alarm_cfg.user_data = data;
	alarm_cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;

	/* Record configured mask and time for get_time; clear pending */
	K_SPINLOCK(&data->lock) {
		data->alarm_mask[id] = mask;
		data->alarm_time[id] = *timeptr;
		data->alarm_pending[id] = false;
	}

	return counter_set_channel_alarm(config->counter_dev, (uint8_t)id, &alarm_cfg);
}

static int rtc_counter_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	struct rtc_counter_data *data = dev->data;
	int ret = 0;

	if (!data->alarm_capable) {
		return -ENOTSUP;
	}

	if (id >= data->num_alarm_chans) {
		return -EINVAL;
	}

	if (mask == NULL || timeptr == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		if (data->alarm_mask[id] == 0U) {
			ret = -EINVAL;
		} else {
			*mask = data->alarm_mask[id];
			*timeptr = data->alarm_time[id];
		}
	}

	return ret;
}

static int rtc_counter_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int ret = 0;
	struct rtc_counter_data *data = dev->data;

	if (!data->alarm_capable) {
		return -ENOTSUP;
	}

	if (id >= data->num_alarm_chans) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		ret = data->alarm_pending[id] ? 1 : 0;
		data->alarm_pending[id] = false;
	}

	return ret;
}

static int rtc_counter_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	struct rtc_counter_data *data = dev->data;

	if (!data->alarm_capable) {
		return -ENOTSUP;
	}

	if (id >= data->num_alarm_chans) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->alarm_callback[id] = callback;
		data->alarm_user_data[id] = user_data;
	}

	return 0;
}

/* Compute rearm tick within current counter window.
 * Returns true and writes the tick to out_ticks; returns false if it should be skipped.
 */
static inline bool rtc_counter_compute_rearm_ticks(int64_t raw_alarm_ticks_64, uint32_t now_raw,
						   uint32_t top, uint32_t *out_ticks)
{
	/* Out of range */
	if (raw_alarm_ticks_64 > (int64_t)top) {
		return false;
	}

	/* In the past: arm to next tick if possible */
	if (raw_alarm_ticks_64 < 0) {
		if (now_raw == top) {
			return false;
		}
		*out_ticks = now_raw + 1U;
		return true;
	}

	/* Within range */
	*out_ticks = (uint32_t)raw_alarm_ticks_64;
	if (*out_ticks <= now_raw) {
		if (now_raw == top) {
			return false;
		}
		*out_ticks = now_raw + 1U;
	}

	return true;
}

/* Recompute and rearm all active alarms after a time base (epoch) change */
static void rtc_counter_reschedule_alarms(const struct device *dev)
{
	const struct rtc_counter_config *config = dev->config;
	struct rtc_counter_data *data = dev->data;
	uint16_t configured_mask;
	struct rtc_time configured_time;
	uint32_t alarm_abs_ticks;
	int64_t epoch;
	int64_t raw_alarm_ticks_64;
	uint32_t top;
	uint32_t now_raw;
	uint32_t alarm_ticks;
	struct counter_alarm_cfg alarm_cfg;

	/* Time base changed: reschedule any active alarms to the new epoch */
	if (!(data->alarm_capable && data->num_alarm_chans > 0U)) {
		return;
	}

	for (uint8_t id = 0U; id < data->num_alarm_chans; id++) {
		/* Snapshot configured alarm (if any) and clear pending */
		K_SPINLOCK(&data->lock) {
			configured_mask = data->alarm_mask[id];
			configured_time = data->alarm_time[id];
			data->alarm_pending[id] = false;
		}

		if (configured_mask == 0U) {
			continue;
		}

		/* Cancel any in-flight alarm before reprogramming */
		(void)counter_cancel_channel_alarm(config->counter_dev, (uint8_t)id);

		if (rtc_counter_time_to_ticks(&configured_time, &alarm_abs_ticks) < 0) {
			/* Should not happen: skip this alarm */
			continue;
		}

		K_SPINLOCK(&data->lock) {
			epoch = data->epoch_offset;
		}

		raw_alarm_ticks_64 = (int64_t)alarm_abs_ticks - epoch;
		top = counter_get_top_value(config->counter_dev);
		if (counter_get_value(config->counter_dev, &now_raw) < 0) {
			continue;
		}

		if (!rtc_counter_compute_rearm_ticks(raw_alarm_ticks_64, now_raw, top,
						     &alarm_ticks)) {
			continue;
		}

		alarm_cfg.callback = rtc_counter_alarm_callback;
		alarm_cfg.ticks = alarm_ticks;
		alarm_cfg.user_data = data;
		alarm_cfg.flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;

		(void)counter_set_channel_alarm(config->counter_dev, (uint8_t)id, &alarm_cfg);
	}
}

#endif /* CONFIG_RTC_ALARM */

static int rtc_counter_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_counter_config *config = dev->config;
	struct rtc_counter_data *data = dev->data;
	uint32_t desired_ticks = 0;
	uint32_t now_ticks = 0;
	int ret;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	ret = rtc_counter_time_to_ticks(timeptr, &desired_ticks);

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

	/* Update the software offset: offset = desired_time - now_ticks */
	K_SPINLOCK(&data->lock) {
		data->epoch_offset = (int64_t)desired_ticks - (int64_t)now_ticks;
	}

#ifdef CONFIG_RTC_ALARM
	rtc_counter_reschedule_alarms(dev);
#endif /* CONFIG_RTC_ALARM */

	/* Restart counter */
	ret = counter_start(config->counter_dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int rtc_counter_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_counter_config *config = dev->config;
	struct rtc_counter_data *data = dev->data;
	uint32_t now_ticks;
	int ret;
	int64_t epoch;
	int64_t current_seconds;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	ret = counter_get_value(config->counter_dev, &now_ticks);

	if (ret < 0) {
		return ret;
	}

	K_SPINLOCK(&data->lock) {
		epoch = data->epoch_offset;
	}

	current_seconds = (int64_t)now_ticks + epoch;

	if (current_seconds < 0 || current_seconds > UINT32_MAX) {
		return -ERANGE;
	}

	memset(timeptr, 0, sizeof(struct rtc_time));
	rtc_counter_ticks_to_time((uint32_t)current_seconds, timeptr);

	return 0;
}

#if defined(CONFIG_RTC_UPDATE)

static int rtc_counter_update_set_callback(const struct device *dev, rtc_update_callback callback,
					   void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);

	return -ENOTSUP;
}

#endif /* CONFIG_RTC_UPDATE */

#if defined(CONFIG_RTC_CALIBRATION)

static int rtc_counter_set_calibration(const struct device *dev, int32_t calibration)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(calibration);
	return -ENOTSUP;
}

static int rtc_counter_get_calibration(const struct device *dev, int32_t *calibration)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(calibration);
	return -ENOTSUP;
}

#endif /* CONFIG_RTC_CALIBRATION */

static int rtc_counter_init(const struct device *dev)
{
	const struct rtc_counter_config *config = dev->config;
	struct rtc_counter_data *data = dev->data;
	uint32_t freq;

	if (!device_is_ready(config->counter_dev)) {
		LOG_ERR("Counter device %s not ready", config->counter_dev->name);
		return -ENODEV;
	}

	/* Require a 1 Hz counter frequency */
	freq = counter_get_frequency(config->counter_dev);

	if (freq != 1U) {
		LOG_ERR("Unsupported counter frequency: %u Hz (expected 1 Hz)", freq);
		return -ENOTSUP;
	}

	/* Start with zero offset until rtc_set_time is called */
	data->epoch_offset = 0;

#ifdef CONFIG_RTC_ALARM
	data->rtc_dev = dev;
	if (config->alarms_count == 0U) {
		data->alarm_capable = false;
	} else {
		data->alarm_capable = true;
		data->num_alarm_chans = config->alarms_count;
		/* Clear per-channel state */
		for (uint8_t i = 0; i < data->num_alarm_chans; i++) {
			data->alarm_callback[i] = NULL;
			data->alarm_user_data[i] = NULL;
			data->alarm_mask[i] = 0;
			memset(&data->alarm_time[i], 0, sizeof(struct rtc_time));
			data->alarm_pending[i] = false;
		}
	}
#endif /* CONFIG_RTC_ALARM */

	return 0;
}

static DEVICE_API(rtc, rtc_counter_driver_api) = {
	.set_time = rtc_counter_set_time,
	.get_time = rtc_counter_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_supported_fields = rtc_counter_alarm_get_supported_fields,
	.alarm_set_time = rtc_counter_alarm_set_time,
	.alarm_get_time = rtc_counter_alarm_get_time,
	.alarm_is_pending = rtc_counter_alarm_is_pending,
	.alarm_set_callback = rtc_counter_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rtc_counter_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION)
	.set_calibration = rtc_counter_set_calibration,
	.get_calibration = rtc_counter_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

/* Ensure RTC init priority is bigger than counter */
BUILD_ASSERT(CONFIG_RTC_INIT_PRIORITY > CONFIG_COUNTER_INIT_PRIORITY,
	     "RTC init priority must be bigger than counter");

#define RTC_COUNTER_ALARMS_COUNT(n) DT_PROP_OR(DT_DRV_INST(n), alarms_count, 0)
#define RTC_COUNTER_ALARMS_SZ(n)    MAX(RTC_COUNTER_ALARMS_COUNT(n), 1)

#if defined(CONFIG_RTC_ALARM)

/* Per-instance static storage for alarm context, sized from DT */
#define RTC_COUNTER_DECLARE_ALARM_STORAGE(n)                                                \
	static rtc_alarm_callback rtc_counter_alarm_callback_arr_##n[RTC_COUNTER_ALARMS_SZ(n)]; \
	static void *rtc_counter_alarm_user_data_arr_##n[RTC_COUNTER_ALARMS_SZ(n)];             \
	static uint16_t rtc_counter_alarm_mask_arr_##n[RTC_COUNTER_ALARMS_SZ(n)];               \
	static struct rtc_time rtc_counter_alarm_time_arr_##n[RTC_COUNTER_ALARMS_SZ(n)];        \
	static bool rtc_counter_alarm_pending_arr_##n[RTC_COUNTER_ALARMS_SZ(n)];

DT_INST_FOREACH_STATUS_OKAY(RTC_COUNTER_DECLARE_ALARM_STORAGE)

#endif /* CONFIG_RTC_ALARM */

#define RTC_COUNTER_DEVICE_INIT(n) \
	static const struct rtc_counter_config rtc_counter_config_##n = {           \
		.counter_dev = DEVICE_DT_GET(DT_INST_PARENT(n)),                        \
		.alarms_count = DT_PROP_OR(DT_DRV_INST(n), alarms_count, 0),            \
	};                                                                          \
	static struct rtc_counter_data rtc_counter_data_##n = {                     \
		IF_ENABLED(CONFIG_RTC_ALARM, (                                          \
			.alarm_callback = rtc_counter_alarm_callback_arr_##n,               \
			.alarm_user_data = rtc_counter_alarm_user_data_arr_##n,             \
			.alarm_mask = rtc_counter_alarm_mask_arr_##n,                       \
			.alarm_time = rtc_counter_alarm_time_arr_##n,                       \
			.alarm_pending = rtc_counter_alarm_pending_arr_##n,                 \
		))                                                                      \
	};                                                                          \
	DEVICE_DT_INST_DEFINE(n, rtc_counter_init, NULL, &rtc_counter_data_##n,     \
				&rtc_counter_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, \
				&rtc_counter_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_COUNTER_DEVICE_INIT)
