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
#define RTC_ALARM_MDAY	RTC_REG_D

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
#define MAX_MON		11
#define MIN_MON		0
#define MIN_YEAR_DIFF	0 /* YEAR - 1900 */
#define MAX_YEAR_DIFF	199 /* YEAR - 1900 */

struct rtc_mc146818_data {
	struct k_spinlock lock;
	uint16_t alarms_count;
	uint16_t mask;
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
	if (timeptr->tm_wday < MIN_WDAY || timeptr->tm_wday > MAX_WDAY) {
		return false;
	}
	if (timeptr->tm_mday < MIN_MDAY || timeptr->tm_mday > MAX_MDAY) {
		return false;
	}
	if (timeptr->tm_mon < MIN_MON || timeptr->tm_mon > MAX_MON) {
		return false;
	}
	if (timeptr->tm_year < MIN_YEAR_DIFF || timeptr->tm_year > MAX_YEAR_DIFF) {
		return false;
	}
	return true;
}

static int rtc_mc146818_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	uint8_t value;
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

	if (!(rtc_read(RTC_DATA) & RTC_DMODE_BIT)) {
		rtc_write(RTC_SEC, (uint8_t)bin2bcd(timeptr->tm_sec));
		rtc_write(RTC_MIN, (uint8_t)bin2bcd(timeptr->tm_min));
		rtc_write(RTC_HOUR, (uint8_t)bin2bcd(timeptr->tm_hour));
		rtc_write(RTC_WDAY, (uint8_t)bin2bcd(timeptr->tm_wday));
		rtc_write(RTC_MDAY, (uint8_t)bin2bcd(timeptr->tm_mday));
		rtc_write(RTC_MONTH, (uint8_t)bin2bcd(timeptr->tm_mon + 1));
		rtc_write(RTC_YEAR, (uint8_t)bin2bcd(timeptr->tm_year));
	} else {
		rtc_write(RTC_SEC, (uint8_t)timeptr->tm_sec);
		rtc_write(RTC_MIN, (uint8_t)timeptr->tm_min);
		rtc_write(RTC_HOUR, (uint8_t)timeptr->tm_hour);
		rtc_write(RTC_WDAY, (uint8_t)timeptr->tm_wday);
		rtc_write(RTC_MDAY, (uint8_t)timeptr->tm_mday);
		rtc_write(RTC_MONTH, (uint8_t)timeptr->tm_mon + 1);
		rtc_write(RTC_YEAR, (uint8_t)timeptr->tm_year);
	}

	if (timeptr->tm_isdst == 1) {
		value |= RTC_DSE_BIT;
	} else {
		value &= (~RTC_DSE_BIT);
	}
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
	uint8_t value;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	/* Validate arguments */
	if (timeptr == NULL) {
		ret = -EINVAL;
		goto out;
	}

	if (!(rtc_read(RTC_ALARM_MDAY) & RTC_VRT_BIT)) {
		ret = -ENODATA;
		goto out;
	}

	while (rtc_read(RTC_UIP) & RTC_UIP_BIT) {
		continue;
	}
	timeptr->tm_year = rtc_read(RTC_YEAR);
	timeptr->tm_mon = rtc_read(RTC_MONTH) - 1;
	timeptr->tm_mday = rtc_read(RTC_MDAY);
	timeptr->tm_wday = rtc_read(RTC_WDAY);
	timeptr->tm_hour = rtc_read(RTC_HOUR);
	timeptr->tm_min = rtc_read(RTC_MIN);
	timeptr->tm_sec = rtc_read(RTC_SEC);

	if (!(rtc_read(RTC_DATA) & RTC_DMODE_BIT)) {
		timeptr->tm_year = bcd2bin(timeptr->tm_year);
		timeptr->tm_mon = bcd2bin(timeptr->tm_mon);
		timeptr->tm_mday = bcd2bin(timeptr->tm_mday);
		timeptr->tm_wday = bcd2bin(timeptr->tm_wday);
		timeptr->tm_hour = bcd2bin(timeptr->tm_hour);
		timeptr->tm_min = bcd2bin(timeptr->tm_min);
		timeptr->tm_sec = bcd2bin(timeptr->tm_sec);
	}

	timeptr->tm_nsec = 0;
	timeptr->tm_yday = 0;
	value = rtc_read(RTC_DATA);
	if (value & RTC_DSE_BIT) {
		timeptr->tm_isdst = 1;
	} else {
		timeptr->tm_isdst = -1;
	}

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

	if ((mask & RTC_ALARM_TIME_MASK_MONTH) &&
	    (timeptr->tm_mon < MIN_WDAY || timeptr->tm_mon > MAX_WDAY)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
	    (timeptr->tm_mday < MIN_MDAY || timeptr->tm_mday > MAX_MDAY)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_YEAR) &&
	    (timeptr->tm_year < MIN_YEAR_DIFF || timeptr->tm_year > MAX_YEAR_DIFF)) {
		return false;
	}

	return true;
}

static int rtc_mc146818_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	struct rtc_mc146818_data * const dev_data = dev->data;

	if (dev_data->alarms_count <= id) {
		return -EINVAL;
	}

	(*mask) = (RTC_ALARM_TIME_MASK_SECOND
		   | RTC_ALARM_TIME_MASK_MINUTE
		   | RTC_ALARM_TIME_MASK_HOUR
		   | RTC_ALARM_TIME_MASK_MONTHDAY
		   | RTC_ALARM_TIME_MASK_MONTH);

	return 0;
}

static int rtc_mc146818_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	struct rtc_mc146818_data * const dev_data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (dev_data->alarms_count <= id) {
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

	dev_data->mask = mask;

	if (!(rtc_read(RTC_DATA) & RTC_DMODE_BIT)) {
		if (mask & RTC_ALARM_TIME_MASK_SECOND) {
			rtc_write(RTC_ALARM_SEC, bin2bcd(timeptr->tm_sec));
		} else {
			rtc_write(RTC_ALARM_SEC, bin2bcd(RTC_ALARM_DC));
		}

		if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
			rtc_write(RTC_ALARM_MIN, bin2bcd(timeptr->tm_min));
		} else {
			rtc_write(RTC_ALARM_SEC, bin2bcd(RTC_ALARM_DC));
		}

		if (mask & RTC_ALARM_TIME_MASK_HOUR) {
			rtc_write(RTC_ALARM_HOUR, bin2bcd(timeptr->tm_hour));
		} else {
			rtc_write(RTC_ALARM_SEC, bin2bcd(RTC_ALARM_DC));
		}

	} else {
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

	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		rtc_write(RTC_ALARM_MDAY, rtc_read(RTC_REG_D) |
			  timeptr->tm_mday);
	} else {
		rtc_write(RTC_ALARM_SEC, rtc_read(RTC_REG_D) &
			  (~RTC_MDAY_ALARM));
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
	int ret;

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	if (dev_data->alarms_count <= id) {
		ret = -EINVAL;
		goto out;
	}

	if (timeptr == NULL) {
		ret = -EINVAL;
		goto out;
	}

	timeptr->tm_sec = rtc_read(RTC_ALARM_SEC);
	timeptr->tm_min = rtc_read(RTC_ALARM_MIN);
	timeptr->tm_hour = rtc_read(RTC_ALARM_HOUR);
	timeptr->tm_mday = (rtc_read(RTC_ALARM_MDAY) & RTC_MDAY_ALARM);

	if (!(rtc_read(RTC_DATA) & RTC_DMODE_BIT)) {
		timeptr->tm_sec = bcd2bin(timeptr->tm_sec);
		timeptr->tm_min = bcd2bin(timeptr->tm_min);
		timeptr->tm_hour = bcd2bin(timeptr->tm_hour);
	}

	(*mask) = dev_data->mask;

	/* Check time valid */
	if (!rtc_mc146818_validate_alarm(timeptr, (*mask))) {
		ret = -ENODATA;
		goto out;
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

	if (dev_data->alarms_count <= id) {
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

	if (dev_data->alarms_count <= id) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&dev_data->lock);

	ret = (dev_data->alarm_pending == true) ? 1 : 0;

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

	ARG_UNUSED(dev_data);

#if defined(CONFIG_RTC_ALARM)
	if (rtc_read(RTC_FLAG) & RTC_AF_BIT) {
		if (dev_data->cb) {
			dev_data->cb(dev, 0, dev_data->cb_data);
			dev_data->alarm_pending = false;
		}
	}
#endif

#if defined(CONFIG_RTC_UPDATE)
	if (rtc_read(RTC_FLAG) & RTC_UEF_BIT) {
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

static int rtc_mc146818_init(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
			DT_INST_IRQ(0, priority),
			rtc_mc146818_isr, NULL,
			DT_INST_IRQ(0, sense));
	irq_enable(DT_INST_IRQN(0));

	return 0;
}

#define RTC_MC146818_DEV_CFG(n)							\
	static struct rtc_mc146818_data rtc_data_##n = {				\
			.alarms_count = DT_INST_PROP(n, alarms_count),		\
			.mask = 0,						\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &rtc_mc146818_init, NULL, &rtc_data_##n,		\
				NULL, POST_KERNEL,				\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
				&rtc_mc146818_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(RTC_MC146818_DEV_CFG)
