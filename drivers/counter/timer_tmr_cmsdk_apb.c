/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <counter.h>
#include <device.h>
#include <errno.h>
#include <init.h>
#include <soc.h>
#include <clock_control/arm_clock_control.h>

#include "timer_cmsdk_apb.h"

typedef void (*timer_config_func_t)(struct device *dev);

static counter_callback_t user_cb;

#define TIMER_MAX_RELOAD	0xFFFFFFFF

enum timer_status_t {
	TIMER_DISABLED = 0,
	TIMER_ENABLED,
};

struct timer_tmr_cmsdk_apb_cfg {
	volatile struct timer_cmsdk_apb *timer;
	timer_config_func_t timer_config_func;
	/* Timer Clock control in Active State */
	const struct arm_clock_control_t timer_cc_as;
	/* Timer Clock control in Sleep State */
	const struct arm_clock_control_t timer_cc_ss;
	/* Timer Clock control in Deep Sleep State */
	const struct arm_clock_control_t timer_cc_dss;
};

struct timer_tmr_cmsdk_apb_dev_data {
	uint32_t load;
	enum timer_status_t status;
};

static int timer_tmr_cmsdk_apb_start(struct device *dev)
{
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Set the timer to Max reload */
	cfg->timer->reload = TIMER_MAX_RELOAD;

	/* Enable the timer */
	cfg->timer->ctrl = TIMER_CTRL_EN;

	/* Update the status */
	data->status = TIMER_ENABLED;

	return 0;
}

static int timer_tmr_cmsdk_apb_stop(struct device *dev)
{
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Disable the timer */
	cfg->timer->ctrl = 0x0;

	/* Update the status */
	data->status = TIMER_DISABLED;

	return 0;
}

static uint32_t timer_tmr_cmsdk_apb_read(struct device *dev)
{
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Return Counter Value */
	uint32_t value = 0;

	value = data->load - cfg->timer->value;

	return value;
}

static int timer_tmr_cmsdk_apb_set_alarm(struct device *dev,
					 counter_callback_t callback,
					 uint32_t count, void *user_data)
{
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_tmr_cmsdk_apb_dev_data *data = dev->driver_data;

	if (data->status == TIMER_DISABLED) {
		return -ENOTSUP;
	}

	/* Set callback */
	user_cb = callback;

	/* Store the reload value */
	data->load = count;

	/* Set value register to count */
	cfg->timer->value = count;

	/* Set the timer reload to count */
	cfg->timer->reload = count;

	/* Enable IRQ */
	cfg->timer->ctrl |= TIMER_CTRL_IRQ_EN;

	return 0;
}

static uint32_t timer_tmr_cmsdk_apb_get_pending_int(struct device *dev)
{
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	return cfg->timer->intstatus;
}

static const struct counter_driver_api timer_tmr_cmsdk_apb_api = {
	.start = timer_tmr_cmsdk_apb_start,
	.stop = timer_tmr_cmsdk_apb_stop,
	.read = timer_tmr_cmsdk_apb_read,
	.set_alarm = timer_tmr_cmsdk_apb_set_alarm,
	.get_pending_int = timer_tmr_cmsdk_apb_get_pending_int,
};

static void timer_tmr_cmsdk_apb_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	if (user_cb) {
		/* user_data paramenter is not used by this driver */
		(*user_cb)(dev, NULL);
	}
}

static int timer_tmr_cmsdk_apb_init(struct device *dev)
{
	const struct timer_tmr_cmsdk_apb_cfg * const cfg =
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
#ifdef CONFIG_TIMER_TMR_CMSDK_APB_0
static void timer_cmsdk_apb_config_0(struct device *dev);

static const struct timer_tmr_cmsdk_apb_cfg timer_tmr_cmsdk_apb_cfg_0 = {
	.timer = ((volatile struct timer_cmsdk_apb *)CMSDK_APB_TIMER0),
	.timer_config_func = timer_cmsdk_apb_config_0,
	.timer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
			.device = CMSDK_APB_TIMER0,},
	.timer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
			.device = CMSDK_APB_TIMER0,},
	.timer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			 .device = CMSDK_APB_TIMER0,},
};

static struct timer_tmr_cmsdk_apb_dev_data timer_tmr_cmsdk_apb_dev_data_0 = {
	.load = TIMER_MAX_RELOAD,
	.status = TIMER_DISABLED,
};

DEVICE_AND_API_INIT(timer_tmr_cmsdk_apb_0,
		    CONFIG_TIMER_TMR_CMSDK_APB_0_DEV_NAME,
		    timer_tmr_cmsdk_apb_init, &timer_tmr_cmsdk_apb_dev_data_0,
		    &timer_tmr_cmsdk_apb_cfg_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &timer_tmr_cmsdk_apb_api);

static void timer_cmsdk_apb_config_0(struct device *dev)
{
	IRQ_CONNECT(CMSDK_APB_TIMER_0_IRQ, CONFIG_TIMER_TMR_CMSDK_APB_0_IRQ_PRI,
		    timer_tmr_cmsdk_apb_isr,
		    DEVICE_GET(timer_tmr_cmsdk_apb_0), 0);
	irq_enable(CMSDK_APB_TIMER_0_IRQ);
}
#endif /* CONFIG_TIMER_TMR_CMSDK_APB_0 */

/* TIMER 1 */
#ifdef CONFIG_TIMER_TMR_CMSDK_APB_1
static void timer_cmsdk_apb_config_1(struct device *dev);

static const struct timer_tmr_cmsdk_apb_cfg timer_tmr_cmsdk_apb_cfg_1 = {
	.timer = ((volatile struct timer_cmsdk_apb *)CMSDK_APB_TIMER1),
	.timer_config_func = timer_cmsdk_apb_config_1,
	.timer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
			.device = CMSDK_APB_TIMER1,},
	.timer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
			.device = CMSDK_APB_TIMER1,},
	.timer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			 .device = CMSDK_APB_TIMER1,},
};

static struct timer_tmr_cmsdk_apb_dev_data timer_tmr_cmsdk_apb_dev_data_1 = {
	.load = TIMER_MAX_RELOAD,
	.status = TIMER_DISABLED,
};

DEVICE_AND_API_INIT(timer_tmr_cmsdk_apb_1,
		    CONFIG_TIMER_TMR_CMSDK_APB_1_DEV_NAME,
		    timer_tmr_cmsdk_apb_init, &timer_tmr_cmsdk_apb_dev_data_1,
		    &timer_tmr_cmsdk_apb_cfg_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &timer_tmr_cmsdk_apb_api);

static void timer_cmsdk_apb_config_1(struct device *dev)
{
	IRQ_CONNECT(CMSDK_APB_TIMER_1_IRQ, CONFIG_TIMER_TMR_CMSDK_APB_1_IRQ_PRI,
		    timer_tmr_cmsdk_apb_isr,
		    DEVICE_GET(timer_tmr_cmsdk_apb_0), 0);
	irq_enable(CMSDK_APB_TIMER_1_IRQ);
}
#endif /* CONFIG_TIMER_TMR_CMSDK_APB_1 */
