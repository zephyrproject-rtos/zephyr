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

#include <hardware/rtc.h>

#define DT_DRV_COMPAT raspberrypi_pico_rtc

#define CLK_DRV DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0))
#define CLK_ID  (clock_control_subsys_t) DT_INST_PHA_BY_IDX(0, clocks, 0, clk_id)

/* struct tm start time:   1st, Jan, 1900 */
#define TM_YEAR_REF 1900
/* See section 4.8.1 of the RP2040 datasheet. */
#define RP2040_RTC_YEAR_MAX 4095
struct rtc_rpi_pico_data {
	struct k_spinlock lock;
};

LOG_MODULE_REGISTER(rtc_rpi, CONFIG_RTC_LOG_LEVEL);

static int rtc_rpi_pico_init(const struct device *dev)
{
	int ret;

	ret = clock_control_on(CLK_DRV, CLK_ID);
	if (ret < 0) {
		return ret;
	}

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

static const struct rtc_driver_api rtc_rpi_pico_driver_api = {
	.set_time = rtc_rpi_pico_set_time,
	.get_time = rtc_rpi_pico_get_time,
};

static struct rtc_rpi_pico_data rtc_data;

DEVICE_DT_INST_DEFINE(0, &rtc_rpi_pico_init, NULL, &rtc_data, NULL, POST_KERNEL,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_rpi_pico_driver_api);
