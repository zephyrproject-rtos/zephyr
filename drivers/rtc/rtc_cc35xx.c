/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_rtc_timer

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/timeutil.h>

#include <ti/devices/cc35xx/inc/hw_rtc.h>
#include <ti/devices/cc35xx/inc/hw_systim.h>
#include <ti/devices/cc35xx/inc/hw_memmap.h>

#include <time.h>
#include <string.h>

#include "rtc_utils.h"

#define RTC_TI_CC35XX_STEP_MASK           (0x7FFFFFFF)
#define RTC_TI_CC35XX_HIGH_RES_RANGE_US   (134LL * USEC_PER_SEC)
#define RTC_TI_CC35XX_HIGH_RES_STEP_SHIFT (2)
#define RTC_TI_CC35XX_LOW_RES_STEP_SHIFT  (20)
#define RTC_TI_CC35XX_DTIME_EXP           (1 << 31)

#define RTC_TI_CC35XX_EPOCH_YEAR (CONFIG_RTC_TI_CC35XX_EPOCH_YEAR - 1900)

#define RTC_TI_CC35XX_TOP_19_BITS_MASK 0xFFFFE000

struct rtc_ti_cc35xx_data {
	struct k_spinlock lock;
};

static inline int64_t rtc_bias_1970_to_xxxx(int reduced_year)
{
	struct tm t = {
		.tm_year = reduced_year,
		.tm_mon = 0,
		.tm_mday = 1,
		.tm_hour = 0,
		.tm_min = 0,
		.tm_sec = 0,
		.tm_isdst = 0,
	};
	return timeutil_timegm64(&t);
}

static int rtc_ti_cc35xx_init(const struct device *dev)
{
	/* Configure RTC to halt when CPU stopped during debug */
	sys_write32(RTC_EMU_HALT_STOP, RTC_BASE + RTC_O_EMU);
	return 0;
}

static uint64_t rtc_ti_cc35xx_get_raw_time(struct rtc_ti_cc35xx_data *data)
{
	k_spinlock_key_t key;
	uint64_t usec;
	uint32_t time524m;
	uint32_t time1u;

	key = k_spin_lock(&data->lock);

	while (true) {
		time1u = sys_read32(RTC_BASE + RTC_O_TIME1U);
		time524m = sys_read32(RTC_BASE + RTC_O_TIME524M);
		/*
		 * Check if TIME1U wrapped around.
		 * In such case read time again to get correct
		 * concatenated value.
		 */
		if ((time1u >> 31) == 1) {
			if ((sys_read32(RTC_BASE + RTC_O_TIME1U) >> 31) == 0) {
				continue;
			}
		}
		break;
	}

	usec = (uint64_t)(time524m & RTC_TI_CC35XX_TOP_19_BITS_MASK) << 19;
	usec |= time1u;
	k_spin_unlock(&data->lock, key);

	return usec;
}

static void rtc_ti_cc35xx_set_dtime(struct rtc_ti_cc35xx_data *data, int64_t delta_us)
{
	k_spinlock_key_t key;
	uint32_t raw_data;

	if (IN_RANGE(delta_us, -RTC_TI_CC35XX_HIGH_RES_RANGE_US,
		     RTC_TI_CC35XX_HIGH_RES_RANGE_US - 1)) {
		raw_data = (uint32_t)((delta_us << RTC_TI_CC35XX_HIGH_RES_STEP_SHIFT) &
				      RTC_TI_CC35XX_STEP_MASK);
	} else {
		raw_data = RTC_TI_CC35XX_DTIME_EXP;
		raw_data |= (uint32_t)((delta_us >> RTC_TI_CC35XX_LOW_RES_STEP_SHIFT) &
				       RTC_TI_CC35XX_STEP_MASK);
	}

	key = k_spin_lock(&data->lock);

	sys_write32(raw_data, RTC_BASE + RTC_O_DTIME);

	/* Set SYNCUP bit to synchronize RTC with system timer */
	sys_write32(SYSTIM_STA_SYNCUP_SET, SYSTIM_BASE + SYSTIM_O_STA);
	/*
	 * Waiting for SYNCUP bit to be cleared which indicated lftick.
	 * This should not take more than one Slow Clock period.
	 */
	while (sys_read32(SYSTIM_BASE + SYSTIM_O_STA) != SYSTIM_STA_VAL_RUN) {
	}
	k_spin_unlock(&data->lock, key);
}

static int rtc_ti_cc35xx_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_ti_cc35xx_data *data = dev->data;
	int64_t sec_unix_target;
	int64_t sec_hw_target;
	int64_t usec_hw_target;
	uint64_t usec_hw_time;
	uint64_t sec_bias;
	int64_t delta_us;

	if (!timeptr || (timeptr->tm_year < RTC_TI_CC35XX_EPOCH_YEAR)) {
		return -EINVAL;
	}

	sec_unix_target = timeutil_timegm64((const struct tm *)timeptr);
	if (sec_unix_target > INT64_MAX / USEC_PER_SEC) {
		return -ERANGE;
	}

	sec_bias = (uint64_t)rtc_bias_1970_to_xxxx(RTC_TI_CC35XX_EPOCH_YEAR);
	sec_hw_target = sec_unix_target - sec_bias;
	usec_hw_target = sec_hw_target * USEC_PER_SEC;
	/*
	 * This function is for setting time using low resolution (1.049s)
	 * so truncate anything smaller to avoid rounding issues.
	 */
	usec_hw_time = (rtc_ti_cc35xx_get_raw_time(data) / USEC_PER_SEC) * USEC_PER_SEC;

	delta_us = usec_hw_target - usec_hw_time;

	rtc_ti_cc35xx_set_dtime(data, delta_us);

	return 0;
}

static int rtc_ti_cc35xx_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_ti_cc35xx_data *data = dev->data;
	uint64_t sec_hw_time;
	uint64_t usec_hw_time;
	uint64_t sec_unix_time;
	uint64_t sec_bias;

	usec_hw_time = rtc_ti_cc35xx_get_raw_time(data);
	sec_hw_time = usec_hw_time / USEC_PER_SEC;
	sec_bias = (uint64_t)rtc_bias_1970_to_xxxx(RTC_TI_CC35XX_EPOCH_YEAR);
	sec_unix_time = sec_hw_time + sec_bias;

	gmtime_r(&sec_unix_time, (struct tm *)timeptr);

	/* Unsupported */
	timeptr->tm_isdst = -1;
	/* Unsupported */
	timeptr->tm_nsec = 0;

	return 0;
}

static DEVICE_API(rtc, rtc_ti_cc35xx_driver_api) = {
	.set_time = rtc_ti_cc35xx_set_time,
	.get_time = rtc_ti_cc35xx_get_time,
};

#define RTC_TI_CC35XX_DEVICE(id)                                                                   \
	static struct rtc_ti_cc35xx_data rtc_ti_cc35xx_data_##id;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(id, rtc_ti_cc35xx_init, NULL, &rtc_ti_cc35xx_data_##id, NULL,        \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rtc_ti_cc35xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_TI_CC35XX_DEVICE);
