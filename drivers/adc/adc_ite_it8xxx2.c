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
#include <drivers/adc/adc_vcmp_ite_it8xxx2.h>
#include <drivers/pinmux.h>
#include <soc.h>
#include <soc_dt.h>
#include <errno.h>
#include <assert.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* ADC internal reference voltage (Unit:mV) */
#define IT8XXX2_ADC_VREF_VOL 3000
/* ADC channels disabled */
#define IT8XXX2_ADC_CHANNEL_DISABLED 0x1F

/* ADC VCMP */
#define IT8XXX2_ADC_CMPXEN		BIT(7)
#define IT8XXX2_ADC_CMPXINTEN		BIT(6)
#define IT8XXX2_ADC_CMPXTMOD		BIT(5)
#define IT8XXX2_ADC_CMPXELSM		BIT(4)
#define IT8XXX2_ADC_CMPXGPOL		BIT(3)
#define IT8XXX2_ADC_CMPCXSELL_MASK	0x7

#define IT8XXX2_ADC_CMPCXSELM		BIT(0)

#define VCMPCTL(base,idx)	ECREG(&base->VCMP0CTL + (idx * 3))
#define VCMPTHRDATM(base,idx)	ECREG(&base->VCMP0THRDATM + (idx * 3))
#define VCMPTHRDATL(base,idx)	ECREG(&base->VCMP0THRDATL + (idx * 3))
#define VCMPCSELM(base,idx)	ECREG(&base->VCMP0CSELM + (idx * 3))

#define IT8XXX2_ADC_VCMP_COUNT   3

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

struct adc_it8xxx2_vcmp_control {
	/*
	 * Selects ADC channel number, for which the measured data is compared
	 * for threshold detection.
	 */
	uint8_t csel;
	/*
	 * Sets relation between measured value and assetion threshold value.
	 * in thrval:
	 * 0: Threshold event is generated if Measured data > thrval.
	 * 1: Threshold event is generated if Measured data <= thrval.
	 */
	bool tmod;
	/* Sets the threshold value to which measured data is compared. */
	uint16_t thrdat;
	/*
	 * Pointer of work queue thread to be notified when threshold assertion
	 * occurs.
	 */
	struct k_work *work;
};

struct adc_it8xxx2_vcmp_data {
        /*
	 * If ADC read is requested on a channle being used as comparator,
	 * we need to resume to comparator operation once ADC capture
	 * is completed. This variable keeps track of current channels being
	 * used as comparators.
         */
	uint16_t cmp_channels;

	uint8_t cmp_enabled;
	/* This structure holds comparatort configurations */
	struct adc_it8xxx2_vcmp_control control[IT8XXX2_ADC_VCMP_COUNT];
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
	/* Comparator data */
	struct adc_it8xxx2_vcmp_data *vcmp_data;
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

static struct adc_it8xxx2_vcmp_data adc_it8xxx2_vcmp_data_0;

#define ADC_IT8XXX2_REG_BASE	\
	((struct adc_it8xxx2_regs *)(DT_INST_REG_ADDR(0)))

static int adc_it8xxx2_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_it8xxx2_cfg *config = dev->config;

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

}

static void adc_disable_measurement(void)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	/*
	 * Disable measurement.
	 * bit(4:0) = 0x1f : channel disable
	 */
	adc_regs->VCH0CTL = IT8XXX2_ADC_DATVAL | IT8XXX2_ADC_CHANNEL_DISABLED;

	adc_regs->VCH0CTL &= ~IT8XXX2_ADC_INTDVEN;

}

static void adc_enable(bool enable)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	if (enable) {
		/* Enable adc interrupt */
		irq_enable(DT_INST_IRQN(0));

		/* ADC module enable */
		adc_regs->ADCCFG |= IT8XXX2_ADC_ADCEN;
	} else {
		/* ADC module disable */
		adc_regs->ADCCFG &= ~IT8XXX2_ADC_ADCEN;

		/* disable adc interrupt */
		irq_disable(DT_INST_IRQN(0));
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

static int adc_it8xxx2_start_read(const struct device *dev,
				  const struct adc_sequence *sequence)
{
	struct adc_it8xxx2_data *data = dev->data;
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

	adc_enable(true);

}

static int adc_it8xxx2_read(const struct device *dev,
			    const struct adc_sequence *sequence)
{
	struct adc_it8xxx2_data *data = dev->data;
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

static void adc_it8xxx2_isr(const void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_vcmp_data *v_data = data->vcmp_data;
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	LOG_DBG("ADC ISR triggered.");

	if (adc_regs->VCH0CTL & (IT8XXX2_ADC_DATVAL | IT8XXX2_ADC_INTDVEN))  {
		/* Read adc raw data of msb and lsb */
		*data->buffer++ = adc_regs->VCH0DATM << 8 | adc_regs->VCH0DATL;

		adc_disable_measurement();

		if (IS_ENABLED(CONFIG_ADC_THRESHOLD_IRQ) &&
		    v_data->cmp_enabled) {
			/* TODO: Handle restoring active ADC comparator */
		} else {
			adc_enable(false);
		}
		adc_context_on_sampling_done(&data->ctx, dev);
	}

	if (!IS_ENABLED(CONFIG_ADC_THRESHOLD_IRQ)) {
		return;
	}

	for (int i = 0; i < IT8XXX2_ADC_VCMP_COUNT; i++) {
		uint8_t vcmpctl = VCMPCTL(adc_regs, i);
		uint8_t vcmpsts = adc_regs->VCMPSTS;

		/* Check comparator interruptions */
		if (vcmpctl & (IT8XXX2_ADC_CMPXEN | IT8XXX2_ADC_CMPXINTEN) &&
		    vcmpsts & BIT(i)) {
			if (v_data->control[i].work) {
				k_work_submit(v_data->control[i].work);
			}
		}
		/* Clear cmp threshold interruption status bit */
		adc_regs->VCMPSTS = vcmpsts;
	}
}

int adc_vcmp_it8xxx2_set_scan_period(const struct device *dev,
				     const enum vcmp_scan_period scan_period)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	ARG_UNUSED(dev);

	if (scan_period < VCMP_SCAN_PERIOD_100US ||
	    scan_period > VCMP_SCAN_PERIOD_5MS) {
		return -ENOTSUP;
	}

	adc_regs->VCMPSCP &= ~(0xF << 4);
	adc_regs->VCMPSCP |= scan_period & (0xF << 4);
	return 0;
}


int adc_vcmp_it8xxx2_ctrl_set_param(const struct device *dev,
				    const uint8_t vcmp,
				    const struct adc_vcmp_it8xxx2_vcmp_control_t
				    *control)
{
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_vcmp_data *const v_data = data->vcmp_data;
	struct adc_it8xxx2_vcmp_control *const v_ctrl =
					&v_data->control[vcmp];
	int ret = 0;

	if (!IS_ENABLED(ADC_VCMP_ITE_IT8XXX2)) {
		return -EOPNOTSUPP;
	}

	if (!control || vcmp >= IT8XXX2_ADC_VCMP_COUNT) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	switch (control->param) {
	case ADC_VCMP_ITE_IT8XXX2_PARAM_CSELL:
		if (control->val > CHIP_ADC_COUNT) {
			ret = -EINVAL;
			break;
		}
		v_ctrl->csel = (uint8_t)control->val;
		break;

	case ADC_VCMP_ITE_IT8XXX2_PARAM_TMOD:
		v_ctrl->tmod = !!control->val;
		break;

	case ADC_VCMP_ITE_IT8XXX2_PARAM_THRDAT:
		if (control->val == 0 || control->val >= ADC_REF_INTERNAL) {
			ret = -EINVAL;
			break;
		}
		v_ctrl->thrdat = (uint16_t)control->val;
		break;

	case ADC_VCMP_ITE_IT8XXX2_PARAM_WORK:
		if (control->val == 0) {
			ret = -EINVAL;
			break;
		}
		v_ctrl->work = (struct k_work *)control->val;
		break;
	default:
		ret = -EINVAL;
	}
	adc_context_release(&data->ctx, 0);
	return ret;
}

static int adc_vcmp_it8xxx2_enable_irq(const struct device *dev,
				       const uint8_t vcmp)
{
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_vcmp_data *v_data = data->vcmp_data;
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	struct adc_it8xxx2_vcmp_control *const v_ctrl =
					&v_data->control[vcmp];

	if (vcmp >= IT8XXX2_ADC_VCMP_COUNT) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	if (v_data->cmp_enabled & BIT(vcmp)) {
		/* Comparator irq is already enabled */
		adc_context_release(&data->ctx, 0);
		return 0;
	}

	if (v_ctrl->csel >= CHIP_ADC_COUNT ||
	    v_ctrl->thrdat == 0 || v_ctrl->work == 0) {
		adc_context_release(&data->ctx, 0);
		LOG_ERR("Threshold selected (%d) is not configured!", vcmp);
		return -EINVAL;
	}
	/* Set channel being used as comparator */
	v_data->cmp_channels |= BIT(v_ctrl->csel);

	if (!data->vcmp_data->cmp_enabled)
		adc_enable(true);

	v_data->cmp_enabled |= BIT(vcmp);

	/* Enable comparator and interruption */
	VCMPCTL(adc_regs, vcmp) |= (IT8XXX2_ADC_CMPXEN | IT8XXX2_ADC_CMPXINTEN);

	adc_context_release(&data->ctx, 0);

	return 0;
}

static int adc_vcmp_it8xxx2_disable_irq(const struct device *dev,
					const uint8_t vcmp)
{
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_vcmp_data *v_data = data->vcmp_data;
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	struct adc_it8xxx2_vcmp_control *const v_ctrl =
					&v_data->control[vcmp];

	if (vcmp >= IT8XXX2_ADC_VCMP_COUNT) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	if (!(v_data->cmp_enabled & BIT(vcmp))) {
		/* Comparator irq is already disabled */
		adc_context_release(&data->ctx, 0);
		return 0;
	}
	/* Clear comparator bit */
	v_data->cmp_channels &= ~BIT(v_ctrl->csel);

	v_data->cmp_enabled &= ~BIT(vcmp);

	if (!data->vcmp_data->cmp_enabled)
		adc_enable(false);

	/* Disable comparator and interruption */
	VCMPCTL(adc_regs, vcmp) &=
			~(IT8XXX2_ADC_CMPXEN | IT8XXX2_ADC_CMPXINTEN);

	adc_context_release(&data->ctx, 0);

	return 0;
}

static int adc_vcmp_it8xxx2_ctrl_setup(const struct device *dev,
				       const uint8_t vcmp)
{
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_vcmp_data *const v_data = data->vcmp_data;
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	struct adc_it8xxx2_vcmp_control *const v_ctrl =
					&v_data->control[vcmp];

	uint8_t vcmpctl;

	if (vcmp >= IT8XXX2_ADC_VCMP_COUNT) {
		return -EINVAL;
	}

	adc_context_lock(&data->ctx, false, NULL);
	/* Set LSB comparator channel selection */
	vcmpctl = (v_ctrl->csel & IT8XXX2_ADC_CMPCXSELL_MASK);

	/* Set edge comparator sense mode */
	vcmpctl &= ~IT8XXX2_ADC_CMPXELSM;

	if (v_ctrl->tmod) {
		vcmpctl |= IT8XXX2_ADC_CMPXTMOD;
	} else {
		vcmpctl &= ~IT8XXX2_ADC_CMPXTMOD;
	}

	/* Set comparator threshold value */
	VCMPTHRDATM(adc_regs, vcmp) = (uint8_t)(v_ctrl->thrdat >> 8);
	VCMPTHRDATL(adc_regs, vcmp) = (uint8_t)v_ctrl->thrdat;

	VCMPCTL(adc_regs, vcmp) = vcmpctl;

	/* Set MSB comparator channel selection */
	if (v_ctrl->csel & ~IT8XXX2_ADC_CMPCXSELL_MASK) {
		VCMPCSELM(adc_regs, vcmp) |= IT8XXX2_ADC_CMPCXSELM;
	} else {
		VCMPCSELM(adc_regs, vcmp) &= ~IT8XXX2_ADC_CMPCXSELM;
	}

	adc_context_release(&data->ctx, 0);

	return 0;
}

int adc_vcmp_it8xxx2_ctrl_enable(const struct device *dev, uint8_t vcmp,
				 const bool enable)
{
	int ret;

	if (!IS_ENABLED(ADC_VCMP_ITE_IT8XXX2)) {
		return -EOPNOTSUPP;
	}

	/* Enable/Disable threshold IRQ */
	if (enable) {
		/* Set control threshold registers */
		ret = adc_vcmp_it8xxx2_ctrl_setup(dev, vcmp);
		if (ret) {
			return ret;
		}
		ret = adc_vcmp_it8xxx2_enable_irq(dev, vcmp);
	} else {
		ret = adc_vcmp_it8xxx2_disable_irq(dev, vcmp);
	}
	return ret;
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
	struct adc_it8xxx2_data *data = dev->data;
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

	if (IS_ENABLED(CONFIG_ADC_THRESHOLD_IRQ)) {
		data->vcmp_data = &adc_it8xxx2_vcmp_data_0;
	}

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

static const struct adc_it8xxx2_cfg adc_it8xxx2_cfg_0[CHIP_ADC_COUNT] =
	IT8XXX2_DT_ALT_ITEMS_LIST(0);

DEVICE_DT_INST_DEFINE(0, adc_it8xxx2_init,
		      NULL,
		      &adc_it8xxx2_data_0,
		      &adc_it8xxx2_cfg_0, PRE_KERNEL_1,
		      CONFIG_ADC_INIT_PRIORITY,
		      &api_it8xxx2_driver_api);
