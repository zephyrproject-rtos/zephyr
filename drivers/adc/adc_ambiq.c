/*
 * Copyright (c) 2024 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ambiq_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/kernel.h>
#include <zephyr/cache.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* ambiq-sdk includes */
#include <soc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_ambiq, CONFIG_ADC_LOG_LEVEL);

/* Number of slots available. */
#define AMBIQ_ADC_SLOT_NUMBER     AM_HAL_ADC_MAX_SLOTS
#define ADC_TRANSFER_TIMEOUT_MSEC 500

#if defined(CONFIG_SOC_SERIES_APOLLO3X)
#define AMBIQ_ADC_DMA_INT (AM_HAL_ADC_INT_DERR | AM_HAL_ADC_INT_DCMP)
#else
#define AMBIQ_ADC_DMA_INT (AM_HAL_ADC_INT_DERR | AM_HAL_ADC_INT_DCMP | AM_HAL_ADC_INT_FIFOOVR1)
#endif

struct adc_ambiq_config {
	uint32_t base;
	int size;
	uint8_t num_channels;
	void (*irq_config_func)(void);
	const struct pinctrl_dev_config *pin_cfg;
};

struct adc_ambiq_data {
	struct adc_context ctx;
	void *adcHandle;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t active_channels;
	struct k_sem dma_done_sem;
	am_hal_adc_dma_config_t dma_cfg;
	am_hal_adc_sample_t *sample_buf;
	bool dma_mode;
};

static int adc_ambiq_set_resolution(am_hal_adc_slot_prec_e *prec, uint8_t adc_resolution)
{
	switch (adc_resolution) {
	case 8:
		*prec = AM_HAL_ADC_SLOT_8BIT;
		break;
	case 10:
		*prec = AM_HAL_ADC_SLOT_10BIT;
		break;
	case 12:
		*prec = AM_HAL_ADC_SLOT_12BIT;
		break;
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	case 14:
		*prec = AM_HAL_ADC_SLOT_14BIT;
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int adc_ambiq_config(const struct device *dev)
{
	struct adc_ambiq_data *data = dev->data;
	am_hal_adc_config_t ADCConfig;

	/* Set up the ADC configuration parameters. These settings are reasonable
	 *  for accurate measurements at a low sample rate.
	 */
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
	ADCConfig.eClock = AM_HAL_ADC_CLKSEL_HFRC;
	ADCConfig.eReference = AM_HAL_ADC_REFSEL_INT_1P5;
#else
	ADCConfig.eClock = AM_HAL_ADC_CLKSEL_HFRC_24MHZ;
	ADCConfig.eRepeatTrigger = AM_HAL_ADC_RPTTRIGSEL_INT;
#endif
	ADCConfig.ePolarity = AM_HAL_ADC_TRIGPOL_RISING;
	ADCConfig.eTrigger = AM_HAL_ADC_TRIGSEL_SOFTWARE;
	ADCConfig.eClockMode = AM_HAL_ADC_CLKMODE_LOW_LATENCY;
	ADCConfig.ePowerMode = AM_HAL_ADC_LPMODE0;
	if (data->dma_mode) {
		ADCConfig.eRepeat = AM_HAL_ADC_REPEATING_SCAN;
	} else {
		ADCConfig.eRepeat = AM_HAL_ADC_SINGLE_SCAN;
	}

	if (AM_HAL_STATUS_SUCCESS != am_hal_adc_configure(data->adcHandle, &ADCConfig)) {
		LOG_ERR("configuring ADC failed.\n");
		return -ENODEV;
	}

	return 0;
}

static int adc_ambiq_slot_config(const struct device *dev, const struct adc_sequence *sequence,
				 am_hal_adc_slot_chan_e channel, uint32_t ui32SlotNumber)
{
	struct adc_ambiq_data *data = dev->data;
	am_hal_adc_slot_config_t ADCSlotConfig;

	if (adc_ambiq_set_resolution(&ADCSlotConfig.ePrecisionMode, sequence->resolution) != 0) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	/* Set up an ADC slot */
	ADCSlotConfig.eMeasToAvg = AM_HAL_ADC_SLOT_AVG_1;
	ADCSlotConfig.eChannel = channel;
	ADCSlotConfig.bWindowCompare = false;
	ADCSlotConfig.bEnabled = true;
#if !defined(CONFIG_SOC_SERIES_APOLLO3X)
	ADCSlotConfig.ui32TrkCyc = AM_HAL_ADC_MIN_TRKCYC;
#endif
	if (AM_HAL_STATUS_SUCCESS !=
	    am_hal_adc_configure_slot(data->adcHandle, ui32SlotNumber, &ADCSlotConfig)) {
		LOG_ERR("configuring ADC Slot 0 failed.\n");
		return -ENODEV;
	}

	return 0;
}

static void adc_ambiq_disable(const struct device *dev)
{
	struct adc_ambiq_data *data = dev->data;
	am_hal_adc_slot_config_t ADCSlotConfig;

	am_hal_adc_interrupt_disable(data->adcHandle, 0xFF);
	ADCSlotConfig.bEnabled = false;
	for (uint8_t slotNum = 0; slotNum < AM_HAL_ADC_MAX_SLOTS; slotNum++) {
		am_hal_adc_configure_slot(data->adcHandle, slotNum, &ADCSlotConfig);
	}
	if (data->dma_mode) {
		ADCn(0)->DMACFG_b.DMAEN = 0;
	}
	am_hal_adc_disable(data->adcHandle);
}

static void adc_ambiq_isr(const struct device *dev)
{
	struct adc_ambiq_data *data = dev->data;
	uint32_t ui32IntMask;
	uint32_t ui32NumSamples;
	am_hal_adc_sample_t Sample;

	/* Read the interrupt status. */
	am_hal_adc_interrupt_status(data->adcHandle, &ui32IntMask, true);

	/*
	 * If we got a conversion completion interrupt (which should be our only
	 * ADC interrupt), go ahead and read the data.
	 */
	if (ui32IntMask & AM_HAL_ADC_INT_CNVCMP) {
		for (uint32_t i = 0; i < data->active_channels; i++) {
			/* Read the value from the FIFO. */
			ui32NumSamples = 1;
			am_hal_adc_samples_read(data->adcHandle, false, NULL, &ui32NumSamples,
						&Sample);
			*data->buffer++ = Sample.ui32Sample;
		}
		am_hal_adc_disable(data->adcHandle);
		adc_context_on_sampling_done(&data->ctx, dev);
	}

	if (data->dma_mode) {
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
		if (ui32IntMask & AM_HAL_ADC_INT_DCMP) {
#else
		if (((ui32IntMask & AM_HAL_ADC_INT_FIFOOVR1) && (ADCn(0)->DMASTAT_b.DMACPL)) ||
		    (ui32IntMask & AM_HAL_ADC_INT_DCMP)) {
#endif
			k_sem_give(&data->dma_done_sem);
		}
	}

	/* Clear the ADC interrupt.*/
	am_hal_adc_interrupt_clear(data->adcHandle, ui32IntMask);
}

static int adc_ambiq_check_buffer_size(const struct adc_sequence *sequence, uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_DBG("Provided buffer is too small (%u/%u)", sequence->buffer_size,
			needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int adc_ambiq_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_ambiq_data *data = dev->data;
	const struct adc_ambiq_config *cfg = dev->config;
	uint8_t channel_id = 0;
	uint32_t channels = 0;
	uint8_t active_channels = 0;
	uint8_t slot_index;

	int error = 0;

	if (sequence->channels & ~BIT_MASK(cfg->num_channels)) {
		LOG_ERR("Incorrect channels, bitmask 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->channels == 0UL) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	error = adc_ambiq_check_buffer_size(sequence, active_channels);
	if (error < 0) {
		return error;
	}

	active_channels = POPCOUNT(sequence->channels);
	if (active_channels > AMBIQ_ADC_SLOT_NUMBER) {
		LOG_ERR("Too many channels for sequencer. Max: %d", AMBIQ_ADC_SLOT_NUMBER);
		return -ENOTSUP;
	}

	error = adc_ambiq_config(dev);
	if (error < 0) {
		return error;
	}

	channels = sequence->channels;
	for (slot_index = 0; slot_index < active_channels; slot_index++) {
		channel_id = find_lsb_set(channels) - 1;
		error = adc_ambiq_slot_config(dev, sequence, channel_id, slot_index);
		if (error < 0) {
			return error;
		}
		channels &= ~BIT(channel_id);
	}
	__ASSERT_NO_MSG(channels == 0);

	if (data->dma_mode) {
		am_hal_adc_dma_config_t ADCDmaConfig = data->dma_cfg;

		if (data->dma_cfg.ui32SampleCount < active_channels) {
			LOG_ERR("Not enough DMA buffer.\n");
			return -EOVERFLOW;
		}
		ADCDmaConfig.ui32SampleCount = active_channels;
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
		/* Start a timer to trigger the ADC periodically. */
		am_hal_ctimer_config_single(3, AM_HAL_CTIMER_TIMERA,
					    AM_HAL_CTIMER_HFRC_3MHZ | AM_HAL_CTIMER_FN_REPEAT);
		am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA3);
		am_hal_ctimer_period_set(3, AM_HAL_CTIMER_TIMERA, 10, 5);
		/* Enable the timer A3 to trigger the ADC directly */
		am_hal_ctimer_adc_trigger_enable();
#else
		am_hal_adc_irtt_config_t ADCIrttConfig;

		/* Set up internal repeat trigger timer */
		ADCIrttConfig.bIrttEnable = true;
		ADCIrttConfig.eClkDiv = AM_HAL_ADC_RPTT_CLK_DIV16; /* 24MHz / 16  = 1.5MHz */
		ADCIrttConfig.ui32IrttCountMax = 750;              /* 1.5MHz / 750 = 2kHz */
		am_hal_adc_configure_irtt(data->adcHandle, &ADCIrttConfig);
#endif
		/* Configure DMA.*/
		if (AM_HAL_STATUS_SUCCESS !=
		    am_hal_adc_configure_dma(data->adcHandle, &ADCDmaConfig)) {
			LOG_ERR("Error - configuring DMA failed.\n");
			return -EINVAL;
		}

		am_hal_adc_interrupt_clear(data->adcHandle, AMBIQ_ADC_DMA_INT);
		am_hal_adc_interrupt_enable(data->adcHandle, AMBIQ_ADC_DMA_INT);
	} else {
		am_hal_adc_interrupt_enable(data->adcHandle, AM_HAL_ADC_INT_CNVCMP);
	}

	data->active_channels = active_channels;
	data->buffer = sequence->buffer;
	/* Start ADC conversion */
	adc_context_start_read(&data->ctx, sequence);

	if (data->dma_mode) {
		if (k_sem_take(&data->dma_done_sem, K_MSEC(ADC_TRANSFER_TIMEOUT_MSEC))) {
			LOG_ERR("Timeout waiting for transfer complete");
			/* cancel timed out transaction */
			adc_ambiq_disable(dev);
			/* clean up for next xfer */
			k_sem_reset(&data->dma_done_sem);
			return -ETIMEDOUT;
		}
#if CONFIG_ADC_AMBIQ_HANDLE_CACHE
		if (!buf_in_nocache((uintptr_t)data->dma_cfg.ui32TargetAddress,
				    data->active_channels * sizeof(uint32_t))) {
			/* Invalidate Dcache after DMA read */
			sys_cache_data_invd_range((void *)data->dma_cfg.ui32TargetAddress,
						  data->active_channels * sizeof(uint32_t));
		}
#endif /* CONFIG_ADC_AMBIQ_HANDLE_CACHE */
		/* Read the value from the FIFO. */
		am_hal_adc_samples_read(data->adcHandle, false,
					(uint32_t *)data->dma_cfg.ui32TargetAddress,
					(uint32_t *)&data->active_channels, data->sample_buf);
		for (uint32_t i = 0; i < data->active_channels; i++) {
			*data->buffer++ = data->sample_buf[i].ui32Sample;
		}
		adc_ambiq_disable(dev);
		adc_context_on_sampling_done(&data->ctx, dev);
	} else {
		error = adc_context_wait_for_completion(&data->ctx);
	}
	pm_device_runtime_put(dev);

	return error;
}

static int adc_ambiq_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_ambiq_data *data = dev->data;
	int error = 0;

	adc_context_lock(&data->ctx, false, NULL);

	error = pm_device_runtime_get(dev);
	if (error < 0) {
		LOG_ERR("Failed to get device runtime PM state");
		adc_context_release(&data->ctx, error);
		return error;
	}

	error = adc_ambiq_start_read(dev, sequence);

	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_ambiq_channel_setup(const struct device *dev, const struct adc_channel_cfg *chan_cfg)
{
	const struct adc_ambiq_config *cfg = dev->config;

	if (chan_cfg->channel_id >= cfg->num_channels) {
		LOG_ERR("unsupported channel id '%d'", chan_cfg->channel_id);
		return -ENOTSUP;
	}

	if (chan_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid");
		return -ENOTSUP;
	}

	if (chan_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Reference is not valid");
		return -ENOTSUP;
	}

	if (chan_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'", chan_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (chan_cfg->differential) {
		LOG_ERR("Differential sampling not supported");
		return -ENOTSUP;
	}

	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ambiq_data *data = CONTAINER_OF(ctx, struct adc_ambiq_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ambiq_data *data = CONTAINER_OF(ctx, struct adc_ambiq_data, ctx);

	data->repeat_buffer = data->buffer;
	/* Enable the ADC. */
	am_hal_adc_enable(data->adcHandle);
	if (data->dma_mode) {
#if defined(CONFIG_SOC_SERIES_APOLLO3X)
		/* Start the ADC repetitive sample timer A3 */
		am_hal_ctimer_start(3, AM_HAL_CTIMER_TIMERA);
#else
		/* Enable internal repeat trigger timer */
		am_hal_adc_irtt_enable(data->adcHandle);
#endif
	}
	/*Trigger the ADC*/
	am_hal_adc_sw_trigger(data->adcHandle);
}

static int adc_ambiq_init(const struct device *dev)
{
	struct adc_ambiq_data *data = dev->data;
	const struct adc_ambiq_config *cfg = dev->config;

	int ret;

	/* Initialize the ADC and get the handle*/
	if (AM_HAL_STATUS_SUCCESS != am_hal_adc_initialize(0, &data->adcHandle)) {
		ret = -ENODEV;
		LOG_ERR("Failed to initialize ADC, code:%d", ret);
		return ret;
	}

	/* power on ADC*/
	ret = am_hal_adc_power_control(data->adcHandle, AM_HAL_SYSCTRL_WAKE, false);

	ret = pinctrl_apply_state(cfg->pin_cfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Enable the ADC interrupts in the ADC. */
	cfg->irq_config_func();
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_ambiq_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_ambiq_data *data = dev->data;
	int error = 0;

	adc_context_lock(&data->ctx, true, async);

	error = pm_device_runtime_get(dev);
	if (error < 0) {
		adc_context_release(&data->ctx, error);
		return error;
	}

	error = adc_ambiq_start_read(dev, sequence);

	adc_context_release(&data->ctx, error);

	return error;
}
#endif

#ifdef CONFIG_PM_DEVICE
static int adc_ambiq_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct adc_ambiq_data *data = dev->data;
	uint32_t ret = 0;
	am_hal_sysctrl_power_state_e status;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		status = AM_HAL_SYSCTRL_WAKE;
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		status = AM_HAL_SYSCTRL_DEEPSLEEP;
		break;
	default:
		return -ENOTSUP;
	}

	ret = am_hal_adc_power_control(data->adcHandle, status, true);

	if (ret != AM_HAL_STATUS_SUCCESS) {
		return -EPERM;
	} else {
		return 0;
	}
}
#endif /* CONFIG_PM_DEVICE */

#ifdef CONFIG_ADC_ASYNC
#define ADC_AMBIQ_DRIVER_API(n)                                                                    \
	static DEVICE_API(adc, adc_ambiq_driver_api_##n) = {                                       \
		.channel_setup = adc_ambiq_channel_setup,                                          \
		.read = adc_ambiq_read,                                                            \
		.read_async = adc_ambiq_read_async,                                                \
		.ref_internal = DT_INST_PROP(n, internal_vref_mv),                                 \
	};
#else
#define ADC_AMBIQ_DRIVER_API(n)                                                                    \
	static DEVICE_API(adc, adc_ambiq_driver_api_##n) = {                                       \
		.channel_setup = adc_ambiq_channel_setup,                                          \
		.read = adc_ambiq_read,                                                            \
		.ref_internal = DT_INST_PROP(n, internal_vref_mv),                                 \
	};
#endif

#define ADC_DMA_CFG(n, buf, size)                                                                  \
	{                                                                                          \
		.bDynamicPriority = true,                                                          \
		.ePriority = AM_HAL_ADC_PRIOR_SERVICE_IMMED,                                       \
		.bDMAEnable = true,                                                                \
		.ui32SampleCount = size,                                                           \
		.ui32TargetAddress = (uint32_t)buf,                                                \
	}

#if CONFIG_ADC_AMBIQ_HANDLE_CACHE
#define __ADC_NOCACHE(n)                                                                           \
	__attribute__((section(DT_INST_PROP_OR(n, dma_buffer_location, ".nocache"))))
#else
#define __ADC_NOCACHE(n)
#endif /* CONFIG_ADC_AMBIQ_HANDLE_CACHE */

#define ADC_AMBIQ_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	ADC_AMBIQ_DRIVER_API(n);                                                                   \
	static void adc_irq_config_func_##n(void)                                                  \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), adc_ambiq_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	IF_ENABLED(DT_INST_PROP(n, dma_mode),                                           \
	(static uint32_t adc_ambiq_dma_buf##n[DT_INST_PROP_OR(n, dma_buffer_size, 128)]  \
	 __ADC_NOCACHE(n);)                   \
	)         \
	IF_ENABLED(DT_INST_PROP(n, dma_mode),                                           \
	(static am_hal_adc_sample_t adc_sample_buf##n[DT_INST_PROP_OR(n, dma_buffer_size, 128)];)) \
	static struct adc_ambiq_data adc_ambiq_data_##n = {                                        \
		ADC_CONTEXT_INIT_TIMER(adc_ambiq_data_##n, ctx),                                   \
		ADC_CONTEXT_INIT_LOCK(adc_ambiq_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(adc_ambiq_data_##n, ctx),                                    \
		.dma_cfg = ADC_DMA_CFG(                                                            \
			n, COND_CODE_1(DT_INST_PROP(n, dma_mode), (adc_ambiq_dma_buf##n), \
									    (NULL)),               \
				    COND_CODE_1(DT_INST_PROP(n, dma_mode),              \
					(DT_INST_PROP_OR(n, dma_buffer_size, 128)), (0))),    \
						.dma_mode = DT_INST_PROP(n, dma_mode),             \
						.dma_done_sem = Z_SEM_INITIALIZER(                 \
							adc_ambiq_data_##n.dma_done_sem, 0, 1),    \
						.sample_buf = COND_CODE_1(     \
							DT_INST_PROP(n, dma_mode), \
							(adc_sample_buf##n), (NULL)),              \
	};                                                                                         \
	const static struct adc_ambiq_config adc_ambiq_config_##n = {                              \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.size = DT_INST_REG_SIZE(n),                                                       \
		.num_channels = DT_INST_PROP(n, channel_count),                                    \
		.irq_config_func = adc_irq_config_func_##n,                                        \
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                      \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(n, adc_ambiq_pm_action);                                          \
	DEVICE_DT_INST_DEFINE(n, adc_ambiq_init, PM_DEVICE_DT_INST_GET(n), &adc_ambiq_data_##n,    \
			      &adc_ambiq_config_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,        \
			      &adc_ambiq_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(ADC_AMBIQ_INIT)
