/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include <nrfx_adc.h>

#define SYS_LOG_DOMAIN "adc_nrfx_adc"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ADC_LEVEL
#include <logging/sys_log.h>

struct driver_data {
	struct adc_context ctx;

	nrf_adc_value_t *buffer;
	u8_t active_channels;
};

static struct driver_data m_data = {
	ADC_CONTEXT_INIT_TIMER(m_data, ctx),
	ADC_CONTEXT_INIT_LOCK(m_data, ctx),
	ADC_CONTEXT_INIT_SYNC(m_data, ctx),
};

static nrfx_adc_channel_t m_channels[CONFIG_ADC_NRFX_ADC_CHANNEL_COUNT];


/* Implementation of the ADC driver API function: adc_channel_setup. */
static int adc_nrfx_channel_setup(struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	u8_t channel_id = channel_cfg->channel_id;
	nrf_adc_config_t *config = &m_channels[channel_id].config;

	if (channel_id >= CONFIG_ADC_NRFX_ADC_CHANNEL_COUNT) {
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		SYS_LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		SYS_LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_3:
		config->scaling = NRF_ADC_CONFIG_SCALING_INPUT_ONE_THIRD;
		break;
	case ADC_GAIN_2_3:
		config->scaling = NRF_ADC_CONFIG_SCALING_INPUT_TWO_THIRDS;
		break;
	case ADC_GAIN_1:
		config->scaling = NRF_ADC_CONFIG_SCALING_INPUT_FULL_SCALE;
		break;
	default:
		SYS_LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		config->reference = NRF_ADC_CONFIG_REF_VBG;
		config->extref    = NRF_ADC_CONFIG_EXTREFSEL_NONE;
		break;
	case ADC_REF_VDD_1_2:
		config->reference = NRF_ADC_CONFIG_REF_SUPPLY_ONE_HALF;
		config->extref    = NRF_ADC_CONFIG_EXTREFSEL_NONE;
		break;
	case ADC_REF_VDD_1_3:
		config->reference = NRF_ADC_CONFIG_REF_SUPPLY_ONE_THIRD;
		config->extref    = NRF_ADC_CONFIG_EXTREFSEL_NONE;
		break;
	case ADC_REF_EXTERNAL0:
		config->reference = NRF_ADC_CONFIG_REF_EXT;
		config->extref    = NRF_ADC_CONFIG_EXTREFSEL_AREF0;
		break;
	case ADC_REF_EXTERNAL1:
		config->reference = NRF_ADC_CONFIG_REF_EXT;
		config->extref    = NRF_ADC_CONFIG_EXTREFSEL_AREF1;
		break;
	default:
		SYS_LOG_ERR("Selected ADC reference is not valid");
		return -EINVAL;
	}

	config->input = channel_cfg->input_positive;

	config->resolution = NRF_ADC_CONFIG_RES_8BIT;

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	ARG_UNUSED(ctx);

	nrfx_adc_buffer_convert(m_data.buffer, m_data.active_channels);
	nrfx_adc_sample();
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	ARG_UNUSED(ctx);

	if (!repeat) {
		m_data.buffer += m_data.active_channels;
	}
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     u8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(nrf_adc_value_t);
	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		SYS_LOG_ERR("Provided buffer is too small (%u/%u)",
				sequence->buffer_size, needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int start_read(struct device *dev, const struct adc_sequence *sequence)
{
	int error = 0;
	u32_t selected_channels = sequence->channels;
	u8_t active_channels;
	u8_t channel_id;
	nrf_adc_config_resolution_t nrf_resolution;

	/* Signal an error if channel selection is invalid (no channels or
	 * a non-existing one is selected).
	 */
	if (!selected_channels ||
	    (selected_channels &
			~BIT_MASK(CONFIG_ADC_NRFX_ADC_CHANNEL_COUNT))) {
		SYS_LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		SYS_LOG_ERR("Oversampling is not supported");
		return -EINVAL;
	}

	switch (sequence->resolution) {
	case  8:
		nrf_resolution = NRF_ADC_CONFIG_RES_8BIT;
		break;
	case  9:
		nrf_resolution = NRF_ADC_CONFIG_RES_9BIT;
		break;
	case 10:
		nrf_resolution = NRF_ADC_CONFIG_RES_10BIT;
		break;
	default:
		SYS_LOG_ERR("ADC resolution value %d is not valid",
			    sequence->resolution);
		return -EINVAL;
	}

	active_channels = 0;
	nrfx_adc_all_channels_disable();

	/* Enable the channels selected for the pointed sequence.
	 */
	channel_id = 0;
	while (selected_channels) {
		if (selected_channels & BIT(0)) {
			/* The nrfx driver requires setting the resolution
			 * for each enabled channel individually.
			 */
			m_channels[channel_id].config.resolution =
				nrf_resolution;
			nrfx_adc_channel_enable(&m_channels[channel_id]);
			++active_channels;
		}
		selected_channels >>= 1;
		++channel_id;
	}

	error = check_buffer_size(sequence, active_channels);
	if (error) {
		return error;
	}

	m_data.buffer = sequence->buffer;
	m_data.active_channels = active_channels;

	adc_context_start_read(&m_data.ctx, sequence);

	if (!error) {
		error = adc_context_wait_for_completion(&m_data.ctx);
		adc_context_release(&m_data.ctx, error);
	}

	return error;
}

/* Implementation of the ADC driver API function: adc_read. */
static int adc_nrfx_read(struct device *dev,
			 const struct adc_sequence *sequence)
{
	adc_context_lock(&m_data.ctx, false, NULL);
	return start_read(dev, sequence);
}

#ifdef CONFIG_ADC_ASYNC
/* Implementation of the ADC driver API function: adc_read_sync. */
static int adc_nrfx_read_async(struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{
	adc_context_lock(&m_data.ctx, true, async);
	return start_read(dev, sequence);
}
#endif

DEVICE_DECLARE(adc_0);

static void event_handler(const nrfx_adc_evt_t *p_event)
{
	struct device *dev = DEVICE_GET(adc_0);

	if (p_event->type == NRFX_ADC_EVT_DONE) {
		adc_context_on_sampling_done(&m_data.ctx, dev);
	}
}

static int init_adc(struct device *dev)
{
	const nrfx_adc_config_t config = NRFX_ADC_DEFAULT_CONFIG;

	nrfx_err_t result = nrfx_adc_init(&config, event_handler);

	if (result != NRFX_SUCCESS) {
		SYS_LOG_ERR("Failed to initialize device: %s",
			    dev->config->name);
		return -EBUSY;
	}

	IRQ_CONNECT(CONFIG_ADC_0_IRQ, CONFIG_ADC_0_IRQ_PRI,
		    nrfx_isr, nrfx_adc_irq_handler, 0);

	adc_context_unlock_unconditionally(&m_data.ctx);

	return 0;
}

static const struct adc_driver_api adc_nrfx_driver_api = {
	.channel_setup = adc_nrfx_channel_setup,
	.read          = adc_nrfx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_nrfx_read_async,
#endif
};

#ifdef CONFIG_ADC_0
DEVICE_AND_API_INIT(adc_0, CONFIG_ADC_0_NAME,
		    init_adc, NULL, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adc_nrfx_driver_api);
#endif /* CONFIG_ADC_0 */
