/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_rtc

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtc_ameba, CONFIG_RTC_LOG_LEVEL);

/* RTC start time: 1st, Jan, 1900 */
#define RTC_YEAR_REF RTC_BASE_YEAR

/* struct tm start time:   1st, Jan, 1900 */
#define TM_YEAR_REF 1900

struct rtc_ameba_config {
	uint32_t async_prescaler;
	uint32_t sync_prescaler;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

#if defined(CONFIG_RTC_ALARM)
	void (*irq_configure)(void);
#endif
};

struct rtc_ameba_data {
	struct k_mutex lock;

#if defined(CONFIG_RTC_ALARM)
	atomic_t alarm_pending;
	rtc_alarm_callback alarm_cb;
	void *alarm_cbdata;
#endif
};

static const uint8_t dim[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**
 * @brief  Judge whether a year is a leap year or not.
 * @param  year: Actual year - 1900.
 * @return Result.
 * @retval 1: This year is a leap year.
 * @retval 0: This year is not a leap year.
 */
static inline bool is_leap_year(uint32_t year)
{
	uint32_t full_year = year + RTC_BASE_YEAR; /* start from 1900 */

	return (!(full_year % 4) && (full_year % 100)) || !(full_year % 400);
}

/**
 * @brief  Calculate total days in a specified month of a specified year.
 * @param  year: Actual year - 1900.
 * @param  month: Specified month, which can be 0~11.
 * @note 0 represents January.
 * @return Number of days in the month of the year.
 */
static uint8_t days_in_month(uint8_t month, int year)
{
	uint8_t ret = dim[month % 12];

	if (ret == 0) {
		ret = is_leap_year(year + month / 12) ? 29 : 28;
	}
	return ret;
}

/**
 * @brief Calculate day of the year according to year, month, and day of the month.
 * @param year Actual year - 1900.
 * @param mon Month (0~11), where 0 represents January.
 * @param mday Day of the month (1~31).
 * @return The day of the year (0~364 or 0~365 for leap years).
 */
static int rtc_calculate_yday(int year, int mon, int mday)
{
	int yday = 0;

	for (int i = 0; i < mon; i++) {
		yday += days_in_month(i, year);
	}

	yday += mday - 1;

	return yday;
}

/**
 * @brief  Calculate month and day of the month according to year and day of the year.
 * @param  year: Actual year - 1900.
 * @param  yday: Day of the year.
 * @param  mon: Pointer to the variable that stores month, which can be 0~11.
 * @note 0 represents January.
 * @param  mday: Pointer to the variable that stores day of month, which can be 1~31.
 * @retval none
 */
static void rtc_calculate_mday(int year, int yday, int *mon, int *mday)
{
	int t_mon = -1, t_yday = yday + 1;

	while (t_yday > 0) {
		t_mon++;
		t_yday -= days_in_month(t_mon, year);
	}

	*mon = t_mon;
	*mday = t_yday + days_in_month(t_mon, year);
}

/**
 * @brief  Calculate the day of a week according to date.
 * @param  year: Actual year - 1900.
 * @param  mon: Month of the year, which can be 0~11.
 * @note 0 represents January.
 * @param  mday: Day of the month.
 * @param  wday: Pointer to the variable that stores day of a week, which can be 0~6.
 * @note 0 represents Sunday.
 * @retval none
 */
static void rtc_calculate_wday(int year, int mon, int mday, int *wday)
{
	int t_year = year + 1900, t_mon = mon + 1;

	if (t_mon == 1 || t_mon == 2) {
		t_year--;
		t_mon += 12;
	}

	int c = t_year / 100;
	int y = t_year % 100;
	int week = (c / 4) - 2 * c + (y + y / 4) + (26 * (t_mon + 1) / 10) + mday - 1;

	while (week < 0) {
		week += 7;
	}
	week %= 7;

	*wday = week;
}

static int rtc_ameba_configure(const struct device *dev)
{
	const struct rtc_ameba_config *cfg = dev->config;
	int err = 0;
	uint32_t initialized = 0;

	RTC_InitTypeDef rtc_initstruct;

	RTC_StructInit(&rtc_initstruct);
	rtc_initstruct.RTC_AsynchPrediv = cfg->async_prescaler;
	rtc_initstruct.RTC_SynchPrediv = cfg->sync_prescaler;
	rtc_initstruct.RTC_HourFormat =
		RTC_HourFormat_24; /* force this hour fmt for time_t reason */

	initialized = RTC_Init(&rtc_initstruct);

	if (!initialized) {
		LOG_ERR("rtc initial fail.");
		err = -EIO;
	}

	return err;
}

static int rtc_ameba_init(const struct device *dev)
{
	const struct rtc_ameba_config *cfg = dev->config;
	struct rtc_ameba_data *data = dev->data;
	int err = 0;

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable RTC bus clock */
	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys)) {
		LOG_ERR("clock op failed");
		return -EIO;
	}

	RTC_Enable(ENABLE);

	k_mutex_init(&data->lock);

	err = rtc_ameba_configure(dev);

#if defined(CONFIG_RTC_ALARM)
	if (cfg->irq_configure != NULL) {
		cfg->irq_configure();
	}
#endif

	return err;
}

static int rtc_ameba_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_ameba_data *data = dev->data;
	int err = 0;
	RTC_TimeTypeDef rtc_timestruct;

	uint32_t real_year = timeptr->tm_year + TM_YEAR_REF;

	if (real_year < RTC_YEAR_REF) {
		/* RTC does not support years before 1900 */
		return -EINVAL;
	}

	if (timeptr->tm_wday == -1) {
		/* day of the week is expected */
		return -EINVAL;
	}

	/* Do not check following two members.
	 * For getting function, need to set designated Unknown value
	 * if (timeptr->tm_isdst != -1)
	 * if (!(timeptr->tm_nsec))
	 */

	err = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (err != 0) {
		LOG_ERR("%s lock fail !!!", __func__);
		return err;
	}

	rtc_timestruct.RTC_Days =
		rtc_calculate_yday(timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday);
	rtc_timestruct.RTC_H12_PMAM = RTC_H12_AM; /* cautious in zsdk */
	rtc_timestruct.RTC_Year = timeptr->tm_year + RTC_BASE_YEAR;
	rtc_timestruct.RTC_Hours = timeptr->tm_hour;
	rtc_timestruct.RTC_Minutes = timeptr->tm_min;
	rtc_timestruct.RTC_Seconds = timeptr->tm_sec;

	RTC_SetTime(RTC_Format_BIN, &rtc_timestruct);

	k_mutex_unlock(&data->lock);

	return err;
}

static int rtc_ameba_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_ameba_data *data = dev->data;
	int err = 0;
	uint32_t ydays_thr;
	RTC_TimeTypeDef rtc_timestruct;

	err = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (err) {
		return err;
	}

	/* step1: get hour, min, sec from RTC */
	RTC_GetTime(RTC_Format_BIN, &rtc_timestruct);

	timeptr->tm_sec = rtc_timestruct.RTC_Seconds;
	timeptr->tm_min = rtc_timestruct.RTC_Minutes;
	timeptr->tm_hour = rtc_timestruct.RTC_Hours;

	timeptr->tm_yday = rtc_timestruct.RTC_Days;
	timeptr->tm_year = rtc_timestruct.RTC_Year - RTC_BASE_YEAR; /* struct tm start from 1900 */

	/* step2: convert to mon, mday */
	rtc_calculate_mday(timeptr->tm_year, timeptr->tm_yday, &timeptr->tm_mon, &timeptr->tm_mday);

	/* step3: convert to wday */
	rtc_calculate_wday(timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, &timeptr->tm_wday);

	/* step4: check and update year or not */
	ydays_thr = (is_leap_year(timeptr->tm_year)) ? 366 : 365;

	if (rtc_timestruct.RTC_Days > (ydays_thr - 1)) {
		rtc_timestruct.RTC_Days -= ydays_thr;
		rtc_timestruct.RTC_Year++;

		timeptr->tm_mon = 0;  /* base 0 [0, 11] */
		timeptr->tm_mday = 1; /* base 1 [1, 31] */
		timeptr->tm_yday = rtc_timestruct.RTC_Days;
		timeptr->tm_year = rtc_timestruct.RTC_Year - RTC_BASE_YEAR;

		RTC_SetTime(RTC_Format_BIN, &rtc_timestruct);
	}

	k_mutex_unlock(&data->lock);

	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;

	return 0;
}

#if defined(CONFIG_RTC_ALARM)
static void rtc_ameba_alarm_isr(const struct device *dev)
{
	struct rtc_ameba_data *data = dev->data;

	/* clear alarm flag */
	RTC_AlarmClear();

	if (data->alarm_cb) {
		data->alarm_cb(dev, 0, data->alarm_cbdata);
		atomic_set(&data->alarm_pending, 0);
	} else {
		atomic_set(&data->alarm_pending, 1);
	}
}

static int rtc_ameba_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		return -EINVAL;
	}

	(*mask) = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		   RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_YEARDAY);

	return 0;
}

static bool rtc_ameba_validate_alarm_time(const struct rtc_time *timeptr, uint16_t mask)
{
	uint16_t sprt_mask = 0;

	rtc_ameba_alarm_get_supported_fields(NULL, 0, &sprt_mask);

	if (sprt_mask & mask) {
		if ((mask & RTC_ALARM_TIME_MASK_SECOND) &&
		    (timeptr->tm_sec < 0 || timeptr->tm_sec > 59)) {
			return false;
		}

		if ((mask & RTC_ALARM_TIME_MASK_MINUTE) &&
		    (timeptr->tm_min < 0 || timeptr->tm_min > 59)) {
			return false;
		}

		if ((mask & RTC_ALARM_TIME_MASK_HOUR) &&
		    (timeptr->tm_hour < 0 || timeptr->tm_hour > 23)) {
			return false;
		}

		if ((mask & RTC_ALARM_TIME_MASK_YEARDAY) &&
		    (timeptr->tm_yday < 0 || timeptr->tm_yday > 365)) {
			return false;
		}
	} else {
		LOG_ERR("Current mask 0x%x not supported", mask);
		return false;
	}

	return true;
}

static int rtc_ameba_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				    const struct rtc_time *timeptr)
{
	struct rtc_ameba_data *data = dev->data;
	int ret = 0;
	RTC_AlarmTypeDef rtc_alarmstruct;
	uint32_t alarm_mask = 0U;

	if (id != 0) {
		return -EINVAL;
	}

	if ((mask > 0) && (timeptr == NULL)) {
		LOG_ERR("Invalid Alarm Set Mask and timeptr !!!");
		return -EINVAL;
	}

	/* Check time valid */
	if (mask > 0) {
		if (!rtc_ameba_validate_alarm_time(timeptr, mask)) {
			return -EINVAL;
		}
	} else {
		/* If the mask parameter is 0, the alarm will be disabled. */
		RTC_AlarmCmd(DISABLE);
		return 0;
	}

	ret = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (ret) {
		return ret;
	}

	/* step1: Init Ameba alarm struct */
	RTC_AlarmStructInit(&rtc_alarmstruct);
	rtc_alarmstruct.RTC_AlarmMask = RTC_AlarmMask_All; /* fix for mask seconds unset above */
	rtc_alarmstruct.RTC_Alarm2Mask = RTC_Alarm2Mask_Days;

	/* step2: Set Ameba RTC_AlarmMask */
	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		rtc_alarmstruct.RTC_AlarmTime.RTC_Seconds = timeptr->tm_sec;
		alarm_mask |= RTC_AlarmMask_Seconds;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		rtc_alarmstruct.RTC_AlarmTime.RTC_Minutes = timeptr->tm_min;
		alarm_mask |= RTC_AlarmMask_Minutes;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		rtc_alarmstruct.RTC_AlarmTime.RTC_Hours = timeptr->tm_hour;
		alarm_mask |= RTC_AlarmMask_Hours;
	}

	/* Note: In Ameba, if the mask parameter is 0, the alarm will be enabled, which is contrary
	 * to zephyr file declarations The RTC alarm will trigger when all enabled fields of
	 * the alarm time match the RTC time.
	 */
	if (alarm_mask != 0) {
		rtc_alarmstruct.RTC_AlarmMask &= (~alarm_mask);
	}

	/* step3: Set Ameba RTC_Alarm2Mask */
	alarm_mask = 0U;
	if (mask & RTC_ALARM_TIME_MASK_YEARDAY) {
		rtc_alarmstruct.RTC_AlarmTime.RTC_Days = timeptr->tm_yday;
		alarm_mask |= RTC_Alarm2Mask_Days;

		rtc_alarmstruct.RTC_Alarm2Mask &= (~alarm_mask);
	}

	RTC_SetAlarm(RTC_Format_BIN, &rtc_alarmstruct);
	RTC_AlarmCmd(ENABLE);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_ameba_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				    struct rtc_time *timeptr)
{
	struct rtc_ameba_data *data = dev->data;
	int ret = 0;

	RTC_AlarmTypeDef rtc_alarmstruct;

	if (id != 0) {
		return -EINVAL;
	}

	if (timeptr == NULL) {
		LOG_ERR("Invalid Get Alarm timeptr");
		return -EINVAL;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	ret = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (ret) {
		return ret;
	}

	/* step1: read RTC reg */
	RTC_GetAlarm(RTC_Format_BIN, &rtc_alarmstruct);

	/* step2: parse RTC_AlarmMask */
	if (!(rtc_alarmstruct.RTC_AlarmMask & RTC_AlarmMask_Seconds)) {
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
		timeptr->tm_sec = rtc_alarmstruct.RTC_AlarmTime.RTC_Seconds;
	}

	if (!(rtc_alarmstruct.RTC_AlarmMask & RTC_AlarmMask_Minutes)) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
		timeptr->tm_min = rtc_alarmstruct.RTC_AlarmTime.RTC_Minutes;
	}

	if (!(rtc_alarmstruct.RTC_AlarmMask & RTC_AlarmMask_Hours)) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
		timeptr->tm_hour = rtc_alarmstruct.RTC_AlarmTime.RTC_Hours;
	}

	/* step3: parse RTC_Alarm2Mask */
	if (!(rtc_alarmstruct.RTC_Alarm2Mask & RTC_Alarm2Mask_Days)) {
		*mask |= RTC_ALARM_TIME_MASK_YEARDAY;
		timeptr->tm_yday = rtc_alarmstruct.RTC_AlarmTime.RTC_Days;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_ameba_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_ameba_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	int pending = atomic_set(&data->alarm_pending, 0);

	return pending ? 1 : 0;
}

static int rtc_ameba_alarm_set_callback(const struct device *dev, uint16_t id,
					rtc_alarm_callback callback, void *user_data)
{
	struct rtc_ameba_data *data = dev->data;

	int ret = 0;

	if (id != 0) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (ret) {
		return ret;
	}

	data->alarm_cb = callback;
	data->alarm_cbdata = user_data;

	/* Note: This enable alarm cb but not alarm, refer to doc.
	 * The alarm will remain enabled until manually disabled using rtc_alarm_set_time().
	 */

	k_mutex_unlock(&data->lock);

	return 0;
}
#endif

static DEVICE_API(rtc, rtc_ameba_driver_api) = {
	.set_time = rtc_ameba_set_time,
	.get_time = rtc_ameba_get_time,

#if defined(CONFIG_RTC_ALARM) || defined(__DOXYGEN__)
	.alarm_get_supported_fields = rtc_ameba_alarm_get_supported_fields,
	.alarm_set_time = rtc_ameba_alarm_set_time,
	.alarm_get_time = rtc_ameba_alarm_get_time,
	.alarm_is_pending = rtc_ameba_alarm_is_pending,
	.alarm_set_callback = rtc_ameba_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

#if defined(CONFIG_RTC_ALARM)
static void rtc_ameba_irq_configure(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rtc_ameba_alarm_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
#endif

static const struct rtc_ameba_config rtc_config = {
	.async_prescaler = DT_INST_PROP(0, async_prescaler),
	.sync_prescaler = DT_INST_PROP(0, sync_prescaler),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, idx),
#ifdef CONFIG_RTC_ALARM
	.irq_configure = rtc_ameba_irq_configure,
#endif
};

static struct rtc_ameba_data rtc_data;

DEVICE_DT_INST_DEFINE(0, &rtc_ameba_init, NULL, &rtc_data, &rtc_config, PRE_KERNEL_1,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_ameba_driver_api);
