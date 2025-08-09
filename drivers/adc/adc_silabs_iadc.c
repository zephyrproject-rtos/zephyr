/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_iadc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <sl_hal_iadc.h>

LOG_MODULE_REGISTER(iadc, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define IADC_DATA_RES12BIT(DATA) ((DATA) & 0x0FFF)
#define IADC_PORT_MASK           0xF0
#define IADC_PIN_MASK            0x0F

struct iadc_dma_channel {
	const struct device *dma_dev;
	uint32_t dma_channel;
	struct dma_block_config blk_cfg;
	struct dma_config dma_cfg;
	uint32_t offset;
	bool enabled;
};

struct iadc_chan_conf {
	sl_hal_iadc_analog_gain_t gain;
	sl_hal_iadc_voltage_reference_t reference;
	sl_hal_iadc_positive_port_input_t pos_port;
	uint8_t pos_pin;
	sl_hal_iadc_negative_port_input_t neg_port;
	uint8_t neg_pin;
	uint8_t iadc_conf_id;
	bool initialized;
};

struct iadc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint32_t clock_rate;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint8_t adc_config_count; /* Number of ADC configs created (max 2) */
	struct iadc_chan_conf chan_conf[SL_HAL_IADC_CHANNEL_ID_MAX - 1];
	struct iadc_dma_channel dma;
};

struct iadc_config {
	sl_hal_iadc_config_t config;
	IADC_TypeDef *base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_cfg_func)(void);
};

static int iadc_find_or_create_adc_config(struct iadc_data *data, sl_hal_iadc_init_t *init,
					  const struct iadc_chan_conf *chan_conf)
{
	int iadc_conf_id;

	/* Check if we can reuse existing ADC configs */
	for (int i = 0; i < data->adc_config_count; i++) {
		if (chan_conf->gain == init->configs[i].analog_gain &&
		    chan_conf->reference == init->configs[i].reference) {
			return i;
		}
	}

	/* If no matching config found, create a new one if possible
	 * and return the new config ID. If not possible, return 0.
	 */
	if (data->adc_config_count >= ARRAY_SIZE(init->configs)) {
		LOG_ERR("Maximum of 2 different ADC configs supported");
		return 0;
	}

	iadc_conf_id = data->adc_config_count;
	init->configs[iadc_conf_id].analog_gain = chan_conf->gain;
	init->configs[iadc_conf_id].reference = chan_conf->reference;
	data->adc_config_count++;

	return iadc_conf_id;
}

static void iadc_configure_scan_table_entry(sl_hal_iadc_scan_table_entry_t *entry,
					    const struct iadc_chan_conf *chan_conf)
{
	*entry = (sl_hal_iadc_scan_table_entry_t){
		.positive_port = chan_conf->pos_port,
		.positive_pin = chan_conf->pos_pin,
		.negative_port = chan_conf->neg_port,
		.negative_pin = chan_conf->neg_pin,
		.config_id = chan_conf->iadc_conf_id,
		.include_in_scan = true,
	};
}

#ifdef CONFIG_ADC_SILABS_IADC_DMA
static int iadc_dma_init(const struct device *dev)
{
	const struct iadc_config *config = dev->config;
	struct iadc_data *data = dev->data;
	struct iadc_dma_channel *dma = &data->dma;

	if (dma->dma_dev) {
		if (!device_is_ready(dma->dma_dev)) {
			LOG_ERR("DMA device not ready");
			return -ENODEV;
		}

		dma->dma_channel = dma_request_channel(dma->dma_dev, NULL);
		if (dma->dma_channel < 0) {
			LOG_ERR("Failed to request DMA channel");
			return -ENODEV;
		}
	}

	/* Configure DMA block */
	memset(&dma->blk_cfg, 0, sizeof(dma->blk_cfg));
	dma->blk_cfg.source_address = (uintptr_t)&((IADC_TypeDef *)config->base)->SCANFIFODATA;
	dma->blk_cfg.dest_address = 0;
	dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma->blk_cfg.block_size = 0;
	dma->dma_cfg.complete_callback_en = 1;
	dma->dma_cfg.channel_priority = 3;
	dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma->dma_cfg.head_block = &dma->blk_cfg;
	dma->dma_cfg.user_data = (void *)dev;

	return 0;
}

static int iadc_dma_start(const struct device *dev)
{
	struct iadc_data *data = dev->data;
	struct iadc_dma_channel *dma = &data->dma;
	int ret;

	if (!dma->dma_dev) {
		return -ENODEV;
	}

	if (dma->enabled) {
		LOG_WRN("DMA was already enabled for IADC");
		return -EBUSY;
	}

	/* Configure DMA channel */
	ret = dma_config(dma->dma_dev, dma->dma_channel, &dma->dma_cfg);
	if (ret) {
		LOG_ERR("DMA config error: %d", ret);
		return ret;
	}

	dma->enabled = true;

	/* Start DMA transfer */
	ret = dma_start(dma->dma_dev, dma->dma_channel);
	if (ret) {
		LOG_ERR("DMA start error: %d", ret);
		dma->enabled = false;
		return ret;
	}

	LOG_DBG("DMA transfer started");

	return 0;
}

static void iadc_dma_stop(const struct device *dev)
{
	struct iadc_data *data = dev->data;
	struct iadc_dma_channel *dma = &data->dma;

	if (!dma->enabled) {
		return;
	}

	/* Stop DMA transfer */
	dma_stop(dma->dma_dev, dma->dma_channel);

	dma->enabled = false;

	LOG_DBG("DMA transfer stopped");
}

static void iadc_dma_cb(const struct device *dma_dev, void *user_data, uint32_t channel, int status)
{
	const struct device *dev = (const struct device *)user_data;
	struct iadc_data *data = dev->data;

	if (status < 0) {
		LOG_ERR("DMA transfer error: %d", status);
		adc_context_complete(&data->ctx, status);
		return;
	}

	iadc_dma_stop(dev);

	adc_context_on_sampling_done(&data->ctx, dev);

	LOG_DBG("DMA transfer completed");
}
#endif

static void iadc_set_config(const struct device *dev)
{
	sl_hal_iadc_scan_table_t scanTable = SL_HAL_IADC_SCANTABLE_DEFAULT;
	sl_hal_iadc_init_scan_t scanInit = SL_HAL_IADC_INITSCAN_DEFAULT;
	sl_hal_iadc_init_t adc_init_config = SL_HAL_IADC_INIT_DEFAULT;
	const struct iadc_config *config = dev->config;
	IADC_TypeDef *iadc = (IADC_TypeDef *)config->base;
	struct iadc_data *data = dev->data;
	struct iadc_chan_conf *chan_conf;

	if (data->dma.dma_dev) {
		scanInit.data_valid_level = SL_HAL_IADC_DATA_VALID_1;
		/* Only needed to wake up DMA if EM is 2/3 */
		scanInit.fifo_dma_wakeup = true;
	}

	data->adc_config_count = 0;

	/*
	 * Process each channel configuration and set up ADC scan sequence.
	 * The IADC hardware supports only 2 different ADC configurations
	 * (gain + reference combinations), so we need to map multiple
	 * channel configs to these 2 available ADC configs.
	 */
	ARRAY_FOR_EACH(data->chan_conf, i) {
		chan_conf = &data->chan_conf[i];

		if (!chan_conf->initialized || (i != find_lsb_set(data->channels) - 1)) {
			continue;
		}

		chan_conf->iadc_conf_id =
			iadc_find_or_create_adc_config(data, &adc_init_config, chan_conf);

		iadc_configure_scan_table_entry(&scanTable.entries[i], chan_conf);

		data->channels &= ~BIT(i);
	}

	sl_hal_iadc_init(iadc, &adc_init_config, data->clock_rate);
	sl_hal_iadc_init_scan(iadc, &scanInit, &scanTable);
	sl_hal_iadc_set_scan_mask_multiple_entries(iadc, &scanTable);
}

static int iadc_check_buffer_size(const struct adc_sequence *sequence, uint8_t active_channels)
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

static int iadc_check_resolution(const struct adc_sequence *sequence)
{
	int value = sequence->resolution;

	/* Base resolution is on 12, it can be changed only up by oversampling */
	if (value != 12) {
		return -EINVAL;
	}

	return value;
}

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct iadc_data *data = dev->data;
	uint32_t channels;
	uint8_t channel_count;
	uint8_t index;
	int res;

	if (sequence->channels == 0) {
		LOG_DBG("No channel requested");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		/* TODO: Implement oversampling for IADC: impact calibration and configuration */
		LOG_ERR("Oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		/* TODO: Implement calibration for IADC */
		LOG_ERR("Calibration is not supported");
		return -ENOTSUP;
	}

	res = iadc_check_resolution(sequence);
	if (res < 0) {
		return -EINVAL;
	}

	channels = sequence->channels;
	channel_count = 0;
	while (channels) {

		index = find_lsb_set(channels) - 1;
		if (index >= SL_HAL_IADC_CHANNEL_ID_MAX - 1) {
			LOG_DBG("Requested channel index not available: %d", index);
			return -EINVAL;
		}

		if (!data->chan_conf[index].initialized) {
			LOG_DBG("Channel not initialized");
			return -EINVAL;
		}
		channel_count++;
		channels &= ~BIT(index);
	}

	res = iadc_check_buffer_size(sequence, channel_count);
	if (res < 0) {
		return res;
	}

	data->buffer = sequence->buffer;

	if (data->dma.dma_dev) {
		/* Need to take care that it will copy 16 bit data and not only 12 bit of the result
		 */
		data->dma.blk_cfg.dest_address = (uintptr_t)data->buffer;
		data->dma.blk_cfg.block_size = channel_count * sizeof(uint16_t);
		data->dma.offset = 0;
	}

	adc_context_start_read(&data->ctx, sequence);

	res = adc_context_wait_for_completion(&data->ctx);

	return res;
}

static void iadc_start_channel(const struct device *dev)
{
	const struct iadc_config *config = dev->config;
	struct iadc_data *data = dev->data;
	IADC_TypeDef *iadc = (IADC_TypeDef *)config->base;

	LOG_DBG("Starting scan");

	iadc_set_config(data->dev);

#ifdef CONFIG_ADC_SILABS_IADC_DMA
	if (data->dma.dma_dev) {
		data->dma.blk_cfg.dest_address = (uintptr_t)data->buffer + data->dma.offset;
		iadc_dma_start(dev);
	} else {
		sl_hal_iadc_enable_interrupts(iadc, IADC_IEN_SCANTABLEDONE);
	}
#else
	sl_hal_iadc_enable_interrupts(iadc, IADC_IEN_SCANTABLEDONE);
#endif

	sl_hal_iadc_start_scan(iadc);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct iadc_data *data = CONTAINER_OF(ctx, struct iadc_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	iadc_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct iadc_data *data = CONTAINER_OF(ctx, struct iadc_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
		data->dma.offset = 0;
	} else {
		data->dma.offset = data->dma.blk_cfg.block_size * (data->ctx.sampling_index);
	}
}

static void iadc_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct iadc_config *config = dev->config;
	struct iadc_data *data = dev->data;
	IADC_TypeDef *iadc = config->base;
	sl_hal_iadc_result_t sample;
	uint32_t flags, err;

	flags = sl_hal_iadc_get_pending_interrupts(iadc);
	sl_hal_iadc_clear_interrupts(iadc, flags);

	err = flags & (IADC_IF_PORTALLOCERR | IADC_IF_POLARITYERR | IADC_IF_EM23ABORTERROR |
		       IADC_IF_SCANFIFOOF | IADC_IF_SCANFIFOUF);

	if (flags & IADC_IF_SCANTABLEDONE) {
		while (sl_hal_iadc_get_scan_fifo_cnt(iadc) > 0) {
			sample = sl_hal_iadc_pull_scan_fifo_result(iadc);
			*data->buffer++ = IADC_DATA_RES12BIT((uint16_t)sample.data);
		}

		adc_context_on_sampling_done(&data->ctx, dev);
	} else if (err) {
		LOG_ERR("IADC conversion error, flags=%08x", err);
		adc_context_complete(&data->ctx, -EIO);
	}
}

static int iadc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct iadc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int iadc_read_async(const struct device *dev, const struct adc_sequence *sequence,
			   struct k_poll_signal *async)
{
	struct iadc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static int iadc_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg)
{
	struct iadc_data *data = dev->data;
	struct iadc_chan_conf *chan_conf = NULL;

	if (channel_cfg->channel_id < SL_HAL_IADC_CHANNEL_ID_MAX - 1) {
		chan_conf = &data->chan_conf[channel_cfg->channel_id];
	} else {
		LOG_DBG("Requested channel index not available: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	chan_conf->initialized = false;

	chan_conf->pos_port = (channel_cfg->input_positive & IADC_PORT_MASK) >> 4;
	chan_conf->pos_pin = channel_cfg->input_positive & IADC_PIN_MASK;

	if (channel_cfg->differential) {
		chan_conf->neg_port = (channel_cfg->input_negative & IADC_PORT_MASK) >> 4;
		chan_conf->neg_pin = channel_cfg->input_negative & IADC_PIN_MASK;
	} else {
		chan_conf->neg_port = SL_HAL_IADC_NEG_PORT_INPUT_GND;
	}

	switch (channel_cfg->gain) {
#if defined(_IADC_CFG_ANALOGGAIN_ANAGAIN0P25)
	case ADC_GAIN_1_4:
		chan_conf->gain = SL_HAL_IADC_ANALOG_GAIN_0P25;
		break;
#endif
	case ADC_GAIN_1_2:
		chan_conf->gain = SL_HAL_IADC_ANALOG_GAIN_0P5;
		break;
	case ADC_GAIN_1:
		chan_conf->gain = SL_HAL_IADC_ANALOG_GAIN_1;
		break;
	case ADC_GAIN_2:
		chan_conf->gain = SL_HAL_IADC_ANALOG_GAIN_2;
		break;
	case ADC_GAIN_3:
		chan_conf->gain = SL_HAL_IADC_ANALOG_GAIN_3;
		break;
	case ADC_GAIN_4:
		chan_conf->gain = SL_HAL_IADC_ANALOG_GAIN_4;
		break;
	default:
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	/* Setup reference */
	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		chan_conf->reference = SL_HAL_IADC_VREF_VDDX;
		break;
	case ADC_REF_INTERNAL:
		chan_conf->reference = SL_HAL_IADC_REFERENCE_VREFINT_1V2;
		break;
#if defined(_IADC_CFG_REFSEL_VREF2P5)
	case ADC_REF_EXTERNAL1:
		chan_conf->reference = SL_HAL_IADC_VREF_EXT_2V5;
		break;
#endif
	case ADC_REF_EXTERNAL0:
		chan_conf->reference = SL_HAL_IADC_VREF_EXT_1V25;
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	chan_conf->initialized = true;

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int iadc_pm_action(const struct device *dev, enum pm_device_action action)
{
	int err;
	const struct iadc_config *config = dev->config;

	if (action == PM_DEVICE_ACTION_RESUME) {
		err = clock_control_on(config->clock_dev,
				       (clock_control_subsys_t)&config->clock_cfg);
		if (err < 0 && err != -EALREADY) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_PM_DEVICE) && (action == PM_DEVICE_ACTION_SUSPEND)) {
		err = clock_control_off(config->clock_dev,
					(clock_control_subsys_t)&config->clock_cfg);
		if (err < 0) {
			return err;
		}

		err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_SLEEP);
		if (err < 0 && err != -ENOENT) {
			return err;
		}
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int iadc_init(const struct device *dev)
{
	const struct iadc_config *config = dev->config;
	struct iadc_data *data = dev->data;
	int ret;

	data->dev = dev;

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg,
				     &data->clock_rate);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_ADC_SILABS_IADC_DMA
	ret = iadc_dma_init(dev);
	if (ret < 0) {
		data->dma.dma_dev = NULL;
	}
#endif

	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, iadc_pm_action);
}

static DEVICE_API(adc, iadc_api) = {
	.channel_setup = iadc_channel_setup,
	.read = iadc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = iadc_read_async,
#endif
	.ref_internal = SL_HAL_IADC_DEFAULT_VREF,
};

#ifdef CONFIG_ADC_SILABS_IADC_DMA
#define IADC_DMA_CHANNEL_INIT(n)                                                                   \
	.dma = {.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(n)),                                    \
		.dma_cfg = {                                                                       \
			.dma_slot =                                                                \
				SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_IDX(n, 0, slot)),  \
			.source_data_size = 2,                                                     \
			.dest_data_size = 2,                                                       \
			.source_burst_length = 2,                                                  \
			.dest_burst_length = 2,                                                    \
			.dma_callback = iadc_dma_cb,                                               \
		}},
#define IADC_DMA_CHANNEL(n) COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas),                            \
		 (IADC_DMA_CHANNEL_INIT(n)), ())
#else
#define IADC_DMA_CHANNEL(n)
#endif

#define IADC_INIT(n)                                                                               \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, iadc_pm_action);                                               \
                                                                                                   \
	static void iadc_config_func_##n(void);                                                    \
                                                                                                   \
	const static struct iadc_config iadc_config_##n = {                                        \
		.base = (IADC_TypeDef *)DT_INST_REG_ADDR(n),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n),                                          \
		.irq_cfg_func = iadc_config_func_##n,                                              \
	};                                                                                         \
                                                                                                   \
	static struct iadc_data iadc_data_##n = {ADC_CONTEXT_INIT_TIMER(iadc_data_##n, ctx),       \
						 ADC_CONTEXT_INIT_LOCK(iadc_data_##n, ctx),        \
						 ADC_CONTEXT_INIT_SYNC(iadc_data_##n, ctx),        \
						 IADC_DMA_CHANNEL(n)};                             \
                                                                                                   \
	static void iadc_config_func_##n(void)                                                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), iadc_isr,                   \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &iadc_init, PM_DEVICE_DT_INST_GET(n), &iadc_data_##n,             \
			      &iadc_config_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &iadc_api);

DT_INST_FOREACH_STATUS_OKAY(IADC_INIT)
