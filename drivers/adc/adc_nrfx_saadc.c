/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include <nrfx_saadc.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_nrfx_saadc);

#define DT_DRV_COMPAT nordic_nrf_saadc

struct driver_data {
	struct adc_context ctx;
	const struct device *dev;

	bool configured[SAADC_CH_NUM];
	const nrfx_saadc_evt_t *event;
};

static struct driver_data m_data = {
	ADC_CONTEXT_INIT_TIMER(m_data, ctx),
	ADC_CONTEXT_INIT_LOCK(m_data, ctx),
	ADC_CONTEXT_INIT_SYNC(m_data, ctx),
};

/* Forward declaration */
static void irq_nrfx_event_handler(nrfx_saadc_evt_t const *event);

/* return number of initialized channelse. This is workaround for nrfx API
 * which force to configure all active channels at once.
 */
static uint8_t channels_cnt_get(uint32_t channel_mask)
{
	uint8_t cnt = 0;

	for (uint8_t i = 0; i < SAADC_CH_NUM; i++) {
		if (!(channel_mask & BIT(i))) {
			continue;
		}
		if (m_data.configured[i]) {
			cnt++;
		}
	}

	return cnt;
}

static int pin_assign(const struct adc_channel_cfg *channel_cfg,
		      nrfx_saadc_channel_t *drv_cfg)
{
	nrf_saadc_input_t analog_pin = channel_cfg->input_positive;

	if (analog_pin > NRF_SAADC_INPUT_VDD) {
		LOG_ERR("Invalid analog positive pin number: %d", analog_pin);
		return -EINVAL;
	} else if (analog_pin == NRF_SAADC_INPUT_DISABLED) {
		LOG_ERR("Analog positive pin not configured.");
		return -EINVAL;
	}
	drv_cfg->pin_p = analog_pin;

	if (channel_cfg->differential) {
		analog_pin = channel_cfg->input_negative;
		if (analog_pin > NRF_SAADC_INPUT_VDD) {
			LOG_ERR("Invalid analog negative pin number: %d",
				analog_pin);
			return -EINVAL;
		} else if (analog_pin == NRF_SAADC_INPUT_DISABLED) {
			LOG_ERR("Analog negative pin not configured.");
			return -EINVAL;
		}
		drv_cfg->pin_n = analog_pin;
	}

	return 0;
}

/* Implementation of the ADC driver API function: adc_channel_setup. */
static int adc_nrfx_channel_setup(const struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;
	nrf_saadc_channel_config_t *ch_cfg;
	nrfx_saadc_channel_t cfg;
	int err;

	if (channel_id >= SAADC_CH_NUM) {
		return -EINVAL;
	}

	ch_cfg = &cfg.channel_config;

	err = pin_assign(channel_cfg, &cfg);
	if (err != 0) {
		return -EINVAL;
	}

	cfg.channel_index = channel_cfg->channel_id;
	ch_cfg->resistor_p = NRF_SAADC_RESISTOR_DISABLED;
	ch_cfg->resistor_n = NRF_SAADC_RESISTOR_DISABLED;
	ch_cfg->burst = NRF_SAADC_BURST_DISABLED;

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_6:
		ch_cfg->gain = NRF_SAADC_GAIN1_6;
		break;
	case ADC_GAIN_1_5:
		ch_cfg->gain = NRF_SAADC_GAIN1_5;
		break;
	case ADC_GAIN_1_4:
		ch_cfg->gain = NRF_SAADC_GAIN1_4;
		break;
	case ADC_GAIN_1_3:
		ch_cfg->gain = NRF_SAADC_GAIN1_3;
		break;
	case ADC_GAIN_1_2:
		ch_cfg->gain = NRF_SAADC_GAIN1_2;
		break;
	case ADC_GAIN_1:
		ch_cfg->gain = NRF_SAADC_GAIN1;
		break;
	case ADC_GAIN_2:
		ch_cfg->gain = NRF_SAADC_GAIN2;
		break;
	case ADC_GAIN_4:
		ch_cfg->gain = NRF_SAADC_GAIN4;
		break;
	default:
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		ch_cfg->reference = NRF_SAADC_REFERENCE_INTERNAL;
		break;
	case ADC_REF_VDD_1_4:
		ch_cfg->reference = NRF_SAADC_REFERENCE_VDD4;
		break;
	default:
		LOG_ERR("Selected ADC reference is not valid");
		return -EINVAL;
	}

	switch (channel_cfg->acquisition_time) {
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
	case ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 40):
		ch_cfg->acq_time = NRF_SAADC_ACQTIME_40US;
		break;
	default:
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	ch_cfg->mode = (channel_cfg->differential ?
		NRF_SAADC_MODE_DIFFERENTIAL : NRF_SAADC_MODE_SINGLE_ENDED);

	nrfx_saadc_channel_config(&cfg);
	/* Mark channel as configured (reaady to be slected) for a sampling. */
	m_data.configured[channel_id] = true;

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	nrfx_err_t err;

	err = nrfx_saadc_mode_trigger();

	if (err != NRFX_SUCCESS) {
		LOG_ERR("Cannot start sampling: %d", err);
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	nrf_saadc_value_t *buffer;
	uint32_t buffer_size;

	ARG_UNUSED(ctx);

	if (repeat) {
		buffer = m_data.event->data.done.p_buffer;
		buffer_size = m_data.event->data.done.size;
	} else {
		buffer = m_data.event->data.done.p_buffer +
			 m_data.event->data.done.size;
		buffer_size = m_data.event->data.done.size;
	}

	nrfx_saadc_buffer_set(buffer, buffer_size);
}

static int get_resolution(const struct adc_sequence *sequence,
			  nrf_saadc_resolution_t *resolution)
{
	switch (sequence->resolution) {
	case  8:
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
		LOG_ERR("ADC resolution value %d is not valid",
			    sequence->resolution);
		return -EINVAL;
	}

	return 0;
}

static int get_oversampling(const struct adc_sequence *sequence,
			    uint8_t active_channels,
			    nrf_saadc_oversample_t *oversampling)
{
	if ((active_channels > 1) && (sequence->oversampling > 0)) {
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
		LOG_ERR("Oversampling value %d is not valid",
			    sequence->oversampling);
		return -EINVAL;
	}

	return 0;
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	uint32_t needed_buffer_size;

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

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	uint32_t selected_channels = sequence->channels;
	nrf_saadc_oversample_t oversampling;
	nrf_saadc_resolution_t resolution;
	nrfx_err_t nrfx_err;
	int channels_cnt;
	int error;

	/* Signal an error if channel selection is invalid (no channels or
	 * a non-existing one is selected).
	 */
	if (!selected_channels ||
	    (selected_channels & ~BIT_MASK(SAADC_CH_NUM))) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	/* Prepare configuration array for nrfx driver */
	channels_cnt = channels_cnt_get(selected_channels);
	if (channels_cnt == 0) {
		LOG_ERR("No channel configured");
		return -EINVAL;
	}

	error = get_resolution(sequence, &resolution);
	if (error) {
		return error;
	}

	error = get_oversampling(sequence, channels_cnt, &oversampling);
	if (error) {
		return -EINVAL;
	}

	nrfx_err = nrfx_saadc_simple_mode_set(sequence->channels,
					      resolution,
					      oversampling,
					      irq_nrfx_event_handler);
	if (nrfx_err != NRFX_SUCCESS) {
		return -EINVAL;
	}

	error = check_buffer_size(sequence, channels_cnt);
	if (error) {
		return error;
	}
	/* Function needs buffer pointer and number of samples not buffer
	 * size.
	 */
	nrfx_saadc_buffer_set(sequence->buffer, channels_cnt);

	adc_context_start_read(&m_data.ctx, sequence);

	error = adc_context_wait_for_completion(&m_data.ctx);
	return error;
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

static void saadc_irq_handler(const struct device *dev)
{
	nrfx_saadc_evt_type_t event;
	nrfx_err_t err;

	event = m_data.event->type;

	if (event == NRFX_SAADC_EVT_DONE) {
		adc_context_on_sampling_done(&m_data.ctx, dev);
	} else if (event == NRFX_SAADC_EVT_CALIBRATEDONE) {
		err = nrfx_saadc_mode_trigger();
		if (err != NRFX_SUCCESS) {
			LOG_ERR("Cannot start sampling: %d", err);
		}

	}
}

static void irq_nrfx_event_handler(nrfx_saadc_evt_t const *event)
{
	m_data.event = event;
	saadc_irq_handler(m_data.dev);
}

static int init_saadc(const struct device *dev)
{
	nrfx_err_t err;

	err = nrfx_saadc_init(DT_INST_IRQ(0, priority));
	if (err != NRFX_SUCCESS) {
		return -EIO;
	}

	m_data.dev = dev;

	IRQ_CONNECT(NRFX_IRQ_NUMBER_GET(NRF_SAADC),
		    DT_INST_IRQ(0, priority),
		    nrfx_isr, nrfx_saadc_irq_handler, 0);

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

/*
 * There is only one instance on supported SoCs, so inst is guaranteed
 * to be 0 if any instance is okay. (We use adc_0 above, so the driver
 * is relying on the numeric instance value in a way that happens to
 * be safe.)
 *
 * Just in case that assumption becomes invalid in the future, we use
 * a BUILD_ASSERT().
 */
#define SAADC_INIT(inst)						\
	BUILD_ASSERT((inst) == 0,					\
		     "multiple instances not supported");		\
	DEVICE_DT_INST_DEFINE(0,					\
			    init_saadc,					\
			    device_pm_control_nop,			\
			    NULL,					\
			    NULL,					\
			    POST_KERNEL,				\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &adc_nrfx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SAADC_INIT)
