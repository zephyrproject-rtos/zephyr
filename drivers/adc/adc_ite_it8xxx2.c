/*
 * Copyright (c) 2021 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_adc

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_ite_it8xxx2);

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <soc_dt.h>
#include <errno.h>
#include <assert.h>
#include <zephyr/irq.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* ADC internal reference voltage (Unit:mV) */
#ifdef CONFIG_ADC_IT8XXX2_VOL_FULL_SCALE
#define IT8XXX2_ADC_VREF_VOL 3300
#else
#define IT8XXX2_ADC_VREF_VOL 3000
#endif
/* ADC channels disabled */
#define IT8XXX2_ADC_CHANNEL_DISABLED 0x1F
/* ADC sample time delay (Unit:us) */
#define IT8XXX2_ADC_SAMPLE_TIME_US 500
/* Wait next clock rising (Clock source 32.768K) */
#define IT8XXX2_WAIT_NEXT_CLOCK_TIME_US 31
/* ADC channels offset */
#define ADC_CHANNEL_SHIFT 5
#define ADC_CHANNEL_OFFSET(ch) ((ch)-CHIP_ADC_CH13-ADC_CHANNEL_SHIFT)

#ifdef CONFIG_ADC_IT8XXX2_VOL_FULL_SCALE
#define ADC_0_7_FULL_SCALE_MASK   GENMASK(7, 0)
#define ADC_8_10_FULL_SCALE_MASK  GENMASK(2, 0)
#define ADC_13_16_FULL_SCALE_MASK GENMASK(3, 0)
#endif

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
	CHIP_ADC_CH13,
	CHIP_ADC_CH14,
	CHIP_ADC_CH15,
	CHIP_ADC_CH16,
	CHIP_ADC_COUNT,
};

struct adc_it8xxx2_data {
	struct adc_context ctx;
	struct k_sem sem;
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
 * Structure adc_it8xxx2_cfg is about the setting of adc
 * this config will be used at initial time
 */
struct adc_it8xxx2_cfg {
	/* ADC alternate configuration */
	const struct pinctrl_dev_config *pcfg;
};

#define ADC_IT8XXX2_REG_BASE	\
	((struct adc_it8xxx2_regs *)(DT_INST_REG_ADDR(0)))

static int adc_it8xxx2_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	/* Support channels 0~7 and 13~16 */
	if (!((channel_id >= 0 && channel_id <= 7) ||
	    (channel_id >= 13 && channel_id <= 16))) {
		LOG_ERR("Channel %d is not valid", channel_id);
		return -EINVAL;
	}

	/* Channels 13~16 should be shifted by 5 */
	if (channel_id > CHIP_ADC_CH7) {
		channel_id -= ADC_CHANNEL_SHIFT;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	LOG_DBG("Channel setup succeeded!");
	return 0;
}

static void adc_disable_measurement(uint32_t ch)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	if (ch <= CHIP_ADC_CH7) {
		/*
		 * Disable measurement.
		 * bit(4:0) = 0x1f : channel disable
		 */
		adc_regs->VCH0CTL = IT8XXX2_ADC_DATVAL |
			IT8XXX2_ADC_CHANNEL_DISABLED;
	} else {
		/*
		 * Channels 13~16 controller setting.
		 * bit7 = 1: End of conversion. New data is available in
		 *           VCHDATL/VCHDATM.
		 */
		adc_regs->adc_vchs_ctrl[ADC_CHANNEL_OFFSET(ch)].VCHCTL =
			IT8XXX2_ADC_DATVAL;
	}

	/* ADC module disable */
	adc_regs->ADCCFG &= ~IT8XXX2_ADC_ADCEN;

	/* disable adc interrupt */
	irq_disable(DT_INST_IRQN(0));
}

static int adc_data_valid(const struct device *dev)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	struct adc_it8xxx2_data *data = dev->data;

	return (data->ch <= CHIP_ADC_CH7) ?
		(adc_regs->VCH0CTL & IT8XXX2_ADC_DATVAL) :
		(adc_regs->ADCDVSTS2 & BIT(ADC_CHANNEL_OFFSET(data->ch)));
}

/* Get result for each ADC selected channel. */
static void adc_it8xxx2_get_sample(const struct device *dev)
{
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;

	if (adc_data_valid(dev)) {
		if (data->ch <= CHIP_ADC_CH7) {
			/* Read adc raw data of msb and lsb */
			*data->buffer++ = adc_regs->VCH0DATM << 8 |
				adc_regs->VCH0DATL;
		} else {
			/* Read adc channels 13~16 raw data of msb and lsb */
			*data->buffer++ =
				adc_regs->adc_vchs_ctrl[ADC_CHANNEL_OFFSET(data->ch)].VCHDATM << 8 |
				adc_regs->adc_vchs_ctrl[ADC_CHANNEL_OFFSET(data->ch)].VCHDATL;
		}
	} else {
		LOG_WRN("ADC failed to read (regs=%x, ch=%d)",
			adc_regs->ADCDVSTS, data->ch);
	}

	adc_disable_measurement(data->ch);
}

static void adc_poll_valid_data(void)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	int valid = 0;

	/*
	 * If the polling waits for a valid data longer than
	 * the sampling time limit, the program will return.
	 */
	for (int i = 0U; i < (IT8XXX2_ADC_SAMPLE_TIME_US /
	     IT8XXX2_WAIT_NEXT_CLOCK_TIME_US); i++) {
		/* Wait next clock time (1/32.768K~=30.5us) */
		k_busy_wait(IT8XXX2_WAIT_NEXT_CLOCK_TIME_US);

		if (adc_data_valid(dev)) {
			valid = 1;
			break;
		}
	}

	if (valid) {
		adc_it8xxx2_get_sample(dev);
	} else {
		LOG_ERR("Sampling timeout.");
		return;
	}
}

static void adc_enable_measurement(uint32_t ch)
{
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	struct adc_it8xxx2_data *data = dev->data;

	if (ch <= CHIP_ADC_CH7) {
		/* Select and enable a voltage channel input for measurement */
		adc_regs->VCH0CTL = (IT8XXX2_ADC_DATVAL | IT8XXX2_ADC_INTDVEN) + ch;
	} else {
		/* Channels 13~16 controller setting */
		adc_regs->adc_vchs_ctrl[ADC_CHANNEL_OFFSET(ch)].VCHCTL =
			IT8XXX2_ADC_DATVAL | IT8XXX2_ADC_INTDVEN | IT8XXX2_ADC_VCHEN;
	}

	/* ADC module enable */
	adc_regs->ADCCFG |= IT8XXX2_ADC_ADCEN;

	/*
	 * In the sampling process, it is possible to read multiple channels
	 * at a time. The ADC sampling of it8xxx2 needs to read each channel
	 * in sequence, so it needs to wait for an interrupt to read data in
	 * the loop through k_sem_take(). But k_timer_start() is used in the
	 * interval test in test_adc.c, so we need to use polling wait instead
	 * of k_sem_take() to wait, otherwise it will cause kernel panic.
	 *
	 * k_is_in_isr() can determine whether to use polling or k_sem_take()
	 * at present.
	 */
	if (k_is_in_isr()) {
		/* polling wait for a valid data */
		adc_poll_valid_data();
	} else {
		/* Enable adc interrupt */
		irq_enable(DT_INST_IRQN(0));
		/* Wait for an interrupt to read valid data. */
		k_sem_take(&data->sem, K_FOREVER);
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

	/* Channels 13~16 should be shifted to the right by 5 */
	if (channel_mask > BIT(CHIP_ADC_CH7)) {
		channel_mask >>= ADC_CHANNEL_SHIFT;
	}

	if (!channel_mask || channel_mask & ~BIT_MASK(CHIP_ADC_COUNT)) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (!sequence->resolution) {
		LOG_ERR("ADC resolution is not valid");
		return -EINVAL;
	}
	LOG_DBG("Configure resolution=%d", sequence->resolution);

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_it8xxx2_data *data =
		CONTAINER_OF(ctx, struct adc_it8xxx2_data, ctx);
	uint32_t channels = ctx->sequence.channels;
	uint8_t channel_count = 0;

	data->repeat_buffer = data->buffer;

	/*
	 * The ADC sampling of it8xxx2 needs to read each channel
	 * in sequence.
	 */
	while (channels) {
		data->ch = find_lsb_set(channels) - 1;
		channels &= ~BIT(data->ch);

		adc_enable_measurement(data->ch);

		channel_count++;
	}

	if (check_buffer_size(&ctx->sequence, channel_count)) {
		return;
	}

	adc_context_on_sampling_done(&data->ctx, DEVICE_DT_INST_GET(0));
}

static int adc_it8xxx2_read(const struct device *dev,
			    const struct adc_sequence *sequence)
{
	struct adc_it8xxx2_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);
	err = adc_it8xxx2_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_it8xxx2_read_async(const struct device *dev,
				  const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	struct adc_it8xxx2_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, true, async);
	err = adc_it8xxx2_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}
#endif /* CONFIG_ADC_ASYNC */

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_it8xxx2_data *data =
		CONTAINER_OF(ctx, struct adc_it8xxx2_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_it8xxx2_isr(const struct device *dev)
{
	struct adc_it8xxx2_data *data = dev->data;

	LOG_DBG("ADC ISR triggered.");

	adc_it8xxx2_get_sample(dev);

	k_sem_give(&data->sem);
}

static const struct adc_driver_api api_it8xxx2_driver_api = {
	.channel_setup = adc_it8xxx2_channel_setup,
	.read = adc_it8xxx2_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_it8xxx2_read_async,
#endif
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
	const struct adc_it8xxx2_cfg *config = dev->config;
	struct adc_it8xxx2_data *data = dev->data;
	struct adc_it8xxx2_regs *const adc_regs = ADC_IT8XXX2_REG_BASE;
	int status;

#ifdef CONFIG_ADC_IT8XXX2_VOL_FULL_SCALE
	/* ADC input voltage 0V ~ AVCC (3.3V) is mapped into 0h-3FFh */
	adc_regs->ADCIVMFSCS1 = ADC_0_7_FULL_SCALE_MASK;
	adc_regs->ADCIVMFSCS2 = ADC_8_10_FULL_SCALE_MASK;
	adc_regs->ADCIVMFSCS3 = ADC_13_16_FULL_SCALE_MASK;
#endif
	/* ADC analog accuracy initialization */
	adc_accuracy_initialization();

	/* Set the pin to ADC alternate function. */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure ADC pins");
		return status;
	}

	/*
	 * The ADC channel conversion time is 30.8*(SCLKDIV+1) us.
	 * (Current setting is 61.6us)
	 *
	 * NOTE: A sample time delay (60us) also need to be included in
	 * conversion time.
	 * In addition, the ADC has a waiting time of 202.8us for
	 * voltage stabilization.
	 *
	 * So the final ADC sample time result is ~= 324.4us.
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

	k_sem_init(&data->sem, 0, 1);
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct adc_it8xxx2_data adc_it8xxx2_data_0 = {
		ADC_CONTEXT_INIT_TIMER(adc_it8xxx2_data_0, ctx),
		ADC_CONTEXT_INIT_LOCK(adc_it8xxx2_data_0, ctx),
		ADC_CONTEXT_INIT_SYNC(adc_it8xxx2_data_0, ctx),
};

PINCTRL_DT_INST_DEFINE(0);

static const struct adc_it8xxx2_cfg adc_it8xxx2_cfg_0 = {
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, adc_it8xxx2_init,
		      NULL,
		      &adc_it8xxx2_data_0,
		      &adc_it8xxx2_cfg_0, PRE_KERNEL_1,
		      CONFIG_ADC_INIT_PRIORITY,
		      &api_it8xxx2_driver_api);
