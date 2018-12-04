/*
 * Copyright (c) 2018, Cue Health Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx_pwm.h>
#include <pwm.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_nrfx);

#define PWM_NRFX_CH_VALUE_NORMAL        (1UL << 15)
#define PWM_NRFX_CH_VALUE_INVERTED      (0)

struct pwm_nrfx_config {
	nrfx_pwm_t pwm;
	nrfx_pwm_config_t config;
	nrf_pwm_sequence_t seq;
};

struct pwm_nrfx_data {
	u16_t current[NRF_PWM_CHANNEL_COUNT];
	u16_t top_value;
};

static int pwm_nrfx_pin_set(struct device *dev, u32_t pwm,
			    u32_t period_cycles, u32_t pulse_cycles)
{
	const struct pwm_nrfx_config *pconfig = dev->config->config_info;
	struct pwm_nrfx_data *pdata = dev->driver_data;

	if (pwm >= NRF_PWM_CHANNEL_COUNT) {
		return -EINVAL;
	}

	if (period_cycles != pdata->top_value) {
		if (period_cycles > PWM_COUNTERTOP_COUNTERTOP_Msk) {
			return -EINVAL;
		}

		pdata->top_value = period_cycles;
		nrf_pwm_configure(pconfig->pwm.p_registers,
				  pconfig->config.base_clock,
				  pconfig->config.count_mode,
				  pdata->top_value);
	}

	if (pulse_cycles > pdata->top_value) {
		return -EINVAL;
	}

	/* Modify only the COMPARE bits while preserving the POLARITY
	 * bit that controls the inversion of the channel.
	 * For more details see product specification.
	 */
	pdata->current[pwm] = ((pdata->current[pwm]
				& ~PWM_COUNTERTOP_COUNTERTOP_Msk)
				| pulse_cycles);

	return 0;
}

static int pwm_nrfx_get_cycles_per_sec(struct device *dev, u32_t pwm,
				       u64_t *cycles)
{
	const struct pwm_nrfx_config *pconfig = dev->config->config_info;

	switch (pconfig->config.base_clock) {
	case NRF_PWM_CLK_16MHz:
		*cycles = 16ul * 1000ul * 1000ul;
		break;
	case NRF_PWM_CLK_8MHz:
		*cycles = 8ul * 1000ul * 1000ul;
		break;
	case NRF_PWM_CLK_4MHz:
		*cycles = 4ul * 1000ul * 1000ul;
		break;
	case NRF_PWM_CLK_2MHz:
		*cycles = 2ul * 1000ul * 1000ul;
		break;
	case NRF_PWM_CLK_1MHz:
		*cycles = 1ul * 1000ul * 1000ul;
		break;
	case NRF_PWM_CLK_500kHz:
		*cycles = 500ul * 1000ul;
		break;
	case NRF_PWM_CLK_250kHz:
		*cycles = 250ul * 1000ul;
		break;
	case NRF_PWM_CLK_125kHz:
		*cycles = 125ul * 1000ul;
		break;
	}
	return 0;
}

static const struct pwm_driver_api pwm_nrfx_drv_api_funcs = {
	.pin_set = pwm_nrfx_pin_set,
	.get_cycles_per_sec = pwm_nrfx_get_cycles_per_sec,
};

static int pwm_nrfx_init(struct device *dev)
{
	const struct pwm_nrfx_config *pconfig = dev->config->config_info;

	nrfx_err_t result = nrfx_pwm_init(&pconfig->pwm, &pconfig->config,
					  NULL);

	if (result != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

	nrfx_pwm_simple_playback(&pconfig->pwm,
				 &pconfig->seq,
				 1,
				 NRFX_PWM_FLAG_LOOP);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

static void pwm_nrfx_uninit(struct device *dev)
{
	const struct pwm_nrfx_config *pconfig = dev->config->config_info;

	nrfx_pwm_uninit(&pconfig->pwm);
}

static int pwm_nrfx_set_power_state(u32_t new_state,
				    u32_t current_state,
				    struct device *dev)
{
	int err = 0;

	switch (new_state) {
	case DEVICE_PM_ACTIVE_STATE:
		err = pwm_nrfx_init(dev);
		break;
	case DEVICE_PM_LOW_POWER_STATE:
	case DEVICE_PM_SUSPEND_STATE:
	case DEVICE_PM_FORCE_SUSPEND_STATE:
	case DEVICE_PM_OFF_STATE:
		if (current_state == DEVICE_PM_ACTIVE_STATE) {
			pwm_nrfx_uninit(dev);
		}
		break;
	default:
		assert(false);
		break;
	}
	return err;
}

static int pwm_nrfx_pm_control(struct device *dev,
			       u32_t ctrl_command,
			       void *context,
			       u32_t *current_state)
{
	int err = 0;

	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		u32_t new_state = *((const u32_t *)context);

		if (new_state != (*current_state)) {
			err = pwm_nrfx_set_power_state(new_state,
						       *current_state,
						       dev);
			if (!err) {
				(*current_state) = new_state;
			}
		}
	} else {
		assert(ctrl_command == DEVICE_PM_GET_POWER_STATE);
		*((u32_t *)context) = (*current_state);
	}

	return err;
}

#define PWM_NRFX_PM_CONTROL(idx)					\
	static int pwm_##idx##_nrfx_pm_control(struct device *dev,	\
					       u32_t ctrl_command,	\
					       void *context)		\
	{								\
		static u32_t current_state = DEVICE_PM_ACTIVE_STATE;	\
		return pwm_nrfx_pm_control(dev, ctrl_command, context,	\
					   &current_state);		\
	}
#else

#define PWM_NRFX_PM_CONTROL(idx)

#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

#define PWM_NRFX_OUTPUT_PIN(dev_idx, ch_idx)		      \
	(DT_NORDIC_NRF_PWM_PWM_##dev_idx##_CH##ch_idx##_PIN | \
	 (DT_NORDIC_NRF_PWM_PWM_##dev_idx##_CH##ch_idx##_INVERTED ?   \
	  NRFX_PWM_PIN_INVERTED : 0))

#define PWM_NRFX_DEFAULT_VALUE(dev_idx, ch_idx)		    \
	(DT_NORDIC_NRF_PWM_PWM_##dev_idx##_CH##ch_idx##_INVERTED ? \
	 PWM_NRFX_CH_VALUE_INVERTED : PWM_NRFX_CH_VALUE_NORMAL)

#define PWM_NRFX_DEVICE(idx)						      \
	static struct pwm_nrfx_data pwm_nrfx_##idx##_data = {		      \
		.current = {						      \
			PWM_NRFX_DEFAULT_VALUE(idx, 0),			      \
			PWM_NRFX_DEFAULT_VALUE(idx, 1),			      \
			PWM_NRFX_DEFAULT_VALUE(idx, 2),			      \
			PWM_NRFX_DEFAULT_VALUE(idx, 3),			      \
		},							      \
		.top_value = NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE		      \
	};								      \
	static const struct pwm_nrfx_config pwm_nrfx_##idx##_config = {	      \
		.pwm = NRFX_PWM_INSTANCE(idx),				      \
		.config = {						      \
			.output_pins = {				      \
				PWM_NRFX_OUTPUT_PIN(idx, 0),		      \
				PWM_NRFX_OUTPUT_PIN(idx, 1),		      \
				PWM_NRFX_OUTPUT_PIN(idx, 2),		      \
				PWM_NRFX_OUTPUT_PIN(idx, 3),		      \
			},						      \
			.base_clock = (nrf_pwm_clk_t)			      \
				      CONFIG_PWM_##idx##_NRF_CLOCK_PRESCALER, \
			.count_mode = NRF_PWM_MODE_UP,			      \
			.top_value = NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE,	      \
			.load_mode = NRF_PWM_LOAD_INDIVIDUAL,		      \
			.step_mode = NRF_PWM_STEP_TRIGGERED,		      \
		},							      \
		.seq.values.p_raw = pwm_nrfx_##idx##_data.current,	      \
		.seq.length = NRF_PWM_CHANNEL_COUNT			      \
	};								      \
	PWM_NRFX_PM_CONTROL(idx)					      \
	DEVICE_DEFINE(pwm_nrfx_##idx, CONFIG_PWM_##idx##_NAME,		      \
		      pwm_nrfx_init, pwm_##idx##_nrfx_pm_control,	      \
		      &pwm_nrfx_##idx##_data,				      \
		      &pwm_nrfx_##idx##_config,				      \
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	      \
		      &pwm_nrfx_drv_api_funcs)

#ifdef CONFIG_PWM_0
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH0_PIN
#define DT_NORDIC_NRF_PWM_PWM_0_CH0_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH0_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_0_CH0_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH1_PIN
#define DT_NORDIC_NRF_PWM_PWM_0_CH1_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH1_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_0_CH1_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH2_PIN
#define DT_NORDIC_NRF_PWM_PWM_0_CH2_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH2_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_0_CH2_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH3_PIN
#define DT_NORDIC_NRF_PWM_PWM_0_CH3_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_0_CH3_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_0_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(0);
#endif

#ifdef CONFIG_PWM_1
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH0_PIN
#define DT_NORDIC_NRF_PWM_PWM_1_CH0_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH0_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_1_CH0_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH1_PIN
#define DT_NORDIC_NRF_PWM_PWM_1_CH1_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH1_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_1_CH1_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH2_PIN
#define DT_NORDIC_NRF_PWM_PWM_1_CH2_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH2_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_1_CH2_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH3_PIN
#define DT_NORDIC_NRF_PWM_PWM_1_CH3_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_1_CH3_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_1_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(1);
#endif

#ifdef CONFIG_PWM_2
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH0_PIN
#define DT_NORDIC_NRF_PWM_PWM_2_CH0_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH0_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_2_CH0_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH1_PIN
#define DT_NORDIC_NRF_PWM_PWM_2_CH1_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH1_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_2_CH1_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH2_PIN
#define DT_NORDIC_NRF_PWM_PWM_2_CH2_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH2_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_2_CH2_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH3_PIN
#define DT_NORDIC_NRF_PWM_PWM_2_CH3_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_2_CH3_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_2_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(2);
#endif

#ifdef CONFIG_PWM_3
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH0_PIN
#define DT_NORDIC_NRF_PWM_PWM_3_CH0_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH0_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_3_CH0_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH1_PIN
#define DT_NORDIC_NRF_PWM_PWM_3_CH1_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH1_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_3_CH1_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH2_PIN
#define DT_NORDIC_NRF_PWM_PWM_3_CH2_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH2_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_3_CH2_INVERTED 0
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH3_PIN
#define DT_NORDIC_NRF_PWM_PWM_3_CH3_PIN NRFX_PWM_PIN_NOT_USED
#endif
#ifndef DT_NORDIC_NRF_PWM_PWM_3_CH3_INVERTED
#define DT_NORDIC_NRF_PWM_PWM_3_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(3);
#endif
