/*
 * Copyright (c) 2018, Cue Health Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <nrfx_pwm.h>
#include <pwm.h>

#define SYS_LOG_DOMAIN "pwm_nrfx"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_PWM_LEVEL
#include <logging/sys_log.h>

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
		SYS_LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

	nrfx_pwm_simple_playback(&pconfig->pwm,
				 &pconfig->seq,
				 1,
				 NRFX_PWM_FLAG_LOOP);

	return 0;
}


#define PWM_NRFX_OUTPUT_PIN(dev_idx, ch_idx)		     \
	(CONFIG_PWM_##dev_idx##_NRF_CH##ch_idx##_PIN |	     \
	 (CONFIG_PWM_##dev_idx##_NRF_CH##ch_idx##_INVERTED ? \
	  NRFX_PWM_PIN_INVERTED : 0))

#define PWM_NRFX_DEFAULT_VALUE(dev_idx, ch_idx)		    \
	(CONFIG_PWM_##dev_idx##_NRF_CH##ch_idx##_INVERTED ? \
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
									      \
	DEVICE_AND_API_INIT(pwm_nrfx_##idx, CONFIG_PWM_##idx##_NAME,	      \
			    pwm_nrfx_init, &pwm_nrfx_##idx##_data,	      \
			    &pwm_nrfx_##idx##_config,			      \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,  \
			    &pwm_nrfx_drv_api_funcs)

#ifdef CONFIG_PWM_0
#ifndef CONFIG_PWM_0_NRF_CH0_INVERTED
#define CONFIG_PWM_0_NRF_CH0_INVERTED 0
#endif
#ifndef CONFIG_PWM_0_NRF_CH1_INVERTED
#define CONFIG_PWM_0_NRF_CH1_INVERTED 0
#endif
#ifndef CONFIG_PWM_0_NRF_CH2_INVERTED
#define CONFIG_PWM_0_NRF_CH2_INVERTED 0
#endif
#ifndef CONFIG_PWM_0_NRF_CH3_INVERTED
#define CONFIG_PWM_0_NRF_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(0);
#endif

#ifdef CONFIG_PWM_1
#ifndef CONFIG_PWM_1_NRF_CH0_INVERTED
#define CONFIG_PWM_1_NRF_CH0_INVERTED 0
#endif
#ifndef CONFIG_PWM_1_NRF_CH1_INVERTED
#define CONFIG_PWM_1_NRF_CH1_INVERTED 0
#endif
#ifndef CONFIG_PWM_1_NRF_CH2_INVERTED
#define CONFIG_PWM_1_NRF_CH2_INVERTED 0
#endif
#ifndef CONFIG_PWM_1_NRF_CH3_INVERTED
#define CONFIG_PWM_1_NRF_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(1);
#endif

#ifdef CONFIG_PWM_2
#ifndef CONFIG_PWM_2_NRF_CH0_INVERTED
#define CONFIG_PWM_2_NRF_CH0_INVERTED 0
#endif
#ifndef CONFIG_PWM_2_NRF_CH1_INVERTED
#define CONFIG_PWM_2_NRF_CH1_INVERTED 0
#endif
#ifndef CONFIG_PWM_2_NRF_CH2_INVERTED
#define CONFIG_PWM_2_NRF_CH2_INVERTED 0
#endif
#ifndef CONFIG_PWM_2_NRF_CH3_INVERTED
#define CONFIG_PWM_2_NRF_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(2);
#endif

#ifdef CONFIG_PWM_3
#ifndef CONFIG_PWM_3_NRF_CH0_INVERTED
#define CONFIG_PWM_3_NRF_CH0_INVERTED 0
#endif
#ifndef CONFIG_PWM_3_NRF_CH1_INVERTED
#define CONFIG_PWM_3_NRF_CH1_INVERTED 0
#endif
#ifndef CONFIG_PWM_3_NRF_CH2_INVERTED
#define CONFIG_PWM_3_NRF_CH2_INVERTED 0
#endif
#ifndef CONFIG_PWM_3_NRF_CH3_INVERTED
#define CONFIG_PWM_3_NRF_CH3_INVERTED 0
#endif
PWM_NRFX_DEVICE(3);
#endif
