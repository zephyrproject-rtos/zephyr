/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Silicon Laboratories Inc.
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"
#include "sl_si91x_calendar.h"

#define DT_DRV_COMPAT silabs_siwx91x_rtc

LOG_MODULE_REGISTER(siwx91x_rtc, CONFIG_RTC_LOG_LEVEL);

#define TM_YEAR_REF          1900
#define SIWX91X_RTC_YEAR_MAX 2399
#define SIWX91X_RTC_YEAR_MIN 2000

struct siwx91x_rtc_config {
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};
struct siwx91x_rtc_data {
	struct k_spinlock lock;
};

static void rtc_time_to_siwx91x_time_set(const struct rtc_time *tm,
					 sl_calendar_datetime_config_t *cldr)
{
	int full_year = tm->tm_year + TM_YEAR_REF;

	cldr->Year = (full_year % 100);
	cldr->Century = (full_year >= 2000) ? (full_year - 2000) / 100 : 0;
	cldr->Month = tm->tm_mon + 1;
	cldr->Day = tm->tm_mday;
	cldr->DayOfWeek = (RTC_DAY_OF_WEEK_T)tm->tm_wday;
	cldr->Hour = tm->tm_hour;
	cldr->Minute = tm->tm_min;
	cldr->Second = tm->tm_sec;
	cldr->MilliSeconds = tm->tm_nsec / NSEC_PER_MSEC;
}

static void siwx91x_time_to_rtc_time_set(const sl_calendar_datetime_config_t *cldr,
					 struct rtc_time *tm)
{
	int full_year = 2000 + (cldr->Century * 100) + cldr->Year;

	tm->tm_year = full_year - TM_YEAR_REF;
	tm->tm_mon = cldr->Month - 1;
	tm->tm_mday = cldr->Day;
	tm->tm_wday = (int)cldr->DayOfWeek;
	tm->tm_hour = cldr->Hour;
	tm->tm_min = cldr->Minute;
	tm->tm_sec = cldr->Second;
	tm->tm_nsec = cldr->MilliSeconds * NSEC_PER_MSEC;
}

static int siwx91x_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	sl_calendar_datetime_config_t siwx91x_time = {};
	struct siwx91x_rtc_data *data = dev->data;
	int year;
	int ret;

	year = timeptr->tm_year + TM_YEAR_REF;
	if (year < SIWX91X_RTC_YEAR_MIN || year > SIWX91X_RTC_YEAR_MAX) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	LOG_DBG("Set RTC time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	rtc_time_to_siwx91x_time_set(timeptr, &siwx91x_time);

	ret = sl_si91x_calendar_set_date_time(&siwx91x_time);
	if (ret) {
		LOG_WRN("Set Timer returned an error - %d!", ret);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int siwx91x_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	sl_calendar_datetime_config_t siwx91x_time = {};
	struct siwx91x_rtc_data *data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = sl_si91x_calendar_get_date_time(&siwx91x_time);
	if (ret != 0) {
		LOG_WRN("Get Timer returned an error - %d!", ret);
		goto unlock;
	}

	siwx91x_time_to_rtc_time_set(&siwx91x_time, timeptr);

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static int siwx91x_rtc_init(const struct device *dev)
{
	const struct siwx91x_rtc_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret) {
		return ret;
	}

	sl_si91x_calendar_init();

	return 0;
}

static DEVICE_API(rtc, siwx91x_rtc_driver_api) = {
	.set_time = siwx91x_rtc_set_time,
	.get_time = siwx91x_rtc_get_time,
};

#define SIWX91X_RTC_INIT(inst)                                                                     \
	static const struct siwx91x_rtc_config siwx91x_rtc_config_##inst = {                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
	};                                                                                         \
                                                                                                   \
	static struct siwx91x_rtc_data siwx91x_rtc_data##inst;                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &siwx91x_rtc_init, NULL, &siwx91x_rtc_data##inst,              \
			      &siwx91x_rtc_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,   \
			      &siwx91x_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_RTC_INIT)
