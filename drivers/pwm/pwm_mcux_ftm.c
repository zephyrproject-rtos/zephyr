/*
 * Copyright (c) 2017, NXP
 * Copyright (c) 2020-2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_ftm_pwm

#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_ftm.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>

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
#ifdef CONFIG_PWM_CAPTURE
	uint32_t overflows;
	struct mcux_ftm_capture_data capture[MAX_CAPTURE_PAIRS];
#endif /* CONFIG_PWM_CAPTURE */
};

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

	if (period_cycles == 0U) {
		LOG_ERR("Channel can not be set to inactive level");
		return -ENOTSUP;
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
		FTM_SetTimerPeriod(config->base, period_cycles);

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

	/* Calculate cycles, check for overflows */
	if (overflows > 0) {
		if (u32_mul_overflow(overflows, config->base->MOD, &cycles)) {
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

static void mcux_ftm_isr(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	bool overflow = false;
	uint32_t flags;
	uint32_t irqs;
	uint16_t cnt;
	uint32_t ch;

	flags = FTM_GetStatusFlags(config->base);
	irqs = FTM_GetEnabledInterrupts(config->base);
	cnt = config->base->CNT;

	if (flags & kFTM_TimeOverflowFlag) {
		data->overflows++;
		overflow = true;
		FTM_ClearStatusFlags(config->base, kFTM_TimeOverflowFlag);
	}

	for (ch = 0; ch < MAX_CHANNELS; ch++) {
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

static int mcux_ftm_init(const struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	ftm_chnl_pwm_config_param_t *channel = data->channel;
	ftm_config_t ftm_config;
	int i;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err != 0) {
		return err;
	}

	if (config->channel_count > ARRAY_SIZE(data->channel)) {
		LOG_ERR("Invalid channel count");
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
		channel->level = kFTM_NoPwmSignal;
		channel->dutyValue = 0;
		channel->firstEdgeValue = 0;
		channel++;
	}

	FTM_GetDefaultConfig(&ftm_config);
	ftm_config.prescale = config->prescale;

	FTM_Init(config->base, &ftm_config);

#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);
	FTM_EnableInterrupts(config->base,
			     kFTM_TimeOverflowInterruptEnable);

	data->period_cycles = 0xFFFFU;
	FTM_SetTimerPeriod(config->base, data->period_cycles);
	FTM_SetSoftwareTrigger(config->base, true);
	FTM_StartTimer(config->base, config->ftm_clock_source);
#endif /* CONFIG_PWM_CAPTURE */

	return 0;
}

static const struct pwm_driver_api mcux_ftm_driver_api = {
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
#define FTM_CONFIG_FUNC(n) \
static void mcux_ftm_config_func_##n(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
		    mcux_ftm_isr, DEVICE_DT_INST_GET(n), 0); \
	irq_enable(DT_INST_IRQN(n)); \
}
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
	.ftm_clock_source = kFTM_FixedClock, \
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
	DEVICE_DT_INST_DEFINE(n, &mcux_ftm_init,		       \
			    NULL, &mcux_ftm_data_##n, \
			    &mcux_ftm_config_##n, \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			    &mcux_ftm_driver_api); \
	FTM_CONFIG_FUNC(n) \
	FTM_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(FTM_DEVICE)
