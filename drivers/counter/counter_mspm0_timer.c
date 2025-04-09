/*
 * Copyright (c) 2025, Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0g1x0x_g3x0x_timer_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <ti/driverlib/dl_timer.h>

LOG_MODULE_REGISTER(mspm0_counter, CONFIG_COUNTER_LOG_LEVEL);

static void counter_mspm0_isr(void *arg);

struct counter_mspm0_data {
	void *user_data;
	counter_top_callback_t top_cb;
};

struct counter_mspm0_config {
	GPTIMER_Regs *base;
	const struct mspm0_clockSys *clock_subsys;
	DL_Timer_ClockConfig clk_config;
	void (*irq_config_func)(const struct device *dev);
	struct counter_config_info counter_info;
};

static int counter_mspm0_start(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	DL_Timer_startCounter(config->base);

	return 0;
}

static int counter_mspm0_stop(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	DL_Timer_stopCounter(config->base);

	return 0;
}

static int counter_mspm0_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_mspm0_config *config = dev->config;

	*ticks = DL_Timer_getTimerCount(config->base);

	return 0;
}

static int counter_mspm0_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *cfg)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;

	if (cfg->ticks > config->counter_info.max_top_value) {
		return -ENOTSUP;
	}

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		DL_Timer_stopCounter(config->base);
		DL_Timer_startCounter(config->base);
	} else if (DL_Timer_getTimerCount(config->base) >= cfg->ticks) {
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			DL_Timer_stopCounter(config->base);
			DL_Timer_startCounter(config->base);
		}
		return -ETIME;
	}

	DL_Timer_setLoadValue(config->base, cfg->ticks);

	data->top_cb = cfg->callback;
	data->user_data = cfg->user_data;
	if (cfg->callback) {
		DL_Timer_clearInterruptStatus(config->base,
					      DL_TIMER_INTERRUPT_LOAD_EVENT);
		DL_Timer_enableInterrupt(config->base,
					 DL_TIMER_INTERRUPT_LOAD_EVENT);
	}

	return 0;
}

static uint32_t counter_mspm0_get_pending_int(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	uint32_t status;

	status = DL_Timer_getPendingInterrupt(config->base);

	return !!(status & DL_TIMER_IIDX_LOAD);
}

static uint32_t counter_mspm0_get_top_value(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	return DL_Timer_getLoadValue(config->base);
}

static uint32_t counter_mspm0_get_freq(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	DL_Timer_ClockConfig clkcfg;
	uint32_t clock_rate;
	int ret;

	ret = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(clkmux)),
				(clock_control_subsys_t)config->clock_subsys,
				&clock_rate);
	if (ret) {
		LOG_ERR("clk get rate err %d", ret);
		return 0;
	}

	DL_Timer_getClockConfig(config->base, &clkcfg);
	clock_rate = clock_rate / ((clkcfg.divideRatio + 1) * (clkcfg.prescale + 1));

	return clock_rate;
}

static int counter_mspm0_init(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	DL_Timer_TimerConfig tim_config = {
			.period = 0,
			.timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP,
			.startTimer = DL_TIMER_STOP,
			};

	if (!device_is_ready(DEVICE_DT_GET(DT_NODELABEL(clkmux)))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	DL_Timer_reset(config->base);
	if (! DL_Timer_isPowerEnabled(config->base)) {
		DL_Timer_enablePower(config->base);
	}

	k_msleep(1);
	DL_Timer_setClockConfig(config->base,
				(DL_Timer_ClockConfig *)&config->clk_config);
	DL_Timer_initTimerMode(config->base, &tim_config);
	DL_Timer_setCounterRepeatMode(config->base,
				      DL_TIMER_REPEAT_MODE_ENABLED);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, mspm0_counter_api) = {
	.start = counter_mspm0_start,
	.stop = counter_mspm0_stop,
	.get_value = counter_mspm0_get_value,
	.set_top_value = counter_mspm0_set_top_value,
	.get_pending_int = counter_mspm0_get_pending_int,
	.get_top_value = counter_mspm0_get_top_value,
	.get_freq = counter_mspm0_get_freq,
};

static void counter_mspm0_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct counter_mspm0_data *data = dev->data;
	const struct counter_mspm0_config *config = dev->config;
	uint32_t status;
	uint32_t now;

	status = DL_Timer_getPendingInterrupt(config->base);
	if ((status & DL_TIMER_IIDX_LOAD) == 0) {
		return;
	}

	counter_mspm0_get_value(dev, &now);
	if (data->top_cb) {
		data->top_cb(dev, data->user_data);
	}
}

#define MSPM0_COUNTER_IRQ_REGISTER(n)						\
	static void mspm0_ ## n ##_irq_register(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    counter_mspm0_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}

#define MSPMO_CLK_DIV(div) 		DT_CAT(DL_TIMER_CLOCK_DIVIDE_, div)

#define COUNTER_DEVICE_INIT_MSPM0(n)						\
	static struct counter_mspm0_data counter_mspm0_data_ ## n;		\
	MSPM0_COUNTER_IRQ_REGISTER(n);						\
										\
	static const struct mspm0_clockSys mspm0_counter_clockSys ## n =	\
		MSPM0_CLOCK_SUBSYS_FN(n);					\
										\
	static const struct counter_mspm0_config counter_mspm0_config_ ## n = {	\
		.base = (GPTIMER_Regs *)DT_REG_ADDR(DT_INST_PARENT(n)),		\
		.clock_subsys = &mspm0_counter_clockSys ## n,			\
		.clk_config = {.clockSel = (DT_INST_CLOCKS_CELL(n, bus) &	\
					    MSPM0_CLOCK_SEL_MASK),		\
			       .divideRatio = MSPMO_CLK_DIV(DT_PROP(DT_DRV_INST(n),ti_clk_div)), \
			       .prescale = DT_PROP(DT_DRV_INST(n), ti_clk_prescaler), },\
		.irq_config_func = (mspm0_ ## n ##_irq_register),			\
		.counter_info = {.max_top_value = (DT_INST_PROP(n, resolution) == 32)	\
							? UINT32_MAX : UINT16_MAX,	\
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
				 .channels = 1},				\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      counter_mspm0_init,				\
			      NULL,						\
			      &counter_mspm0_data_ ## n,			\
			      &counter_mspm0_config_ ## n,			\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,	\
			      &mspm0_counter_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT_MSPM0)
