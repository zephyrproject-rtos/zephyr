/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtc_jdp

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <fsl_rtc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mcux_rtc_jdp, CONFIG_COUNTER_LOG_LEVEL);

/* RTC and API channels */
#define RTC_CHANNEL_COUNT 2U
#define RTC_ALARM_CHANNEL 0U
#define API_ALARM_CHANNEL 1U

struct mcux_rtc_jdp_data {
	counter_alarm_callback_t alarm_callback[RTC_CHANNEL_COUNT];
	counter_top_callback_t top_callback;
	void *alarm_user_data[RTC_CHANNEL_COUNT];
	void *top_user_data;
	uint32_t guard_period;
	uint32_t sw_pending_mask;
};

struct mcux_rtc_jdp_config {
	struct counter_config_info info;
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	uint8_t clock_source;
	rtc_clock_divide_t clock_divide;
	uint8_t irqn;
};

static int mcux_rtc_jdp_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);

	RTC_EnableRTC(config->base);
	RTC_EnableInterrupts(config->base,
			     (uint32_t)(kRTC_RTCInterruptEnable | kRTC_APIInterruptEnable |
					kRTC_CounterRollOverInterruptEnable));
	return 0;
}

static int mcux_rtc_jdp_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);

	RTC_DisableInterrupts(config->base,
			      (uint32_t)(kRTC_RTCInterruptEnable | kRTC_APIInterruptEnable |
					 kRTC_CounterRollOverInterruptEnable));
	RTC_DisableRTC(config->base);
	RTC_DisableAPI(config->base);

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
		/* Wait to allow the compare value to latch */
		k_busy_wait(100U);
		RTC_EnableInterrupts(config->base, kRTC_APIInterruptEnable);
		RTC_EnableAPI(config->base);
		return 0;
	}

	return -EINVAL;
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
	/* now - target (wrap-safe) */
	uint32_t back = now2 - target;
	/* (target - 1) - now (wrap-safe) */
	uint32_t diff = (target - 1U) - now2;

	if (is_abs != false) {
		if (back <= data->guard_period) {
			if (retp != NULL) {
				*retp = -ETIME;
			}
			if (irq_on_late != false) {
				data->sw_pending_mask |= BIT(chan_id);
				irq_enable(config->irqn);
				NVIC_SetPendingIRQ(config->irqn);
			} else {
				data->alarm_callback[chan_id] = NULL;
				data->alarm_user_data[chan_id] = NULL;
			}
			return 1;
		}
		return 0;
	}

	/* Relative */
	if ((diff > max_rel_val) || (diff == 0U)) {
		if (retp != NULL) {
			*retp = 0;
		}
		if (irq_on_late != false) {
			data->sw_pending_mask |= BIT(chan_id);
			irq_enable(config->irqn);
			NVIC_SetPendingIRQ(config->irqn);
		} else {
			data->alarm_callback[chan_id] = NULL;
			data->alarm_user_data[chan_id] = NULL;
		}
		return 1;
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
	int err;

	info = dev->config;
	config = CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	data = dev->data;

	if ((chan_id >= info->channels) || (alarm_cfg == NULL) || (alarm_cfg->callback == NULL)) {
		return -EINVAL;
	}
	if (data->alarm_callback[chan_id] != NULL) {
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
		return err;
	}

	/* Normal case */
	irq_enable(config->irqn);
	return mcux_rtc_jdp_program_compare(config, chan_id, val);
}

static int mcux_rtc_jdp_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	struct mcux_rtc_jdp_data *data = dev->data;

	if (chan_id >= info->channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (chan_id == RTC_ALARM_CHANNEL) {
		RTC_DisableInterrupts(config->base, (uint32_t)(kRTC_RTCInterruptEnable));
	} else if (chan_id == API_ALARM_CHANNEL) {
		RTC_DisableInterrupts(config->base, (uint32_t)(kRTC_APIInterruptEnable));
		RTC_DisableAPI(config->base);
	} else {
		/* Unsupported channel */
		return -EINVAL;
	}

	data->alarm_callback[chan_id] = NULL;

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

	/* Handle RTC match interrupt or software-pending */
	if ((status & kRTC_RTCInterruptFlag) != 0U ||
	    (data->sw_pending_mask & BIT(RTC_ALARM_CHANNEL))) {
		if (data->alarm_callback[RTC_ALARM_CHANNEL] != NULL) {
			counter_alarm_callback_t cb = data->alarm_callback[RTC_ALARM_CHANNEL];
			void *ud = data->alarm_user_data[RTC_ALARM_CHANNEL];

			data->alarm_callback[RTC_ALARM_CHANNEL] = NULL;
			data->alarm_user_data[RTC_ALARM_CHANNEL] = NULL;
			data->sw_pending_mask &= ~BIT(RTC_ALARM_CHANNEL);
			cb(dev, RTC_ALARM_CHANNEL, current, ud);
		}

		RTC_ClearInterruptFlags(config->base, kRTC_RTCInterruptFlag);
	}

	/* Handle API match interrupt or software-pending */
	if ((status & kRTC_APIInterruptFlag) != 0U ||
	    (data->sw_pending_mask & BIT(API_ALARM_CHANNEL))) {
		if (data->alarm_callback[API_ALARM_CHANNEL] != NULL) {
			counter_alarm_callback_t cb = data->alarm_callback[API_ALARM_CHANNEL];
			void *ud = data->alarm_user_data[API_ALARM_CHANNEL];

			data->alarm_callback[API_ALARM_CHANNEL] = NULL;
			data->alarm_user_data[API_ALARM_CHANNEL] = NULL;
			data->sw_pending_mask &= ~BIT(API_ALARM_CHANNEL);
			cb(dev, API_ALARM_CHANNEL, current, ud);
		}

		RTC_ClearInterruptFlags(config->base, kRTC_APIInterruptFlag);
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

static int mcux_rtc_jdp_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_jdp_config *config =
		CONTAINER_OF(info, struct mcux_rtc_jdp_config, info);
	rtc_config_t rtc_config;

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

	return 0;
}

static const struct counter_driver_api mcux_rtc_jdp_driver_api = {
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
	DEVICE_DT_INST_DEFINE(n, &mcux_rtc_jdp_init, NULL,                     \
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

DT_INST_FOREACH_STATUS_OKAY(MCUX_RTC_JDP_INIT)
