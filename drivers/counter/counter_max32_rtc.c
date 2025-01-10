/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_rtc_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <rtc.h>
#include <wrap_max32_lp.h>

/* Resoultion is 1sec for time of day alarm*/
#define MAX32_RTC_COUNTER_FREQ 1

/* 20bits used for time of day alarm */
#define MAX32_RTC_COUNTER_MAX_VALUE ((1 << 21) - 1)

#define MAX32_RTC_COUNTER_INT_FL MXC_RTC_INT_FL_LONG
#define MAX32_RTC_COUNTER_INT_EN MXC_RTC_INT_EN_LONG

struct max32_rtc_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
};

struct max32_rtc_config {
	struct counter_config_info info;
	mxc_rtc_regs_t *regs;
	void (*irq_func)(void);
};

static int api_start(const struct device *dev)
{
	while (MXC_RTC_Start() == E_BUSY) {
		;
	}

	while (MXC_RTC_EnableInt(MAX32_RTC_COUNTER_INT_EN) == E_BUSY) {
		;
	}

	return 0;
}

static int api_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	while (MXC_RTC_DisableInt(MAX32_RTC_COUNTER_INT_EN) == E_BUSY) {
		;
	}
	MXC_RTC_Stop();

	return 0;
}

static int api_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct max32_rtc_config *cfg = dev->config;
	mxc_rtc_regs_t *regs = cfg->regs;
	uint32_t sec = 0, subsec = 0;

	/* Read twice incase of glitch */
	sec = regs->sec;
	if (regs->sec != sec) {
		sec = regs->sec;
	}

	/* Read twice incase of glitch */
	subsec = regs->ssec;
	if (regs->ssec != subsec) {
		subsec = regs->ssec;
	}

	*ticks = sec;
	if (subsec >= (MXC_RTC_MAX_SSEC / 2)) {
		*ticks += 1;
	}

	return 0;
}

static int api_set_top_value(const struct device *dev, const struct counter_top_cfg *counter_cfg)
{
	const struct max32_rtc_config *cfg = dev->config;
	struct max32_rtc_data *const data = dev->data;

	if (counter_cfg->ticks == 0) {
		return -EINVAL;
	}

	if (counter_cfg->ticks != cfg->info.max_top_value) {
		return -ENOTSUP;
	}

	data->top_callback = counter_cfg->callback;
	data->top_user_data = counter_cfg->user_data;

	return 0;
}

static uint32_t api_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);
	int flags = MXC_RTC_GetFlags();

	if (flags & MAX32_RTC_COUNTER_INT_FL) {
		return 1;
	}

	return 0;
}

static uint32_t api_get_top_value(const struct device *dev)
{
	const struct max32_rtc_config *cfg = dev->config;

	return cfg->info.max_top_value;
}

static int api_set_alarm(const struct device *dev, uint8_t chan,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	int ret;
	struct max32_rtc_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t current;

	/* Alarm frequenct is 1Hz so that it seems ticks becomes 0
	 * some times, in that case system being blocked.
	 * Set it to 1 if ticks is 0
	 */
	if (ticks == 0) {
		ticks = 1;
	}

	if (alarm_cfg->ticks > api_get_top_value(dev)) {
		return -EINVAL;
	}

	if (data->alarm_callback != NULL) {
		return -EBUSY;
	}

	api_stop(dev);

	api_get_value(dev, &current);
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	ret = MXC_RTC_SetTimeofdayAlarm(ticks);
	if (ret == E_BUSY) {
		ret = -EBUSY;
	}

	if (ret == 0) {
		data->alarm_callback = alarm_cfg->callback;
		data->alarm_user_data = alarm_cfg->user_data;
	}

	api_start(dev);

	return ret;
}

static int api_cancel_alarm(const struct device *dev, uint8_t chan)
{
	struct max32_rtc_data *data = dev->data;

	while (MXC_RTC_DisableInt(MAX32_RTC_COUNTER_INT_EN) == E_BUSY) {
		;
	}
	data->alarm_callback = NULL;

	return 0;
}

static void rtc_max32_isr(const struct device *dev)
{
	struct max32_rtc_data *const data = dev->data;
	int flags = MXC_RTC_GetFlags();

	if (flags & MAX32_RTC_COUNTER_INT_FL) {
		if (data->alarm_callback) {
			counter_alarm_callback_t cb;
			uint32_t current;

			api_get_value(dev, &current);

			cb = data->alarm_callback;
			data->alarm_callback = NULL;
			cb(dev, 0, current, data->alarm_user_data);
		}
	}

	/* Clear all flags */
	MXC_RTC_ClearFlags(flags);
}

static int rtc_max32_init(const struct device *dev)
{
	const struct max32_rtc_config *cfg = dev->config;

	while (MXC_RTC_Init(0, 0) == E_BUSY) {
		;
	}

	api_stop(dev);

	cfg->irq_func();

	return 0;
}

static DEVICE_API(counter, counter_rtc_max32_driver_api) = {
	.start = api_start,
	.stop = api_stop,
	.get_value = api_get_value,
	.set_top_value = api_set_top_value,
	.get_pending_int = api_get_pending_int,
	.get_top_value = api_get_top_value,
	.set_alarm = api_set_alarm,
	.cancel_alarm = api_cancel_alarm,
};

#define COUNTER_RTC_MAX32_INIT(_num)                                                               \
	static struct max32_rtc_data rtc_max32_data_##_num;                                        \
	static void max32_rtc_irq_init_##_num(void)                                                \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority), rtc_max32_isr,        \
			    DEVICE_DT_INST_GET(_num), 0);                                          \
		irq_enable(DT_INST_IRQN(_num));                                                    \
		if (DT_INST_PROP(_num, wakeup_source)) {                                           \
			MXC_LP_EnableRTCAlarmWakeup();                                             \
		}                                                                                  \
	};                                                                                         \
	static const struct max32_rtc_config rtc_max32_config_##_num = {                           \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = MAX32_RTC_COUNTER_MAX_VALUE,                      \
				.freq = MAX32_RTC_COUNTER_FREQ,                                    \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.regs = (mxc_rtc_regs_t *)DT_INST_REG_ADDR(_num),                                  \
		.irq_func = max32_rtc_irq_init_##_num,                                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_num, &rtc_max32_init, NULL, &rtc_max32_data_##_num,                 \
			      &rtc_max32_config_##_num, PRE_KERNEL_1,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rtc_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RTC_MAX32_INIT)
