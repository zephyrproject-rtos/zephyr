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

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_nrfx);

#define PWM_NRFX_CH_POLARITY_MASK          BIT(15)
#define PWM_NRFX_CH_PULSE_CYCLES_MASK      BIT_MASK(15)
#define PWM_NRFX_CH_VALUE(value, inverted) \
	(value | (inverted ? 0 : PWM_NRFX_CH_POLARITY_MASK))

struct pwm_nrfx_config {
	nrfx_pwm_t pwm;
	nrfx_pwm_config_t initial_config;
	nrf_pwm_sequence_t seq;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
};

struct pwm_nrfx_data {
	uint32_t period_cycles;
	uint16_t current[NRF_PWM_CHANNEL_COUNT];
	uint16_t countertop;
	uint8_t  prescaler;
	uint8_t  initially_inverted;
};


static int pwm_period_check_and_set(const struct pwm_nrfx_config *config,
				    struct pwm_nrfx_data *data,
				    uint32_t channel,
				    uint32_t period_cycles)
{
	uint8_t i;
	uint8_t prescaler;
	uint32_t countertop;

	/* If any other channel (other than the one being configured) is set up
	 * with a non-zero pulse cycle, the period that is currently set cannot
	 * be changed, as this would influence the output for this channel.
	 */
	for (i = 0; i < NRF_PWM_CHANNEL_COUNT; ++i) {
		if (i != channel) {
			uint16_t channel_pulse_cycle =
				data->current[i]
				& PWM_NRFX_CH_PULSE_CYCLES_MASK;
			if (channel_pulse_cycle > 0) {
				LOG_ERR("Incompatible period.");
				return -EINVAL;
			}
		}
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
			data->countertop    = (uint16_t)countertop;

			nrf_pwm_configure(config->pwm.p_registers,
					  data->prescaler,
					  config->initial_config.count_mode,
					  data->countertop);
			return 0;
		}

		countertop >>= 1;
		++prescaler;
	} while (prescaler <= PWM_PRESCALER_PRESCALER_Msk);

	LOG_ERR("Prescaler for period_cycles %u not found.", period_cycles);
	return -EINVAL;
}

static bool pwm_channel_is_active(uint8_t channel,
				  const struct pwm_nrfx_data *data)
{
	uint16_t pulse_cycle =
		data->current[channel] & PWM_NRFX_CH_PULSE_CYCLES_MASK;

	return (pulse_cycle > 0 && pulse_cycle < data->countertop);
}

static bool any_other_channel_is_active(uint8_t channel,
					const struct pwm_nrfx_data *data)
{
	uint8_t i;

	for (i = 0; i < NRF_PWM_CHANNEL_COUNT; ++i) {
		if (i != channel && pwm_channel_is_active(i, data)) {
			return true;
		}
	}

	return false;
}

static bool channel_psel_get(uint32_t channel, uint32_t *psel,
			     const struct pwm_nrfx_config *config)
{
	*psel = nrf_pwm_pin_get(config->pwm.p_registers, channel);

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
	bool inverted = (flags & PWM_POLARITY_INVERTED);
	bool was_stopped;

	if (channel >= NRF_PWM_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel: %u.", channel);
		return -EINVAL;
	}

	/* Check if nrfx_pwm_stop function was called in previous
	 * pwm_nrfx_set_cycles call. Relying only on state returned by
	 * nrfx_pwm_is_stopped may cause race condition if the
	 * pwm_nrfx_set_cycles is called multiple times in quick succession.
	 */
	was_stopped = !pwm_channel_is_active(channel, data) &&
		      !any_other_channel_is_active(channel, data);

	/* If this PWM is in center-aligned mode, pulse and period lengths
	 * are effectively doubled by the up-down count, so halve them here
	 * to compensate.
	 */
	if (config->initial_config.count_mode == NRF_PWM_MODE_UP_AND_DOWN) {
		period_cycles /= 2;
		pulse_cycles /= 2;
	}

	/* Check if period_cycles is either matching currently used, or
	 * possible to use with our prescaler options.
	 * Don't do anything if the period length happens to be zero.
	 * In such case, the channel is treated as inactive.
	 */
	if (period_cycles != 0 && period_cycles != data->period_cycles) {
		int ret = pwm_period_check_and_set(config, data, channel,
						   period_cycles);
		if (ret) {
			return ret;
		}
	}

	data->current[channel] =
		PWM_NRFX_CH_VALUE(pulse_cycles >> data->prescaler, inverted);

	LOG_DBG("channel %u, pulse %u, period %u, prescaler: %u.",
		channel, pulse_cycles, period_cycles, data->prescaler);

	/* If this channel turns out to not need to be driven by the PWM
	 * peripheral (it is off or fully on - duty 0% or 100%), set properly
	 * the GPIO configuration for its output pin. This will provide
	 * the correct output state for this channel when the PWM peripheral
	 * is disabled after all channels appear to be inactive.
	 */
	if (!pwm_channel_is_active(channel, data)) {
		uint32_t psel;

		if (channel_psel_get(channel, &psel, config)) {
			/* If pulse 0% and pin not inverted, set LOW.
			 * If pulse 100% and pin inverted, set LOW.
			 * If pulse 0% and pin inverted, set HIGH.
			 * If pulse 100% and pin not inverted, set HIGH.
			 */
			bool pulse_0_and_not_inverted =
				(pulse_cycles == 0U) && !inverted;
			bool pulse_100_and_inverted =
				(pulse_cycles == period_cycles) && inverted;
			uint32_t value = (pulse_0_and_not_inverted ||
					  pulse_100_and_inverted) ? 0 : 1;

			nrf_gpio_pin_write(psel, value);
		}

		if (!any_other_channel_is_active(channel, data)) {
			nrfx_pwm_stop(&config->pwm, false);
		}
	} else {
		/* Since we are playing the sequence in a loop, the
		 * sequence only has to be started when its not already
		 * playing. The new channel values will be used
		 * immediately when they are written into the seq array.
		 */
		if (was_stopped) {
			/* Wait until PWM will be stopped and then start the
			 * sequence.
			 */
			while (!nrfx_pwm_is_stopped(&config->pwm)) {
			}
			nrfx_pwm_simple_playback(&config->pwm,
						 &config->seq,
						 1,
						 NRFX_PWM_FLAG_LOOP);
		}
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
	struct pwm_nrfx_data *data = dev->data;

#ifdef CONFIG_PINCTRL
	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}

	data->initially_inverted = 0;
	for (size_t i = 0; i < ARRAY_SIZE(data->current); i++) {
		uint32_t psel;

		if (channel_psel_get(i, &psel, config)) {
			/* Mark channels as inverted according to what initial
			 * state of their outputs has been set by pinctrl (high
			 * idle state means that the channel is inverted).
			 */
			data->initially_inverted |=
				nrf_gpio_pin_out_read(psel) ? BIT(i) : 0;
		}
	}
#endif

	for (size_t i = 0; i < ARRAY_SIZE(data->current); i++) {
		bool inverted = data->initially_inverted & BIT(i);

		data->current[i] = PWM_NRFX_CH_VALUE(0, inverted);
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
#ifdef CONFIG_PINCTRL
	const struct pwm_nrfx_config *config = dev->config;
#endif
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
#endif
		ret = pwm_nrfx_init(dev);
		break;

	case PM_DEVICE_ACTION_SUSPEND:
		pwm_nrfx_uninit(dev);

#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
#endif
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

#define PWM_CH_INVERTED(dev_idx, ch_idx) \
	PWM_PROP(dev_idx, ch##ch_idx##_inverted)

#define PWM_OUTPUT_PIN(dev_idx, ch_idx)					\
	COND_CODE_1(DT_NODE_HAS_PROP(PWM(dev_idx), ch##ch_idx##_pin),	\
		(PWM_PROP(dev_idx, ch##ch_idx##_pin) |			\
			(PWM_CH_INVERTED(dev_idx, ch_idx)		\
			 ? NRFX_PWM_PIN_INVERTED : 0)),			\
		(NRFX_PWM_PIN_NOT_USED))

#define PWM_NRFX_DEVICE(idx)						      \
	NRF_DT_CHECK_PIN_ASSIGNMENTS(PWM(idx), 1,			      \
				     ch0_pin, ch1_pin, ch2_pin, ch3_pin);     \
	static struct pwm_nrfx_data pwm_nrfx_##idx##_data = {		      \
		COND_CODE_1(CONFIG_PINCTRL, (),				      \
			(.initially_inverted =				      \
				(PWM_CH_INVERTED(idx, 0) ? BIT(0) : 0) |      \
				(PWM_CH_INVERTED(idx, 1) ? BIT(1) : 0) |      \
				(PWM_CH_INVERTED(idx, 2) ? BIT(2) : 0) |      \
				(PWM_CH_INVERTED(idx, 3) ? BIT(3) : 0),))     \
	};								      \
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_DEFINE(PWM(idx))));	      \
	static const struct pwm_nrfx_config pwm_nrfx_##idx##_config = {	      \
		.pwm = NRFX_PWM_INSTANCE(idx),				      \
		.initial_config = {					      \
			COND_CODE_1(CONFIG_PINCTRL,			      \
				(.skip_gpio_cfg = true,			      \
				 .skip_psel_cfg = true,),		      \
				(.output_pins = {			      \
					PWM_OUTPUT_PIN(idx, 0),		      \
					PWM_OUTPUT_PIN(idx, 1),		      \
					PWM_OUTPUT_PIN(idx, 2),		      \
					PWM_OUTPUT_PIN(idx, 3),		      \
				 },))					      \
			.base_clock = NRF_PWM_CLK_1MHz,			      \
			.count_mode = (PWM_PROP(idx, center_aligned)	      \
				       ? NRF_PWM_MODE_UP_AND_DOWN	      \
				       : NRF_PWM_MODE_UP),		      \
			.top_value = 1000,				      \
			.load_mode = NRF_PWM_LOAD_INDIVIDUAL,		      \
			.step_mode = NRF_PWM_STEP_TRIGGERED,		      \
		},							      \
		.seq.values.p_raw = pwm_nrfx_##idx##_data.current,	      \
		.seq.length = NRF_PWM_CHANNEL_COUNT,			      \
		IF_ENABLED(CONFIG_PINCTRL,				      \
			(.pcfg = PINCTRL_DT_DEV_CONFIG_GET(PWM(idx)),))	      \
	};								      \
	PM_DEVICE_DT_DEFINE(PWM(idx), pwm_nrfx_pm_action);		      \
	DEVICE_DT_DEFINE(PWM(idx),					      \
			 pwm_nrfx_init, PM_DEVICE_DT_GET(PWM(idx)),	      \
			 &pwm_nrfx_##idx##_data,			      \
			 &pwm_nrfx_##idx##_config,			      \
			 POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,     \
			 &pwm_nrfx_drv_api_funcs)

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm0), okay)
PWM_NRFX_DEVICE(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm1), okay)
PWM_NRFX_DEVICE(1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm2), okay)
PWM_NRFX_DEVICE(2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pwm3), okay)
PWM_NRFX_DEVICE(3);
#endif
