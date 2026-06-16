/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_wake_timer

#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#if defined(CONFIG_GIC)
#include <zephyr/drivers/interrupt_controller/gic.h>
#endif /* CONFIG_GIC */
#include <zephyr/spinlock.h>
#include <fsl_waketimer.h>

/*
 * The WAKETIMER is a single-instance 32-bit down-counter clocked at 1 kHz
 * (the 16 kHz source divided by a 4-bit ripple counter, enabled through
 * WAKE_TIMER_CTRL[OSC_DIV_ENA]). Writing a non-zero value to WAKE_TIMER_CNT
 * loads the counter and launches a countdown; when the counter reaches zero it
 * raises WAKE_FLAG (and optionally an interrupt) and then halts itself until it
 * is reloaded. The hardware therefore behaves as a one-shot down-counter and
 * has a single count/reload register.
 *
 * Because of that single-register constraint the periodic top-value callback
 * and the single-shot alarm both need the same resource, so they are mutually
 * exclusive and selected at build time through CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM
 * (the same approach used by the MCUX LPTMR counter driver):
 *   - alarm disabled (default): the driver provides a periodic top-value
 *     counter (reloaded from the ISR) and reports alarms as unsupported.
 *   - alarm enabled: the driver provides a single-shot relative channel alarm
 *     and reports absolute alarms and set-top-value as unsupported.
 *
 * Limitation of the default (non-alarm) build: a read-only user that never
 * registers a top callback gets a counter that counts down from the top value
 * once and then halts at zero, rather than wrapping like a free-running
 * counter. Reloading on every period is intentionally avoided here so the wake
 * timer does not raise periodic wake events the application never requested.
 * With the default top value (the full 32-bit range, ~49 days at 1 kHz) this is
 * not observable in practice.
 *
 * The shared state and the hardware programming sequence are protected by a
 * spinlock so that a concurrent stop/cancel/set cannot leave the timer armed
 * (which, on a wake timer, would spuriously wake the chip from low power).
 */

struct mcux_wake_timer_config {
	struct counter_config_info info;
	WAKETIMER_Type *base;
	unsigned int irqn;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_wake_timer_data {
	struct k_spinlock lock;
#if defined(CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM)
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
	bool alarm_active;
	bool alarm_sw_pending;
#else
	counter_top_callback_t top_callback;
	void *top_user_data;
	uint32_t top;
#endif
};

static ALWAYS_INLINE void irq_set_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	arm_gic_irq_set_pending(irq);
#else
	NVIC_SetPendingIRQ(irq);
#endif /* CONFIG_GIC */
}

static ALWAYS_INLINE bool irq_is_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	return arm_gic_irq_is_pending(irq);
#else
	return NVIC_GetPendingIRQ((IRQn_Type)irq) != 0U;
#endif /* CONFIG_GIC */
}

static ALWAYS_INLINE void irq_clear_pending(unsigned int irq)
{
#if defined(CONFIG_GIC)
	arm_gic_irq_clear_pending(irq);
#else
	NVIC_ClearPendingIRQ((IRQn_Type)irq);
#endif /* CONFIG_GIC */
}

/* Load a non-zero count and launch the countdown. The counter must be halted
 * (it always is after init, a stop, or a previous time-out) before a write.
 */
static ALWAYS_INLINE void mcux_wake_timer_load(WAKETIMER_Type *base, uint32_t ticks)
{
	WAKETIMER_StartTimer(base, ticks);
}

static int mcux_wake_timer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_wake_timer_config *config = dev->config;

	*ticks = WAKETIMER_GetCurrentTimerValue(config->base);

	return 0;
}

static uint32_t mcux_wake_timer_get_pending_int(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;

	if ((config->base->WAKE_TIMER_CTRL &
	     WAKETIMER_WAKE_TIMER_CTRL_WAKE_FLAG_MASK) != 0U) {
		return 1U;
	}

	/* Also report pending if the IRQ is pending in the interrupt controller.
	 * This covers the software-pending path where we force the IRQ without a
	 * hardware time-out.
	 */
	if (((config->base->WAKE_TIMER_CTRL & WAKETIMER_WAKE_TIMER_CTRL_INTR_EN_MASK) != 0U) &&
	    irq_is_pending(config->irqn)) {
		return 1U;
	}

	return 0U;
}

static uint32_t mcux_wake_timer_get_freq(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;

	return config->info.freq;
}

#if defined(CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM)

static int mcux_wake_timer_start(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Starting resets the timer to a free-running readable down-counter and
	 * cancels any pending alarm, so the alarm state must be cleared here too;
	 * otherwise a later set_alarm() would keep returning -EBUSY.
	 */
	WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	WAKETIMER_HaltTimer(config->base);
	irq_clear_pending(config->irqn);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_sw_pending = false;
	data->alarm_active = false;

	mcux_wake_timer_load(config->base, config->info.max_top_value);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_wake_timer_stop(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	WAKETIMER_HaltTimer(config->base);
	irq_clear_pending(config->irqn);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_sw_pending = false;
	data->alarm_active = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_wake_timer_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ticks;
	bool sw_pending;

	/* Counter API: alarm callback cannot be NULL. */
	if ((chan_id >= config->info.channels) || (alarm_cfg == NULL) ||
	    (alarm_cfg->callback == NULL) ||
	    (alarm_cfg->ticks > config->info.max_top_value)) {
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U) {
		return -ENOTSUP;
	}

	ticks = alarm_cfg->ticks;

	/* A zero relative delay must fire from ISR context, because the hardware
	 * needs a non-zero load to start.
	 */
	sw_pending = (ticks == 0U);

	key = k_spin_lock(&data->lock);

	if (data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	data->alarm_active = true;
	data->alarm_sw_pending = sw_pending;

	/* Program the hardware while holding the lock: otherwise a concurrent
	 * cancel could return success and yet this code would leave the timer
	 * armed, spuriously waking the chip from low-power mode.
	 */
	WAKETIMER_HaltTimer(config->base);
	WAKETIMER_ClearStatusFlags(config->base, kWAKETIMER_WakeFlag);
	irq_clear_pending(config->irqn);
	WAKETIMER_EnableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);

	if (sw_pending) {
		/* Trigger the callback immediately through the interrupt path. */
		irq_set_pending(config->irqn);
	} else {
		mcux_wake_timer_load(config->base, ticks);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_wake_timer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key;

	if (chan_id >= config->info.channels) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (!data->alarm_active) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/* Disarm the hardware under the lock so the cancel actually takes effect
	 * even if it races with set_alarm()'s programming sequence.
	 */
	WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	WAKETIMER_HaltTimer(config->base);
	WAKETIMER_ClearStatusFlags(config->base, kWAKETIMER_WakeFlag);
	irq_clear_pending(config->irqn);

	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_sw_pending = false;
	data->alarm_active = false;

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_wake_timer_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static uint32_t mcux_wake_timer_get_top_value(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;

	return config->info.max_top_value;
}

#else /* !CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM */

static int mcux_wake_timer_start(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	WAKETIMER_HaltTimer(config->base);

	if (data->top_callback != NULL) {
		WAKETIMER_ClearStatusFlags(config->base, kWAKETIMER_WakeFlag);
		WAKETIMER_EnableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	} else {
		/*
		 * Read-only use: with no callback there is no ISR to reload the
		 * one-shot counter, so it counts down from the top value once and
		 * then halts at zero instead of wrapping (see the file header).
		 */
		WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	}

	mcux_wake_timer_load(config->base, data->top);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_wake_timer_stop(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	WAKETIMER_HaltTimer(config->base);
	irq_clear_pending(config->irqn);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_wake_timer_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);

	return -ENOTSUP;
}

static int mcux_wake_timer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);

	return -ENOTSUP;
}

static int mcux_wake_timer_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key;
	bool running;

	if (cfg->ticks == 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* The counter halts at zero, so a non-zero current value means it is
	 * still counting down (i.e. it was started).
	 */
	running = (WAKETIMER_GetCurrentTimerValue(config->base) != 0U);

	if (running && (cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		/* The hardware cannot change the period without reloading (and
		 * thereby resetting) the counter.
		 */
		k_spin_unlock(&data->lock, key);
		return -ENOTSUP;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;
	data->top = cfg->ticks;

	if (running) {
		WAKETIMER_HaltTimer(config->base);
		if (cfg->callback != NULL) {
			WAKETIMER_ClearStatusFlags(config->base, kWAKETIMER_WakeFlag);
			WAKETIMER_EnableInterrupts(config->base,
						   kWAKETIMER_WakeInterruptEnable);
		} else {
			WAKETIMER_DisableInterrupts(config->base,
						    kWAKETIMER_WakeInterruptEnable);
		}
		mcux_wake_timer_load(config->base, data->top);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static uint32_t mcux_wake_timer_get_top_value(const struct device *dev)
{
	const struct mcux_wake_timer_data *data = dev->data;

	return data->top;
}

#endif /* CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM */

static uint32_t mcux_wake_timer_get_guard_period(const struct device *dev, uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(flags);

	return 0U;
}

static int mcux_wake_timer_set_guard_period(const struct device *dev, uint32_t ticks,
					    uint32_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ticks);
	ARG_UNUSED(flags);

	/*
	 * The counter runs at 1 kHz, so the guard period used by the late-alarm
	 * detection would round down to zero ticks and be meaningless. Report
	 * guard periods as unsupported instead.
	 */
	return -ENOSYS;
}

static void mcux_wake_timer_isr(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;
	struct mcux_wake_timer_data *data = dev->data;
	k_spinlock_key_t key;

	WAKETIMER_ClearStatusFlags(config->base, kWAKETIMER_WakeFlag);

#if defined(CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM)
	key = k_spin_lock(&data->lock);

	if ((data->alarm_callback != NULL) && data->alarm_active) {
		counter_alarm_callback_t callback = data->alarm_callback;
		void *user_data = data->alarm_user_data;
		bool sw_pending = data->alarm_sw_pending;
		uint32_t current;

		WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);

		/* On a real time-out the counter has halted at zero. */
		current = sw_pending ? 0U :
			WAKETIMER_GetCurrentTimerValue(config->base);

		WAKETIMER_HaltTimer(config->base);

		data->alarm_callback = NULL;
		data->alarm_user_data = NULL;
		data->alarm_sw_pending = false;
		data->alarm_active = false;

		k_spin_unlock(&data->lock, key);

		callback(dev, 0, current, user_data);

		return;
	}

	k_spin_unlock(&data->lock, key);
#else
	key = k_spin_lock(&data->lock);
	counter_top_callback_t callback = data->top_callback;
	void *user_data = data->top_user_data;

	if (callback != NULL) {
		/* The hardware is one-shot: reload to emulate periodic operation
		 * before notifying the application.
		 */
		mcux_wake_timer_load(config->base, data->top);
	}

	k_spin_unlock(&data->lock, key);

	if (callback != NULL) {
		callback(dev, user_data);
	}
#endif /* CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM */
}

static int mcux_wake_timer_init(const struct device *dev)
{
	const struct mcux_wake_timer_config *config = dev->config;
	waketimer_config_t wt_config;

	WAKETIMER_GetDefaultConfig(&wt_config);
	/* The interrupt source is armed per-operation; the NVIC line is owned by
	 * the Zephyr IRQ subsystem, not by the HAL.
	 */
	wt_config.enableInterrupt = false;
	/* Required: divide the 16 kHz source down to the 1 kHz counter clock. */
	wt_config.enableOSCDivide = true;
	wt_config.callback = NULL;
	WAKETIMER_Init(config->base, &wt_config);

	WAKETIMER_DisableInterrupts(config->base, kWAKETIMER_WakeInterruptEnable);
	WAKETIMER_HaltTimer(config->base);

#if !defined(CONFIG_COUNTER_MCUX_WAKE_TIMER_ALARM)
	{
		struct mcux_wake_timer_data *data = dev->data;

		data->top = config->info.max_top_value;
	}
#endif

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, mcux_wake_timer_driver_api) = {
	.start = mcux_wake_timer_start,
	.stop = mcux_wake_timer_stop,
	.get_value = mcux_wake_timer_get_value,
	.set_alarm = mcux_wake_timer_set_alarm,
	.cancel_alarm = mcux_wake_timer_cancel_alarm,
	.set_top_value = mcux_wake_timer_set_top_value,
	.get_pending_int = mcux_wake_timer_get_pending_int,
	.get_top_value = mcux_wake_timer_get_top_value,
	.get_guard_period = mcux_wake_timer_get_guard_period,
	.set_guard_period = mcux_wake_timer_set_guard_period,
	.get_freq = mcux_wake_timer_get_freq,
};

#define COUNTER_MCUX_WAKE_TIMER_DEVICE_INIT(n)						\
	static void mcux_wake_timer_irq_config_##n(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),			\
			    mcux_wake_timer_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));						\
	}										\
											\
	static struct mcux_wake_timer_data mcux_wake_timer_data_##n;			\
											\
	static const struct mcux_wake_timer_config mcux_wake_timer_config_##n = {	\
		.info = {								\
			.max_top_value = UINT32_MAX,					\
			.freq = DT_INST_PROP_OR(n, clock_frequency, 1000),		\
			.flags = 0,							\
			.channels = 1,							\
		},									\
		.base = (WAKETIMER_Type *)DT_INST_REG_ADDR(n),				\
		.irqn = DT_INST_IRQN(n),						\
		.irq_config_func = mcux_wake_timer_irq_config_##n,			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, &mcux_wake_timer_init, NULL,				\
			      &mcux_wake_timer_data_##n,				\
			      &mcux_wake_timer_config_##n,				\
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			      &mcux_wake_timer_driver_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_WAKE_TIMER_DEVICE_INIT)
