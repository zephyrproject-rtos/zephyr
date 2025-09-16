/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adc_context.h"
#include <nrfx_saadc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc-v3.h>
#include <zephyr/dt-bindings/adc/nrf-saadc-nrf54l.h>
#include <zephyr/dt-bindings/adc/nrf-saadc-haltium.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <dmm.h>

LOG_MODULE_REGISTER(adc_nrfx_saadc, CONFIG_ADC_LOG_LEVEL);

#define DT_DRV_COMPAT nordic_nrf_saadc

#if (NRF_SAADC_HAS_AIN_AS_PIN)

#if defined(CONFIG_NRF_PLATFORM_HALTIUM)
static const uint32_t saadc_psels[NRF_SAADC_AIN13 + 1] = {
	[NRF_SAADC_AIN0] = NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
	[NRF_SAADC_AIN1] = NRF_PIN_PORT_TO_PIN_NUMBER(1U, 1),
	[NRF_SAADC_AIN2] = NRF_PIN_PORT_TO_PIN_NUMBER(2U, 1),
	[NRF_SAADC_AIN3] = NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
	[NRF_SAADC_AIN4] = NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
	[NRF_SAADC_AIN5] = NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
	[NRF_SAADC_AIN6] = NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
	[NRF_SAADC_AIN7] = NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
	[NRF_SAADC_AIN8] = NRF_PIN_PORT_TO_PIN_NUMBER(0U, 9),
	[NRF_SAADC_AIN9] = NRF_PIN_PORT_TO_PIN_NUMBER(1U, 9),
	[NRF_SAADC_AIN10] = NRF_PIN_PORT_TO_PIN_NUMBER(2U, 9),
	[NRF_SAADC_AIN11] = NRF_PIN_PORT_TO_PIN_NUMBER(3U, 9),
	[NRF_SAADC_AIN12] = NRF_PIN_PORT_TO_PIN_NUMBER(4U, 9),
	[NRF_SAADC_AIN13] = NRF_PIN_PORT_TO_PIN_NUMBER(5U, 9),
};
#elif defined(CONFIG_SOC_NRF54L05) || defined(CONFIG_SOC_NRF54L10) || defined(CONFIG_SOC_NRF54L15)
static const uint32_t saadc_psels[NRF_SAADC_DVDD + 1] = {
	[NRF_SAADC_AIN0] = NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
	[NRF_SAADC_AIN1] = NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
	[NRF_SAADC_AIN2] = NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
	[NRF_SAADC_AIN3] = NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
	[NRF_SAADC_AIN4] = NRF_PIN_PORT_TO_PIN_NUMBER(11U, 1),
	[NRF_SAADC_AIN5] = NRF_PIN_PORT_TO_PIN_NUMBER(12U, 1),
	[NRF_SAADC_AIN6] = NRF_PIN_PORT_TO_PIN_NUMBER(13U, 1),
	[NRF_SAADC_AIN7] = NRF_PIN_PORT_TO_PIN_NUMBER(14U, 1),
	[NRF_SAADC_VDD]  = NRF_SAADC_INPUT_VDD,
	[NRF_SAADC_AVDD] = NRF_SAADC_INPUT_AVDD,
	[NRF_SAADC_DVDD] = NRF_SAADC_INPUT_DVDD,
};
#elif defined(NRF54LM20A_ENGA_XXAA)
static const uint32_t saadc_psels[NRF_SAADC_DVDD + 1] = {
	[NRF_SAADC_AIN0] = NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
	[NRF_SAADC_AIN1] = NRF_PIN_PORT_TO_PIN_NUMBER(31U, 1),
	[NRF_SAADC_AIN2] = NRF_PIN_PORT_TO_PIN_NUMBER(30U, 1),
	[NRF_SAADC_AIN3] = NRF_PIN_PORT_TO_PIN_NUMBER(29U, 1),
	[NRF_SAADC_AIN4] = NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
	[NRF_SAADC_AIN5] = NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
	[NRF_SAADC_AIN6] = NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
	[NRF_SAADC_AIN7] = NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
	[NRF_SAADC_VDD]  = NRF_SAADC_INPUT_VDD,
	[NRF_SAADC_AVDD] = NRF_SAADC_INPUT_AVDD,
	[NRF_SAADC_DVDD] = NRF_SAADC_INPUT_DVDD,
};
#elif defined(NRF54LV10A_ENGA_XXAA)
static const uint32_t saadc_psels[NRF_SAADC_AIN7 + 1] = {
	[NRF_SAADC_AIN0] = NRF_PIN_PORT_TO_PIN_NUMBER(0U, 1),
	[NRF_SAADC_AIN1] = NRF_PIN_PORT_TO_PIN_NUMBER(1U, 1),
	[NRF_SAADC_AIN2] = NRF_PIN_PORT_TO_PIN_NUMBER(2U, 1),
	[NRF_SAADC_AIN3] = NRF_PIN_PORT_TO_PIN_NUMBER(3U, 1),
	[NRF_SAADC_AIN4] = NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
	[NRF_SAADC_AIN5] = NRF_PIN_PORT_TO_PIN_NUMBER(10U, 1),
	[NRF_SAADC_AIN6] = NRF_PIN_PORT_TO_PIN_NUMBER(11U, 1),
	[NRF_SAADC_AIN7] = NRF_PIN_PORT_TO_PIN_NUMBER(12U, 1),
};
#elif defined(NRF54LS05B_ENGA_XXAA)
static const uint32_t saadc_psels[NRF_SAADC_AIN3 + 1] = {
	[NRF_SAADC_AIN0] = NRF_PIN_PORT_TO_PIN_NUMBER(4U, 1),
	[NRF_SAADC_AIN1] = NRF_PIN_PORT_TO_PIN_NUMBER(5U, 1),
	[NRF_SAADC_AIN2] = NRF_PIN_PORT_TO_PIN_NUMBER(6U, 1),
	[NRF_SAADC_AIN3] = NRF_PIN_PORT_TO_PIN_NUMBER(7U, 1),
};
#endif

#else
BUILD_ASSERT((NRF_SAADC_AIN0 == NRF_SAADC_INPUT_AIN0) &&
	     (NRF_SAADC_AIN1 == NRF_SAADC_INPUT_AIN1) &&
	     (NRF_SAADC_AIN2 == NRF_SAADC_INPUT_AIN2) &&
	     (NRF_SAADC_AIN3 == NRF_SAADC_INPUT_AIN3) &&
	     (NRF_SAADC_AIN4 == NRF_SAADC_INPUT_AIN4) &&
	     (NRF_SAADC_AIN5 == NRF_SAADC_INPUT_AIN5) &&
	     (NRF_SAADC_AIN6 == NRF_SAADC_INPUT_AIN6) &&
	     (NRF_SAADC_AIN7 == NRF_SAADC_INPUT_AIN7) &&
#if defined(SAADC_CH_PSELP_PSELP_VDDHDIV5)
	     (NRF_SAADC_VDDHDIV5 == NRF_SAADC_INPUT_VDDHDIV5) &&
#endif
#if defined(SAADC_CH_PSELP_PSELP_VDD)
	     (NRF_SAADC_VDD == NRF_SAADC_INPUT_VDD) &&
#endif
	     1,
	     "Definitions from nrf-adc.h do not match those from nrf_saadc.h");
#endif

struct driver_data {
	struct adc_context ctx;
	uint8_t single_ended_channels;
	uint8_t divide_single_ended_value;
	uint8_t active_channel_cnt;
	void *mem_reg;
	void *user_buffer;
	struct k_timer timer;
	bool internal_timer_enabled;
};

static struct driver_data m_data = {
	ADC_CONTEXT_INIT_LOCK(m_data, ctx),
	ADC_CONTEXT_INIT_SYNC(m_data, ctx),
	.mem_reg = DMM_DEV_TO_REG(DT_NODELABEL(adc)),
	.internal_timer_enabled = false,
};

/* Maximum value of the internal timer interval in microseconds. */
#define ADC_INTERNAL_TIMER_INTERVAL_MAX_US 128U

/* Forward declaration */
static void event_handler(const nrfx_saadc_evt_t *event);

/* Helper function to convert acquisition time to register TACQ value. */
static int acq_time_set(nrf_saadc_channel_config_t *ch_cfg, uint16_t acquisition_time)
{
#if NRF_SAADC_HAS_ACQTIME_ENUM
	switch (acquisition_time) {
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 3):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_3US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 5):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_5US;
		break;
	case ADC_ACQ_TIME_DEFAULT:
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_10US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 15):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_15US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_20US;
		break;
	case ADC_ACQ_TIME_MAX:
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_40US;
		break;
	default:
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}
#else
#define MINIMUM_ACQ_TIME_IN_NS 125
#define DEFAULT_ACQ_TIME_IN_NS 10000

	nrf_saadc_acqtime_t tacq = 0;
	uint16_t acq_time =
		(acquisition_time == ADC_ACQ_TIME_DEFAULT
			 ? DEFAULT_ACQ_TIME_IN_NS
			 : (ADC_ACQ_TIME_VALUE(acquisition_time) *
			    (ADC_ACQ_TIME_UNIT(acquisition_time) == ADC_ACQ_TIME_MICROSECONDS
				     ? 1000
				     : 1)));

	tacq = (nrf_saadc_acqtime_t)(acq_time / MINIMUM_ACQ_TIME_IN_NS) - 1;
	if ((tacq > NRF_SAADC_ACQTIME_MAX) || (acq_time < MINIMUM_ACQ_TIME_IN_NS)) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	} else {
		ch_cfg->acq_time = tacq;
	}
#endif

	LOG_DBG("ADC acquisition_time: %d", acquisition_time);

	return 0;
}

static int input_assign(nrf_saadc_input_t *pin_p,
			nrf_saadc_input_t *pin_n,
			const struct adc_channel_cfg *channel_cfg)
{
#if (NRF_SAADC_HAS_AIN_AS_PIN)
	if (channel_cfg->input_positive > ARRAY_SIZE(saadc_psels) ||
	    channel_cfg->input_positive < NRF_SAADC_AIN0) {
		LOG_ERR("Invalid analog positive input number: %d", channel_cfg->input_positive);
		return -EINVAL;
	}

	*pin_p = saadc_psels[channel_cfg->input_positive];

	if (channel_cfg->differential) {
		if (channel_cfg->input_negative > ARRAY_SIZE(saadc_psels) ||
		    (IS_ENABLED(CONFIG_NRF_PLATFORM_HALTIUM) &&
		     (channel_cfg->input_positive > NRF_SAADC_AIN7) !=
		     (channel_cfg->input_negative > NRF_SAADC_AIN7))) {
			LOG_ERR("Invalid analog negative input number: %d",
				channel_cfg->input_negative);
			return -EINVAL;
		}
		*pin_n = channel_cfg->input_negative == NRF_SAADC_GND ?
			 NRF_SAADC_INPUT_DISABLED :
			 saadc_psels[channel_cfg->input_negative];
	} else {
		*pin_n = NRF_SAADC_INPUT_DISABLED;
	}
#else
	*pin_p = channel_cfg->input_positive;
	*pin_n = (channel_cfg->differential && (channel_cfg->input_negative != NRF_SAADC_GND))
			 ? channel_cfg->input_negative
			 : NRF_SAADC_INPUT_DISABLED;
#endif
	LOG_DBG("ADC positive input: %d", *pin_p);
	LOG_DBG("ADC negative input: %d", *pin_n);

	return 0;
}

static int gain_set(nrf_saadc_channel_config_t *ch_cfg, enum adc_gain gain)
{
#if NRF_SAADC_HAS_CH_GAIN
	switch (gain) {
#if defined(SAADC_CH_CONFIG_GAIN_Gain1_6)
	case ADC_GAIN_1_6:
		ch_cfg->gain = NRF_SAADC_GAIN1_6;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain1_5)
	case ADC_GAIN_1_5:
		ch_cfg->gain = NRF_SAADC_GAIN1_5;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain1_4) || defined(SAADC_CH_CONFIG_GAIN_Gain2_8)
	case ADC_GAIN_1_4:
		ch_cfg->gain = NRF_SAADC_GAIN1_4;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain2_7)
	case ADC_GAIN_2_7:
		ch_cfg->gain = NRF_SAADC_GAIN2_7;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain1_3) || defined(SAADC_CH_CONFIG_GAIN_Gain2_6)
	case ADC_GAIN_1_3:
		ch_cfg->gain = NRF_SAADC_GAIN1_3;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain2_5)
	case ADC_GAIN_2_5:
		ch_cfg->gain = NRF_SAADC_GAIN2_5;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain1_2) || defined(SAADC_CH_CONFIG_GAIN_Gain2_4)
	case ADC_GAIN_1_2:
		ch_cfg->gain = NRF_SAADC_GAIN1_2;
		break;
#endif
#if defined(SAADC_CH_CONFIG_GAIN_Gain2_3)
	case ADC_GAIN_2_3:
		ch_cfg->gain = NRF_SAADC_GAIN2_3;
		break;
#endif
	case ADC_GAIN_1:
		ch_cfg->gain = NRF_SAADC_GAIN1;
		break;
	case ADC_GAIN_2:
		ch_cfg->gain = NRF_SAADC_GAIN2;
		break;
#if defined(SAADC_CH_CONFIG_GAIN_Gain4)
	case ADC_GAIN_4:
		ch_cfg->gain = NRF_SAADC_GAIN4;
		break;
#endif
	default:
#else
	if (gain != ADC_GAIN_1) {
#endif /* defined(NRF_SAADC_HAS_CH_GAIN) */
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	return 0;
}

static int reference_set(nrf_saadc_channel_config_t *ch_cfg, enum adc_reference reference)
{
	switch (reference) {
#if defined(SAADC_CH_CONFIG_REFSEL_Internal)
	case ADC_REF_INTERNAL:
		ch_cfg->reference = NRF_SAADC_REFERENCE_INTERNAL;
		break;
#endif
#if defined(SAADC_CH_CONFIG_REFSEL_VDD1_4)
	case ADC_REF_VDD_1_4:
		ch_cfg->reference = NRF_SAADC_REFERENCE_VDD4;
		break;
#endif
#if defined(SAADC_CH_CONFIG_REFSEL_External)
	case ADC_REF_EXTERNAL0:
		ch_cfg->reference = NRF_SAADC_REFERENCE_EXTERNAL;
		break;
#endif
	default:
		LOG_ERR("Selected ADC reference is not valid");
		return -EINVAL;
	}

	return 0;
}

/* Implementation of the ADC driver API function: adc_channel_setup. */
static int adc_nrfx_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	int err;
	nrf_saadc_channel_config_t *ch_cfg;
	nrfx_saadc_channel_t cfg = {
		.channel_config = {
#if NRF_SAADC_HAS_CH_CONFIG_RES
			.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
			.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
#endif
#if NRF_SAADC_HAS_CH_BURST
			.burst = NRF_SAADC_BURST_DISABLED,
#endif
		},
		.channel_index = channel_cfg->channel_id,
	};

	if (channel_cfg->channel_id >= SAADC_CH_NUM) {
		LOG_ERR("Invalid channel ID: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	ch_cfg = &cfg.channel_config;

	err = input_assign(&cfg.pin_p, &cfg.pin_n, channel_cfg);
	if (err != 0) {
		return err;
	}

	err = gain_set(ch_cfg, channel_cfg->gain);
	if (err != 0) {
		return err;
	}

	err = reference_set(ch_cfg, channel_cfg->reference);
	if (err != 0) {
		return err;
	}

	err = acq_time_set(ch_cfg, channel_cfg->acquisition_time);
	if (err != 0) {
		return err;
	}

	/* Store channel mode to allow correcting negative readings in single-ended mode
	 * after ADC sequence ends.
	 */
	if (channel_cfg->differential) {
		if (channel_cfg->input_negative == NRF_SAADC_GND) {
			ch_cfg->mode = NRF_SAADC_MODE_SINGLE_ENDED;
			m_data.single_ended_channels |= BIT(channel_cfg->channel_id);
			m_data.divide_single_ended_value |= BIT(channel_cfg->channel_id);
		} else {
			ch_cfg->mode = NRF_SAADC_MODE_DIFFERENTIAL;
			m_data.single_ended_channels &= ~BIT(channel_cfg->channel_id);
		}
	} else {
		ch_cfg->mode = NRF_SAADC_MODE_SINGLE_ENDED;
		m_data.single_ended_channels |= BIT(channel_cfg->channel_id);
		m_data.divide_single_ended_value &= ~BIT(channel_cfg->channel_id);
	}

	nrfx_err_t ret = nrfx_saadc_channel_config(&cfg);

	if (ret != NRFX_SUCCESS) {
		LOG_ERR("Cannot configure channel %d: 0x%08x", channel_cfg->channel_id, ret);
		return -EINVAL;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	if (ctx->sequence.calibrate) {
		nrfx_saadc_offset_calibrate(event_handler);
	} else {
		nrfx_err_t ret = nrfx_saadc_mode_trigger();

		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Cannot start sampling: 0x%08x", ret);
			adc_context_complete(ctx, -EIO);
		}
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	if (!m_data.internal_timer_enabled) {
		void *samples_buffer;

		if (!repeat) {
			m_data.user_buffer =
				(uint16_t *)m_data.user_buffer + m_data.active_channel_cnt;
		}

		int error = dmm_buffer_in_prepare(
			m_data.mem_reg, m_data.user_buffer,
			NRFX_SAADC_SAMPLES_TO_BYTES(m_data.active_channel_cnt), &samples_buffer);
		if (error != 0) {
			LOG_ERR("DMM buffer allocation failed err=%d", error);
			adc_context_complete(ctx, -EIO);
			return;
		}

		nrfx_err_t nrfx_err =
			nrfx_saadc_buffer_set(samples_buffer, m_data.active_channel_cnt);
		if (nrfx_err != NRFX_SUCCESS) {
			LOG_ERR("Failed to set buffer: 0x%08x", nrfx_err);
			adc_context_complete(ctx, -EIO);
		}
	}
}

static inline void adc_context_enable_timer(struct adc_context *ctx)
{
	if (!m_data.internal_timer_enabled) {
		k_timer_start(&m_data.timer, K_NO_WAIT, K_USEC(ctx->options.interval_us));
	} else {
		nrfx_err_t ret = nrfx_saadc_mode_trigger();

		if (ret != NRFX_SUCCESS) {
			LOG_ERR("Cannot start sampling: 0x%08x", ret);
			adc_context_complete(&m_data.ctx, -EIO);
		}
	}
}

static inline void adc_context_disable_timer(struct adc_context *ctx)
{
	if (!m_data.internal_timer_enabled) {
		k_timer_stop(&m_data.timer);
	}
}

static void external_timer_expired_handler(struct k_timer *timer_id)
{
	ARG_UNUSED(timer_id);

	adc_context_request_next_sampling(&m_data.ctx);
}

static int get_resolution(const struct adc_sequence *sequence, nrf_saadc_resolution_t *resolution)
{
	switch (sequence->resolution) {
	case 8:
		*resolution = NRF_SAADC_RESOLUTION_8BIT;
		break;
	case 10:
		*resolution = NRF_SAADC_RESOLUTION_10BIT;
		break;
	case 12:
		*resolution = NRF_SAADC_RESOLUTION_12BIT;
		break;
	case 14:
		*resolution = NRF_SAADC_RESOLUTION_14BIT;
		break;
	default:
		LOG_ERR("ADC resolution value %d is not valid", sequence->resolution);
		return -EINVAL;
	}

	return 0;
}

static int get_oversampling(const struct adc_sequence *sequence, uint8_t active_channel_cnt,
			    nrf_saadc_oversample_t *oversampling)
{
	if ((active_channel_cnt > 1) && (sequence->oversampling > 0)) {
		LOG_ERR(
			"Oversampling is supported for single channel only");
		return -EINVAL;
	}

	switch (sequence->oversampling) {
	case 0:
		*oversampling = NRF_SAADC_OVERSAMPLE_DISABLED;
		break;
	case 1:
		*oversampling = NRF_SAADC_OVERSAMPLE_2X;
		break;
	case 2:
		*oversampling = NRF_SAADC_OVERSAMPLE_4X;
		break;
	case 3:
		*oversampling = NRF_SAADC_OVERSAMPLE_8X;
		break;
	case 4:
		*oversampling = NRF_SAADC_OVERSAMPLE_16X;
		break;
	case 5:
		*oversampling = NRF_SAADC_OVERSAMPLE_32X;
		break;
	case 6:
		*oversampling = NRF_SAADC_OVERSAMPLE_64X;
		break;
	case 7:
		*oversampling = NRF_SAADC_OVERSAMPLE_128X;
		break;
	case 8:
		*oversampling = NRF_SAADC_OVERSAMPLE_256X;
		break;
	default:
		LOG_ERR("Oversampling value %d is not valid", sequence->oversampling);
		return -EINVAL;
	}

	return 0;
}

static int check_buffer_size(const struct adc_sequence *sequence, uint8_t active_channel_cnt)
{
	size_t needed_buffer_size;

	needed_buffer_size = NRFX_SAADC_SAMPLES_TO_BYTES(active_channel_cnt);

	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)",
			sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static bool has_single_ended(const struct adc_sequence *sequence)
{
	return sequence->channels & m_data.single_ended_channels;
}

static void correct_single_ended(const struct adc_sequence *sequence, nrf_saadc_value_t *buffer,
				 uint16_t num_samples)
{
	int16_t *sample = (int16_t *)buffer;
	uint8_t selected_channels = sequence->channels;
	uint8_t divide_mask = m_data.divide_single_ended_value;
	uint8_t single_ended_mask = m_data.single_ended_channels;

	if (m_data.internal_timer_enabled) {
		if (selected_channels & divide_mask) {
			for (int i = 0; i < num_samples; i++) {
				sample[i] /= 2;
			}
		} else {
			for (int i = 0; i < num_samples; i++) {
				if (sample[i] < 0) {
					sample[i] = 0;
				}
			}
		}
		return;
	}

	for (uint16_t channel_bit = BIT(0); channel_bit <= single_ended_mask; channel_bit <<= 1) {
		if (!(channel_bit & selected_channels)) {
			continue;
		}

		if (channel_bit & single_ended_mask) {
			if (channel_bit & divide_mask) {
				*sample /= 2;
			} else if (*sample < 0) {
				*sample = 0;
			}
		}
		sample++;
	}
}

/* The internal timer runs at 16 MHz, so to convert the interval in microseconds
 * to the internal timer CC value, we can use the formula:
 * interval_cc = interval_us * 16 MHz
 * where 16 MHz is the frequency of the internal timer.
 *
 * The maximum value for interval_cc is 2047, which corresponds to
 * approximately 7816 Hz ~ 128us.
 * The minimum value for interval_cc is depends on the SoC.
 */
static inline uint16_t interval_to_cc(uint16_t interval_us)
{
	NRFX_ASSERT((interval_us <= ADC_INTERNAL_TIMER_INTERVAL_MAX_US) && (interval_us > 0));

	return (interval_us * 16) - 1;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	nrfx_err_t nrfx_err;
	int error;
	uint32_t selected_channels = sequence->channels;
	nrf_saadc_resolution_t resolution;
	nrf_saadc_oversample_t oversampling;
	uint8_t active_channel_cnt = 0U;
	uint8_t channel_id = 0U;
	void *samples_buffer;

	/* Signal an error if channel selection is invalid (no channels or
	 * a non-existing one is selected).
	 */
	if (!selected_channels ||
	    (selected_channels & ~BIT_MASK(SAADC_CH_NUM))) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	do {
		if (selected_channels & BIT(channel_id)) {
			/* Signal an error if a selected channel has not been configured yet. */
			if (0 == (nrfx_saadc_channels_configured_get() & BIT(channel_id))) {
				LOG_ERR("Channel %u not configured", channel_id);
				return -EINVAL;
			}
			++active_channel_cnt;
		}
	} while (++channel_id < SAADC_CH_NUM);

	if (active_channel_cnt == 0) {
		LOG_ERR("No channel configured");
		return -EINVAL;
	}

	error = get_resolution(sequence, &resolution);
	if (error) {
		return error;
	}

	error = get_oversampling(sequence, active_channel_cnt, &oversampling);
	if (error) {
		return error;
	}

	if ((active_channel_cnt == 1) && (sequence->options != NULL) &&
	    (sequence->options->callback == NULL) &&
	    (sequence->options->interval_us <= ADC_INTERNAL_TIMER_INTERVAL_MAX_US) &&
	    (sequence->options->interval_us > 0)) {

		nrfx_saadc_adv_config_t adv_config = {
			.oversampling = oversampling,
			.burst = NRF_SAADC_BURST_DISABLED,
			.internal_timer_cc = interval_to_cc(sequence->options->interval_us),
			.start_on_end = true,
		};

		m_data.internal_timer_enabled = true;

		nrfx_err = nrfx_saadc_advanced_mode_set(selected_channels, resolution, &adv_config,
							event_handler);
	} else {
		m_data.internal_timer_enabled = false;

		nrfx_err = nrfx_saadc_simple_mode_set(selected_channels, resolution, oversampling,
						      event_handler);
	}

	if (nrfx_err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	error = check_buffer_size(sequence, active_channel_cnt);
	if (error) {
		return error;
	}

	m_data.active_channel_cnt = active_channel_cnt;
	m_data.user_buffer = sequence->buffer;

	error = dmm_buffer_in_prepare(
		m_data.mem_reg, m_data.user_buffer,
		(m_data.internal_timer_enabled
			 ? NRFX_SAADC_SAMPLES_TO_BYTES(1 + sequence->options->extra_samplings)
			 : NRFX_SAADC_SAMPLES_TO_BYTES(active_channel_cnt)),
		&samples_buffer);
	if (error != 0) {
		LOG_ERR("DMM buffer allocation failed err=%d", error);
		return error;
	}

	/* Buffer is filled in chunks, each chunk composed of number of samples equal to number
	 * of active channels. Buffer pointer is advanced and reloaded after each chunk.
	 */
	nrfx_err = nrfx_saadc_buffer_set(
		samples_buffer,
		(m_data.internal_timer_enabled
			 ? (1 + sequence->options->extra_samplings)
			 : active_channel_cnt));
	if (nrfx_err != NRFX_SUCCESS) {
		LOG_ERR("Failed to set buffer: 0x%08x", nrfx_err);
		return -EINVAL;
	}

	adc_context_start_read(&m_data.ctx, sequence);

	return adc_context_wait_for_completion(&m_data.ctx);
}

/* Implementation of the ADC driver API function: adc_read. */
static int adc_nrfx_read(const struct device *dev,
			 const struct adc_sequence *sequence)
{
	int error;

	adc_context_lock(&m_data.ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&m_data.ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
/* Implementation of the ADC driver API function: adc_read_async. */
static int adc_nrfx_read_async(const struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	int error;

	adc_context_lock(&m_data.ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&m_data.ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static void event_handler(const nrfx_saadc_evt_t *event)
{
	nrfx_err_t err;

	if (event->type == NRFX_SAADC_EVT_DONE) {
		dmm_buffer_in_release(
			m_data.mem_reg, m_data.user_buffer,
			(m_data.internal_timer_enabled
				 ? NRFX_SAADC_SAMPLES_TO_BYTES(
					   1 + m_data.ctx.sequence.options->extra_samplings)
				 : NRFX_SAADC_SAMPLES_TO_BYTES(m_data.active_channel_cnt)),
			event->data.done.p_buffer);

		if (has_single_ended(&m_data.ctx.sequence)) {
			correct_single_ended(&m_data.ctx.sequence, m_data.user_buffer,
					     event->data.done.size);
		}
		nrfy_saadc_disable(NRF_SAADC);
		adc_context_on_sampling_done(&m_data.ctx, DEVICE_DT_INST_GET(0));
	} else if (event->type == NRFX_SAADC_EVT_CALIBRATEDONE) {
		err = nrfx_saadc_mode_trigger();
		if (err != NRFX_SUCCESS) {
			LOG_ERR("Cannot start sampling: 0x%08x", err);
			adc_context_complete(&m_data.ctx, -EIO);
		}
	} else if (event->type == NRFX_SAADC_EVT_FINISHED) {
		adc_context_complete(&m_data.ctx, 0);
	}
}

static int init_saadc(const struct device *dev)
{
	nrfx_err_t err;

	k_timer_init(&m_data.timer, external_timer_expired_handler, NULL);

	/* The priority value passed here is ignored (see nrfx_glue.h). */
	err = nrfx_saadc_init(0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to initialize device: %s", dev->name);
		return -EIO;
	}

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), nrfx_isr, nrfx_saadc_irq_handler, 0);

	adc_context_unlock_unconditionally(&m_data.ctx);

	return 0;
}

static DEVICE_API(adc, adc_nrfx_driver_api) = {
	.channel_setup = adc_nrfx_channel_setup,
	.read          = adc_nrfx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_nrfx_read_async,
#endif
#if defined(NRF54LV10A_ENGA_XXAA)
	.ref_internal  = 1300,
#elif defined(CONFIG_SOC_COMPATIBLE_NRF54LX)
	.ref_internal  = 900,
#elif defined(CONFIG_NRF_PLATFORM_HALTIUM)
	.ref_internal  = 1024,
#else
	.ref_internal  = 600,
#endif
};

#if defined(CONFIG_NRF_PLATFORM_HALTIUM)
/* AIN8-AIN14 inputs are on 3v3 GPIO port and they cannot be mixed with other
 * analog inputs (from 1v8 ports) in differential mode.
 */
#define CH_IS_3V3(val) (val >= NRF_SAADC_AIN8)

#define MIXED_3V3_1V8_INPUTS(node)					\
	(DT_NODE_HAS_PROP(node, zephyr_input_negative) &&		\
	 (CH_IS_3V3(DT_PROP_OR(node, zephyr_input_negative, 0)) !=	\
	  CH_IS_3V3(DT_PROP_OR(node, zephyr_input_positive, 0))))
#else
#define MIXED_3V3_1V8_INPUTS(node) false
#endif

#define VALIDATE_CHANNEL_CONFIG(node)					\
	BUILD_ASSERT(MIXED_3V3_1V8_INPUTS(node) == false,		\
		     "1v8 inputs cannot be mixed with 3v3 inputs");

/* Validate configuration of all channels. */
DT_FOREACH_CHILD(DT_DRV_INST(0), VALIDATE_CHANNEL_CONFIG)

NRF_DT_CHECK_NODE_HAS_REQUIRED_MEMORY_REGIONS(DT_DRV_INST(0));

DEVICE_DT_INST_DEFINE(0, init_saadc, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_ADC_INIT_PRIORITY, &adc_nrfx_driver_api);
