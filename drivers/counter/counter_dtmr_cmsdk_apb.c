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

#define DUALTIMER_MAX_RELOAD	0xFFFFFFFF

struct counter_dtmr_cmsdk_apb_cfg {
	volatile struct dualtimer_cmsdk_apb *dtimer;
	/* Dualtimer Clock control in Active State */
	const struct arm_clock_control_t dtimer_cc_as;
	/* Dualtimer Clock control in Sleep State */
	const struct arm_clock_control_t dtimer_cc_ss;
	/* Dualtimer Clock control in Deep Sleep State */
	const struct arm_clock_control_t dtimer_cc_dss;
};

static int counter_dtmr_cmsdk_apb_start(struct device *dev)
{
	const struct counter_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	/* Set the dualtimer to Max reload */
	cfg->dtimer->timer1load = DUALTIMER_MAX_RELOAD;

	/* Enable the dualtimer in 32 bit mode */
	cfg->dtimer->timer1ctrl = (DUALTIMER_CTRL_EN | DUALTIMER_CTRL_SIZE_32);

	return 0;
}

static int counter_dtmr_cmsdk_apb_stop(struct device *dev)
{
	const struct counter_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	/* Disable the dualtimer */
	cfg->dtimer->timer1ctrl = 0x0;

	return 0;
}

static uint32_t counter_dtmr_cmsdk_apb_read(struct device *dev)
{
	const struct counter_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	/* Return Counter Value */
	uint32_t value = 0;

	value = DUALTIMER_MAX_RELOAD - cfg->dtimer->timer1value;

	return value;
}

static int counter_dtmr_cmsdk_apb_set_alarm(struct device *dev,
					   counter_callback_t callback,
					   uint32_t count, void *user_data)
{
	return -ENODEV;
}

static const struct counter_driver_api counter_dtmr_cmsdk_apb_api = {
	.start = counter_dtmr_cmsdk_apb_start,
	.stop = counter_dtmr_cmsdk_apb_stop,
	.read = counter_dtmr_cmsdk_apb_read,
	.set_alarm = counter_dtmr_cmsdk_apb_set_alarm,
};

static int counter_dtmr_cmsdk_apb_init(struct device *dev)
{
#ifdef CONFIG_CLOCK_CONTROL
	/* Enable clock for subsystem */
	struct device *clk =
		device_get_binding(CONFIG_ARM_CLOCK_CONTROL_DEV_NAME);

	const struct counter_dtmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->dtimer_cc_as);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->dtimer_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t *) &cfg->dtimer_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#else
	ARG_UNUSED(dev);
#endif /* CONFIG_CLOCK_CONTROL */

	return 0;
}

/* COUNTER 0 */
#ifdef CONFIG_COUNTER_DTMR_CMSDK_APB_0
static const struct counter_dtmr_cmsdk_apb_cfg counter_dtmr_cmsdk_apb_cfg_0 = {
	.dtimer = ((volatile struct dualtimer_cmsdk_apb *)CMSDK_APB_DTIMER),
	.dtimer_cc_as = {.bus = CMSDK_APB, .state = SOC_ACTIVE,
			 .device = CMSDK_APB_DTIMER,},
	.dtimer_cc_ss = {.bus = CMSDK_APB, .state = SOC_SLEEP,
			 .device = CMSDK_APB_DTIMER,},
	.dtimer_cc_dss = {.bus = CMSDK_APB, .state = SOC_DEEPSLEEP,
			  .device = CMSDK_APB_DTIMER,},
};

DEVICE_AND_API_INIT(counter_dtmr_cmsdk_apb_0,
		    CONFIG_COUNTER_DTMR_CMSDK_APB_0_DEV_NAME,
		    counter_dtmr_cmsdk_apb_init, NULL,
		    &counter_dtmr_cmsdk_apb_cfg_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &counter_dtmr_cmsdk_apb_api);
#endif /* CONFIG_COUNTER_DTMR_CMSDK_APB_0 */
