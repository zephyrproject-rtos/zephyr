/*
 * Copyright (c) 2022 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_rtc_emul

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#include "rtc_utils.h"

struct rtc_emul_data;

struct rtc_emul_work_delayable {
	struct k_work_delayable dwork;
	const struct device *dev;
};

struct rtc_emul_alarm {
	struct rtc_time datetime;
	rtc_alarm_callback callback;
	void *user_data;
	uint16_t mask;
	bool pending;
};

struct rtc_emul_data {
	bool datetime_set;

	struct rtc_time datetime;

	struct k_spinlock lock;

	struct rtc_emul_work_delayable dwork;

#ifdef CONFIG_RTC_ALARM
	struct rtc_emul_alarm *alarms;
	uint16_t alarms_count;
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_callback_user_data;
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
	int32_t calibration;
#endif /* CONFIG_RTC_CALIBRATION */
};

static const uint8_t rtc_emul_days_in_month[12] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static const uint8_t rtc_emul_days_in_month_with_leap[12] = {
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static bool rtc_emul_is_leap_year(struct rtc_time *datetime)
{
	if ((datetime->tm_year % 400 == 0) ||
	    (((datetime->tm_year % 100) > 0) && ((datetime->tm_year % 4) == 0))) {
		return true;
	}

	return false;
}

static int rtc_emul_get_days_in_month(struct rtc_time *datetime)
{
	const uint8_t *dim = (rtc_emul_is_leap_year(datetime) == true) ?
			     (rtc_emul_days_in_month_with_leap) :
			     (rtc_emul_days_in_month);

	return dim[datetime->tm_mon];
}

static void rtc_emul_increment_tm(struct rtc_time *datetime)
{
	/* Increment second */
	datetime->tm_sec++;

	/* Validate second limit */
	if (datetime->tm_sec < 60) {
		return;
	}

	datetime->tm_sec = 0;

	/* Increment minute */
	datetime->tm_min++;

	/* Validate minute limit */
	if (datetime->tm_min < 60) {
		return;
	}

	datetime->tm_min = 0;

	/* Increment hour */
	datetime->tm_hour++;

	/* Validate hour limit */
	if (datetime->tm_hour < 24) {
		return;
	}

	datetime->tm_hour = 0;

	/* Increment day */
	datetime->tm_wday++;
	datetime->tm_mday++;
	datetime->tm_yday++;

	/* Limit week day */
	if (datetime->tm_wday > 6) {
		datetime->tm_wday = 0;
	}

	/* Validate month limit */
	if (datetime->tm_mday <= rtc_emul_get_days_in_month(datetime)) {
		return;
	}

	datetime->tm_mday = 1;

	/* Increment month */
	datetime->tm_mon++;

	/* Validate month limit */
	if (datetime->tm_mon < 12) {
		return;
	}

	/* Increment year */
	datetime->tm_mon = 0;
	datetime->tm_yday = 0;
	datetime->tm_year++;
}

#ifdef CONFIG_RTC_ALARM
static void rtc_emul_test_alarms(const struct device *dev)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;
	struct rtc_emul_alarm *alarm;

	for (uint16_t i = 0; i < data->alarms_count; i++) {
		alarm = &data->alarms[i];

		if (alarm->mask == 0) {
			continue;
		}

		if ((alarm->mask & RTC_ALARM_TIME_MASK_SECOND) &&
		    (alarm->datetime.tm_sec != data->datetime.tm_sec)) {
			continue;
		}

		if ((alarm->mask & RTC_ALARM_TIME_MASK_MINUTE) &&
		    (alarm->datetime.tm_min != data->datetime.tm_min)) {
			continue;
		}

		if ((alarm->mask & RTC_ALARM_TIME_MASK_HOUR) &&
		    (alarm->datetime.tm_hour != data->datetime.tm_hour)) {
			continue;
		}

		if ((alarm->mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
		    (alarm->datetime.tm_mday != data->datetime.tm_mday)) {
			continue;
		}

		if ((alarm->mask & RTC_ALARM_TIME_MASK_MONTH) &&
		    (alarm->datetime.tm_mon != data->datetime.tm_mon)) {
			continue;
		}

		if ((alarm->mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
		    (alarm->datetime.tm_wday != data->datetime.tm_wday)) {
			continue;
		}

		if (alarm->callback == NULL) {
			alarm->pending = true;

			continue;
		}

		alarm->callback(dev, i, alarm->user_data);

		alarm->pending = false;
	}
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static void rtc_emul_invoke_update_callback(const struct device *dev)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->update_callback == NULL) {
		return;
	}

	data->update_callback(dev, data->update_callback_user_data);
}
#endif /* CONFIG_RTC_UPDATE */

static void rtc_emul_update(struct k_work *work)
{
	struct rtc_emul_work_delayable *work_delayable = (struct rtc_emul_work_delayable *)work;
	const struct device *dev = work_delayable->dev;
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	k_work_schedule(&work_delayable->dwork, K_MSEC(1000));

	K_SPINLOCK(&data->lock) {
		rtc_emul_increment_tm(&data->datetime);

#ifdef CONFIG_RTC_ALARM
		rtc_emul_test_alarms(dev);
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
		rtc_emul_invoke_update_callback(dev);
#endif /* CONFIG_RTC_UPDATE */
	}
}

static int rtc_emul_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	/* Validate arguments */
	if (timeptr == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock)
	{
		data->datetime = *timeptr;
		data->datetime.tm_isdst = -1;
		data->datetime.tm_nsec = 0;

		data->datetime_set = true;
	}

	return 0;
}

static int rtc_emul_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;
	int ret = 0;

	/* Validate arguments */
	if (timeptr == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock)
	{
		/* Validate RTC time is set */
		if (data->datetime_set == false) {
			ret = -ENODATA;

			K_SPINLOCK_BREAK;
		}

		*timeptr = data->datetime;
	}

	return ret;
}

#ifdef CONFIG_RTC_ALARM
static int  rtc_emul_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_SECOND
		 | RTC_ALARM_TIME_MASK_MINUTE
		 | RTC_ALARM_TIME_MASK_HOUR
		 | RTC_ALARM_TIME_MASK_MONTHDAY
		 | RTC_ALARM_TIME_MASK_MONTH
		 | RTC_ALARM_TIME_MASK_WEEKDAY);

	return 0;
}

static int rtc_emul_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	if ((mask > 0) && (timeptr == NULL)) {
		return -EINVAL;
	}

	if (mask > 0) {
		if (rtc_utils_validate_rtc_time(timeptr, mask) == false) {
			return -EINVAL;
		}
	}

	K_SPINLOCK(&data->lock)
	{
		data->alarms[id].mask = mask;

		if (timeptr != NULL) {
			data->alarms[id].datetime = *timeptr;
		}
	}

	return 0;
}

static int rtc_emul_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock)
	{
		*timeptr = data->alarms[id].datetime;
		*mask = data->alarms[id].mask;
	}

	return 0;
}

static int rtc_emul_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;
	int ret = 0;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock)
	{
		ret = (data->alarms[id].pending == true) ? 1 : 0;

		data->alarms[id].pending = false;
	}

	return ret;
}

static int rtc_emul_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock)
	{
		data->alarms[id].callback = callback;
		data->alarms[id].user_data = user_data;
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_emul_update_set_callback(const struct device *dev,
					    rtc_update_callback callback, void *user_data)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	K_SPINLOCK(&data->lock)
	{
		data->update_callback = callback;
		data->update_callback_user_data = user_data;
	}

	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
static int rtc_emul_set_calibration(const struct device *dev, int32_t calibration)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	K_SPINLOCK(&data->lock)
	{
		data->calibration = calibration;
	}

	return 0;
}

static int rtc_emul_get_calibration(const struct device *dev, int32_t *calibration)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	K_SPINLOCK(&data->lock)
	{
		*calibration = data->calibration;
	}

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

static DEVICE_API(rtc, rtc_emul_driver_api) = {
	.set_time = rtc_emul_set_time,
	.get_time = rtc_emul_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_emul_alarm_get_supported_fields,
	.alarm_set_time = rtc_emul_alarm_set_time,
	.alarm_get_time = rtc_emul_alarm_get_time,
	.alarm_is_pending = rtc_emul_alarm_is_pending,
	.alarm_set_callback = rtc_emul_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_emul_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_emul_set_calibration,
	.get_calibration = rtc_emul_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

int rtc_emul_init(const struct device *dev)
{
	struct rtc_emul_data *data = (struct rtc_emul_data *)dev->data;

	data->dwork.dev = dev;
	k_work_init_delayable(&data->dwork.dwork, rtc_emul_update);

	k_work_schedule(&data->dwork.dwork, K_MSEC(1000));

	return 0;
}

#ifdef CONFIG_RTC_ALARM
#define RTC_EMUL_DEVICE_DATA(id)								\
	static struct rtc_emul_alarm rtc_emul_alarms_##id[DT_INST_PROP(id, alarms_count)];	\
												\
	struct rtc_emul_data rtc_emul_data_##id = {						\
		.alarms = rtc_emul_alarms_##id,							\
		.alarms_count = ARRAY_SIZE(rtc_emul_alarms_##id),				\
	};
#else
#define RTC_EMUL_DEVICE_DATA(id)								\
	struct rtc_emul_data rtc_emul_data_##id;
#endif /* CONFIG_RTC_ALARM */

#define RTC_EMUL_DEVICE(id)									\
	RTC_EMUL_DEVICE_DATA(id)								\
												\
	DEVICE_DT_INST_DEFINE(id, rtc_emul_init, NULL, &rtc_emul_data_##id, NULL, POST_KERNEL,	\
			      CONFIG_RTC_INIT_PRIORITY, &rtc_emul_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_EMUL_DEVICE);
