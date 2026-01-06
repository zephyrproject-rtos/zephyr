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
#include <zephyr/spinlock.h>

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
#if defined(CONFIG_COUNTER_MCUX_LPTMR_ALARM)
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
	bool alarm_active;
	struct k_spinlock lock;
	uint32_t guard_period;
#else
	counter_top_callback_t top_callback;
	void *top_user_data;
#endif
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

static uint32_t mcux_lptmr_get_pending_int(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	uint32_t mask = LPTMR_CSR_TCF_MASK | LPTMR_CSR_TIE_MASK;

	return (uint32_t)!!((config->base->CSR & mask) == mask);
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

#if defined(CONFIG_COUNTER_MCUX_LPTMR_ALARM)
static int mcux_lptmr_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	uint32_t current;
	uint32_t max_rel_val;
	bool irq_on_late = false;
	uint32_t ticks;
	bool absolute;
	int err = 0;
	uint32_t top = mcux_lptmr_get_top_value(dev);

	/* Counter API: Alarm callback cannot be NULL. */
	if ((alarm_cfg == NULL) || (alarm_cfg->callback == NULL)) {
		return -EINVAL;
	}

	/* Only channel 0 is supported */
	if (chan_id != 0U) {
		return -EINVAL;
	}

	/* Reject only absolute alarms that exceed current top */
	absolute = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U;
	if (absolute && (alarm_cfg->ticks > top)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->alarm_active = true;

	k_spin_unlock(&data->lock, key);

	current = LPTMR_GetCurrentTimerCount(config->base);
	max_rel_val = (current + data->guard_period) % config->info.max_top_value;
	ticks = alarm_cfg->ticks;
	/* 'absolute' already computed for validation above */

	if (!absolute) {
		/* Late detection heuristic for relative alarms. */
		irq_on_late = ticks < (config->info.max_top_value / 2U);
		ticks = (ticks + current) % config->info.max_top_value;
	} else {
		irq_on_late = (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0U;
		ticks = alarm_cfg->ticks % config->info.max_top_value;
	}

	/* HW constraint: stop before altering period (CMR) */
	LPTMR_StopTimer(config->base);
	LPTMR_SetTimerPeriod(config->base, ticks);
	LPTMR_EnableInterrupts(config->base, kLPTMR_TimerInterruptEnable);
	LPTMR_StartTimer(config->base);

	if (ticks <= max_rel_val) {
		err = -ETIME;
		if (irq_on_late) {
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			/* Cancel alarm silently when late and no immediate expire requested */
			k_spinlock_key_t key2 = k_spin_lock(&data->lock);

			data->alarm_callback = NULL;
			data->alarm_user_data = NULL;
			data->alarm_active = false;
			k_spin_unlock(&data->lock, key2);
		}
	}

	return err;
}

static int mcux_lptmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(chan_id);

	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (!data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	LPTMR_DisableInterrupts(config->base, kLPTMR_TimerInterruptEnable);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_active = false;

	k_spin_unlock(&data->lock, key);

	LPTMR_StopTimer(config->base);

	return 0;
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

static int mcux_lptmr_set_top_value(const struct device *dev,
				    const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}
#else
static int mcux_lptmr_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);

	return -ENOTSUP;
}

static int mcux_lptmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);

	return -ENOTSUP;
}

static int mcux_lptmr_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(guard);
	ARG_UNUSED(flags);

	return -ENOTSUP;
}

static uint32_t mcux_lptmr_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	return 0U;
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
#endif /* CONFIG_COUNTER_MCUX_LPTMR_ALARM */

static void mcux_lptmr_isr(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
	struct mcux_lptmr_data *data = dev->data;
	uint32_t flags;

	flags = LPTMR_GetStatusFlags(config->base);
	LPTMR_ClearStatusFlags(config->base, flags);

#if defined(CONFIG_COUNTER_MCUX_LPTMR_ALARM)
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	counter_alarm_callback_t callback = data->alarm_callback;

	if ((callback != NULL) && (data->alarm_active)) {
		void *user_data = data->alarm_user_data;

		LPTMR_DisableInterrupts(config->base, kLPTMR_TimerInterruptEnable);

		data->alarm_callback = NULL;
		data->alarm_user_data = NULL;
		data->alarm_active = false;

		k_spin_unlock(&data->lock, key);

		uint32_t current_count = LPTMR_GetCurrentTimerCount(config->base);

		LPTMR_StopTimer(config->base);
		LPTMR_SetTimerPeriod(config->base, config->info.max_top_value);

		callback(dev, 0, current_count, user_data);

		return;
	}

	k_spin_unlock(&data->lock, key);
#else
	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
#endif
}

static int mcux_lptmr_init(const struct device *dev)
{
	const struct mcux_lptmr_config *config = dev->config;
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

#if defined(CONFIG_COUNTER_MCUX_LPTMR_ALARM)
	struct mcux_lptmr_data *data = dev->data;

	data->guard_period = 0U;
#endif

	return 0;
}

static DEVICE_API(counter, mcux_lptmr_driver_api) = {
	.start = mcux_lptmr_start,
	.stop = mcux_lptmr_stop,
	.set_alarm = mcux_lptmr_set_alarm,
	.cancel_alarm = mcux_lptmr_cancel_alarm,
	.get_value = mcux_lptmr_get_value,
	.set_top_value = mcux_lptmr_set_top_value,
	.get_pending_int = mcux_lptmr_get_pending_int,
	.get_top_value = mcux_lptmr_get_top_value,
	.get_freq = mcux_lptmr_get_freq,
	.set_guard_period = mcux_lptmr_set_guard_period,
	.get_guard_period = mcux_lptmr_get_guard_period,
};

/*
 * Devicetree mapping notes
 * - In time counter mode, prescaler divides by 2^(value + 1)
 * - In pulse counter mode, glitch filter recognizes a change after 2^value edges
 * - prescale-glitch-filter-bypass bypasses prescaler/glitch filter entirely
 */
#define MCUX_LPTMR_PRESCALE_GLITCH_VAL(n) DT_INST_PROP(n, prescale_glitch_filter)
#define MCUX_LPTMR_MODE(n) DT_INST_PROP(n, timer_mode_sel)
/*
 * Default must be false so prescale-glitch-filter can be used without requiring
 * an explicit bypass property.
 */
#define MCUX_LPTMR_BYPASS(n) DT_INST_PROP_OR(n, prescale_glitch_filter_bypass, false)

#define MCUX_LPTMR_TIME_DIV(n) BIT(MCUX_LPTMR_PRESCALE_GLITCH_VAL(n) + 1)
#define MCUX_LPTMR_PULSE_DIV(n) BIT(MCUX_LPTMR_PRESCALE_GLITCH_VAL(n))

#define MCUX_LPTMR_EFFECTIVE_FREQ(n) \
	(MCUX_LPTMR_BYPASS(n) ? DT_INST_PROP(n, clock_frequency) : \
		((MCUX_LPTMR_MODE(n) == kLPTMR_TimerModeTimeCounter) ? \
			(DT_INST_PROP(n, clock_frequency) / MCUX_LPTMR_TIME_DIV(n)) : \
			(DT_INST_PROP(n, clock_frequency) / MCUX_LPTMR_PULSE_DIV(n))))

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
		DT_INST_PROP(n, prescale_glitch_filter) > 15),			\
		"prescale-glitch-filter must be in range 0..15");			\
								\
	BUILD_ASSERT(!(DT_INST_PROP(n, timer_mode_sel) == 1 &&			\
		!MCUX_LPTMR_BYPASS(n) &&				\
		DT_INST_PROP(n, prescale_glitch_filter) == 0),			\
		"Pulse mode: prescale-glitch-filter=0 is invalid unless bypass is enabled");\
										\
	BUILD_ASSERT(DT_INST_PROP(n, resolution) <= 32 &&			\
		DT_INST_PROP(n, resolution) > 0,				\
		"LPTMR resolution property should be a width between 0 and 32");\
										\
	static struct mcux_lptmr_config mcux_lptmr_config_##n = {		\
		.info = {							\
			.max_top_value =					\
				GENMASK(DT_INST_PROP(n, resolution) - 1, 0),	\
			.freq = MCUX_LPTMR_EFFECTIVE_FREQ(n),			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,			\
			.channels = 1,						\
		},								\
		.base = (LPTMR_Type *)DT_INST_REG_ADDR(n),			\
		.clk_source = DT_INST_PROP(n, clk_source),			\
		.bypass_prescaler_glitch = MCUX_LPTMR_BYPASS(n),	\
		.mode = DT_INST_PROP(n, timer_mode_sel),			\
		.pin = DT_INST_PROP_OR(n, input_pin, 0),			\
		.polarity = DT_INST_PROP(n, active_low),			\
		.prescaler_glitch = (lptmr_prescaler_glitch_value_t)		\
			MCUX_LPTMR_PRESCALE_GLITCH_VAL(n),			\
		.irqn = DT_INST_IRQN(n),							\
		.irq_config_func = mcux_lptmr_irq_config_##n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &mcux_lptmr_init, NULL,			\
		&mcux_lptmr_data_##n,						\
		&mcux_lptmr_config_##n,						\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,			\
		&mcux_lptmr_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_LPTMR_DEVICE_INIT)
