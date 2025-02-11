/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_adc

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <errno.h>
#include <reg/adc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

struct adc_kb1200_config {
	/* ADC Register base address */
	struct adc_regs *adc;
	/* Pin control */
	const struct pinctrl_dev_config *pcfg;
};

struct adc_kb1200_data {
	struct adc_context ctx;
	const struct device *adc_dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint16_t *buf_end;
};

/* ADC local functions */
static bool adc_kb1200_validate_buffer_size(const struct adc_sequence *sequence)
{
	int chan_count = 0;
	size_t buff_need;
	uint32_t chan_mask;

	for (chan_mask = 0x80; chan_mask != 0; chan_mask >>= 1) {
		if (chan_mask & sequence->channels) {
			chan_count++;
		}
	}

	buff_need = chan_count * sizeof(uint16_t);

	if (sequence->options) {
		buff_need *= 1 + sequence->options->extra_samplings;
	}

	if (buff_need > sequence->buffer_size) {
		return false;
	}

	return true;
}
/* ADC Sample Flow (by using adc_context.h api function)
 *  1. Start ADC sampling (set up flag ctx->sync)
 *     adc_context_start_read() -> adc_context_start_sampling()
 *  2. Wait ADC sample finish (by monitor flag ctx->sync)
 *     adc_context_wait_for_completion
 *  3. Finish ADC sample (isr clear flag ctx->sync)
 *     adc_context_on_sampling_done -> adc_context_complete
 */
static int adc_kb1200_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_kb1200_config *config = dev->config;
	struct adc_kb1200_data *data = dev->data;
	int error = 0;

	if (!sequence->channels || (sequence->channels & ~BIT_MASK(ADC_MAX_CHAN))) {
		printk("Invalid ADC channels.\n");
		return -EINVAL;
	}
	/* Fixed 10 bit resolution of ene ADC */
	if (sequence->resolution != ADC_RESOLUTION) {
		printk("Unfixed 10 bit ADC resolution.\n");
		return -ENOTSUP;
	}
	/* Check sequence->buffer_size is enough */
	if (!adc_kb1200_validate_buffer_size(sequence)) {
		printk("ADC buffer size too small.\n");
		return -ENOMEM;
	}

	/* assign record buffer pointer */
	data->buffer = sequence->buffer;
	data->buf_end = data->buffer + sequence->buffer_size / sizeof(uint16_t);
	/* store device for adc_context_start_read() */
	data->adc_dev = dev;
	/* Inform adc start sampling */
	adc_context_start_read(&data->ctx, sequence);
	/* Since kb1200 adc has no irq. So need polling the adc conversion
	 * flag to be valid, then record adc value.
	 */
	uint32_t channels = (config->adc->ADCCFG & ADC_CHANNEL_BIT_MASK) >> ADC_CHANNEL_BIT_POS;

	while (channels) {
		int count;
		int ch_num;

		count = 0;
		ch_num = find_lsb_set(channels) - 1;
		/* wait valid flag */
		while (config->adc->ADCDAT[ch_num] & ADC_INVALID_VALUE) {
			k_busy_wait(ADC_WAIT_TIME);
			count++;
			if (count >= ADC_WAIT_CNT) {
				printk("ADC busy timeout...\n");
				error = -EBUSY;
				break;
			}
		}
		/* check buffer size is enough then record adc value */
		if (data->buffer < data->buf_end) {
			*data->buffer = (uint16_t)(config->adc->ADCDAT[ch_num]);
			data->buffer++;
		} else {
			error = -EINVAL;
			break;
		}

		/* clear completed channel */
		channels &= ~BIT(ch_num);
	}
	/* Besause polling the adc conversion flag. don't need wait_for_completion*/

	/* Inform adc sampling is done */
	adc_context_on_sampling_done(&data->ctx, dev);
	return error;
}

/* ADC api functions */
static int adc_kb1200_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id >= ADC_MAX_CHAN) {
		printk("Invalid channel %d.\n", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		printk("Unsupported channel acquisition time.\n");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		printk("Differential channels are not supported.\n");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		printk("Unsupported channel gain %d.\n", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		printk("Unsupported channel reference.\n");
		return -ENOTSUP;
	}
	printk("ADC channel %d configured.\n", channel_cfg->channel_id);
	return 0;
}

static int adc_kb1200_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_kb1200_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_kb1200_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
static int adc_kb1200_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_kb1200_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_kb1200_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

/* ADC api function (using by adc_context.H function) */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_kb1200_data *data = CONTAINER_OF(ctx, struct adc_kb1200_data, ctx);
	const struct device *dev = data->adc_dev;
	const struct adc_kb1200_config *config = dev->config;

	data->repeat_buffer = data->buffer;
	config->adc->ADCCFG = (config->adc->ADCCFG & ~ADC_CHANNEL_BIT_MASK) |
		      (ctx->sequence.channels << ADC_CHANNEL_BIT_POS);
	config->adc->ADCCFG |= ADC_FUNCTION_ENABLE;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_kb1200_data *data = CONTAINER_OF(ctx, struct adc_kb1200_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

struct adc_driver_api adc_kb1200_api = {
	.channel_setup = adc_kb1200_channel_setup,
	.read = adc_kb1200_read,
	.ref_internal = ADC_VREF_ANALOG,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_kb1200_read_async,
#endif
};

static int adc_kb1200_init(const struct device *dev)
{
	const struct adc_kb1200_config *config = dev->config;
	struct adc_kb1200_data *data = dev->data;
	int ret;

	adc_context_unlock_unconditionally(&data->ctx);
	/* Configure pin-mux for ADC device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		printk("ADC pinctrl setup failed (%d).\n", ret);
		return ret;
	}

	return 0;
}

#define ADC_KB1200_DEVICE(inst)                                                                    \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static struct adc_kb1200_data adc_kb1200_data_##inst = {                                   \
		ADC_CONTEXT_INIT_TIMER(adc_kb1200_data_##inst, ctx),                               \
		ADC_CONTEXT_INIT_LOCK(adc_kb1200_data_##inst, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(adc_kb1200_data_##inst, ctx),                                \
	};                                                                                         \
	static const struct adc_kb1200_config adc_kb1200_config_##inst = {                         \
		.adc = (struct adc_regs *)DT_INST_REG_ADDR(inst),                                  \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &adc_kb1200_init, NULL, &adc_kb1200_data_##inst,               \
			      &adc_kb1200_config_##inst, PRE_KERNEL_1,                             \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &adc_kb1200_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_KB1200_DEVICE)
