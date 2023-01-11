/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_smartbond_sdadc

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include <DA1469xAB.h>
#include "adc_context.h"
#include <zephyr/dt-bindings/adc/smartbond-adc.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/drivers/pinctrl.h>

LOG_MODULE_REGISTER(adc_smartbond_sdadc);

struct sdadc_smartbond_cfg {
	const struct pinctrl_dev_config *pcfg;
	/** Value for SDADC_CLK_FREQ */
	uint8_t sdadc_clk_freq;
};

struct sdadc_smartbond_data {
	struct adc_context ctx;

	/* Buffer to store channel data */
	uint16_t *buffer;
	/* Copy of channel mask from sequence */
	uint32_t channel_read_mask;
	/* Number of bits in sequence channels */
	uint8_t sequence_channel_count;
	/* Index in buffer to store current value to */
	uint8_t result_index;
};

#define SMARTBOND_SDADC_CHANNEL_COUNT	8

struct sdadc_smartbond_channel_cfg {
	uint32_t sd_adc_ctrl_reg;
};

static struct sdadc_smartbond_channel_cfg m_sdchannels[SMARTBOND_SDADC_CHANNEL_COUNT];

/* Implementation of the ADC driver API function: adc_channel_setup. */
static int sdadc_smartbond_channel_setup(const struct device *dev,
					 const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;
	struct sdadc_smartbond_channel_cfg *config = &m_sdchannels[channel_id];

	if (channel_id >= SMARTBOND_SDADC_CHANNEL_COUNT) {
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->input_positive > SMARTBOND_SDADC_VBAT) {
		LOG_ERR("Channels out of range");
		return -EINVAL;
	}
	if (channel_cfg->differential) {
		if (channel_cfg->input_negative >= SMARTBOND_SDADC_VBAT) {
			LOG_ERR("Differential negative channels out of range");
			return -EINVAL;
		}
	}

	config->sd_adc_ctrl_reg = 0;
	if ((channel_cfg->input_positive == SMARTBOND_SDADC_VBAT &&
	     channel_cfg->gain != ADC_GAIN_1_4) ||
	    (channel_cfg->input_positive != SMARTBOND_SDADC_VBAT &&
	     channel_cfg->gain != ADC_GAIN_1)) {
		LOG_ERR("ADC gain should be 1/4 for VBAT and 1 for all other channels");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		break;
	default:
		LOG_ERR("Selected ADC reference is not valid");
		return -EINVAL;
	}

	config->sd_adc_ctrl_reg =
		channel_cfg->input_positive << SDADC_SDADC_CTRL_REG_SDADC_INP_SEL_Pos;
	if (channel_cfg->differential) {
		config->sd_adc_ctrl_reg |=
			channel_cfg->input_negative << SDADC_SDADC_CTRL_REG_SDADC_INN_SEL_Pos;
	} else {
		config->sd_adc_ctrl_reg |= SDADC_SDADC_CTRL_REG_SDADC_SE_Msk;
	}

	return 0;
}

#define PER_CHANNEL_ADC_CONFIG_MASK (SDADC_SDADC_CTRL_REG_SDADC_INP_SEL_Msk |	\
				     SDADC_SDADC_CTRL_REG_SDADC_INN_SEL_Msk |	\
				     SDADC_SDADC_CTRL_REG_SDADC_SE_Msk		\
)

static int pop_count(uint32_t n)
{
	return __builtin_popcount(n);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	uint32_t val;
	struct sdadc_smartbond_data *data =
		CONTAINER_OF(ctx, struct sdadc_smartbond_data, ctx);
	/* Extract lower channel from sequence mask */
	int current_channel = u32_count_trailing_zeros(data->channel_read_mask);

	if (ctx->sequence.calibrate) {
		/* TODO: Add calibration code */
	} else {
		val = SDADC->SDADC_CTRL_REG & ~PER_CHANNEL_ADC_CONFIG_MASK;
		val |= m_sdchannels[current_channel].sd_adc_ctrl_reg;
		val |= SDADC_SDADC_CTRL_REG_SDADC_START_Msk |
		       SDADC_SDADC_CTRL_REG_SDADC_MINT_Msk;
		val |= (ctx->sequence.oversampling - 7) << SDADC_SDADC_CTRL_REG_SDADC_OSR_Pos;

		SDADC->SDADC_CTRL_REG = val;
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct sdadc_smartbond_data *data =
		CONTAINER_OF(ctx, struct sdadc_smartbond_data, ctx);

	if (!repeat) {
		data->buffer += data->sequence_channel_count;
	}
}

static int check_buffer_size(const struct adc_sequence *sequence,
			     uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);
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
	int error;
	struct sdadc_smartbond_data *data = dev->data;

	if (sequence->oversampling < 7U || sequence->oversampling > 10) {
		LOG_ERR("Invalid oversampling");
		return -EINVAL;
	}

	if ((sequence->channels == 0) ||
	    ((sequence->channels & ~BIT_MASK(SMARTBOND_SDADC_CHANNEL_COUNT)) != 0)) {
		LOG_ERR("Channel scanning is not supported");
		return -EINVAL;
	}

	if (sequence->resolution < 8 || sequence->resolution > 15) {
		LOG_ERR("ADC resolution value %d is not valid",
			sequence->resolution);
		return -EINVAL;
	}

	error = check_buffer_size(sequence, 1);
	if (error) {
		return error;
	}

	data->buffer = sequence->buffer;
	data->channel_read_mask = sequence->channels;
	data->sequence_channel_count = pop_count(sequence->channels);
	data->result_index = 0;

	adc_context_start_read(&data->ctx, sequence);

	error = adc_context_wait_for_completion(&data->ctx);
	return error;
}

static void sdadc_smartbond_isr(const struct device *dev)
{
	struct sdadc_smartbond_data *data = dev->data;
	int current_channel = u32_count_trailing_zeros(data->channel_read_mask);

	SDADC->SDADC_CLEAR_INT_REG = 0;
	/* Store current channel value, result is left justified, move bits right */
	data->buffer[data->result_index++] = ((uint16_t)SDADC->SDADC_RESULT_REG) >>
		(16 - data->ctx.sequence.resolution);
	/* Exclude channel from mask for further reading */
	data->channel_read_mask ^= 1 << current_channel;

	if (data->channel_read_mask == 0) {
		adc_context_on_sampling_done(&data->ctx, dev);
	} else {
		adc_context_start_sampling(&data->ctx);
	}

	LOG_DBG("%s ISR triggered.", dev->name);
}

/* Implementation of the ADC driver API function: adc_read. */
static int sdadc_smartbond_read(const struct device *dev,
				const struct adc_sequence *sequence)
{
	int error;
	struct sdadc_smartbond_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
/* Implementation of the ADC driver API function: adc_read_sync. */
static int sdadc_smartbond_read_async(const struct device *dev,
				      const struct adc_sequence *sequence,
				      struct k_poll_signal *async)
{
	struct sdadc_smartbond_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static int sdadc_smartbond_init(const struct device *dev)
{
	int err;
	struct sdadc_smartbond_data *data = dev->data;
	const struct sdadc_smartbond_cfg *config = dev->config;

	SDADC->SDADC_CTRL_REG = SDADC_SDADC_CTRL_REG_SDADC_EN_Msk;
	SDADC->SDADC_CLEAR_INT_REG = 0x0;
	SDADC->SDADC_TEST_REG =
		(SDADC->SDADC_TEST_REG & ~SDADC_SDADC_TEST_REG_SDADC_CLK_FREQ_Msk) |
		(config->sdadc_clk_freq) << SDADC_SDADC_TEST_REG_SDADC_CLK_FREQ_Pos;
	/*
	 * Configure dt provided device signals when available.
	 * pinctrl is optional so ENOENT is not setup failure.
	 */
	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0 && err != -ENOENT) {
		LOG_ERR("ADC pinctrl setup failed (%d)", err);
		return err;
	}
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    sdadc_smartbond_isr, DEVICE_DT_INST_GET(0), 0);

	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
	NVIC_EnableIRQ(DT_INST_IRQN(0));

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api sdadc_smartbond_driver_api = {
	.channel_setup = sdadc_smartbond_channel_setup,
	.read          = sdadc_smartbond_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = sdadc_smartbond_read_async,
#endif
	.ref_internal  = 1200,
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
#define SDADC_INIT(inst)							\
	BUILD_ASSERT((inst) == 0,						\
		     "multiple instances not supported");			\
	PINCTRL_DT_INST_DEFINE(inst);						\
	static const struct sdadc_smartbond_cfg sdadc_smartbond_cfg_##inst = {	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.sdadc_clk_freq = DT_INST_PROP(inst, clock_freq),		\
	};									\
	static struct sdadc_smartbond_data sdadc_smartbond_data_##inst = {	\
		ADC_CONTEXT_INIT_TIMER(sdadc_smartbond_data_##inst, ctx),	\
		ADC_CONTEXT_INIT_LOCK(sdadc_smartbond_data_##inst, ctx),	\
		ADC_CONTEXT_INIT_SYNC(sdadc_smartbond_data_##inst, ctx),	\
	};									\
	DEVICE_DT_INST_DEFINE(0,						\
			      sdadc_smartbond_init, NULL,			\
			      &sdadc_smartbond_data_##inst,			\
			      &sdadc_smartbond_cfg_##inst,			\
			      POST_KERNEL,					\
			      CONFIG_ADC_INIT_PRIORITY,				\
			      &sdadc_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SDADC_INIT)
