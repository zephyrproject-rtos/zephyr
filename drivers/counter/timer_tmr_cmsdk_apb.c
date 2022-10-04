/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_timer

#include <zephyr/drivers/counter.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <zephyr/drivers/clock_control/arm_clock_control.h>

#include "timer_cmsdk_apb.h"

typedef void (*timer_config_func_t)(const struct device *dev);

struct tmr_cmsdk_apb_cfg {
	struct counter_config_info info;
	volatile struct timer_cmsdk_apb *timer;
	timer_config_func_t timer_config_func;
	/* Timer Clock control in Active State */
	const struct arm_clock_control_t timer_cc_as;
	/* Timer Clock control in Sleep State */
	const struct arm_clock_control_t timer_cc_ss;
	/* Timer Clock control in Deep Sleep State */
	const struct arm_clock_control_t timer_cc_dss;
};

struct tmr_cmsdk_apb_dev_data {
	counter_top_callback_t top_callback;
	void *top_user_data;

	uint32_t load;
};

static int tmr_cmsdk_apb_start(const struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;
	struct tmr_cmsdk_apb_dev_data *data = dev->data;

	/* Set the timer reload to count */
	cfg->timer->reload = data->load;

	cfg->timer->ctrl = TIMER_CTRL_EN;

	return 0;
}

static int tmr_cmsdk_apb_stop(const struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;
	/* Disable the timer */
	cfg->timer->ctrl = 0x0;

	return 0;
}

static int tmr_cmsdk_apb_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;
	struct tmr_cmsdk_apb_dev_data *data = dev->data;

	/* Get Counter Value */
	*ticks = data->load - cfg->timer->value;
	return 0;
}

static int tmr_cmsdk_apb_set_top_value(const struct device *dev,
				       const struct counter_top_cfg *top_cfg)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;
	struct tmr_cmsdk_apb_dev_data *data = dev->data;

	/* Counter is always reset when top value is updated. */
	if (top_cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		return -ENOTSUP;
	}

	data->top_callback = top_cfg->callback;
	data->top_user_data = top_cfg->user_data;

	/* Store the reload value */
	data->load = top_cfg->ticks;

	/* Set value register to count */
	cfg->timer->value = top_cfg->ticks;

	/* Set the timer reload to count */
	cfg->timer->reload = top_cfg->ticks;

	/* Enable IRQ */
	cfg->timer->ctrl |= TIMER_CTRL_IRQ_EN;

	return 0;
}

static uint32_t tmr_cmsdk_apb_get_top_value(const struct device *dev)
{
	struct tmr_cmsdk_apb_dev_data *data = dev->data;

	uint32_t ticks = data->load;

	return ticks;
}

static uint32_t tmr_cmsdk_apb_get_pending_int(const struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;

	return cfg->timer->intstatus;
}

static const struct counter_driver_api tmr_cmsdk_apb_api = {
	.start = tmr_cmsdk_apb_start,
	.stop = tmr_cmsdk_apb_stop,
	.get_value = tmr_cmsdk_apb_get_value,
	.set_top_value = tmr_cmsdk_apb_set_top_value,
	.get_pending_int = tmr_cmsdk_apb_get_pending_int,
	.get_top_value = tmr_cmsdk_apb_get_top_value,
};

static void tmr_cmsdk_apb_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct tmr_cmsdk_apb_dev_data *data = dev->data;
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;

	cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int tmr_cmsdk_apb_init(const struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config;

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	const struct device *const clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0));

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->timer_cc_as);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->timer_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->timer_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	cfg->timer_config_func(dev);

	return 0;
}

#define TIMER_CMSDK_INIT(inst)						\
	static void timer_cmsdk_apb_config_##inst(const struct device *dev); \
									\
	static const struct tmr_cmsdk_apb_cfg tmr_cmsdk_apb_cfg_##inst = { \
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.freq = 24000000U,				\
			.flags = 0,					\
			.channels = 0U,					\
		},							\
		.timer = ((volatile struct timer_cmsdk_apb *)DT_INST_REG_ADDR(inst)), \
		.timer_config_func = timer_cmsdk_apb_config_##inst,	\
		.timer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,	\
				.device = DT_INST_REG_ADDR(inst),},	\
		.timer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,	\
				.device = DT_INST_REG_ADDR(inst),},	\
		.timer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP, \
				 .device = DT_INST_REG_ADDR(inst),},	\
	};								\
									\
	static struct tmr_cmsdk_apb_dev_data tmr_cmsdk_apb_dev_data_##inst = { \
		.load = UINT32_MAX,					\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    tmr_cmsdk_apb_init,				\
			    NULL,			\
			    &tmr_cmsdk_apb_dev_data_##inst,		\
			    &tmr_cmsdk_apb_cfg_##inst, POST_KERNEL,	\
			    CONFIG_COUNTER_INIT_PRIORITY,		\
			    &tmr_cmsdk_apb_api);			\
									\
	static void timer_cmsdk_apb_config_##inst(const struct device *dev) \
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority),		\
			    tmr_cmsdk_apb_isr,				\
			    DEVICE_DT_INST_GET(inst),			\
			    0);						\
		irq_enable(DT_INST_IRQN(inst));				\
	}

DT_INST_FOREACH_STATUS_OKAY(TIMER_CMSDK_INIT)
