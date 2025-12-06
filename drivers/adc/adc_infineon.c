/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief ADC driver for Infineon CAT1 MCU family.
 */

#define DT_DRV_COMPAT infineon_cat1_adc

#include <zephyr/drivers/adc.h>
#include <cyhal_adc.h>
#include <cyhal_utils_impl.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ifx_cat1_adc, CONFIG_ADC_LOG_LEVEL);

#if defined(PASS_SARMUX_PADS0_PORT)
#define _ADC_PORT PASS_SARMUX_PADS0_PORT
#elif defined(ADCMIC_GPIO_ADC_IN0_PORT)
#define _ADC_PORT ADCMIC_GPIO_ADC_IN0_PORT
#else
#error The selected device does not supported ADC
#endif

#define ADC_CAT1_EVENTS_MASK (CYHAL_ADC_EOS | CYHAL_ADC_ASYNC_READ_COMPLETE)

#define ADC_CAT1_DEFAULT_ACQUISITION_NS (1000u)
#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1A)
#define ADC_CAT1_RESOLUTION             (12u)
#define ADC_CAT1_REF_INTERNAL_MV        (1200u)
#elif defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
#define ADC_CAT1_RESOLUTION             (16u)
#define ADC_CAT1_REF_INTERNAL_MV        (3600u)
#endif

#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
#define IFX_ADC_NUM_CHANNELS                                                                       \
	ARRAY_SIZE(cyhal_pin_map_adcmic_gpio_adc_in)
#else
#define IFX_ADC_NUM_CHANNELS CY_SAR_SEQ_NUM_CHANNELS
#endif

struct ifx_cat1_adc_data {
	struct adc_context ctx;
	const struct device *dev;
	cyhal_adc_t adc_obj;
	cyhal_adc_channel_t adc_chan_obj[IFX_ADC_NUM_CHANNELS];
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t channels_mask;
#ifdef CONFIG_SOC_FAMILY_INFINEON_CAT1B
	struct k_work adc_worker_thread;
#endif
};

struct ifx_cat1_adc_config {
	uint8_t irq_priority;
};

#ifdef CONFIG_SOC_FAMILY_INFINEON_CAT1B
static void ifx_cat1_adc_worker(struct k_work *adc_worker_thread)
{
	struct ifx_cat1_adc_data *data =
		CONTAINER_OF(adc_worker_thread, struct ifx_cat1_adc_data, adc_worker_thread);

	uint32_t channels = data->channels;
	int32_t result;
	uint32_t channel_id;

	while (channels != 0) {
		channel_id = find_lsb_set(channels) - 1;
		channels &= ~BIT(channel_id);

		result = cyhal_adc_read(&data->adc_chan_obj[channel_id]);
		/* Legacy API for BWC. Convert from signed to unsigned by adding 0x800 to
		 * convert the lowest signed 12-bit number to 0x0.
		 */
		*data->buffer = (uint16_t)(result + 0x800);
		data->buffer++;
	}
	adc_context_on_sampling_done(&data->ctx, data->dev);
}
#else
static void _cyhal_adc_event_callback(void *callback_arg, cyhal_adc_event_t event)
{
	const struct device *dev = (const struct device *) callback_arg;
	struct ifx_cat1_adc_data *data = dev->data;
	uint32_t channels = data->channels;
	int32_t result;
	uint32_t channel_id;

	while (channels != 0) {
		channel_id = find_lsb_set(channels) - 1;
		channels &= ~BIT(channel_id);

		result = Cy_SAR_GetResult32(data->adc_chan_obj[channel_id].adc->base,
					    data->adc_chan_obj[channel_id].channel_idx);
		/* Legacy API for BWC. Convert from signed to unsigned by adding 0x800 to
		 * convert the lowest signed 12-bit number to 0x0.
		 */
		*data->buffer = (uint16_t)(result + 0x800);
		data->buffer++;
	}

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("%s ISR triggered.", dev->name);
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ifx_cat1_adc_data *data = CONTAINER_OF(ctx, struct ifx_cat1_adc_data, ctx);

	data->repeat_buffer = data->buffer;

#if defined(CONFIG_SOC_FAMILY_INFINEON_CAT1B)
	k_work_submit(&data->adc_worker_thread);
#else
	Cy_SAR_StartConvert(data->adc_obj.base, CY_SAR_START_CONVERT_SINGLE_SHOT);
#endif
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct ifx_cat1_adc_data *data = CONTAINER_OF(ctx, struct ifx_cat1_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int ifx_cat1_adc_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	struct ifx_cat1_adc_data *data = dev->data;
	cy_rslt_t result;

	cyhal_gpio_t vplus = CYHAL_GET_GPIO(_ADC_PORT, channel_cfg->input_positive);
	cyhal_gpio_t vminus = channel_cfg->differential
				      ? CYHAL_GET_GPIO(_ADC_PORT, channel_cfg->input_negative)
				      : CYHAL_ADC_VNEG;
	uint32_t acquisition_ns = ADC_CAT1_DEFAULT_ACQUISITION_NS;

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Selected ADC reference is not valid");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		switch (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time)) {
		case ADC_ACQ_TIME_MICROSECONDS:
			acquisition_ns = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time) * 1000;
			break;
		case ADC_ACQ_TIME_NANOSECONDS:
			acquisition_ns = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
			break;
		default:
			LOG_ERR("Selected ADC acquisition time units is not valid");
			return -EINVAL;
		}
	}

	/* ADC channel configuration */
	const cyhal_adc_channel_config_t channel_config = {
		/* Disable averaging for channel */
		.enable_averaging = false,
		/* Minimum acquisition time set to 1us */
		.min_acquisition_ns = acquisition_ns,
		/* Sample channel when ADC performs a scan */
		.enabled = true
	};

	/* Initialize a channel and configure it to scan the input pin(s). */
	cyhal_adc_channel_free(&data->adc_chan_obj[channel_cfg->channel_id]);
	result = cyhal_adc_channel_init_diff(&data->adc_chan_obj[channel_cfg->channel_id],
					     &data->adc_obj, vplus, vminus, &channel_config);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("ADC channel initialization failed. Error: 0x%08X\n", (unsigned int)result);
		return -EIO;
	}

	data->channels_mask |= BIT(channel_cfg->channel_id);

	return 0;
}

static int validate_buffer_size(const struct adc_sequence *sequence)
{
	int active_channels = 0;
	int total_buffer_size;

	for (int i = 0; i < IFX_ADC_NUM_CHANNELS; i++) {
		if (sequence->channels & BIT(i)) {
			active_channels++;
		}
	}

	total_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		total_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < total_buffer_size) {
		return -ENOMEM;
	}

	return 0;
}

static int start_read(const struct device *dev,
		      const struct adc_sequence *sequence)
{
	struct ifx_cat1_adc_data *data = dev->data;
	uint32_t channels = sequence->channels;
	uint32_t unconfigured_channels = channels & ~data->channels_mask;

	if (sequence->resolution != ADC_CAT1_RESOLUTION) {
		LOG_ERR("Invalid ADC resolution (%d)", sequence->resolution);
		return -EINVAL;
	}

	if (unconfigured_channels != 0) {
		LOG_ERR("ADC channel(s) not configured: 0x%08X\n", unconfigured_channels);
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -ENOTSUP;
	}

	int return_val = validate_buffer_size(sequence);

	if (return_val < 0) {
		LOG_ERR("Invalid sequence buffer size");
		return return_val;
	}

	data->channels = channels;
	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ifx_cat1_adc_read(const struct device *dev,
			     const struct adc_sequence *sequence)
{
	int ret;
	struct ifx_cat1_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int ifx_cat1_adc_read_async(const struct device *dev,
				   const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	int ret;
	struct ifx_cat1_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	ret = start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static int ifx_cat1_adc_init(const struct device *dev)
{
	struct ifx_cat1_adc_data *data = dev->data;
	const struct ifx_cat1_adc_config *config = dev->config;
	cy_rslt_t result;

	data->dev = dev;

	/* Initialize ADC. The ADC block which can connect to the input pin is selected */
	result = cyhal_adc_init(&data->adc_obj, CYHAL_GET_GPIO(_ADC_PORT, 0), NULL);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("ADC initialization failed. Error: 0x%08X\n", (unsigned int)result);
		return -EIO;
	}

	/* Enable ADC Interrupt */
	cyhal_adc_enable_event(&data->adc_obj, (cyhal_adc_event_t)ADC_CAT1_EVENTS_MASK,
			       config->irq_priority, true);

#ifndef CONFIG_SOC_FAMILY_INFINEON_CAT1B
	cyhal_adc_register_callback(&data->adc_obj, _cyhal_adc_event_callback, (void *) dev);
#endif

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_cat1_driver_api) = {
	.channel_setup = ifx_cat1_adc_channel_setup,
	.read = ifx_cat1_adc_read,
	#ifdef CONFIG_ADC_ASYNC
	.read_async = ifx_cat1_adc_read_async,
	#endif
	.ref_internal = ADC_CAT1_REF_INTERNAL_MV
};
#ifdef CONFIG_SOC_FAMILY_INFINEON_CAT1B
#define ADC_WORKER_THREAD_INIT() .adc_worker_thread = Z_WORK_INITIALIZER(ifx_cat1_adc_worker),
#else
#define ADC_WORKER_THREAD_INIT()
#endif

/* Macros for ADC instance declaration */
#define INFINEON_CAT1_ADC_INIT(n)                                                                  \
	static struct ifx_cat1_adc_data ifx_cat1_adc_data##n = {                                   \
		ADC_CONTEXT_INIT_TIMER(ifx_cat1_adc_data##n, ctx),                                 \
		ADC_CONTEXT_INIT_LOCK(ifx_cat1_adc_data##n, ctx),                                  \
		ADC_CONTEXT_INIT_SYNC(ifx_cat1_adc_data##n, ctx), ADC_WORKER_THREAD_INIT()};       \
                                                                                                   \
	static const struct ifx_cat1_adc_config adc_cat1_cfg_##n = {                               \
		.irq_priority = DT_INST_IRQ(n, priority),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ifx_cat1_adc_init, NULL, &ifx_cat1_adc_data##n,                   \
			      &adc_cat1_cfg_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,            \
			      &adc_cat1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INFINEON_CAT1_ADC_INIT)
