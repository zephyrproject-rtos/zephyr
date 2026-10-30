/* SPDX-License-Identifier: Apache-2.0 */

/*
 * Copyright (c) 2026 Gail Rojas
 *
 * RTC_C calendar-mode driver for TI MSP432P4XX.
 *
 * Clock source: BCLK (routed to REFO by the SOC patch at ~32.768 kHz).
 * Binary (non-BCD) mode is used throughout.
 */

#define DT_DRV_COMPAT ti_msp432p4xx_rtc

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include "rtc_utils.h"

struct rtc_msp432p4xx_config {
	RTC_C_Type *base;
};

struct rtc_msp432p4xx_data {
	struct k_spinlock lock;
};

static int rtc_msp432p4xx_set_time(const struct device *dev,
				    const struct rtc_time *timeptr)
{
	const struct rtc_msp432p4xx_config *cfg = dev->config;
	struct rtc_msp432p4xx_data *data = dev->data;
	RTC_C_Type *rtc = cfg->base;
	k_spinlock_key_t key;
	uint16_t tim0, tim1, date;
	int year, mon;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	year = timeptr->tm_year + 1900;
	mon  = timeptr->tm_mon  + 1;

	tim0 = (uint16_t)timeptr->tm_sec |
	       ((uint16_t)timeptr->tm_min << RTC_C_TIM0_MIN_OFS);
	tim1 = (uint16_t)timeptr->tm_hour |
	       ((uint16_t)timeptr->tm_wday << RTC_C_TIM1_DOW_OFS);
	date = (uint16_t)timeptr->tm_mday |
	       ((uint16_t)mon << RTC_C_DATE_MON_OFS);

	key = k_spin_lock(&data->lock);

	/*
	 * Unlock the RTC_C write-protect key.  CTL0[15:8] holds the key;
	 * CTL0[7:0] holds interrupt-enable and flag bits.  A plain
	 * assignment would zero the low byte and silently clear any alarm
	 * or ready interrupts the caller configured.  Use read-modify-write
	 * to preserve those bits.
	 *
	 * Assert HOLD to freeze the calendar counters, write all six time
	 * registers, then clear HOLD and re-lock.  Back-to-back writes are
	 * safe while HOLD is asserted; the 2-3 BCLK settling delay in
	 * SLAU356 section 6.4.2 applies only when running.
	 */
	rtc->CTL0  = (rtc->CTL0 & 0x00FFU) | RTC_C_KEY_VAL;
	rtc->CTL13 |= RTC_C_CTL13_HOLD;
	rtc->TIM0  = tim0;
	rtc->TIM1  = tim1;
	rtc->DATE  = date;
	rtc->YEAR  = (uint16_t)year;
	rtc->CTL13 &= ~(uint16_t)RTC_C_CTL13_HOLD;
	rtc->CTL0  = rtc->CTL0 & 0x00FFU;

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int rtc_msp432p4xx_get_time(const struct device *dev,
				    struct rtc_time *timeptr)
{
	const struct rtc_msp432p4xx_config *cfg = dev->config;
	struct rtc_msp432p4xx_data *data = dev->data;
	RTC_C_Type *rtc = cfg->base;
	k_spinlock_key_t key;
	uint16_t tim0, tim1, date, year;
	uint8_t attempts = 3U;
	bool ok = false;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/*
	 * Double-read consistency check: read all four calendar registers,
	 * then read them again and accept only when both snapshots agree.
	 * A mismatch means a 1-second rollover crossed the read window;
	 * up to three attempts total.  This replaces CTL13_RDY polling (a
	 * 30 us hardware pulse, once per second) and works on the first
	 * call immediately after set_time without any timeout.
	 */
	do {
		tim0 = rtc->TIM0;
		tim1 = rtc->TIM1;
		date = rtc->DATE;
		year = rtc->YEAR;
		if (rtc->TIM0 == tim0 && rtc->TIM1 == tim1 &&
		    rtc->DATE == date && rtc->YEAR == year) {
			ok = true;
			break;
		}
	} while (--attempts > 0U);

	k_spin_unlock(&data->lock, key);

	if (!ok) {
		return -EIO;
	}

	timeptr->tm_sec  = (int)(tim0 & RTC_C_TIM0_SEC_MASK);
	timeptr->tm_min  = (int)((tim0 & RTC_C_TIM0_MIN_MASK)
				 >> RTC_C_TIM0_MIN_OFS);
	timeptr->tm_hour = (int)(tim1 & RTC_C_TIM1_HOUR_MASK);
	timeptr->tm_wday = (int)((tim1 & RTC_C_TIM1_DOW_MASK)
				 >> RTC_C_TIM1_DOW_OFS);
	timeptr->tm_mday = (int)(date & RTC_C_DATE_DAY_MASK);
	timeptr->tm_mon  = (int)((date & RTC_C_DATE_MON_MASK)
				 >> RTC_C_DATE_MON_OFS) - 1;
	timeptr->tm_year = (int)year - 1900;
	timeptr->tm_yday  = -1;
	timeptr->tm_nsec  = 0;
	timeptr->tm_isdst = -1;

	return 0;
}

static int rtc_msp432p4xx_init(const struct device *dev)
{
	const struct rtc_msp432p4xx_config *cfg = dev->config;
	RTC_C_Type *rtc = cfg->base;

	/*
	 * Unlock (preserving CTL0[7:0] interrupt bits), then assert HOLD
	 * before writing any other CTL13 bits.  The TRM states that CTL13
	 * writes are silently ignored while HOLD=0; after a warm reset the
	 * previous boot left HOLD=0, so the MODE write would be discarded
	 * without this step and the RTC would remain in counter mode.
	 *
	 * Enable calendar mode (MODE), keep binary (BCD=0, default), clear
	 * HOLD to start ticking, then re-lock.  Time registers hold their
	 * power-on values until the first set_time call.
	 */
	rtc->CTL0  = (rtc->CTL0 & 0x00FFU) | RTC_C_KEY_VAL;
	rtc->CTL13 |= RTC_C_CTL13_HOLD;
	rtc->CTL13 |= RTC_C_CTL13_MODE;
	rtc->CTL13 &= ~(uint16_t)RTC_C_CTL13_HOLD;
	rtc->CTL0  = rtc->CTL0 & 0x00FFU;

	return 0;
}

static DEVICE_API(rtc, rtc_msp432p4xx_driver_api) = {
	.set_time = rtc_msp432p4xx_set_time,
	.get_time = rtc_msp432p4xx_get_time,
};

#define RTC_MSP432P4XX_DEVICE_INIT(n)					\
	static struct rtc_msp432p4xx_data rtc_data_##n;			\
									\
	static const struct rtc_msp432p4xx_config rtc_config_##n = {	\
		.base = (RTC_C_Type *)DT_INST_REG_ADDR(n),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &rtc_msp432p4xx_init, NULL,		\
			      &rtc_data_##n, &rtc_config_##n,		\
			      PRE_KERNEL_1,				\
			      CONFIG_RTC_INIT_PRIORITY,			\
			      &rtc_msp432p4xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_MSP432P4XX_DEVICE_INIT)
