/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_adc

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_ite_it8xxx2);

#include <drivers/adc.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DEV_DATA(dev) ((struct adc_it8xxx2_data * const)(dev)->data)

/* Data structure to define ADC channel control registers. */
struct adc_ctrl_t {
	/* The voltage channel control register. */
	volatile uint8_t *adc_ctrl;
	/* The voltage channel data buffer MSB. */
	volatile uint8_t *adc_datm;
	/* The voltage channel data buffer LSB. */
	volatile uint8_t *adc_datl;
};

/* Data structure of ADC channel control registers. */
static const struct adc_ctrl_t adc_ctrl_regs[] = {
	{&IT83XX_ADC_VCH0CTL, &IT83XX_ADC_VCH0DATM, &IT83XX_ADC_VCH0DATL},
	{&IT83XX_ADC_VCH1CTL, &IT83XX_ADC_VCH1DATM, &IT83XX_ADC_VCH1DATL},
	{&IT83XX_ADC_VCH2CTL, &IT83XX_ADC_VCH2DATM, &IT83XX_ADC_VCH2DATL},
	{&IT83XX_ADC_VCH3CTL, &IT83XX_ADC_VCH3DATM, &IT83XX_ADC_VCH3DATL},
	{&IT83XX_ADC_VCH4CTL, &IT83XX_ADC_VCH4DATM, &IT83XX_ADC_VCH4DATL},
	{&IT83XX_ADC_VCH5CTL, &IT83XX_ADC_VCH5DATM, &IT83XX_ADC_VCH5DATL},
	{&IT83XX_ADC_VCH6CTL, &IT83XX_ADC_VCH6DATM, &IT83XX_ADC_VCH6DATL},
	{&IT83XX_ADC_VCH7CTL, &IT83XX_ADC_VCH7DATM, &IT83XX_ADC_VCH7DATL},
};

/* List of ADC channels. */
enum chip_adc_channel {
	CHIP_ADC_CH0 = 0,
	CHIP_ADC_CH1,
	CHIP_ADC_CH2,
	CHIP_ADC_CH3,
	CHIP_ADC_CH4,
	CHIP_ADC_CH5,
	CHIP_ADC_CH6,
	CHIP_ADC_CH7,
	CHIP_ADC_COUNT,
};

struct adc_it8xxx2_data {
	struct adc_context ctx;
	/* Save ADC result to the buffer. */
	uint16_t *buffer;
	/*
	 * The sample buffer pointer should be prepared
	 * for writing of next sampling results.
	 */
	uint16_t *repeat_buffer;
};

static int adc_it8xxx2_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= CHIP_ADC_COUNT) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	if (channel_cfg->channel_id < CHIP_ADC_CH4) {
		/* For channel 0 ~ 3 control register. */
		*adc_ctrl_regs[channel_cfg->channel_id].adc_ctrl =
			(IT83XX_ADC_DATVAL | IT83XX_ADC_INTDVEN) +
			channel_cfg->channel_id;
	} else {
		/* For channel 4 ~ 7 control register. */
		*adc_ctrl_regs[channel_cfg->channel_id].adc_ctrl =
			IT83XX_ADC_DATVAL | IT83XX_ADC_INTDVEN | IT83XX_ADC_VCHEN;
	}
	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static uint8_t count_channels(uint8_t ch)
{
	uint8_t count = 0;
	uint8_t bit;

	bit = find_lsb_set(ch);
	while (bit != 0) {
		uint8_t idx = bit - 1;

		ch &= ~BIT(idx);
		bit = find_lsb_set(ch);
		count++;
	}

	return count;
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

static int adc_it8xxx2_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	uint8_t channels = sequence->channels;
	uint8_t channel_count;
	int err;

	if (!channels || channels & ~BIT_MASK(CHIP_ADC_COUNT)) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (!sequence->resolution) {
		LOG_ERR("ADC resolution is not valid");
		return -EINVAL;
	}
	LOG_DBG("Configure resolution=%d", sequence->resolution);

	channel_count = count_channels(channels);
	err = check_buffer_size(sequence, channel_count);
	if (err) {
		return err;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_it8xxx2_data *data =
		CONTAINER_OF(ctx, struct adc_it8xxx2_data, ctx);

	data->repeat_buffer = data->buffer;

	/* enable adc interrupt */
	irq_enable(DT_INST_IRQN(0));

	/* ADC module enable */
	IT83XX_ADC_ADCCFG |= IT83XX_ADC_ADCEN;
}

static int adc_it8xxx2_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	int err;

	adc_context_lock(&data->ctx, false, NULL);
	err = adc_it8xxx2_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_it8xxx2_data *data =
		CONTAINER_OF(ctx, struct adc_it8xxx2_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_data_valid(enum chip_adc_channel adc_ch)
{
	return IT83XX_ADC_ADCDVSTS & BIT(adc_ch);
}

/* Get result for each ADC selected channel. */
static void adc_it8xxx2_get_sample(const struct device *dev)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	uint8_t channels = data->ctx.sequence.channels;
	uint8_t bit;
	bool valid = false;

	bit = find_lsb_set(channels);
	while (bit != 0) {
		uint8_t idx = bit - 1;

		if (adc_data_valid(idx)) {
			/* Read adc raw data of msb and lsb */
			uint16_t val = (*adc_ctrl_regs[idx].adc_datm << 8) |
						*adc_ctrl_regs[idx].adc_datl;
			/* Raw data multiply resolution. */
			*data->buffer++ = val * data->ctx.sequence.resolution;

			/* W/C data valid flag */
			IT83XX_ADC_ADCDVSTS = BIT(idx);
			valid = 1;
		}

		if (!valid) {
			LOG_WRN("ADC failed to read (regs=%x, ch=%d)",
				IT83XX_ADC_ADCDVSTS, idx);
		}

		channels &= ~BIT(idx);
		bit = find_lsb_set(channels);
	}

	/* ADC module disable */
	IT83XX_ADC_ADCCFG &= ~IT83XX_ADC_ADCEN;
	/* disable adc interrupt */
	irq_disable(DT_INST_IRQN(0));

}

static void adc_it8xxx2_isr(const void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_it8xxx2_data *data = DEV_DATA(dev);

	LOG_DBG("ADC ISR triggered.");

	adc_it8xxx2_get_sample(dev);

	adc_context_on_sampling_done(&data->ctx, dev);
}

static const struct adc_driver_api api_it8xxx2_driver_api = {
	.channel_setup = adc_it8xxx2_channel_setup,
	.read = adc_it8xxx2_read,
};

/*
 * ADC analog accuracy initialization (only once after VSTBY power on)
 *
 * Write 1 to this bit and write 0 to this bit immediately once and
 * only once during the firmware initialization and do not write 1 again
 * after initialization since IT83xx takes much power consumption
 * if this bit is set as 1
 */
static void adc_accuracy_initialization(void)
{
	/* Start adc accuracy initialization */
	IT83XX_ADC_ADCSTS |= IT83XX_ADC_AINITB;
	/* Enable automatic HW calibration. */
	IT83XX_ADC_KDCTL |= IT83XX_ADC_AHCE;
	/* short delay for adc accuracy initialization */
	IT83XX_GCTRL_WNCKR = 0;
	/* Stop adc accuracy initialization */
	IT83XX_ADC_ADCSTS &= ~IT83XX_ADC_AINITB;
}

static int adc_it8xxx2_init(const struct device *dev)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);

	/* ADC analog accuracy initialization */
	adc_accuracy_initialization();

	/*
	 * The ADC channel conversion time is 30.8*(SCLKDIV+1) us.
	 * (Current setting is 61.6us)
	 *
	 * NOTE: A sample time delay (60us) also need to be included in
	 * conversion time, so the final result is ~= 121.6us.
	 */
	IT83XX_ADC_ADCSTS &= ~IT83XX_ADC_ADCCTS1;
	IT83XX_ADC_ADCCFG &= ~IT83XX_ADC_ADCCTS0;
	/*
	 * bit[5-0]@ADCCTL : SCLKDIV
	 * SCLKDIV has to be equal to or greater than 1h;
	 */
	IT83XX_ADC_ADCCTL = 1;

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    adc_it8xxx2_isr, DEVICE_DT_INST_GET(0), 0);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct adc_it8xxx2_data adc_it8xxx2_data_0 = {
		ADC_CONTEXT_INIT_TIMER(adc_it8xxx2_data_0, ctx),
		ADC_CONTEXT_INIT_LOCK(adc_it8xxx2_data_0, ctx),
		ADC_CONTEXT_INIT_SYNC(adc_it8xxx2_data_0, ctx),
};
DEVICE_DT_INST_DEFINE(0, adc_it8xxx2_init,
				device_pm_control_nop,
				&adc_it8xxx2_data_0,
				NULL, POST_KERNEL,
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
				&api_it8xxx2_driver_api);
