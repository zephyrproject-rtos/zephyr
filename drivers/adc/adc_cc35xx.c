/*
 * Copyright (c) 2025 Conclusive Engineering Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc35xx_adc
#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <soc.h>

#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/devicetree/dma.h>
#include <zephyr/drivers/dma.h>
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */

/* Driverlib includes */
#include <driverlib/adc.h>
#include <driverlib/cpu.h>
#include <inc/hw_types.h>

#define CC35XX_CHAN_COUNT   16
/* hardware supports MEMCTL0..5 for sequence */
#define CC35XX_SEQUENCE_MAX 6

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
LOG_MODULE_REGISTER(adc_cc35xx);

#define ADC_INT_CHAN_MASK                                                                          \
	(ADC_INT_MEMRES_00 | ADC_INT_MEMRES_01 | ADC_INT_MEMRES_02 | ADC_INT_MEMRES_03 |           \
	 ADC_INT_MEMRES_04 | ADC_INT_MEMRES_05 | ADC_INT_MEMRES_06 | ADC_INT_MEMRES_07 |           \
	 ADC_INT_MEMRES_08 | ADC_INT_MEMRES_09 | ADC_INT_MEMRES_10 | ADC_INT_MEMRES_11 |           \
	 ADC_INT_MEMRES_12 | ADC_INT_MEMRES_13 | ADC_INT_MEMRES_14 | ADC_INT_MEMRES_15)

#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN

struct adc_cc35xx_dma_stream {
	const struct device *dev_dma;
	uint32_t dma_channel;
	struct dma_config dma_cfg;
	struct dma_block_config blk_cfg;
};
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */

struct adc_cc35xx_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	struct {
		int vref;
	} channel_config[CC35XX_CHAN_COUNT];
	size_t active_channels;
	uint32_t clkdiv;
#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN
	struct adc_cc35xx_dma_stream dma;
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */
};

struct adc_cc35xx_cfg {
	mem_addr_t base;
	const struct pinctrl_dev_config *pcfg;
	uint32_t clkdiv;
	uint16_t sample_period;
};

#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN
static int adc_cc35xx_fifo_enable(const struct device *dev, bool enable)
{
	const struct adc_cc35xx_cfg *config = dev->config;
	uint32_t ctl2_reg = sys_read32(config->base + ADC_O_CTL2);

	if (enable) {
		ctl2_reg |= ADC_CTL2_FIFOEN_ENABLE;
	} else {
		ctl2_reg &= ~ADC_CTL2_FIFOEN_ENABLE;
	}
	sys_write32(ctl2_reg, config->base + ADC_O_CTL2);

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_cc35xx_data *data = CONTAINER_OF(ctx, struct adc_cc35xx_data, ctx);
	uint32_t channel_irq = 0;

	const struct device *dev = data->dev;
	const struct adc_cc35xx_cfg *config = data->dev->config;
	mem_addr_t irq_set_reg;
	int ret;

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;
	/* wait for interrupt from the last configured channel, so we get
	 * single interrupt once all channels are done
	 */
	channel_irq = ADC_INT_MEMRES_00 << (data->active_channels - 1);
	ADCClearInterrupt(ADC_INT_UVIFG);
	ADCDisableConversion();

	data->dma.blk_cfg.source_address = config->base + ADC_O_FIFODATA;
	data->dma.blk_cfg.dest_address = (uint32_t)data->buffer;
	data->dma.blk_cfg.block_size = sizeof(uint16_t) * data->active_channels;

	sys_write32(ADC_INT_CHAN_MASK, ADC_BASE + ADC_O_INTEVT2CLR);
	ret = dma_config(data->dma.dev_dma, data->dma.dma_channel, &data->dma.dma_cfg);
	if (ret) {
		LOG_ERR("Error configuring DMA for ADC, error: %d", ret);
		return;
	}
	ADCEnableDmaTrigger();
	ADCEnableInterrupt(ADC_INT_DMADONE);
	ret = dma_start(data->dma.dev_dma, data->dma.dma_channel);
	if (ret != 0) {
		LOG_ERR("Error start DMA for ADC, error: %d", ret);
		goto dma_error;
	}
	channel_irq |= ADC_INT_DMADONE;
	irq_set_reg = ADC_BASE + ADC_O_INTEVT2BM;

	sys_write32(channel_irq, irq_set_reg);
	adc_cc35xx_fifo_enable(dev, true);

	ADCEnableConversion();
	ADCStartConversion();

	return;
dma_error:
	ADCDisableInterrupt(ADC_INT_DMADONE);
	ADCDisableDmaTrigger();
}

static void adc_context_stop_sampling(struct adc_context *ctx)
{
	struct adc_cc35xx_data *data = CONTAINER_OF(ctx, struct adc_cc35xx_data, ctx);
	const struct device *dev = data->dev;

	ADCStopConversion();
	ADCDisableConversion();

	dma_stop(data->dma.dev_dma, data->dma.dma_channel);
	ADCDisableInterrupt(ADC_INT_DMADONE);
	ADCDisableDmaTrigger();

	adc_cc35xx_fifo_enable(dev, false);
}

#else
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_cc35xx_data *data = CONTAINER_OF(ctx, struct adc_cc35xx_data, ctx);
	uint32_t channel_irq = 0;

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;
	/* Wait for interrupt from the last configured channel, so we get
	 * single interrupt once all channels are done
	 */
	channel_irq = ADC_INT_MEMRES_00 << (data->active_channels - 1);

	ADCEnableConversion();
	ADCStartConversion();
	ADCEnableInterrupt(channel_irq);
}
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_cc35xx_data *data = CONTAINER_OF(ctx, struct adc_cc35xx_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer += data->active_channels;
	}
}

static int adc_cc35xx_get_reference(enum adc_reference zephyr_reference)
{
	switch (zephyr_reference) {
	case ADC_REF_VDD_1:
		return ADC_VDDA_REFERENCE;

	case ADC_REF_INTERNAL:
		return ADC_INTERNAL_REFERENCE;

	case ADC_REF_EXTERNAL0:
		return ADC_EXTERNAL_REFERENCE;

	default:
		return -1;
	}
}

static int adc_cc35xx_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	struct adc_cc35xx_data *data = dev->data;
	const uint8_t ch = channel_cfg->channel_id;
	int vref;

	if (ch >= CC35XX_CHAN_COUNT) {
		LOG_ERR("Channel %d is not available, hardware max channel is %d", ch,
			CC35XX_CHAN_COUNT);
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

	vref = adc_cc35xx_get_reference(channel_cfg->reference);
	if (vref == -1) {
		LOG_ERR("Reference is not valid");
		return -EINVAL;
	}

	data->channel_config[ch].vref = vref;

	return 0;
}

static int adc_cc35xx_read(const struct device *dev, const struct adc_sequence *sequence,
			   bool asynchronous, struct k_poll_signal *sig)
{
	struct adc_cc35xx_data *data = dev->data;
	const struct adc_cc35xx_cfg *config = dev->config;
	size_t exp_size;
	int ret;

	if (sequence->resolution != 12) {
		LOG_ERR("Only 12 bit resolution is supported, requested resolution is %d",
			sequence->resolution);
		return -EINVAL;
	}

#ifndef CONFIG_ADC_CC35XX_DMA_DRIVEN
	ADCDisableConversion();
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */

	data->active_channels = 0;
	for (int ch = 0; ch < CC35XX_CHAN_COUNT; ++ch) {
		if (sequence->channels & BIT(ch)) {
			ADCSetInput(data->channel_config[ch].vref, ch, ADC_FULL_SCALE_RANGE_0V0_3V3,
				    data->active_channels);
			data->active_channels++;
		}
	}
	exp_size = data->active_channels * sizeof(uint16_t);

	if (data->active_channels == 0) {
		LOG_ERR("No channels selected for conversion");
		return -EINVAL;
	}

	if (data->active_channels > CC35XX_SEQUENCE_MAX) {
		LOG_ERR("Cannot convert more than 6 channels in a sequence");
		return -ERANGE;
	}

	if (sequence->buffer_size < exp_size) {
		LOG_ERR("Required buffer size is %u, but %u got", exp_size, sequence->buffer_size);
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;

	ADCSetMemctlRange(0, data->active_channels - 1);
	ADCSetSampleDuration(data->clkdiv, config->sample_period);
	ADCSetSamplingMode(ADC_SAMPLE_MODE_AUTO);
	ADCSetSequence(ADC_SEQUENCE_SEQUENCE);
	ADCSetTriggerSource(ADC_TRIGGER_SOURCE_SOFTWARE);

	adc_context_lock(&data->ctx, asynchronous, sig);
	adc_context_start_read(&data->ctx, sequence);
	ret = adc_context_wait_for_completion(&data->ctx);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_cc35xx_read_sync(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_cc35xx_read(dev, sequence, false, NULL);
}

#ifdef CONFIG_ADC_ASYNC
static int adc_cc35xx_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	return adc_cc35xx_read(dev, sequence, true, async);
}
#endif /* CONFIG_ADC_ASYNC */

#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN
static void adc_cc35xx_isr(const struct device *dev)
{
	uint32_t irq_status = ADCMaskedInterruptStatus();
	struct adc_cc35xx_data *data = dev->data;

	ADCDisableConversion();
	ADCStopConversion();

	if (irq_status & ADC_INT_UVIFG) {
		ADCClearInterrupt(ADC_INT_UVIFG);
		adc_context_stop_sampling(&data->ctx);
		adc_context_start_sampling(&data->ctx);

		return;
	}

	if (irq_status & ADC_INT_DMADONE) {
		ADCDisableInterrupt(ADC_INT_DMADONE);
		ADCClearInterrupt(ADC_INT_DMADONE);

		ADCStopConversion();
		ADCDisableConversion();
		ADCDisableInterrupt(ADC_INT_DMADONE);
		ADCDisableDmaTrigger();

		adc_cc35xx_fifo_enable(dev, false);

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}

	if (irq_status & ADC_INT_CHAN_MASK) {

		ADCDisableInterrupt(ADC_INT_CHAN_MASK);
		ADCClearInterrupt(ADC_INT_CHAN_MASK);
	}
}
#else
static void adc_cc35xx_isr(const struct device *dev)
{
	const struct adc_cc35xx_cfg *config = dev->config;
	struct adc_cc35xx_data *data = dev->data;
	mem_addr_t reg;
	uint32_t irq_status = ADCMaskedInterruptStatus();

	if (irq_status & ADC_INT_UVIFG) {
		ADCClearInterrupt(ADC_INT_UVIFG);

		return;
	}

	if (irq_status & ADC_INT_CHAN_MASK) {
		for (unsigned int chan = 0; chan < data->active_channels; chan++) {
			reg = config->base + ADC_O_MEMRES_0 + 4 * chan;
			*(data->buffer + chan) = sys_read16(reg);
		}

		ADCDisableInterrupt(ADC_INT_CHAN_MASK);
		ADCClearInterrupt(ADC_INT_CHAN_MASK);
		ADCStopConversion();
		ADCDisableConversion();
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */

/**
 * Translate integer divide value user passes in DTS to register value
 */
static uint32_t adc_cc35xx_sclkdiv_reg_val(int div)
{
	switch (div) {
	case 1:
		return ADC_CLOCK_DIVIDER_1;
	case 2:
		return ADC_CLOCK_DIVIDER_2;
	case 4:
		return ADC_CLOCK_DIVIDER_4;
	case 8:
		return ADC_CLOCK_DIVIDER_8;
	case 16:
		return ADC_CLOCK_DIVIDER_16;
	case 24:
		return ADC_CLOCK_DIVIDER_24;
	case 32:
		return ADC_CLOCK_DIVIDER_32;
	case 48:
		return ADC_CLOCK_DIVIDER_48;
	default:
		__ASSERT(0, "Invalid ADC clock divider passed");
		return ADC_CLOCK_DIVIDER_1;
	}
}

static int adc_cc35xx_init(const struct device *dev)
{
	struct adc_cc35xx_data *data = dev->data;
	const struct adc_cc35xx_cfg *config = dev->config;
	int ret;

	data->dev = dev;

	LOG_DBG("Initializing...");

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Applying ADC pinctrl state failed");
		return ret;
	}

	data->clkdiv = adc_cc35xx_sclkdiv_reg_val(config->clkdiv);
	ADCSetSamplingClk(ADC_SAMPLE_CLK_SOC_CLK);
	ADCDisableInterrupt(ADC_INT_ALL);
	sys_write32(ADC_CLKCFG_EN, config->base + ADC_O_CLKCFG);
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), adc_cc35xx_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));

#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN
	if (data->dma.dev_dma != NULL) {
		if (!device_is_ready(data->dma.dev_dma)) {
			return -ENODEV;
		}
		data->dma.blk_cfg.source_address = config->base + ADC_O_FIFODATA;
		data->dma.dma_cfg.head_block = &data->dma.blk_cfg;
		data->dma.dma_cfg.user_data = (void *)data;
	}
#endif /* CONFIG_ADC_CC35XX_DMA_DRIVEN */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, cc35xx_driver_api) = {
	.channel_setup = adc_cc35xx_channel_setup,
	.read = adc_cc35xx_read_sync,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_cc35xx_read_async,
#endif
	.ref_internal = DT_INST_PROP(0, vref_internal_mv),
};

#ifdef CONFIG_ADC_CC35XX_DMA_DRIVEN
#define ADC_CC35XX_DMA_CHANNEL_INIT(n, dir, src_burst, dst_burst)                                  \
	.dev_dma = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_IDX(n, 0)),                                  \
	.dma_channel = DT_INST_DMAS_CELL_BY_IDX(n, 0, channel),                                    \
	.dma_cfg = {                                                                               \
		.dma_slot = DT_INST_DMAS_CELL_BY_IDX(n, 0, channel_config),                        \
		.channel_direction = PERIPHERAL_TO_MEMORY,                                         \
		.source_data_size = sizeof(uint32_t),                                              \
		.dest_data_size = sizeof(uint32_t),                                                \
		.source_burst_length = src_burst,                                                  \
		.dest_burst_length = dst_burst,                                                    \
		.block_count = 1,                                                                  \
	},

#define ADC_CC35XX_DMA_CHANNEL(n, dir, src_burst, dst_burst)                                       \
	.dma = {COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                                        \
		(ADC_CC35XX_DMA_CHANNEL_INIT(n, dir, src_burst, dst_burst)), (NULL))},
#else
#define ADC_CC35XX_DMA_CHANNEL(n, dir, src_burst, dst_burst)
#endif

#define CC35XX_ADC_INIT(n)                                                                         \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct adc_cc35xx_cfg adc_cc35xx_cfg_##n = {                                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clkdiv = DT_INST_PROP(n, clkdiv),                                                 \
		.sample_period = DT_INST_PROP(n, sample_period),                                   \
	};                                                                                         \
	static struct adc_cc35xx_data adc_cc35xx_data_##n = {                                      \
		ADC_CONTEXT_INIT_TIMER(adc_cc35xx_data_##n, ctx),                                  \
		ADC_CONTEXT_INIT_LOCK(adc_cc35xx_data_##n, ctx),                                   \
		ADC_CONTEXT_INIT_SYNC(adc_cc35xx_data_##n, ctx),                                   \
		ADC_CC35XX_DMA_CHANNEL(n, rx, 1, 1)};                                              \
	DEVICE_DT_INST_DEFINE(n, &adc_cc35xx_init, NULL, &adc_cc35xx_data_##n,                     \
			      &adc_cc35xx_cfg_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,          \
			      &cc35xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CC35XX_ADC_INIT)
