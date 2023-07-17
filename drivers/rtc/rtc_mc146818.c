/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT motorola_mc146818

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/sys_io.h>

#define RTC_STD_INDEX (DT_INST_REG_ADDR_BY_IDX(0, 0))
#define RTC_STD_TARGET (DT_INST_REG_ADDR_BY_IDX(0, 1))

/* Time indices in RTC RAM */
#define RTC_SEC		0x00
#define RTC_MIN		0x02
#define RTC_HOUR	0x04

/* Day of week index in RTC RAM */
#define RTC_WDAY	0x06

/* Day of month index in RTC RAM */
#define RTC_MDAY	0x07

/* Month and year index in RTC RAM */
#define RTC_MONTH	0x08
#define RTC_YEAR	0x09

/* Y2K Bugfix */
#define RTC_CENTURY	0x32

/* Alarm time indices in RTC RAM */
#define RTC_ALARM_SEC	0x01
#define RTC_ALARM_MIN	0x03
#define RTC_ALARM_HOUR	0x05

/* Registers A-D indeces in RTC RAM */
#define RTC_REG_A	0x0A
#define RTC_REG_B	0x0B
#define RTC_REG_C	0x0C
#define RTC_REG_D	0x0D

#define RTC_UIP		RTC_REG_A
#define RTC_DATA	RTC_REG_B
#define RTC_FLAG	RTC_REG_C

/* Alarm don't case state */
#define RTC_ALARM_DC	0xFF

/* Update In Progress bit in REG_A */
#define RTC_UIP_BIT	BIT(7)

/* Update Cycle Inhibit bit in REG_B */
#define RTC_UCI_BIT	BIT(7)

/* Periodic Interrupt Enable bit in REG_B */
#define RTC_PIE_BIT	BIT(6)

/* Alarm Interrupt Enable bit in REG_B */
#define RTC_AIE_BIT	BIT(5)

/* Update-ended Interrupt Enable bit in REG_B */
#define RTC_UIE_BIT	BIT(4)

/* Data mode bit in  REG_B */
#define RTC_DMODE_BIT	BIT(2)

/* Hour Format bit in REG_B */
#define RTC_HFORMAT_BIT	BIT(1)

/* Daylight Savings Enable Format bit in REG_B */
#define RTC_DSE_BIT	BIT(0)

/* Interrupt Request Flag bit in REG_C */
#define RTC_IRF_BIT	BIT(7)

/* Periodic Flag bit in REG_C */
#define RTC_PF_BIT	BIT(6)

/* Alarm Flag bit in REG_C */
#define RTC_AF_BIT	BIT(5)

/* Update-end Flag bit in REG_C */
#define RTC_UEF_BIT	BIT(4)

/* VRT bit in REG_D */
#define RTC_VRT_BIT	BIT(7)

/* Month day Alarm bits in REG_D */
#define RTC_MDAY_ALARM	BIT_MASK(5)

/* Maximum and Minimum values of time */
#define MIN_SEC		0
#define MAX_SEC		59
#define MIN_MIN		0
#define MAX_MIN		59
#define MIN_HOUR	0
#define MAX_HOUR	23
#define MAX_WDAY	7
#define MIN_WDAY	1
#define MAX_MDAY	31
#define MIN_MDAY	1
#define MAX_MON		12
#define MIN_MON		1
#define MIN_YEAR_DIFF	0 /* YEAR - 1900 */
#define MAX_YEAR_DIFF	99 /* YEAR - 1999 */

/* Input clock frequency mapped to divider bits */
#define RTC_IN_CLK_DIV_BITS_4194304 (0)
#define RTC_IN_CLK_DIV_BITS_1048576 (1 << 4)
#define RTC_IN_CLK_DIV_BITS_32768   (2 << 4)

struct rtc_mc146818_data {
	struct k_spinlock lock;
	bool alarm_pending;
	rtc_alarm_callback cb;
	void *cb_data;
	rtc_update_callback update_cb;
	void *update_cb_data;
};

static uint8_t rtc_read(int reg)
{
	uint8_t value;

	sys_out8(reg, RTC_STD_INDEX);
	value = sys_in8(RTC_STD_TARGET);

	return value;
}

static void rtc_write(int reg, uint8_t value)
{
	sys_out8(reg, RTC_STD_INDEX);
	sys_out8(value, RTC_STD_TARGET);
}

static bool rtc_mc146818_validate_time(const struct rtc_time *timeptr)
{
	if (timeptr->tm_sec < MIN_SEC || timeptr->tm_sec > MAX_SEC) {
		return false;
	}
	if (timeptr->tm_min < MIN_MIN || timeptr->tm_min > MAX_MIN) {
		return false;
	}
	if (timeptr->tm_hour < MIN_HOUR || timeptr->tm_hour > MAX_HOUR) {
		return false;
	}
	if (timeptr->tm_wday + 1 < MIN_WDAY || timeptr->tm_wday + 1 > MAX_WDAY) {
		return false;
	}
	if (timeptr->tm_mday < MIN_MDAY || timeptr->tm_mday > MAX_MDAY) {
		return false;
	}
	if (timeptr->tm_mon + 1 < MIN_MON || timeptr->tm_mon + 1 > MAX_MON) {
		return false;
	}
	if (timeptr->tm_year - 70 < MIN_YEAR_DIFF || timeptr->tm_year - 70 > MAX_YEAR_DIFF) {
		return false;
	}
	return true;
}

static int rtc_mc146818_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	uint8_t value;
	int year;
	int cent;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (timeptr == NULL) {
		ret = -EINVAL;
		goto out;
	}

	/* Check time valid */
	if (!rtc_mc146818_validate_time(timeptr)) {
		ret = -EINVAL;
		goto out;
	}

	value = rtc_read(RTC_DATA);
	rtc_write(RTC_DATA, value | RTC_UCI_BIT);

	year = (1900 + timeptr->tm_year) % 100;
	cent = (1900 + timeptr->tm_year) / 100;

	rtc_write(RTC_SEC, (uint8_t)timeptr->tm_sec);
	rtc_write(RTC_MIN, (uint8_t)timeptr->tm_min);
	rtc_write(RTC_HOUR, (uint8_t)timeptr->tm_hour);
	rtc_write(RTC_WDAY, (uint8_t)timeptr->tm_wday);
	rtc_write(RTC_MDAY, (uint8_t)timeptr->tm_mday);
	rtc_write(RTC_MONTH, (uint8_t)timeptr->tm_mon + 1);
	rtc_write(RTC_YEAR, year);
	rtc_write(RTC_CENTURY, cent);

	value &= (~RTC_UCI_BIT);
	rtc_write(RTC_DATA, value);
	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int rtc_mc146818_get_time(const struct device *dev, struct rtc_time  *timeptr)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	int ret;
	uint8_t cent;
	uint8_t year;
	uint8_t value;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	/* Validate arguments */
	if (timeptr == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (!(rtc_read(RTC_REG_D) & RTC_VRT_BIT)) {
		ret = -ENODATA;
		goto out;
	}

	while (rtc_read(RTC_UIP) & RTC_UIP_BIT) {
		continue;
	}

	cent = rtc_read(RTC_CENTURY);
	year = rtc_read(RTC_YEAR);
	timeptr->tm_mon = rtc_read(RTC_MONTH) - 1;
	timeptr->tm_mday = rtc_read(RTC_MDAY);
	timeptr->tm_wday = rtc_read(RTC_WDAY) - 1;
	timeptr->tm_hour = rtc_read(RTC_HOUR);
	timeptr->tm_min = rtc_read(RTC_MIN);
	timeptr->tm_sec = rtc_read(RTC_SEC);

	timeptr->tm_year = 100 * (int)cent + year - 1900;

	timeptr->tm_nsec = 0;
	timeptr->tm_yday = 0;
	value = rtc_read(RTC_DATA);

	/* Check time valid */
	if (!rtc_mc146818_validate_time(timeptr)) {
		ret = -ENODATA;
		goto out;
	}
	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

#if defined(CONFIG_RTC_ALARM)
static bool rtc_mc146818_validate_alarm(const struct rtc_time *timeptr, uint32_t mask)
{
	if ((mask & RTC_ALARM_TIME_MASK_SECOND) &&
	    (timeptr->tm_sec < MIN_SEC || timeptr->tm_sec > MAX_SEC)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) &&
	    (timeptr->tm_min < MIN_MIN || timeptr->tm_min > MAX_MIN)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) &&
	    (timeptr->tm_hour < MIN_HOUR || timeptr->tm_hour > MAX_HOUR)) {
		return false;
	}

	return true;
}

static int rtc_mc146818_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						   uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		return -EINVAL;
	}

	(*mask) = (RTC_ALARM_TIME_MASK_SECOND
		   | RTC_ALARM_TIME_MASK_MINUTE
		   | RTC_ALARM_TIME_MASK_HOUR);

	return 0;
}

static int rtc_mc146818_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				       const struct rtc_time *timeptr)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (id != 0) {
		ret = -EINVAL;
		goto out;
	}

	if ((mask > 0) && (timeptr == NULL)) {
		ret = -EINVAL;
		goto out;
	}

	/* Check time valid */
	if (!rtc_mc146818_validate_alarm(timeptr, mask)) {
		ret = -EINVAL;
		goto out;
	}

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		rtc_write(RTC_ALARM_SEC, timeptr->tm_sec);
	} else {
		rtc_write(RTC_ALARM_SEC, RTC_ALARM_DC);
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		rtc_write(RTC_ALARM_MIN, timeptr->tm_min);
	} else {
		rtc_write(RTC_ALARM_SEC, RTC_ALARM_DC);
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		rtc_write(RTC_ALARM_HOUR, timeptr->tm_hour);
	} else {
		rtc_write(RTC_ALARM_SEC, RTC_ALARM_DC);
	}

	rtc_write(RTC_DATA, rtc_read(RTC_DATA) | RTC_AIE_BIT);
	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int rtc_mc146818_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	uint8_t value;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (id != 0) {
		ret = -EINVAL;
		goto out;
	}

	if (timeptr == NULL) {
		ret = -EINVAL;
		goto out;
	}

	(*mask) = 0;

	value = rtc_read(RTC_ALARM_SEC);
	if (value <= MAX_SEC) {
		timeptr->tm_sec = value;
		(*mask) |= RTC_ALARM_TIME_MASK_SECOND;
	}

	value = rtc_read(RTC_ALARM_MIN);
	if (value <= MAX_SEC) {
		timeptr->tm_min = value;
		(*mask) |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	value = rtc_read(RTC_ALARM_HOUR);
	if (value <= MAX_SEC) {
		timeptr->tm_hour = value;
		(*mask) |= RTC_ALARM_TIME_MASK_HOUR;
	}

	ret = 0;
out:
	k_spin_unlock(&dev_data->lock, key);
	return ret;
}

static int rtc_mc146818_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	struct rtc_mc146818_data * const dev_data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->cb = callback;
	dev_data->cb_data = user_data;

	if (callback != NULL) {
		/* Enable Alarm callback */
		rtc_write(RTC_DATA, (rtc_read(RTC_DATA) | RTC_AIE_BIT));
	} else {
		/* Disable Alarm callback */
		rtc_write(RTC_DATA, (rtc_read(RTC_DATA) & (~RTC_AIE_BIT)));
	}

	k_spin_unlock(&dev_data->lock, key);
	return 0;
}

static int rtc_mc146818_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	int ret;

	if (id != 0) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ret = dev_data->alarm_pending ? 1 : 0;
	dev_data->alarm_pending = false;

	k_spin_unlock(&dev_data->lock, key);
	return ret;
}
#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)
static int rtc_mc146818_update_set_callback(const struct device *dev,
					rtc_update_callback callback, void *user_data)
{
	struct rtc_mc146818_data * const dev_data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	dev_data->update_cb = callback;
	dev_data->update_cb_data = user_data;

	if (callback != NULL) {
		/* Enable update callback */
		rtc_write(RTC_DATA, (rtc_read(RTC_DATA) | RTC_UIE_BIT));
	} else {
		/* Disable update callback */
		rtc_write(RTC_DATA, (rtc_read(RTC_DATA) & (~RTC_UIE_BIT)));
	}


	k_spin_unlock(&dev_data->lock, key);
	return 0;
}

#endif /* CONFIG_RTC_UPDATE */

static void rtc_mc146818_isr(const struct device *dev)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	uint8_t regc;

	ARG_UNUSED(dev_data);

	/* Read register, which clears the register */
	regc = rtc_read(RTC_FLAG);

#if defined(CONFIG_RTC_ALARM)
	if (regc & RTC_AF_BIT) {
		if (dev_data->cb) {
			dev_data->cb(dev, 0, dev_data->cb_data);
			dev_data->alarm_pending = false;
		} else {
			dev_data->alarm_pending = true;
		}
	}
#endif

#if defined(CONFIG_RTC_UPDATE)
	if (regc & RTC_UEF_BIT) {
		if (dev_data->update_cb) {
			dev_data->update_cb(dev, dev_data->update_cb_data);
		}
	}
#endif
}

struct rtc_driver_api rtc_mc146818_driver_api = {
	.set_time = rtc_mc146818_set_time,
	.get_time = rtc_mc146818_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_supported_fields = rtc_mc146818_alarm_get_supported_fields,
	.alarm_set_time = rtc_mc146818_alarm_set_time,
	.alarm_get_time = rtc_mc146818_alarm_get_time,
	.alarm_is_pending = rtc_mc146818_alarm_is_pending,
	.alarm_set_callback = rtc_mc146818_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rtc_mc146818_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
};

#define RTC_MC146818_INIT_FN_DEFINE(n)						\
	static int rtc_mc146818_init##n(const struct device *dev)		\
	{									\
		rtc_write(RTC_REG_A,						\
			  _CONCAT(RTC_IN_CLK_DIV_BITS_,				\
				  DT_INST_PROP(n, clock_frequency)));		\
										\
		rtc_write(RTC_REG_B, RTC_DMODE_BIT | RTC_HFORMAT_BIT);		\
										\
		IRQ_CONNECT(DT_INST_IRQN(0),					\
				DT_INST_IRQ(0, priority),			\
				rtc_mc146818_isr, DEVICE_DT_INST_GET(n),	\
				DT_INST_IRQ(0, sense));				\
										\
		irq_enable(DT_INST_IRQN(0));					\
										\
		return 0;							\
	}

#define RTC_MC146818_DEV_CFG(inst)						\
	struct rtc_mc146818_data rtc_mc146818_data##inst;			\
										\
	RTC_MC146818_INIT_FN_DEFINE(inst)					\
										\
	DEVICE_DT_INST_DEFINE(inst, &rtc_mc146818_init##inst, NULL,		\
			      &rtc_mc146818_data##inst, NULL, POST_KERNEL,	\
			      CONFIG_RTC_INIT_PRIORITY,				\
			      &rtc_mc146818_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(RTC_MC146818_DEV_CFG)
