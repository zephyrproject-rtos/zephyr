/*
 * Copyright (c) 2024 ANITRA system s.r.o.
 * Copyright (c) 2026 Janez Ugovsek <janez@ugovsek.info>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv3028_rtc

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <rv3028.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "rtc_utils.h"

#define RV3028_REG_EVENT_CONTROL        0x13
LOG_MODULE_REGISTER(rtc_rv3028, CONFIG_RTC_LOG_LEVEL);

/* Convert part per billion calibration value to a number of clock pulses added or removed each
 * 2^20 clock cycles so it is suitable for the EEOffset register field
 *
 * nb_pulses = ppb * 2^20 / 10^9 = ppb * 2^11 / 5^9 = ppb * 2048 / 1953125
 */
#define PPB_TO_NB_PULSES(ppb) DIV_ROUND_CLOSEST((ppb) * 2048, 1953125)

/* Convert EEOffset register value (number of clock pulses added or removed each 2^20 clock cycles)
 * to part per billion calibration value
 *
 * ppb = nb_pulses * 10^9 / 2^20 = nb_pulses * 5^9 / 2^11 = nb_pulses * 1953125 / 2048
 */
#define NB_PULSES_TO_PPB(pulses) DIV_ROUND_CLOSEST((pulses) * 1953125, 2048)

#define MAX_PPB NB_PULSES_TO_PPB(255)
#define MIN_PPB NB_PULSES_TO_PPB(-256)

/* RTC alarm time fields supported by the RV3028 */
#define RV3028_RTC_ALARM_TIME_MASK                                                                 \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY)

/* RTC time fields supported by the RV3028 */
#define RV3028_RTC_TIME_MASK                                                                       \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct rv3028_config {
	const struct device *mfd;
};

struct rv3028_data {
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif /* CONFIG_RTC_UPDATE */
};

static int rv3028_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rv3028_config *config = dev->config;
	uint8_t date[7];
	int err;

	if ((timeptr == NULL) || (!rtc_utils_validate_rtc_time(timeptr, RV3028_RTC_TIME_MASK)) ||
	    (timeptr->tm_year < RV3028_YEAR_OFFSET)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	LOG_DBG("set time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	date[0] = bin2bcd(timeptr->tm_sec) & RV3028_SECONDS_MASK;
	date[1] = bin2bcd(timeptr->tm_min) & RV3028_MINUTES_MASK;
	date[2] = bin2bcd(timeptr->tm_hour) & RV3028_HOURS_24H_MASK;
	date[3] = timeptr->tm_wday & RV3028_WEEKDAY_MASK;
	date[4] = bin2bcd(timeptr->tm_mday) & RV3028_DATE_MASK;
	date[5] = bin2bcd(timeptr->tm_mon + RV3028_MONTH_OFFSET) & RV3028_MONTH_MASK;
	date[6] = bin2bcd(timeptr->tm_year - RV3028_YEAR_OFFSET) & RV3028_YEAR_MASK;

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_write_regs(config->mfd, RV3028_REG_SECONDS, &date, sizeof(date));
	if (err) {
		goto unlock;
	}

	/* Clear Power On Reset Flag */
	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_STATUS, RV3028_STATUS_PORF, 0);

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

static int rv3028_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rv3028_config *config = dev->config;
	uint8_t status;
	uint8_t date[7];
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	mfd_rv3028_lock_sem(config->mfd);
	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if (status & RV3028_STATUS_PORF) {
		/* Power On Reset Flag indicates invalid data */
		err = -ENODATA;
		goto unlock;
	}

	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_SECONDS, date, sizeof(date));
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);
	if (err) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	timeptr->tm_sec = bcd2bin(date[0] & RV3028_SECONDS_MASK);
	timeptr->tm_min = bcd2bin(date[1] & RV3028_MINUTES_MASK);
	timeptr->tm_hour = bcd2bin(date[2] & RV3028_HOURS_24H_MASK);
	timeptr->tm_wday = date[3] & RV3028_WEEKDAY_MASK;
	timeptr->tm_mday = bcd2bin(date[4] & RV3028_DATE_MASK);
	timeptr->tm_mon = bcd2bin(date[5] & RV3028_MONTH_MASK) - RV3028_MONTH_OFFSET;
	timeptr->tm_year = bcd2bin(date[6] & RV3028_YEAR_MASK) + RV3028_YEAR_OFFSET;
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static void rv3028_rtc_alarm_isr(const struct device *dev)
{
	struct rv3028_data *data = dev->data;

	if (data->alarm_callback != NULL) {
		data->alarm_callback(dev, 0U, data->alarm_user_data);
	}
}

static int rv3028_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	*mask = RV3028_RTC_ALARM_TIME_MASK;

	return 0;
}

static int rv3028_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr)
{
	const struct rv3028_config *config = dev->config;
	uint8_t regs[3];

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	if (mask & ~(RV3028_RTC_ALARM_TIME_MASK)) {
		LOG_ERR("unsupported alarm field mask 0x%04x", mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("invalid alarm time");
		return -EINVAL;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[0] = bin2bcd(timeptr->tm_min) & RV3028_ALARM_MINUTES_MASK;
	} else {
		regs[0] = RV3028_ALARM_MINUTES_AE_M;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[1] = bin2bcd(timeptr->tm_hour) & RV3028_ALARM_HOURS_24H_MASK;
	} else {
		regs[1] = RV3028_ALARM_HOURS_AE_H;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = bin2bcd(timeptr->tm_mday) & RV3028_ALARM_DATE_MASK;
	} else {
		regs[2] = RV3028_ALARM_DATE_AE_WD;
	}

	LOG_DBG("set alarm: mday = %d, hour = %d, min = %d, mask = 0x%04x", timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, mask);

	/* Write registers RV3028_REG_ALARM_MINUTES through RV3028_REG_ALARM_WEEKDAY */
	return mfd_rv3028_write_regs(config->mfd, RV3028_REG_ALARM_MINUTES, &regs, sizeof(regs));
}

static int rv3028_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr)
{
	const struct rv3028_config *config = dev->config;
	uint8_t regs[3];
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	/* Read registers RV3028_REG_ALARM_MINUTES through RV3028_REG_ALARM_WEEKDAY */
	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_ALARM_MINUTES, &regs, sizeof(regs));
	if (err) {
		return err;
	}

	memset(timeptr, 0U, sizeof(*timeptr));
	*mask = 0U;

	if ((regs[0] & RV3028_ALARM_MINUTES_AE_M) == 0) {
		timeptr->tm_min = bcd2bin(regs[0] & RV3028_ALARM_MINUTES_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((regs[1] & RV3028_ALARM_HOURS_AE_H) == 0) {
		timeptr->tm_hour = bcd2bin(regs[1] & RV3028_ALARM_HOURS_24H_MASK);
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((regs[2] & RV3028_ALARM_DATE_AE_WD) == 0) {
		timeptr->tm_mday = bcd2bin(regs[2] & RV3028_ALARM_DATE_MASK);
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	LOG_DBG("get alarm: mday = %d, hour = %d, min = %d, mask = 0x%04x", timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, *mask);

	return 0;
}

static int rv3028_alarm_is_pending(const struct device *dev, uint16_t id)
{
	const struct rv3028_config *config = dev->config;
	uint8_t status;
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	mfd_rv3028_lock_sem(config->mfd);

	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_STATUS, &status);
	if (err) {
		goto unlock;
	}

	if (status & RV3028_STATUS_AF) {
		/* Clear alarm flag */
		status &= ~(RV3028_STATUS_AF);

		err = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_STATUS, status);
		if (err) {
			goto unlock;
		}

		/* Alarm pending */
		err = 1;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

static int rv3028_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
	const struct rv3028_config *config = dev->config;
	struct rv3028_data *data = dev->data;
	int err;

	if (id != 0U) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	mfd_rv3028_lock_sem(config->mfd);

	mfd_rv3028_set_irq_handler(config->mfd, dev, RV3028_DEV_RTC_ALARM, rv3028_rtc_alarm_isr);
	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL2, RV3028_CONTROL2_AIE,
				     callback != NULL ? RV3028_CONTROL2_AIE : 0);
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)

static void rv3028_rtc_update_isr(const struct device *dev)
{
	struct rv3028_data *data = dev->data;

	if (data->update_callback != NULL) {
		data->update_callback(dev, data->update_user_data);
	}
}

static int rv3028_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	const struct rv3028_config *config = dev->config;
	struct rv3028_data *data = dev->data;
	int err;
	uint8_t val = 0;

	mfd_rv3028_lock_sem(config->mfd);

	mfd_rv3028_set_irq_handler(config->mfd, dev, RV3028_DEV_RTC_UPDATE, rv3028_rtc_update_isr);
	data->update_callback = callback;
	data->update_user_data = user_data;

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL1, RV3028_CONTROL1_USEL, val);
	if (err) {
		goto unlock;
	}

	val = 0;
	if (callback != NULL) {
		val |= RV3028_CONTROL2_UIE;
	}

	err = mfd_rv3028_update_reg8(config->mfd, RV3028_REG_CONTROL2, RV3028_CONTROL2_UIE, val);
	if (err) {
		goto unlock;
	}

unlock:
	mfd_rv3028_unlock_sem(config->mfd);

	return err;
}

#endif /* defined(CONFIG_RTC_UPDATE) */

#ifdef CONFIG_RTC_CALIBRATION
static int rv3028_set_calibration(const struct device *dev, int32_t freq_ppb)
{
	const struct rv3028_config *config = dev->config;
	int err;
	int32_t nb_pulses;
	uint16_t offset;
	uint8_t val_backup;

	if ((freq_ppb > MAX_PPB) || (freq_ppb < MIN_PPB)) {
		/* out of supported range */
		return -EINVAL;
	}

	nb_pulses = PPB_TO_NB_PULSES(freq_ppb);
	offset = nb_pulses & 0x1FF;

	LOG_DBG("Set calibration: frequency ppb: %d, offset value: %d", NB_PULSES_TO_PPB(nb_pulses),
		offset);

	/* Refresh the settings in the RAM with the settings from the EEPROM */
	err = mfd_rv3028_enter_eerd(config->mfd);
	if (err) {
		return -ENODEV;
	}
	err = mfd_rv3028_refresh(config->mfd);
	if (err) {
		mfd_rv3028_exit_eerd(config->mfd);
		return err;
	}

	err = mfd_rv3028_read_reg8(config->mfd, RV3028_REG_BACKUP, &val_backup);
	if (err) {
		mfd_rv3028_exit_eerd(config->mfd);
		return err;
	}

	/* LSB of offset is stored in BACKUP register */
	val_backup &= ~BIT(RV3028_BACKUP_OFFSET_BIT_INDEX);
	val_backup |= (offset & 0x01) << RV3028_BACKUP_OFFSET_BIT_INDEX;

	err = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_BACKUP, val_backup);
	if (err) {
		mfd_rv3028_exit_eerd(config->mfd);
		return err;
	}

	err = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_OFFSET, offset >> 1);
	if (err) {
		mfd_rv3028_exit_eerd(config->mfd);
		return err;
	}

	return mfd_rv3028_update(config->mfd);
}

static int rv3028_get_calibration(const struct device *dev, int32_t *freq_ppb)
{
	const struct rv3028_config *config = dev->config;
	uint8_t regs[2];
	int err;
	uint16_t offset;
	int32_t nb_pulses;

	/* Read OFFSET and BACKUP register */
	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_OFFSET, regs, sizeof(regs));
	if (err) {
		return err;
	}

	/* LSB of offset is stored in BACKUP register */
	offset = (regs[0] << 1) | ((regs[1] & BIT(RV3028_BACKUP_OFFSET_BIT_INDEX)) >>
				   RV3028_BACKUP_OFFSET_BIT_INDEX);
	nb_pulses = sign_extend(offset, RV3028_OFFSET_SIGN_BIT_INDEX);
	*freq_ppb = NB_PULSES_TO_PPB(nb_pulses);

	LOG_DBG("Get calibration: frequency ppb: %d, offset value: %d", *freq_ppb, offset);

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

static int rv3028_init(const struct device *dev)
{
	const struct rv3028_config *config = dev->config;
	uint8_t regs[3];
	int err;

	err = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_CONTROL1, RV3028_CONTROL1_WADA);
	if (err) {
		return -ENODEV;
	}

	/* Disable interrupts and use 24 hour mode */
	err = mfd_rv3028_write_reg8(config->mfd, RV3028_REG_CONTROL2, 0);
	if (err) {
		return -ENODEV;
	}

	err = mfd_rv3028_read_regs(config->mfd, RV3028_REG_ALARM_MINUTES, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	regs[0] |= RV3028_ALARM_MINUTES_AE_M;
	regs[1] |= RV3028_ALARM_HOURS_AE_H;
	regs[2] |= RV3028_ALARM_DATE_AE_WD;

	err = mfd_rv3028_write_regs(config->mfd, RV3028_REG_ALARM_MINUTES, regs, sizeof(regs));
	if (err) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(rtc, rv3028_driver_api) = {
	.set_time = rv3028_set_time,
	.get_time = rv3028_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv3028_alarm_get_supported_fields,
	.alarm_set_time = rv3028_alarm_set_time,
	.alarm_get_time = rv3028_alarm_get_time,
	.alarm_is_pending = rv3028_alarm_is_pending,
	.alarm_set_callback = rv3028_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rv3028_update_set_callback,
#endif /* defined(CONFIG_RTC_UPDATE) */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rv3028_set_calibration,
	.get_calibration = rv3028_get_calibration
#endif /* CONFIG_RTC_CALIBRATION */
};

#define RV3028_INIT(inst)                                                                          \
	static const struct rv3028_config rv3028_config_##inst = {                                 \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
                                                                                                   \
	static struct rv3028_data rv3028_data_##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &rv3028_init, NULL, &rv3028_data_##inst,                       \
			      &rv3028_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &rv3028_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV3028_INIT)
