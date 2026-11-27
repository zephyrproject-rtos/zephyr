/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtc_jdp

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/clock.h>
#include <fsl_rtc.h>
#include <zephyr/logging/log.h>

/*
 * Skip the instance reserved as the system timer via zephyr,system-timer.
 * When both the counter and the system timer drivers are enabled, they must
 * operate on separate hardware (the RTC is a single instance).
 */
#define COUNTER_MCUX_RTC_JDP_IS_SYSTEM_TIMER(n)				\
	COND_CODE_1(DT_HAS_CHOSEN(zephyr_system_timer),			\
		(DT_SAME_NODE(DT_INST(n, nxp_rtc_jdp),			\
			      DT_CHOSEN(zephyr_system_timer))),		\
		(0))

#define COUNTER_MCUX_RTC_JDP_COUNT_USABLE(n) \
	+ (!COUNTER_MCUX_RTC_JDP_IS_SYSTEM_TIMER(n))

#define COUNTER_MCUX_RTC_JDP_DEVICE_COUNT \
	(0 DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_RTC_JDP_COUNT_USABLE))

#if COUNTER_MCUX_RTC_JDP_DEVICE_COUNT > 0

LOG_MODULE_REGISTER(mcux_rtc_jdp, CONFIG_COUNTER_LOG_LEVEL);

/* RTC and API channels */
#define RTC_CHANNEL_COUNT 2U
#define RTC_ALARM_CHANNEL 0U
#define API_ALARM_CHANNEL 1U

/*
 * Safety window (in RTC counter cycles) to avoid missing compares programmed
 * too close to the current count.
 *
 * Per RTC_JDP IP/SDK guidance, the minimum valid compare values also provide
 * a practical lower bound that avoids invalid/too-close programming.
 */
#define RTC_JDP_SAFETY_WINDOW_CYCLES(chan_id) \
	(((chan_id) == API_ALARM_CHANNEL) ? MINIMUM_APIVAL : MINIMUM_RTCVAL)

struct mcux_rtc_jdp_data {
	counter_alarm_callback_t alarm_callback[RTC_CHANNEL_COUNT];
	counter_top_callback_t top_callback;
	void *alarm_user_data[RTC_CHANNEL_COUNT];
	void *top_user_data;
	uint32_t guard_period;
	uint32_t sw_pending_mask;
	uint32_t wake_mask;
	struct k_spinlock lock;
};

struct mcux_rtc_jdp_config {
	struct counter_config_info info;
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	uint8_t clock_source;
	rtc_clock_divide_t clock_divide;
	uint8_t irqn;
	struct wuc_dt_spec wuc[RTC_CHANNEL_COUNT];
	bool wakeup_source;
};

static int mcux_rtc_jdp_enable_wakeup(const struct device *dev, uint8_t chan_id)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	int ret;

	if (!IS_ENABLED(CONFIG_WUC) || !config->wakeup_source ||
	    config->wuc[chan_id].dev == NULL ||
	    (IS_ENABLED(CONFIG_PM_DEVICE) && !pm_device_wakeup_is_enabled(dev))) {
		return 0;
	}

	ret = wuc_enable_wakeup_source_dt(&config->wuc[chan_id]);
	if (ret == 0) {
		data->wake_mask |= BIT(chan_id);
	}
	return ret;
}

static int mcux_rtc_jdp_disable_wakeup(const struct device *dev, uint8_t chan_id)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	int ret;

	if ((data->wake_mask & BIT(chan_id)) == 0U) {
		return 0;
	}

	/* Internal wake status can disappear when the RTC flag is cleared. */
	ret = wuc_check_wakeup_source_triggered_dt(&config->wuc[chan_id]);
	if (ret < 0 && ret != -ENOSYS) {
		return ret;
	}
	ret = wuc_disable_wakeup_source_dt(&config->wuc[chan_id]);
	if (ret == 0) {
		data->wake_mask &= ~BIT(chan_id);
	}
	return ret;
}

static int mcux_rtc_jdp_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t previous = data->wake_mask;
	int ret;

	for (uint8_t channel = 0U; channel < info->channels; channel++) {
		if (data->alarm_callback[channel] == NULL ||
		    (data->sw_pending_mask & BIT(channel)) != 0U) {
			continue;
		}
		ret = mcux_rtc_jdp_enable_wakeup(dev, channel);
		if (ret < 0) {
			for (uint8_t i = 0U; i < channel; i++) {
				if ((previous & BIT(i)) == 0U &&
				    mcux_rtc_jdp_disable_wakeup(dev, i) < 0) {
					LOG_ERR("Failed to roll back channel %u wake-up", i);
				}
			}
			k_spin_unlock(&data->lock, key);
			return ret;
		}
	}

	RTC_EnableRTC(config->base);
	RTC_EnableInterrupts(config->base,
			     (uint32_t)(kRTC_RTCInterruptEnable | kRTC_APIInterruptEnable |
					kRTC_CounterRollOverInterruptEnable));
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int mcux_rtc_jdp_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret;

	for (uint8_t channel = 0U; channel < info->channels; channel++) {
		ret = mcux_rtc_jdp_disable_wakeup(dev, channel);
		if (ret < 0) {
			k_spin_unlock(&data->lock, key);
			return ret;
		}
	}

	RTC_DisableInterrupts(config->base,
			      (uint32_t)(kRTC_RTCInterruptEnable | kRTC_APIInterruptEnable |
					 kRTC_CounterRollOverInterruptEnable));
	RTC_DisableRTC(config->base);
	RTC_DisableAPI(config->base);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_rtc_jdp_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);

	*ticks = RTC_GetCountValue(config->base);
	return 0;
}

/* Helper to program compare for a channel */
static inline int mcux_rtc_jdp_program_compare(const struct mcux_rtc_jdp_config *config,
					       uint8_t chan_id, uint32_t val)
{
	if (chan_id == RTC_ALARM_CHANNEL) {
		if (val <= MINIMUM_RTCVAL) {
			val = MINIMUM_RTCVAL + 1U;
		}
		RTC_SetRTCValue(config->base, val);
		RTC_EnableInterrupts(config->base, kRTC_RTCInterruptEnable);
		return 0;
	}

	if (chan_id == API_ALARM_CHANNEL) {
		if (val <= MINIMUM_APIVAL) {
			val = MINIMUM_APIVAL + 1U;
		}
		RTC_SetAPIValue(config->base, val);
		RTC_EnableInterrupts(config->base, kRTC_APIInterruptEnable);
		RTC_EnableAPI(config->base);
		return 0;
	}

	return -EINVAL;
}

static inline void mcux_rtc_jdp_alarm_drop(struct mcux_rtc_jdp_data *data, uint8_t chan_id)
{
	data->alarm_callback[chan_id] = NULL;
	data->alarm_user_data[chan_id] = NULL;
}

static inline void mcux_rtc_jdp_alarm_sw_pend(struct mcux_rtc_jdp_data *data,
				     const struct mcux_rtc_jdp_config *config,
				     uint8_t chan_id)
{
	data->sw_pending_mask |= BIT(chan_id);
	irq_enable(config->irqn);
	k_irq_set_pending(config->irqn);
}

static inline int mcux_rtc_jdp_alarm_handle_late_or_drop(struct mcux_rtc_jdp_data *data,
					const struct mcux_rtc_jdp_config *config,
					uint8_t chan_id,
					bool irq_on_late,
					int *retp,
					int ret)
{
	if (retp != NULL) {
		*retp = ret;
	}

	if (irq_on_late != false) {
		mcux_rtc_jdp_alarm_sw_pend(data, config, chan_id);
	} else {
		mcux_rtc_jdp_alarm_drop(data, chan_id);
	}

	return 1;
}

static inline int mcux_rtc_jdp_alarm_handle_sw_pend(struct mcux_rtc_jdp_data *data,
					    const struct mcux_rtc_jdp_config *config,
					    uint8_t chan_id,
					    int *retp,
					    int ret)
{
	if (retp != NULL) {
		*retp = ret;
	}

	mcux_rtc_jdp_alarm_sw_pend(data, config, chan_id);
	return 1;
}

/* Evaluate late/immediate and handle (SW fire or drop).
 * Returns 1 if handled (and stores return code in *retp), 0 otherwise.
 */
static inline int mcux_rtc_jdp_eval_and_handle(const struct device *dev, uint8_t chan_id,
					       bool is_abs, uint32_t target, uint32_t max_rel_val,
					       bool irq_on_late, int *retp)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	uint32_t now2 = RTC_GetCountValue(config->base);
	/* target - now (wrap-safe) */
	uint32_t fwd = target - now2;
	/* (target - 1) - now (wrap-safe) */
	uint32_t diff = (target - 1U) - now2;
	uint32_t abs_late_thresh;

	if (is_abs) {
		/*
		 * Late-to-set detection for absolute alarms.
		 *
		 * Zephyr's counter tests expect that setting an absolute alarm for the
		 * current tick (or slightly in the past) is treated as "late" and returns
		 * -ETIME, while still expiring immediately when EXPIRE_WHEN_LATE is set.
		 */
		abs_late_thresh = MAX(data->guard_period, 1U) + 1U;
		/*
		 * Determine whether the counter has just passed the intended target.
		 *
		 * Per Zephyr counter API, an absolute alarm set "in the past" may be treated
		 * as either late (return -ETIME) or intentional wrap-around. The guard period
		 * lets the driver decide: if the target is within the guard window behind the
		 * current tick, treat it as late; otherwise allow wrap-around.
		 *
		 * Use wrap-safe unsigned arithmetic; avoid signed deltas (targets may set bit 31).
		 */
		uint32_t back = now2 - target;

		if (back <= abs_late_thresh) {
			return mcux_rtc_jdp_alarm_handle_late_or_drop(data, config, chan_id,
							     irq_on_late, retp, -ETIME);
		}

		/*
		 * For close-future absolute deadlines, avoid missing the compare due to
		 * RTCVAL/APIVAL synchronization by expiring via SW-pending.
		 */
		if (fwd <= RTC_JDP_SAFETY_WINDOW_CYCLES(chan_id)) {
			return mcux_rtc_jdp_alarm_handle_sw_pend(data, config, chan_id, retp, 0);
		}

		return 0;
	}

	/*
	 * If the compare is within the synchronization safety window, don't rely on
	 * hardware match; trigger a SW-pending interrupt to deliver the callback.
	 */
	if (fwd <= RTC_JDP_SAFETY_WINDOW_CYCLES(chan_id)) {
		return mcux_rtc_jdp_alarm_handle_sw_pend(data, config, chan_id, retp, 0);
	}

	/* Relative */
	if ((diff > max_rel_val) || (diff == 0U)) {
		return mcux_rtc_jdp_alarm_handle_late_or_drop(data, config, chan_id,
							     irq_on_late, retp, 0);
	}

	return 0;
}

static int mcux_rtc_jdp_set_alarm(const struct device *dev, uint8_t chan_id,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_config_info *info;
	const struct mcux_rtc_jdp_config *config;
	struct mcux_rtc_jdp_data *data;
	uint32_t top;
	bool is_abs;
	uint32_t val;
	uint32_t now;
	bool irq_on_late;
	uint32_t max_rel_val;
	k_spinlock_key_t key;
	int err;

	info = dev->config;
	config = CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	data = dev->data;

	if ((chan_id >= info->channels) || (alarm_cfg == NULL) || (alarm_cfg->callback == NULL)) {
		return -EINVAL;
	}
	key = k_spin_lock(&data->lock);
	if (data->alarm_callback[chan_id] != NULL) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	top = info->max_top_value;
	is_abs = ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U) ? true : false;
	val = alarm_cfg->ticks;
	now = RTC_GetCountValue(config->base);
	err = 0;
	irq_on_late = false;
	max_rel_val = 0U;

	if (is_abs != false) {
		__ASSERT_NO_MSG(data->guard_period < top);
		irq_on_late = ((alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0U)
				      ? true
				      : false;
	} else {
		irq_on_late = (val < (top / 2U)) ? true : false;
		max_rel_val = (irq_on_late != false) ? (top / 2U) : top;
		/* wraps naturally on 32-bit */
		val = now + val;
	}

	/* Store handler first (single-shot semantics handled in ISR) */
	data->alarm_callback[chan_id] = alarm_cfg->callback;
	data->alarm_user_data[chan_id] = alarm_cfg->user_data;

	/* Re-read counter and evaluate/handle late or immediate */
	if (mcux_rtc_jdp_eval_and_handle(dev, chan_id, is_abs, val, max_rel_val, irq_on_late,
					 &err) != 0) {
		k_spin_unlock(&data->lock, key);
		return err;
	}

	err = mcux_rtc_jdp_enable_wakeup(dev, chan_id);
	if (err < 0) {
		mcux_rtc_jdp_alarm_drop(data, chan_id);
		k_spin_unlock(&data->lock, key);
		return err;
	}

	/* Normal case */
	irq_enable(config->irqn);
	err = mcux_rtc_jdp_program_compare(config, chan_id, val);
	k_spin_unlock(&data->lock, key);
	return err;
}

static int mcux_rtc_jdp_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	if (chan_id >= info->channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	ret = mcux_rtc_jdp_disable_wakeup(dev, chan_id);
	if (ret < 0) {
		k_spin_unlock(&data->lock, key);
		return ret;
	}
	if (chan_id == RTC_ALARM_CHANNEL) {
		RTC_DisableInterrupts(config->base, (uint32_t)(kRTC_RTCInterruptEnable));
	} else if (chan_id == API_ALARM_CHANNEL) {
		RTC_DisableInterrupts(config->base, (uint32_t)(kRTC_APIInterruptEnable));
		RTC_DisableAPI(config->base);
	}

	RTC_ClearInterruptFlags(config->base, chan_id == RTC_ALARM_CHANNEL ?
				kRTC_RTCInterruptFlag : kRTC_APIInterruptFlag);
	data->sw_pending_mask &= ~BIT(chan_id);
	mcux_rtc_jdp_alarm_drop(data, chan_id);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int mcux_rtc_jdp_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;

	if (cfg->ticks != info->max_top_value) {
		LOG_ERR("Top value can only be set to max value 0x%x", info->max_top_value);
		return -ENOTSUP;
	}

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		RTC_DisableRTC(config->base);
		/* Counter will reset to 0 when re-enabled */
		RTC_EnableRTC(config->base);
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	return 0;
}

static uint32_t mcux_rtc_jdp_get_pending_int(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);

	uint32_t flags =
		RTC_GetInterruptFlags(config->base) &
		(kRTC_RTCInterruptFlag | kRTC_APIInterruptFlag | kRTC_CounterRollOverInterruptFlag);

	return (flags != 0U) ? 1U : 0U;
}

static uint32_t mcux_rtc_jdp_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}

static uint32_t mcux_rtc_jdp_get_guard_period(const struct device *dev, uint32_t flags)
{
	struct mcux_rtc_jdp_data *data = dev->data;

	if (flags & COUNTER_GUARD_PERIOD_LATE_TO_SET) {
		return data->guard_period;
	}

	return 0;
}

static int mcux_rtc_jdp_set_guard_period(const struct device *dev, uint32_t ticks, uint32_t flags)
{
	struct mcux_rtc_jdp_data *data = dev->data;

	if (flags & COUNTER_GUARD_PERIOD_LATE_TO_SET) {
		data->guard_period = ticks;
		return 0;
	}

	return -ENOSYS;
}

static void mcux_rtc_jdp_isr(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;
	uint32_t status = RTC_GetInterruptFlags(config->base);
	uint32_t current = RTC_GetCountValue(config->base);

	for (uint8_t channel = 0U; channel < info->channels; channel++) {
		uint32_t flag = channel == RTC_ALARM_CHANNEL ?
			kRTC_RTCInterruptFlag : kRTC_APIInterruptFlag;
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		counter_alarm_callback_t cb;
		void *user_data;
		int ret;

		if ((status & flag) == 0U && (data->sw_pending_mask & BIT(channel)) == 0U) {
			k_spin_unlock(&data->lock, key);
			continue;
		}
		ret = mcux_rtc_jdp_disable_wakeup(dev, channel);
		if (ret < 0) {
			LOG_ERR("Cannot disarm channel %u wake-up: %d", channel, ret);
		}
		cb = data->alarm_callback[channel];
		user_data = data->alarm_user_data[channel];
		mcux_rtc_jdp_alarm_drop(data, channel);
		data->sw_pending_mask &= ~BIT(channel);
		RTC_ClearInterruptFlags(config->base, flag);
		k_spin_unlock(&data->lock, key);

		if (cb != NULL) {
			cb(dev, channel, current, user_data);
		}
	}

	/* Handle counter rollover interrupt */
	if ((status & kRTC_CounterRollOverInterruptFlag) != 0U) {
		if (data->top_callback != NULL) {
			data->top_callback(dev, data->top_user_data);
		}
		RTC_ClearInterruptFlags(config->base, kRTC_CounterRollOverInterruptFlag);
	}
}

static uint32_t mcux_rtc_jdp_get_freq(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->freq;
}

static int mcux_rtc_jdp_pm_action(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);

	/* The always-on counter keeps running; PM metadata controls wake-up policy. */
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_RESUME:
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mcux_rtc_jdp_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	rtc_config_t rtc_config;

	if (IS_ENABLED(CONFIG_WUC) && config->wakeup_source) {
		for (uint8_t channel = 0U; channel < info->channels; channel++) {
			if (config->wuc[channel].dev != NULL &&
			    !device_is_ready(config->wuc[channel].dev)) {
				LOG_ERR("Channel %u wake-up controller is not ready", channel);
				return -ENODEV;
			}
		}
	}

	/* Get default configuration */
	RTC_GetDefaultConfig(&rtc_config);

	/* Override with device-specific configuration */
	rtc_config.clockSource = (rtc_clock_source_t)config->clock_source;
	rtc_config.clockDivide = config->clock_divide;

	/* Initialize RTC */
	RTC_Init(config->base, &rtc_config);

	/* Clear any pending interrupts */
	RTC_ClearInterruptFlags(config->base, kRTC_AllInterruptFlags);

	/* Configure interrupts */
	config->irq_config_func(dev);

	/* Start the counter */
	RTC_EnableRTC(config->base);

	return pm_device_driver_init(dev, mcux_rtc_jdp_pm_action);
}

static DEVICE_API(counter, mcux_rtc_jdp_driver_api) = {
	.start = mcux_rtc_jdp_start,
	.stop = mcux_rtc_jdp_stop,
	.get_value = mcux_rtc_jdp_get_value,
	.set_alarm = mcux_rtc_jdp_set_alarm,
	.cancel_alarm = mcux_rtc_jdp_cancel_alarm,
	.set_top_value = mcux_rtc_jdp_set_top_value,
	.get_pending_int = mcux_rtc_jdp_get_pending_int,
	.get_top_value = mcux_rtc_jdp_get_top_value,
	.get_guard_period = mcux_rtc_jdp_get_guard_period,
	.set_guard_period = mcux_rtc_jdp_set_guard_period,
	.get_freq = mcux_rtc_jdp_get_freq,
};

/* Map numeric prescaler (1/32/512/16384) to SDK enum at build time */
#define RTC_JDP_DIV_ENUM(inst)                                             \
	(DT_INST_PROP(inst, prescaler) == 1     ? kRTC_ClockDivide1            \
	 : DT_INST_PROP(inst, prescaler) == 32  ? kRTC_ClockDivide32           \
	 : DT_INST_PROP(inst, prescaler) == 512 ? kRTC_ClockDivide512          \
						: kRTC_ClockDivide16384)

#define MCUX_RTC_JDP_INIT(n)                                               \
	static struct mcux_rtc_jdp_data mcux_rtc_jdp_data_##n;                 \
	static void mcux_rtc_jdp_irq_config_##n(const struct device *dev);     \
	static const struct mcux_rtc_jdp_config mcux_rtc_jdp_config_##n = {    \
		.base = (RTC_Type *)DT_INST_REG_ADDR(n),                           \
		.irq_config_func = mcux_rtc_jdp_irq_config_##n,                    \
		.clock_source = DT_INST_PROP(n, clock_source),                     \
		.clock_divide = RTC_JDP_DIV_ENUM(n),                               \
		.irqn = DT_INST_IRQN(n),                                           \
		.wuc = {COND_CODE_1(CONFIG_WUC,                                   \
			(WUC_DT_SPEC_INST_GET_BY_IDX_OR(n, 0, {0})), ({0})),           \
			COND_CODE_1(CONFIG_WUC,                                       \
			(WUC_DT_SPEC_INST_GET_BY_IDX_OR(n, 1, {0})), ({0}))},           \
		.wakeup_source = DT_INST_PROP_OR(n, wakeup_source, 0),             \
		.info =                                                            \
			{                                                              \
				.max_top_value = UINT32_MAX,                               \
				.freq = (DT_INST_PROP(n, clock_frequency) +                \
					(DT_INST_PROP(n, prescaler) / 2))                      \
					/ DT_INST_PROP(n, prescaler),                          \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                     \
				.channels = RTC_CHANNEL_COUNT,                             \
			},                                                             \
	};                                                                     \
	PM_DEVICE_DT_INST_DEFINE(n, mcux_rtc_jdp_pm_action);                  \
	DEVICE_DT_INST_DEFINE(n, &mcux_rtc_jdp_init, PM_DEVICE_DT_INST_GET(n), \
				&mcux_rtc_jdp_data_##n,                                    \
				&mcux_rtc_jdp_config_##n.info, POST_KERNEL,                \
				CONFIG_COUNTER_INIT_PRIORITY, &mcux_rtc_jdp_driver_api);   \
	static void mcux_rtc_jdp_irq_config_##n(const struct device *dev)      \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),             \
				mcux_rtc_jdp_isr,                                          \
				DEVICE_DT_INST_GET(n), 0);                                 \
		irq_enable(DT_INST_IRQN(n));                                       \
	}

#define MCUX_RTC_JDP_INIT_COND(n)					\
	COND_CODE_0(COUNTER_MCUX_RTC_JDP_IS_SYSTEM_TIMER(n),		\
		(MCUX_RTC_JDP_INIT(n)), ())

DT_INST_FOREACH_STATUS_OKAY(MCUX_RTC_JDP_INIT_COND)

#endif /* COUNTER_MCUX_RTC_JDP_DEVICE_COUNT > 0 */
