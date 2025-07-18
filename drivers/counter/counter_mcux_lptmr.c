/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lptmr

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <fsl_lptmr.h>

#define NUM_CHANNELS 1

struct mcux_lptmr_config {
	struct counter_config_info info;
	LPTMR_Type *base;
	lptmr_prescaler_clock_select_t clk_source;
	lptmr_prescaler_glitch_value_t prescaler_glitch;
	bool bypass_prescaler_glitch;
	lptmr_timer_mode_t mode;
	lptmr_pin_select_t pin;
	lptmr_pin_polarity_t polarity;
	void (*irq_config_func)(const struct device *dev);
	unsigned int irqn;
};

struct mcux_lptmr_data {
	counter_top_callback_t top_callback;
	counter_alarm_callback_t alarm_callback;
	void *top_user_data;
	void *alarm_user_data;
	uint32_t guard_period;
};

static int mcux_lptmr_start(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	LPTMR_EnableInterrupts(config->base,
			       kLPTMR_TimerInterruptEnable);
	LPTMR_StartTimer(config->base);

	return 0;
}

static int mcux_lptmr_stop(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	LPTMR_DisableInterrupts(config->base,
				kLPTMR_TimerInterruptEnable);
	LPTMR_StopTimer(config->base);

	return 0;
}

static int mcux_lptmr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_lptmr_config *config = dev->config;

	*ticks = LPTMR_GetCurrentTimerCount(config->base);

	return 0;
}

static int mcux_lptmr_set_top_value(const struct device *dev,
				    const struct counter_top_cfg *cfg)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (config->base->CSR & LPTMR_CSR_TEN_MASK) {
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			return -ENOTSUP;
		}
		LPTMR_StopTimer(config->base);
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
		LPTMR_StartTimer(config->base);
	} else {
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
	}

	return 0;
}

static uint32_t mcux_lptmr_get_pending_int(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	uint32_t mask = LPTMR_CSR_TCF_MASK | LPTMR_CSR_TIE_MASK;
	uint32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);

	return ((flags & mask) == mask);
}

static uint32_t mcux_lptmr_get_top_value(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	return (config->base->CMR & LPTMR_CMR_COMPARE_MASK) + 1U;
}

static void mcux_lptmr_isr(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	uint32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);
	LPTMR_ClearStatusFlags(config->base, flags);

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}

	if (data->alarm_callback) {
		uint32_t ticks = LPTMR_GetCurrentTimerCount(config->base);
		counter_alarm_callback_t alarm_callback = data->alarm_callback;
		void *alarm_user_data = data->alarm_user_data;

		data->alarm_callback = NULL;
		data->alarm_user_data = NULL;
		alarm_callback(dev, 0, ticks, alarm_user_data);
	}
}

static int mcux_lptmr_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	struct mcux_lptmr_data *data = dev->data;

	ARG_UNUSED(flags);

	__ASSERT_NO_MSG(guard < mcux_lptmr_get_top_value(dev));
	data->guard_period = guard;

	return 0;
}

static uint32_t mcux_lptmr_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct mcux_lptmr_data *data = dev->data;

	ARG_UNUSED(flags);

	return data->guard_period;
}

static int mcux_lptmr_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	uint32_t current;
	uint32_t max_rel_val;
	bool irq_on_late = false;
	uint32_t ticks = alarm_cfg->ticks;
	bool absolute = alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE;
	int err = 0;
	uint32_t top = mcux_lptmr_get_top_value(dev);

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	if (chan_id != 0) {
		return -EINVAL;
	}

	if (data->alarm_callback) {
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	current = LPTMR_GetCurrentTimerCount(config->base);
	max_rel_val = (current + data->guard_period) % config->info.max_top_value;

	if (absolute == 0) {
		/*
		 * If relative value is smaller than half of the counter range it is assumed
		 * that there is a risk of setting value too late and late detection algorithm
		 * must be applied. When late setting is detected, interrupt shall be
		 * triggered for immediate expiration of the timer. Detection is performed
		 * by limiting relative distance between CMP and CNT.
		 *
		 * Note that half of counter range is an arbitrary value.
		 */
		irq_on_late = ticks < (config->info.max_top_value / 2);
		ticks = (ticks + current) % config->info.max_top_value;
	} else {
		irq_on_late = alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE;
		ticks = alarm_cfg->ticks % config->info.max_top_value;
	}

	/*
	 * When LPTMR is enabled, the CMR can be altered only when CSR[TCF] = 1, since this cannot
	 * be guaranteed, the timer must be stopped before setting the CMR.
	 */
	LPTMR_StopTimer(config->base);

	LPTMR_SetTimerPeriod(config->base, ticks);
	LPTMR_EnableInterrupts(config->base, kLPTMR_TimerInterruptEnable);

	LPTMR_StartTimer(config->base);

	if (ticks <= max_rel_val) {
		if (absolute) {
			err = -ETIME;
		}

		if (irq_on_late) {
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			data->alarm_callback = NULL;
		}
	}

	return err;
}

static int mcux_lptmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;

	if (chan_id != 0) {
		return -EINVAL;
	}

	LPTMR_DisableInterrupts(config->base, kLPTMR_TimerInterruptEnable);
	data->alarm_callback = NULL;

	return 0;
}

static int mcux_lptmr_init(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	lptmr_config_t lptmr_config;

	LPTMR_GetDefaultConfig(&lptmr_config);
	lptmr_config.timerMode = config->mode;
	lptmr_config.enableFreeRunning = false;
	lptmr_config.prescalerClockSource = config->clk_source;
	lptmr_config.bypassPrescaler = config->bypass_prescaler_glitch;
	lptmr_config.value = config->prescaler_glitch;

	if (config->mode == kLPTMR_TimerModePulseCounter) {
		lptmr_config.pinSelect = config->pin;
		lptmr_config.pinPolarity = config->polarity;
	}

	LPTMR_Init(config->base, &lptmr_config);

	LPTMR_SetTimerPeriod(config->base, config->info.max_top_value);

	config->irq_config_func(dev);

	data->guard_period = 0;

	return 0;
}

static DEVICE_API(counter, mcux_lptmr_driver_api) = {
	.start = mcux_lptmr_start,
	.stop = mcux_lptmr_stop,
	.get_value = mcux_lptmr_get_value,
	.set_top_value = mcux_lptmr_set_top_value,
	.get_pending_int = mcux_lptmr_get_pending_int,
	.get_top_value = mcux_lptmr_get_top_value,
	.set_alarm = mcux_lptmr_set_alarm,
	.cancel_alarm = mcux_lptmr_cancel_alarm,
	.set_guard_period = mcux_lptmr_set_guard_period,
	.get_guard_period = mcux_lptmr_get_guard_period,
};

#define COUNTER_MCUX_LPTMR_DEVICE_INIT(n)					\
	static void mcux_lptmr_irq_config_##n(const struct device *dev)		\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			mcux_lptmr_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static struct mcux_lptmr_data mcux_lptmr_data_##n;			\
	static void mcux_lptmr_irq_config_##n(const struct device *dev);	\
										\
	BUILD_ASSERT(!(DT_INST_PROP(n, timer_mode_sel) == 1 &&			\
		DT_INST_PROP(n, prescale_glitch_filter) == 16),			\
		"Pulse mode cannot have a glitch value of 16");			\
										\
	BUILD_ASSERT(DT_INST_PROP(n, resolution) <= 32 &&			\
		DT_INST_PROP(n, resolution) > 0,				\
		"LPTMR resolution property should be a width between 0 and 32");\
										\
	static struct mcux_lptmr_config mcux_lptmr_config_##n = {		\
		.info = {							\
			.max_top_value =					\
				GENMASK(DT_INST_PROP(n, resolution) - 1, 0),	\
			.freq = DT_INST_PROP(n, clock_frequency) /		\
				DT_INST_PROP(n, prescaler),			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = NUM_CHANNELS,                                          \
		},								\
		.base = (LPTMR_Type *)DT_INST_REG_ADDR(n),			\
		.clk_source = DT_INST_PROP(n, clk_source),			\
		.bypass_prescaler_glitch =					\
			1 - DT_INST_PROP(n, timer_mode_sel),			\
		.mode = DT_INST_PROP(n, timer_mode_sel),			\
		.pin = DT_INST_PROP_OR(n, input_pin, 0),			\
		.polarity = DT_INST_PROP(n, active_low),			\
		.prescaler_glitch = DT_INST_PROP(n, prescale_glitch_filter) +	\
			DT_INST_PROP(n, timer_mode_sel) - 1,			\
		.irq_config_func = mcux_lptmr_irq_config_##n,			\
		.irqn = DT_INST_IRQN(n),                                \
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &mcux_lptmr_init, NULL,			\
		&mcux_lptmr_data_##n,						\
		&mcux_lptmr_config_##n,						\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,			\
		&mcux_lptmr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_LPTMR_DEVICE_INIT)
