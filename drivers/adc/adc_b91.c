/*
 * Copyright (c) 2022 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT telink_b91_adc

/* Local driver headers */
#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* Zephyr Device Tree headers */
#include <zephyr/dt-bindings/adc/b91-adc.h>

/* Zephyr Logging headers */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_b91, CONFIG_ADC_LOG_LEVEL);

/* Telink HAL headers */
#include <adc.h>

/* ADC B91 defines */
#define SIGN_BIT_POSITION          (13)
#define AREG_ADC_DATA_STATUS       (0xf6)
#define ADC_DATA_READY             BIT(0)

/* B91 ADC driver data */
struct b91_adc_data {
	struct adc_context ctx;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint8_t differential;
	uint8_t resolution_divider;
	struct k_sem acq_sem;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_B91_ACQUISITION_THREAD_STACK_SIZE);
};

struct b91_adc_cfg {
	uint32_t sample_freq;
	uint16_t vref_internal_mv;
};

/* Validate ADC data buffer size */
static int adc_b91_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t needed = sizeof(int16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/* Validate ADC read API input parameters */
static int adc_b91_validate_sequence(const struct adc_sequence *sequence)
{
	int status;

	if (sequence->channels != BIT(0)) {
		LOG_ERR("Only channel 0 is supported.");
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling is not supported.");
		return -ENOTSUP;
	}

	status = adc_b91_validate_buffer_size(sequence);
	if (status) {
		LOG_ERR("Buffer size too small.");
		return status;
	}

	return 0;
}

/* Convert dts pin to B91 SDK pin */
static adc_input_pin_def_e adc_b91_get_pin(uint8_t dt_pin)
{
	adc_input_pin_def_e adc_pin;

	switch (dt_pin) {
	case DT_ADC_GPIO_PB0:
		adc_pin = ADC_GPIO_PB0;
		break;
	case DT_ADC_GPIO_PB1:
		adc_pin = ADC_GPIO_PB1;
		break;
	case DT_ADC_GPIO_PB2:
		adc_pin = ADC_GPIO_PB2;
		break;
	case DT_ADC_GPIO_PB3:
		adc_pin = ADC_GPIO_PB3;
		break;
	case DT_ADC_GPIO_PB4:
		adc_pin = ADC_GPIO_PB4;
		break;
	case DT_ADC_GPIO_PB5:
		adc_pin = ADC_GPIO_PB5;
		break;
	case DT_ADC_GPIO_PB6:
		adc_pin = ADC_GPIO_PB6;
		break;
	case DT_ADC_GPIO_PB7:
		adc_pin = ADC_GPIO_PB7;
		break;
	case DT_ADC_GPIO_PD0:
		adc_pin = ADC_GPIO_PD0;
		break;
	case DT_ADC_GPIO_PD1:
		adc_pin = ADC_GPIO_PD1;
		break;
	case DT_ADC_VBAT:
		adc_pin = ADC_VBAT;
		break;

	default:
		adc_pin = NOINPUTN;
		break;
	}

	return adc_pin;
}

/* Get ADC value */
static signed short adc_b91_get_code(void)
{
	signed short adc_code;

	analog_write_reg8(areg_adc_data_sample_control,
			  analog_read_reg8(areg_adc_data_sample_control) | FLD_NOT_SAMPLE_ADC_DATA);

	adc_code = analog_read_reg16(areg_adc_misc_l);

	analog_write_reg8(areg_adc_data_sample_control,
		analog_read_reg8(areg_adc_data_sample_control) & (~FLD_NOT_SAMPLE_ADC_DATA));

	return adc_code;
}

/* ADC Context API implementation: start sampling */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct b91_adc_data *data =
		CONTAINER_OF(ctx, struct b91_adc_data, ctx);

	data->repeat_buffer = data->buffer;

	adc_power_on();

	k_sem_give(&data->acq_sem);
}

/* ADC Context API implementation: buffer pointer */
static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct b91_adc_data *data =
		CONTAINER_OF(ctx, struct b91_adc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Start ADC measurements */
static int adc_b91_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct b91_adc_data *data = dev->data;

	/* Validate input parameters */
	status = adc_b91_validate_sequence(sequence);
	if (status != 0) {
		return status;
	}

	/* Set resolution */
	switch (sequence->resolution) {
	case 14:
		adc_set_resolution(ADC_RES14);
		data->resolution_divider = 1;
		break;
	case 12:
		adc_set_resolution(ADC_RES12);
		data->resolution_divider = 4;
		break;
	case 10:
		adc_set_resolution(ADC_RES10);
		data->resolution_divider = 16;
		break;
	case 8:
		adc_set_resolution(ADC_RES8);
		data->resolution_divider = 64;
		break;
	default:
		LOG_ERR("Selected ADC resolution is not supported.");
		return -EINVAL;
	}

	/* Save buffer */
	data->buffer = sequence->buffer;

	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

/* Main ADC Acquisition thread */
static void adc_b91_acquisition_thread(const struct device *dev)
{
	int16_t adc_code;
	struct b91_adc_data *data = dev->data;

	while (true) {
		/* Wait for Acquisition semaphore */
		k_sem_take(&data->acq_sem, K_FOREVER);

		/* Wait for ADC data ready */
		while ((analog_read_reg8(AREG_ADC_DATA_STATUS) & ADC_DATA_READY)
				!= ADC_DATA_READY) {
		}

		/* Perform read */
		adc_code = (adc_b91_get_code() / data->resolution_divider);
		if (!data->differential) {
			/* Sign bit is not used in case of single-ended configuration */
			adc_code = adc_code * 2;

			/* Do not return negative value for single-ended configuration */
			if (adc_code < 0) {
				adc_code = 0;
			}
		}
		*data->buffer++ = adc_code;

		/* Power off ADC */
		adc_power_off();

		/* Release ADC context */
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

/* ADC Driver initialization */
static int adc_b91_init(const struct device *dev)
{
	struct b91_adc_data *data = dev->data;

	k_sem_init(&data->acq_sem, 0, 1);

	k_thread_create(&data->thread, data->stack,
			CONFIG_ADC_B91_ACQUISITION_THREAD_STACK_SIZE,
			(k_thread_entry_t)adc_b91_acquisition_thread,
			(void *)dev, NULL, NULL,
			CONFIG_ADC_B91_ACQUISITION_THREAD_PRIO,
			0, K_NO_WAIT);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/* API implementation: channel_setup */
static int adc_b91_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	adc_ref_vol_e vref_internal_mv;
	adc_sample_freq_e sample_freq;
	adc_pre_scale_e pre_scale;
	adc_sample_cycle_e sample_cycl;
	adc_input_pin_def_e input_positive;
	adc_input_pin_def_e input_negative;
	struct b91_adc_data *data = dev->data;
	const struct b91_adc_cfg *config = dev->config;

	/* Check channel ID */
	if (channel_cfg->channel_id > 0) {
		LOG_ERR("Only channel 0 is supported.");
		return -EINVAL;
	}

	/* Check reference */
	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Selected ADC reference is not supported.");
		return -EINVAL;
	}

	/* Check internal reference */
	switch (config->vref_internal_mv) {
	case 900:
		vref_internal_mv = ADC_VREF_0P9V;
		break;
	case 1200:
		vref_internal_mv = ADC_VREF_1P2V;
		break;
	default:
		LOG_ERR("Selected reference voltage is not supported.");
		return -EINVAL;
	}

	/* Check sample frequency */
	switch (config->sample_freq) {
	case 23000:
		sample_freq = ADC_SAMPLE_FREQ_23K;
		break;
	case 48000:
		sample_freq = ADC_SAMPLE_FREQ_48K;
		break;
	case 96000:
		sample_freq = ADC_SAMPLE_FREQ_96K;
		break;
	default:
		LOG_ERR("Selected sample frequency is not supported.");
		return -EINVAL;
	}

	/* Check gain */
	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		pre_scale = ADC_PRESCALE_1;
		break;
	case ADC_GAIN_1_4:
		pre_scale = ADC_PRESCALE_1F4;
		break;
	default:
		LOG_ERR("Selected ADC gain is not supported.");
		return -EINVAL;
	}

	/* Check acquisition time */
	switch (channel_cfg->acquisition_time) {
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 3):
		sample_cycl = ADC_SAMPLE_CYC_3;
		break;
	case ADC_ACQ_TIME_DEFAULT:
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 6):
		sample_cycl = ADC_SAMPLE_CYC_6;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 9):
		sample_cycl = ADC_SAMPLE_CYC_9;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 12):
		sample_cycl = ADC_SAMPLE_CYC_12;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 18):
		sample_cycl = ADC_SAMPLE_CYC_18;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 24):
		sample_cycl = ADC_SAMPLE_CYC_24;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 36):
		sample_cycl = ADC_SAMPLE_CYC_36;
		break;
	case ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 48):
		sample_cycl = ADC_SAMPLE_CYC_48;
		break;

	default:
		LOG_ERR("Selected ADC acquisition time is not supported.");
		return -EINVAL;
	}

	/* Check for valid pins configuration */
	input_positive = adc_b91_get_pin(channel_cfg->input_positive);
	input_negative = adc_b91_get_pin(channel_cfg->input_negative);
	if ((input_positive == (uint8_t)ADC_VBAT || input_negative == (uint8_t)ADC_VBAT) &&
		channel_cfg->differential) {
		LOG_ERR("VBAT pin is not available for differential mode.");
		return -EINVAL;
	} else if (channel_cfg->differential && (input_negative == (uint8_t)NOINPUTN)) {
		LOG_ERR("Negative input is not selected.");
		return -EINVAL;
	}

	/* Init ADC */
	data->differential = channel_cfg->differential;
	adc_init(vref_internal_mv, pre_scale, sample_freq);
	adc_set_vbat_divider(ADC_VBAT_DIV_OFF);
	adc_set_tsample_cycle(sample_cycl);

	/* Init ADC Pins */
	if (channel_cfg->differential) {
		/* Differential pins configuration */
		adc_pin_config(ADC_GPIO_MODE, input_positive);
		adc_pin_config(ADC_GPIO_MODE, input_negative);
		adc_set_diff_input(channel_cfg->input_positive, channel_cfg->input_negative);
	} else if (input_positive == (uint8_t)ADC_VBAT) {
		/* Single-ended Vbat pin configuration */
		adc_set_diff_input(ADC_VBAT, GND);
	} else {
		/* Single-ended GPIO pin configuration */
		adc_pin_config(ADC_GPIO_MODE, input_positive);
		adc_set_diff_input(channel_cfg->input_positive, GND);
	}

	return 0;
}

/* API implementation: read */
static int adc_b91_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	int status;
	struct b91_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	status = adc_b91_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, status);

	return status;
}

#ifdef CONFIG_ADC_ASYNC
/* API implementation: read_async */
static int adc_b91_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	int status;
	struct b91_adc_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	status = adc_b91_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, status);

	return status;
}
#endif /* CONFIG_ADC_ASYNC */

static struct b91_adc_data data_0 = {
	ADC_CONTEXT_INIT_TIMER(data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(data_0, ctx),
};

static const struct b91_adc_cfg cfg_0 = {
	.sample_freq = DT_INST_PROP(0, sample_freq),
	.vref_internal_mv = DT_INST_PROP(0, vref_internal_mv),
};

static const struct adc_driver_api adc_b91_driver_api = {
	.channel_setup = adc_b91_channel_setup,
	.read = adc_b91_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_b91_read_async,
#endif
	.ref_internal = cfg_0.vref_internal_mv,
};

DEVICE_DT_INST_DEFINE(0, adc_b91_init, NULL,
		      &data_0,  &cfg_0,
		      POST_KERNEL,
		      CONFIG_ADC_INIT_PRIORITY,
		      &adc_b91_driver_api);
