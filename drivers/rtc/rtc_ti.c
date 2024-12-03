/*
 * Copyright (c) 2024 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT ti_rtc

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include "rtc_utils.h"

#include <ti/driverlib/dl_rtc_common.h>

LOG_MODULE_REGISTER(rtc_ti, CONFIG_RTC_LOG_LEVEL);

#define RTC_TI_ALARM_1	1
#define RTC_TI_ALARM_2	2

#define RTC_TI_ALARM_1_MASK	BIT(2)
#define RTC_TI_ALARM_2_MASK	BIT(3)

struct rtc_ti_config {
	RTC_Regs *base;
};

#ifdef CONFIG_RTC_ALARM
struct rtc_ti_alarm {
	uint16_t mask;
	rtc_alarm_callback callback;
	void *user_data;
};
#endif

struct rtc_ti_data {
	struct k_mutex lock;
#ifdef CONFIG_RTC_ALARM
	struct rtc_ti_alarm rtc_alarm_1;
	struct rtc_ti_alarm rtc_alarm_2;
#endif
};

static int rtc_ti_set_time(const struct device *dev,
			   const struct rtc_time *timeptr)
{
	int ret;
	const struct rtc_ti_config *cfg = dev->config;
	struct rtc_ti_data *data = dev->data;

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, 0)) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	DL_RTC_Common_setCalendarSecondsBinary(cfg->base, timeptr->tm_sec);
	DL_RTC_Common_setCalendarMinutesBinary(cfg->base, timeptr->tm_min);
	DL_RTC_Common_setCalendarHoursBinary(cfg->base, timeptr->tm_hour);
	DL_RTC_Common_setCalendarDayOfWeekBinary(cfg->base, timeptr->tm_wday);
	DL_RTC_Common_setCalendarDayOfMonthBinary(cfg->base, timeptr->tm_mday);
	DL_RTC_Common_setCalendarMonthBinary(cfg->base, timeptr->tm_mon);
	DL_RTC_Common_setCalendarYearBinary(cfg->base, timeptr->tm_year);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_ti_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	int ret;
	const struct rtc_ti_config *cfg = dev->config;
	struct rtc_ti_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	timeptr->tm_sec  = DL_RTC_Common_getCalendarSecondsBinary(cfg->base);
	timeptr->tm_min  = DL_RTC_Common_getCalendarMinutesBinary(cfg->base);
	timeptr->tm_hour = DL_RTC_Common_getCalendarHoursBinary(cfg->base);
	timeptr->tm_mday = DL_RTC_Common_getCalendarDayOfMonthBinary(cfg->base);
	timeptr->tm_mon  = DL_RTC_Common_getCalendarMonthBinary(cfg->base);
	timeptr->tm_year = DL_RTC_Common_getCalendarYearBinary(cfg->base);
	timeptr->tm_wday = DL_RTC_Common_getCalendarDayOfWeekBinary(cfg->base);

	k_mutex_unlock(&data->lock);

	return 0;
}

#if defined CONFIG_RTC_ALARM
static int rtc_ti_alarm_get_supported_fields(const struct device *dev,
					      uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	if (mask == NULL) {
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
		 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTHDAY);

	return 0;
}

static inline void rtc_ti_set_alarm1(const struct device *dev, uint16_t mask,
				    const struct rtc_time *timeptr)
{
	const struct rtc_ti_config *cfg = dev->config;

	WRITE_BIT(cfg->base->CPU_INT.IMASK, 2, 0);
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		cfg->base->A1MIN = 0;
		DL_RTC_Common_setAlarm1MinutesBinary(cfg->base, timeptr->tm_min);
		DL_RTC_Common_enableAlarm1MinutesBinary(cfg->base);
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		DL_RTC_Common_setAlarm1HoursBinary(cfg->base, timeptr->tm_hour);
		DL_RTC_Common_enableAlarm1HoursBinary(cfg->base);
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		DL_RTC_Common_setAlarm1DayOfWeekBinary(cfg->base, timeptr->tm_wday);
		DL_RTC_Common_enableAlarm1DayOfWeekBinary(cfg->base);
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		DL_RTC_Common_setAlarm1DayOfMonthBinary(cfg->base, timeptr->tm_mday);
		DL_RTC_Common_enableAlarm1DayOfMonthBinary(cfg->base);
	}

	WRITE_BIT(cfg->base->CPU_INT.IMASK, 2, 1);
}


static inline void rtc_ti_set_alarm2(const struct device *dev, uint16_t mask,
				    const struct rtc_time *timeptr)
{
	const struct rtc_ti_config *cfg = dev->config;

	WRITE_BIT(cfg->base->CPU_INT.IMASK, 3, 0);
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		cfg->base->A2MIN = 0;
		DL_RTC_Common_setAlarm2MinutesBinary(cfg->base, timeptr->tm_min);
		DL_RTC_Common_enableAlarm2MinutesBinary(cfg->base);
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		DL_RTC_Common_setAlarm2HoursBinary(cfg->base, timeptr->tm_hour);
		DL_RTC_Common_enableAlarm2HoursBinary(cfg->base);
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		DL_RTC_Common_setAlarm2DayOfWeekBinary(cfg->base, timeptr->tm_wday);
		DL_RTC_Common_enableAlarm2DayOfWeekBinary(cfg->base);
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		DL_RTC_Common_setAlarm2DayOfMonthBinary(cfg->base, timeptr->tm_mday);
		DL_RTC_Common_enableAlarm2DayOfMonthBinary(cfg->base);
	}

	WRITE_BIT(cfg->base->CPU_INT.IMASK, 3, 1);
}

static inline void rtc_ti_clear_alarm(const struct device *dev, uint16_t id)
{
	const struct rtc_ti_config *cfg = dev->config;

	if (id == RTC_TI_ALARM_1) {
		cfg->base->CPU_INT.ICLR |= RTC_TI_ALARM_1_MASK;
		cfg->base->A1MIN = 0x00;
		cfg->base->A1HOUR = 0x00;
		cfg->base->A1DAY = 0x00;
	} else if (id == RTC_TI_ALARM_2) {
		cfg->base->CPU_INT.ICLR |= RTC_TI_ALARM_2_MASK;
		cfg->base->A2MIN = 0x00;
		cfg->base->A2HOUR = 0x00;
		cfg->base->A2DAY = 0x00;
	}
}

static int rtc_ti_alarm_set_time(const struct device *dev, uint16_t id,
				 uint16_t mask, const struct rtc_time *timeptr)
{
	int ret;
	struct rtc_ti_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	rtc_ti_clear_alarm(dev, id);
	switch (id) {
	case RTC_TI_ALARM_1:
		rtc_ti_set_alarm1(dev, mask, timeptr);
		data->rtc_alarm_1.mask = mask;
		break;
	case RTC_TI_ALARM_2:
		rtc_ti_set_alarm2(dev, mask, timeptr);
		data->rtc_alarm_2.mask = mask;
		break;
	default:
		ret = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int rtc_ti_get_alarm1(const struct device *dev, struct rtc_time *timeptr)
{
	uint16_t return_mask = 0;
	uint16_t alarm_mask = 0;
	const struct rtc_ti_config *cfg = dev->config;
	struct rtc_ti_data *data = dev->data;

	alarm_mask = data->rtc_alarm_1.mask;
	if (alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = DL_RTC_Common_getAlarm1MinutesBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = DL_RTC_Common_getAlarm1HoursBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (alarm_mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		timeptr->tm_wday = DL_RTC_Common_getAlarm1DayOfWeekBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	if (alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday =  DL_RTC_Common_getAlarm1DayOfMonthBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	return return_mask;
}

static int rtc_ti_get_alarm2(const struct device *dev, struct rtc_time *timeptr)
{
	uint16_t return_mask = 0;
	uint16_t alarm_mask = 0;
	const struct rtc_ti_config *cfg = dev->config;
	struct rtc_ti_data *data = dev->data;

	alarm_mask = data->rtc_alarm_2.mask;
	if (alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = DL_RTC_Common_getAlarm2MinutesBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = DL_RTC_Common_getAlarm2HoursBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (alarm_mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		timeptr->tm_wday = DL_RTC_Common_getAlarm2DayOfWeekBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	if (alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday =  DL_RTC_Common_getAlarm2DayOfMonthBinary(cfg->base);
		return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	return return_mask;
}

static int rtc_ti_alarm_get_time(const struct device *dev, uint16_t id,
				 uint16_t *mask, struct rtc_time *timeptr)
{
	int ret;
	struct rtc_ti_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	switch (id) {
	case RTC_TI_ALARM_1:
		*mask = rtc_ti_get_alarm1(dev, timeptr);
		break;
	case RTC_TI_ALARM_2:
		*mask = rtc_ti_get_alarm2(dev, timeptr);
		break;
	default:
		ret = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int rtc_ti_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
	int ret;
	struct rtc_ti_data *data = dev->data;

	if (callback == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	switch (id) {
	case RTC_TI_ALARM_1:
		data->rtc_alarm_1.callback = callback;
		data->rtc_alarm_1.user_data = user_data;
		break;
	case RTC_TI_ALARM_2:
		data->rtc_alarm_2.callback = callback;
		data->rtc_alarm_2.user_data = user_data;
		break;
	default:
		ret = -EINVAL;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int rtc_ti_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int ret;
	const struct rtc_ti_config *cfg = dev->config;
	struct rtc_ti_data *data = dev->data;

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	ret = DL_RTC_Common_getEnabledInterrupts(cfg->base,
		id == RTC_TI_ALARM_1 ? RTC_TI_ALARM_1_MASK : RTC_TI_ALARM_2_MASK);

	k_mutex_unlock(&data->lock);

	return ret;
}
#endif

static void rtc_ti_isr(const struct device *dev)
{
#if CONFIG_RTC_ALARM
	uint8_t id = 0;
	uint32_t key = irq_lock();
	const struct rtc_ti_config *cfg = dev->config;
	struct rtc_ti_data *data = dev->data;
	struct rtc_ti_alarm *alarm = NULL;

	if (DL_RTC_Common_getEnabledInterruptStatus(cfg->base, RTC_TI_ALARM_1_MASK)) {
		cfg->base->CPU_INT.ICLR = RTC_TI_ALARM_1_MASK;
		alarm = &data->rtc_alarm_1;
		id = RTC_TI_ALARM_1;
	} else if (DL_RTC_Common_getEnabledInterruptStatus(cfg->base, RTC_TI_ALARM_2_MASK)) {
		cfg->base->CPU_INT.ICLR = RTC_TI_ALARM_2_MASK;
		alarm = &data->rtc_alarm_2;
		id = RTC_TI_ALARM_2;
	} else {
		irq_unlock(key);
		return;
	}

	if (alarm->callback) {
		alarm->callback(dev, id, alarm->user_data);
	}

	irq_unlock(key);
#endif
}

#ifdef CONFIG_RTC_ALARM
static inline void rtc_ti_irq_config(const struct device *dev)
{
	irq_disable(DT_INST_IRQN(0));
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    rtc_ti_isr, DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
}
#endif

static int rtc_ti_init(const struct device *dev)
{
	const struct rtc_ti_config *cfg = dev->config;

	/* Enable power to RTC module */
	if (!DL_RTC_Common_isPowerEnabled(cfg->base)) {
		DL_RTC_Common_enablePower(cfg->base);
	}

	DL_RTC_Common_enableClockControl(cfg->base);
	DL_RTC_Common_setClockFormat(cfg->base, DL_RTC_COMMON_FORMAT_BINARY);

#ifdef CONFIG_RTC_ALARM
	rtc_ti_irq_config(dev);
#endif

	return 0;
}

static const struct rtc_driver_api rtc_ti_driver_api = {
	.set_time = rtc_ti_set_time,
	.get_time = rtc_ti_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_ti_alarm_get_supported_fields,
	.alarm_set_time = rtc_ti_alarm_set_time,
	.alarm_get_time = rtc_ti_alarm_get_time,
	.alarm_is_pending = rtc_ti_alarm_is_pending,
	.alarm_set_callback = rtc_ti_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

static struct rtc_ti_data rtc_data;
static struct rtc_ti_config rtc_config = {
	.base = (RTC_Regs *)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, &rtc_ti_init, NULL, &rtc_data, &rtc_config, PRE_KERNEL_1,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_ti_driver_api);
