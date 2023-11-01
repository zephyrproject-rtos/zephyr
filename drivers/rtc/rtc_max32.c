/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_rtc

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/spinlock.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/timeutil.h>

#include <rtc.h>
#include <time.h>

#include "rtc_utils.h"

#define MSEC_TO_NSEC(x) ((x) * 1000000)

/* Converts a time in milleseconds to the equivalent RSSA register value. */
#define NSEC_TO_MSEC(x) ((x) / 1000000)
#define NSEC_TO_RSSA(x) (0 - ((NSEC_TO_MSEC(x) * 4096) / 1000))

#define MAX_PPB 127
#define MIN_PPB -127

#define SECS_PER_MIN  60
#define SECS_PER_HOUR (60 * SECS_PER_MIN)
#define SECS_PER_DAY  (24 * SECS_PER_HOUR)
#define SECS_PER_WEEK (7 * SECS_PER_DAY)

#define MAX_ALARM_SEC SECS_PER_WEEK - 1
#define RTC_ALARM_MASK                                                                             \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

struct max32_rtc_data {
	struct k_spinlock lock;
	uint16_t alarms_count;
	uint16_t mask;
#ifdef CONFIG_RTC_ALARM
	bool alarm_pending;
	rtc_alarm_callback alarm_cb;
	void *alarm_cb_data;
	time_t alarm_sec;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_cb;
	void *update_cb_data;
#endif /* CONFIG_RTC_UPDATE */
};

struct max32_rtc_config {
	mxc_rtc_regs_t *regs;
	void (*irq_func)(void);
};

static inline void convert_to_rtc_time(uint32_t sec, uint32_t subsec, struct rtc_time *timeptr)
{
	time_t tm_t = sec;

	gmtime_r(&tm_t, (struct tm *)timeptr);

	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;
	/* add subsec too */
	if (subsec >= (MXC_RTC_MAX_SSEC / 2)) {
		timeptr->tm_sec += 1;
	}
}

static int api_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	time_t sec;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	sec = timeutil_timegm((struct tm *)timeptr);
	if (sec == -1) {
		return -EINVAL;
	}

	while (MXC_RTC_Init((uint32_t)sec, NSEC_TO_RSSA(timeptr->tm_nsec)) == E_BUSY) {
		;
	}

#ifdef CONFIG_RTC_ALARM
	struct max32_rtc_data *data = dev->data;
	uint32_t alarm_time = 0;

	if (data->alarm_sec != -1) {
		while (MXC_RTC_DisableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {
			;
		}

		if (data->alarm_sec < SECS_PER_MIN) { /* less than a minute */
			alarm_time = sec - (sec % SECS_PER_MIN) + (data->alarm_sec);
			if ((sec % SECS_PER_MIN) > data->alarm_sec) {
				alarm_time += SECS_PER_MIN;
			}
		} else if (data->alarm_sec < SECS_PER_HOUR) { /* less than a hour */
			alarm_time = sec - (sec % SECS_PER_HOUR) + (data->alarm_sec);
			if ((sec % SECS_PER_HOUR) > data->alarm_sec) {
				alarm_time += SECS_PER_HOUR;
			}
		} else if (data->alarm_sec < SECS_PER_DAY) { /* less than a day */
			alarm_time = sec - (sec % SECS_PER_DAY) + (data->alarm_sec);
			if ((sec % SECS_PER_DAY) > data->alarm_sec) {
				alarm_time += SECS_PER_DAY;
			}
		} else if (data->alarm_sec < SECS_PER_WEEK) { /* less than a week */
			uint8_t wday_alarm = (data->alarm_sec) / SECS_PER_DAY;

			alarm_time = sec -
				     ((timeptr->tm_wday * SECS_PER_DAY) + (sec % SECS_PER_DAY)) +
				     (data->alarm_sec);
			if (timeptr->tm_wday > wday_alarm) {
				alarm_time += SECS_PER_WEEK;
			}
		}

		MXC_RTC_SetTimeofdayAlarm(alarm_time);
		while (MXC_RTC_EnableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {
			;
		}
	}
#endif /* CONFIG_RTC_ALARM */

	while (MXC_RTC_Start() == E_BUSY) {
		;
	}

	return 0;
}

static int api_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	ARG_UNUSED(dev);
	uint32_t sec, subsec;

	while (MXC_RTC_GetTime(&sec, &subsec) != E_NO_ERROR) {
		;
	}
	convert_to_rtc_time(sec, subsec, timeptr);
	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int api_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	struct max32_rtc_data *data = dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	*mask = RTC_ALARM_MASK;

	return 0;
}

static int api_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
			      const struct rtc_time *timeptr)
{
	struct max32_rtc_data *const data = dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	if (mask & ~RTC_ALARM_MASK) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		return -EINVAL;
	}

	/* means disable alarm */
	if (mask == 0) {
		data->mask = 0;
		data->alarm_sec = -1;
		while (MXC_RTC_DisableInt(MXC_RTC_INT_EN_LONG) == E_BUSY) {
			;
		}
		return 0;
	}

	if (timeptr == NULL) {
		return -EINVAL;
	}

	/* Calculating sec number from timeptr */
	time_t alarm_sec = 0;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		alarm_sec += timeptr->tm_sec;
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		alarm_sec += (SECS_PER_MIN * timeptr->tm_min);
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		alarm_sec += (SECS_PER_HOUR * timeptr->tm_hour);
	}
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		alarm_sec += (SECS_PER_DAY * timeptr->tm_wday);
	}

	if (alarm_sec > MAX_ALARM_SEC) {
		return -EINVAL;
	}

	data->mask = mask;
	data->alarm_sec = alarm_sec;

	return 0;
}

static int api_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
			      struct rtc_time *timeptr)
{
	struct max32_rtc_data *const data = dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	if (timeptr == NULL) {
		return -EINVAL;
	}

	convert_to_rtc_time(data->alarm_sec, 0, timeptr);
	*mask = data->mask;

	return 0;
}

static int api_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct max32_rtc_data *const data = dev->data;
	int ret;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	ret = (data->alarm_pending == true) ? 1 : 0;
	data->alarm_pending = false;

	return ret;
}

static int api_alarm_set_callback(const struct device *dev, uint16_t id,
				  rtc_alarm_callback callback, void *user_data)
{
	struct max32_rtc_data *const data = dev->data;

	if (data->alarms_count <= id) {
		return -EINVAL;
	}

	data->alarm_cb = callback;
	data->alarm_cb_data = user_data;

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int api_update_set_callback(const struct device *dev, rtc_update_callback callback,
				   void *user_data)
{
	struct max32_rtc_data *const data = dev->data;
	const uint32_t sub_sec_alarm_max = 0xFFFFFFFF;

	while (MXC_RTC_SetSubsecondAlarm(sub_sec_alarm_max - MXC_RTC_MAX_SSEC) == E_BUSY) {
		;
	}

	data->update_cb = callback;
	data->update_cb_data = user_data;
	if (data->update_cb == NULL) {
		while (MXC_RTC_DisableInt(MXC_RTC_INT_EN_SHORT) == E_BUSY) {
			;
		}
	} else {
		while (MXC_RTC_EnableInt(MXC_RTC_INT_EN_SHORT) == E_BUSY) {
			;
		}
	}

	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION
static int api_set_calibration(const struct device *dev, int32_t calibration)
{
	ARG_UNUSED(dev);
	if ((calibration > MAX_PPB) || (calibration < MIN_PPB)) {
		return -EINVAL;
	}

	while (MXC_RTC_Trim(calibration) == E_BUSY) {
		;
	}

	return 0;
}

static int api_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct max32_rtc_config *cfg = dev->config;

	*calibration = cfg->regs->trim & MXC_F_RTC_TRIM_TRIM;

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

static void rtc_max32_isr(const struct device *dev)
{
#if defined(CONFIG_RTC_ALARM) || (CONFIG_RTC_UPDATE)
	struct max32_rtc_data *const data = dev->data;
	int flags = MXC_RTC_GetFlags();
#else
	ARG_UNUSED(dev);
#endif

#ifdef CONFIG_RTC_ALARM
	if (flags & MXC_RTC_INT_FL_LONG) {
		if (data->alarm_cb) {
			data->alarm_cb(dev, 0, data->alarm_cb_data);
			data->alarm_pending = false;
		} else {
			data->alarm_pending = true;
		}
		MXC_RTC_ClearFlags(MXC_RTC_INT_FL_LONG);
	}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	if (flags & MXC_RTC_INT_FL_SHORT) {
		if (data->update_cb) {
			data->update_cb(dev, data->update_cb_data);
		}
		MXC_RTC_ClearFlags(MXC_RTC_INT_FL_SHORT);
	}
#endif /* CONFIG_RTC_UPDATE */
}

const struct rtc_driver_api rtc_max32_driver_api = {
	.set_time = api_set_time,
	.get_time = api_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = api_alarm_get_supported_fields,
	.alarm_set_time = api_alarm_set_time,
	.alarm_get_time = api_alarm_get_time,
	.alarm_is_pending = api_alarm_is_pending,
	.alarm_set_callback = api_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = api_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = api_set_calibration,
	.get_calibration = api_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

static int rtc_max32_init(const struct device *dev)
{
#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	const struct max32_rtc_config *cfg = dev->config;

	cfg->irq_func();
#endif /* CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE */
	return 0;
}

#define RTC_MAX32_INIT(_num)                                                                       \
	static void max32_rtc_irq_init_##_num(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority), rtc_max32_isr,        \
			    DEVICE_DT_INST_GET(_num), 0);                                          \
		irq_enable(DT_INST_IRQN(_num));                                                    \
	};                                                                                         \
	static const struct max32_rtc_config rtc_max32_config_##_num = {                           \
		.regs = (mxc_rtc_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.irq_func = max32_rtc_irq_init_##_num,                                             \
	};                                                                                         \
	static struct max32_rtc_data rtc_data_##_num = {                                           \
		.alarms_count = DT_INST_PROP(_num, alarms_count),                                  \
		.mask = 0,                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, &rtc_max32_init, NULL, &rtc_data_##_num,                       \
			      &rtc_max32_config_##_num, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rtc_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_MAX32_INIT)
