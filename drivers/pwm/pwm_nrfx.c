/*
 * Copyright (c) 2018, Cue Health Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx_pwm.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <hal/nrf_gpio.h>
#include <stdbool.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_nrfx, CONFIG_PWM_LOG_LEVEL);

/* NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED can be undefined or defined
 * to 0 or 1, hence the use of #if IS_ENABLED().
 */
#if IS_ENABLED(NRFX_PWM_NRF52_ANOMALY_109_WORKAROUND_ENABLED)
#define ANOMALY_109_IRQ_CONNECT(...) IRQ_CONNECT(__VA_ARGS__)
#define ANOMALY_109_EGU_IRQ_CONNECT(idx) _EGU_IRQ_CONNECT(idx)
#define _EGU_IRQ_CONNECT(idx) \
	extern void nrfx_egu_##idx##_irq_handler(void); \
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(egu##idx)), \
		    DT_IRQ(DT_NODELABEL(egu##idx), priority), \
		    nrfx_isr, nrfx_egu_##idx##_irq_handler, 0)
#else
#define ANOMALY_109_IRQ_CONNECT(...)
#define ANOMALY_109_EGU_IRQ_CONNECT(idx)
#endif

#define PWM_NRFX_CH_POLARITY_MASK BIT(15)
#define PWM_NRFX_CH_COMPARE_MASK  BIT_MASK(15)
#define PWM_NRFX_CH_VALUE(compare_value, inverted) \
	(compare_value | (inverted ? 0 : PWM_NRFX_CH_POLARITY_MASK))

struct pwm_nrfx_config {
	nrfx_pwm_t pwm;
	nrfx_pwm_config_t initial_config;
	nrf_pwm_sequence_t seq;
	const struct pinctrl_dev_config *pcfg;
};

struct pwm_nrfx_data {
	uint32_t period_cycles;
	/* Bit mask indicating channels that need the PWM generation. */
	uint8_t  pwm_needed;
	uint8_t  prescaler;
	bool     stop_requested;
};
/* Ensure the pwm_needed bit mask can accommodate all available channels. */
#if (NRF_PWM_CHANNEL_COUNT > 8)
#error "Current implementation supports maximum 8 channels."
#endif

static uint16_t *seq_values_ptr_get(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;

	return (uint16_t *)config->seq.values.p_raw;
}

static bool pwm_period_check_and_set(const struct device *dev,
				     uint32_t channel, uint32_t period_cycles)
{
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;
	uint8_t prescaler;
	uint32_t countertop;

	/* If the currently configured period matches the requested one,
	 * nothing more needs to be done.
	 */
	if (period_cycles == data->period_cycles) {
		return true;
	}

	/* If any other channel is driven by the PWM peripheral, the period
	 * that is currently set cannot be changed, as this would influence
	 * the output for that channel.
	 */
	if ((data->pwm_needed & ~BIT(channel)) != 0) {
		LOG_ERR("Incompatible period.");
		return false;
	}

	/* Try to find a prescaler that will allow setting the requested period
	 * after prescaling as the countertop value for the PWM peripheral.
	 */
	prescaler = 0;
	countertop = period_cycles;
	do {
		if (countertop <= PWM_COUNTERTOP_COUNTERTOP_Msk) {
			data->period_cycles = period_cycles;
			data->prescaler     = prescaler;

			nrf_pwm_configure(config->pwm.p_reg,
					  data->prescaler,
					  config->initial_config.count_mode,
					  (uint16_t)countertop);
			return true;
		}

		countertop >>= 1;
		++prescaler;
	} while (prescaler <= PWM_PRESCALER_PRESCALER_Msk);

	LOG_ERR("Prescaler for period_cycles %u not found.", period_cycles);
	return false;
}

static bool channel_psel_get(uint32_t channel, uint32_t *psel,
			     const struct pwm_nrfx_config *config)
{
	*psel = nrf_pwm_pin_get(config->pwm.p_reg, (uint8_t)channel);

	return (((*psel & PWM_PSEL_OUT_CONNECT_Msk) >> PWM_PSEL_OUT_CONNECT_Pos)
		== PWM_PSEL_OUT_CONNECT_Connected);
}

static int pwm_nrfx_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	/* We assume here that period_cycles will always be 16MHz
	 * peripheral clock. Since pwm_nrfx_get_cycles_per_sec() function might
	 * be removed, see ISSUE #6958.
	 * TODO: Remove this comment when issue has been resolved.
	 */
	const struct pwm_nrfx_config *config = dev->config;
	struct pwm_nrfx_data *data = dev->data;
	uint16_t compare_value;
	bool inverted = (flags & PWM_POLARITY_INVERTED);
	bool needs_pwm = false;

	if (channel >= NRF_PWM_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel: %u.", channel);
		return -EINVAL;
	}

	/* If this PWM is in center-aligned mode, pulse and period lengths
	 * are effectively doubled by the up-down count, so halve them here
	 * to compensate.
	 */
	if (config->initial_config.count_mode == NRF_PWM_MODE_UP_AND_DOWN) {
		period_cycles /= 2;
		pulse_cycles /= 2;
	}

	if (pulse_cycles == 0) {
		/* Constantly inactive (duty 0%). */
		compare_value = 0;
	} else if (pulse_cycles >= period_cycles) {
		/* Constantly active (duty 100%). */
		/* This value is always greater than or equal to COUNTERTOP. */
		compare_value = PWM_NRFX_CH_COMPARE_MASK;
	} else {
		/* PWM generation needed. Check if the requested period matches
		 * the one that is currently set, or the PWM peripheral can be
		 * reconfigured accordingly.
		 */
		if (!pwm_period_check_and_set(dev, channel, period_cycles)) {
			return -EINVAL;
		}

		compare_value = (uint16_t)(pulse_cycles >> data->prescaler);
		needs_pwm = true;
	}

	seq_values_ptr_get(dev)[channel] = PWM_NRFX_CH_VALUE(compare_value, inverted);

	LOG_DBG("channel %u, pulse %u, period %u, prescaler: %u.",
		channel, pulse_cycles, period_cycles, data->prescaler);

	/* If this channel does not need to be driven by the PWM peripheral
	 * because its state is to be constant (duty 0% or 100%), set properly
	 * the GPIO configuration for its output pin. This will provide
	 * the correct output state for this channel when the PWM peripheral
	 * is stopped.
	 */
	if (!needs_pwm) {
		uint32_t psel;

		if (channel_psel_get(channel, &psel, config)) {
			uint32_t out_level = (pulse_cycles == 0) ? 0 : 1;

			if (inverted) {
				out_level ^= 1;
			}

			nrf_gpio_pin_write(psel, out_level);
		}

		data->pwm_needed &= ~BIT(channel);
	} else {
		data->pwm_needed |= BIT(channel);
	}

	/* If the PWM generation is not needed for any channel (all are set
	 * to constant inactive or active state), stop the PWM peripheral.
	 * Otherwise, request a playback of the defined sequence so that
	 * the PWM peripheral loads `seq_values` into its internal compare
	 * registers and drives its outputs accordingly.
	 */
	if (data->pwm_needed == 0) {
		/* Don't wait here for the peripheral to actually stop. Instead,
		 * ensure it is stopped before starting the next playback.
		 */
		nrfx_pwm_stop(&config->pwm, false);
		data->stop_requested = true;
	} else {
		if (data->stop_requested) {
			data->stop_requested = false;

			/* After a stop is requested, the PWM peripheral stops
			 * pulse generation at the end of the current period,
			 * and till that moment, it ignores any start requests,
			 * so ensure here that it is stopped.
			 */
			while (!nrfx_pwm_stopped_check(&config->pwm)) {
			}
		}

		/* It is sufficient to play the sequence once without looping.
		 * The PWM generation will continue with the loaded values
		 * until another playback is requested (new values will be
		 * loaded then) or the PWM peripheral is stopped.
		 */
		nrfx_pwm_simple_playback(&config->pwm, &config->seq, 1, 0);
	}

	return 0;
}

static int pwm_nrfx_get_cycles_per_sec(const struct device *dev, uint32_t channel,
				       uint64_t *cycles)
{
	/* TODO: Since this function might be removed, we will always return
	 * 16MHz from this function and handle the conversion with prescaler,
	 * etc, in the pin set function. See issue #6958.
	 */
	*cycles = 16ul * 1000ul * 1000ul;

	return 0;
}

static const struct pwm_driver_api pwm_nrfx_drv_api_funcs = {
	.set_cycles = pwm_nrfx_set_cycles,
	.get_cycles_per_sec = pwm_nrfx_get_cycles_per_sec,
};

static int pwm_nrfx_init(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;
	uint8_t initially_inverted = 0;

	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	ANOMALY_109_EGU_IRQ_CONNECT(NRFX_PWM_NRF52_ANOMALY_109_EGU_INSTANCE);

	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < NRF_PWM_CHANNEL_COUNT; i++) {
		uint32_t psel;

		if (channel_psel_get(i, &psel, config)) {
			/* Mark channels as inverted according to what initial
			 * state of their outputs has been set by pinctrl (high
			 * idle state means that the channel is inverted).
			 */
			initially_inverted |= nrf_gpio_pin_out_read(psel) ?
					      BIT(i) : 0;
		}
	}

	for (size_t i = 0; i < NRF_PWM_CHANNEL_COUNT; i++) {
		bool inverted = initially_inverted & BIT(i);

		seq_values_ptr_get(dev)[i] = PWM_NRFX_CH_VALUE(0, inverted);
	}

	nrfx_err_t result = nrfx_pwm_init(&config->pwm,
					  &config->initial_config,
					  NULL,
					  NULL);
	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return -EBUSY;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void pwm_nrfx_uninit(const struct device *dev)
{
	const struct pwm_nrfx_config *config = dev->config;

	nrfx_pwm_uninit(&config->pwm);

	memset(dev->data, 0, sizeof(struct pwm_nrfx_data));
}

static int pwm_nrfx_pm_action(const struct device *dev,
			      enum pm_device_action action)
{
	const struct pwm_nrfx_config *config = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
		ret = pwm_nrfx_init(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		pwm_nrfx_uninit(dev);

		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
		break;

	default:
		return -ENOTSUP;
	}

	return ret;
}
#else

#define pwm_nrfx_pm_action NULL

#endif /* CONFIG_PM_DEVICE */

#define PWM(dev_idx) DT_NODELABEL(pwm##dev_idx)
#define PWM_PROP(dev_idx, prop) DT_PROP(PWM(dev_idx), prop)
#define PWM_HAS_PROP(idx, prop) DT_NODE_HAS_PROP(PWM(idx), prop)

#define PWM_MEMORY_SECTION(idx)						      \
	COND_CODE_1(PWM_HAS_PROP(idx, memory_regions),			      \
		(__attribute__((__section__(LINKER_DT_NODE_REGION_NAME(	      \
			DT_PHANDLE(PWM(idx), memory_regions)))))),	      \
		())

#define PWM_NRFX_DEVICE(idx)						      \
	NRF_DT_CHECK_NODE_HAS_PINCTRL_SLEEP(PWM(idx));			      \
	static struct pwm_nrfx_data pwm_nrfx_##idx##_data;		      \
	static uint16_t pwm_##idx##_seq_values[NRF_PWM_CHANNEL_COUNT]	      \
			PWM_MEMORY_SECTION(idx);			      \
	PINCTRL_DT_DEFINE(PWM(idx));					      \
	static const struct pwm_nrfx_config pwm_nrfx_##idx##_config = {	      \
		.pwm = NRFX_PWM_INSTANCE(idx),				      \
		.initial_config = {					      \
			.skip_gpio_cfg = true,				      \
			.skip_psel_cfg = true,				      \
			.base_clock = NRF_PWM_CLK_1MHz,			      \
			.count_mode = (PWM_PROP(idx, center_aligned)	      \
				       ? NRF_PWM_MODE_UP_AND_DOWN	      \
				       : NRF_PWM_MODE_UP),		      \
			.top_value = 1000,				      \
			.load_mode = NRF_PWM_LOAD_INDIVIDUAL,		      \
			.step_mode = NRF_PWM_STEP_TRIGGERED,		      \
		},							      \
		.seq.values.p_raw = pwm_##idx##_seq_values,		      \
		.seq.length = NRF_PWM_CHANNEL_COUNT,			      \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(PWM(idx)),		      \
	};								      \
	static int pwm_nrfx_init##idx(const struct device *dev)		      \
	{								      \
		ANOMALY_109_IRQ_CONNECT(				      \
			DT_IRQN(PWM(idx)), DT_IRQ(PWM(idx), priority),	      \
			nrfx_isr, nrfx_pwm_##idx##_irq_handler, 0);	      \
		return pwm_nrfx_init(dev);				      \
	};								      \
	PM_DEVICE_DT_DEFINE(PWM(idx), pwm_nrfx_pm_action);		      \
	DEVICE_DT_DEFINE(PWM(idx),					      \
			 pwm_nrfx_init##idx, PM_DEVICE_DT_GET(PWM(idx)),      \
			 &pwm_nrfx_##idx##_data,			      \
			 &pwm_nrfx_##idx##_config,			      \
			 POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,		      \
			 &pwm_nrfx_drv_api_funcs)

#ifdef CONFIG_HAS_HW_NRF_PWM0
PWM_NRFX_DEVICE(0);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM1
PWM_NRFX_DEVICE(1);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM2
PWM_NRFX_DEVICE(2);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM3
PWM_NRFX_DEVICE(3);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM20
PWM_NRFX_DEVICE(20);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM21
PWM_NRFX_DEVICE(21);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM22
PWM_NRFX_DEVICE(22);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM120
PWM_NRFX_DEVICE(120);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM130
PWM_NRFX_DEVICE(130);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM131
PWM_NRFX_DEVICE(131);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM132
PWM_NRFX_DEVICE(132);
#endif

#ifdef CONFIG_HAS_HW_NRF_PWM133
PWM_NRFX_DEVICE(133);
#endif
