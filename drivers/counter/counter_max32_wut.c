/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max32_wut

#include <zephyr/drivers/counter.h>
#include <zephyr/dt-bindings/clock/adi_max32_clock.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <wut.h>
#include <wrap_max32_lp.h>
#include <wrap_max32_sys.h>

LOG_MODULE_REGISTER(counter_max32_wut, CONFIG_COUNTER_LOG_LEVEL);

#define MAX32_WUT_COUNTER_FREQ 32768

struct max32_wut_alarm_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct max32_wut_data {
	struct max32_wut_alarm_data alarm;
	uint32_t guard_period;
};

struct max32_wut_config {
	struct counter_config_info info;
	mxc_wut_regs_t *regs;
	int clock_source;
	int prescaler;
	void (*irq_config)(const struct device *dev);
	uint32_t irq_number;
	bool wakeup_source;
};

static int counter_max32_wut_start(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;

	MXC_WUT_Enable(cfg->regs);

	return 0;
}

static int counter_max32_wut_stop(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;

	MXC_WUT_Disable(cfg->regs);

	return 0;
}

static int counter_max32_wut_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct max32_wut_config *cfg = dev->config;

	*ticks = MXC_WUT_GetCount(cfg->regs);

	return 0;
}

static int counter_max32_wut_set_top_value(const struct device *dev,
					   const struct counter_top_cfg *top_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(top_cfg);

	return -ENOTSUP;
}

static uint32_t counter_max32_wut_get_pending_int(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;

	return MXC_WUT_GetFlags(cfg->regs);
}

static uint32_t counter_max32_wut_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return UINT32_MAX;
}

static uint32_t counter_max32_wut_get_freq(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;

	return cfg->info.freq;
}

static uint32_t counter_max32_wut_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct max32_wut_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int counter_max32_wut_set_guard_period(const struct device *dev, uint32_t ticks,
					      uint32_t flags)
{
	struct max32_wut_data *data = dev->data;

	ARG_UNUSED(flags);

	if (ticks > counter_max32_wut_get_top_value(dev)) {
		return -EINVAL;
	}

	data->guard_period = ticks;

	return 0;
}

static int counter_max32_wut_set_alarm(const struct device *dev, uint8_t chan,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	const struct max32_wut_config *cfg = dev->config;
	struct max32_wut_data *data = dev->data;
	uint32_t now_ticks, top_ticks;
	uint64_t abs_ticks, min_abs_ticks;
	bool irq_on_late = false;
	bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;

	counter_max32_wut_get_value(dev, &now_ticks);

	top_ticks = counter_max32_wut_get_top_value(dev);
	if (alarm_cfg->ticks > top_ticks) {
		return -EINVAL;
	}

	if (data->alarm.callback != NULL) {
		return -EBUSY;
	}

	MXC_WUT_ClearFlags(cfg->regs);

	data->alarm.callback = alarm_cfg->callback;
	data->alarm.user_data = alarm_cfg->user_data;

	if (absolute) {
		abs_ticks = alarm_cfg->ticks;
	} else {
		abs_ticks = (uint64_t)now_ticks + alarm_cfg->ticks;
	}

	min_abs_ticks = (uint64_t)now_ticks + data->guard_period;
	if ((!absolute && (abs_ticks < now_ticks)) || (abs_ticks > min_abs_ticks)) {
		MXC_WUT_SetCompare(cfg->regs, abs_ticks & top_ticks);
		MXC_WUT_Enable(cfg->regs);
		return 0;
	}

	irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
	if (irq_on_late || !absolute) {
		NVIC_SetPendingIRQ(cfg->irq_number);
	} else {
		data->alarm.callback = NULL;
		data->alarm.user_data = NULL;
	}

	return -ETIME;
}

static int counter_max32_wut_cancel_alarm(const struct device *dev, uint8_t chan)
{
	ARG_UNUSED(chan);
	struct max32_wut_data *data = dev->data;

	counter_max32_wut_stop(dev);

	data->alarm.callback = NULL;
	data->alarm.user_data = NULL;

	return 0;
}

static void counter_max32_wut_isr(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;
	struct max32_wut_data *data = dev->data;

	if (data->alarm.callback) {
		counter_alarm_callback_t cb = data->alarm.callback;

		data->alarm.callback = NULL;
		cb(dev, 0, MXC_WUT_GetCount(cfg->regs), data->alarm.user_data);
	}

	MXC_WUT_ClearFlags(cfg->regs);
}

static void counter_max32_wut_hw_init(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;

	Wrap_MXC_SYS_Select32KClockSource(cfg->clock_source);

	cfg->irq_config(dev);

	if (cfg->wakeup_source) {
		MXC_LP_EnableWUTAlarmWakeup();
	}
}

static int counter_max32_wut_init(const struct device *dev)
{
	const struct max32_wut_config *cfg = dev->config;
	uint8_t prescaler_lo, prescaler_hi;
	mxc_wut_pres_t pres;
	mxc_wut_cfg_t wut_cfg;

	counter_max32_wut_hw_init(dev);

	prescaler_lo = FIELD_GET(GENMASK(2, 0), LOG2(cfg->prescaler));
	prescaler_hi = FIELD_GET(BIT(3), LOG2(cfg->prescaler));

	pres = (prescaler_hi << MXC_F_WUT_CTRL_PRES3_POS) |
	       (prescaler_lo << MXC_F_WUT_CTRL_PRES_POS);

	MXC_WUT_Init(cfg->regs, pres);

	wut_cfg.mode = MXC_WUT_MODE_COMPARE;
	wut_cfg.cmp_cnt = cfg->info.max_top_value;
	MXC_WUT_Config(cfg->regs, &wut_cfg);

	MXC_WUT_SetCount(cfg->regs, 0);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int counter_max32_wut_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		counter_max32_wut_hw_init(dev);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(counter, counter_max32_wut_driver_api) = {
	.start = counter_max32_wut_start,
	.stop = counter_max32_wut_stop,
	.get_value = counter_max32_wut_get_value,
	.set_top_value = counter_max32_wut_set_top_value,
	.get_pending_int = counter_max32_wut_get_pending_int,
	.get_top_value = counter_max32_wut_get_top_value,
	.get_freq = counter_max32_wut_get_freq,
	.set_alarm = counter_max32_wut_set_alarm,
	.cancel_alarm = counter_max32_wut_cancel_alarm,
	.get_guard_period = counter_max32_wut_get_guard_period,
	.set_guard_period = counter_max32_wut_set_guard_period,
};

#define TIMER(_num)    DT_INST_PARENT(_num)
#define MAX32_TIM(idx) ((mxc_wut_regs_t *)DT_REG_ADDR(TIMER(idx)))

#define COUNTER_MAX32_WUT_DEFINE(_num)                                                             \
	static void max32_wut_irq_init_##_num(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(TIMER(_num)), DT_IRQ(TIMER(_num), priority),                   \
			    counter_max32_wut_isr, DEVICE_DT_INST_GET(_num), 0);                   \
		irq_enable(DT_IRQN(TIMER(_num)));                                                  \
	};                                                                                         \
	static const struct max32_wut_config max32_wut_config_##_num = {                           \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT32_MAX,                                       \
				.freq = MAX32_WUT_COUNTER_FREQ / DT_PROP(TIMER(_num), prescaler),  \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 1,                                                     \
			},                                                                         \
		.regs = (mxc_wut_regs_t *)DT_REG_ADDR(TIMER(_num)),                                \
		.clock_source =                                                                    \
			DT_PROP_OR(TIMER(_num), clock_source, ADI_MAX32_PRPH_CLK_SRC_ERTCO),       \
		.prescaler = DT_PROP(TIMER(_num), prescaler),                                      \
		.irq_config = max32_wut_irq_init_##_num,                                           \
		.irq_number = DT_IRQN(TIMER(_num)),                                                \
		.wakeup_source = DT_PROP(TIMER(_num), wakeup_source),                              \
	};                                                                                         \
	static struct max32_wut_data max32_wut_data##_num;                                         \
	PM_DEVICE_DT_INST_DEFINE(_num, counter_max32_wut_pm_action);                               \
	DEVICE_DT_INST_DEFINE(_num, &counter_max32_wut_init, PM_DEVICE_DT_INST_GET(_num),          \
			      &max32_wut_data##_num, &max32_wut_config_##_num, PRE_KERNEL_1,       \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_max32_wut_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MAX32_WUT_DEFINE)
