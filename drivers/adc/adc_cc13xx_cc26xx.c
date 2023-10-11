/*
 * Copyright (c) 2022 Florin Stancu <niflostancu@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_adc

#include <errno.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_cc13xx_cc26xx);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/adc.h>
#include <soc.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <driverlib/interrupt.h>
#include <driverlib/ioc.h>
#include <driverlib/rom.h>
#include <driverlib/prcm.h>
#include <driverlib/aux_adc.h>
#include <ti/devices/cc13x2_cc26x2/inc/hw_aux_evctl.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"


/**
 * Channels are based on ADC_COMPB_IN_* hal_ti definitions, max. index is 16 (included).
 */
#define MAX_CHAN_ID 0x10

/** Internal sample time unit conversion entry. */
struct adc_cc13xx_cc26xx_sample_time_entry {
	uint16_t time_us;
	uint8_t reg_value;
};

/** Maps standard unit sample times (us) to internal (raw hal_ti register) values */
static const struct adc_cc13xx_cc26xx_sample_time_entry adc_cc13xx_sample_times[] = {
	{ 2, AUXADC_SAMPLE_TIME_2P7_US },
	{ 5, AUXADC_SAMPLE_TIME_5P3_US },
	{ 10, AUXADC_SAMPLE_TIME_10P6_US },
	{ 21, AUXADC_SAMPLE_TIME_21P3_US },
	{ 42, AUXADC_SAMPLE_TIME_42P6_US },
	{ 85, AUXADC_SAMPLE_TIME_85P3_US },
	{ 170, AUXADC_SAMPLE_TIME_170_US },
	{ 341, AUXADC_SAMPLE_TIME_341_US },
	{ 682, AUXADC_SAMPLE_TIME_682_US },
	{ 1370, AUXADC_SAMPLE_TIME_1P37_MS },
	{ 2730, AUXADC_SAMPLE_TIME_2P73_MS },
	{ 5460, AUXADC_SAMPLE_TIME_5P46_MS },
	{ 10900, AUXADC_SAMPLE_TIME_10P9_MS },
};

struct adc_cc13xx_cc26xx_data {
	struct adc_context ctx;
	const struct device *dev;
	uint32_t ref_source;
	uint8_t sample_time;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
};

struct adc_cc13xx_cc26xx_cfg {
	unsigned long base;
	void (*irq_cfg_func)(void);
};


static void adc_cc13xx_cc26xx_isr(const struct device *dev);


static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_cc13xx_cc26xx_data *data =
		CONTAINER_OF(ctx, struct adc_cc13xx_cc26xx_data, ctx);

	data->repeat_buffer = data->buffer;

	AUXADCEnableSync(data->ref_source, data->sample_time, AUXADC_TRIGGER_MANUAL);
	AUXADCGenManualTrigger();
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct adc_cc13xx_cc26xx_data *data =
		CONTAINER_OF(ctx, struct adc_cc13xx_cc26xx_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer++;
	}
}

static int adc_cc13xx_cc26xx_init(const struct device *dev)
{
	struct adc_cc13xx_cc26xx_data *data = dev->data;
	const struct adc_cc13xx_cc26xx_cfg *config = dev->config;

	data->dev = dev;

	/* clear any previous events */
	AUXADCDisable();
	HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOMCUFLAGSCLR) =
		(AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_IRQ | AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_DONE);

	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static int adc_cc13xx_cc26xx_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	struct adc_cc13xx_cc26xx_data *data = dev->data;
	const uint8_t ch = channel_cfg->channel_id;
	uint16_t sample_time_us = 0;
	uint8_t i;

	if (ch > MAX_CHAN_ID) {
		LOG_ERR("Channel 0x%X is not supported, max 0x%X", ch, MAX_CHAN_ID);
		return -EINVAL;
	}

	switch (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time)) {
	case ADC_ACQ_TIME_TICKS:
		data->sample_time = (uint16_t)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
		break;
	case ADC_ACQ_TIME_MICROSECONDS:
		sample_time_us = (uint16_t)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		sample_time_us = (uint16_t)(
				ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time) * 1000);
		break;
	default:
		data->sample_time = AUXADC_SAMPLE_TIME_170_US;
		break;
	}
	if (sample_time_us) {
		/* choose the nearest sample time configuration */
		data->sample_time = adc_cc13xx_sample_times[0].reg_value;
		for (i = 0; i < ARRAY_SIZE(adc_cc13xx_sample_times); i++) {
			if (adc_cc13xx_sample_times[i].time_us >= sample_time_us) {
				break;
			}
			data->sample_time = adc_cc13xx_sample_times[i].reg_value;
		}
		if (i >= ARRAY_SIZE(adc_cc13xx_sample_times)) {
			LOG_ERR("Acquisition time is not valid");
			return -EINVAL;
		}
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid");
		return -EINVAL;
	}

	if (channel_cfg->reference == ADC_REF_INTERNAL) {
		data->ref_source = AUXADC_REF_FIXED;
	} else if (channel_cfg->reference == ADC_REF_VDD_1) {
		data->ref_source = AUXADC_REF_VDDS_REL;
	} else {
		LOG_ERR("Reference is not valid");
		return -EINVAL;
	}

	LOG_DBG("Setup %d acq time %d", ch, data->sample_time);

	AUXADCDisable();
	AUXADCSelectInput(ch);
	return 0;
}

static int cc13xx_cc26xx_read(const struct device *dev,
		       const struct adc_sequence *sequence,
		       bool asynchronous,
		       struct k_poll_signal *sig)
{
	struct adc_cc13xx_cc26xx_data *data = dev->data;
	int rv;
	size_t exp_size;

	if (sequence->resolution != 12) {
		LOG_ERR("Only 12 Resolution is supported, but %d got",
			sequence->resolution);
		return -EINVAL;
	}

	exp_size = sizeof(uint16_t);
	if (sequence->options) {
		exp_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < exp_size) {
		LOG_ERR("Required buffer size is %u, but %u got",
			exp_size, sequence->buffer_size);
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;

	adc_context_lock(&data->ctx, asynchronous, sig);
	adc_context_start_read(&data->ctx, sequence);
	rv = adc_context_wait_for_completion(&data->ctx);
	adc_context_release(&data->ctx, rv);
	return rv;
}

static int adc_cc13xx_cc26xx_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	return cc13xx_cc26xx_read(dev, sequence, false, NULL);
}

#ifdef CONFIG_ADC_ASYNC
static int adc_cc13xx_cc26xx_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	return cc13xx_cc26xx_read(dev, sequence, true, async);
}
#endif

/**
 * AUX_ADC_IRQ handler, called for either of these events:
 * - conversion complete or DMA done (if used);
 * - FIFO underflow or overflow;
 */
static void adc_cc13xx_cc26xx_isr(const struct device *dev)
{
	struct adc_cc13xx_cc26xx_data *data = dev->data;
	/* get the statuses of ADC_DONE and ADC_IRQ events in order to clear them both */
	uint32_t ev_status = (
		HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOMCUFLAGS) &
		(AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_IRQ | AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_DONE)
	);
	uint32_t fifo_status;
	uint32_t adc_value;

	/* clear the AUXADC-related event flags */
	HWREG(AUX_EVCTL_BASE + AUX_EVCTL_O_EVTOMCUFLAGSCLR) = ev_status;
	/* check the ADC FIFO's status */
	fifo_status = AUXADCGetFifoStatus();
	LOG_DBG("ISR flags 0x%08X fifo 0x%08X", ev_status, fifo_status);
	if ((fifo_status & (AUX_ANAIF_ADCFIFOSTAT_OVERFLOW | AUX_ANAIF_ADCFIFOSTAT_UNDERFLOW))) {
		AUXADCFlushFifo();
	}
	if ((fifo_status & AUX_ANAIF_ADCFIFOSTAT_EMPTY_M)) {
		/* no ADC values available */
		return;
	}
	adc_value = AUXADCPopFifo();
	LOG_DBG("ADC buf %04X val %d", (unsigned int)data->buffer, adc_value);
	*data->buffer = adc_value;
	AUXADCDisable();

	adc_context_on_sampling_done(&data->ctx, dev);
}

static const struct adc_driver_api cc13xx_cc26xx_driver_api = {
	.channel_setup = adc_cc13xx_cc26xx_channel_setup,
	.read = adc_cc13xx_cc26xx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_cc13xx_cc26xx_read_async,
#endif
	.ref_internal = 4300, /* fixed reference: 4.3V */
};

#define CC13XX_CC26XX_ADC_INIT(index)						 \
	static void adc_cc13xx_cc26xx_cfg_func_##index(void);			 \
	static const struct adc_cc13xx_cc26xx_cfg adc_cc13xx_cc26xx_cfg_##index = { \
		.base = DT_INST_REG_ADDR(index),				 \
		.irq_cfg_func = adc_cc13xx_cc26xx_cfg_func_##index,		 \
	};									 \
	static struct adc_cc13xx_cc26xx_data adc_cc13xx_cc26xx_data_##index = { \
		ADC_CONTEXT_INIT_TIMER(adc_cc13xx_cc26xx_data_##index, ctx),	 \
		ADC_CONTEXT_INIT_LOCK(adc_cc13xx_cc26xx_data_##index, ctx),	 \
		ADC_CONTEXT_INIT_SYNC(adc_cc13xx_cc26xx_data_##index, ctx),	 \
	};									 \
	DEVICE_DT_INST_DEFINE(index,						 \
		&adc_cc13xx_cc26xx_init, NULL,					 \
		&adc_cc13xx_cc26xx_data_##index,				 \
		&adc_cc13xx_cc26xx_cfg_##index, POST_KERNEL,			 \
		CONFIG_ADC_INIT_PRIORITY,					 \
		&cc13xx_cc26xx_driver_api);					 \
										 \
	static void adc_cc13xx_cc26xx_cfg_func_##index(void)			 \
	{									 \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority),	 \
				adc_cc13xx_cc26xx_isr, DEVICE_DT_INST_GET(index), 0); \
		irq_enable(DT_INST_IRQN(index));				 \
	}

DT_INST_FOREACH_STATUS_OKAY(CC13XX_CC26XX_ADC_INIT)
