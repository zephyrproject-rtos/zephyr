/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_adc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_cc23x0, CONFIG_ADC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#include <driverlib/adc.h>
#include <driverlib/clkctl.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define CPU_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

#define ADC_CC23_CH_UNDEF	0xff
#define ADC_CC23_CH_COUNT	16
#define ADC_CC23_CH_MAX		(ADC_CC23_CH_COUNT - 1)

/* ADC provides four result storage registers */
#define ADC_CC23_MEM_COUNT	4
#define ADC_CC23_MEM_MAX	(ADC_CC23_MEM_COUNT - 1)

#define ADC_CC23_MAX_CYCLES	1023

#define ADC_CC23_INT_MASK (ADC_INT_MEMRES_00 | \
			   ADC_INT_MEMRES_01 | \
			   ADC_INT_MEMRES_02 | \
			   ADC_INT_MEMRES_03)

static const uint32_t clk_dividers[] = {1, 2, 4, 8, 16, 24, 32, 48};

struct adc_cc23x0_config {
	const struct pinctrl_dev_config *pincfg;
	void (*irq_cfg_func)(void);
};

struct adc_cc23x0_data {
	struct adc_context ctx;
	const struct device *dev;
	uint32_t res;
	uint32_t adj_gain[ADC_CC23_MEM_COUNT];
	uint32_t ch_sel[ADC_CC23_MEM_COUNT];
	uint32_t mem_index[ADC_CC23_CH_COUNT];
	uint32_t curr_mem_index;
	uint32_t channels;
	uint8_t ch_count;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
};

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_cc23x0_data *data = CONTAINER_OF(ctx, struct adc_cc23x0_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	ADCManualTrigger();
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_cc23x0_data *data = CONTAINER_OF(ctx, struct adc_cc23x0_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer += data->ch_count;
	}
}

static void adc_cc23x0_isr(const struct device *dev)
{
	struct adc_cc23x0_data *data = dev->data;
	uint32_t int_flags;
	uint32_t adc_val;
	int i;

	int_flags = ADCMaskedInterruptStatus();

	for (i = 0 ; i < ADC_CC23_MEM_COUNT ; i++) {
		if (int_flags & (ADC_INT_MEMRES_00 << i)) {
			adc_val = ADCAdjustValueForGain(ADCReadResultNonBlocking(i),
							data->res, data->adj_gain[i]);

			*(data->buffer + i) = adc_val;
			data->channels &= ~BIT(data->ch_sel[i]);

			LOG_DBG("Mem %d, Ch %d, Val %d", i, data->ch_sel[i], adc_val);

			ADCClearInterrupt(ADC_INT_MEMRES_00 << i);
		}
	}

	if (!data->channels) {
		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int adc_cc23x0_read_common(const struct device *dev,
				  const struct adc_sequence *sequence,
				  bool asynchronous,
				  struct k_poll_signal *sig)
{
	struct adc_cc23x0_data *data = dev->data;
	uint32_t bitmask;
	uint32_t ch_start = ADC_CC23_CH_UNDEF;
	uint32_t ch = ADC_CC23_CH_UNDEF;
	size_t exp_size;
	int ret;
	int i;

	/* Set resolution */
	switch (sequence->resolution) {
	case 8:
		data->res = ADC_RESOLUTION_8_BIT;
		break;
	case 10:
		data->res = ADC_RESOLUTION_10_BIT;
		break;
	case 12:
		data->res = ADC_RESOLUTION_12_BIT;
		break;
	default:
		LOG_ERR("Resolution is not valid");
		return -EINVAL;
	}

	ADCSetResolution(data->res);

	/* Set sequence */
	bitmask = sequence->channels;
	data->ch_count = POPCOUNT(bitmask);

	if (data->ch_count == 1) {
		ADCSetSequence(ADC_SEQUENCE_SINGLE);
		ch = find_lsb_set(bitmask) - 1;
		ADCSetMemctlRange(data->mem_index[ch], data->mem_index[ch]);
	} else if (data->ch_count <= ADC_CC23_MEM_COUNT) {
		ADCSetSequence(ADC_SEQUENCE_SEQUENCE);
		for (i = 0 ; i < ADC_CC23_CH_COUNT ; i++) {
			if (bitmask & BIT(i)) {
				if (ch_start == ADC_CC23_CH_UNDEF) {
					ch_start = i;
				} else {
					ch = i;
				}
			}
		}
		ADCSetMemctlRange(data->mem_index[ch_start], data->mem_index[ch]);
	} else {
		LOG_ERR("Too many channels in the sequence, max %d", ADC_CC23_MEM_COUNT);
		return -EINVAL;
	}

	exp_size = data->ch_count * sizeof(uint16_t);
	if (sequence->options) {
		exp_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < exp_size) {
		LOG_ERR("Required buffer size is %u but got %u", exp_size, sequence->buffer_size);
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;

	/* Start read */
	adc_context_lock(&data->ctx, asynchronous, sig);
	adc_context_start_read(&data->ctx, sequence);
	ret = adc_context_wait_for_completion(&data->ctx);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_cc23x0_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	return adc_cc23x0_read_common(dev, sequence, false, NULL);
}

#ifdef CONFIG_ADC_ASYNC
static int adc_cc23x0_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	return adc_cc23x0_read_common(dev, sequence, true, async);
}
#endif

static uint32_t adc_cc23x0_clkdiv_to_field(uint32_t clk_div)
{
	switch (clk_div) {
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
		return ADC_CLOCK_DIVIDER_1;
	}
}

static int adc_cc23x0_calc_clk_cfg(uint32_t acq_time_ns, uint32_t *clk_div, uint16_t *clk_cycles)
{
	const uint32_t base_clock = CPU_FREQ;
	uint32_t divider;
	uint32_t clock_frequency;
	uint16_t min_cycles;
	uint16_t cycles;
	float clock_period_ns;

	/* Initialize best values with maximum possible cycles */
	min_cycles = ADC_CC23_MAX_CYCLES;
	*clk_div = 0;
	*clk_cycles = 0;

	LOG_DBG("Requested sample duration: %u ns", acq_time_ns);

	/* Iterate through each divider */
	ARRAY_FOR_EACH(clk_dividers, i) {
		divider = clk_dividers[i];
		clock_frequency = base_clock / divider;

		/* Calculate the clock period */
		clock_period_ns = 1.0E9 / clock_frequency;

		/* Calculate the number of cycles needed to meet or exceed acq_time_ns */
		cycles = DIV_ROUND_UP(acq_time_ns, clock_period_ns);

		/* Check if this configuration is valid and optimal */
		if (cycles <= ADC_CC23_MAX_CYCLES && cycles < min_cycles) {
			min_cycles = cycles;
			*clk_div = divider;
			*clk_cycles = cycles;

			LOG_DBG("Divider: %u, Cycles: %u, Actual sample duration: %u ns",
				divider, cycles, (uint32_t)clock_period_ns * cycles);
		}
	}

	/* Check if a valid configuration was found */
	if (*clk_div == 0) {
		return -EINVAL;
	}

	return 0;
}

static int adc_cc23x0_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	struct adc_cc23x0_data *data = dev->data;
	const uint8_t ch = channel_cfg->channel_id;
	uint32_t ref;
	uint32_t acq_time_ns;
	uint32_t clk_div;
	uint16_t clk_cycles;
	int ret;

	LOG_DBG("Channel %d", ch);

	if (data->curr_mem_index > ADC_CC23_MEM_MAX) {
		LOG_ERR("%d channels are already configured", ADC_CC23_MEM_COUNT);
		return -ENOMEM;
	}

	if (ch > ADC_CC23_CH_MAX) {
		LOG_ERR("Channel %d is not supported, max %d", ch, ADC_CC23_CH_MAX);
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

	/* Set reference source, input channel, and storage index */
	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		ref = ADC_FIXED_REFERENCE_1V4;
		break;
	case ADC_REF_EXTERNAL0:
		ref = ADC_EXTERNAL_REFERENCE;
		break;
	case ADC_REF_VDD_1:
		ref = ADC_VDDS_REFERENCE;
		break;
	default:
		LOG_ERR("Reference is not valid");
		return -EINVAL;
	}

	data->ch_sel[data->curr_mem_index] = ch;
	data->mem_index[ch] = data->curr_mem_index;

	ADCSetInput(ref, ch, data->curr_mem_index);

	/*
	 * Set adjustment offset, and get adjustment gain. Both adjustment values depend on
	 * reference source. Internal gain will be used for subsequent measurement compensation.
	 */
	ADCSetAdjustmentOffset(ref);
	data->adj_gain[data->curr_mem_index] = ADCGetAdjustmentGain(ref);

	data->curr_mem_index++;

	/* Set acquisition time */
	switch (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time)) {
	case ADC_ACQ_TIME_TICKS:
		clk_div = 1;
		clk_cycles = (uint16_t)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
		break;
	case ADC_ACQ_TIME_MICROSECONDS:
		acq_time_ns = 1000 * (uint16_t)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
		ret = adc_cc23x0_calc_clk_cfg(acq_time_ns, &clk_div, &clk_cycles);
		if (ret) {
			LOG_DBG("No valid clock configuration found");
			return ret;
		}
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		acq_time_ns = (uint16_t)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
		ret = adc_cc23x0_calc_clk_cfg(acq_time_ns, &clk_div, &clk_cycles);
		if (ret) {
			LOG_DBG("No valid clock configuration found");
			return ret;
		}
		break;
	default:
		clk_div = clk_dividers[ARRAY_SIZE(clk_dividers) - 1];
		clk_cycles = 1;
	}

	ADCSetSampleDuration(adc_cc23x0_clkdiv_to_field(clk_div), clk_cycles);

	return 0;
}

static int adc_cc23x0_init(const struct device *dev)
{
	const struct adc_cc23x0_config *cfg = dev->config;
	struct adc_cc23x0_data *data = dev->data;
	int ret;

	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to apply ADC pinctrl state");
		return ret;
	}

	data->dev = dev;
	cfg->irq_cfg_func();

	/* Enable clock */
	CLKCTLEnable(CLKCTL_BASE, CLKCTL_ADC0);

	/* Enable interrupts */
	ADCEnableInterrupt(ADC_CC23_INT_MASK);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api adc_cc23x0_driver_api = {
	.channel_setup = adc_cc23x0_channel_setup,
	.read = adc_cc23x0_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_cc23x0_read_async,
#endif
	.ref_internal = 1400,
};

#define CC23X0_ADC_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	static void adc_cc23x0_cfg_func_##n(void);				\
										\
	static const struct adc_cc23x0_config adc_cc23x0_config_##n = {		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.irq_cfg_func = adc_cc23x0_cfg_func_##n,			\
	};									\
										\
	static struct adc_cc23x0_data adc_cc23x0_data_##n = {			\
		ADC_CONTEXT_INIT_TIMER(adc_cc23x0_data_##n, ctx),		\
		ADC_CONTEXT_INIT_LOCK(adc_cc23x0_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_cc23x0_data_##n, ctx),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      &adc_cc23x0_init,					\
			      NULL,						\
			      &adc_cc23x0_data_##n,				\
			      &adc_cc23x0_config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_ADC_INIT_PRIORITY,				\
			      &adc_cc23x0_driver_api);				\
										\
	static void adc_cc23x0_cfg_func_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    adc_cc23x0_isr,					\
			    DEVICE_DT_INST_GET(n),				\
			    0);							\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(CC23X0_ADC_INIT)
