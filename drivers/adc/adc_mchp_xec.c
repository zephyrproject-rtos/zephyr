/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_adc

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_mchp_xec);

#include <drivers/adc.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define XEC_ADC_VREF_ANALOG 3300

/* ADC Control Register */
#define XEC_ADC_CTRL_SINGLE_DONE_STATUS		BIT(7)
#define XEC_ADC_CTRL_REPEAT_DONE_STATUS		BIT(6)
#define XER_ADC_CTRL_SOFT_RESET			BIT(4)
#define XEC_ADC_CTRL_POWER_SAVER_DIS		BIT(3)
#define XEC_ADC_CTRL_START_REPEAT		BIT(2)
#define XEC_ADC_CTRL_START_SINGLE		BIT(1)
#define XEC_ADC_CTRL_ACTIVATE			BIT(0)

struct adc_xec_data {
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
};

struct adc_xec_regs {
	uint32_t control_reg;
	uint32_t delay_reg;
	uint32_t status_reg;
	uint32_t single_reg;
	uint32_t repeat_reg;
	uint32_t channel_read_reg[8];
	uint32_t unused[18];
	uint32_t config_reg;
	uint32_t vref_channel_reg;
	uint32_t vref_control_reg;
	uint32_t sar_control_reg;
};

#define ADC_XEC_REG_BASE						\
	((struct adc_xec_regs *)(DT_INST_REG_ADDR(0)))


DEVICE_DECLARE(adc_xec);

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_xec_data *data = CONTAINER_OF(ctx, struct adc_xec_data, ctx);
	struct adc_xec_regs *adc_regs = ADC_XEC_REG_BASE;

	data->repeat_buffer = data->buffer;

	adc_regs->single_reg = ctx->sequence.channels;
	adc_regs->control_reg |= XEC_ADC_CTRL_START_SINGLE;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_xec_data *data = CONTAINER_OF(ctx, struct adc_xec_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_xec_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct adc_xec_regs *adc_regs = ADC_XEC_REG_BASE;
	uint32_t reg;

	ARG_UNUSED(dev);

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= MCHP_ADC_MAX_CHAN) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	/* Setup VREF */
	reg = adc_regs->vref_channel_reg;
	reg &= ~MCHP_ADC_CH_VREF_SEL_MASK(channel_cfg->channel_id);

	if (channel_cfg->reference == ADC_REF_INTERNAL) {
		reg |= MCHP_ADC_CH_VREF_SEL_PAD(channel_cfg->channel_id);
	} else if (channel_cfg->reference == ADC_REF_EXTERNAL0) {
		reg |= MCHP_ADC_CH_VREF_SEL_GPIO(channel_cfg->channel_id);
	} else {
		return -EINVAL;
	}

	adc_regs->vref_channel_reg = reg;

	/* Differential mode? */
	reg = adc_regs->sar_control_reg;
	reg &= ~BIT(MCHP_ADC_SAR_CTRL_SELDIFF_POS);
	if (channel_cfg->differential != 0) {
		reg |= MCHP_ADC_SAR_CTRL_SELDIFF_EN;
	}
	adc_regs->sar_control_reg = reg;

	return 0;
}

static bool adc_xec_validate_buffer_size(const struct adc_sequence *sequence)
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

static int adc_xec_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	struct adc_xec_regs *adc_regs = ADC_XEC_REG_BASE;
	struct adc_xec_data *data = dev->data;
	uint32_t reg;

	if (sequence->channels & ~BIT_MASK(MCHP_ADC_MAX_CHAN)) {
		LOG_ERR("Incorrect channels, bitmask 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->channels == 0UL) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (!adc_xec_validate_buffer_size(sequence)) {
		LOG_ERR("Incorrect buffer size");
		return -ENOMEM;
	}

	/* Setup ADC resolution */
	reg = adc_regs->sar_control_reg;
	reg &= ~(MCHP_ADC_SAR_CTRL_RES_MASK |
		 (1 << MCHP_ADC_SAR_CTRL_SHIFTD_POS));

	if (sequence->resolution == 12) {
		reg |= MCHP_ADC_SAR_CTRL_RES_12_BITS;
	} else if (sequence->resolution == 10) {
		reg |= MCHP_ADC_SAR_CTRL_RES_10_BITS;
		reg |= MCHP_ADC_SAR_CTRL_SHIFTD_EN;
	} else {
		return -EINVAL;
	}

	adc_regs->sar_control_reg = reg;

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_xec_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_xec_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_xec_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
static int adc_xec_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_xec_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_xec_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static void xec_adc_get_sample(const struct device *dev)
{
	struct adc_xec_regs *adc_regs = ADC_XEC_REG_BASE;
	struct adc_xec_data *data = dev->data;
	uint32_t idx;
	uint32_t channels = adc_regs->status_reg;
	uint32_t ch_status = channels;
	uint32_t bit;

	/*
	 * Using the enabled channel bit set, from
	 * lowest channel number to highest, find out
	 * which channel is enabled and copy the ADC
	 * values from hardware registers to the data
	 * buffer.
	 */
	bit = find_lsb_set(channels);
	while (bit != 0) {
		idx = bit - 1;

		*data->buffer = (uint16_t)adc_regs->channel_read_reg[idx];
		data->buffer++;

		channels &= ~BIT(idx);
		bit = find_lsb_set(channels);
	}

	/* Clear the status register */
	adc_regs->status_reg = ch_status;
}

static void adc_xec_isr(const struct device *dev)
{
	struct adc_xec_regs *adc_regs = ADC_XEC_REG_BASE;
	struct adc_xec_data *data = dev->data;
	uint32_t reg;

	/* Clear START_SINGLE bit and clear SINGLE_DONE_STATUS */
	reg = adc_regs->control_reg;
	reg &= ~XEC_ADC_CTRL_START_SINGLE;
	reg |= XEC_ADC_CTRL_SINGLE_DONE_STATUS;
	adc_regs->control_reg = reg;

	/* Also clear GIRQ source status bit */
	MCHP_GIRQ_SRC(MCHP_ADC_GIRQ) = MCHP_ADC_SNG_DONE_GIRQ_VAL;

	xec_adc_get_sample(dev);

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("ADC ISR triggered.");
}

struct adc_driver_api adc_xec_api = {
	.channel_setup = adc_xec_channel_setup,
	.read = adc_xec_read,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_xec_read_async,
#endif
	.ref_internal = XEC_ADC_VREF_ANALOG,
};

static int adc_xec_init(const struct device *dev)
{
	struct adc_xec_regs *adc_regs = ADC_XEC_REG_BASE;
	struct adc_xec_data *data = dev->data;

	adc_regs->control_reg =  XEC_ADC_CTRL_ACTIVATE
		| XEC_ADC_CTRL_POWER_SAVER_DIS
		| XEC_ADC_CTRL_SINGLE_DONE_STATUS
		| XEC_ADC_CTRL_REPEAT_DONE_STATUS;

	MCHP_GIRQ_SRC(MCHP_ADC_GIRQ) = MCHP_ADC_SNG_DONE_GIRQ_VAL;
	MCHP_GIRQ_ENSET(MCHP_ADC_GIRQ) = MCHP_ADC_SNG_DONE_GIRQ_VAL;

	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    adc_xec_isr, DEVICE_GET(adc_xec), 0);
	irq_enable(DT_INST_IRQN(0));

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct adc_xec_data adc_xec_dev_data_0 = {
	ADC_CONTEXT_INIT_TIMER(adc_xec_dev_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_xec_dev_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_xec_dev_data_0, ctx),
};

DEVICE_AND_API_INIT(adc_xec, DT_INST_LABEL(0),
		    adc_xec_init, &adc_xec_dev_data_0, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adc_xec_api);
