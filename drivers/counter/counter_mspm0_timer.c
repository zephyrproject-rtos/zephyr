/*
 * Copyright (c) 2025, Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_timer_counter

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <ti/driverlib/dl_timera.h>
#include <ti/driverlib/dl_timerg.h>
#include <ti/driverlib/dl_timer.h>

LOG_MODULE_REGISTER(mspm0_counter, CONFIG_COUNTER_LOG_LEVEL);

struct counter_mspm0_data {
	void *user_data_top;
	void *user_data;
	counter_top_callback_t top_cb;
	counter_alarm_callback_t alarm_cb;
};

struct counter_mspm0_config {
	struct counter_config_info counter_info;
	GPTIMER_Regs *base;
	const struct device *clock_dev;
	const struct mspm0_sys_clock clock_subsys;
	DL_Timer_ClockConfig clk_config;
	void (*irq_config_func)(void);
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
	data->user_data_top = cfg->user_data;
	if (cfg->callback) {
		DL_Timer_clearInterruptStatus(config->base,
					      DL_TIMER_INTERRUPT_LOAD_EVENT);
		DL_Timer_enableInterrupt(config->base,
					 DL_TIMER_INTERRUPT_LOAD_EVENT);
	}

	return 0;
}

static uint32_t counter_mspm0_get_top_value(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;

	return DL_Timer_getLoadValue(config->base);
}

static int counter_mspm0_set_alarm(const struct device *dev,
				   uint8_t chan_id,
				   const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;
	uint32_t top = counter_mspm0_get_top_value(dev);
	uint32_t ticks = alarm_cfg->ticks;

	ARG_UNUSED(chan_id);

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	if (data->alarm_cb != NULL) {
		LOG_DBG("Alarm busy\n");
		return -EBUSY;
	}

	if ((COUNTER_ALARM_CFG_ABSOLUTE & alarm_cfg->flags) == 0) {
		ticks += DL_Timer_getTimerCount(config->base);
		if (ticks > top) {
			ticks %= top;
		}
	}

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	DL_Timer_setCaptureCompareValue(config->base, ticks,
					DL_TIMER_CC_0_INDEX);
	DL_Timer_clearInterruptStatus(config->base,
				      DL_TIMER_INTERRUPT_CC0_UP_EVENT);
	DL_Timer_enableInterrupt(config->base,
				 DL_TIMER_INTERRUPT_CC0_UP_EVENT);

	return 0;
}

static int counter_mspm0_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_mspm0_config *config = dev->config;
	struct counter_mspm0_data *data = dev->data;

	ARG_UNUSED(chan_id);

	DL_Timer_disableInterrupt(config->base,
				  DL_TIMER_INTERRUPT_CC0_UP_EVENT);
	data->alarm_cb = NULL;

	return 0;
}

static uint32_t counter_mspm0_get_pending_int(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	uint32_t status;

	status = DL_Timer_getRawInterruptStatus(config->base,
				(DL_TIMER_INTERRUPT_LOAD_EVENT |
				 DL_TIMER_INTERRUPT_CC0_UP_EVENT));

	return !!status;
}

static uint32_t counter_mspm0_get_freq(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	DL_Timer_ClockConfig clkcfg;
	uint32_t clock_rate;
	int ret;

	ret = clock_control_get_rate(config->clock_dev,
				(clock_control_subsys_t)&config->clock_subsys,
				&clock_rate);
	if (ret != 0) {
		LOG_ERR("clk get rate err %d", ret);
		return 0;
	}

	DL_Timer_getClockConfig(config->base, &clkcfg);
	clock_rate = clock_rate /
			((clkcfg.divideRatio + 1) * (clkcfg.prescale + 1));

	return clock_rate;
}

static int counter_mspm0_init(const struct device *dev)
{
	const struct counter_mspm0_config *config = dev->config;
	DL_Timer_TimerConfig tim_config = {
			.period = config->counter_info.max_top_value,
			.timerMode = DL_TIMER_TIMER_MODE_PERIODIC_UP,
			.startTimer = DL_TIMER_STOP,
			};

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	DL_Timer_reset(config->base);
	if (!DL_Timer_isPowerEnabled(config->base)) {
		DL_Timer_enablePower(config->base);
	}

	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);
	DL_Timer_setClockConfig(config->base,
				(DL_Timer_ClockConfig *)&config->clk_config);
	DL_Timer_initTimerMode(config->base, &tim_config);
	DL_Timer_setCounterRepeatMode(config->base,
				      DL_TIMER_REPEAT_MODE_ENABLED);

	config->irq_config_func();

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
	.cancel_alarm = counter_mspm0_cancel_alarm,
	.set_alarm = counter_mspm0_set_alarm,
};

static void counter_mspm0_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct counter_mspm0_data *data = dev->data;
	const struct counter_mspm0_config *config = dev->config;
	uint32_t status;

	status = DL_Timer_getPendingInterrupt(config->base);
	if ((status == DL_TIMER_IIDX_CC0_UP) && data->alarm_cb) {
		uint32_t now;
		counter_alarm_callback_t alarm_cb = data->alarm_cb;

		counter_mspm0_get_value(dev, &now);
		data->alarm_cb = NULL;
		alarm_cb(dev, 0, now, data->user_data);
	} else if ((status == DL_TIMER_IIDX_LOAD) && data->top_cb) {
		data->top_cb(dev, data->user_data_top);
	}
}

#define MSPM0_COUNTER_IRQ_REGISTER(n)							\
	static void mspm0_ ## n ##_irq_register(void)					\
	{										\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)),					\
			    DT_IRQ(DT_INST_PARENT(n), priority),			\
			    counter_mspm0_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));					\
	}

#define MSPM0_CLK_DIV(div)		DT_CAT(DL_TIMER_CLOCK_DIVIDE_, div)

#define COUNTER_DEVICE_INIT_MSPM0(n)							\
	static struct counter_mspm0_data counter_mspm0_data_ ## n;			\
	MSPM0_COUNTER_IRQ_REGISTER(n)							\
											\
	static const struct counter_mspm0_config counter_mspm0_config_ ## n = {		\
		.base = (GPTIMER_Regs *)DT_REG_ADDR(DT_INST_PARENT(n)),			\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(			\
						DT_INST_PARENT(n), 0)),			\
		.clock_subsys = {							\
			.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),	\
			},								\
		.irq_config_func = (mspm0_ ## n ##_irq_register),			\
		.clk_config = {								\
			.clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(			\
				DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk)),	\
			.divideRatio = MSPM0_CLK_DIV(DT_PROP(DT_INST_PARENT(n),		\
						     ti_clk_div)),			\
			.prescale = DT_PROP(DT_INST_PARENT(n), ti_clk_prescaler),	\
			},								\
		.counter_info = {.max_top_value = (DT_INST_PROP(n, resolution) == 32)	\
							? UINT32_MAX : UINT16_MAX,	\
				 .flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
				 .channels = 1},					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			      counter_mspm0_init,					\
			      NULL,							\
			      &counter_mspm0_data_ ## n,				\
			      &counter_mspm0_config_ ## n,				\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			      &mspm0_counter_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_DEVICE_INIT_MSPM0)
