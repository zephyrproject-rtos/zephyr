/*
 * Copyright 2017, 2024, 2026 NXP
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_ftm_pwm

#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_ftm.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux_ftm, CONFIG_PWM_LOG_LEVEL);

#define MAX_CHANNELS ARRAY_SIZE(FTM0->CONTROLS)

/* PWM capture operates on channel pairs */
#define MAX_CAPTURE_PAIRS (MAX_CHANNELS / 2U)
#define PAIR_1ST_CH(pair) (pair * 2U)
#define PAIR_2ND_CH(pair) (PAIR_1ST_CH(pair) + 1)

struct mcux_ftm_config {
	FTM_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	ftm_clock_source_t ftm_clock_source;
	ftm_clock_prescale_t prescale;
	uint8_t channel_count;
	ftm_pwm_mode_t mode;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
	const struct pinctrl_dev_config *pincfg;
};

#ifdef CONFIG_PM_DEVICE
/* Channel configuration saved across a low power transition. */
struct mcux_ftm_pm_channel {
	uint32_t pulse_cycles;
	pwm_flags_t flags;
	bool configured;
};
#endif /* CONFIG_PM_DEVICE */

struct mcux_ftm_capture_data {
	ftm_dual_edge_capture_param_t param;
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t first_edge_overflows;
	uint16_t first_edge_cnt;
	bool first_edge_overflow;
	bool pulse_capture;
};

struct mcux_ftm_data {
	uint32_t clock_freq;
	uint32_t period_cycles;
	ftm_chnl_pwm_config_param_t channel[MAX_CHANNELS];
	/* Whether the policy lock is currently held. */
	bool pm_lock_held;
	/* Guards pm_lock_held: a one-shot capture ends in the ISR. */
	struct k_spinlock lock;
#ifdef CONFIG_PM_DEVICE
	struct mcux_ftm_pm_channel pm_channel[MAX_CHANNELS];
	uint32_t pm_period_cycles;
#endif /* CONFIG_PM_DEVICE */
#ifdef CONFIG_PWM_CAPTURE
	uint32_t overflows;
	struct mcux_ftm_capture_data capture[MAX_CAPTURE_PAIRS];
#endif /* CONFIG_PWM_CAPTURE */
};

static bool mcux_ftm_pwm_active(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	for (uint8_t channel = 0U; channel < config->channel_count; channel++) {
		if (data->channel[channel].level != kFTM_NoPwmSignal) {
			return true;
		}
	}

	return false;
}

#ifdef CONFIG_PWM_CAPTURE
/* Report whether any channel pair still has an active capture. */
static bool mcux_ftm_capture_pending(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	uint32_t irqs = FTM_GetEnabledInterrupts(config->base);

	for (uint32_t pair = 0U; pair < MAX_CAPTURE_PAIRS; pair++) {
		if (irqs & BIT(PAIR_2ND_CH(pair))) {
			return true;
		}
	}

	return false;
}
#endif /* CONFIG_PWM_CAPTURE */

/*
 * Hold the policy lock for as long as a channel drives an output or a pair is
 * capturing: a waveform has no completion event that could bound a shorter
 * window, and a capture dies with the counter.
 *
 * Idempotent, so any path that changes what the instance is doing can just
 * call it.
 */
static void mcux_ftm_pm_lock_sync(const struct device *dev)
{
	struct mcux_ftm_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	bool active = mcux_ftm_pwm_active(dev);

#ifdef CONFIG_PWM_CAPTURE
	active = active || mcux_ftm_capture_pending(dev);
#endif /* CONFIG_PWM_CAPTURE */

	if (active != data->pm_lock_held) {
		if (active) {
			pm_policy_device_power_lock_get(dev);
		} else {
			pm_policy_device_power_lock_put(dev);
		}

		data->pm_lock_held = active;
	}

	k_spin_unlock(&data->lock, key);
}

static int mcux_ftm_release_channel(const struct device *dev, uint32_t channel)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	status_t status;

	data->channel[channel].level = kFTM_NoPwmSignal;
	data->channel[channel].dutyValue = 0U;

	status = FTM_SetupPwmMode(config->base, data->channel, config->channel_count,
				  config->mode);
	if (status != kStatus_Success) {
		LOG_ERR("Could not release channel %u", channel);
		return -EIO;
	}

	FTM_SetSoftwareTrigger(config->base, true);

#ifdef CONFIG_PM_DEVICE
	/* Nothing left here for a resume to replay. */
	data->pm_channel[channel].configured = false;
#endif /* CONFIG_PM_DEVICE */

	mcux_ftm_pm_lock_sync(dev);

	return 0;
}

static int mcux_ftm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	status_t status;
#ifdef CONFIG_PWM_CAPTURE
	uint32_t pair = channel / 2U;
	uint32_t irqs;
#endif /* CONFIG_PWM_CAPTURE */

	if ((period_cycles == 0U) && (pulse_cycles != 0U)) {
		LOG_ERR("A pulse of %u does not fit a zero period", pulse_cycles);
		return -EINVAL;
	}

	if (period_cycles > UINT16_MAX) {
		LOG_ERR("Period cycles must be less or equal than %u", UINT16_MAX);
		return -EINVAL;
	}

	if (channel >= config->channel_count) {
		LOG_ERR("Invalid channel");
		return -ENOTSUP;
	}

#ifdef CONFIG_PWM_CAPTURE
	irqs = FTM_GetEnabledInterrupts(config->base);
	if (irqs & BIT(PAIR_2ND_CH(pair))) {
		LOG_ERR("Cannot set PWM, capture in progress on pair %d", pair);
		return -EBUSY;
	}
#endif /* CONFIG_PWM_CAPTURE */

	if (period_cycles == 0U) {
		return mcux_ftm_release_channel(dev, channel);
	}

	data->channel[channel].dutyValue = pulse_cycles;

	if ((flags & PWM_POLARITY_INVERTED) == 0) {
		data->channel[channel].level = kFTM_HighTrue;
	} else {
		data->channel[channel].level = kFTM_LowTrue;
	}

	LOG_DBG("pulse_cycles=%d, period_cycles=%d, flags=%d",
		pulse_cycles, period_cycles, flags);

	if (period_cycles != data->period_cycles) {
#ifdef CONFIG_PWM_CAPTURE
		if (irqs & BIT_MASK(ARRAY_SIZE(data->channel))) {
			LOG_ERR("Cannot change period, capture in progress");
			return -EBUSY;
		}
#endif /* CONFIG_PWM_CAPTURE */

		if (data->period_cycles != 0) {
			/* Only warn when not changing from zero */
			LOG_WRN("Changing period cycles from %d to %d"
				" affects all %d channels in %s",
				data->period_cycles, period_cycles,
				config->channel_count, dev->name);
		}

		data->period_cycles = period_cycles;

		FTM_StopTimer(config->base);
		/* CNTIN is zero, so a period lasts MOD + 1 ticks. MOD has to be one
		 * short of the tick count, or the period runs a tick long and 100%
		 * duty is unreachable: the output stays active for a whole period
		 * only once CnV > MOD.
		 */
		FTM_SetTimerPeriod(config->base, period_cycles - 1U);

		FTM_SetSoftwareTrigger(config->base, true);
		FTM_StartTimer(config->base, config->ftm_clock_source);
	}

	status = FTM_SetupPwmMode(config->base, data->channel,
				  config->channel_count, config->mode);
	if (status != kStatus_Success) {
		LOG_ERR("Could not set up pwm");
		return -ENOTSUP;
	}
	FTM_SetSoftwareTrigger(config->base, true);

	mcux_ftm_pm_lock_sync(dev);

#ifdef CONFIG_PM_DEVICE
	/* Remember the accepted setup so that the resume can replay it. */
	data->pm_channel[channel].pulse_cycles = pulse_cycles;
	data->pm_channel[channel].flags = flags;
	data->pm_channel[channel].configured = true;
	data->pm_period_cycles = period_cycles;
#endif /* CONFIG_PM_DEVICE */

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int mcux_ftm_configure_capture(const struct device *dev,
				      uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb,
				      void *user_data)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	ftm_dual_edge_capture_param_t *param;
	uint32_t pair = channel / 2U;

	if (channel & 0x1U) {
		LOG_ERR("PWM capture only supported on even channels");
		return -ENOTSUP;
	}

	if (pair >= ARRAY_SIZE(data->capture)) {
		LOG_ERR("Invalid channel pair %d", pair);
		return -EINVAL;
	}

	if (FTM_GetEnabledInterrupts(config->base) & BIT(PAIR_2ND_CH(pair))) {
		LOG_ERR("Capture already active on channel pair %d", pair);
		return -EBUSY;
	}

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No capture type specified");
		return -EINVAL;
	}

	if ((flags & PWM_CAPTURE_TYPE_MASK) == PWM_CAPTURE_TYPE_BOTH) {
		LOG_ERR("Cannot capture both period and pulse width");
		return -ENOTSUP;
	}

	data->capture[pair].callback = cb;
	data->capture[pair].user_data = user_data;
	param = &data->capture[pair].param;

	if ((flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS) {
		param->mode = kFTM_Continuous;
	} else {
		param->mode = kFTM_OneShot;
	}

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture[pair].pulse_capture = false;

		if (flags & PWM_POLARITY_INVERTED) {
			param->currChanEdgeMode = kFTM_FallingEdge;
			param->nextChanEdgeMode = kFTM_FallingEdge;
		} else {
			param->currChanEdgeMode = kFTM_RisingEdge;
			param->nextChanEdgeMode = kFTM_RisingEdge;
		}
	} else {
		data->capture[pair].pulse_capture = true;

		if (flags & PWM_POLARITY_INVERTED) {
			param->currChanEdgeMode = kFTM_FallingEdge;
			param->nextChanEdgeMode = kFTM_RisingEdge;
		} else {
			param->currChanEdgeMode = kFTM_RisingEdge;
			param->nextChanEdgeMode = kFTM_FallingEdge;
		}
	}

	return 0;
}

static int mcux_ftm_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	uint32_t pair = channel / 2U;

	if (channel & 0x1U) {
		LOG_ERR("PWM capture only supported on even channels");
		return -ENOTSUP;
	}

	if (pair >= ARRAY_SIZE(data->capture)) {
		LOG_ERR("Invalid channel pair %d", pair);
		return -EINVAL;
	}

	if (!data->capture[pair].callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	if (FTM_GetEnabledInterrupts(config->base) & BIT(PAIR_2ND_CH(pair))) {
		LOG_ERR("Capture already active on channel pair %d", pair);
		return -EBUSY;
	}

	FTM_ClearStatusFlags(config->base, BIT(PAIR_1ST_CH(pair)) |
			     BIT(PAIR_2ND_CH(pair)));

	FTM_SetupDualEdgeCapture(config->base, pair, &data->capture[pair].param,
				 CONFIG_PWM_CAPTURE_MCUX_FTM_FILTER_VALUE);

	FTM_EnableInterrupts(config->base, BIT(PAIR_1ST_CH(pair)) |
			     BIT(PAIR_2ND_CH(pair)));

	/* Keep the instance out of low power while a capture is running: the
	 * counter must not be halted mid-measurement. The busy flag only blocks
	 * system PM when CONFIG_PM_NEED_ALL_DEVICES_IDLE=y; the policy lock
	 * covers the rest.
	 */
	pm_device_busy_set(dev);
	mcux_ftm_pm_lock_sync(dev);

	return 0;
}

static int mcux_ftm_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	uint32_t pair = channel / 2U;

	if (channel & 0x1U) {
		LOG_ERR("PWM capture only supported on even channels");
		return -ENOTSUP;
	}

	if (pair >= ARRAY_SIZE(data->capture)) {
		LOG_ERR("Invalid channel pair %d", pair);
		return -EINVAL;
	}

	FTM_DisableInterrupts(config->base, BIT(PAIR_1ST_CH(pair)) |
			      BIT(PAIR_2ND_CH(pair)));

	/* Clear Dual Edge Capture Enable bit */
	config->base->COMBINE &= ~(1UL << (FTM_COMBINE_DECAP0_SHIFT +
		(FTM_COMBINE_COMBINE1_SHIFT * pair)));

	/* Release the low power block once no pair is capturing any more. */
	if (!mcux_ftm_capture_pending(dev)) {
		pm_device_busy_clear(dev);
	}

	mcux_ftm_pm_lock_sync(dev);

	return 0;
}

static void mcux_ftm_capture_first_edge(const struct device *dev, uint32_t channel,
					uint16_t cnt, bool overflow)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	struct mcux_ftm_capture_data *capture;
	uint32_t pair = channel / 2U;

	__ASSERT_NO_MSG(pair < ARRAY_SIZE(data->capture));
	capture = &data->capture[pair];

	FTM_DisableInterrupts(config->base, BIT(PAIR_1ST_CH(pair)));

	capture->first_edge_cnt = cnt;
	capture->first_edge_overflows = data->overflows;
	capture->first_edge_overflow = overflow;

	LOG_DBG("pair = %d, 1st cnt = %u, 1st ovf = %d", pair, cnt, overflow);
}

static void mcux_ftm_capture_second_edge(const struct device *dev, uint32_t channel,
					 uint16_t cnt, bool overflow)

{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	uint32_t second_edge_overflows = data->overflows;
	struct mcux_ftm_capture_data *capture;
	uint32_t pair = channel / 2U;
	uint32_t overflows;
	uint32_t first_cnv;
	uint32_t second_cnv;
	uint32_t cycles = 0;
	int status = 0;

	__ASSERT_NO_MSG(pair < ARRAY_SIZE(data->capture));
	capture = &data->capture[pair];

	first_cnv = config->base->CONTROLS[PAIR_1ST_CH(pair)].CnV;
	second_cnv = config->base->CONTROLS[PAIR_2ND_CH(pair)].CnV;

	if (capture->pulse_capture) {
		/* Clear both edge flags for pulse capture to capture first edge overflow counter */
		FTM_ClearStatusFlags(config->base, BIT(PAIR_1ST_CH(pair)) | BIT(PAIR_2ND_CH(pair)));
	} else {
		/* Only clear second edge flag for period capture as next first edge is this edge */
		FTM_ClearStatusFlags(config->base, BIT(PAIR_2ND_CH(pair)));
	}

	if (unlikely(capture->first_edge_overflow && first_cnv > capture->first_edge_cnt)) {
		/* Compensate for the overflow registered in the same IRQ */
		capture->first_edge_overflows--;
	}

	if (unlikely(overflow && second_cnv > cnt)) {
		/* Compensate for the overflow registered in the same IRQ */
		second_edge_overflows--;
	}

	overflows = second_edge_overflows - capture->first_edge_overflows;

	/* Calculate cycles, check for overflows. CNTIN is zero, so each
	 * rollover spans MOD + 1 ticks.
	 */
	if (overflows > 0) {
		if (u32_mul_overflow(overflows, config->base->MOD + 1U, &cycles)) {
			LOG_ERR("overflow while calculating cycles");
			status = -ERANGE;
		} else {
			cycles -= first_cnv;
			if (u32_add_overflow(cycles, second_cnv, &cycles)) {
				LOG_ERR("overflow while calculating cycles");
				cycles = 0;
				status = -ERANGE;
			}
		}
	} else {
		cycles = second_cnv - first_cnv;
	}

	LOG_DBG("pair = %d, 1st ovfs = %u, 2nd ovfs = %u, ovfs = %u, 1st cnv = %u, "
		"2nd cnv = %u, cycles = %u, 2nd cnt = %u, 2nd ovf = %d",
		pair, capture->first_edge_overflows, second_edge_overflows, overflows, first_cnv,
		second_cnv, cycles, cnt, overflow);

	if (capture->pulse_capture) {
		capture->callback(dev, pair, 0, cycles, status,
				  capture->user_data);
	} else {
		capture->callback(dev, pair, cycles, 0, status,
				  capture->user_data);
	}

	if (capture->param.mode == kFTM_OneShot) {
		/* One-shot capture done */
		FTM_DisableInterrupts(config->base, BIT(PAIR_2ND_CH(pair)));

		/* The pair stopped capturing on its own; drop the low power
		 * block if it was the last one still active.
		 */
		if (!mcux_ftm_capture_pending(dev)) {
			pm_device_busy_clear(dev);
		}

		mcux_ftm_pm_lock_sync(dev);
	} else if (capture->pulse_capture) {
		/* Prepare for first edge of next pulse capture */
		FTM_EnableInterrupts(config->base, BIT(PAIR_1ST_CH(pair)));
	} else {
		/* First edge of next period capture is second edge of this capture (this edge) */
		capture->first_edge_cnt = cnt;
		capture->first_edge_overflows = second_edge_overflows;
		capture->first_edge_overflow = false;
	}
}

static bool mcux_ftm_handle_overflow(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	if (FTM_GetStatusFlags(config->base) & kFTM_TimeOverflowFlag) {
		data->overflows++;
		FTM_ClearStatusFlags(config->base, kFTM_TimeOverflowFlag);
		return true;
	}

	return false;
}

static void mcux_ftm_irq_handler(const struct device *dev, uint32_t chan_start, uint32_t chan_end)
{
	const struct mcux_ftm_config *config = dev->config;
	bool overflow;
	uint32_t flags;
	uint32_t irqs;
	uint16_t cnt;
	uint32_t ch;

	flags = FTM_GetStatusFlags(config->base);
	irqs = FTM_GetEnabledInterrupts(config->base);
	cnt = config->base->CNT;

	overflow = mcux_ftm_handle_overflow(dev);

	for (ch = chan_start; ch < chan_end; ch++) {
		if ((flags & BIT(ch)) && (irqs & BIT(ch))) {
			if (ch & 1) {
				mcux_ftm_capture_second_edge(dev, ch, cnt, overflow);
			} else {
				mcux_ftm_capture_first_edge(dev, ch, cnt, overflow);
			}
		}
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static int mcux_ftm_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

/* Bring the hardware up from an unknown state: ungate the peripheral clock and
 * program what the devicetree describes.
 */
static int mcux_ftm_hw_init(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	ftm_chnl_pwm_config_param_t *channel = data->channel;
	ftm_config_t ftm_config;
	int i;

	/* The block comes up with no PWM on any channel, so data->channel[], which
	 * is what FTM_SetupPwmMode() consumes, has to say the same.
	 */
	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
		channel->level = kFTM_NoPwmSignal;
		channel->dutyValue = 0;
		channel->firstEdgeValue = 0;
		channel++;
	}

	FTM_GetDefaultConfig(&ftm_config);
	ftm_config.prescale = config->prescale;

	if (FTM_Init(config->base, &ftm_config) != kStatus_Success) {
		LOG_ERR("Could not initialise the FTM");
		return -EIO;
	}

#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);
	FTM_EnableInterrupts(config->base,
			     kFTM_TimeOverflowInterruptEnable);

	/* Capture is not restored across a suspend; a consumer has to call
	 * pwm_configure_capture() again. Only the bookkeeping is cleared here --
	 * the channel interrupt enables and COMBINE[DECAP] went down in
	 * mcux_ftm_suspend().
	 */
	data->overflows = 0U;
	for (uint32_t pair = 0U; pair < ARRAY_SIZE(data->capture); pair++) {
		memset(&data->capture[pair], 0, sizeof(data->capture[pair]));
	}

	/* Capture measurements need the overflow count, so a capture build keeps
	 * the counter running whether or not a capture is armed.
	 */
	data->period_cycles = 0xFFFFU;
	FTM_SetTimerPeriod(config->base, data->period_cycles - 1U);
	FTM_SetSoftwareTrigger(config->base, true);
	FTM_StartTimer(config->base, config->ftm_clock_source);
#else
	data->period_cycles = 0U;
#endif /* CONFIG_PWM_CAPTURE */

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int mcux_ftm_restore_channels(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	uint8_t channel;
	int ret = 0;

	/* Force the period to be reprogrammed by the first restored channel: that
	 * is what restarts the counter, and it keeps the "changing period cycles"
	 * warning out of the resume path.
	 */
	data->period_cycles = 0U;

	for (channel = 0U; channel < config->channel_count; channel++) {
		int err;

		if (!data->pm_channel[channel].configured) {
			continue;
		}

		err = mcux_ftm_set_cycles(dev, channel, data->pm_period_cycles,
					  data->pm_channel[channel].pulse_cycles,
					  data->pm_channel[channel].flags);
		if (err != 0) {
			LOG_ERR("Could not restore channel %u: %d", channel, err);
			if (ret == 0) {
				ret = err;
			}
		}
	}

	return ret;
}

static int mcux_ftm_suspend(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_SLEEP);
	if (err < 0 && err != -ENOENT) {
		return err;
	}

#ifdef CONFIG_PWM_CAPTURE
	/*
	 * Before the clock goes: neither FTM_Deinit() nor the FTM_Init() on the way
	 * back clears CnSC[CHIE] or COMBINE[DECAP], so a capture left armed would
	 * come back armed in hardware. disable_capture() also drops the busy flag.
	 */
	for (uint32_t pair = 0U; pair < config->channel_count / 2U; pair++) {
		(void)mcux_ftm_disable_capture(dev, PAIR_1ST_CH(pair));
	}
#endif /* CONFIG_PWM_CAPTURE */

	/*
	 * FTM_Deinit() clears SC[CLKS], stopping the waveform, and gates the
	 * peripheral clock.
	 */
	FTM_Deinit(config->base);

	/* Released here rather than by mcux_ftm_pm_lock_sync(): that reads
	 * data->channel[], which still holds the setup the resume replays, so it
	 * would not release the lock.
	 */
	if (data->pm_lock_held) {
		pm_policy_device_power_lock_put(dev);
		data->pm_lock_held = false;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static int mcux_ftm_resume(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	int err;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	err = mcux_ftm_hw_init(dev);
	if (err != 0) {
		return err;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && err != -ENOENT) {
		goto power_down;
	}

#ifdef CONFIG_PM_DEVICE
	err = mcux_ftm_restore_channels(dev);
	if (err != 0) {
		goto power_down;
	}
#endif /* CONFIG_PM_DEVICE */

	mcux_ftm_pm_lock_sync(dev);

	return 0;

power_down:
	FTM_Deinit(config->base);

	if (data->pm_lock_held) {
		pm_policy_device_power_lock_put(dev);
		data->pm_lock_held = false;
	}

	return err;
}

static int mcux_ftm_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return mcux_ftm_resume(dev);
#ifdef CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		return mcux_ftm_suspend(dev);
	case PM_DEVICE_ACTION_TURN_OFF:
		/*
		 * Nothing: the suspend that must precede this halted the counter
		 * and gated the clock, and the resume replays the saved setup
		 * whether or not the peripheral lost power.
		 */
		return 0;
#endif /* CONFIG_PM_DEVICE */
	case PM_DEVICE_ACTION_TURN_ON:
		/*
		 * Nothing. The peripheral is brought up from RESUME, which a TURN_ON is
		 * always followed by, so an instance left SUSPENDED by runtime
		 * device PM boots with its clock gated.
		 */
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mcux_ftm_init(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	if (config->channel_count > ARRAY_SIZE(data->channel)) {
		LOG_ERR("Invalid channel count");
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	return pm_device_driver_init(dev, mcux_ftm_pm_action);
}

static DEVICE_API(pwm, mcux_ftm_driver_api) = {
	.set_cycles = mcux_ftm_set_cycles,
	.get_cycles_per_sec = mcux_ftm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_ftm_configure_capture,
	.enable_capture = mcux_ftm_enable_capture,
	.disable_capture = mcux_ftm_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

#define TO_FTM_PRESCALE_DIVIDE(val) _DO_CONCAT(kFTM_Prescale_Divide_, val)

#ifdef CONFIG_PWM_CAPTURE
#if IS_EQ(DT_NUM_IRQS(DT_DRV_INST(0)), 1)
static void mcux_ftm_isr(const struct device *dev)
{
	const struct mcux_ftm_config *cfg = dev->config;

	mcux_ftm_irq_handler(dev, 0, cfg->channel_count);
}

#define FTM_CONFIG_FUNC(n) \
static void mcux_ftm_config_func_##n(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
		    mcux_ftm_isr, DEVICE_DT_INST_GET(n), 0); \
	irq_enable(DT_INST_IRQN(n)); \
}
#else /* Multiple interrupts */
#define FTM_ISR_FUNC_NAME(suffix) _DO_CONCAT(mcux_ftm_isr_, suffix)
#define FTM_ISR_FUNC(chan_start, chan_end) \
static void mcux_ftm_isr_##chan_start##_##chan_end(const struct device *dev) \
{ \
	mcux_ftm_irq_handler(dev, chan_start, chan_end + 1); \
}

#define FTM_ISR_CONFIG(node_id, prop, idx) \
do { \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq), \
		    DT_IRQ_BY_IDX(node_id, idx, priority), \
		    FTM_ISR_FUNC_NAME(DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)), \
		    DEVICE_DT_GET(node_id), \
		    0); \
	irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq)); \
} while (false);

#define FTM_CONFIG_FUNC(n) \
static void mcux_ftm_config_func_##n(const struct device *dev) \
{ \
	DT_INST_FOREACH_PROP_ELEM(n, interrupt_names, FTM_ISR_CONFIG) \
}

#if DT_INST_IRQ_HAS_NAME(0, overflow)
static void mcux_ftm_isr_overflow(const struct device *dev)
{
	mcux_ftm_handle_overflow(dev);
}
#endif
#if DT_INST_IRQ_HAS_NAME(0, 0_1)
FTM_ISR_FUNC(0, 1)
#endif
#if DT_INST_IRQ_HAS_NAME(0, 2_3)
FTM_ISR_FUNC(2, 3)
#endif
#if DT_INST_IRQ_HAS_NAME(0, 4_5)
FTM_ISR_FUNC(4, 5)
#endif
#if DT_INST_IRQ_HAS_NAME(0, 6_7)
FTM_ISR_FUNC(6, 7)
#endif
#endif /* IS_EQ(DT_NUM_IRQS(DT_DRV_INST(0)), 1) */
#define FTM_CFG_CAPTURE_INIT(n) \
	.irq_config_func = mcux_ftm_config_func_##n
#define FTM_INIT_CFG(n)	FTM_DECLARE_CFG(n, FTM_CFG_CAPTURE_INIT(n))
#else /* !CONFIG_PWM_CAPTURE */
#define FTM_CONFIG_FUNC(n)
#define FTM_CFG_CAPTURE_INIT
#define FTM_INIT_CFG(n)	FTM_DECLARE_CFG(n, FTM_CFG_CAPTURE_INIT)
#endif /* !CONFIG_PWM_CAPTURE */

#define FTM_DECLARE_CFG(n, CAPTURE_INIT) \
static const struct mcux_ftm_config mcux_ftm_config_##n = { \
	.base = (FTM_Type *)DT_INST_REG_ADDR(n),\
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
	.clock_subsys = (clock_control_subsys_t) \
		DT_INST_CLOCKS_CELL(n, name), \
	.ftm_clock_source = (ftm_clock_source_t)(DT_INST_ENUM_IDX(n, clock_source) + 1U), \
	.prescale = TO_FTM_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)),\
	.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn((FTM_Type *) \
		DT_INST_REG_ADDR(n)), \
	.mode = kFTM_EdgeAlignedPwm, \
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
	CAPTURE_INIT \
}

#define FTM_DEVICE(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static struct mcux_ftm_data mcux_ftm_data_##n; \
	static const struct mcux_ftm_config mcux_ftm_config_##n; \
	PM_DEVICE_DT_INST_DEFINE(n, mcux_ftm_pm_action); \
	DEVICE_DT_INST_DEFINE(n, &mcux_ftm_init,		       \
			    PM_DEVICE_DT_INST_GET(n), &mcux_ftm_data_##n, \
			    &mcux_ftm_config_##n, \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			    &mcux_ftm_driver_api); \
	FTM_CONFIG_FUNC(n) \
	FTM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(FTM_DEVICE)
