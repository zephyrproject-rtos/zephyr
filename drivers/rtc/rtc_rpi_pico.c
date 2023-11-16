/*
 * Copyright (c) 2024 Andrew Featherstone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>

#include <hardware/irq.h>
#include <hardware/rtc.h>
#include <hardware/regs/rtc.h>

#include "rtc_utils.h"

#define DT_DRV_COMPAT raspberrypi_pico_rtc

#define CLK_DRV DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0))
#define CLK_ID  (clock_control_subsys_t) DT_INST_PHA_BY_IDX(0, clocks, 0, clk_id)

/* struct tm start time:   1st, Jan, 1900 */
#define TM_YEAR_REF 1900
/* See section 4.8.1 of the RP2040 datasheet. */
#define RP2040_RTC_YEAR_MAX 4095
#ifdef CONFIG_RTC_ALARM
static int rtc_rpi_pico_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				       struct rtc_time *timeptr);
#endif
struct rtc_rpi_pico_data {
	struct k_spinlock lock;

#ifdef CONFIG_RTC_ALARM
	struct rtc_time alarm_time;
	uint16_t alarm_mask;
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	bool alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

static struct rtc_rpi_pico_data rtc_data;

LOG_MODULE_REGISTER(rtc_rpi, CONFIG_RTC_LOG_LEVEL);

#ifdef CONFIG_RTC_ALARM
static void rtc_rpi_isr(const struct device *dev)
{
	struct rtc_rpi_pico_data *data = dev->data;

	rtc_alarm_callback callback;
	void *user_data;

	rtc_disable_alarm();

	K_SPINLOCK(&data->lock) {
		callback = data->alarm_callback;
		user_data = data->alarm_user_data;
	}

	if (callback != NULL) {
		callback(dev, 0, user_data);
	} else {
		data->alarm_pending = true;
	}
	/* re-enable the alarm. */
	rtc_enable_alarm();
}
#endif

static int rtc_rpi_pico_init(const struct device *dev)
{
	int ret;
#ifdef CONFIG_RTC_ALARM
	struct rtc_rpi_pico_data *data = dev->data;
#endif

	ret = clock_control_on(CLK_DRV, CLK_ID);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_RTC_ALARM
	data->alarm_mask = 0;
	data->alarm_callback = NULL;
	data->alarm_pending = false;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rtc_rpi_isr, DEVICE_DT_INST_GET(0),
		    0);
	irq_enable(DT_INST_IRQN(0));
#endif
	rtc_init();
	return 0;
}

static int rtc_rpi_pico_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_rpi_pico_data *data = dev->data;
	int err = 0;

	if (timeptr->tm_year + TM_YEAR_REF > RP2040_RTC_YEAR_MAX) {
		return -EINVAL;
	}

	if (timeptr->tm_wday == -1) {
		/* day of the week is expected */
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	datetime_t dt = {
		.year = timeptr->tm_year + TM_YEAR_REF,
		.month = timeptr->tm_mon + 1,
		.day = timeptr->tm_mday,
		.dotw = timeptr->tm_wday,
		.hour = timeptr->tm_hour,
		.min = timeptr->tm_min,
		.sec = timeptr->tm_sec,
	};
	/* Use the validation in the Pico SDK. */
	if (!rtc_set_datetime(&dt)) {
		err = -EINVAL;
	}
	k_spin_unlock(&data->lock, key);

	return err;
}

static int rtc_rpi_pico_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_rpi_pico_data *data = dev->data;
	datetime_t dt;
	int err = 0;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (!rtc_get_datetime(&dt)) {
		err = -ENODATA;
	}

	timeptr->tm_sec = dt.sec;
	timeptr->tm_min = dt.min;
	timeptr->tm_hour = dt.hour;
	timeptr->tm_mday = dt.day;
	timeptr->tm_mon = dt.month - 1;
	timeptr->tm_year = dt.year - TM_YEAR_REF;
	timeptr->tm_wday = dt.dotw;
	/* unknown values */
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;
	k_spin_unlock(&data->lock, key);

	return err;
}

#if defined(CONFIG_RTC_ALARM)
static int rtc_rpi_pico_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						   uint16_t *supported_fields)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		return -EINVAL;
	}
	*supported_fields = RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
			    RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_WEEKDAY |
			    RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH |
			    RTC_ALARM_TIME_MASK_YEAR;

	return 0;
}

static int rtc_rpi_pico_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				       const struct rtc_time *alarm)
{
	struct rtc_rpi_pico_data *data = dev->data;
	int err = 0;
	uint16_t mask_available;

	(void)rtc_rpi_pico_alarm_get_supported_fields(NULL, 0, &mask_available);

	if (mask & ~mask_available) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(alarm, mask)) {
		return -EINVAL;
	}

	LOG_INF("Setting alarm");

	rtc_disable_alarm();
	if (mask == 0) {
		/* Disable the alarm */
		data->alarm_mask = 0;
	}
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Clear before updating. */
	rtc_hw->irq_setup_0 = 0;
	rtc_hw->irq_setup_1 = 0;

	/* Set the match enable bits for things we care about */
	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		hw_set_bits(&rtc_hw->irq_setup_0,
			    RTC_IRQ_SETUP_0_YEAR_ENA_BITS |
				    ((alarm->tm_year + TM_YEAR_REF) << RTC_IRQ_SETUP_0_YEAR_LSB));
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		hw_set_bits(&rtc_hw->irq_setup_0,
			    RTC_IRQ_SETUP_0_MONTH_ENA_BITS |
				    (alarm->tm_mon << RTC_IRQ_SETUP_0_MONTH_LSB));
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		hw_set_bits(&rtc_hw->irq_setup_0,
			    RTC_IRQ_SETUP_0_DAY_ENA_BITS |
				    ((alarm->tm_mday + 1) << RTC_IRQ_SETUP_0_DAY_LSB));
	}
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		hw_set_bits(&rtc_hw->irq_setup_1,
			    RTC_IRQ_SETUP_1_DOTW_ENA_BITS |
				    (alarm->tm_wday << RTC_IRQ_SETUP_1_DOTW_LSB));
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		hw_set_bits(&rtc_hw->irq_setup_1,
			    RTC_IRQ_SETUP_1_HOUR_ENA_BITS |
				    (alarm->tm_hour << RTC_IRQ_SETUP_1_HOUR_LSB));
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		hw_set_bits(&rtc_hw->irq_setup_1,
			    RTC_IRQ_SETUP_1_MIN_ENA_BITS |
				    (alarm->tm_min << RTC_IRQ_SETUP_1_MIN_LSB));
	}
	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		hw_set_bits(&rtc_hw->irq_setup_1,
			    RTC_IRQ_SETUP_1_SEC_ENA_BITS |
				    (alarm->tm_sec << RTC_IRQ_SETUP_1_SEC_LSB));
	}
	data->alarm_time = *alarm;
	data->alarm_mask = mask;
	k_spin_unlock(&data->lock, key);

	/* Enable the IRQ at the peri */
	rtc_hw->inte = RTC_INTE_RTC_BITS;

	rtc_enable_alarm();

	return err;
}

static int rtc_rpi_pico_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				       struct rtc_time *timeptr)
{
	struct rtc_rpi_pico_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		*timeptr = data->alarm_time;
		*mask = data->alarm_mask;
	}

	return 0;
}

static int rtc_rpi_pico_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_rpi_pico_data *data = dev->data;
	int ret = 0;

	if (id != 0) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		ret = data->alarm_pending ? 1 : 0;
		data->alarm_pending = false;
	}

	return ret;
}

static int rtc_rpi_pico_alarm_set_callback(const struct device *dev, uint16_t id,
					   rtc_alarm_callback callback, void *user_data)
{
	struct rtc_rpi_pico_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->alarm_callback = callback;
		data->alarm_user_data = user_data;
		if ((callback == NULL) && (user_data == NULL)) {
			rtc_disable_alarm();
		}
	}

	return 0;
}

#endif /* CONFIG_RTC_ALARM */

static const struct rtc_driver_api rtc_rpi_pico_driver_api = {
	.set_time = rtc_rpi_pico_set_time,
	.get_time = rtc_rpi_pico_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_supported_fields = rtc_rpi_pico_alarm_get_supported_fields,
	.alarm_set_time = rtc_rpi_pico_alarm_set_time,
	.alarm_get_time = rtc_rpi_pico_alarm_get_time,
	.alarm_is_pending = rtc_rpi_pico_alarm_is_pending,
	.alarm_set_callback = rtc_rpi_pico_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

DEVICE_DT_INST_DEFINE(0, &rtc_rpi_pico_init, NULL, &rtc_data, NULL, POST_KERNEL,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_rpi_pico_driver_api);
