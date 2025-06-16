/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_nct_adc

#include <assert.h>
#include <zephyr/drivers/adc.h>
#include <common/reg/reg_def.h>
#include <common/reg/reg_access.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <soc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_nct, CONFIG_ADC_LOG_LEVEL);

enum adc_channel_en {
	CHEN0_AVSB = 0,
	CHEN1_VSB,
	CHEN2_VCC,
	CHEN3_VHIF,
	CHEN4_VIN7,
	CHEN5_VIN5,
	CHEN6_VIN16,
	CHEN7_THR16,
	CHEN8_VIN15,
	CHEN9_THR15,
	CHEN10_VIN14,
	CHEN11_THR14,
	CHEN12_VIN1,
	CHEN13_THR1,
	CHEN14_VIN2,
	CHEN15_THR2,
	CHEN16_VIN3,
	CHEN17_VTT,
	CHEN18_VBAT,
	CHEN19_TD2P,
	CHEN20_TD1P,
	CHEN21_TD0P,
	CHEN22_TD3P,
	CHEN23_TD4P
};

/* Device config */
struct adc_nct_config {
	/* adc controller base address */
	uintptr_t base;
	/* the number of ADC channels */
	const uint8_t channel_count;
	/* routine for configuring ADC's ISR */
	void (*irq_cfg_func)(void);
	const struct pinctrl_dev_config *pcfg;
};

/* Driver data */
struct adc_nct_data {
	/* mutex of ADC channels */
	struct adc_context ctx;
	uint8_t CurChannel_Idx;
	/*
	 * Bit-mask indicating the channels to be included in each sampling
	 * of this sequence.
	 */
	uint32_t channels;
	/* ADC Device pointer used in api functions */
	const struct device *adc_dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	/* end pointer of buffer to ensure enough space for storing ADC data. */
	uint16_t *buf_end;
};

/* Driver convenience defines */
#define HAL_INSTANCE(dev) ((struct adc_reg *)((const struct adc_nct_config *)(dev)->config)->base)

static int adc_nct_validate_buffer_size(const struct device *dev,
					    const struct adc_sequence *sequence)
{
	const struct adc_nct_config *config = dev->config;
	uint8_t channels = 0;
	uint32_t mask;
	size_t needed;

	for (mask = BIT(config->channel_count - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(uint16_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOSPC;
	}

	return 0;
}

static void nct_adc_start_convert(const struct device *dev)
{
	struct adc_reg *const adc_regs = HAL_INSTANCE(dev);
	struct adc_nct_data *const data = dev->data;
	uint8_t IsDiode;

	IsDiode = false;

	data->CurChannel_Idx = find_lsb_set(data->channels) - 1;

	/* Set voltage large than 2.048V */
	adc_regs->ADCACTRL1 &= ~MaskBit(NCT_ACTRL1_PWCTRL);

	/* Start ADC scan conversion */
	adc_regs->DSADCCTRL0 &= ~0x1F; /* CH_SEL Mask */

	if ((data->CurChannel_Idx >= CHEN19_TD2P) && (data->CurChannel_Idx <= CHEN23_TD4P)) {
		IsDiode = true;
		/* Set voltage smaller than 2.048V */
		adc_regs->ADCACTRL1 |= MaskBit(NCT_ACTRL1_PWCTRL);
	}
	if (IsDiode) {
		adc_regs->DSADCCTRL0 &= ~MaskBit(NCT_CTRL0_VNT);
		switch (data->CurChannel_Idx) {
		case CHEN19_TD2P:
			adc_regs->DSADCCTRL0 |= 0x00;
			adc_regs->ADCTM &= ~(0x03 << NCT_TM_T_MODE1);
			break;
		case CHEN20_TD1P:
			adc_regs->DSADCCTRL0 |= 0x01;
			adc_regs->ADCTM &= ~(0x03 << NCT_TM_T_MODE2);
			break;
		case CHEN21_TD0P:
			adc_regs->DSADCCTRL0 |= 0x02;
			adc_regs->ADCTM &= ~(0x03 << NCT_TM_T_MODE3);
			break;
		case CHEN22_TD3P:
			adc_regs->DSADCCTRL0 |= 0x03;
			adc_regs->ADCTM &= ~(0x03 << NCT_TM_T_MODE4);
			break;
		case CHEN23_TD4P:
			adc_regs->DSADCCTRL0 |= 0x04;
			adc_regs->ADCTM &= ~(0x03 << NCT_TM_T_MODE5);
			break;
		default:
			break;
		}
	} else   {
		adc_regs->DSADCCTRL0 |= MaskBit(NCT_CTRL0_VNT);
		if ((data->CurChannel_Idx >= CHEN4_VIN7) && (data->CurChannel_Idx <= CHEN18_VBAT)) {
			switch (data->CurChannel_Idx) {
			case CHEN4_VIN7:
				adc_regs->DSADCCTRL0 |= 0x06;
				break;
			case CHEN5_VIN5:
				adc_regs->DSADCCTRL0 |= 0x07;
				break;
			case CHEN6_VIN16:
				adc_regs->DSADCCTRL0 |= 0x08;
				adc_regs->DSADCCTRL6 |= MaskBit(0);
				break;
			case CHEN7_THR16:
				adc_regs->DSADCCTRL0 |= 0x08;
				adc_regs->DSADCCTRL6 &= ~MaskBit(0);
				/* Set voltage smaller than 2.048V */
				adc_regs->ADCACTRL1 |= MaskBit(NCT_ACTRL1_PWCTRL);
				break;
			case CHEN8_VIN15:
				adc_regs->DSADCCTRL0 |= 0x09;
				adc_regs->DSADCCTRL6 |= MaskBit(1);
				break;
			case CHEN9_THR15:
				adc_regs->DSADCCTRL0 |= 0x09;
				adc_regs->DSADCCTRL6 &= ~MaskBit(1);
				/* Set voltage smaller than 2.048V */
				adc_regs->ADCACTRL1 |= MaskBit(NCT_ACTRL1_PWCTRL);
				break;
			case CHEN10_VIN14:
				adc_regs->DSADCCTRL0 |= 0x0A;
				adc_regs->DSADCCTRL6 |= MaskBit(2);
				break;
			case CHEN11_THR14:
				adc_regs->DSADCCTRL0 |= 0x0A;
				adc_regs->DSADCCTRL6 &= ~MaskBit(2);
				/* Set voltage smaller than 2.048V */
				adc_regs->ADCACTRL1 |= MaskBit(NCT_ACTRL1_PWCTRL);
				break;
			case CHEN12_VIN1:
				adc_regs->DSADCCTRL0 |= 0x0B;
				adc_regs->DSADCCTRL6 |= MaskBit(3);
				break;
			case CHEN13_THR1:
				adc_regs->DSADCCTRL0 |= 0x0B;
				adc_regs->DSADCCTRL6 &= ~MaskBit(3);
				/* Set voltage smaller than 2.048V */
				adc_regs->ADCACTRL1 |= MaskBit(NCT_ACTRL1_PWCTRL);
				break;
			case CHEN14_VIN2:
				adc_regs->DSADCCTRL0 |= 0x0C;
				adc_regs->DSADCCTRL6 |= MaskBit(4);
				break;
			case CHEN15_THR2:
				adc_regs->DSADCCTRL0 |= 0x0C;
				adc_regs->DSADCCTRL6 &= ~MaskBit(4);
				/* Set voltage smaller than 2.048V */
				adc_regs->ADCACTRL1 |= MaskBit(NCT_ACTRL1_PWCTRL);
				break;
			case CHEN16_VIN3:
				adc_regs->DSADCCTRL0 |= 0x0D;
				break;
			case CHEN17_VTT:
				adc_regs->DSADCCTRL0 |= 0x0E;
				break;
			case CHEN18_VBAT:
				adc_regs->DSADCCTRL0 |= 0x0F;
				break;

			}
		} else   {
			adc_regs->DSADCCTRL0 |= data->CurChannel_Idx;
		}

	}

	/* clear sts */
	adc_regs->DSADCSTS = adc_regs->DSADCSTS;
	/* enable interrupt(ADC_ICEN) and start */
	adc_regs->DSADCCFG |= (MaskBit(NCT_CFG_ICEN) | MaskBit(NCT_CFG_START));
}

/* ADC local functions */
static void adc_nct_isr(const struct device *dev)
{
	struct adc_reg *const adc_regs = HAL_INSTANCE(dev);
	struct adc_nct_data *const data = dev->data;
	uint16_t result, channel;

	/* clear sts */
	adc_regs->DSADCSTS = adc_regs->DSADCSTS;

	/* get data */
	channel = data->channels;
	result = adc_regs->TCHNDAT;
	result &= ~MaskBit(NCT_TCHNDATA_NEW);
	if ((data->CurChannel_Idx >= CHEN0_AVSB) && (data->CurChannel_Idx <= CHEN18_VBAT)) {
		if ((data->CurChannel_Idx == CHEN7_THR16) ||
		    (data->CurChannel_Idx == CHEN9_THR15) ||
		    (data->CurChannel_Idx == CHEN11_THR14) ||
		    (data->CurChannel_Idx == CHEN13_THR1) ||
		    (data->CurChannel_Idx == CHEN15_THR2)) {

			result = (result << 5);
		} else   {
			result <<= 1; /* X2 for large than 2.048V by Spec.*/
		}
	} else   {
		/* TDxP */
		result = (result << 5);
	}

	if (data->buffer < data->buf_end) {
		*data->buffer++ = result;
	}
	data->channels &= ~BIT(data->CurChannel_Idx);

	if (data->channels) {
		nct_adc_start_convert(dev);
	} else   {
		/* Inform sampling is done */
		adc_context_on_sampling_done(&data->ctx, data->adc_dev);
	}

}

static int adc_nct_start_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	const struct adc_nct_config *config = dev->config;
	struct adc_nct_data *const data = dev->data;
	int error = 0;

	if (!sequence->channels ||
	    (sequence->channels & ~BIT_MASK(config->channel_count))) {
		LOG_ERR("Invalid ADC channels");
		return -EINVAL;
	}

	/* Fixed 10 bit resolution of nct ADC */
	if (sequence->resolution != 10) {
		LOG_ERR("Unfixed 10 bit ADC resolution");
		return -ENOTSUP;
	}

	error = adc_nct_validate_buffer_size(dev, sequence);
	if (error) {
		LOG_ERR("ADC buffer size too small");
		return error;
	}

	/* Save ADC sequence sampling buffer and its end pointer address */
	data->buffer = sequence->buffer;
	data->buf_end = data->buffer + sequence->buffer_size / sizeof(uint16_t);

	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);
	error = adc_context_wait_for_completion(&data->ctx);

	return error;
}

/* ADC api functions */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_nct_data *const data =
		CONTAINER_OF(ctx, struct adc_nct_data, ctx);

	data->repeat_buffer = data->buffer;
	data->channels = ctx->sequence.channels;

	if (!data->channels) {
		LOG_ERR("No ADC channel can start sampling!!");
	} else   {
		nct_adc_start_convert(data->adc_dev);
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_nct_data *const data =
		CONTAINER_OF(ctx, struct adc_nct_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_nct_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_nct_config *config = dev->config;
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id >= config->channel_count) {
		LOG_ERR("Invalid channel %d", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference");
		return -ENOTSUP;
	}

	return 0;
}

static int adc_nct_read(const struct device *dev,
			    const struct adc_sequence *sequence)
{
	struct adc_nct_data *const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = adc_nct_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
static int adc_nct_read_async(const struct device *dev,
				  const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	struct adc_nct_data *const data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = adc_nct_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_nct_init(const struct device *dev)
{
	const struct adc_nct_config *const config = dev->config;
	struct adc_nct_data *const data = ((struct adc_nct_data *)(dev)->data);
	int ret;

	/* Save ADC device in data */
	data->adc_dev = dev;

	/* Configure ADC interrupt and enable it */
	config->irq_cfg_func();

	/* Initialize mutex of ADC channels */
	adc_context_unlock_unconditionally(&data->ctx);

	/* Configure pin-mux for ADC device */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("ADC pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define NCT_ADC_INIT(n)							\
										\
	static void adc_nct_irq_cfg_func_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    adc_nct_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static const struct adc_driver_api adc_nct_driver_api_##n = {		\
		.channel_setup = adc_nct_channel_setup,			\
		.read = adc_nct_read,						\
		.ref_internal = DT_INST_PROP(n, vref_mv),			\
		IF_ENABLED(CONFIG_ADC_ASYNC,					\
			(.read_async = adc_nct_read_async,))			\
	};									\
										\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	static const struct adc_nct_config adc_nct_cfg_##n = {		\
		.base = DT_INST_REG_ADDR(n),					\
		.channel_count = DT_INST_PROP(n, channel_count),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.irq_cfg_func = adc_nct_irq_cfg_func_##n,			\
	};									\
	static struct adc_nct_data adc_nct_data_##n = {			\
		ADC_CONTEXT_INIT_TIMER(adc_nct_data_##n, ctx),			\
		ADC_CONTEXT_INIT_LOCK(adc_nct_data_##n, ctx),			\
		ADC_CONTEXT_INIT_SYNC(adc_nct_data_##n, ctx),			\
	};									\
	DEVICE_DT_INST_DEFINE(n,						\
			     adc_nct_init, NULL,				\
			     &adc_nct_data_##n, &adc_nct_cfg_##n,		\
			     PRE_KERNEL_1, CONFIG_ADC_INIT_PRIORITY,		\
			     &adc_nct_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(NCT_ADC_INIT)
