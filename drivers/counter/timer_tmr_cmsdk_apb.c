/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/counter.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include <clock_control/arm_clock_control.h>

#include "timer_cmsdk_apb.h"

typedef void (*timer_config_func_t)(struct device *dev);

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

	u32_t load;
};

static int tmr_cmsdk_apb_start(struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Set the timer reload to count */
	cfg->timer->reload = data->load;

	cfg->timer->ctrl = TIMER_CTRL_EN;

	return 0;
}

static int tmr_cmsdk_apb_stop(struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	/* Disable the timer */
	cfg->timer->ctrl = 0x0;

	return 0;
}

static u32_t tmr_cmsdk_apb_read(struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Return Counter Value */
	return data->load - cfg->timer->value;
}

static int tmr_cmsdk_apb_set_top_value(struct device *dev,
				       const struct counter_top_cfg *top_cfg)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct tmr_cmsdk_apb_dev_data *data = dev->driver_data;

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

static u32_t tmr_cmsdk_apb_get_top_value(struct device *dev)
{
	struct tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	u32_t ticks = data->load;

	return ticks;
}

static u32_t tmr_cmsdk_apb_get_pending_int(struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	return cfg->timer->intstatus;
}

static const struct counter_driver_api tmr_cmsdk_apb_api = {
	.start = tmr_cmsdk_apb_start,
	.stop = tmr_cmsdk_apb_stop,
	.read = tmr_cmsdk_apb_read,
	.set_top_value = tmr_cmsdk_apb_set_top_value,
	.get_pending_int = tmr_cmsdk_apb_get_pending_int,
	.get_top_value = tmr_cmsdk_apb_get_top_value,
};

static void tmr_cmsdk_apb_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct tmr_cmsdk_apb_dev_data *data = dev->driver_data;
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

static int tmr_cmsdk_apb_init(struct device *dev)
{
	const struct tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	struct device *clk =
		device_get_binding(CONFIG_ARM_CLOCK_CONTROL_DEV_NAME);

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->timer_cc_as);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->timer_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->timer_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	cfg->timer_config_func(dev);

	return 0;
}

/* TIMER 0 */
#ifdef DT_INST_0_ARM_CMSDK_TIMER
static void timer_cmsdk_apb_config_0(struct device *dev);

static const struct tmr_cmsdk_apb_cfg tmr_cmsdk_apb_cfg_0 = {
	.info = {
			.max_top_value = UINT32_MAX,
			.freq = 24000000U,
			.flags = 0,
			.channels = 0U,
	},
	.timer = ((volatile struct timer_cmsdk_apb *)DT_INST_0_ARM_CMSDK_TIMER_BASE_ADDRESS),
	.timer_config_func = timer_cmsdk_apb_config_0,
	.timer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
			.device = DT_INST_0_ARM_CMSDK_TIMER_BASE_ADDRESS,},
	.timer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
			.device = DT_INST_0_ARM_CMSDK_TIMER_BASE_ADDRESS,},
	.timer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			 .device = DT_INST_0_ARM_CMSDK_TIMER_BASE_ADDRESS,},
};

static struct tmr_cmsdk_apb_dev_data tmr_cmsdk_apb_dev_data_0 = {
	.load = UINT32_MAX,
};

DEVICE_AND_API_INIT(tmr_cmsdk_apb_0,
		    DT_INST_0_ARM_CMSDK_TIMER_LABEL,
		    tmr_cmsdk_apb_init, &tmr_cmsdk_apb_dev_data_0,
		    &tmr_cmsdk_apb_cfg_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &tmr_cmsdk_apb_api);

static void timer_cmsdk_apb_config_0(struct device *dev)
{
	IRQ_CONNECT(DT_INST_0_ARM_CMSDK_TIMER_IRQ_0, DT_INST_0_ARM_CMSDK_TIMER_IRQ_0_PRIORITY,
		    tmr_cmsdk_apb_isr,
		    DEVICE_GET(tmr_cmsdk_apb_0), 0);
	irq_enable(DT_INST_0_ARM_CMSDK_TIMER_IRQ_0);
}
#endif /* DT_INST_0_ARM_CMSDK_TIMER */

/* TIMER 1 */
#ifdef DT_INST_1_ARM_CMSDK_TIMER
static void timer_cmsdk_apb_config_1(struct device *dev);

static const struct tmr_cmsdk_apb_cfg tmr_cmsdk_apb_cfg_1 = {
	.info = {
			.max_top_value = UINT32_MAX,
			.freq = 24000000U,
			.flags = 0,
			.channels = 0U,
	},
	.timer = ((volatile struct timer_cmsdk_apb *)DT_INST_1_ARM_CMSDK_TIMER_BASE_ADDRESS),
	.timer_config_func = timer_cmsdk_apb_config_1,
	.timer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
			.device = DT_INST_1_ARM_CMSDK_TIMER_BASE_ADDRESS,},
	.timer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
			.device = DT_INST_1_ARM_CMSDK_TIMER_BASE_ADDRESS,},
	.timer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			 .device = DT_INST_1_ARM_CMSDK_TIMER_BASE_ADDRESS,},
};

static struct tmr_cmsdk_apb_dev_data tmr_cmsdk_apb_dev_data_1 = {
	.load = UINT32_MAX,
};

DEVICE_AND_API_INIT(tmr_cmsdk_apb_1,
		    DT_INST_1_ARM_CMSDK_TIMER_LABEL,
		    tmr_cmsdk_apb_init, &tmr_cmsdk_apb_dev_data_1,
		    &tmr_cmsdk_apb_cfg_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &tmr_cmsdk_apb_api);

static void timer_cmsdk_apb_config_1(struct device *dev)
{
	IRQ_CONNECT(DT_INST_1_ARM_CMSDK_TIMER_IRQ_0, DT_INST_1_ARM_CMSDK_TIMER_IRQ_0_PRIORITY,
		    tmr_cmsdk_apb_isr,
		    DEVICE_GET(tmr_cmsdk_apb_1), 0);
	irq_enable(DT_INST_1_ARM_CMSDK_TIMER_IRQ_0);
}
#endif /* DT_INST_1_ARM_CMSDK_TIMER */
