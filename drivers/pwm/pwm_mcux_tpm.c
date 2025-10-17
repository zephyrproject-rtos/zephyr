/*
 * Copyright 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright 2020, 2024-2025 NXP
 *
 * Heavily based on pwm_mcux_ftm.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_tpm

#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_tpm.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>

#if defined(CONFIG_SOC_MIMX9596)
#include <zephyr/dt-bindings/clock/imx95_clock.h>
#endif

LOG_MODULE_REGISTER(pwm_mcux_tpm, CONFIG_PWM_LOG_LEVEL);

#ifdef CONFIG_PWM_CAPTURE
#if !defined(FSL_FEATURE_TPM_HAS_COMBINE) || (FSL_FEATURE_TPM_HAS_COMBINE == 0)
#error "TPM combine mode not available, PWM capture feature is unsupported."
#endif
#endif

#define TPM_MAX_CHANNELS TPM_CnSC_COUNT
#define TPM_COMBINE_SHIFT (8U)

/* PWM capture operates on channel pairs */
#define TPM_MAX_CAPTURE_PAIRS (TPM_MAX_CHANNELS / 2U)
#define TPM_PAIR_FIRST_CH(pair) (pair * 2U)
#define TPM_PAIR_SECOND_CH(pair) (TPM_PAIR_FIRST_CH(pair) + 1)
#define TPM_WHICH_PAIR(ch) (ch / 2U)

#define DEV_CFG(_dev) ((const struct mcux_tpm_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_tpm_data *)(_dev)->data)
#define TPM_TYPE_BASE(dev, name) ((TPM_Type *)DEVICE_MMIO_NAMED_GET(dev, name))

struct mcux_tpm_config {
	DEVICE_MMIO_NAMED_ROM(base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	tpm_clock_source_t tpm_clock_source;
	tpm_clock_prescale_t prescale;
	uint8_t channel_count;
	tpm_pwm_mode_t mode;
	const struct pinctrl_dev_config *pincfg;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
};

#ifdef CONFIG_PWM_CAPTURE
struct mcux_tpm_capture_data {
	tpm_dual_edge_capture_param_t param;
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t first_edge_overflows;
	uint32_t first_edge_cnt; /* Counter value after entering first edge interrupt */
	uint32_t first_edge_cnv; /* CnV value When first edge is captured */
	bool first_edge_overflow;
	bool first_chan_captured;
	bool pulse_capture;
	bool continuous_capture;
};
#endif /* CONFIG_PWM_CAPTURE */

struct mcux_tpm_data {
	DEVICE_MMIO_NAMED_RAM(base);
	uint32_t clock_freq;
	uint32_t period_cycles;
	tpm_chnl_pwm_signal_param_t channel[TPM_MAX_CHANNELS];
#ifdef CONFIG_PWM_CAPTURE
	uint32_t overflows;
	struct mcux_tpm_capture_data capture[TPM_MAX_CAPTURE_PAIRS];
#endif /* CONFIG_PWM_CAPTURE */
};

static int mcux_tpm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
#ifdef CONFIG_PWM_CAPTURE
	uint32_t pair = TPM_WHICH_PAIR(channel);
	uint32_t irqs;
#endif /* CONFIG_PWM_CAPTURE */

	if (channel >= config->channel_count) {
		LOG_ERR("Invalid channel");
		return -ENOTSUP;
	}

	if (period_cycles == 0 || period_cycles == TPM_MAX_COUNTER_VALUE(base)) {
		return -ENOTSUP;
	}

	if ((TPM_MAX_COUNTER_VALUE(base) == 0xFFFFU) &&
	    (pulse_cycles > TPM_MAX_COUNTER_VALUE(base))) {
		return -ENOTSUP;
	}

#ifdef CONFIG_PWM_CAPTURE
	irqs = TPM_GetEnabledInterrupts(base);
	if (irqs & BIT(TPM_PAIR_SECOND_CH(pair))) {
		LOG_ERR("Cannot set PWM, capture in progress on pair %d", pair);
		return -EBUSY;
	}
#endif /* CONFIG_PWM_CAPTURE */

	LOG_DBG("pulse_cycles=%d, period_cycles=%d, flags=%d", pulse_cycles, period_cycles, flags);

	if (period_cycles != data->period_cycles) {
		uint32_t pwm_freq;
		status_t status;

		if (data->period_cycles != 0) {
			/* Only warn when not changing from zero */
			LOG_WRN("Changing period cycles from %d to %d"
				" affects all %d channels in %s",
				data->period_cycles, period_cycles,
				config->channel_count, dev->name);
		}

		data->period_cycles = period_cycles;

		pwm_freq = (data->clock_freq >> config->prescale) /
			   period_cycles;

		LOG_DBG("pwm_freq=%d, clock_freq=%d", pwm_freq,
			data->clock_freq);

		if (pwm_freq == 0U) {
			LOG_ERR("Could not set up pwm_freq=%d", pwm_freq);
			return -EINVAL;
		}

		TPM_StopTimer(base);

		/* Set counter back to zero */
		base->CNT = 0;

		status = TPM_SetupPwm(base, data->channel,
				      config->channel_count, config->mode,
				      pwm_freq, data->clock_freq);

		if (status != kStatus_Success) {
			LOG_ERR("Could not set up pwm");
			return -ENOTSUP;
		}
		TPM_StartTimer(base, config->tpm_clock_source);
	}

	if ((flags & PWM_POLARITY_INVERTED) == 0 &&
		   data->channel[channel].level != kTPM_HighTrue) {
		data->channel[channel].level = kTPM_HighTrue;
		TPM_UpdateChnlEdgeLevelSelect(base, channel, kTPM_HighTrue);
	} else if ((flags & PWM_POLARITY_INVERTED) != 0 &&
		   data->channel[channel].level != kTPM_LowTrue) {
		data->channel[channel].level = kTPM_LowTrue;
		TPM_UpdateChnlEdgeLevelSelect(base, channel, kTPM_LowTrue);
	}

	if (pulse_cycles == period_cycles) {
		pulse_cycles = period_cycles + 1U;
	}

	base->CONTROLS[channel].CnV = pulse_cycles;

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int mcux_tpm_configure_capture(const struct device *dev,
				      uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb,
				      void *user_data)
{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;
	tpm_dual_edge_capture_param_t *param;
	uint32_t pair = TPM_WHICH_PAIR(channel);

	if ((channel & 0x1U) == 0x1U) {
		LOG_ERR("PWM capture only supported on even channels");
		return -ENOTSUP;
	}

	if (pair >= ARRAY_SIZE(data->capture)) {
		LOG_ERR("Invalid channel pair %d", pair);
		return -EINVAL;
	}

	if ((TPM_GetEnabledInterrupts(base) & BIT(TPM_PAIR_SECOND_CH(pair))) != 0) {
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
		data->capture[pair].continuous_capture = true;
	} else {
		data->capture[pair].continuous_capture = false;
	}

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture[pair].pulse_capture = false;

		if (flags & PWM_POLARITY_INVERTED) {
			param->currChanEdgeMode = kTPM_FallingEdge;
			param->nextChanEdgeMode = kTPM_FallingEdge;
		} else {
			param->currChanEdgeMode = kTPM_RisingEdge;
			param->nextChanEdgeMode = kTPM_RisingEdge;
		}
	} else {
		data->capture[pair].pulse_capture = true;

		if (flags & PWM_POLARITY_INVERTED) {
			param->currChanEdgeMode = kTPM_FallingEdge;
			param->nextChanEdgeMode = kTPM_RisingEdge;
		} else {
			param->currChanEdgeMode = kTPM_RisingEdge;
			param->nextChanEdgeMode = kTPM_FallingEdge;
		}
	}

	return 0;
}

static int mcux_tpm_enable_capture(const struct device *dev, uint32_t channel)
{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;
	uint32_t pair = TPM_WHICH_PAIR(channel);

	if ((channel & 0x1U) == 0x1U) {
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

	if ((TPM_GetEnabledInterrupts(base) & BIT(TPM_PAIR_SECOND_CH(pair))) != 0) {
		LOG_ERR("Capture already active on channel pair %d", pair);
		return -EBUSY;
	}

	TPM_ClearStatusFlags(base, BIT(TPM_PAIR_FIRST_CH(pair)) |
			     BIT(TPM_PAIR_SECOND_CH(pair)));

	TPM_SetupDualEdgeCapture(base, pair, &data->capture[pair].param,
				 CONFIG_PWM_CAPTURE_MCUX_TPM_FILTER_VALUE);

	TPM_EnableInterrupts(base, BIT(TPM_PAIR_FIRST_CH(pair)) |
			     BIT(TPM_PAIR_SECOND_CH(pair)));

	return 0;
}

static int mcux_tpm_disable_capture(const struct device *dev, uint32_t channel)
{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;
	uint32_t pair = TPM_WHICH_PAIR(channel);

	if ((channel & 0x1U) == 0x1U) {
		LOG_ERR("PWM capture only supported on even channels");
		return -ENOTSUP;
	}

	if (pair >= ARRAY_SIZE(data->capture)) {
		LOG_ERR("Invalid channel pair %d", pair);
		return -EINVAL;
	}

	TPM_DisableInterrupts(base, BIT(TPM_PAIR_FIRST_CH(pair)) |
			      BIT(TPM_PAIR_SECOND_CH(pair)));

	/* Disable input capture combine mode */
	base->COMBINE &= ~(TPM_COMBINE_COMBINE0_MASK << ((TPM_COMBINE_SHIFT * pair)));

	return 0;
}

static void mcux_tpm_capture_first_edge(const struct device *dev, uint32_t channel, uint32_t cnt,
					bool overflow)
{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;
	struct mcux_tpm_capture_data *capture;
	uint32_t pair = TPM_WHICH_PAIR(channel);

	__ASSERT_NO_MSG(pair < ARRAY_SIZE(data->capture));

	capture = &data->capture[pair];
	capture->first_edge_cnv = TPM_GetChannelValue(base, channel);
	capture->first_edge_cnt = cnt;
	capture->first_edge_overflows = data->overflows;
	capture->first_edge_overflow = overflow;
	capture->first_chan_captured = true;

	/* Disable interrupt for first edge to prepare for second edge capture */
	TPM_DisableInterrupts(base, BIT(channel));
	TPM_ClearStatusFlags(base, BIT(channel));

	LOG_DBG("pair = %d, 1st ovfs = %d, 1st cnt = %u, 1st cnv = %u, 1st ovf = %d", pair,
		capture->first_edge_overflows, cnt, capture->first_edge_cnv, overflow);
}

static void mcux_tpm_capture_second_edge(const struct device *dev, uint32_t channel,
					 uint32_t cnt, bool overflow)

{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;
	uint32_t second_edge_overflows = data->overflows;
	uint32_t pair = TPM_WHICH_PAIR(channel);
	struct mcux_tpm_capture_data *capture;
	uint32_t overflows;
	uint32_t first_cnv;
	uint32_t second_cnv;
	uint32_t cycles;
	int status = 0;

	__ASSERT_NO_MSG(pair < ARRAY_SIZE(data->capture));
	capture = &data->capture[pair];
	first_cnv = capture->first_edge_cnv;
	second_cnv =  TPM_GetChannelValue(base, channel);

	if (unlikely(capture->first_edge_overflow && first_cnv > capture->first_edge_cnt)) {
		/* Compensate for the overflow registered in the same IRQ */
		capture->first_edge_overflows--;
	}

	if (unlikely(overflow && second_cnv > cnt)) {
		/* Compensate for the overflow registered in the same IRQ */
		second_edge_overflows--;
	}

	overflows = second_edge_overflows - capture->first_edge_overflows;

	/* Calculate cycles, check for overflows */
	if (overflows > 0) {
		if (u32_mul_overflow(overflows, base->MOD, &cycles)) {
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
		pair, capture->first_edge_overflows, second_edge_overflows,
		overflows,	capture->first_edge_cnv, second_cnv, cycles, cnt, overflow);

	if (capture->pulse_capture) {
		capture->callback(dev, pair, 0, cycles, status,
				  capture->user_data);
	} else {
		capture->callback(dev, pair, cycles, 0, status,
				  capture->user_data);
	}

	/* Prepare for next capture */
	capture->first_chan_captured = false;
	TPM_ClearStatusFlags(base, BIT(channel));
	if (capture->continuous_capture) {
		if (capture->pulse_capture) {
			/* Prepare for first edge of next pulse capture */
			TPM_EnableInterrupts(base, BIT(TPM_PAIR_FIRST_CH(pair)));
		} else {
			/* In continuous period capture mode, first edge of next period capture
			 * is second edge of this capture (this edge)
			 */
			capture->first_edge_cnv = second_cnv;
			capture->first_edge_cnt = cnt;
			capture->first_edge_overflows = second_edge_overflows;
			capture->first_edge_overflow = (overflows > 0);
			capture->first_chan_captured = true;
		}
	} else {
		/* One-shot capture done */
		TPM_DisableInterrupts(base, BIT(TPM_PAIR_SECOND_CH(pair)));
	}
}

static bool mcux_tpm_handle_overflow(const struct device *dev)
{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;

	if (TPM_GetStatusFlags(base) & kTPM_TimeOverflowFlag) {
		TPM_ClearStatusFlags(base, kTPM_TimeOverflowFlag);
		data->overflows++;
		return true;
	}

	return false;
}

static void mcux_tpm_irq_handler(const struct device *dev, uint32_t chan_start, uint32_t chan_end)
{
	TPM_Type *base = TPM_TYPE_BASE(dev, base);
	struct mcux_tpm_data *data = dev->data;
	struct mcux_tpm_capture_data *capture;
	bool overflow;
	uint32_t flags;
	uint32_t irqs;
	uint32_t cnt;
	uint32_t ch;
	uint32_t first_chan;
	uint32_t second_chan;

	flags = TPM_GetStatusFlags(base);
	irqs = TPM_GetEnabledInterrupts(base);
	cnt = base->CNT;
	overflow = mcux_tpm_handle_overflow(dev);

	for (ch = chan_start; ch < chan_end; ch = ch + 2U) {
		first_chan = ch;
		second_chan = ch + 1U;
		capture = &data->capture[TPM_WHICH_PAIR(first_chan)];

		if ((flags & BIT(second_chan)) && (irqs & BIT(second_chan))) {
			if (capture->first_chan_captured) {
				mcux_tpm_capture_second_edge(dev, second_chan, cnt, overflow);
			} else {
				/* Missed first edge, clear second edge flag */
				TPM_ClearStatusFlags(base, BIT(second_chan));
			}
		} else if ((flags & BIT(first_chan)) && (irqs & BIT(first_chan)) &&
					!capture->first_chan_captured) {
			mcux_tpm_capture_first_edge(dev, first_chan, cnt, overflow);
		} else {
			/* No action required */
		}
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static int mcux_tpm_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

static int mcux_tpm_init(const struct device *dev)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;
	tpm_chnl_pwm_signal_param_t *channel = data->channel;
	tpm_config_t tpm_config;
	TPM_Type *base;
	int i;
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	base = TPM_TYPE_BASE(dev, base);

	if (config->channel_count > ARRAY_SIZE(data->channel)) {
		LOG_ERR("Invalid channel count");
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#if defined(CONFIG_SOC_MIMX9596)
	/* IMX9596 AON and WAKEUP clocks aren't controllable */
	if (config->clock_subsys != (clock_control_subsys_t)IMX95_CLK_BUSWAKEUP &&
	    config->clock_subsys != (clock_control_subsys_t)IMX95_CLK_BUSAON) {
#endif
		if (clock_control_on(config->clock_dev, config->clock_subsys)) {
			LOG_ERR("Could not turn on clock");
			return -EINVAL;
		}
#if defined(CONFIG_SOC_MIMX9596)
	}
#endif

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
#if !(defined(FSL_FEATURE_TPM_HAS_PAUSE_LEVEL_SELECT) && FSL_FEATURE_TPM_HAS_PAUSE_LEVEL_SELECT)
		channel->level = kTPM_NoPwmSignal;
#else
		channel->level = kTPM_HighTrue;
		channel->pauseLevel = kTPM_ClearOnPause;
#endif
		channel->dutyCyclePercent = 0;
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
		channel->firstEdgeDelayPercent = 0;
#endif
		channel++;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	TPM_GetDefaultConfig(&tpm_config);
	tpm_config.prescale = config->prescale;

	TPM_Init(base, &tpm_config);

#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);
	TPM_EnableInterrupts(base, kTPM_TimeOverflowInterruptEnable);
	data->period_cycles = 0xFFFFU;
	TPM_SetTimerPeriod(base, data->period_cycles);
	TPM_StartTimer(base, config->tpm_clock_source);
#endif /* CONFIG_PWM_CAPTURE */

	return 0;
}

static DEVICE_API(pwm, mcux_tpm_driver_api) = {
	.set_cycles = mcux_tpm_set_cycles,
	.get_cycles_per_sec = mcux_tpm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_tpm_configure_capture,
	.enable_capture = mcux_tpm_enable_capture,
	.disable_capture = mcux_tpm_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

#define TO_TPM_PRESCALE_DIVIDE(val) _DO_CONCAT(kTPM_Prescale_Divide_, val)

#ifdef CONFIG_PWM_CAPTURE

static void mcux_tpm_isr(const struct device *dev)
{
	const struct mcux_tpm_config *cfg = dev->config;

	mcux_tpm_irq_handler(dev, 0, cfg->channel_count);
}

#define TPM_CONFIG_FUNC(n) \
static void mcux_tpm_config_func_##n(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
	mcux_tpm_isr, DEVICE_DT_INST_GET(n), 0); \
	irq_enable(DT_INST_IRQN(n)); \
}
#define TPM_CFG_CAPTURE_INIT(n) \
	.irq_config_func = mcux_tpm_config_func_##n
#define TPM_INIT_CFG(n)	TPM_DECLARE_CFG(n, TPM_CFG_CAPTURE_INIT(n))
#else /* !CONFIG_PWM_CAPTURE */
#define TPM_CONFIG_FUNC(n)
#define TPM_CFG_CAPTURE_INIT
#define TPM_INIT_CFG(n)	TPM_DECLARE_CFG(n, TPM_CFG_CAPTURE_INIT)
#endif /* !CONFIG_PWM_CAPTURE */

#define TPM_DECLARE_CFG(n, CAPTURE_INIT) \
	static const struct mcux_tpm_config mcux_tpm_config_##n = { \
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(n)), \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clock_subsys = (clock_control_subsys_t) \
			DT_INST_CLOCKS_CELL(n, name), \
		.tpm_clock_source = kTPM_SystemClock, \
		.prescale = TO_TPM_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)), \
		.channel_count = FSL_FEATURE_TPM_CHANNEL_COUNTn((TPM_Type *) \
			DT_INST_REG_ADDR(n)), \
		.mode = kTPM_EdgeAlignedPwm, \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		CAPTURE_INIT \
	}

#define TPM_DEVICE(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static const struct mcux_tpm_config mcux_tpm_config_##n; \
	static struct mcux_tpm_data mcux_tpm_data_##n; \
	DEVICE_DT_INST_DEFINE(n, &mcux_tpm_init, NULL, \
			    &mcux_tpm_data_##n, \
			    &mcux_tpm_config_##n, \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			    &mcux_tpm_driver_api); \
	TPM_CONFIG_FUNC(n) \
	TPM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(TPM_DEVICE)
