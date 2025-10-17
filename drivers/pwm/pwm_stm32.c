/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2023 Nobleo Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_pwm

#include <errno.h>

#include <soc.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_tim.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/dt-bindings/pwm/stm32_pwm.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

LOG_MODULE_REGISTER(pwm_stm32, CONFIG_PWM_LOG_LEVEL);

/* L0 series MCUs only have 16-bit timers and don't have below macro defined */
#ifndef IS_TIM_32B_COUNTER_INSTANCE
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) (0)
#endif

#ifdef CONFIG_PWM_CAPTURE

/**
 * @brief Capture state when in 4-channel support mode
 */
enum capture_state {
	CAPTURE_STATE_IDLE = 0,
	CAPTURE_STATE_WAIT_FOR_UPDATE_EVENT = 1,
	CAPTURE_STATE_WAIT_FOR_PULSE_START = 2,
	CAPTURE_STATE_WAIT_FOR_PERIOD_END = 3
};

/** Return the complementary channel number
 * that is used to capture the end of the pulse.
 */
static const uint32_t complementary_channel[] = {2, 1, 4, 3};

struct pwm_stm32_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t period;
	uint32_t pulse;
	uint32_t overflows;
	uint8_t skip_irq;
	bool capture_period;
	bool capture_pulse;
	bool continuous;
	uint8_t channel;

	/* only used when four_channel_capture_support */
	enum capture_state state;
};

/* When PWM capture is done by resetting the counter with UIF then the
 * first capture is always nonsense, second is nonsense when polarity changed
 * This is not the case when using four-channel-support.
 */
#define SKIPPED_PWM_CAPTURES 2u

#endif /*CONFIG_PWM_CAPTURE*/

/** PWM data. */
struct pwm_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
	/* Reset controller device configuration */
	const struct reset_dt_spec reset;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_stm32_capture_data capture;
#endif /* CONFIG_PWM_CAPTURE */
};

/** PWM configuration. */
struct pwm_stm32_config {
	TIM_TypeDef *timer;
	uint32_t prescaler;
	uint32_t countermode;
	const struct stm32_pclken *pclken;
	size_t pclk_len;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
	const bool four_channel_capture_support;
#endif /* CONFIG_PWM_CAPTURE */
};

/** Maximum number of timer channels : some stm32 soc have 6 else only 4 */
#if defined(LL_TIM_CHANNEL_CH6)
#define TIMER_HAS_6CH 1
#define TIMER_MAX_CH 6u
#else
#define TIMER_HAS_6CH 0
#define TIMER_MAX_CH 4u
#endif

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_TIM_CHANNEL_CH1, LL_TIM_CHANNEL_CH2,
	LL_TIM_CHANNEL_CH3, LL_TIM_CHANNEL_CH4,
#if TIMER_HAS_6CH
	LL_TIM_CHANNEL_CH5, LL_TIM_CHANNEL_CH6
#endif
};

/** STM32 MCUs have between 1 and 4 complementary channels */
static const uint32_t ch2ll_n[] = {
#if defined(LL_TIM_CHANNEL_CH1N)
	LL_TIM_CHANNEL_CH1N,
#if defined(LL_TIM_CHANNEL_CH2N)
	LL_TIM_CHANNEL_CH2N,
#if defined(LL_TIM_CHANNEL_CH3N)
	LL_TIM_CHANNEL_CH3N,
#if defined(LL_TIM_CHANNEL_CH4N)
	LL_TIM_CHANNEL_CH4N,
#endif /* LL_TIM_CHANNEL_CH4N */
#endif /* LL_TIM_CHANNEL_CH3N */
#endif /* LL_TIM_CHANNEL_CH2N */
#endif /* LL_TIM_CHANNEL_CH1N */
};
/** Maximum number of complemented timer channels is ARRAY_SIZE(ch2ll_n)*/

/** Channel to compare set function mapping. */
static void (*const set_timer_compare[TIMER_MAX_CH])(TIM_TypeDef *,
						     uint32_t) = {
	LL_TIM_OC_SetCompareCH1, LL_TIM_OC_SetCompareCH2,
	LL_TIM_OC_SetCompareCH3, LL_TIM_OC_SetCompareCH4,
#if TIMER_HAS_6CH
	LL_TIM_OC_SetCompareCH5, LL_TIM_OC_SetCompareCH6
#endif
};

/** Channel to capture get function mapping. */
#if !defined(CONFIG_SOC_SERIES_STM32MP1X)
static uint32_t __maybe_unused (*const get_channel_capture[])(const TIM_TypeDef *) = {
#else
static uint32_t __maybe_unused (*const get_channel_capture[])(TIM_TypeDef *) = {
#endif
	LL_TIM_IC_GetCaptureCH1, LL_TIM_IC_GetCaptureCH2,
	LL_TIM_IC_GetCaptureCH3, LL_TIM_IC_GetCaptureCH4
};


/** Channel to enable capture interrupt mapping. */
static void __maybe_unused (*const enable_capture_interrupt[])(TIM_TypeDef *) = {
	LL_TIM_EnableIT_CC1, LL_TIM_EnableIT_CC2,
	LL_TIM_EnableIT_CC3, LL_TIM_EnableIT_CC4
};

/** Channel to disable capture interrupt mapping. */
static void __maybe_unused (*const disable_capture_interrupt[])(TIM_TypeDef *) = {
	LL_TIM_DisableIT_CC1, LL_TIM_DisableIT_CC2,
	LL_TIM_DisableIT_CC3, LL_TIM_DisableIT_CC4
};

/** Channel to is capture active flag mapping. */
#if !defined(CONFIG_SOC_SERIES_STM32MP1X)
static uint32_t __maybe_unused (*const is_capture_active[])(const TIM_TypeDef *) = {
#else
static uint32_t __maybe_unused (*const is_capture_active[])(TIM_TypeDef *) = {
#endif
	LL_TIM_IsActiveFlag_CC1, LL_TIM_IsActiveFlag_CC2,
	LL_TIM_IsActiveFlag_CC3, LL_TIM_IsActiveFlag_CC4
};

/** Channel to clearing capture flag mapping. */
static void __maybe_unused (*const clear_capture_interrupt[])(TIM_TypeDef *) = {
	LL_TIM_ClearFlag_CC1, LL_TIM_ClearFlag_CC2,
	LL_TIM_ClearFlag_CC3, LL_TIM_ClearFlag_CC4
};

/**
 * Obtain LL polarity from PWM flags.
 *
 * @param flags PWM flags.
 *
 * @return LL polarity.
 */
static uint32_t get_polarity(pwm_flags_t flags)
{
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		return LL_TIM_OCPOLARITY_HIGH;
	}

	return LL_TIM_OCPOLARITY_LOW;
}

/**
 * @brief  Check if LL counter mode is center-aligned.
 *
 * @param  ll_countermode LL counter mode.
 *
 * @return `true` when center-aligned, otherwise `false`.
 */
static inline bool is_center_aligned(const uint32_t ll_countermode)
{
	return ((ll_countermode == LL_TIM_COUNTERMODE_CENTER_DOWN) ||
		(ll_countermode == LL_TIM_COUNTERMODE_CENTER_UP) ||
		(ll_countermode == LL_TIM_COUNTERMODE_CENTER_UP_DOWN));
}

static int pwm_stm32_set_cycles(const struct device *dev, uint32_t channel,
				uint32_t period_cycles, uint32_t pulse_cycles,
				pwm_flags_t flags)
{
	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;

	uint32_t ll_channel;
	uint32_t current_ll_channel; /* complementary output if used */
	uint32_t negative_ll_channel;

	if (channel < 1u || channel > TIMER_MAX_CH) {
		LOG_ERR("Invalid channel (%d)", channel);
		return -EINVAL;
	}

	/*
	 * Non 32-bit timers count from 0 up to the value in the ARR register
	 * (16-bit). Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!IS_TIM_32B_COUNTER_INSTANCE(timer) &&
	    (period_cycles > UINT16_MAX + 1)) {
		LOG_ERR("Cannot set PWM output, value exceeds 16-bit timer limit.");
		return -ENOTSUP;
	}

#ifdef CONFIG_PWM_CAPTURE
	if (LL_TIM_IsEnabledIT_CC1(timer) || LL_TIM_IsEnabledIT_CC2(timer) ||
	    LL_TIM_IsEnabledIT_CC3(timer) || LL_TIM_IsEnabledIT_CC4(timer)) {
		LOG_ERR("Cannot set PWM output, capture in progress");
		return -EBUSY;
	}
#endif /* CONFIG_PWM_CAPTURE */

	ll_channel = ch2ll[channel - 1u];

	if (channel <= ARRAY_SIZE(ch2ll_n)) {
		negative_ll_channel = ch2ll_n[channel - 1u];
	} else {
		negative_ll_channel = 0;
	}

	/* in LL_TIM_CC_DisableChannel and LL_TIM_CC_IsEnabledChannel,
	 * the channel param could be the complementary one
	 */
	if ((flags & STM32_PWM_COMPLEMENTARY_MASK) == STM32_PWM_COMPLEMENTARY) {
		if (!negative_ll_channel) {
			/* setting a flag on a channel that has not this capability */
			LOG_ERR("Channel %d has NO complementary output", channel);
			return -EINVAL;
		}
		current_ll_channel = negative_ll_channel;
	} else {
		current_ll_channel = ll_channel;
	}

	if (period_cycles == 0u) {
		LL_TIM_CC_DisableChannel(timer, current_ll_channel);
		return 0;
	}

	if (cfg->countermode == LL_TIM_COUNTERMODE_UP) {
		/* remove 1 period cycle, accounts for 1 extra low cycle */
		period_cycles -= 1U;
	} else if (cfg->countermode == LL_TIM_COUNTERMODE_DOWN) {
		/* remove 1 pulse cycle, accounts for 1 extra high cycle */
		pulse_cycles -= 1U;
		/* remove 1 period cycle, accounts for 1 extra low cycle */
		period_cycles -= 1U;
	} else if (is_center_aligned(cfg->countermode)) {
		pulse_cycles /= 2U;
		period_cycles /= 2U;
	} else {
		return -ENOTSUP;
	}

	LL_TIM_OC_SetPolarity(timer, current_ll_channel, get_polarity(flags));
	set_timer_compare[channel - 1u](timer, pulse_cycles);
	LL_TIM_SetAutoReload(timer, period_cycles);

	if (!LL_TIM_CC_IsEnabledChannel(timer, current_ll_channel)) {
#ifdef CONFIG_PWM_CAPTURE
		if (IS_TIM_SLAVE_INSTANCE(timer)) {
			LL_TIM_SetSlaveMode(timer,
					LL_TIM_SLAVEMODE_DISABLED);
			LL_TIM_SetTriggerInput(timer, LL_TIM_TS_ITR0);
			LL_TIM_DisableMasterSlaveMode(timer);
		}
#endif /* CONFIG_PWM_CAPTURE */

		LL_TIM_OC_SetMode(timer, ll_channel, LL_TIM_OCMODE_PWM1);
#ifdef LL_TIM_OCIDLESTATE_LOW
		LL_TIM_OC_SetIdleState(timer, current_ll_channel, LL_TIM_OCIDLESTATE_LOW);
#endif
		LL_TIM_CC_EnableChannel(timer, current_ll_channel);
		LL_TIM_EnableARRPreload(timer);
		/* in LL_TIM_OC_EnablePreload, the channel is always the non-complementary */
		LL_TIM_OC_EnablePreload(timer, ll_channel);
		LL_TIM_GenerateEvent_UPDATE(timer);
	}

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static void init_capture_channels(const struct device *dev, uint32_t channel,
				pwm_flags_t flags)
{
	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;
	bool is_inverted = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;
	uint32_t ll_channel = ch2ll[channel - 1];
	uint32_t ll_complementary_channel = ch2ll[complementary_channel[channel - 1] - 1];


	/* Setup main channel */
	LL_TIM_IC_SetPrescaler(timer, ll_channel, LL_TIM_ICPSC_DIV1);
	LL_TIM_IC_SetFilter(timer, ll_channel, LL_TIM_IC_FILTER_FDIV1);
	LL_TIM_IC_SetActiveInput(timer, ll_channel, LL_TIM_ACTIVEINPUT_DIRECTTI);
	LL_TIM_IC_SetPolarity(timer, ll_channel,
			      is_inverted ? LL_TIM_IC_POLARITY_FALLING : LL_TIM_IC_POLARITY_RISING);

	/* Setup complementary channel */
	LL_TIM_IC_SetPrescaler(timer, ll_complementary_channel, LL_TIM_ICPSC_DIV1);
	LL_TIM_IC_SetFilter(timer, ll_complementary_channel, LL_TIM_IC_FILTER_FDIV1);
	LL_TIM_IC_SetActiveInput(timer, ll_complementary_channel, LL_TIM_ACTIVEINPUT_INDIRECTTI);
	LL_TIM_IC_SetPolarity(timer, ll_complementary_channel,
			      is_inverted ? LL_TIM_IC_POLARITY_RISING : LL_TIM_IC_POLARITY_FALLING);
}

static int pwm_stm32_configure_capture(const struct device *dev,
				       uint32_t channel, pwm_flags_t flags,
				       pwm_capture_callback_handler_t cb,
				       void *user_data)
{
	/*
	 * Capture is implemented in two different ways, depending on the
	 * four-channel-capture-support setting in the node.
	 *  - Two Channel Support:
	 *	Only two channels (1 and 2) are available for capture. It uses
	 *	the slave mode controller to reset the counter on each edge.
	 *  - Four Channel Support:
	 *	All four channels are available for capture. Instead of the
	 *	slave mode controller it uses the ISR to reset the counter.
	 *	This is slightly less accurate, but still within acceptable
	 *	bounds.
	 */

	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;
	struct pwm_stm32_data *data = dev->data;
	struct pwm_stm32_capture_data *cpt = &data->capture;

	if (!cfg->four_channel_capture_support) {
		if ((channel != 1u) && (channel != 2u)) {
			LOG_ERR("PWM capture only supported on first two channels");
			return -ENOTSUP;
		}
	} else {
		if ((channel < 1u) || (channel > 4u)) {
			LOG_ERR("PWM capture only exists on channels 1, 2, 3 and 4.");
			return -ENOTSUP;
		}
	}

	if (LL_TIM_IsEnabledIT_CC1(timer) || LL_TIM_IsEnabledIT_CC2(timer) ||
	    LL_TIM_IsEnabledIT_CC3(timer) || LL_TIM_IsEnabledIT_CC4(timer)) {
		LOG_ERR("PWM capture already in progress");
		return -EBUSY;
	}

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No PWM capture type specified");
		return -EINVAL;
	}

	if (!cfg->four_channel_capture_support && !IS_TIM_SLAVE_INSTANCE(timer)) {
		/* slave mode is only used when not in four channel mode */
		LOG_ERR("Timer does not support slave mode for PWM capture");
		return -ENOTSUP;
	}

	cpt->callback = cb; /* even if the cb is reset, this is not an error */
	cpt->user_data = user_data;
	cpt->capture_period = (flags & PWM_CAPTURE_TYPE_PERIOD) ? true : false;
	cpt->capture_pulse = (flags & PWM_CAPTURE_TYPE_PULSE) ? true : false;
	cpt->continuous = (flags & PWM_CAPTURE_MODE_CONTINUOUS) ? true : false;

	/* Prevents faulty behavior while making changes */
	LL_TIM_SetSlaveMode(timer, LL_TIM_SLAVEMODE_DISABLED);

	init_capture_channels(dev, channel, flags);

	if (!cfg->four_channel_capture_support) {
		if (channel == 1u) {
			LL_TIM_SetTriggerInput(timer, LL_TIM_TS_TI1FP1);
		} else {
			LL_TIM_SetTriggerInput(timer, LL_TIM_TS_TI2FP2);
		}
		LL_TIM_SetSlaveMode(timer, LL_TIM_SLAVEMODE_RESET);
	}

	LL_TIM_EnableARRPreload(timer);
	if (!IS_TIM_32B_COUNTER_INSTANCE(timer)) {
		LL_TIM_SetAutoReload(timer, 0xffffu);
	} else {
		LL_TIM_SetAutoReload(timer, 0xffffffffu);
	}
	LL_TIM_EnableUpdateEvent(timer);

	return 0;
}

static int pwm_stm32_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;
	struct pwm_stm32_data *data = dev->data;
	struct pwm_stm32_capture_data *cpt = &data->capture;

	if (!cfg->four_channel_capture_support) {
		if ((channel != 1u) && (channel != 2u)) {
			LOG_ERR("PWM capture only supported on first two channels");
			return -ENOTSUP;
		}
	} else {
		if ((channel < 1u) || (channel > 4u)) {
			LOG_ERR("PWM capture only exists on channels 1, 2, 3 and 4.");
			return -ENOTSUP;
		}
	}

	if (LL_TIM_IsEnabledIT_CC1(timer) || LL_TIM_IsEnabledIT_CC2(timer) ||
	    LL_TIM_IsEnabledIT_CC3(timer) || LL_TIM_IsEnabledIT_CC4(timer)) {
		LOG_ERR("PWM capture already active");
		return -EBUSY;
	}

	if (!data->capture.callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	cpt->channel = channel;
	cpt->state = CAPTURE_STATE_WAIT_FOR_PULSE_START;
	data->capture.skip_irq = cfg->four_channel_capture_support ?  0 : SKIPPED_PWM_CAPTURES;
	data->capture.overflows = 0u;

	clear_capture_interrupt[channel - 1](timer);
	LL_TIM_ClearFlag_UPDATE(timer);

	LL_TIM_SetUpdateSource(timer, LL_TIM_UPDATESOURCE_COUNTER);

	enable_capture_interrupt[channel - 1](timer);

	LL_TIM_CC_EnableChannel(timer, ch2ll[channel - 1]);
	LL_TIM_CC_EnableChannel(timer, ch2ll[complementary_channel[channel - 1] - 1]);
	LL_TIM_GenerateEvent_UPDATE(timer);

	return 0;
}

static int pwm_stm32_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;

	if (!cfg->four_channel_capture_support) {
		if ((channel != 1u) && (channel != 2u)) {
			LOG_ERR("PWM capture only supported on first two channels");
			return -ENOTSUP;
		}
	} else {
		if ((channel < 1u) || (channel > 4u)) {
			LOG_ERR("PWM capture only exists on channels 1, 2, 3 and 4.");
			return -ENOTSUP;
		}
	}

	LL_TIM_SetUpdateSource(timer, LL_TIM_UPDATESOURCE_REGULAR);

	disable_capture_interrupt[channel - 1](timer);

	LL_TIM_DisableIT_UPDATE(timer);
	LL_TIM_CC_DisableChannel(timer, ch2ll[channel - 1]);
	LL_TIM_CC_DisableChannel(timer, ch2ll[complementary_channel[channel - 1] - 1]);

	return 0;
}

static void pwm_stm32_isr(const struct device *dev)
{
	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;
	struct pwm_stm32_data *data = dev->data;
	struct pwm_stm32_capture_data *cpt = &data->capture;
	uint8_t channel = cpt->channel;
	int status = 0;

	if (cpt->skip_irq != 0u) {
		if (LL_TIM_IsActiveFlag_UPDATE(timer)) {
			LL_TIM_ClearFlag_UPDATE(timer);
		}

		if (LL_TIM_IsActiveFlag_CC1(timer)
				|| LL_TIM_IsActiveFlag_CC2(timer)
				|| LL_TIM_IsActiveFlag_CC3(timer)
				|| LL_TIM_IsActiveFlag_CC4(timer)) {
			LL_TIM_ClearFlag_CC1(timer);
			LL_TIM_ClearFlag_CC2(timer);
			LL_TIM_ClearFlag_CC3(timer);
			LL_TIM_ClearFlag_CC4(timer);
			cpt->skip_irq--;
		}

		return;
	}

	if (LL_TIM_IsActiveFlag_UPDATE(timer)) {
		LL_TIM_ClearFlag_UPDATE(timer);
		if (cfg->four_channel_capture_support &&
				cpt->state == CAPTURE_STATE_WAIT_FOR_UPDATE_EVENT) {
			/* Special handling of UPDATE event in case it's triggered */
			cpt->state = CAPTURE_STATE_WAIT_FOR_PERIOD_END;
		} else {
			cpt->overflows++;
		}
	}

	if (!cfg->four_channel_capture_support) {
		if (is_capture_active[channel - 1](timer) ||
		    is_capture_active[complementary_channel[channel - 1] - 1](timer)) {
			clear_capture_interrupt[channel - 1](timer);
			clear_capture_interrupt
					[complementary_channel[channel - 1] - 1](timer);

			cpt->period = get_channel_capture[channel - 1](timer);
			cpt->pulse = get_channel_capture
					[complementary_channel[channel - 1] - 1](timer);
		}
	} else {
		if (cpt->state == CAPTURE_STATE_WAIT_FOR_PULSE_START &&
		    is_capture_active[channel - 1](timer)) {
			/* Reset the counter manually instead of automatically by HW
			 * This sets the pulse-start at 0 and makes the pulse-end
			 * and period related to that number. Sure we loose some
			 * accuracy but it's within acceptable range.
			 *
			 * This is done through an UPDATE event to also reset
			 * the prescalar. This could look like an overflow event
			 * and might therefore require special handling.
			 */
			cpt->state = CAPTURE_STATE_WAIT_FOR_UPDATE_EVENT;
			LL_TIM_GenerateEvent_UPDATE(timer);

		} else if ((cpt->state == CAPTURE_STATE_WAIT_FOR_UPDATE_EVENT ||
				cpt->state == CAPTURE_STATE_WAIT_FOR_PERIOD_END) &&
			    is_capture_active[channel - 1](timer)) {
			cpt->state = CAPTURE_STATE_IDLE;
			/* The end of the period. Both capture channels should now contain
			 * the timer value when the pulse and period ended respectively.
			 */
			cpt->pulse = get_channel_capture[complementary_channel[channel - 1] - 1]
					(timer);
			cpt->period = get_channel_capture[channel - 1](timer);
			/* Reset the counter manually for next cycle */
			LL_TIM_GenerateEvent_UPDATE(timer);
		}

		clear_capture_interrupt[channel - 1](timer);

		if (cpt->state != CAPTURE_STATE_IDLE) {
			/* Still waiting for a complete capture */
			return;
		}
	}

	if (cpt->overflows) {
		status = -ERANGE;
	}

	if (!cpt->continuous) {
		pwm_stm32_disable_capture(dev, channel);
	} else {
		cpt->overflows = 0u;
		cpt->state = CAPTURE_STATE_WAIT_FOR_PERIOD_END;
	}

	if (cpt->callback != NULL) {
		cpt->callback(dev, channel, cpt->capture_period ? cpt->period : 0u,
				cpt->capture_pulse ? cpt->pulse : 0u, status, cpt->user_data);
	}
}

#endif /* CONFIG_PWM_CAPTURE */

static int pwm_stm32_get_cycles_per_sec(const struct device *dev,
					uint32_t channel, uint64_t *cycles)
{
	struct pwm_stm32_data *data = dev->data;
	const struct pwm_stm32_config *cfg = dev->config;

	*cycles = (uint64_t)(data->tim_clk / (cfg->prescaler + 1));

	return 0;
}

static DEVICE_API(pwm, pwm_stm32_driver_api) = {
	.set_cycles = pwm_stm32_set_cycles,
	.get_cycles_per_sec = pwm_stm32_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_stm32_configure_capture,
	.enable_capture = pwm_stm32_enable_capture,
	.disable_capture = pwm_stm32_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

static int pwm_stm32_init(const struct device *dev)
{
	struct pwm_stm32_data *data = dev->data;
	const struct pwm_stm32_config *cfg = dev->config;
	TIM_TypeDef *timer = cfg->timer;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	uint32_t tim_clk;
	int r;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable clock and store its speed */
	r = clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}

	if (cfg->pclk_len > 1) {
		/* Enable Timer clock source */
		r = clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL);
		if (r != 0) {
			LOG_ERR("Could not configure clock (%d)", r);
			return r;
		}

		r = clock_control_get_rate(clk, (clock_control_subsys_t)&cfg->pclken[1], &tim_clk);
		if (r < 0) {
			LOG_ERR("Timer clock rate get error (%d)", r);
			return r;
		}
	} else {
		LOG_ERR("Timer clock source is not specified");
		return -EINVAL;
	}

	data->tim_clk = tim_clk;

	/* Reset timer to default state using RCC */
	(void)reset_line_toggle_dt(&data->reset);

	/* configure pinmux */
	r = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (r < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", r);
		return r;
	}

	/* initialize timer */
	LL_TIM_SetPrescaler(timer, cfg->prescaler);
	LL_TIM_SetAutoReload(timer, 0U);

	if (IS_TIM_COUNTER_MODE_SELECT_INSTANCE(timer)) {
		LL_TIM_SetCounterMode(timer, cfg->countermode);
	}

	if (IS_TIM_CLOCK_DIVISION_INSTANCE(timer)) {
		LL_TIM_SetClockDivision(timer, LL_TIM_CLOCKDIVISION_DIV1);
	}

#ifdef IS_TIM_REPETITION_COUNTER_INSTANCE
	if (IS_TIM_REPETITION_COUNTER_INSTANCE(timer)) {
		LL_TIM_SetRepetitionCounter(timer, 0U);
	}
#endif

#if !defined(CONFIG_SOC_SERIES_STM32L0X) && !defined(CONFIG_SOC_SERIES_STM32L1X)
	/* enable outputs and counter */
	if (IS_TIM_BREAK_INSTANCE(timer)) {
		LL_TIM_EnableAllOutputs(timer);
	}
#endif

	LL_TIM_EnableCounter(timer);

#ifdef CONFIG_PWM_CAPTURE
	cfg->irq_config_func(dev);
#endif /* CONFIG_PWM_CAPTURE */

	return 0;
}

#define PWM(index) DT_INST_PARENT(index)

#ifdef CONFIG_PWM_CAPTURE
#define IRQ_CONNECT_AND_ENABLE_BY_NAME(index, name)				\
{										\
	IRQ_CONNECT(DT_IRQ_BY_NAME(PWM(index), name, irq),			\
			DT_IRQ_BY_NAME(PWM(index), name, priority),		\
			pwm_stm32_isr, DEVICE_DT_INST_GET(index), 0);		\
	irq_enable(DT_IRQ_BY_NAME(PWM(index), name, irq));			\
}

#define IRQ_CONNECT_AND_ENABLE_DEFAULT(index)					\
{										\
	IRQ_CONNECT(DT_IRQN(PWM(index)),					\
			DT_IRQ(PWM(index), priority),				\
			pwm_stm32_isr, DEVICE_DT_INST_GET(index), 0);		\
	irq_enable(DT_IRQN(PWM(index)));					\
}

#define IRQ_CONFIG_FUNC(index)                                                  \
static void pwm_stm32_irq_config_func_##index(const struct device *dev)		\
{										\
	COND_CODE_1(DT_IRQ_HAS_NAME(PWM(index), cc),				\
		(IRQ_CONNECT_AND_ENABLE_BY_NAME(index, cc)),			\
		(IRQ_CONNECT_AND_ENABLE_DEFAULT(index))				\
	);									\
}
#define CAPTURE_INIT(index)                                                                        \
	.irq_config_func = pwm_stm32_irq_config_func_##index,                                      \
	.four_channel_capture_support = DT_INST_PROP(index, four_channel_capture_support)
#else
#define IRQ_CONFIG_FUNC(index)
#define CAPTURE_INIT(index)
#endif /* CONFIG_PWM_CAPTURE */

#define PWM_DEVICE_INIT(index)                                                 \
	static struct pwm_stm32_data pwm_stm32_data_##index = {		       \
		.reset = RESET_DT_SPEC_GET(PWM(index)),			       \
	};								       \
									       \
	IRQ_CONFIG_FUNC(index)						       \
									       \
	PINCTRL_DT_INST_DEFINE(index);					       \
									       \
	static const struct stm32_pclken pclken_##index[] =		       \
					STM32_DT_CLOCKS(PWM(index));	      \
									       \
	static const struct pwm_stm32_config pwm_stm32_config_##index = {      \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(PWM(index)),	       \
		.prescaler = DT_PROP(PWM(index), st_prescaler),		       \
		.countermode = DT_PROP(PWM(index), st_countermode),	       \
		.pclken = pclken_##index,				       \
		.pclk_len = DT_NUM_CLOCKS(PWM(index)),			       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(index),		       \
		CAPTURE_INIT(index)					       \
	};                                                                     \
									       \
	DEVICE_DT_INST_DEFINE(index, &pwm_stm32_init, NULL,                    \
			    &pwm_stm32_data_##index,                           \
			    &pwm_stm32_config_##index, POST_KERNEL,            \
			    CONFIG_PWM_INIT_PRIORITY,                          \
			    &pwm_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT)
