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

/*
 * Skip the instance reserved as the system timer via zephyr,system-timer.
 * When both drivers are enabled, they must operate on separate hardware.
 */
#define COUNTER_TMR_CMSDK_IS_SYSTEM_TIMER(n)                                                       \
	COND_CODE_1(DT_HAS_CHOSEN(zephyr_system_timer),			\
		(DT_SAME_NODE(DT_INST(n, DT_DRV_COMPAT),		\
			      DT_CHOSEN(zephyr_system_timer))),		\
		(0))

#define COUNTER_TMR_CMSDK_COUNT_USABLE(n) + (!COUNTER_TMR_CMSDK_IS_SYSTEM_TIMER(n))

#define COUNTER_TMR_CMSDK_DEVICE_COUNT                                                             \
	(0 DT_INST_FOREACH_STATUS_OKAY(COUNTER_TMR_CMSDK_COUNT_USABLE))

#if COUNTER_TMR_CMSDK_DEVICE_COUNT > 0

/*
 * Check for 'clocks' phandle and get the clock_frequency.
 * Use the fallback value if the 'clocks' phandle doesn't exist.
 */
#define TIMER_CMSDK_FREQ(inst)                                                                     \
	DT_PROP_BY_PHANDLE_IDX_OR(DT_DRV_INST(inst), clocks, 0, clock_frequency, 24000000U)

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
#ifdef CONFIG_CLOCK_CONTROL
	/* clock control device handle */
	const struct device *const clk;
#endif
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

static DEVICE_API(counter, tmr_cmsdk_apb_api) = {
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
	const struct device *const clk = cfg->clk;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

#ifdef CONFIG_SOC_SERIES_BEETLE
	clock_control_on(clk, (clock_control_subsys_t) &cfg->timer_cc_as);
	clock_control_on(clk, (clock_control_subsys_t) &cfg->timer_cc_ss);
	clock_control_on(clk, (clock_control_subsys_t) &cfg->timer_cc_dss);
#endif /* CONFIG_SOC_SERIES_BEETLE */
#endif /* CONFIG_CLOCK_CONTROL */

	cfg->timer_config_func(dev);

	return 0;
}

#ifdef CONFIG_CLOCK_CONTROL
#define TMR_CMSDK_CLOCK_DEV_ASSIGN(inst) .clk = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),
#else
#define TMR_CMSDK_CLOCK_DEV_ASSIGN(inst)
#endif

#define TIMER_CMSDK_INIT(inst)                                                                     \
	static void timer_cmsdk_apb_config_##inst(const struct device *dev);                       \
                                                                                                   \
	static const struct tmr_cmsdk_apb_cfg tmr_cmsdk_apb_cfg_##inst = {                         \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = TIMER_CMSDK_FREQ(inst),                                    \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 0U,                                                    \
			},                                                                         \
		.timer = ((volatile struct timer_cmsdk_apb *)DT_INST_REG_ADDR(inst)),              \
		.timer_config_func = timer_cmsdk_apb_config_##inst,                                \
		.timer_cc_as =                                                                     \
			{                                                                          \
				.bus = CMSDK_APB,                                                  \
				.state = SOC_ACTIVE,                                               \
				.device = DT_INST_REG_ADDR(inst),                                  \
			},                                                                         \
		.timer_cc_ss =                                                                     \
			{                                                                          \
				.bus = CMSDK_APB,                                                  \
				.state = SOC_SLEEP,                                                \
				.device = DT_INST_REG_ADDR(inst),                                  \
			},                                                                         \
		.timer_cc_dss =                                                                    \
			{                                                                          \
				.bus = CMSDK_APB,                                                  \
				.state = SOC_DEEPSLEEP,                                            \
				.device = DT_INST_REG_ADDR(inst),                                  \
			},                                                                         \
		TMR_CMSDK_CLOCK_DEV_ASSIGN(inst)};                                                 \
                                                                                                   \
	static struct tmr_cmsdk_apb_dev_data tmr_cmsdk_apb_dev_data_##inst = {                     \
		.load = UINT32_MAX,                                                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, tmr_cmsdk_apb_init, NULL, &tmr_cmsdk_apb_dev_data_##inst,      \
			      &tmr_cmsdk_apb_cfg_##inst, POST_KERNEL,                              \
			      CONFIG_COUNTER_INIT_PRIORITY, &tmr_cmsdk_apb_api);                   \
                                                                                                   \
	static void timer_cmsdk_apb_config_##inst(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), tmr_cmsdk_apb_isr,    \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}

#define COUNTER_TMR_CMSDK_DEVICE_INIT_COND(n)                                                      \
	COND_CODE_0(COUNTER_TMR_CMSDK_IS_SYSTEM_TIMER(n),		\
		(TIMER_CMSDK_INIT(n)), ())

DT_INST_FOREACH_STATUS_OKAY(COUNTER_TMR_CMSDK_DEVICE_INIT_COND)
#endif /* COUNTER_TMR_CMSDK_DEVICE_COUNT > 0 */
