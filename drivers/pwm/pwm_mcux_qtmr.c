/*
 * Copyright (c) 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_qtmr_pwm

#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <fsl_qtmr.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux_qtmr, CONFIG_PWM_LOG_LEVEL);

#define CHANNEL_COUNT TMR_CNTR_COUNT

struct pwm_mcux_qtmr_config {
	TMR_Type *base;
	uint32_t prescaler;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
};

#ifdef CONFIG_PWM_CAPTURE
struct pwm_mcux_qtmr_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t overflow_count;
	uint32_t channel;
	bool continuous : 1;
	bool overflowed : 1;
	bool pulse_capture: 1;
	bool first_edge_captured : 1;
};
#endif /* CONFIG_PWM_CAPTURE */

struct pwm_mcux_qtmr_data {
	struct k_mutex lock;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_mcux_qtmr_capture_data capture;
#endif /* CONFIG_PWM_CAPTURE */
};

static int mcux_qtmr_pwm_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;
	uint32_t highCount, lowCount;
	uint16_t reg;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	/* Counter values to generate a PWM signal */
	highCount = pulse_cycles;
	lowCount = period_cycles - pulse_cycles;

	if (highCount > 0U) {
		highCount -= 1U;
	}
	if (lowCount > 0U) {
		lowCount -= 1U;
	}

	if ((highCount > 0xFFFFU) || (lowCount > 0xFFFFU)) {
		/* This should not be a 16-bit overflow value. If it is, change to a larger divider
		 * for clock source.
		 */
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Set OFLAG pin for output mode and force out a low on the pin */
	config->base->CHANNEL[channel].SCTRL |= (TMR_SCTRL_FORCE_MASK | TMR_SCTRL_OEN_MASK);

	QTMR_StopTimer(config->base, channel);

	/* Setup the compare registers for PWM output */
	config->base->CHANNEL[channel].COMP1 = (uint16_t)lowCount;
	config->base->CHANNEL[channel].COMP2 = (uint16_t)highCount;

	/* Setup the pre-load registers for PWM output */
	config->base->CHANNEL[channel].CMPLD1 = (uint16_t)lowCount;
	config->base->CHANNEL[channel].CMPLD2 = (uint16_t)highCount;

	reg = config->base->CHANNEL[channel].CSCTRL;
	/* Setup the compare load control for COMP1 and COMP2.
	 * Load COMP1 when CSCTRL[TCF2] is asserted, load COMP2 when CSCTRL[TCF1] is asserted
	 */
	reg &= (uint16_t)(~(TMR_CSCTRL_CL1_MASK | TMR_CSCTRL_CL2_MASK));
	reg |= (TMR_CSCTRL_CL1(kQTMR_LoadOnComp2) | TMR_CSCTRL_CL2(kQTMR_LoadOnComp1));
	config->base->CHANNEL[channel].CSCTRL = reg;

	reg = config->base->CHANNEL[channel].CTRL;
	reg &= ~(uint16_t)TMR_CTRL_OUTMODE_MASK;
	if (pulse_cycles == 0U) {
		/* 0% duty cycle, clear OFLAG output on compare */
		reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_ClearOnCompare));
	} else if (pulse_cycles == period_cycles) {
		/* 100% duty cycle, set OFLAG output on compare */
		reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_SetOnCompare));
	} else {
		/* Toggle OFLAG output using alternating compare register */
		reg |= (TMR_CTRL_LENGTH_MASK | TMR_CTRL_OUTMODE(kQTMR_ToggleOnAltCompareReg));
	}

	config->base->CHANNEL[channel].CTRL = reg;

	QTMR_StartTimer(config->base, channel, kQTMR_PriSrcRiseEdge);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_qtmr_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					    uint64_t *cycles)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	uint32_t clock_freq;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		return -EINVAL;
	}

	*cycles = clock_freq / config->prescaler;

	return 0;
}
#ifdef CONFIG_PWM_CAPTURE
static inline bool mcux_qtmr_channel_is_active(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;

	return (config->base->CHANNEL[channel].CTRL & TMR_CTRL_CM_MASK) != 0U;
}

static int mcux_qtmr_configure_capture(const struct device *dev,
				      uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb,
				      void *user_data)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;
	bool inverted = (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (mcux_qtmr_channel_is_active(dev, channel)) {
		LOG_ERR("pwm capture in progress");
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

	data->capture.callback = cb;
	data->capture.user_data = user_data;
	data->capture.channel = channel;
	data->capture.continuous =
		(flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS;

	if (flags & PWM_CAPTURE_TYPE_PERIOD) {
		data->capture.pulse_capture = false;
		/* set reloadOnCapture to true to reload counter when capture event happends,
		 * so only second caputre value is needed when calculating ticks
		 */
		QTMR_SetupInputCapture(config->base,
					(qtmr_channel_selection_t)channel,
					(qtmr_input_source_t)channel,
					inverted, true, kQTMR_RisingEdge);
	} else {
		data->capture.pulse_capture = true;
		QTMR_SetupInputCapture(config->base,
					(qtmr_channel_selection_t)channel,
					(qtmr_input_source_t)channel,
					inverted, true, kQTMR_RisingAndFallingEdge);
	}
	QTMR_EnableInterrupts(config->base, channel,
				kQTMR_EdgeInterruptEnable | kQTMR_OverflowInterruptEnable);

	return 0;
}

static int mcux_qtmr_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (!data->capture.callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	if (mcux_qtmr_channel_is_active(dev, channel)) {
		LOG_ERR("PWM capture already enabled");
		return -EBUSY;
	}

	data->capture.overflowed = false;
	data->capture.first_edge_captured = false;
	data->capture.overflow_count = 0;
	QTMR_StartTimer(config->base, channel, kQTMR_PriSrcRiseEdge);

	return 0;
}

static int mcux_qtmr_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;

	if (channel >= CHANNEL_COUNT) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	QTMR_StopTimer(config->base, channel);
	return 0;
}

static int mcux_qtmr_calc_ticks(uint32_t overflows, uint32_t capture,
			       uint32_t *result)
{
	uint32_t pulse;

	/* Calculate cycles from overflow counter */
	if (u32_mul_overflow(overflows, 0xFFFFU, &pulse)) {
		return -ERANGE;
	}

	/* Add pulse width */
	if (u32_add_overflow(pulse, capture, &pulse)) {
		return -ERANGE;
	}

	*result = pulse;

	return 0;
}

static void mcux_qtmr_isr(const struct device *dev)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;
	uint32_t ticks = 0;
	uint32_t timeCapt = 0;
	int err = 0;
	uint32_t flags;

	flags = QTMR_GetStatus(config->base, data->capture.channel);
	QTMR_ClearStatusFlags(config->base, data->capture.channel, flags);

	if ((flags & kQTMR_OverflowFlag) != 0U) {
		data->capture.overflowed |= u32_add_overflow(1,
				data->capture.overflow_count, &data->capture.overflow_count);
	}

	if ((flags & kQTMR_EdgeFlag) == 0U) {
		return;
	}

	if (!data->capture.first_edge_captured) {
		data->capture.first_edge_captured = true;
		data->capture.overflow_count = 0;
		data->capture.overflowed = false;
		return;
	}

	if (data->capture.overflowed) {
		err = -ERANGE;
	} else {
		/* calculate ticks from second capture */
		timeCapt = config->base->CHANNEL[data->capture.channel].CAPT;
		err = mcux_qtmr_calc_ticks(data->capture.overflow_count, timeCapt, &ticks);
	}

	if (data->capture.pulse_capture) {
		data->capture.callback(dev, data->capture.channel,
					0, ticks, err, data->capture.user_data);
	} else {
		data->capture.callback(dev, data->capture.channel,
					ticks, 0, err, data->capture.user_data);
	}

	/* Prepare for next capture */
	data->capture.overflowed = false;
	data->capture.overflow_count = 0;

	if (data->capture.continuous) {
		if (data->capture.pulse_capture) {
			data->capture.first_edge_captured = false;
		} else {
			/* No action required. In continuous period capture mode,
			 * first edge of next period captureis second edge of this
			 * capture (this edge)
			 */
		}
	} else {
		/* Stop timer and disable interrupts for single capture*/
		data->capture.first_edge_captured = false;
		QTMR_DisableInterrupts(config->base, data->capture.channel,
			kQTMR_EdgeInterruptEnable | kQTMR_OverflowInterruptEnable);
		QTMR_StopTimer(config->base, data->capture.channel);
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static int mcux_qtmr_pwm_init(const struct device *dev)
{
	const struct pwm_mcux_qtmr_config *config = dev->config;
	struct pwm_mcux_qtmr_data *data = dev->data;
	qtmr_config_t qtmr_config;
	int err;

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	k_mutex_init(&data->lock);

	QTMR_GetDefaultConfig(&qtmr_config);
	qtmr_config.primarySource = kQTMR_ClockDivide_1 + (31 - __builtin_clz(config->prescaler));

#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);

	qtmr_config.faultFilterCount = CONFIG_PWM_CAPTURE_MCUX_QTMR_FILTER_COUNT;
	qtmr_config.faultFilterPeriod = CONFIG_PWM_CAPTURE_MCUX_QTMR_FILTER_PERIOD;
#endif /* CONFIG_PWM_CAPTURE */

	for (int i = 0; i < CHANNEL_COUNT; i++) {
		QTMR_Init(config->base, i, &qtmr_config);
	}

	return 0;
}

static DEVICE_API(pwm, pwm_mcux_qtmr_driver_api) = {
	.set_cycles = mcux_qtmr_pwm_set_cycles,
	.get_cycles_per_sec = mcux_qtmr_pwm_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcux_qtmr_configure_capture,
	.enable_capture = mcux_qtmr_enable_capture,
	.disable_capture = mcux_qtmr_disable_capture,
#endif
};

#ifdef CONFIG_PWM_CAPTURE
#define QTMR_CONFIG_FUNC(n) \
static void mcux_qtmr_config_func_##n(const struct device *dev) \
{ \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), \
	mcux_qtmr_isr, DEVICE_DT_INST_GET(n), 0); \
	irq_enable(DT_INST_IRQN(n)); \
}
#define QTMR_CFG_CAPTURE_INIT(n) \
	.irq_config_func = mcux_qtmr_config_func_##n
#define QTMR_INIT_CFG(n)	QTMR_DECLARE_CFG(n, QTMR_CFG_CAPTURE_INIT(n))
#else /* !CONFIG_PWM_CAPTURE */
#define QTMR_CONFIG_FUNC(n)
#define QTMR_CFG_CAPTURE_INIT
#define QTMR_INIT_CFG(n)	QTMR_DECLARE_CFG(n, QTMR_CFG_CAPTURE_INIT)
#endif /* !CONFIG_PWM_CAPTURE */

#define QTMR_DECLARE_CFG(n, CAPTURE_INIT) \
	static const struct pwm_mcux_qtmr_config pwm_mcux_qtmr_config_##n = { \
		.base = (TMR_Type *)DT_INST_REG_ADDR(n),                                           \
		.prescaler = DT_INST_PROP(n, prescaler),                                           \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		CAPTURE_INIT \
	}

#define QTMR_DEVICE(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static const struct pwm_mcux_qtmr_config pwm_mcux_qtmr_config_##n; \
	static struct pwm_mcux_qtmr_data pwm_mcux_qtmr_data_##n; \
	DEVICE_DT_INST_DEFINE(n, &mcux_qtmr_pwm_init, NULL, \
			    &pwm_mcux_qtmr_data_##n, \
			    &pwm_mcux_qtmr_config_##n, \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			    &pwm_mcux_qtmr_driver_api); \
	QTMR_CONFIG_FUNC(n) \
	QTMR_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(QTMR_DEVICE)
