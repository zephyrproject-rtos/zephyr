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

#include "dualtimer_cmsdk_apb.h"

typedef void (*dtimer_config_func_t)(struct device *dev);

static counter_callback_t user_cb;

#define DUALTIMER_MAX_RELOAD	0xFFFFFFFF

enum dtimer_status_t {
	DTIMER_DISABLED = 0,
	DTIMER_ENABLED,
};

struct timer_dtmr_cmsdk_apb_cfg {
	volatile struct dualtimer_cmsdk_apb *dtimer;
	dtimer_config_func_t dtimer_config_func;
	/* Dualtimer Clock control in Active State */
	const struct arm_clock_control_t dtimer_cc_as;
	/* Dualtimer Clock control in Sleep State */
	const struct arm_clock_control_t dtimer_cc_ss;
	/* Dualtimer Clock control in Deep Sleep State */
	const struct arm_clock_control_t dtimer_cc_dss;
};

struct timer_dtmr_cmsdk_apb_dev_data {
	uint32_t load;
	enum dtimer_status_t status;
};

static int timer_dtmr_cmsdk_apb_start(struct device *dev)
{
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_dtmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Set the dualtimer to Max reload */
	cfg->dtimer->timer1load = DUALTIMER_MAX_RELOAD;

	/* Enable the dualtimer in 32 bit mode */
	cfg->dtimer->timer1ctrl = (DUALTIMER_CTRL_EN | DUALTIMER_CTRL_SIZE_32);

	/* Update the status */
	data->status = DTIMER_ENABLED;

	return 0;
}

static int timer_dtmr_cmsdk_apb_stop(struct device *dev)
{
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_dtmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Disable the dualtimer */
	cfg->dtimer->timer1ctrl = 0x0;

	/* Update the status */
	data->status = DTIMER_DISABLED;

	return 0;
}

static uint32_t timer_dtmr_cmsdk_apb_read(struct device *dev)
{
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_dtmr_cmsdk_apb_dev_data *data = dev->driver_data;

	/* Return Timer Value */
	uint32_t value = 0;

	value = data->load - cfg->dtimer->timer1value;

	return value;
}

static int timer_dtmr_cmsdk_apb_set_alarm(struct device *dev,
					   counter_callback_t callback,
					   uint32_t count, void *user_data)
{
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;
	struct timer_dtmr_cmsdk_apb_dev_data *data = dev->driver_data;

	if (data->status == DTIMER_DISABLED) {
		return -ENOTSUP;
	}

	/* Set callback */
	user_cb = callback;

	/* Store the reload value */
	data->load = count;

	/* Set the timer to count */
	cfg->dtimer->timer1load = count;

	/* Enable IRQ */
	cfg->dtimer->timer1ctrl |= (DUALTIMER_CTRL_INTEN
				    | DUALTIMER_CTRL_MODE);

	return 0;
}

static uint32_t timer_dtmr_cmsdk_apb_get_pending_int(struct device *dev)
{
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	return cfg->dtimer->timer1ris;
}

static const struct counter_driver_api timer_dtmr_cmsdk_apb_api = {
	.start = timer_dtmr_cmsdk_apb_start,
	.stop = timer_dtmr_cmsdk_apb_stop,
	.read = timer_dtmr_cmsdk_apb_read,
	.set_alarm = timer_dtmr_cmsdk_apb_set_alarm,
	.get_pending_int = timer_dtmr_cmsdk_apb_get_pending_int,
};

static void timer_dtmr_cmsdk_apb_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	cfg->dtimer->timer1intclr = DUALTIMER_INTCLR;
	if (user_cb) {
		/* user_data paramenter is not used by this driver */
		(*user_cb)(dev, NULL);
	}

}

static int timer_dtmr_cmsdk_apb_init(struct device *dev)
{
	const struct timer_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	struct device *clk =
		device_get_binding(CONFIG_ARM_CLOCK_CONTROL_DEV_NAME);

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->dtimer_cc_as);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->dtimer_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->dtimer_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	cfg->dtimer_config_func(dev);

	return 0;
}

/* TIMER 0 */
#ifdef CONFIG_TIMER_DTMR_CMSDK_APB_0
static void dtimer_cmsdk_apb_config_0(struct device *dev);

static const struct timer_dtmr_cmsdk_apb_cfg timer_dtmr_cmsdk_apb_cfg_0 = {
	.dtimer = ((volatile struct dualtimer_cmsdk_apb *)CMSDK_APB_DTIMER),
	.dtimer_config_func = dtimer_cmsdk_apb_config_0,
	.dtimer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
			 .device = CMSDK_APB_DTIMER,},
	.dtimer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
			 .device = CMSDK_APB_DTIMER,},
	.dtimer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			  .device = CMSDK_APB_DTIMER,},
};

static struct timer_dtmr_cmsdk_apb_dev_data timer_dtmr_cmsdk_apb_dev_data_0 = {
	.load = DUALTIMER_MAX_RELOAD,
	.status = DTIMER_DISABLED,
};

DEVICE_AND_API_INIT(timer_dtmr_cmsdk_apb_0,
		    CONFIG_TIMER_DTMR_CMSDK_APB_0_DEV_NAME,
		    timer_dtmr_cmsdk_apb_init,
		    &timer_dtmr_cmsdk_apb_dev_data_0,
		    &timer_dtmr_cmsdk_apb_cfg_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &timer_dtmr_cmsdk_apb_api);

static void dtimer_cmsdk_apb_config_0(struct device *dev)
{
	IRQ_CONNECT(CMSDK_APB_DUALTIMER_IRQ,
		    CONFIG_TIMER_DTMR_CMSDK_APB_0_IRQ_PRI,
		    timer_dtmr_cmsdk_apb_isr,
		    DEVICE_GET(timer_dtmr_cmsdk_apb_0), 0);
	irq_enable(CMSDK_APB_DUALTIMER_IRQ);
}
#endif /* CONFIG_TIMER_DTMR_CMSDK_APB_0 */
