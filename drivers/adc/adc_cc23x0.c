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
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <driverlib/adc.h>
#include <driverlib/clkctl.h>

#include <inc/hw_memmap.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_CC23X0_CH_UNDEF	0xff
#define ADC_CC23X0_CH_COUNT	16
#define ADC_CC23X0_CH_MAX	(ADC_CC23X0_CH_COUNT - 1)

/* ADC provides four result storage registers */
#define ADC_CC23X0_MEM_COUNT	4
#define ADC_CC23X0_MEM_MAX	(ADC_CC23X0_MEM_COUNT - 1)

#define ADC_CC23X0_MAX_CYCLES	1023

#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
#define ADC_CC23X0_REG_GET(offset) (ADC_BASE + (offset))
#define ADC_CC23X0_INT_MASK	ADC_INT_DMADONE
#else
#define ADC_CC23X0_INT_MASK	(ADC_INT_MEMRES_00 | \
				 ADC_INT_MEMRES_01 | \
				 ADC_INT_MEMRES_02 | \
				 ADC_INT_MEMRES_03)
#endif

#define ADC_CC23X0_INT_MEMRES(i)	(ADC_INT_MEMRES_00 << (i))

#define ADC_CC23X0_MEMCTL(base, i)	HWREG((base) + ADC_O_MEMCTL0 + sizeof(uint32_t) * (i))

static const uint8_t clk_dividers[] = {1, 2, 4, 8, 16, 24, 32, 48};

struct adc_cc23x0_config {
	const struct pinctrl_dev_config *pincfg;
	void (*irq_cfg_func)(void);
	uint32_t base;
#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
	const struct device *dma_dev;
	uint8_t dma_channel;
	uint8_t dma_trigsrc;
#endif
};

struct adc_cc23x0_data {
	struct adc_context ctx;
	const struct device *dev;
	uint32_t res;
	uint32_t ref_volt[ADC_CC23X0_CH_COUNT];
	uint16_t clk_cycles;
	uint8_t clk_div;
	uint8_t ch_sel[ADC_CC23X0_MEM_COUNT];
	uint8_t ch_count;
	uint8_t mem_index;
	uint16_t *buffer;
#ifdef CONFIG_PM_DEVICE
	bool configured;
#endif
};

static inline void adc_cc23x0_pm_policy_state_lock_get(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static inline void adc_cc23x0_pm_policy_state_lock_put(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_cc23x0_data *data = CONTAINER_OF(ctx, struct adc_cc23x0_data, ctx);
#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
	const struct adc_cc23x0_config *cfg = data->dev->config;

	struct dma_block_config block_cfg = {
		.source_address = ADC_CC23X0_REG_GET(ADC_O_MEMRES0),
		.dest_address = (uint32_t)(data->buffer),
		.source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
		.block_size = data->ch_count * sizeof(*data->buffer),
	};

	struct dma_config dma_cfg = {
		.dma_slot = cfg->dma_trigsrc,
		.channel_direction = PERIPHERAL_TO_MEMORY,
		.block_count = 1,
		.head_block = &block_cfg,
		.source_data_size = sizeof(uint32_t),
		.dest_data_size = sizeof(*data->buffer),
		.source_burst_length = block_cfg.block_size,
		.dma_callback = NULL,
		.user_data = NULL,
	};

	int ret;

	ret = pm_device_runtime_get(cfg->dma_dev);
	if (ret) {
		LOG_ERR("Failed to resume DMA (%d)", ret);
		return;
	}

	ret = dma_config(cfg->dma_dev, cfg->dma_channel, &dma_cfg);
	if (ret) {
		LOG_ERR("Failed to configure DMA (%d)", ret);
		return;
	}

	ADCEnableDMATrigger();

	dma_start(cfg->dma_dev, cfg->dma_channel);
#else
	data->mem_index = 0;
#endif

	adc_cc23x0_pm_policy_state_lock_get();

	ADCManualTrigger();
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_cc23x0_data *data = CONTAINER_OF(ctx, struct adc_cc23x0_data, ctx);

	if (!repeat) {
		data->buffer += data->ch_count;
	}
}

static void adc_cc23x0_isr(const struct device *dev)
{
	struct adc_cc23x0_data *data = dev->data;
#ifndef CONFIG_ADC_CC23X0_DMA_DRIVEN
	uint32_t adc_val;
	uint8_t ch;
#endif

#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
	const struct adc_cc23x0_config *cfg = dev->config;
	int ret;

	/*
	 * In DMA mode, do not compensate for the ADC internal gain with
	 * ADCAdjustValueForGain() function. To perform this compensation,
	 * reading the data from the buffer and overwriting them would be
	 * necessary, which would mitigate the advantage of using DMA.
	 */
	ADCClearInterrupt(ADC_INT_DMADONE);
	LOG_DBG("DMA done");

	ret = pm_device_runtime_put(cfg->dma_dev);
	if (ret) {
		LOG_ERR("Failed to suspend DMA (%d)", ret);
		return;
	}

	adc_cc23x0_pm_policy_state_lock_put();
	adc_context_on_sampling_done(&data->ctx, dev);
#else
	/*
	 * Even when there are multiple channels, only 1 flag can be set because
	 * of the trigger policy (next conversion requires a trigger)
	 */
	ch = data->ch_sel[data->mem_index];

	/*
	 * Both adjustment offset and adjustment gain depend on reference source.
	 * Internal gain is used for measurement compensation.
	 */
	adc_val = ADCAdjustValueForGain(ADCReadResultNonBlocking(data->mem_index),
					data->res,
					ADCGetAdjustmentGain(data->ref_volt[ch]));
	data->buffer[data->mem_index] = adc_val;

	ADCClearInterrupt(ADC_CC23X0_INT_MEMRES(data->mem_index));

	LOG_DBG("Mem %u, Ch %u, Val %d", data->mem_index, ch, adc_val);

	if (++data->mem_index < data->ch_count) {
		/* Set adjustment offset for the next channel */
		ch = data->ch_sel[data->mem_index];
		ADCSetAdjustmentOffset(data->ref_volt[ch]);
		LOG_DBG("Next Ch %u", ch);

		/* Trigger next conversion */
		ADCManualTrigger();
	} else {
		adc_cc23x0_pm_policy_state_lock_put();
		adc_context_on_sampling_done(&data->ctx, dev);
	}
#endif
}

static int adc_cc23x0_read_common(const struct device *dev,
				  const struct adc_sequence *sequence,
				  bool asynchronous,
				  struct k_poll_signal *sig)
{
#ifndef CONFIG_ADC_CC23X0_DMA_DRIVEN
	const struct adc_cc23x0_config *cfg = dev->config;
#endif
	struct adc_cc23x0_data *data = dev->data;
	uint32_t bitmask;
	uint8_t ch_start = ADC_CC23X0_CH_UNDEF;
	uint8_t mem_index = 0;
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
		ch_start = find_lsb_set(bitmask) - 1;

		data->ch_sel[0] = ch_start;

		/* Set input channel, memory range, and mode */
		ADCSetInput(data->ref_volt[ch_start], ch_start, 0);
		ADCSetMemctlRange(0, 0);
		ADCSetSequence(ADC_SEQUENCE_SINGLE);

		/* Set adjustment offset for this channel */
		ADCSetAdjustmentOffset(data->ref_volt[ch_start]);

#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
		ADCEnableDMAInterrupt(ADC_CC23X0_INT_MEMRES(0));
#endif
	} else if (data->ch_count <= ADC_CC23X0_MEM_COUNT) {
		for (i = 0; i < ADC_CC23X0_CH_COUNT; i++) {
			if (!(bitmask & BIT(i))) {
				continue;
			}

			if (ch_start == ADC_CC23X0_CH_UNDEF) {
				ch_start = i;
			}

			data->ch_sel[mem_index] = i;

			/* Set input channel */
			ADCSetInput(data->ref_volt[i], i, mem_index);

#ifndef CONFIG_ADC_CC23X0_DMA_DRIVEN
			/* Set trigger policy so next conversion requires a trigger */
			ADC_CC23X0_MEMCTL(cfg->base, mem_index) |= ADC_MEMCTL0_TRG;
#endif

			mem_index++;
		}

		/* Set memory range and mode */
		ADCSetMemctlRange(0, mem_index - 1);
		ADCSetSequence(ADC_SEQUENCE_SEQUENCE);

		/* Set adjustment offset for the first channel */
		ADCSetAdjustmentOffset(data->ref_volt[ch_start]);

#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
		/*
		 * DMA transfer will be triggered when the last storage register
		 * of the sequence is loaded with a new conversion result
		 */
		ADCEnableDMAInterrupt(ADC_CC23X0_INT_MEMRES(mem_index - 1));
#endif
	} else {
		LOG_ERR("Too many channels in the sequence, max %u", ADC_CC23X0_MEM_COUNT);
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

static uint32_t adc_cc23x0_clkdiv_to_field(uint8_t clk_div)
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

static int adc_cc23x0_calc_clk_cfg(uint32_t acq_time_ns, uint8_t *clk_div, uint16_t *clk_cycles)
{
	uint32_t samp_duration_ns;
	uint32_t min_delta_res = UINT32_MAX;
	uint32_t delta_res;
	uint16_t min_cycles = ADC_CC23X0_MAX_CYCLES;
	uint16_t cycles;
	uint8_t divider;
	float clock_period_ns;

	*clk_div = 0;
	*clk_cycles = 0;

	LOG_DBG("Requested sample duration: %u ns", acq_time_ns);

	/* Iterate through each divider */
	ARRAY_FOR_EACH(clk_dividers, i) {
		divider = clk_dividers[i];
		clock_period_ns = 1.0E9 * divider / TI_CC23X0_DT_CPU_CLK_FREQ_HZ;

		/* Calculate the number of cycles needed to meet or exceed acq_time_ns */
		cycles = DIV_ROUND_UP(acq_time_ns, clock_period_ns);

		/* Calculate the delta between the requested and actual sample durations */
		samp_duration_ns = clock_period_ns * cycles;
		delta_res = samp_duration_ns - acq_time_ns;

		/* Check if this configuration is valid and optimal */
		if (cycles <= min_cycles && delta_res <= min_delta_res) {
			min_delta_res = delta_res;
			min_cycles = cycles;
			*clk_div = divider;
			*clk_cycles = cycles;

			LOG_DBG("Divider: %u, Cycles: %u, Actual sample duration: %u ns",
				divider, cycles, samp_duration_ns);
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
	uint16_t clk_cycles;
	uint8_t clk_div;
	int ret;

	LOG_DBG("Channel %u", ch);

	if (ch > ADC_CC23X0_CH_MAX) {
		LOG_ERR("Channel %u is not supported, max %u", ch, ADC_CC23X0_CH_MAX);
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

	/* Set reference source */
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

	data->ref_volt[ch] = ref;

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

	if (!data->clk_cycles) {
		data->clk_div = clk_div;
		data->clk_cycles = clk_cycles;
		ADCSetSampleDuration(adc_cc23x0_clkdiv_to_field(clk_div), data->clk_cycles);
	} else if (clk_div != data->clk_div || clk_cycles != data->clk_cycles) {
		LOG_ERR("Multiple sample durations are not supported");
		return -EINVAL;
	}

#ifdef CONFIG_PM_DEVICE
	data->configured = true;
#endif

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
	ADCEnableInterrupt(ADC_CC23X0_INT_MASK);

#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
	if (!device_is_ready(cfg->dma_dev)) {
		return -ENODEV;
	}

	ret = pm_device_runtime_enable(cfg->dma_dev);
	if (ret) {
		LOG_ERR("Failed to enable DMA runtime PM");
		return ret;
	}
#endif

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int adc_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct adc_cc23x0_data *data = dev->data;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		CLKCTLDisable(CLKCTL_BASE, CLKCTL_ADC0);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		CLKCTLEnable(CLKCTL_BASE, CLKCTL_ADC0);
		ADCEnableInterrupt(ADC_CC23X0_INT_MASK);

		/* Restore context if needed */
		if (data->configured) {
			ADCSetSampleDuration(adc_cc23x0_clkdiv_to_field(data->clk_div),
					     data->clk_cycles);
		}

		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(adc, adc_cc23x0_driver_api) = {
	.channel_setup = adc_cc23x0_channel_setup,
	.read = adc_cc23x0_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_cc23x0_read_async,
#endif
	.ref_internal = 1400,
};

#ifdef CONFIG_ADC_CC23X0_DMA_DRIVEN
#define ADC_CC23X0_DMA_INIT(n)						\
	.dma_dev = DEVICE_DT_GET(TI_CC23X0_DT_INST_DMA_CTLR(n, dma)),	\
	.dma_channel = TI_CC23X0_DT_INST_DMA_CHANNEL(n, dma),		\
	.dma_trigsrc = TI_CC23X0_DT_INST_DMA_TRIGSRC(n, dma),
#else
#define ADC_CC23X0_DMA_INIT(n)
#endif

#define CC23X0_ADC_INIT(n)							\
	PINCTRL_DT_INST_DEFINE(n);						\
	PM_DEVICE_DT_INST_DEFINE(n, adc_cc23x0_pm_action);			\
										\
	static void adc_cc23x0_cfg_func_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    adc_cc23x0_isr,					\
			    DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	static const struct adc_cc23x0_config adc_cc23x0_config_##n = {		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.irq_cfg_func = adc_cc23x0_cfg_func_##n,			\
		.base = DT_INST_REG_ADDR(n),					\
		ADC_CC23X0_DMA_INIT(n)						\
	};									\
										\
	static struct adc_cc23x0_data adc_cc23x0_data_##n = {			\
		ADC_CONTEXT_INIT_TIMER(adc_cc23x0_data_##n, ctx),		\
		ADC_CONTEXT_INIT_LOCK(adc_cc23x0_data_##n, ctx),		\
		ADC_CONTEXT_INIT_SYNC(adc_cc23x0_data_##n, ctx),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      adc_cc23x0_init,					\
			      PM_DEVICE_DT_INST_GET(n),				\
			      &adc_cc23x0_data_##n,				\
			      &adc_cc23x0_config_##n,				\
			      POST_KERNEL,					\
			      CONFIG_ADC_INIT_PRIORITY,				\
			      &adc_cc23x0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CC23X0_ADC_INIT)
