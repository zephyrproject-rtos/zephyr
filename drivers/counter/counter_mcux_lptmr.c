/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lptmr

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <fsl_lptmr.h>

struct mcux_lptmr_config {
	struct counter_config_info info;
	LPTMR_Type *base;
	lptmr_prescaler_clock_select_t clk_source;
	lptmr_prescaler_glitch_value_t prescaler_glitch;
	bool bypass_prescaler_glitch;
	lptmr_timer_mode_t mode;
	lptmr_pin_select_t pin;
	lptmr_pin_polarity_t polarity;
	bool free_running;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_lptmr_data {
	counter_top_callback_t top_callback;
	void *top_user_data;
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
	bool alarm_active;
	uint32_t alarm_target_ticks;
	uint32_t current_top;
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
	uint32_t now;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	/* Top value can only be changed when there is no active channel alarm. */
	if (data->alarm_active) {
		return -EBUSY;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;
	data->current_top = cfg->ticks;

	if (config->base->CSR & LPTMR_CSR_TEN_MASK) {
		/* If the 'COUNTER_TOP_CFG_DONT_RESET' flag is true, then we further examine
		 * the current counter value and the 'COUNTER_TOP_CFG_RESET_WHEN_LATE' flag.
		 */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			now = LPTMR_GetCurrentTimerCount(config->base);
			if (now >= cfg->ticks) {
				/* Current counter value is greater than or equal to the
				 * new top value.
				 * - If the 'COUNTER_TOP_CFG_RESET_WHEN_LATE' flag is set,
				 *   go through stop-configure-start LPTMR flow.
				 *
				 * - Else, return -ETIME.
				 */
				if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
					LPTMR_StopTimer(config->base);
					LPTMR_SetTimerPeriod(config->base, cfg->ticks);
					LPTMR_StartTimer(config->base);
				} else {
					return -ETIME;
				}
			} else {
				/* Current counter value is samller than the new top value.
				 * Update compare register without stopping (free-running mode).
				 */
				LPTMR_SetTimerPeriod(config->base, cfg->ticks);
			}
		} else {
			/* Go through stop-configure-start LPTMR flow. */
			LPTMR_StopTimer(config->base);
			LPTMR_SetTimerPeriod(config->base, cfg->ticks);
			LPTMR_StartTimer(config->base);
		}
	} else {
		LPTMR_SetTimerPeriod(config->base, cfg->ticks);
	}

	return 0;
}

static uint32_t mcux_lptmr_get_pending_int(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	return LPTMR_GetStatusFlags(config->base);
}

static uint32_t mcux_lptmr_get_top_value(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	return (config->base->CMR & LPTMR_CMR_COMPARE_MASK) + 1U;
}

static uint32_t mcux_lptmr_get_freq(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;

	return config->info.freq;
}

static void mcux_lptmr_isr(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	counter_alarm_callback_t callback = data->alarm_callback;

	if ((callback != NULL) && (data->alarm_active)) {
		/* Stop the one-shot alarm timer, CSR[TCF] is cleared when LPTMR is disabled. */
		LPTMR_StopTimer(config->base);

		/* Mask LPTMR interrupt first to avoid a second ISR due to reprogramming */
		LPTMR_DisableInterrupts(config->base, kLPTMR_TimerInterruptEnable);

		/* Set back the current top value */
		LPTMR_SetTimerPeriod(config->base, data->current_top);

		data->alarm_callback = NULL;
		data->alarm_user_data = NULL;
		data->alarm_target_ticks = 0U;
		data->alarm_active = false;

		/* Re-enable LPTMR and interrupt if periodic top callback is not NULL. */
		if (data->top_callback != NULL) {
			LPTMR_EnableInterrupts(config->base, kLPTMR_TimerInterruptEnable);
			LPTMR_StartTimer(config->base);
		}

		callback(dev, 0, data->alarm_target_ticks, data->alarm_user_data);
	} else if (data->top_callback) {
		/* Free-running top/overflow callback */
		LPTMR_ClearStatusFlags(config->base, LPTMR_GetStatusFlags(config->base));
		data->top_callback(dev, data->top_user_data);
	} else {
		/* No consumer registered, just ack the interrupt. */
		LPTMR_ClearStatusFlags(config->base, LPTMR_GetStatusFlags(config->base));
	}
}

static int mcux_lptmr_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);

	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	uint32_t now;
	uint32_t target;
	uint32_t delta;

	/* Counter device driver model constraint: Alarm callback cannot be NULL. */
	if ((alarm_cfg == NULL) || (alarm_cfg->callback == NULL)) {
		return -EINVAL;
	}

	if (data->alarm_active) {
		return -EBUSY;
	}

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) { /* Absolute */
		if (alarm_cfg->ticks > data->current_top) {
			return -EINVAL;
		}

		now = LPTMR_GetCurrentTimerCount(config->base);
		if (now >= alarm_cfg->ticks) { /* Already late */
			if (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) {
				/* Immediate expiration in caller context */
				alarm_cfg->callback(dev, 0, alarm_cfg->ticks, alarm_cfg->user_data);
				return 0;
			}
			return -ETIME;
		}

		target = alarm_cfg->ticks;
		delta = target - now;
	} else { /* Relative */
		delta = alarm_cfg->ticks;

		if (delta > data->current_top) { /* Too large delta */
			return -EINVAL;
		}

		now = LPTMR_GetCurrentTimerCount(config->base);
		/* Absolute target relative to now, modulo current top value */
		target = (now + delta) % data->current_top;
	}

	/* Ensure timer is stopped and status flags cleared before reprogramming
	 * so that a smaller new period does not immediately assert TCF due to
	 * an already advanced CNR value.
	 * CNR[COUNTER] is reset when LPTMR is disabled.
	 */
	LPTMR_StopTimer(config->base);
	LPTMR_DisableInterrupts(config->base, kLPTMR_TimerInterruptEnable);

	LPTMR_SetTimerPeriod(config->base, delta);
	LPTMR_EnableInterrupts(config->base, kLPTMR_TimerInterruptEnable);
	LPTMR_StartTimer(config->base);

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->alarm_target_ticks = target;
	data->alarm_active = true;

	return 0;
}

static int mcux_lptmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);

	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;

	/* Stop LPTMR, CNR[COUNTER] is reset when LPTMR is disabled.
	 * CSR[TCF] is cleared by disabling LPTMR.
	 */
	LPTMR_StopTimer(config->base);
	LPTMR_DisableInterrupts(config->base, kLPTMR_TimerInterruptEnable);

	/* Reset period back to current_top for free-run/top tracking; counter
	 * will be restarted when next alarm is set or when top callback active.
	 */
	LPTMR_SetTimerPeriod(config->base, data->current_top);

	if (data->top_callback) {
		/* Restart timer for periodic top callbacks */
		LPTMR_EnableInterrupts(config->base, kLPTMR_TimerInterruptEnable);
		LPTMR_StartTimer(config->base);
	}

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_target_ticks = 0U;
	data->alarm_active = false;

	return 0;
}

static int mcux_lptmr_init(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	lptmr_config_t lptmr_config;

	LPTMR_GetDefaultConfig(&lptmr_config);
	lptmr_config.timerMode = config->mode;
	lptmr_config.enableFreeRunning = config->free_running;
	lptmr_config.prescalerClockSource = config->clk_source;
	lptmr_config.bypassPrescaler = config->bypass_prescaler_glitch;
	lptmr_config.value = config->prescaler_glitch;

	if (config->mode == kLPTMR_TimerModePulseCounter) {
		lptmr_config.pinSelect = config->pin;
		lptmr_config.pinPolarity = config->polarity;
	}

	LPTMR_Init(config->base, &lptmr_config);

	LPTMR_SetTimerPeriod(config->base, config->info.max_top_value);

	/* Initialize runtime top tracker */
	((struct mcux_lptmr_data *)dev->data)->current_top = config->info.max_top_value;

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, mcux_lptmr_driver_api) = {
	.start = mcux_lptmr_start,
	.stop = mcux_lptmr_stop,
	.get_value = mcux_lptmr_get_value,
	.set_top_value = mcux_lptmr_set_top_value,
	.get_pending_int = mcux_lptmr_get_pending_int,
	.get_top_value = mcux_lptmr_get_top_value,
	.get_freq = mcux_lptmr_get_freq,
	.set_alarm = mcux_lptmr_set_alarm,
	.cancel_alarm = mcux_lptmr_cancel_alarm,
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
				BIT(DT_INST_PROP(n, prescale_glitch_filter)),	\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = 1,						\
		},								\
		.base = (LPTMR_Type *)DT_INST_REG_ADDR(n),			\
		.clk_source = DT_INST_PROP(n, clk_source),			\
		.bypass_prescaler_glitch = (DT_INST_PROP(n,			\
			prescale_glitch_filter) == 0),				\
		.mode = DT_INST_PROP(n, timer_mode_sel),			\
		.pin = DT_INST_PROP_OR(n, input_pin, 0),			\
		.polarity = DT_INST_PROP(n, active_low),			\
		.free_running = DT_INST_PROP(n, free_running),			\
		.prescaler_glitch = (DT_INST_PROP(n,				\
			prescale_glitch_filter) == 0) ? 0 : DT_INST_PROP(n,	\
			prescale_glitch_filter) + DT_INST_PROP(n,		\
			timer_mode_sel) - 1,					\
		.irq_config_func = mcux_lptmr_irq_config_##n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &mcux_lptmr_init, NULL,			\
		&mcux_lptmr_data_##n,						\
		&mcux_lptmr_config_##n,						\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,			\
		&mcux_lptmr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_LPTMR_DEVICE_INIT)
