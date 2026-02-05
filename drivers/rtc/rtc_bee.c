/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_rtc

#include <stdbool.h>
#include "time.h"
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/timeutil.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_rtc.h>
#include <rtl_rcc.h>
#include <rtl_nvic.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_rtc.h>
#include <rtl876x_rcc.h>
#include <rtl876x_nvic.h>
#include <vector_table.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtc_bee, CONFIG_RTC_LOG_LEVEL);

#ifdef CONFIG_RTC_ALARM
struct rtc_bee_ch_data {
	rtc_alarm_callback callback;
	void *user_data;
	bool enabled;
	bool pending;
	time_t alarm_time_sec;
	time_t alarm_time_nsec;
	uint16_t mask;
	struct rtc_time origin_time;
	uint32_t cycles;
	struct k_mutex lock;
};
#endif

#ifdef CONFIG_RTC_UPDATE
struct rtc_bee_updata_data {
	rtc_update_callback callback;
	void *user_data;
	bool enabled;
	uint32_t cur_cnt;
};
#endif

struct rtc_bee_data {
	uint32_t freq;
	time_t last_update_time_sec;
	time_t last_update_time_nsec;
	uint32_t last_update_rtc_cnt;
	struct rtc_time last_update_time;
	struct k_mutex lock;
#ifdef CONFIG_RTC_UPDATE
	struct rtc_bee_updata_data update;
#endif
#ifdef CONFIG_RTC_ALARM
	struct rtc_bee_ch_data *alarm;
#endif
};

struct rtc_bee_config {
	uint32_t reg;
	uint32_t src_freq;
	uint32_t prescaler;
	uint8_t channels;
	void (*irq_config)(const struct device *dev);
	void (*set_irq_pending)(void);
	uint32_t (*get_irq_pending)(void);
};

#ifdef CONFIG_RTC_ALARM
static const uint32_t rtc_cmp_int_table[] = {RTC_INT_COMP0, RTC_INT_COMP1, RTC_INT_COMP2,
					     RTC_INT_COMP3};
#endif

static time_t cnt2sec(const struct device *dev, uint32_t cnt)
{
	struct rtc_bee_data *data = dev->data;

	return cnt / data->freq;
}

static time_t cnt2nsec(const struct device *dev, uint32_t cnt)
{
	struct rtc_bee_data *data = dev->data;

	return (uint64_t)cnt * NSEC_PER_SEC / data->freq;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_bee_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr);

static uint32_t sec2cnt(const struct device *dev, time_t sec)
{
	struct rtc_bee_data *data = dev->data;

	return sec * data->freq;
}

static uint32_t nsec2cnt(const struct device *dev, time_t nsec)
{
	struct rtc_bee_data *data = dev->data;

	return nsec * data->freq / NSEC_PER_SEC;
}
#endif

static int rtc_bee_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_bee_config *cfg = dev->config;
	struct rtc_bee_data *data = dev->data;

	int err = 0;

	err = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (err) {
		return err;
	}

	RTC_INTConfig(RTC_INT_OVF, DISABLE);

	data->last_update_time_sec = timeutil_timegm((struct tm *)timeptr);
	data->last_update_time_nsec = timeptr->tm_nsec;

	data->last_update_rtc_cnt = RTC_GetCounter();

#ifdef CONFIG_RTC_ALARM
	/* update alarm time if set alarm before set time*/
	for (uint8_t i = 0; i < cfg->channels; i++) {
		if (data->alarm[i].enabled) {
			rtc_bee_alarm_set_time(dev, i, data->alarm[i].mask,
					       &(data->alarm[i].origin_time));
		}
	}
#endif

	RTC_INTConfig(RTC_INT_OVF, ENABLE);

	k_mutex_unlock(&data->lock);

	return err;
}

static int rtc_bee_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_bee_data *data = dev->data;
	uint32_t elapsed_cnt;
	time_t temp_time_sec;
	time_t temp_time_nsec;

	if (data->last_update_rtc_cnt != UINT32_MAX) {
		elapsed_cnt = RTC_GetCounter() - data->last_update_rtc_cnt;
	} else {
		elapsed_cnt = RTC_GetCounter();
	}

	temp_time_sec = cnt2sec(dev, elapsed_cnt);
	temp_time_nsec = cnt2nsec(dev, elapsed_cnt - temp_time_sec * data->freq);
	temp_time_nsec += data->last_update_time_nsec;
	temp_time_sec = temp_time_sec + data->last_update_time_sec + temp_time_nsec / NSEC_PER_SEC;
	temp_time_nsec = temp_time_nsec % NSEC_PER_SEC;

	gmtime_r(&temp_time_sec, (struct tm *)timeptr);

	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = temp_time_nsec;

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_bee_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	const struct rtc_bee_config *cfg = dev->config;

	if (id > cfg->channels) {
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_NSEC | RTC_ALARM_TIME_MASK_SECOND |
		 RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
		 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH |
		 RTC_ALARM_TIME_MASK_YEAR);

	return 0;
}

static int rtc_bee_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	const struct rtc_bee_config *cfg = dev->config;
	struct rtc_bee_data *data = dev->data;
	time_t temp_time_sec;
	time_t temp_time_nsec;
	time_t cur_time_sec;
	time_t cur_time_nsec;
	time_t alarm_sec;
	time_t alarm_nsec;
	struct rtc_time temp_time;
	struct rtc_time cur_time;
	uint32_t alarm_cnt;

	if (id > cfg->channels || (mask && (timeptr == 0))) {
		return -EINVAL;
	}

	if ((mask & RTC_ALARM_TIME_MASK_SECOND && timeptr->tm_sec > 60) ||
	    (mask & RTC_ALARM_TIME_MASK_MINUTE && timeptr->tm_min > 60) ||
	    (mask & RTC_ALARM_TIME_MASK_HOUR && timeptr->tm_hour > 24) ||
	    (mask & RTC_ALARM_TIME_MASK_MONTHDAY && timeptr->tm_mday > 31) ||
	    (mask & RTC_ALARM_TIME_MASK_MONTH && timeptr->tm_mon > 11) ||
	    (mask & RTC_ALARM_TIME_MASK_YEAR && timeptr->tm_year > 7100) ||
	    (mask & RTC_ALARM_TIME_MASK_WEEKDAY && timeptr->tm_wday > 7) ||
	    (mask & RTC_ALARM_TIME_MASK_YEARDAY && timeptr->tm_yday > 365) ||
	    (mask & RTC_ALARM_TIME_MASK_NSEC && timeptr->tm_nsec > 1000000000)) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	if (mask == 0 || timeptr == 0) {
		RTC_INTConfig(rtc_cmp_int_table[id], DISABLE);

		/* clear pending if callback is called or if alarm is disabled */
		data->alarm[id].alarm_time_sec = 0;
		data->alarm[id].alarm_time_nsec = 0;
		data->alarm[id].pending = false;
		data->alarm[id].enabled = false;

		irq_unlock(key);
		return 0;
	}

	rtc_bee_get_time(dev, &cur_time);
	cur_time_sec = timeutil_timegm((struct tm *)(&cur_time));
	cur_time_nsec = cur_time.tm_nsec;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		temp_time.tm_sec = timeptr->tm_sec;
	} else {
		temp_time.tm_sec = 0;
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		temp_time.tm_min = timeptr->tm_min;
	} else {
		temp_time.tm_min = cur_time.tm_min;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		temp_time.tm_hour = timeptr->tm_hour;
	} else {
		temp_time.tm_hour = cur_time.tm_hour;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		temp_time.tm_mday = timeptr->tm_mday;
	} else {
		temp_time.tm_mday = cur_time.tm_mday;
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		temp_time.tm_mon = timeptr->tm_mon;
	} else {
		temp_time.tm_mon = cur_time.tm_mon;
	}

	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		temp_time.tm_year = timeptr->tm_year;
	} else {
		temp_time.tm_year = cur_time.tm_year;
	}

	if (mask & RTC_ALARM_TIME_MASK_NSEC) {
		temp_time.tm_nsec = timeptr->tm_nsec;
	} else {
		temp_time.tm_nsec = cur_time.tm_nsec;
	}

	temp_time_sec = timeutil_timegm((struct tm *)(&temp_time));
	temp_time_nsec = temp_time.tm_nsec;

	alarm_nsec = (temp_time_sec * NSEC_PER_SEC + temp_time_nsec) -
		     (cur_time_sec * NSEC_PER_SEC + cur_time_nsec);
	alarm_sec = alarm_nsec / NSEC_PER_SEC;
	alarm_nsec = alarm_nsec - alarm_sec * NSEC_PER_SEC;

	alarm_cnt = sec2cnt(dev, alarm_sec) + nsec2cnt(dev, alarm_nsec);

	data->alarm[id].mask = mask;
	data->alarm[id].alarm_time_sec = alarm_sec;
	data->alarm[id].alarm_time_nsec = alarm_nsec;
	data->alarm[id].origin_time = *timeptr;
	data->alarm[id].pending = false;
	data->alarm[id].enabled = true;

	RTC_SetCompValue(id, RTC_GetCounter() + alarm_cnt);

	RTC_INTConfig(rtc_cmp_int_table[id], ENABLE);

	irq_unlock(key);

	return 0;
}

static int rtc_bee_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	const struct rtc_bee_config *cfg = dev->config;
	struct rtc_bee_data *data = dev->data;

	if (id > cfg->channels || !timeptr) {
		return -EINVAL;
	}

	if (data->alarm[id].enabled) {
		*mask = data->alarm[id].mask;
		*timeptr = data->alarm[id].origin_time;
	}

	return 0;
}

static int rtc_bee_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_bee_data *data = dev->data;

	return data->alarm[id].pending;
}

static int rtc_bee_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
	const struct rtc_bee_config *cfg = dev->config;
	struct rtc_bee_data *data = dev->data;

	if (id > cfg->channels) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	data->alarm[id].callback = callback;
	data->alarm[id].user_data = user_data;

	irq_unlock(key);

	return 0;
}
#endif

#ifdef CONFIG_RTC_UPDATE
static int rtc_bee_update_set_callback(const struct device *dev, rtc_update_callback callback,
				       void *user_data)
{
	struct rtc_bee_data *data = dev->data;

	data->update.callback = callback;
	data->update.user_data = user_data;

	if (callback == NULL) {
		data->update.enabled = false;
		RTC_INTConfig(RTC_INT_TICK, DISABLE);

		return 0;
	}

	data->update.enabled = true;
	RTC_INTConfig(RTC_INT_TICK, ENABLE);

	return 0;
}
#endif

#ifdef CONFIG_RTC_ALARM
static void alarm_irq_handle(const struct device *dev, uint32_t chan)
{
	struct rtc_bee_data *data = dev->data;
	struct rtc_bee_ch_data *alarm = &data->alarm[chan];
	rtc_alarm_callback cb;
	struct rtc_time cur_time;
	time_t cur_time_sec;
	time_t cur_time_nsec;

	rtc_bee_get_time(dev, &cur_time);
	cur_time_sec = timeutil_timegm((struct tm *)(&cur_time));
	cur_time_nsec = cur_time.tm_nsec;

	cb = alarm->callback;

	if (cur_time_sec > alarm->alarm_time_sec ||
	    (cur_time_sec == alarm->alarm_time_sec && cur_time_nsec >= alarm->alarm_time_nsec)) {
		rtc_bee_alarm_set_time(dev, chan, data->alarm[chan].mask,
				       &(data->alarm[chan].origin_time));
		alarm->pending = true;
		if (cb) {
			cb(dev, chan, alarm->user_data);

			/* clear pending if callback is called or if alarm is disabled */
			alarm->pending = false;
		}
	}

	RTC_ClearCompINT(chan);
}
#endif

static int rtc_bee_init(const struct device *dev)
{
	const struct rtc_bee_config *cfg = dev->config;
	struct rtc_bee_data *data = dev->data;

	__ASSERT(cfg->prescaler <= 4096, "rtc prescaler should be less than 4096");

	data->freq = cfg->src_freq / cfg->prescaler;

	RTC_DeInit();
	RTC_SetPrescaler(cfg->prescaler - 1);
	RTC_ResetPrescalerCounter();
	RTC_ResetCounter();
	RTC_INTConfig(RTC_INT_OVF, ENABLE);
	RTC_NvCmd(ENABLE);
	RTC_Cmd(ENABLE);

	cfg->irq_config(dev);

	return 0;
}

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
static void rtc_irq_handler(const struct device *dev)
{
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
static void rtc_irq_handler(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
#endif
	const struct rtc_bee_config *cfg = dev->config;
	struct rtc_bee_data *data = dev->data;
	time_t temp_time_sec;
	time_t temp_time_nsec;

	if (RTC_GetINTStatus(RTC_INT_OVF) == SET) {
		/* updata tm */
		if (data->last_update_rtc_cnt != UINT32_MAX) {
			/* first time enter isr after set time */
			temp_time_sec = cnt2sec(dev, ~data->last_update_rtc_cnt);
			temp_time_nsec = cnt2nsec(dev, ~data->last_update_rtc_cnt -
							       temp_time_sec * data->freq);
			data->last_update_time_nsec += temp_time_nsec;
			data->last_update_time_sec = data->last_update_time_sec + temp_time_sec +
						     data->last_update_time_nsec / NSEC_PER_SEC;
			data->last_update_time_nsec = data->last_update_time_nsec % NSEC_PER_SEC;

			data->last_update_rtc_cnt = UINT32_MAX;
		} else {
			temp_time_sec = cnt2sec(dev, UINT32_MAX);
			temp_time_nsec = cnt2nsec(dev, UINT32_MAX - temp_time_sec * data->freq);
			data->last_update_time_nsec += temp_time_nsec;
			data->last_update_time_sec = data->last_update_time_sec + temp_time_sec +
						     data->last_update_time_nsec / NSEC_PER_SEC;
			data->last_update_time_nsec = data->last_update_time_nsec % NSEC_PER_SEC;
		}

		RTC_ClearINTPendingBit(RTC_INT_OVF);
	}

#ifdef CONFIG_RTC_ALARM
	for (uint32_t i = 0; i < cfg->channels; i++) {
		if (RTC_GetINTStatus(rtc_cmp_int_table[i])) {
			alarm_irq_handle(dev, i);
		}
	}
#endif

#ifdef CONFIG_RTC_UPDATE
	if (RTC_GetINTStatus(RTC_INT_TICK) == SET) {
		data->update.cur_cnt++;

		if (data->update.cur_cnt >= data->freq) {
			data->update.cur_cnt = 0;
			if (data->update.callback) {
				data->update.callback(dev, data->update.user_data);
			}
		}

		RTC_ClearINTPendingBit(RTC_INT_TICK);
	}
#endif
}

static DEVICE_API(rtc, rtc_bee_driver_api) = {
	.set_time = rtc_bee_set_time,
	.get_time = rtc_bee_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_bee_alarm_get_supported_fields,
	.alarm_set_time = rtc_bee_alarm_set_time,
	.alarm_get_time = rtc_bee_alarm_get_time,
	.alarm_is_pending = rtc_bee_alarm_is_pending,
	.alarm_set_callback = rtc_bee_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_bee_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */

	/* RTC_CALIBRATION not supported */
};

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#define RTC_IRQ_CONFIG_FUNC(index)                                                                 \
	static void irq_config_##index(const struct device *dev)                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), rtc_irq_handler,    \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#define RTC_IRQ_CONFIG_FUNC(index)                                                                 \
	static void irq_config_##index(const struct device *dev)                                   \
	{                                                                                          \
		RamVectorTableUpdate(RTC_VECTORn, rtc_irq_handler);                                \
		NVIC_InitTypeDef NVIC_InitStruct;                                                  \
		NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;                                        \
		NVIC_InitStruct.NVIC_IRQChannelPriority = 2;                                       \
		NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;                                       \
		NVIC_Init(&NVIC_InitStruct);                                                       \
	}
#endif

#define RTC_IRQ_CONFIG(index)                                                                      \
	RTC_IRQ_CONFIG_FUNC(index);                                                                \
	static void set_irq_pending_##index(void)                                                  \
	{                                                                                          \
		(NVIC_SetPendingIRQ(DT_INST_IRQN(index)));                                         \
	}                                                                                          \
	static uint32_t get_irq_pending_##index(void)                                              \
	{                                                                                          \
		return NVIC_GetPendingIRQ(DT_INST_IRQN(index));                                    \
	}

#ifdef CONFIG_RTC_ALARM
#define RTC_BEE_ALARM_DEFINE(index)                                                                \
	static struct rtc_bee_ch_data rtc_bee_alarm_data_##index[DT_INST_PROP(index, alarms_count)];
#else
#define RTC_BEE_ALARM_DEFINE(index)
#endif

#ifdef CONFIG_RTC_ALARM
#define RTC_BEE_ALARM_INIT(index) .alarm = rtc_bee_alarm_data_##index,
#else
#define RTC_BEE_ALARM_INIT(index)
#endif

#ifdef CONFIG_RTC_UPDATE
#define RTC_BEE_UPDATE_INIT                                                                        \
	.update = {                                                                                \
		.callback = NULL,                                                                  \
		.user_data = NULL,                                                                 \
		.enabled = false,                                                                  \
		.cur_cnt = 0,                                                                      \
	},
#else
#define RTC_BEE_UPDATE_INIT
#endif

#define RTC_BEE_INIT(index)                                                                        \
	RTC_IRQ_CONFIG(index);                                                                     \
	RTC_BEE_ALARM_DEFINE(index)                                                                \
	static struct rtc_bee_data rtc_bee_data_##index = {                                        \
		RTC_BEE_UPDATE_INIT RTC_BEE_ALARM_INIT(index)};                                    \
	static const struct rtc_bee_config rtc_bee_config_##index = {                              \
		.reg = DT_INST_REG_ADDR(index),                                                    \
		.src_freq = DT_INST_PROP_OR(index, src_clk_freq, 32000),                           \
		.prescaler = DT_INST_PROP(index, prescaler),                                       \
		.channels = DT_INST_PROP(index, alarms_count),                                     \
		.irq_config = irq_config_##index,                                                  \
		.set_irq_pending = set_irq_pending_##index,                                        \
		.get_irq_pending = get_irq_pending_##index,                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, rtc_bee_init, NULL, &rtc_bee_data_##index,                    \
			      &rtc_bee_config_##index, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,      \
			      &rtc_bee_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_BEE_INIT);
