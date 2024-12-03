/*
 * Copyright (c) 2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <wrap_max32_tmr.h>

/** MAX32 MCUs does not have multiple channels */
#define MAX32_TIMER_CH 1U

struct max32_tmr_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	uint32_t guard_period;
};

struct max32_tmr_ch_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct max32_tmr_config {
	struct counter_config_info info;
	struct max32_tmr_ch_data *ch_data;
	mxc_tmr_regs_t *regs;
	const struct device *clock;
	struct max32_perclk perclk;
	int clock_source;
	int prescaler;
	void (*irq_func)(const struct device *dev);
	bool wakeup_source;
};

static int api_start(const struct device *dev)
{
	const struct max32_tmr_config *cfg = dev->config;

	Wrap_MXC_TMR_EnableInt(cfg->regs);
	MXC_TMR_Start(cfg->regs);

	return 0;
}

static int api_stop(const struct device *dev)
{
	const struct max32_tmr_config *cfg = dev->config;

	Wrap_MXC_TMR_DisableInt(cfg->regs);
	MXC_TMR_Stop(cfg->regs);

	return 0;
}

static int api_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct max32_tmr_config *cfg = dev->config;

	*ticks = MXC_TMR_GetCount(cfg->regs);
	return 0;
}

static int api_set_top_value(const struct device *dev, const struct counter_top_cfg *counter_cfg)
{
	const struct max32_tmr_config *cfg = dev->config;

	if (counter_cfg->ticks == 0) {
		return -EINVAL;
	}

	if (counter_cfg->ticks != cfg->info.max_top_value) {
		return -ENOTSUP;
	}

	return 0;
}

static uint32_t api_get_pending_int(const struct device *dev)
{
	const struct max32_tmr_config *cfg = dev->config;

	return Wrap_MXC_TMR_GetPendingInt(cfg->regs);
}

static uint32_t api_get_top_value(const struct device *dev)
{
	const struct max32_tmr_config *cfg = dev->config;

	return cfg->info.max_top_value;
}

static uint32_t api_get_freq(const struct device *dev)
{
	const struct max32_tmr_config *cfg = dev->config;

	return cfg->info.freq;
}

static int set_cc(const struct device *dev, uint8_t id, uint32_t val, uint32_t flags)
{
	const struct max32_tmr_config *config = dev->config;
	struct max32_tmr_data *data = dev->data;
	mxc_tmr_regs_t *regs = config->regs;

	bool absolute = flags & COUNTER_ALARM_CFG_ABSOLUTE;
	uint32_t top = api_get_top_value(dev);
	int err = 0;
	uint32_t now;
	uint32_t diff;
	uint32_t max_rel_val = top;
	bool irq_on_late = 0;

	now = MXC_TMR_GetCount(regs);
	MXC_TMR_ClearFlags(regs);

	if (absolute) {
		max_rel_val = top - data->guard_period;
		irq_on_late = flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	} else {
		val = now + val;
	}

	MXC_TMR_SetCompare(regs, val);
	now = MXC_TMR_GetCount(regs);

	diff = (val - now);
	if (diff > max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		/* Interrupt is triggered always for relative alarm and
		 * for absolute depending on the flag.
		 */
		if (irq_on_late) {
			NVIC_SetPendingIRQ(MXC_TMR_GET_IRQ(MXC_TMR_GET_IDX(regs)));
		} else {
			config->ch_data[id].callback = NULL;
		}
	} else {
		api_start(dev);
	}

	return err;
}

static int api_set_alarm(const struct device *dev, uint8_t chan,
			 const struct counter_alarm_cfg *alarm_cfg)
{
	const struct max32_tmr_config *cfg = dev->config;
	struct max32_tmr_ch_data *chdata = &cfg->ch_data[chan];

	if (alarm_cfg->ticks > api_get_top_value(dev)) {
		return -EINVAL;
	}

	if (chdata->callback) {
		return -EBUSY;
	}

	chdata->callback = alarm_cfg->callback;
	chdata->user_data = alarm_cfg->user_data;

	return set_cc(dev, chan, alarm_cfg->ticks, alarm_cfg->flags);
}

static int api_cancel_alarm(const struct device *dev, uint8_t chan)
{
	const struct max32_tmr_config *cfg = dev->config;

	MXC_TMR_Stop(cfg->regs);
	MXC_TMR_SetCount(cfg->regs, 0);
	MXC_TMR_SetCompare(cfg->regs, cfg->info.max_top_value);
	Wrap_MXC_TMR_DisableInt(cfg->regs);
	cfg->ch_data[chan].callback = NULL;

	return 0;
}

static uint32_t api_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct max32_tmr_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int api_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	struct max32_tmr_data *data = dev->data;

	ARG_UNUSED(flags);

	if (ticks > api_get_top_value(dev)) {
		return -EINVAL;
	}

	data->guard_period = ticks;
	return 0;
}

static void max32_alarm_irq_handle(const struct device *dev, uint32_t id)
{
	const struct max32_tmr_config *cfg = dev->config;
	struct max32_tmr_ch_data *chdata;
	counter_alarm_callback_t cb;

	chdata = &cfg->ch_data[id];
	cb = chdata->callback;
	chdata->callback = NULL;

	if (cb) {
		cb(dev, id, MXC_TMR_GetCount(cfg->regs), chdata->user_data);
	}
}

static void counter_max32_isr(const struct device *dev)
{
	const struct max32_tmr_config *cfg = dev->config;
	struct max32_tmr_data *data = dev->data;

	MXC_TMR_ClearFlags(cfg->regs);
	Wrap_MXC_TMR_ClearWakeupFlags(cfg->regs);

	max32_alarm_irq_handle(dev, 0);

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int max32_counter_init(const struct device *dev)
{
	int ret = 0;
	const struct max32_tmr_config *cfg = dev->config;
	mxc_tmr_regs_t *regs = cfg->regs;
	wrap_mxc_tmr_cfg_t tmr_cfg;
	int prescaler_index;

	prescaler_index = LOG2(cfg->prescaler);
	if (prescaler_index == 0) {
		tmr_cfg.pres = TMR_PRES_1; /* TMR_PRES_1 is 0 */
	} else {
		/* TMR_PRES_2 is  1<<X */
		tmr_cfg.pres = TMR_PRES_2 + (prescaler_index - 1);
	}
	tmr_cfg.mode = TMR_MODE_COMPARE;
	tmr_cfg.cmp_cnt = cfg->info.max_top_value;
	tmr_cfg.bitMode = 0; /* Timer Mode 32 bit */
	tmr_cfg.pol = 0;

	tmr_cfg.clock = Wrap_MXC_TMR_GetClockIndex(cfg->clock_source);
	if (tmr_cfg.clock < 0) {
		return -ENOTSUP;
	}

	MXC_TMR_Shutdown(regs);

	/* enable clock */
	ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
	if (ret) {
		return ret;
	}

	ret = Wrap_MXC_TMR_Init(regs, &tmr_cfg);
	if (ret != E_NO_ERROR) {
		return ret;
	}

	/* Set preload and actually pre-load the counter */
	MXC_TMR_SetCompare(regs, cfg->info.max_top_value);

	cfg->irq_func(dev);

	if (cfg->wakeup_source) {
		/* Clear Wakeup status */
		MXC_LP_ClearWakeStatus();
		/* Enable Timer wake-up source */
		Wrap_MXC_TMR_EnableWakeup(regs, &tmr_cfg);
	}

	return 0;
}

static DEVICE_API(counter, counter_max32_driver_api) = {
	.start = api_start,
	.stop = api_stop,
	.get_value = api_get_value,
	.set_top_value = api_set_top_value,
	.get_pending_int = api_get_pending_int,
	.get_top_value = api_get_top_value,
	.get_freq = api_get_freq,
	.set_alarm = api_set_alarm,
	.cancel_alarm = api_cancel_alarm,
	.get_guard_period = api_get_guard_period,
	.set_guard_period = api_set_guard_period,
};

#define TIMER(_num)    DT_INST_PARENT(_num)
#define MAX32_TIM(idx) ((mxc_tmr_regs_t *)DT_REG_ADDR(TIMER(idx)))

#define COUNTER_MAX32_DEFINE(_num)                                                                 \
	static struct max32_tmr_ch_data counter##_num##_ch_data[MAX32_TIMER_CH];                   \
	static void max32_tmr_irq_init_##_num(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(TIMER(_num)), DT_IRQ(TIMER(_num), priority),                   \
			    counter_max32_isr, DEVICE_DT_INST_GET(_num), 0);                       \
		irq_enable(DT_IRQN(TIMER(_num)));                                                  \
	};                                                                                         \
	static const struct max32_tmr_config max32_tmr_config_##_num = {                           \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = WRAP_MXC_IS_32B_TIMER(MAX32_TIM(_num))            \
							 ? UINT32_MAX                              \
							 : UINT16_MAX,                             \
				.freq = ADI_MAX32_GET_PRPH_CLK_FREQ(                               \
						DT_PROP(TIMER(_num), clock_source)) /              \
					DT_PROP(TIMER(_num), prescaler),                           \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = MAX32_TIMER_CH,                                        \
			},                                                                         \
		.regs = (mxc_tmr_regs_t *)DT_REG_ADDR(TIMER(_num)),                                \
		.clock = DEVICE_DT_GET(DT_CLOCKS_CTLR(TIMER(_num))),                               \
		.perclk.bus = DT_CLOCKS_CELL(TIMER(_num), offset),                                 \
		.perclk.bit = DT_CLOCKS_CELL(TIMER(_num), bit),                                    \
		.clock_source = DT_PROP(TIMER(_num), clock_source),                                \
		.prescaler = DT_PROP(TIMER(_num), prescaler),                                      \
		.irq_func = max32_tmr_irq_init_##_num,                                             \
		.ch_data = counter##_num##_ch_data,                                                \
		.wakeup_source = DT_PROP(TIMER(_num), wakeup_source),                              \
	};                                                                                         \
	static struct max32_tmr_data max32_tmr_data##_num;                                         \
	DEVICE_DT_INST_DEFINE(_num, &max32_counter_init, NULL, &max32_tmr_data##_num,              \
			      &max32_tmr_config_##_num, PRE_KERNEL_1,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_max32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MAX32_DEFINE)
