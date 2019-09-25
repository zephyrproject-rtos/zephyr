/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include <hal/nrf_saadc.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_nrfx_saadc);

struct driver_data {
	struct adc_context ctx;

	u8_t positive_inputs[NRF_SAADC_CHANNEL_COUNT];
};

static struct driver_data m_data = {
	ADC_CONTEXT_INIT_TIMER(m_data, ctx),
	ADC_CONTEXT_INIT_LOCK(m_data, ctx),
	ADC_CONTEXT_INIT_SYNC(m_data, ctx),
};


/* Implementation of the ADC driver API function: adc_channel_setup. */
static int adc_nrfx_channel_setup(struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	nrf_saadc_channel_config_t config = {
		.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
		.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
		.burst      = NRF_SAADC_BURST_DISABLED,
	};
	u8_t channel_id = channel_cfg->channel_id;

	if (channel_id >= NRF_SAADC_CHANNEL_COUNT) {
		return -EINVAL;
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_6:
		config.gain = NRF_SAADC_GAIN1_6;
		break;
	case ADC_GAIN_1_5:
		config.gain = NRF_SAADC_GAIN1_5;
		break;
	case ADC_GAIN_1_4:
		config.gain = NRF_SAADC_GAIN1_4;
		break;
	case ADC_GAIN_1_3:
		config.gain = NRF_SAADC_GAIN1_3;
		break;
	case ADC_GAIN_1_2:
		config.gain = NRF_SAADC_GAIN1_2;
		break;
	case ADC_GAIN_1:
		config.gain = NRF_SAADC_GAIN1;
		break;
	case ADC_GAIN_2:
		config.gain = NRF_SAADC_GAIN2;
		break;
	case ADC_GAIN_4:
		config.gain = NRF_SAADC_GAIN4;
		break;
	default:
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		config.reference = NRF_SAADC_REFERENCE_INTERNAL;
		break;
	case ADC_REF_VDD_1_4:
		config.reference = NRF_SAADC_REFERENCE_VDD4;
		break;
	default:
		LOG_ERR("Selected ADC reference is not valid");
		return -EINVAL;
	}

	switch (channel_cfg->acquisition_time) {
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 3):
		config.acq_time = NRF_SAADC_ACQTIME_3US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 5):
		config.acq_time = NRF_SAADC_ACQTIME_5US;
		break;
	case ADC_ACQ_TIME_DEFAULT:
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10):
		config.acq_time = NRF_SAADC_ACQTIME_10US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 15):
		config.acq_time = NRF_SAADC_ACQTIME_15US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 20):
		config.acq_time = NRF_SAADC_ACQTIME_20US;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40):
		config.acq_time = NRF_SAADC_ACQTIME_40US;
		break;
	default:
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	config.mode = (channel_cfg->differential ?
		NRF_SAADC_MODE_DIFFERENTIAL : NRF_SAADC_MODE_SINGLE_ENDED);

	/* Keep the channel disabled in hardware (set positive input to
	 * NRF_SAADC_INPUT_DISABLED) until it is selected to be included
	 * in a sampling sequence.
	 */
	config.pin_p = NRF_SAADC_INPUT_DISABLED;
	config.pin_n = channel_cfg->input_negative;

	nrf_saadc_channel_init(channel_id, &config);

	/* Store the positive input selection in a dedicated array,
	 * to get it later when the channel is selected for a sampling
	 * and to mark the channel as configured (ready to be selected).
	 */
	m_data.positive_inputs[channel_id] = channel_cfg->input_positive;

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	nrf_saadc_enable();

	if (ctx->sequence.calibrate) {
		nrf_saadc_task_trigger(NRF_SAADC_TASK_CALIBRATEOFFSET);
	} else {
		nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
		nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	ARG_UNUSED(ctx);

	if (!repeat) {
		nrf_saadc_buffer_pointer_set(
			nrf_saadc_buffer_pointer_get() +
			nrf_saadc_amount_get());
	}
}

static int set_resolution(const struct adc_sequence *sequence)
{
	nrf_saadc_resolution_t nrf_resolution;

	switch (sequence->resolution) {
	case  8:
		nrf_resolution = NRF_SAADC_RESOLUTION_8BIT;
		break;
	case 10:
		nrf_resolution = NRF_SAADC_RESOLUTION_10BIT;
		break;
	case 12:
		nrf_resolution = NRF_SAADC_RESOLUTION_12BIT;
		break;
	case 14:
		nrf_resolution = NRF_SAADC_RESOLUTION_14BIT;
		break;
	default:
		LOG_ERR("ADC resolution value %d is not valid",
			    sequence->resolution);
		return -EINVAL;
	}

	nrf_saadc_resolution_set(nrf_resolution);
	return 0;
}

static int set_oversampling(const struct adc_sequence *sequence,
			    u8_t active_channels)
{
	nrf_saadc_oversample_t nrf_oversampling;

	if ((active_channels > 1) && (sequence->oversampling > 0)) {
		LOG_ERR(
			"Oversampling is supported for single channel only");
		return -EINVAL;
	}

	switch (sequence->oversampling) {
	case 0:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_DISABLED;
		break;
	case 1:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_2X;
		break;
	case 2:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_4X;
		break;
	case 3:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_8X;
		break;
	case 4:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_16X;
		break;
	case 5:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_32X;
		break;
	case 6:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_64X;
		break;
	case 7:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_128X;
		break;
	case 8:
		nrf_oversampling = NRF_SAADC_OVERSAMPLE_256X;
		break;
	default:
		LOG_ERR("Oversampling value %d is not valid",
			    sequence->oversampling);
		return -EINVAL;
	}

	nrf_saadc_oversample_set(nrf_oversampling);
	return 0;
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     u8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(nrf_saadc_value_t);
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

static int start_read(struct device *dev, const struct adc_sequence *sequence)
{
	int error;
	u32_t selected_channels = sequence->channels;
	u8_t active_channels;
	u8_t channel_id;

	/* Signal an error if channel selection is invalid (no channels or
	 * a non-existing one is selected).
	 */
	if (!selected_channels ||
	    (selected_channels & ~BIT_MASK(NRF_SAADC_CHANNEL_COUNT))) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	active_channels = 0U;

	/* Enable only the channels selected for the pointed sequence.
	 * Disable all the rest.
	 */
	channel_id = 0U;
	do {
		if (selected_channels & BIT(channel_id)) {
			/* Signal an error if a selected channel has not been
			 * configured yet.
			 */
			if (m_data.positive_inputs[channel_id] == 0U) {
				LOG_ERR("Channel %u not configured",
					    channel_id);
				return -EINVAL;
			}
			/* When oversampling is used, the burst mode needs to
			 * be activated. Unfortunately, this mode cannot be
			 * activated permanently in the channel setup, because
			 * then the multiple channel sampling fails (the END
			 * event is not generated) after switching to a single
			 * channel sampling and back. Thus, when oversampling
			 * is not used (hence, the multiple channel sampling is
			 * possible), the burst mode have to be deactivated.
			 */
			nrf_saadc_burst_set(channel_id,
				(sequence->oversampling != 0U ?
					NRF_SAADC_BURST_ENABLED :
					NRF_SAADC_BURST_DISABLED));
			nrf_saadc_channel_pos_input_set(
				channel_id,
				m_data.positive_inputs[channel_id]);
			++active_channels;
		} else {
			nrf_saadc_channel_pos_input_set(
				channel_id,
				NRF_SAADC_INPUT_DISABLED);
		}
	} while (++channel_id < NRF_SAADC_CHANNEL_COUNT);

	error = set_resolution(sequence);
	if (error) {
		return error;
	}

	error = set_oversampling(sequence, active_channels);
	if (error) {
		return error;
	}

	error = check_buffer_size(sequence, active_channels);
	if (error) {
		return error;
	}

	nrf_saadc_buffer_init((nrf_saadc_value_t *)sequence->buffer,
			      active_channels);

	adc_context_start_read(&m_data.ctx, sequence);

	error = adc_context_wait_for_completion(&m_data.ctx);
	return error;
}

/* Implementation of the ADC driver API function: adc_read. */
static int adc_nrfx_read(struct device *dev,
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
static int adc_nrfx_read_async(struct device *dev,
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

static void saadc_irq_handler(void *param)
{
	struct device *dev = (struct device *)param;

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_END)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

		nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);
		nrf_saadc_disable();

		adc_context_on_sampling_done(&m_data.ctx, dev);
	} else if (nrf_saadc_event_check(NRF_SAADC_EVENT_CALIBRATEDONE)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_CALIBRATEDONE);

		/*
		 * The workaround for Nordic nRF52832 anomalies 86 and
		 * 178 is an explicit STOP after CALIBRATEOFFSET
		 * before issuing START.
		 */
		nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);
		nrf_saadc_task_trigger(NRF_SAADC_TASK_START);
		nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
	}
}

DEVICE_DECLARE(adc_0);

static int init_saadc(struct device *dev)
{
	nrf_saadc_event_clear(NRF_SAADC_EVENT_END);
	nrf_saadc_event_clear(NRF_SAADC_EVENT_CALIBRATEDONE);
	nrf_saadc_int_enable(NRF_SAADC_INT_END
			     | NRF_SAADC_INT_CALIBRATEDONE);
	NRFX_IRQ_ENABLE(DT_NORDIC_NRF_SAADC_ADC_0_IRQ_0);

	IRQ_CONNECT(DT_NORDIC_NRF_SAADC_ADC_0_IRQ_0,
		    DT_NORDIC_NRF_SAADC_ADC_0_IRQ_0_PRIORITY,
		    saadc_irq_handler, DEVICE_GET(adc_0), 0);

	adc_context_unlock_unconditionally(&m_data.ctx);

	return 0;
}

static const struct adc_driver_api adc_nrfx_driver_api = {
	.channel_setup = adc_nrfx_channel_setup,
	.read          = adc_nrfx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_nrfx_read_async,
#endif
	.ref_internal  = 600,
};

#ifdef CONFIG_ADC_0
DEVICE_AND_API_INIT(adc_0, DT_NORDIC_NRF_SAADC_ADC_0_LABEL,
		    init_saadc, NULL, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adc_nrfx_driver_api);
#endif /* CONFIG_ADC_0 */
