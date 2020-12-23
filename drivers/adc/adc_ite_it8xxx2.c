/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
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

#define DEV_DATA(dev) \
	((struct adc_it8xxx2_data * const)(dev)->data)

/* External channels (maximum). */
#define IT8XXX2_CHANNEL_COUNT 8

/* Data structure to define ADC channel control registers. */
struct adc_ctrl_t {
	volatile uint8_t *adc_ctrl;
	volatile uint8_t *adc_datm;
	volatile uint8_t *adc_datl;
};

/* Data structure of ADC channel control registers. */
const struct adc_ctrl_t adc_ctrl_regs[] = {
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
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t adc_ch;
	uint8_t resolution;
};
static void adc_it8xxx2_isr(const void *arg);

static int adc_it8xxx2_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);

	data->adc_ch = channel_cfg->channel_id;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -EINVAL;
	}

	if (data->adc_ch >= IT8XXX2_CHANNEL_COUNT) {
		LOG_ERR("Channel %d is not valid", data->adc_ch);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	if (data->adc_ch < CHIP_ADC_CH4) {
		/*
		 * for channel 0, 1, 2, and 3
		 * bit4 ~ bit0 : indicates voltage channel[x]
		 *               input is selected for measurement (enable)
		 * bit5 : data valid interrupt of adc.
		 * bit7 : W/C data valid flag
		 */
		*adc_ctrl_regs[data->adc_ch].adc_ctrl = 0xa0 + data->adc_ch;
	} else {
		/*
		 * for channel 4 ~ 7.
		 * bit4 : voltage channel enable (ch 4~7)
		 * bit5 : data valid interrupt of adc.
		 * bit7 : W/C data valid flag
		 */
		*adc_ctrl_regs[data->adc_ch].adc_ctrl = 0xb0;
	}

	irq_connect_dynamic(DT_INST_IRQN(0), 0, adc_it8xxx2_isr, dev, 0);
	/* enable adc interrupt */
	irq_enable(DT_INST_IRQN(0));

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static void adc_disable_channel(const struct device *dev, uint8_t ch)
{
	if (ch < CHIP_ADC_CH4) {
		/*
		 * for channel 0, 1, 2, and 3
		 * bit4 ~ bit0 : indicates voltage channel[x]
		 *               input is selected for measurement (disable)
		 * bit 7 : W/C data valid flag
		 */
		*adc_ctrl_regs[ch].adc_ctrl = 0x9F;
	} else {
		/*
		 * for channel 4 ~ 7.
		 * bit4 : voltage channel disable (ch 4~7)
		 * bit7 : W/C data valid flag
		 */
		*adc_ctrl_regs[ch].adc_ctrl = 0x80;
	}

	/* bit 0 : adc module disable */
	IT83XX_ADC_ADCCFG &= ~0x01;

	/* disable adc interrupt */
	irq_disable(DT_INST_IRQN(0));
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
	int err;

	data->resolution = sequence->resolution;
	LOG_DBG("Configure resolution=%d", sequence->resolution);

	if (!sequence->channels ||
		sequence->channels & ~BIT_MASK(IT8XXX2_CHANNEL_COUNT)) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	err = check_buffer_size(sequence, 1);
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

	/* bit 0 : adc module enable */
	IT83XX_ADC_ADCCFG |= 0x01;
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

static void adc_it8xxx2_get_sample(const struct device *dev)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	uint8_t valid = 0;

	if (adc_data_valid(data->adc_ch)) {

		*data->buffer = ((*adc_ctrl_regs[data->adc_ch].adc_datm << 8) +
						*adc_ctrl_regs[data->adc_ch].adc_datl) *
						data->resolution;

		data->buffer++;

		/* W/C data valid flag */
		IT83XX_ADC_ADCDVSTS = BIT(data->adc_ch);
		valid = 1;
	}

	if (!valid) {
		printk("ADC failed to read!!! (regs=%x, ch=%d)",
			IT83XX_ADC_ADCDVSTS,
			data->adc_ch);
	}
	/* disable adc interrupt */
	adc_disable_channel(dev, data->adc_ch);
}

static void adc_it8xxx2_isr(const void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_it8xxx2_data *data = DEV_DATA(dev);

	adc_it8xxx2_get_sample(dev);

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("ADC ISR triggered.");
}

struct adc_driver_api api_it8xxx2_driver_api = {
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
	/* bit3 : start adc accuracy initialization */
	IT83XX_ADC_ADCSTS |= 0x08;
	/* Enable automatic HW calibration. */
	IT83XX_ADC_KDCTL |= IT83XX_ADC_AHCE;
	/* short delay for adc accuracy initialization */
	IT83XX_GCTRL_WNCKR = 0;
	/* bit3 : stop adc accuracy initialization */
	IT83XX_ADC_ADCSTS &= ~0x08;
}

static int adc_it8xxx2_init(const struct device *dev)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);

	/* ADC analog accuracy initialization */
	adc_accuracy_initialization();

	/*
	 * bit7@ADCSTS     : ADCCTS1 = 0
	 * bit5@ADCCFG     : ADCCTS0 = 0
	 * bit[5-0]@ADCCTL : SCLKDIV
	 * The ADC channel conversion time is 30.8*(SCLKDIV+1) us.
	 * (Current setting is 61.6us)
	 *
	 * NOTE: A sample time delay (60us) also need to be included in
	 * conversion time, so the final result is ~= 121.6us.
	 */
	IT83XX_ADC_ADCSTS &= ~BIT(7);
	IT83XX_ADC_ADCCFG &= ~BIT(5);
	IT83XX_ADC_ADCCTL = 1;

	/* disable adc interrupt */
	irq_disable(DT_INST_IRQN(0));

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define ADC_ITE_IT8XXX2_INIT(idx)					\
	static struct adc_it8xxx2_data adc_it8xxx2_data_##idx = {	\
		ADC_CONTEXT_INIT_TIMER(adc_it8xxx2_data_##idx, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc_it8xxx2_data_##idx, ctx),	\
		ADC_CONTEXT_INIT_SYNC(adc_it8xxx2_data_##idx, ctx),	\
	};								\
	DEVICE_DT_INST_DEFINE(idx,					\
				adc_it8xxx2_init,			\
				device_pm_control_nop,			\
				&adc_it8xxx2_data_##idx,		\
				NULL,					\
				POST_KERNEL,				\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
				&api_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_ITE_IT8XXX2_INIT)
