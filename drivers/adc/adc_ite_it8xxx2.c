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
#include <drivers/pinmux.h>
#include <soc.h>
#include <errno.h>
#include <assert.h>
#include <dt-bindings/pinctrl/it8xxx2-pinctrl.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DEV_DATA(dev) ((struct adc_it8xxx2_data * const)(dev)->data)

#define DEV_CFG(dev) ((struct adc_it8xxx2_cfg * const)(dev)->config)

#define DEV_PIN(adc_ch)      DT_PHA(DT_PHANDLE_BY_IDX(DT_DRV_INST(0), \
	pinctrl_0, adc_ch), pinctrls, pin)
#define DEV_ALT_FUNC(adc_ch) DT_PHA(DT_PHANDLE_BY_IDX(DT_DRV_INST(0), \
	pinctrl_0, adc_ch), pinctrls, alt_func)
#define DEV_PINMUX(adc_ch)   DEVICE_DT_GET(DT_PHANDLE_BY_IDX(DT_NODELABEL \
	(pinctrl_adc##adc_ch), pinctrls, 0))

/* ADC internal reference voltage (Unit:mV) */
#define IT8XXX2_ADC_VREF_VOL 3000
/* ADC channels disabled */
#define IT8XXX2_ADC_CHANNEL_DISABLED 0x1F

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
	/* Channel ID */
	uint32_t ch;
	/* Save ADC result to the buffer. */
	uint16_t *buffer;
	/*
	 * The sample buffer pointer should be prepared
	 * for writing of next sampling results.
	 */
	uint16_t *repeat_buffer;
};

/*
 * Strcture adc_it8xxx2_cfg is about the setting of adc
 * this config will be used at initial time
 */
struct adc_it8xxx2_cfg {
	/* Pinmux control group */
	const struct device *pinctrls;
	/* GPIO pin */
	uint8_t pin;
	/* Alternate function */
	uint8_t alt_fun;
};

#define ADC_IT8XXX2_REG_BASE	\
	((struct adc_it8xxx2_regs *)(DT_INST_REG_ADDR(0)))

static int adc_it8xxx2_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	struct adc_it8xxx2_cfg *config = DEV_CFG(dev);

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

	/* The channel is set to ADC alternate function */
	pinmux_pin_set(config[channel_cfg->channel_id].pinctrls,
		config[channel_cfg->channel_id].pin,
		config[channel_cfg->channel_id].alt_fun);
	LOG_DBG("Channel setup succeeded!");
	return 0;
}

static void adc_enable_measurement(uint32_t ch)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	/* Select and enable a voltage channel input for measurement */
	adc_regs->VCH0CTL = (IT8XXX2_ADC_DATVAL | IT8XXX2_ADC_INTDVEN) + ch;

	/* Enable adc interrupt */
	irq_enable(DT_INST_IRQN(0));

	/* ADC module enable */
	adc_regs->ADCCFG |= IT8XXX2_ADC_ADCEN;
}

static void adc_disable_measurement(void)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	/*
	 * Disable measurement.
	 * bit(4:0) = 0x1f : channel disable
	 */
	adc_regs->VCH0CTL = IT8XXX2_ADC_DATVAL | IT8XXX2_ADC_CHANNEL_DISABLED;

	/* ADC module disable */
	adc_regs->ADCCFG &= ~IT8XXX2_ADC_ADCEN;

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
	uint32_t channel_mask = sequence->channels;

	if (!channel_mask || channel_mask & ~BIT_MASK(CHIP_ADC_COUNT)) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (!sequence->resolution) {
		LOG_ERR("ADC resolution is not valid");
		return -EINVAL;
	}
	LOG_DBG("Configure resolution=%d", sequence->resolution);

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_it8xxx2_data *data =
		CONTAINER_OF(ctx, struct adc_it8xxx2_data, ctx);

	data->repeat_buffer = data->buffer;

	adc_enable_measurement(data->ch);
}

static int adc_it8xxx2_read(const struct device *dev,
			    const struct adc_sequence *sequence)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	uint32_t channel_mask = sequence->channels;
	uint8_t channel_count = 0;
	int err = 0;

	data->buffer = sequence->buffer;

	while (channel_mask) {
		adc_context_lock(&data->ctx, false, NULL);
		data->ch = find_lsb_set(channel_mask) - 1;

		err = adc_it8xxx2_start_read(dev, sequence);
		if (err) {
			return err;
		}

		channel_mask &= ~BIT(data->ch);
		channel_count++;
		adc_context_release(&data->ctx, err);
	}

	err = check_buffer_size(sequence, channel_count);

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

/* Get result for each ADC selected channel. */
static void adc_it8xxx2_get_sample(const struct device *dev)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	bool valid = false;

	if (adc_regs->VCH0CTL & IT8XXX2_ADC_DATVAL) {
		/* Read adc raw data of msb and lsb */
		*data->buffer++ = adc_regs->VCH0DATM << 8 | adc_regs->VCH0DATL;

		valid = 1;
	}

	if (!valid) {
		LOG_WRN("ADC failed to read (regs=%x, ch=%d)",
			adc_regs->ADCDVSTS, data->ch);
	}

	adc_disable_measurement();
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
	.ref_internal = IT8XXX2_ADC_VREF_VOL,
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
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	/* Start adc accuracy initialization */
	adc_regs->ADCSTS |= IT8XXX2_ADC_AINITB;
	/* Enable automatic HW calibration. */
	adc_regs->KDCTL |= IT8XXX2_ADC_AHCE;
	/* Stop adc accuracy initialization */
	adc_regs->ADCSTS &= ~IT8XXX2_ADC_AINITB;
}

static int adc_it8xxx2_init(const struct device *dev)
{
	struct adc_it8xxx2_data *data = DEV_DATA(dev);
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	/* ADC analog accuracy initialization */
	adc_accuracy_initialization();

	/*
	 * The ADC channel conversion time is 30.8*(SCLKDIV+1) us.
	 * (Current setting is 61.6us)
	 *
	 * NOTE: A sample time delay (60us) also need to be included in
	 * conversion time, so the final result is ~= 121.6us.
	 */
	adc_regs->ADCSTS &= ~IT8XXX2_ADC_ADCCTS1;
	adc_regs->ADCCFG &= ~IT8XXX2_ADC_ADCCTS0;
	/*
	 * bit[5-0]@ADCCTL : SCLKDIV
	 * SCLKDIV has to be equal to or greater than 1h;
	 */
	adc_regs->ADCCTL = 1;
	/*
	 * Enable this bit, and data of VCHxDATL/VCHxDATM will be
	 * kept until data valid is cleared.
	 */
	adc_regs->ADCGCR |= IT8XXX2_ADC_DBKEN;

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
#define ITE_DT_ITEMS_BY_CH(adc_ch, _)        \
	{                                    \
	  .pin = DEV_PIN(adc_ch),            \
	  .alt_fun = DEV_ALT_FUNC(adc_ch),   \
	  .pinctrls = DEV_PINMUX(adc_ch),    \
	},

static const struct adc_it8xxx2_cfg adc_it8xxx2_cfg_0[CHIP_ADC_COUNT] = {
	UTIL_LISTIFY(DT_INST_PROP_LEN(0, pinctrl_0), ITE_DT_ITEMS_BY_CH, _)
};
DEVICE_DT_INST_DEFINE(0, adc_it8xxx2_init,
		      NULL,
		      &adc_it8xxx2_data_0,
		      &adc_it8xxx2_cfg_0, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &api_it8xxx2_driver_api);
