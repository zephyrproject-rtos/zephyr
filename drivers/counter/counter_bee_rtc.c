/*
 * Copyright(c) 2026, Realtek Semiconductor Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_bee_counter_rtc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/bee_clock_control.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include <zephyr/sys/atomic.h>

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#include <rtl_rtc.h>
#include <rtl_rcc.h>
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#include <rtl876x_rtc.h>
#include <rtl876x_rcc.h>
#include <rtl876x_nvic.h>
#include <vector_table.h>
#else
#error "Unsupported Realtek Bee SoC series"
#endif

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_bee_rtc, CONFIG_COUNTER_LOG_LEVEL);

struct counter_bee_rtc_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct counter_bee_rtc_data {
	counter_top_callback_t top_cb;
	void *top_user_data;
	uint32_t guard_period;
	uint32_t freq;
	bool enabled;
	struct counter_bee_rtc_ch_data alarm[];
};

struct counter_bee_rtc_config {
	struct counter_config_info counter_info;
	uint32_t reg;
	uint32_t src_clk_freq;
	uint16_t prescaler;
	void (*irq_config)(void);
	void (*set_irq_pending)(void);
	uint32_t (*get_irq_pending)(void);
};

static const uint32_t rtc_cmp_int_table[] = {RTC_INT_COMP0, RTC_INT_COMP1, RTC_INT_COMP2,
					     RTC_INT_COMP3};

static bool rtc_late_int_table[] = {false, false, false, false};

static int counter_bee_rtc_start(const struct device *dev)
{
	struct counter_bee_rtc_data *data = dev->data;

	RTC_Cmd(ENABLE);
	data->enabled = true;

	return 0;
}

static int counter_bee_rtc_stop(const struct device *dev)
{
	struct counter_bee_rtc_data *data = dev->data;

	RTC_Cmd(DISABLE);
	RTC_ResetCounter();
	RTC_ResetPrescalerCounter();
	data->enabled = false;

	return 0;
}

static int counter_bee_rtc_get_value(const struct device *dev, uint32_t *ticks)
{
	struct counter_bee_rtc_data *data = dev->data;

	*ticks = RTC_GetCounter();

	/* Use a software flag to return cnt=0 after reset, avoiding the time-consuming 1T hardware
	 * clear delay caused by the low RTC frequency
	 */
	if (!data->enabled) {
		*ticks = 0;
	}

	LOG_DBG("ticks=%d\n", *ticks);

	return 0;
}

static uint32_t counter_bee_rtc_get_top_value(const struct device *dev)
{
	const struct counter_bee_rtc_config *cfg = dev->config;

	LOG_DBG("topvalue=%x\n", cfg->counter_info.max_top_value);

	return cfg->counter_info.max_top_value;
}

static int counter_bee_rtc_set_alarm(const struct device *dev, uint8_t chan,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	struct counter_bee_rtc_data *data = dev->data;
	__maybe_unused const struct counter_bee_rtc_config *cfg = dev->config;
	struct counter_bee_rtc_ch_data *chdata = &data->alarm[chan];
	uint32_t now;
	__maybe_unused uint32_t diff;

	if (alarm_cfg->ticks > counter_bee_rtc_get_top_value(dev)) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;

	if (!(alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE)) {
		RTC_INTConfig(rtc_cmp_int_table[chan], ENABLE);
		counter_bee_rtc_get_value(dev, &now);
		RTC_SetCompValue(chan, now + (alarm_cfg->ticks < 2 ? 2 : alarm_cfg->ticks));
		return 0;
	}

	counter_bee_rtc_get_value(dev, &now);
	if (alarm_cfg->ticks > now) {
		RTC_INTConfig(rtc_cmp_int_table[chan], ENABLE);
		RTC_SetCompValue(chan, alarm_cfg->ticks);
		return 0;
	}

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
		counter_bee_rtc_get_value(dev, &now);
		diff = now - alarm_cfg->ticks;
		LOG_DBG("diff=%d, guard_period=%d\n", diff, data->guard_period);
		if (diff < data->guard_period) {
			rtc_late_int_table[chan] = true;
			cfg->set_irq_pending();
			RTC_INTConfig(rtc_cmp_int_table[chan], DISABLE);
			return -ETIME;
		}
	}
#endif

	chdata->callback = NULL;

	RTC_INTConfig(rtc_cmp_int_table[chan], DISABLE);

	return -ETIME;
}

static int counter_bee_rtc_cancel_alarm(const struct device *dev, uint8_t chan)
{
	struct counter_bee_rtc_data *data = dev->data;

	RTC_INTConfig(rtc_cmp_int_table[chan], DISABLE);
	data->alarm[chan].callback = NULL;

	return 0;
}

static int counter_bee_rtc_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *top_cfg)
{
	const struct counter_bee_rtc_config *cfg = dev->config;

	LOG_DBG("ticks=%x\n", top_cfg->ticks);
	if (top_cfg->ticks != cfg->counter_info.max_top_value) {
		LOG_ERR("Unspported set top value");
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t counter_bee_rtc_get_pending_int(const struct device *dev)
{
	const struct counter_bee_rtc_config *cfg = dev->config;

	return cfg->get_irq_pending();
}

static uint32_t counter_bee_rtc_get_freq(const struct device *dev)
{
	struct counter_bee_rtc_data *data = dev->data;

	LOG_DBG("freq=%d\n", data->freq);

	return data->freq;
}

__maybe_unused static uint32_t counter_bee_rtc_get_guard_period(const struct device *dev,
								uint32_t flags)
{
	struct counter_bee_rtc_data *data = dev->data;

	return data->guard_period;
}

__maybe_unused static int counter_bee_rtc_set_guard_period(const struct device *dev, uint32_t guard,
							   uint32_t flags)
{
	struct counter_bee_rtc_data *data = dev->data;

	__ASSERT_NO_MSG(guard < counter_bee_rtc_get_top_value(dev));

	data->guard_period = guard;

	return 0;
}

static void alarm_irq_handle(const struct device *dev, uint32_t chan)
{
	struct counter_bee_rtc_data *data = dev->data;
	struct counter_bee_rtc_ch_data *alarm = &data->alarm[chan];
	counter_alarm_callback_t cb;
	uint32_t ticks;

	counter_bee_rtc_get_value(dev, &ticks);
	cb = alarm->callback;
	alarm->callback = NULL;

	if (cb) {
		cb(dev, chan, ticks, alarm->user_data);
		rtc_late_int_table[chan] = false;
	}

	RTC_ClearCompINT(chan);
}

static void counter_bee_rtc_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct counter_bee_rtc_config *cfg = dev->config;

	for (uint32_t i = 0; i < cfg->counter_info.channels; i++) {
		if (RTC_GetINTStatus(rtc_cmp_int_table[i]) || rtc_late_int_table[i]) {
			alarm_irq_handle(dev, i);
		}
	}
}

static int counter_bee_rtc_init(const struct device *dev)
{
	const struct counter_bee_rtc_config *cfg = dev->config;
	struct counter_bee_rtc_data *data = dev->data;

	data->freq = cfg->src_clk_freq / cfg->prescaler;

	cfg->irq_config();

	__ASSERT(cfg->prescaler <= 0xfff, "rtc prescaler should be less than 0xfff");

	RTC_DeInit();
	RTC_SetPrescaler(cfg->prescaler - 1);
	RTC_ResetCounter();
	RTC_NvCmd(ENABLE);

	return 0;
}

static DEVICE_API(counter, counter_bee_rtc_driver_api) = {
	.start = counter_bee_rtc_start,
	.stop = counter_bee_rtc_stop,
	.get_value = counter_bee_rtc_get_value,
	.set_alarm = counter_bee_rtc_set_alarm,
	.cancel_alarm = counter_bee_rtc_cancel_alarm,
	.set_top_value = counter_bee_rtc_set_top_value,
	.get_pending_int = counter_bee_rtc_get_pending_int,
	.get_top_value = counter_bee_rtc_get_top_value,
#if defined(CONFIG_SOC_SERIES_RTL87X2G)
	.get_guard_period = counter_bee_rtc_get_guard_period,
	.set_guard_period = counter_bee_rtc_set_guard_period,
#endif
	.get_freq = counter_bee_rtc_get_freq,
};

#if defined(CONFIG_SOC_SERIES_RTL87X2G)
#define RTC_IRQ_CONFIG_FUNC(index)                                                                 \
	static void irq_config_##index(void)                                                       \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),                     \
			    counter_bee_rtc_isr, DEVICE_DT_INST_GET(index), 0);                    \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
#elif defined(CONFIG_SOC_SERIES_RTL8752H)
#define RTC_IRQ_CONFIG_FUNC(index)                                                                 \
	static void counter_bee_rtc_isr_wrapper_##index(void)                                      \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_INST_GET(index);                              \
		counter_bee_rtc_isr((void *)dev);                                                  \
	}                                                                                          \
	static void irq_config_##index(void)                                                       \
	{                                                                                          \
		RamVectorTableUpdate(RTC_VECTORn, counter_bee_rtc_isr_wrapper_##index);            \
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

#define BEE_RTC_INIT(index)                                                                        \
	RTC_IRQ_CONFIG(index);                                                                     \
	static struct rtc_data_##index {                                                           \
		struct counter_bee_rtc_data data;                                                  \
		struct counter_bee_rtc_ch_data alarm[DT_INST_PROP(index, channels)];               \
	} counter_bee_rtc_data_##index = {0};                                                      \
	static const struct counter_bee_rtc_config counter_bee_rtc_config_##index = {              \
		.counter_info =                                                                    \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.freq = 0,                                                         \
				.channels = DT_INST_PROP(index, channels),                         \
			},                                                                         \
		.reg = DT_INST_REG_ADDR(index),                                                    \
		.src_clk_freq = DT_INST_PROP_OR(index, src_clk_freq, 32000),                       \
		.prescaler = DT_INST_PROP(index, prescaler),                                       \
		.irq_config = irq_config_##index,                                                  \
		.set_irq_pending = set_irq_pending_##index,                                        \
		.get_irq_pending = get_irq_pending_##index,                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, counter_bee_rtc_init, NULL, &counter_bee_rtc_data_##index,    \
			      &counter_bee_rtc_config_##index, POST_KERNEL,                        \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_bee_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BEE_RTC_INIT);
