/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_npcm_rtc

#include "soc_miwu.h"
#include <assert.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/espi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/misc/c2h_npcm/c2h_npcm.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include "rtc_utils.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rtc_npcm);

/* RTC support 2000 ~ 2099 */
#define NPCM_RTC_YEAR_MIN 2000
#define NPCM_RTC_YEAR_MAX 2099
/* The valid range is 0 - 99 in the reg RTCYEAR, since year of tm start time is 1900
 * If the year to be stored is 2000, the difference is 100 and would be out of range.
 * Use NPCM_YEAR_GAP when dealing with the year calculation.
 */
#define NPCM_YEAR_GAP 100
/* struct tm start time:   1st, Jan, 1900 */
#define TM_YEAR_REF 1900
#define DELAY_COUNT 500
#define TIMER_DELAY_COUNT 10

struct rtc_npcm_config {
	struct c2h_reg *const inst_c2h;
	const struct npcm_wui rtcwk;
	const struct device *c2h_dev;
};

/* Driver data */
struct rtc_npcm_data {
	struct k_spinlock lock;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	bool alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

struct rtc_npcm_time {
	uint8_t year;		/* Year value */
	uint8_t month;		/* Month value */
	uint8_t day;		/* Day value */
	uint8_t day_of_week;	/* Day of week value */
	uint8_t hour;		/* Hour value */
	uint8_t minute;	        /* Minute value */
	uint8_t second;	        /* Second value */
	uint8_t time_scale;	/* 12-Hour, 24-Hour */
	uint8_t am_pm;		/* Only Time Scale select 12-hr used */
};

struct miwu_callback rtc_miwu_cb;

static int rtc_npcm_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_npcm_data *data = dev->data;
	const struct rtc_npcm_config *cfg = dev->config;
	const struct device *c2h_dev = cfg->c2h_dev;
	uint32_t real_year = timeptr->tm_year + TM_YEAR_REF;
	uint8_t i, val;

	if ((real_year < NPCM_RTC_YEAR_MIN) || (real_year > NPCM_RTC_YEAR_MAX)) {
		/* RTC can't support years out of 2000 ~ 2099 */
		return -EINVAL;
	}

	if (timeptr->tm_wday == -1) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	val = rtc_read_offset(c2h_dev, RTC_CFG);
	val &= ~RTC_CFG_ENRTCTIME_Msk;
	rtc_write_offset(c2h_dev, RTC_CFG, val);

	for (i = 0; i < TIMER_DELAY_COUNT; i++) {
		if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_ENRTCTIMESTS_Msk))
			break;
		k_busy_wait(1);
	}

	if (i == TIMER_DELAY_COUNT) {
		LOG_ERR("%s Unable to disable RTC timer", __func__);
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* tm_year is the difference between the user input and 1900
	 * For example, if the user sets year 2025, tm_year value is 125
	 * The valid range in the register is 0 to 99. Need to subtract
	 * 100 if tm_year value is over 100.
	 */

	/* Set year */
	rtc_write_offset(c2h_dev, RTC_YEAR, bin2bcd((timeptr->tm_year - NPCM_YEAR_GAP)));

	/*Set month */
	/* tm Month = 0 - 11, Jan in NPCM RTCMONTH reg starts from 1 */
	rtc_write_offset(c2h_dev, RTC_MONTH, bin2bcd(timeptr->tm_mon + 1));

	/* Set weekdays */
	/* tm Sunday = 0, Sunday in NPCM RTCWEEKDAY reg starts from 1 */
	rtc_write_offset(c2h_dev, RTC_WEEKDAY, (timeptr->tm_wday + 1));

	/* Set days */
	rtc_write_offset(c2h_dev, RTC_DAY, bin2bcd(timeptr->tm_mday));

	/* Set hours */
	rtc_write_offset(c2h_dev, RTC_HOUR, bin2bcd(timeptr->tm_hour));

	/* Set minutes */
	rtc_write_offset(c2h_dev, RTC_MIN, bin2bcd(timeptr->tm_min));

	/* Set seconds */
	rtc_write_offset(c2h_dev, RTC_SEC, bin2bcd(timeptr->tm_sec));

	LOG_DBG("YEAR %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_YEAR)));
	LOG_DBG("MONTH %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_MONTH)));
	LOG_DBG("WDAY %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_WEEKDAY)));
	LOG_DBG("DAY %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_DAY)));
	LOG_DBG("HOUR %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_HOUR)));
	LOG_DBG("MIN %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_MIN)));
	LOG_DBG("SEC %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_SEC)));

	val = rtc_read_offset(c2h_dev, RTC_CFG);
	val |= RTC_CFG_ENRTCTIME_Msk;
	rtc_write_offset(c2h_dev, RTC_CFG, val);
	LOG_DBG("CTL 0x%x CFG 0x%x", rtc_read_offset(c2h_dev, RTC_CTL),
			rtc_read_offset(c2h_dev, RTC_CFG));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_npcm_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_npcm_data *data = dev->data;
	const struct rtc_npcm_config *cfg = dev->config;
	const struct device *c2h_dev = cfg->c2h_dev;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Nanoseconds */
	timeptr->tm_nsec = 0;

	/* Get seconds */
	timeptr->tm_sec = bcd2bin(rtc_read_offset(c2h_dev, RTC_SEC));

	/* Get minutes */
	timeptr->tm_min = bcd2bin(rtc_read_offset(c2h_dev, RTC_MIN));

	/* Get hours */
	timeptr->tm_hour = bcd2bin(rtc_read_offset(c2h_dev, RTC_HOUR));

	/* Get days */
	timeptr->tm_mday = bcd2bin(rtc_read_offset(c2h_dev, RTC_DAY));

	/* Get weekdays */
	timeptr->tm_wday = rtc_read_offset(c2h_dev, RTC_WEEKDAY) - 1;

	/* Get month */
	timeptr->tm_mon = bcd2bin(rtc_read_offset(c2h_dev, RTC_MONTH)) - 1;

	/* Get year */
	timeptr->tm_year = bcd2bin(rtc_read_offset(c2h_dev, RTC_YEAR)) + NPCM_YEAR_GAP;

	/* Day number not used */
	timeptr->tm_yday = -1;

	/* DST not used  */
	timeptr->tm_isdst = -1;

	LOG_DBG("G SEC %d", timeptr->tm_sec);
	LOG_DBG("G MIN %d", timeptr->tm_min);
	LOG_DBG("G HOUR %d", timeptr->tm_hour);
	LOG_DBG("G DAY %d", timeptr->tm_mday);
	LOG_DBG("G WDAY %d", timeptr->tm_wday);
	LOG_DBG("G MONTH %d", timeptr->tm_mon);
	LOG_DBG("G YEAR %d", timeptr->tm_year);

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_npcm_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	*mask =  RTC_ALARM_TIME_MASK_SECOND
		 | RTC_ALARM_TIME_MASK_MINUTE
		 | RTC_ALARM_TIME_MASK_HOUR
		 | RTC_ALARM_TIME_MASK_MONTHDAY
		 | RTC_ALARM_TIME_MASK_MONTH
		 | RTC_ALARM_TIME_MASK_YEAR
		 | RTC_ALARM_TIME_MASK_WEEKDAY;

	return 0;
}

static int rtc_npcm_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	struct rtc_npcm_data *data = dev->data;
	const struct rtc_npcm_config *cfg = dev->config;
	const struct device *c2h_dev = cfg->c2h_dev;
	uint32_t real_year;
	uint16_t mask_capable;
	uint8_t i, val;

	rtc_npcm_alarm_get_supported_fields(dev, 0, &mask_capable);

	if ((id != 0)) {
		return -EINVAL;
	}

	if ((mask != 0) && (timeptr == NULL)) {
		return -EINVAL;
	}

	if (mask & ~mask_capable) {
		return -EINVAL;
	}

	if (rtc_utils_validate_rtc_time(timeptr, mask) == false) {
		return -EINVAL;
	}

	/* rtc_utils_validate_rtc_time supports tm_year from 0 (inclusive) to 199 (inclusive) */
	if (timeptr != NULL) {
		real_year = timeptr->tm_year + TM_YEAR_REF;
		/* RTC can't support years out of 2000 ~ 2099.
		 * IOW, tm_year valid range is from 100 (inclusive) - 199 (inclusive), not
		 * less than 100 since tm year starts from 1900.
		 */
		if ((real_year < NPCM_RTC_YEAR_MIN) || (real_year > NPCM_RTC_YEAR_MAX))
			return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if ((mask == 0) || (timeptr == NULL)) {
		/* Disable rtc alarm interrupt */
		val = rtc_read_offset(c2h_dev, RTC_CTS);
		if (rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_PADSTS_Msk) {
			val = rtc_read_offset(c2h_dev, RTC_CTL);
			val &= ~RTC_CTRL_AIE_Msk;
			rtc_write_offset(c2h_dev, RTC_CTL, val);
		}
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	val = rtc_read_offset(c2h_dev, RTC_CFG);
	val &= ~RTC_CFG_ENRTCTIME_Msk;
	rtc_write_offset(c2h_dev, RTC_CFG, val);

	for (i = 0; i < TIMER_DELAY_COUNT; i++) {
		if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_ENRTCTIMESTS_Msk))
			break;
		k_busy_wait(1);
	}

	if (i == TIMER_DELAY_COUNT) {
		LOG_ERR("%s Unable to disable RTC timer", __func__);
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/*
	 * The first bit in most alarm regs is used as enabled/disabled flag.
	 * The mask will clean it and also the unused bits
	 */
	if ((mask & RTC_ALARM_TIME_MASK_SECOND) != 0) {
		rtc_write_offset(c2h_dev, RTC_SEC_ALARM, (RTC_SECONDALARM_AENS_Msk |
					bin2bcd(timeptr->tm_sec)));
		val = rtc_read_offset(c2h_dev, RTC_SEC_ALARM);
		val &= ~RTC_SECONDALARM_AENS_Msk;
		rtc_write_offset(c2h_dev, RTC_SEC_ALARM, val);
		LOG_DBG("A SEC %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_SEC_ALARM)));
	} else {
		/* alarm disabled */
		if (!(rtc_read_offset(c2h_dev, RTC_SEC_ALARM) & RTC_SECONDALARM_AENS_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_SEC_ALARM);
			val |= RTC_SECONDALARM_AENS_Msk;
			rtc_write_offset(c2h_dev, RTC_SEC_ALARM, val);
		}
	}

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0) {
		rtc_write_offset(c2h_dev, RTC_MIN_ALARM, (RTC_MINUTALARM_AENM_Msk |
					bin2bcd(timeptr->tm_min)));
		val = rtc_read_offset(c2h_dev, RTC_MIN_ALARM);
		val &= ~RTC_MINUTALARM_AENM_Msk;
		rtc_write_offset(c2h_dev, RTC_MIN_ALARM, val);
		LOG_DBG("A MIN %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_MIN_ALARM)));
	} else {
		if (!(rtc_read_offset(c2h_dev, RTC_MIN_ALARM) & RTC_MINUTALARM_AENM_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_MIN_ALARM);
			val |= RTC_MINUTALARM_AENM_Msk;
			rtc_write_offset(c2h_dev, RTC_MIN_ALARM, val);
		}
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0) {
		val = rtc_read_offset(c2h_dev, RTC_CTS);
		val |= RTC_CTS_AENH_Msk;
		rtc_write_offset(c2h_dev, RTC_CTS, val);
		rtc_write_offset(c2h_dev, RTC_HOUR_ALARM, bin2bcd(timeptr->tm_hour));
		val = rtc_read_offset(c2h_dev, RTC_CTS);
		val &= ~RTC_CTS_AENH_Msk;
		rtc_write_offset(c2h_dev, RTC_CTS, val);
		LOG_DBG("A HOUR %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_HOUR_ALARM)));
	} else {
		if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_AENH_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_CTS);
			val |= RTC_CTS_AENH_Msk;
			rtc_write_offset(c2h_dev, RTC_CTS, val);
		}
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0) {
		rtc_write_offset(c2h_dev, RTC_DAY_ALARM, (RTC_DAYALARM_AEND_Msk |
					bin2bcd(timeptr->tm_mday)));
		val = rtc_read_offset(c2h_dev, RTC_DAY_ALARM);
		val &= ~RTC_DAYALARM_AEND_Msk;
		rtc_write_offset(c2h_dev, RTC_DAY_ALARM, val);
		LOG_DBG("A DAY %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_DAY_ALARM)));
	} else {
		if (!(rtc_read_offset(c2h_dev, RTC_DAY_ALARM) & RTC_DAYALARM_AEND_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_DAY_ALARM);
			val |= RTC_DAYALARM_AEND_Msk;
			rtc_write_offset(c2h_dev, RTC_DAY_ALARM, val);
		}
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTH) != 0) {
		rtc_write_offset(c2h_dev, RTC_MONTH_ALARM, (RTC_MONTHALARM_AENMON_Msk |
					bin2bcd(timeptr->tm_mon + 1)));
		val = rtc_read_offset(c2h_dev, RTC_MONTH_ALARM);
		val &= ~RTC_MONTHALARM_AENMON_Msk;
		rtc_write_offset(c2h_dev, RTC_MONTH_ALARM, val);
		LOG_DBG("A MONTH %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_MONTH_ALARM)));
	} else {
		if (!(rtc_read_offset(c2h_dev, RTC_MONTH_ALARM) & RTC_MONTHALARM_AENMON_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_MONTH_ALARM);
			val |= RTC_MONTHALARM_AENMON_Msk;
			rtc_write_offset(c2h_dev, RTC_MONTH_ALARM, val);
		}
	}

	if ((mask & RTC_ALARM_TIME_MASK_YEAR) != 0) {
		val = rtc_read_offset(c2h_dev, RTC_CTS);
		val |= RTC_CTS_AENY_Msk;
		rtc_write_offset(c2h_dev, RTC_CTS, val);
		rtc_write_offset(c2h_dev, RTC_YEAR_ALARM, bin2bcd(timeptr->tm_year -
					NPCM_YEAR_GAP));
		val = rtc_read_offset(c2h_dev, RTC_CTS);
		val &= ~RTC_CTS_AENY_Msk;
		rtc_write_offset(c2h_dev, RTC_CTS, val);
		LOG_DBG("A YEAR %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_YEAR_ALARM)));
	} else {
		if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_AENY_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_CTS);
			val |= RTC_CTS_AENY_Msk;
			rtc_write_offset(c2h_dev, RTC_CTS, val);
		}
	}

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) != 0) {
		rtc_write_offset(c2h_dev, RTC_WEEKDAY_ALARM, (RTC_WEEKDAYALARM_AENW_Msk |
					(timeptr->tm_wday + 1)));
		val = rtc_read_offset(c2h_dev, RTC_WEEKDAY_ALARM);
		val &= ~RTC_WEEKDAYALARM_AENW_Msk;
		rtc_write_offset(c2h_dev, RTC_WEEKDAY_ALARM, val);
		LOG_DBG("A WDAY %d", bcd2bin(rtc_read_offset(c2h_dev, RTC_WEEKDAY_ALARM)));
	} else {
		if (!(rtc_read_offset(c2h_dev, RTC_WEEKDAY_ALARM) & RTC_WEEKDAYALARM_AENW_Msk)) {
			val = rtc_read_offset(c2h_dev, RTC_WEEKDAY_ALARM);
			val |= RTC_WEEKDAYALARM_AENW_Msk;
			rtc_write_offset(c2h_dev, RTC_WEEKDAY_ALARM, val);
		}
	}

	val = rtc_read_offset(c2h_dev, RTC_CTL);
	val |= RTC_CTRL_AIE_Msk;
	rtc_write_offset(c2h_dev, RTC_CTL, val);

	val = rtc_read_offset(c2h_dev, RTC_CFG);
	val |= RTC_CFG_ENRTCTIME_Msk;
	rtc_write_offset(c2h_dev, RTC_CFG, val);

	LOG_DBG("A CTL 0x%x CFG 0x%x", rtc_read_offset(c2h_dev, RTC_CTL),
			rtc_read_offset(c2h_dev, RTC_CFG));

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_npcm_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	struct rtc_npcm_data *data = dev->data;
	const struct rtc_npcm_config *cfg = dev->config;
	const struct device *c2h_dev = cfg->c2h_dev;

	if ((id != 0) || (mask == NULL) || (timeptr == NULL)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!(rtc_read_offset(c2h_dev, RTC_SEC_ALARM) & RTC_SECONDALARM_AENS_Msk)) {
		timeptr->tm_sec = bcd2bin((rtc_read_offset(c2h_dev, RTC_SEC_ALARM) &
					RTC_SECONDALARM_Msk));
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if (!(rtc_read_offset(c2h_dev, RTC_MIN_ALARM) & RTC_MINUTALARM_AENM_Msk)) {
		timeptr->tm_min = bcd2bin((rtc_read_offset(c2h_dev, RTC_MIN_ALARM) &
					RTC_MINUTALARM_MINUTALARM_Msk));
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_AENH_Msk)) {
		timeptr->tm_hour = bcd2bin((rtc_read_offset(c2h_dev, RTC_HOUR_ALARM) &
					RTC_HOURALARM_HOURALARM_Msk));
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (!(rtc_read_offset(c2h_dev, RTC_DAY_ALARM) & RTC_DAYALARM_AEND_Msk)) {
		timeptr->tm_mday = bcd2bin((rtc_read_offset(c2h_dev, RTC_DAY_ALARM) &
					RTC_DAYALARM_Msk));
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if (!(rtc_read_offset(c2h_dev, RTC_MONTH_ALARM) & RTC_MONTHALARM_AENMON_Msk)) {
		timeptr->tm_mon = bcd2bin((rtc_read_offset(c2h_dev, RTC_MONTH_ALARM) &
					RTC_MONTHALARM_Msk)) - 1;
		*mask |= RTC_ALARM_TIME_MASK_MONTH;
	}

	if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_AENY_Msk)) {
		timeptr->tm_year = bcd2bin(rtc_read_offset(c2h_dev, RTC_YEAR_ALARM)) +
			NPCM_YEAR_GAP;
		*mask |= RTC_ALARM_TIME_MASK_YEAR;
	}

	if (!(rtc_read_offset(c2h_dev, RTC_WEEKDAY_ALARM) & RTC_WEEKDAYALARM_AENW_Msk)) {
		timeptr->tm_wday = (rtc_read_offset(c2h_dev, RTC_WEEKDAY_ALARM) &
					RTC_WEEKDAYALARM_WEEKALARM_Msk) - 1;
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}


static int rtc_npcm_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_npcm_data *data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	ret = data->alarm_pending ? 1 : 0;
	data->alarm_pending = false;

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int rtc_npcm_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	struct rtc_npcm_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;


	k_spin_unlock(&data->lock, key);

	return 0;
}

#endif /* CONFIG_RTC_ALARM */

static const struct rtc_driver_api rtc_npcm_driver_api = {
	.set_time = rtc_npcm_set_time,
	.get_time = rtc_npcm_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_npcm_alarm_get_supported_fields,
	.alarm_set_time = rtc_npcm_alarm_set_time,
	.alarm_get_time = rtc_npcm_alarm_get_time,
	.alarm_is_pending = rtc_npcm_alarm_is_pending,
	.alarm_set_callback = rtc_npcm_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

static void rtc_npcm_isr(const struct device *dev, struct npcm_wui *wui)
{
	const struct rtc_npcm_config *cfg = dev->config;
	const struct device *c2h_dev = cfg->c2h_dev;

#ifdef CONFIG_RTC_ALARM
	struct rtc_npcm_data *const data = dev->data;
#endif
	ARG_UNUSED(wui);

#ifdef CONFIG_RTC_ALARM
	if (rtc_read_offset(c2h_dev, RTC_ALMFLG) & RTC_ALMFLG_AF_Msk) {
		rtc_alarm_callback callback;
		void *user_data;

		/* Clear rtc alarm interrupt flag*/
		rtc_write_offset(c2h_dev, RTC_ALMFLG, RTC_ALMFLG_AF_Msk);

		callback = data->alarm_callback;
		user_data = data->alarm_user_data;
		data->alarm_pending = callback ? false : true;

		if (callback != NULL) {
			callback(dev, 0, user_data);
		}
	}
#endif
}

static int rtc_npcm_init(const struct device *dev)
{
	struct rtc_npcm_config *cfg = (struct rtc_npcm_config *)dev->config;
	struct c2h_reg *const inst_c2h = cfg->inst_c2h;
	const struct device *c2h_dev = cfg->c2h_dev;
	uint8_t i, val;

	/* Enable Core-to-Host access module */
	inst_c2h->SIBCTRL |= BIT(NPCM_SIBCTRL_CSAE);
	rtc_write_offset(c2h_dev, RTC_CFG, 0x0);
	val = rtc_read_offset(c2h_dev, RTC_CFG);
	val |= RTC_CFG_ENRTCPAD_Msk;
	rtc_write_offset(c2h_dev, RTC_CFG, val);
	for (i = 0; i < DELAY_COUNT; i++) {
		if ((rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_PADSTS_Msk) &&
				(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_RTCPAD05STS_Msk))
			break;
		k_busy_wait(1000);
	}

	if (i == DELAY_COUNT) {
		LOG_ERR("%s Unable to enable RTC PAD", __func__);
		return -1;
	}

	val = rtc_read_offset(c2h_dev, RTC_CFG);
	val &= ~RTC_CFG_ENRTCTIME_Msk;
	rtc_write_offset(c2h_dev, RTC_CFG, val);

	for (i = 0; i < TIMER_DELAY_COUNT; i++) {
		if (!(rtc_read_offset(c2h_dev, RTC_CTS) & RTC_CTS_ENRTCTIMESTS_Msk))
			break;
		k_busy_wait(1);
	}

	if (i == TIMER_DELAY_COUNT) {
		LOG_ERR("%s Unable to disable RTC timer", __func__);
		return -EINVAL;
	}

	/* Initialize a miwu device input and its callback function */
	npcm_miwu_init_dev_callback(&rtc_miwu_cb, &cfg->rtcwk, rtc_npcm_isr,
			dev);

	npcm_miwu_manage_callback(&rtc_miwu_cb, true);

	/*
	 * Configure the rtc wake-up event
	 */
	npcm_miwu_interrupt_configure(&cfg->rtcwk,
			NPCM_MIWU_MODE_EDGE, NPCM_MIWU_TRIG_HIGH);

	npcm_miwu_irq_enable(&cfg->rtcwk);

	return 0;
}

/* RTC driver registration */

#define NPCM_RTC_INIT(inst)						                        \
									                        \
	static const struct rtc_npcm_config rtc_npcm_config_##inst = {	                        \
		.inst_c2h = (struct c2h_reg *)DT_REG_ADDR(DT_NODELABEL(c2h)),			\
		.rtcwk = NPCM_DT_WUI_ITEM_BY_NAME(inst, rtc_wk),				\
		.c2h_dev = DEVICE_DT_GET(DT_NODELABEL(c2h))					\
	};								                        \
									                        \
	static struct rtc_npcm_data rtc_npcm_data_##inst;                                       \
												\
	DEVICE_DT_INST_DEFINE(inst,					                        \
			      rtc_npcm_init,							\
			      NULL,					                        \
			      &rtc_npcm_data_##inst, &rtc_npcm_config_##inst,			\
			      PRE_KERNEL_2,				                        \
			      CONFIG_RTC_INIT_PRIORITY, &rtc_npcm_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(NPCM_RTC_INIT)
