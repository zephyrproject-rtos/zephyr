/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_pwt

#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <fsl_pwt.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_mcux_pwt, CONFIG_PWM_LOG_LEVEL);

/* Number of PWT input ports */
#define PWT_INPUTS 4U

struct mcux_pwt_config {
	PWT_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	pwt_clock_source_t pwt_clock_source;
	pwt_clock_prescale_t prescale;
	void (*irq_config_func)(const struct device *dev);
	const struct pinctrl_dev_config *pincfg;
};

struct mcux_pwt_data {
	uint32_t clock_freq;
	uint32_t period_cycles;
	uint32_t high_overflows;
	uint32_t low_overflows;
	pwm_capture_callback_handler_t callback;
	void *user_data;
	pwt_config_t pwt_config;
	bool continuous : 1;
	bool inverted : 1;
	bool overflowed : 1;
};

static inline bool mcux_pwt_is_active(const struct device *dev)
{
	const struct mcux_pwt_config *config = dev->config;

	return !!(config->base->CS & PWT_CS_PWTEN_MASK);
}

static int mcux_pwt_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(period_cycles);
	ARG_UNUSED(pulse_cycles);
	ARG_UNUSED(flags);

	LOG_ERR("pwt only supports pwm capture");

	return -ENOTSUP;
}

static int mcux_pwt_configure_capture(const struct device *dev,
				      uint32_t channel, pwm_flags_t flags,
				      pwm_capture_callback_handler_t cb,
				      void *user_data)
{
	const struct mcux_pwt_config *config = dev->config;
	struct mcux_pwt_data *data = dev->data;

	if (channel >= PWT_INPUTS) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (mcux_pwt_is_active(dev)) {
		LOG_ERR("pwm capture in progress");
		return -EBUSY;
	}

	data->callback = cb;
	data->user_data = user_data;

	data->pwt_config.inputSelect = channel;

	data->continuous =
		(flags & PWM_CAPTURE_MODE_MASK) == PWM_CAPTURE_MODE_CONTINUOUS;
	data->inverted =
		(flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED;

	PWT_Init(config->base, &data->pwt_config);
	PWT_EnableInterrupts(config->base,
			     kPWT_PulseWidthReadyInterruptEnable |
			     kPWT_CounterOverflowInterruptEnable);

	return 0;
}

static int mcux_pwt_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct mcux_pwt_config *config = dev->config;
	struct mcux_pwt_data *data = dev->data;

	if (channel >= PWT_INPUTS) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	if (!data->callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	if (mcux_pwt_is_active(dev)) {
		LOG_ERR("PWM capture already enabled");
		return -EBUSY;
	}

	data->overflowed = false;
	data->high_overflows = 0;
	data->low_overflows = 0;
	PWT_StartTimer(config->base);

	return 0;
}

static int mcux_pwt_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct mcux_pwt_config *config = dev->config;

	if (channel >= PWT_INPUTS) {
		LOG_ERR("invalid channel %d", channel);
		return -EINVAL;
	}

	PWT_StopTimer(config->base);

	return 0;
}

static int mcux_pwt_calc_period(uint16_t ppw, uint16_t npw,
				uint32_t high_overflows,
				uint32_t low_overflows,
				uint32_t *result)
{
	uint32_t period;

	/* Calculate sum of overflow counters */
	if (u32_add_overflow(high_overflows, low_overflows, &period)) {
		return -ERANGE;
	}

	/* Calculate cycles from sum of overflow counters */
	if (u32_mul_overflow(period, 0xFFFFU, &period)) {
		return -ERANGE;
	}

	/* Add positive pulse width */
	if (u32_add_overflow(period, ppw, &period)) {
		return -ERANGE;
	}

	/* Add negative pulse width */
	if (u32_add_overflow(period, npw, &period)) {
		return -ERANGE;
	}

	*result = period;

	return 0;
}

static int mcux_pwt_calc_pulse(uint16_t pw, uint32_t overflows,
			       uint32_t *result)
{
	uint32_t pulse;

	/* Calculate cycles from overflow counter */
	if (u32_mul_overflow(overflows, 0xFFFFU, &pulse)) {
		return -ERANGE;
	}

	/* Add pulse width */
	if (u32_add_overflow(pulse, pw, &pulse)) {
		return -ERANGE;
	}

	*result = pulse;

	return 0;
}

static void mcux_pwt_isr(const struct device *dev)
{
	const struct mcux_pwt_config *config = dev->config;
	struct mcux_pwt_data *data = dev->data;
	uint32_t period = 0;
	uint32_t pulse = 0;
	uint32_t flags;
	uint16_t ppw;
	uint16_t npw;
	int err;

	flags = PWT_GetStatusFlags(config->base);

	if (flags & kPWT_CounterOverflowFlag) {
		if (config->base->CR & PWT_CR_LVL_MASK) {
			data->overflowed |= u32_add_overflow(1,
				data->high_overflows, &data->high_overflows);
		} else {
			data->overflowed |= u32_add_overflow(1,
				data->low_overflows, &data->low_overflows);
		}

		PWT_ClearStatusFlags(config->base, kPWT_CounterOverflowFlag);
	}

	if (flags & kPWT_PulseWidthValidFlag) {
		ppw = PWT_ReadPositivePulseWidth(config->base);
		npw = PWT_ReadNegativePulseWidth(config->base);

		if (!data->continuous) {
			PWT_StopTimer(config->base);
		}

		if (data->inverted) {
			err = mcux_pwt_calc_pulse(npw, data->low_overflows,
						  &pulse);
		} else {
			err = mcux_pwt_calc_pulse(ppw, data->high_overflows,
						  &pulse);
		}

		if (err == 0) {
			err = mcux_pwt_calc_period(ppw, npw,
						   data->high_overflows,
						   data->low_overflows,
						   &period);
		}

		if (data->overflowed) {
			err = -ERANGE;
		}

		LOG_DBG("period = %d, pulse = %d, err = %d", period, pulse,
			err);

		if (data->callback) {
			data->callback(dev, data->pwt_config.inputSelect,
				       period, pulse, err, data->user_data);
		}

		data->overflowed = false;
		data->high_overflows = 0;
		data->low_overflows = 0;
		PWT_ClearStatusFlags(config->base, kPWT_PulseWidthValidFlag);
	}
}

static int mcux_pwt_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct mcux_pwt_config *config = dev->config;
	struct mcux_pwt_data *data = dev->data;

	ARG_UNUSED(channel);

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

static int mcux_pwt_init(const struct device *dev)
{
	const struct mcux_pwt_config *config = dev->config;
	struct mcux_pwt_data *data = dev->data;
	pwt_config_t *pwt_config = &data->pwt_config;
	int err;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("could not get clock frequency");
		return -EINVAL;
	}

	PWT_GetDefaultConfig(pwt_config);
	pwt_config->clockSource = config->pwt_clock_source;
	pwt_config->prescale = config->prescale;
	pwt_config->enableFirstCounterLoad = true;
	PWT_Init(config->base, pwt_config);

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	config->irq_config_func(dev);

	return 0;
}

static const struct pwm_driver_api mcux_pwt_driver_api = {
	.set_cycles = mcux_pwt_set_cycles,
	.get_cycles_per_sec = mcux_pwt_get_cycles_per_sec,
	.configure_capture = mcux_pwt_configure_capture,
	.enable_capture = mcux_pwt_enable_capture,
	.disable_capture = mcux_pwt_disable_capture,
};

#define TO_PWT_PRESCALE_DIVIDE(val) _DO_CONCAT(kPWT_Prescale_Divide_, val)

#define PWT_DEVICE(n) \
	static void mcux_pwt_config_func_##n(const struct device *dev);	\
									\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	static const struct mcux_pwt_config mcux_pwt_config_##n = {	\
		.base = (PWT_Type *)DT_INST_REG_ADDR(n),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clock_subsys =						\
		(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),	\
		.pwt_clock_source = kPWT_BusClock,			\
		.prescale =						\
		TO_PWT_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)),	\
		.irq_config_func = mcux_pwt_config_func_##n,		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	static struct mcux_pwt_data mcux_pwt_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n, &mcux_pwt_init,			\
			NULL, &mcux_pwt_data_##n,			\
			&mcux_pwt_config_##n,				\
			POST_KERNEL,					\
			CONFIG_PWM_INIT_PRIORITY,			\
			&mcux_pwt_driver_api);				\
									\
	static void mcux_pwt_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	\
			mcux_pwt_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));				\
	}

DT_INST_FOREACH_STATUS_OKAY(PWT_DEVICE)
