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

struct timer_cmsdk_apb {
	/* Offset: 0x000 (R/W) control register */
	volatile uint32_t ctrl;
	/* Offset: 0x004 (R/W) current value register */
	volatile uint32_t value;
	/* Offset: 0x008 (R/W) reload value register */
	volatile uint32_t reload;
	union {
		/* Offset: 0x00C (R/ ) interrupt status register */
		volatile uint32_t intstatus;
		/* Offset: 0x00C ( /W) interruptclear register */
		volatile uint32_t intclear;
	} intreg;
};

#define TIMER_CTRL_IRQ_EN       (1 << 3)
#define TIMER_CTRL_SEL_EXT_CLK  (1 << 2)
#define TIMER_CTRL_SEL_EXT_EN   (1 << 1)
#define TIMER_CTRL_EN           (1 << 0)
#define TIMER_CTRL_INT_CLEAR    (1 << 0)

#define TIMER_MAX_RELOAD	0xFFFFFFFF

struct counter_tmr_cmsdk_apb_cfg {
	volatile struct timer_cmsdk_apb *timer;
};

static int counter_tmr_cmsdk_apb_start(struct device *dev)
{
	const struct counter_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	/* Set the timer to Max reload */
	cfg->timer->reload = TIMER_MAX_RELOAD;

	/* Enable the timer */
	cfg->timer->ctrl = TIMER_CTRL_EN;

	return 0;
}

static int counter_tmr_cmsdk_apb_stop(struct device *dev)
{
	const struct counter_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	/* Disable the timer */
	cfg->timer->ctrl = 0x0;

	return 0;
}

static uint32_t counter_tmr_cmsdk_apb_read(struct device *dev)
{
	const struct counter_tmr_cmsdk_apb_cfg * const cfg =
						dev->config->config_info;

	/* Return Counter Value */
	uint32_t value = 0;

	value = TIMER_MAX_RELOAD - cfg->timer->value;

	return value;
}

static int counter_tmr_cmsdk_apb_set_alarm(struct device *dev,
					   counter_callback_t callback,
					   uint32_t count, void *user_data)
{
	return -ENODEV;
}

static const struct counter_driver_api counter_tmr_cmsdk_apb_api = {
	.start = counter_tmr_cmsdk_apb_start,
	.stop = counter_tmr_cmsdk_apb_stop,
	.read = counter_tmr_cmsdk_apb_read,
	.set_alarm = counter_tmr_cmsdk_apb_set_alarm,
};

static int counter_tmr_cmsdk_apb_init(struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

/* COUNTER 0 */
#ifdef CONFIG_COUNTER_TMR_CMSDK_APB_0
static const struct counter_tmr_cmsdk_apb_cfg counter_tmr_cmsdk_apb_cfg_0 = {
	.timer = ((volatile struct timer_cmsdk_apb *)CMSDK_APB_TIMER0),
};

DEVICE_AND_API_INIT(counter_tmr_cmsdk_apb_0,
		    CONFIG_COUNTER_TMR_CMSDK_APB_0_DEV_NAME,
		    counter_tmr_cmsdk_apb_init, NULL,
		    &counter_tmr_cmsdk_apb_cfg_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &counter_tmr_cmsdk_apb_api);
#endif /* CONFIG_COUNTER_TMR_CMSDK_APB_0 */

/* COUNTER 1 */
#ifdef CONFIG_COUNTER_TMR_CMSDK_APB_1
static const struct counter_tmr_cmsdk_apb_cfg counter_tmr_cmsdk_apb_cfg_1 = {
	.timer = ((volatile struct timer_cmsdk_apb *)CMSDK_APB_TIMER1),
};

DEVICE_AND_API_INIT(counter_tmr_cmsdk_apb_1,
		    CONFIG_COUNTER_TMR_CMSDK_APB_1_DEV_NAME,
		    counter_tmr_cmsdk_apb_init, NULL,
		    &counter_tmr_cmsdk_apb_cfg_1, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &counter_tmr_cmsdk_apb_api);
#endif /* CONFIG_COUNTER_TMR_CMSDK_APB_1 */
