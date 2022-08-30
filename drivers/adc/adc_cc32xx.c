/*
 * Copyright (c) 2021 Pavlo Hamov <pasha.gamov@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc32xx_adc

#include <errno.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>

/* Driverlib includes */
#include <inc/hw_types.h>
#include <driverlib/pin.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/prcm.h>
#include <driverlib/adc.h>

#define CHAN_COUNT 4

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_cc32xx);

#define ISR_MASK (ADC_DMA_DONE | ADC_FIFO_OVERFLOW | ADC_FIFO_UNDERFLOW	\
		  | ADC_FIFO_EMPTY | ADC_FIFO_FULL)

struct adc_cc32xx_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t offset[CHAN_COUNT];
	size_t active_channels;
};

struct adc_cc32xx_cfg {
	unsigned long base;
	void (*irq_cfg_func)(void);
};

static const int s_chPin[CHAN_COUNT] = {
	PIN_57,
	PIN_58,
	PIN_59,
	PIN_60,
};

static const int s_channel[CHAN_COUNT] = {
	ADC_CH_0,
	ADC_CH_1,
	ADC_CH_2,
	ADC_CH_3,
};

static inline void start_sampling(unsigned long base, int ch)
{
	MAP_ADCChannelEnable(base, ch);
	for (int i = 0; i < 5; i++) {
		while (!MAP_ADCFIFOLvlGet(base, ch)) {
		}
		MAP_ADCFIFORead(base, ch);
	}
	MAP_ADCIntClear(base, ch, ISR_MASK);
	MAP_ADCIntEnable(base, ch, ISR_MASK);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_cc32xx_data *data =
		CONTAINER_OF(ctx, struct adc_cc32xx_data, ctx);
	const struct adc_cc32xx_cfg *config = data->dev->config;

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	for (int i = 0; i < CHAN_COUNT; ++i) {
		if (ctx->sequence.channels & BIT(i)) {
			start_sampling(config->base, s_channel[i]);
		}
	}
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct adc_cc32xx_data *data =
		CONTAINER_OF(ctx, struct adc_cc32xx_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer += data->active_channels;
	}
}

static int adc_cc32xx_init(const struct device *dev)
{
	struct adc_cc32xx_data *data = dev->data;
	const struct adc_cc32xx_cfg *config = dev->config;

	data->dev = dev;

	LOG_DBG("Initializing....");

	for (int i = 0; i < CHAN_COUNT; ++i) {
		const int ch = s_channel[i];

		MAP_ADCIntDisable(config->base, ch, ISR_MASK);
		MAP_ADCChannelDisable(config->base, ch);
		MAP_ADCDMADisable(config->base, ch);
		MAP_ADCIntClear(config->base, ch, ISR_MASK);
	}
	MAP_ADCEnable(config->base);
	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static int adc_cc32xx_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_cc32xx_cfg *config = dev->config;
	const uint8_t ch = channel_cfg->channel_id;

	if (ch >= CHAN_COUNT) {
		LOG_ERR("Channel %d is not supported, max %d", ch, CHAN_COUNT);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Reference is not valid");
		return -EINVAL;
	}

	LOG_DBG("Setup %d", ch);

	MAP_ADCChannelDisable(config->base, s_channel[ch]);
	MAP_ADCIntDisable(config->base, s_channel[ch], ISR_MASK);
	MAP_PinDirModeSet(s_chPin[ch], PIN_DIR_MODE_IN);
	MAP_PinTypeADC(s_chPin[ch], PIN_MODE_255);

	return 0;
}

static int cc32xx_read(const struct device *dev,
		       const struct adc_sequence *sequence,
		       bool asynchronous,
		       struct k_poll_signal *sig)
{
	struct adc_cc32xx_data *data = dev->data;
	int rv;
	size_t exp_size;

	if (sequence->resolution != 12) {
		LOG_ERR("Only 12 Resolution is supported, but %d got",
			sequence->resolution);
		return -EINVAL;
	}

	data->active_channels = 0;
	for (int i = 0; i < CHAN_COUNT; ++i) {
		if (!(sequence->channels & BIT(i))) {
			continue;
		}
		data->offset[i] = data->active_channels++;
	}
	exp_size = data->active_channels * sizeof(uint16_t);
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

static int adc_cc32xx_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	return cc32xx_read(dev, sequence, false, NULL);
}

#ifdef CONFIG_ADC_ASYNC
static int adc_cc32xx_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	return cc32xx_read(dev, sequence, true, async);
}
#endif

static void adc_cc32xx_isr(const struct device *dev, int no)
{
	const struct adc_cc32xx_cfg *config = dev->config;
	struct adc_cc32xx_data *data = dev->data;
	const int chan = s_channel[no];
	unsigned long mask = MAP_ADCIntStatus(config->base, chan);
	int cnt = 0;
	int rv = 0;

	MAP_ADCIntClear(config->base, chan, mask);

	if ((mask & ADC_FIFO_EMPTY) || !(mask & ADC_FIFO_FULL)) {
		return;
	}

	while (MAP_ADCFIFOLvlGet(config->base, chan)) {
		rv += (MAP_ADCFIFORead(config->base, chan) >> 2) & 0x0FFF;
		cnt++;
	}

	*(data->buffer + data->offset[no]) = rv / cnt;
	data->channels &= ~BIT(no);

	MAP_ADCIntDisable(config->base, chan, ISR_MASK);
	MAP_ADCChannelDisable(config->base, chan);

	LOG_DBG("ISR %d, 0x%lX %d %d", chan, mask, rv, cnt);
	if (!data->channels) {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static void adc_cc32xx_isr_ch0(const struct device *dev)
{
	adc_cc32xx_isr(dev, 0);
}

static void adc_cc32xx_isr_ch1(const struct device *dev)
{
	adc_cc32xx_isr(dev, 1);
}

static void adc_cc32xx_isr_ch2(const struct device *dev)
{
	adc_cc32xx_isr(dev, 2);
}

static void adc_cc32xx_isr_ch3(const struct device *dev)
{
	adc_cc32xx_isr(dev, 3);
}

static const struct adc_driver_api cc32xx_driver_api = {
	.channel_setup = adc_cc32xx_channel_setup,
	.read = adc_cc32xx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_cc32xx_read_async,
#endif
	.ref_internal = 1467,
};

#define cc32xx_ADC_IRQ_CONNECT(index, chan)			       \
	do {							       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(index, chan, irq),      \
			    DT_INST_IRQ_BY_IDX(index, chan, priority), \
			    adc_cc32xx_isr_ch##chan,		       \
			    DEVICE_DT_INST_GET(index), 0);	       \
		irq_enable(DT_INST_IRQ_BY_IDX(index, chan, irq));      \
	} while (false)

#define cc32xx_ADC_INIT(index)							 \
										 \
	static void adc_cc32xx_cfg_func_##index(void);				 \
										 \
	static const struct adc_cc32xx_cfg adc_cc32xx_cfg_##index = {		 \
		.base = DT_INST_REG_ADDR(index),				 \
		.irq_cfg_func = adc_cc32xx_cfg_func_##index,			 \
	};									 \
	static struct adc_cc32xx_data adc_cc32xx_data_##index = {		 \
		ADC_CONTEXT_INIT_TIMER(adc_cc32xx_data_##index, ctx),		 \
		ADC_CONTEXT_INIT_LOCK(adc_cc32xx_data_##index, ctx),		 \
		ADC_CONTEXT_INIT_SYNC(adc_cc32xx_data_##index, ctx),		 \
	};									 \
										 \
	DEVICE_DT_INST_DEFINE(index,						 \
			      &adc_cc32xx_init, NULL, &adc_cc32xx_data_##index,	 \
			      &adc_cc32xx_cfg_##index, POST_KERNEL,		 \
			      CONFIG_ADC_INIT_PRIORITY,				 \
			      &cc32xx_driver_api);				 \
										 \
	static void adc_cc32xx_cfg_func_##index(void)				 \
	{									 \
		cc32xx_ADC_IRQ_CONNECT(index, 0);				 \
		cc32xx_ADC_IRQ_CONNECT(index, 1);				 \
		cc32xx_ADC_IRQ_CONNECT(index, 2);				 \
		cc32xx_ADC_IRQ_CONNECT(index, 3);				 \
	}

DT_INST_FOREACH_STATUS_OKAY(cc32xx_ADC_INIT)
