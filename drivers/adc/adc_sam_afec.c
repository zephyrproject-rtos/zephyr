/*
 * Copyright (c) 2017 comsuisse AG
 * Copyright (c) 2018 Justin Watson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family ADC (AFEC) driver.
 *
 * This is an implementation of the Zephyr ADC driver using the SAM Analog
 * Front-End Controller (AFEC) peripheral.
 */

#include <errno.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/adc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_sam_afec);

#define NUM_CHANNELS 12

#define CONF_ADC_PRESCALER ((SOC_ATMEL_SAM_MCK_FREQ_HZ / 15000000) - 1)

typedef void (*cfg_func_t)(struct device *dev);

struct adc_sam_data {
	struct adc_context ctx;
	struct device *dev;

	/* Pointer to the buffer in the sequence. */
	u16_t *buffer;

	/* Pointer to the beginning of a sample. Consider the number of
	 * channels in the sequence: this buffer changes by that amount
	 * so all the channels would get repeated.
	 */
	u16_t *repeat_buffer;

	/* Bit mask of the channels to be sampled. */
	u32_t channels;

	/* Index of the channel being sampled. */
	u8_t channel_id;
};

struct adc_sam_cfg {
	Afec *regs;
	cfg_func_t cfg_func;
	u32_t periph_id;
	struct soc_gpio_pin afec_trg_pin;
};

#define DEV_CFG(dev) \
	((const struct adc_sam_cfg *const)(dev)->config->config_info)

#define DEV_DATA(dev) \
	((struct adc_sam_data *)(dev)->driver_data)

static int adc_sam_channel_setup(struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_sam_cfg * const cfg = DEV_CFG(dev);
	Afec *const afec = cfg->regs;

	u8_t channel_id = channel_cfg->channel_id;

	/* Clear the gain bits for the channel. */
	afec->AFEC_CGR &= ~(3 << channel_id * 2U);

	switch (channel_cfg->gain) {
	case ADC_GAIN_1:
		/* A value of 0 in this register is a gain of 1. */
		break;
	case ADC_GAIN_1_2:
		afec->AFEC_CGR |= (1 << (channel_id * 2U));
		break;
	case ADC_GAIN_1_4:
		afec->AFEC_CGR |= (2 << (channel_id * 2U));
		break;
	default:
		LOG_ERR("Selected ADC gain is not valid");
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("Selected reference is not valid");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential input is not supported");
		return -EINVAL;
	}

	/* Set single ended channels to unsigned and differential channels
	 * to signed conversions.
	 */
	afec->AFEC_EMR &= ~(AFEC_EMR_SIGNMODE(
			  AFEC_EMR_SIGNMODE_SE_UNSG_DF_SIGN_Val));

	return 0;
}

static void adc_sam_start_conversion(struct device *dev)
{
	const struct adc_sam_cfg *const cfg = DEV_CFG(dev);
	struct adc_sam_data *data = DEV_DATA(dev);
	Afec *const afec = cfg->regs;

	data->channel_id = find_lsb_set(data->channels) - 1;

	LOG_DBG("Starting channel %d", data->channel_id);

	/* Disable all channels. */
	afec->AFEC_CHDR = 0xfff;
	afec->AFEC_IDR = 0xfff;

	/* Enable the ADC channel. This also enables/selects the channel pin as
	 * an input to the AFEC (50.5.1 SAM E70 datasheet).
	 */
	afec->AFEC_CHER = (1 << data->channel_id);

	/* Enable the interrupt for the channel. */
	afec->AFEC_IER = (1 << data->channel_id);

	/* Start the conversions. */
	afec->AFEC_CR = AFEC_CR_START;
}

/**
 * This is only called once at the beginning of all the conversions,
 * all channels as a group.
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_sam_data *data = CONTAINER_OF(ctx, struct adc_sam_data, ctx);

	data->channels = ctx->sequence.channels;

	adc_sam_start_conversion(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_sam_data *data = CONTAINER_OF(ctx, struct adc_sam_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     u8_t active_channels)
{
	size_t needed_buffer_size;
	needed_buffer_size = active_channels * sizeof(u16_t);
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
	struct adc_sam_data *data = DEV_DATA(dev);
	int error = 0;
	u32_t channels = sequence->channels;

	data->channels = 0U;

	/* Signal an error if the channel selection is invalid (no channels or
	 * a non-existing one is selected).
	 */
	if (channels == 0U ||
	    (channels & (~0UL << NUM_CHANNELS))) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("Oversampling is not supported");
		return -EINVAL;
	}

	if (sequence->resolution != 12U) {
		/* TODO JKW: Support the Enhanced Resolution Mode 50.6.3 page
		 * 1544.
		 */
		LOG_ERR("ADC resolution value %d is not valid",
			    sequence->resolution);
		return -EINVAL;
	}

	u8_t num_active_channels = 0U;
	u8_t channel = 0U;

	while (channels > 0) {
		if (channels & 1) {
			++num_active_channels;
		}
		channels >>= 1;
		++channel;
	}

	error = check_buffer_size(sequence, num_active_channels);
	if (error) {
		return error;
	}

	/* In the context you have a pointer to the adc_sam_data structure
	 * only.
	 */
	data->buffer = sequence->buffer;
	data->repeat_buffer = sequence->buffer;

	/* At this point we allow the scheduler to do other things while
	 * we wait for the conversions to complete. This is provided by the
	 * adc_context functions. However, the caller of this function is
	 * blocked until the results are in.
	 */
	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);
	return error;
}

static int adc_sam_read(struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_sam_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_sam_init(struct device *dev)
{
	const struct adc_sam_cfg *const cfg = DEV_CFG(dev);
	struct adc_sam_data *data = DEV_DATA(dev);
	Afec *const afec = cfg->regs;

	/* Reset the AFEC. */
	afec->AFEC_CR = AFEC_CR_SWRST;

	afec->AFEC_MR = AFEC_MR_TRGEN_DIS
		      | AFEC_MR_SLEEP_NORMAL
		      | AFEC_MR_FWUP_OFF
		      | AFEC_MR_FREERUN_OFF
		      | AFEC_MR_PRESCAL(CONF_ADC_PRESCALER)
		      | AFEC_MR_STARTUP_SUT96
		      | AFEC_MR_ONE
		      | AFEC_MR_USEQ_NUM_ORDER;

	/* Set all channels CM voltage to Vrefp/2 (512). */
	for (int i = 0; i < NUM_CHANNELS; i++) {
		afec->AFEC_CSELR = i;
		afec->AFEC_COCR = 512;
	}

	/* Enable PGA and Current Bias. */
	afec->AFEC_ACR = AFEC_ACR_PGA0EN
		       | AFEC_ACR_PGA1EN
		       | AFEC_ACR_IBCTL(1);

	soc_pmc_peripheral_enable(cfg->periph_id);

	cfg->cfg_func(dev);

	data->dev = dev;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_sam_read_async(struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_sam_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static const struct adc_driver_api adc_sam_api = {
	.channel_setup = adc_sam_channel_setup,
	.read = adc_sam_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_sam_read_async,
#endif
};

static void adc_sam_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_sam_data *data = DEV_DATA(dev);
	const struct adc_sam_cfg *const cfg = DEV_CFG(dev);
	Afec *const afec = cfg->regs;
	u16_t result;

	afec->AFEC_CHDR |= BIT(data->channel_id);
	afec->AFEC_IDR |= BIT(data->channel_id);

	afec->AFEC_CSELR = AFEC_CSELR_CSEL(data->channel_id);
	result = (u16_t)(afec->AFEC_CDR);

	*data->buffer++ = result;
	data->channels &= ~BIT(data->channel_id);

	if (data->channels) {
		adc_sam_start_conversion(dev);
	} else {
		/* Called once all conversions have completed.*/
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

#ifdef CONFIG_ADC_0
static void adc0_sam_cfg_func(struct device *dev);

static const struct adc_sam_cfg adc0_sam_cfg = {
	.regs = (Afec *)DT_ADC_0_BASE_ADDRESS,
	.cfg_func = adc0_sam_cfg_func,
	.periph_id = DT_ADC_0_PERIPHERAL_ID,
	.afec_trg_pin = PIN_AFE0_ADTRG,
};

static struct adc_sam_data adc0_sam_data = {
	ADC_CONTEXT_INIT_TIMER(adc0_sam_data, ctx),
	ADC_CONTEXT_INIT_LOCK(adc0_sam_data, ctx),
	ADC_CONTEXT_INIT_SYNC(adc0_sam_data, ctx),
};

DEVICE_AND_API_INIT(adc0_sam, DT_ADC_0_NAME, adc_sam_init,
		    &adc0_sam_data, &adc0_sam_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &adc_sam_api);

static void adc0_sam_cfg_func(struct device *dev)
{
	IRQ_CONNECT(DT_ADC_0_IRQ, DT_ADC_0_IRQ_PRI, adc_sam_isr,
		    DEVICE_GET(adc0_sam), 0);
	irq_enable(DT_ADC_0_IRQ);
}

#endif /* CONFIG_ADC_0 */

#ifdef CONFIG_ADC_1
static void adc1_sam_cfg_func(struct device *dev);

static const struct adc_sam_cfg adc1_sam_cfg = {
	.regs = (Afec *)DT_ADC_1_BASE_ADDRESS,
	.cfg_func = adc1_sam_cfg_func,
	.periph_id = DT_ADC_1_PERIPHERAL_ID,
	.afec_trg_pin = PIN_AFE1_ADTRG,
};

static struct adc_sam_data adc1_sam_data = {
	ADC_CONTEXT_INIT_TIMER(adc1_sam_data, ctx),
	ADC_CONTEXT_INIT_LOCK(adc1_sam_data, ctx),
	ADC_CONTEXT_INIT_SYNC(adc1_sam_data, ctx),
};

DEVICE_AND_API_INIT(adc1_sam, DT_ADC_1_NAME, adc_sam_init,
		    &adc1_sam_data, &adc1_sam_cfg, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &adc_sam_api);

static void adc1_sam_cfg_func(struct device *dev)
{
	IRQ_CONNECT(DT_ADC_1_IRQ, DT_ADC_1_IRQ_PRI, adc_sam_isr,
		    DEVICE_GET(adc1_sam), 0);
	irq_enable(DT_ADC_1_IRQ);
}

#endif /* CONFIG_ADC_1 */
