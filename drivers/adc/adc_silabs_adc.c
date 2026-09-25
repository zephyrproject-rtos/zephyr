/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_series3_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>

#include <sl_hal_adc.h>

LOG_MODULE_REGISTER(silabs_adc, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_PORT_MASK            0xF0
#define ADC_PIN_MASK             0x0F
#define ADC_MAX_ACQUISITION_TIME (_ADC_CFG_ATIME_MASK >> _ADC_CFG_ATIME_SHIFT)

/* ADC_SRC_CLK = branch / (src_prescale + 1), ADC_CORE_CLK = ADC_SRC_CLK / (adc_prescale + 1) */
#define ADC_CLK_SRC_MAX  MHZ(40)
#define ADC_CLK_CORE_MIN MHZ(17)
#define ADC_CLK_CORE_MAX MHZ(22)

struct adc_dma_channel {
	const struct device *dma_dev;
	struct dma_block_config blk_cfg;
	struct dma_config dma_cfg;
	int dma_channel;
	bool enabled;
};

struct adc_chan_conf {
	sl_hal_adc_analog_gain_t gain;
	sl_hal_adc_voltage_reference_t reference;
	sl_hal_adc_port_positive_t pos_port;
	uint8_t pos_pin;
	sl_hal_adc_port_negative_t neg_port;
	uint8_t neg_pin;
	uint16_t acquisition_time;
	uint8_t adc_conf_id;
	bool initialized;
};

struct silabs_adc_data {
	const struct device *dev;
	struct adc_context ctx;
	struct adc_chan_conf chan_conf[SL_HAL_ADC_CHANNEL_ID_MAX];
	struct adc_dma_channel dma;
	atomic_t sampling;
	uint8_t adc_config_count; /* Number of ADC configs created (max 2) */
	uint8_t src_prescale;
	uint8_t adc_prescale;
	uint32_t clock_rate;
	uint32_t core_clock_rate;
	uint32_t channels;
	uint16_t active_channels;
	sl_hal_adc_alignment_t alignment;
	sl_hal_adc_samples_t average;
	uint8_t *buffer;
};

struct adc_config {
	ADC_TypeDef *base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	struct silabs_clock_control_cmu_config clock_cfg;
	void (*irq_cfg_func)(void);
};

static void silabs_adc_stop(const struct device *dev);

static bool silabs_adc_core_clock_valid(uint32_t freq)
{
	return (freq >= ADC_CLK_CORE_MIN) && (freq <= ADC_CLK_CORE_MAX);
}

static int silabs_adc_calculate_prescalers(uint32_t clock_rate, uint8_t *src_prescale,
					   uint8_t *adc_prescale)
{
	for (uint8_t src_div = 0U; src_div <= SL_HAL_ADC_HSCLKRATE_DIV8; src_div++) {
		uint32_t src_clk = clock_rate / (src_div + 1U);
		uint32_t core_clk = src_clk / 2U;

		if (src_clk > ADC_CLK_SRC_MAX) {
			continue;
		}

		if (silabs_adc_core_clock_valid(src_clk)) {
			*src_prescale = src_div;
			*adc_prescale = 0U;
			return 0;
		}

		if (silabs_adc_core_clock_valid(core_clk)) {
			*src_prescale = src_div;
			*adc_prescale = 1U;
			return 0;
		}
	}

	LOG_ERR("No valid ADC prescalers for clock rate %u Hz", clock_rate);
	return -EINVAL;
}

static int silabs_adc_find_or_create_adc_config(struct silabs_adc_data *data,
						sl_hal_adc_init_t *init,
						const struct adc_chan_conf *chan_conf)
{
	int adc_conf_id;

	/* Check if we can reuse existing ADC configs */
	for (int i = 0; i < data->adc_config_count; i++) {
		if (chan_conf->gain == init->config[i].gain &&
		    chan_conf->acquisition_time == init->config[i].acquisition_time) {
			return i;
		}
	}

	if (data->adc_config_count >= ARRAY_SIZE(init->config)) {
		LOG_ERR("Maximum of 2 different ADC configs supported");
		return -EINVAL;
	}

	adc_conf_id = data->adc_config_count;
	init->config[adc_conf_id].gain = chan_conf->gain;
	init->config[adc_conf_id].acquisition_time = chan_conf->acquisition_time;
	data->adc_config_count++;

	return adc_conf_id;
}

static void silabs_adc_configure_scan_entry(sl_hal_adc_scan_entry_t *entry,
					    const struct adc_chan_conf *chan_conf)
{
	*entry = (sl_hal_adc_scan_entry_t){
		.pos_port = chan_conf->pos_port,
		.pos_pin = chan_conf->pos_pin,
		.neg_port = chan_conf->neg_port,
		.neg_pin = chan_conf->neg_pin,
		.config_id = chan_conf->adc_conf_id,
		.compare = false,
	};
}

static bool silabs_adc_clear_sampling(struct silabs_adc_data *data)
{
	return atomic_cas(&data->sampling, 1, 0);
}

static void silabs_adc_complete_error(struct adc_context *ctx, int err)
{
	if ((ctx->sequence.options != NULL) && (ctx->options.interval_us != 0U)) {
		adc_context_disable_timer(ctx);
	}
	adc_context_complete(ctx, err);
}

#ifdef CONFIG_ADC_SILABS_ADC_DMA
static int silabs_adc_dma_init(const struct device *dev)
{
	const struct adc_config *config = dev->config;
	struct silabs_adc_data *data = dev->data;
	struct adc_dma_channel *dma = &data->dma;

	if (dma->dma_dev == NULL) {
		return 0;
	}

	if (!device_is_ready(dma->dma_dev)) {
		LOG_ERR("DMA device not ready");
		return -ENODEV;
	}

	dma->dma_channel = dma_request_channel(dma->dma_dev, NULL);
	if (dma->dma_channel < 0) {
		LOG_ERR("Failed to request DMA channel");
		return -ENODEV;
	}

	memset(&dma->blk_cfg, 0, sizeof(dma->blk_cfg));
	dma->blk_cfg.source_address = (uintptr_t)&(config->base)->SCANFIFODATA;
	dma->blk_cfg.source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	dma->blk_cfg.dest_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	dma->dma_cfg.source_data_size = sizeof(uint16_t);
	dma->dma_cfg.dest_data_size = sizeof(uint16_t);
	dma->dma_cfg.source_burst_length = sizeof(uint16_t);
	dma->dma_cfg.dest_burst_length = sizeof(uint16_t);
	dma->dma_cfg.complete_callback_en = 1;
	dma->dma_cfg.channel_priority = 3;
	dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma->dma_cfg.head_block = &dma->blk_cfg;
	dma->dma_cfg.user_data = data;

	return 0;
}

static int silabs_adc_dma_start(const struct device *dev)
{
	struct silabs_adc_data *data = dev->data;
	struct adc_dma_channel *dma = &data->dma;
	int ret;

	if (dma->dma_dev == NULL) {
		return -ENODEV;
	}

	if (dma->enabled) {
		return -EBUSY;
	}

	ret = dma_config(dma->dma_dev, dma->dma_channel, &dma->dma_cfg);
	if (ret < 0) {
		LOG_ERR("DMA config error: %d", ret);
		return ret;
	}

	dma->enabled = true;

	ret = dma_start(dma->dma_dev, dma->dma_channel);
	if (ret < 0) {
		LOG_ERR("DMA start error: %d", ret);
		dma->enabled = false;
		return ret;
	}

	return 0;
}

static void silabs_adc_dma_stop(const struct device *dev)
{
	struct silabs_adc_data *data = dev->data;
	struct adc_dma_channel *dma = &data->dma;

	if (!dma->enabled) {
		return;
	}

	dma_stop(dma->dma_dev, dma->dma_channel);

	dma->enabled = false;
}

static void silabs_adc_dma_cb(const struct device *dma_dev, void *user_data, uint32_t channel,
			      int status)
{
	struct silabs_adc_data *data = user_data;
	const struct device *dev = data->dev;
	const struct adc_config *config = dev->config;
	uint32_t flags;

	flags = sl_hal_adc_get_enabled_pending_interrupts(config->base);

	if (!silabs_adc_clear_sampling(data)) {
		/* ADC ISR has already handled an error and ended sampling */
		return;
	}

	if (status < 0) {
		silabs_adc_stop(dev);
		LOG_ERR("DMA transfer error: %d", status);
		silabs_adc_complete_error(&data->ctx, status);
		return;
	}

	if (flags != 0U) {
		silabs_adc_stop(dev);
		LOG_ERR("ADC error, flags=%08x", flags);
		silabs_adc_complete_error(&data->ctx, -EIO);
		return;
	}

	silabs_adc_dma_stop(dev);

	adc_context_on_sampling_done(&data->ctx, dev);
}
#endif /* CONFIG_ADC_SILABS_ADC_DMA */

static void silabs_adc_stop(const struct device *dev)
{
	const struct adc_config *config = dev->config;

	sl_hal_adc_disable_interrupts(config->base, _ADC_IEN_MASK);
	sl_hal_adc_stop(config->base);
	sl_hal_adc_clear_interrupts(config->base, _ADC_IF_MASK);
#ifdef CONFIG_ADC_SILABS_ADC_DMA
	silabs_adc_dma_stop(dev);
#endif
}

/* Oversampling and resolution are common for both ADC configs
 * because they are not configurable per channel inside a ADC
 * sequence and are common for a sequence.
 */
static int silabs_adc_set_config(const struct device *dev)
{
	const struct adc_config *config = dev->config;
	ADC_TypeDef *adc = config->base;
	struct silabs_adc_data *data = dev->data;
	sl_hal_adc_init_t init = SL_HAL_ADC_INIT_DEFAULT;

	init.config[0] = (sl_hal_adc_config_t)SL_HAL_ADC_CONFIG_DEFAULT;
	init.config[1] = (sl_hal_adc_config_t)SL_HAL_ADC_CONFIG_DEFAULT;
	init.config[0].average = data->average;
	init.config[1].average = data->average;
	init.alignment = data->alignment;
	init.scan_trigger = SL_HAL_ADC_TRIGGER_IMMEDIATE;
	init.scan_trigger_action = SL_HAL_ADC_TRIGGER_ACTION_ONCE;
	init.voltage_reference = 255;
	struct adc_chan_conf *chan_conf;
	uint32_t channels;
	int res;

	data->adc_config_count = 0;
	channels = data->channels;

	/*
	 * Process each channel configuration and set up ADC scan sequence.
	 * The ADC hardware supports only 2 different ADC configurations
	 * (gain + acquisition time), so we need to map
	 * multiple channel configs to these 2 available ADC configs.
	 * Only one reference configuration is supported.
	 */
	ARRAY_FOR_EACH(data->chan_conf, i) {
		chan_conf = &data->chan_conf[i];

		if (!chan_conf->initialized || (i != find_lsb_set(channels) - 1)) {
			continue;
		}

		res = silabs_adc_find_or_create_adc_config(data, &init, chan_conf);
		if (res < 0) {
			LOG_DBG("ADC: too many different ADC configurations");
			return res;
		}

		if (init.voltage_reference < 255 &&
		    chan_conf->reference != init.voltage_reference) {
			LOG_DBG("ADC: too many different voltage references");
			return -EINVAL;
		}
		init.voltage_reference = chan_conf->reference;

		chan_conf->adc_conf_id = res;

		silabs_adc_configure_scan_entry(&init.entries[i], chan_conf);

		channels &= ~BIT(i);
	}

	sl_hal_adc_disable(adc);
	sl_hal_adc_wait_ready(adc);
	sl_hal_adc_init(adc, &init, data->clock_rate);
	sl_hal_adc_set_clock_prescalers(adc, data->clock_rate, data->src_prescale,
					data->adc_prescale);
	sl_hal_adc_enable(adc);
	sl_hal_adc_flush_fifo(adc);
	while ((sl_hal_adc_get_status(adc) & ADC_STATUS_SCANFIFOFLUSHING) != 0U) {
	}

	sl_hal_adc_set_scan_mask(adc, data->channels);

	return 0;
}

static int silabs_adc_check_buffer_size(const struct adc_sequence *sequence,
					uint16_t active_channels)
{
	size_t needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options != NULL) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_DBG("Provided buffer is too small (%u/%u)", sequence->buffer_size,
			needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int silabs_adc_check_oversampling_and_resolution(const struct adc_sequence *sequence,
							struct silabs_adc_data *data)
{
	switch (sequence->resolution) {
	case 8:
		data->alignment = SL_HAL_ADC_ALIGNMENT_RIGHT_8;
		break;
	case 12:
		data->alignment = SL_HAL_ADC_ALIGNMENT_RIGHT_12;
		break;
	case 16:
		if (sequence->oversampling < 4) {
			LOG_ERR("16-bit resolution is only supported when oversampling >= 4.");
			return -EINVAL;
		}
		data->alignment = SL_HAL_ADC_ALIGNMENT_RIGHT_16;
		break;
	default:
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -EINVAL;
	}

	if (sequence->oversampling > SL_HAL_ADC_AVERAGE_X1024) {
		LOG_ERR("Unsupported oversampling %d", sequence->oversampling);
		return -EINVAL;
	}
	if ((sequence->oversampling > SL_HAL_ADC_AVERAGE_X1) && (data->adc_prescale == 0U)) {
		LOG_ERR("Oversampling is not supported when ADC core clock is undivided");
		return -EINVAL;
	}
	data->average = (sl_hal_adc_samples_t)sequence->oversampling;

	return 0;
}

static int silabs_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct silabs_adc_data *data = dev->data;
	uint32_t channels;
	uint16_t channel_count;
	uint16_t index;
	int res;

	if (sequence->channels == 0) {
		LOG_DBG("No channel requested");
		return -EINVAL;
	}

	res = silabs_adc_check_oversampling_and_resolution(sequence, data);
	if (res < 0) {
		return res;
	}

	if (sequence->calibrate) {
		LOG_DBG("Runtime calibration is not supported");
		/* Do not return an error, API contract states "ADC implementations that do not
		 * support calibration should ignore this flag.""
		 */
	}

	channels = sequence->channels;
	channel_count = 0;
	while (channels != 0U) {
		index = find_lsb_set(channels) - 1;
		if (index >= SL_HAL_ADC_CHANNEL_ID_MAX) {
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

	res = silabs_adc_check_buffer_size(sequence, channel_count);
	if (res < 0) {
		return res;
	}

	data->buffer = sequence->buffer;
	data->active_channels = channel_count;

	if (data->dma.dma_dev != NULL) {
		data->dma.blk_cfg.dest_address = (uintptr_t)data->buffer;
		data->dma.blk_cfg.block_size = channel_count * sizeof(uint16_t);
	}

	data->channels = sequence->channels;

	res = silabs_adc_set_config(data->dev);
	if (res < 0) {
		return res;
	}

	adc_context_start_read(&data->ctx, sequence);

	res = adc_context_wait_for_completion(&data->ctx);

	return res;
}

static void silabs_adc_start_scan(const struct device *dev)
{
	const struct adc_config *config = dev->config;
	struct silabs_adc_data *data = dev->data;
	ADC_TypeDef *adc = (ADC_TypeDef *)config->base;

	sl_hal_adc_clear_interrupts(adc, _ADC_IF_MASK);

#ifdef CONFIG_ADC_SILABS_ADC_DMA
	if (data->dma.dma_dev) {
		int ret;

		data->dma.blk_cfg.dest_address = (uintptr_t)data->buffer;
		ret = silabs_adc_dma_start(dev);
		if (ret < 0) {
			silabs_adc_complete_error(&data->ctx, ret);
			return;
		}
	} else {
		sl_hal_adc_enable_interrupts(adc, ADC_IEN_SCANTABLEDONE);
	}
#else
	sl_hal_adc_enable_interrupts(adc, ADC_IEN_SCANTABLEDONE);
#endif

	sl_hal_adc_enable_interrupts(adc, (ADC_IEN_PORTALLOCERR | ADC_IEN_POLARITYERR |
					   ADC_IEN_SCANFIFOOF | ADC_IEN_SCANFIFOUF));

	atomic_set(&data->sampling, 1);

	sl_hal_adc_start(adc);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct silabs_adc_data *data = CONTAINER_OF(ctx, struct silabs_adc_data, ctx);

	silabs_adc_start_scan(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct silabs_adc_data *data = CONTAINER_OF(ctx, struct silabs_adc_data, ctx);

	if (!repeat_sampling) {
		data->buffer += data->active_channels * sizeof(uint16_t);
	}
}

static void silabs_adc_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct adc_config *config = dev->config;
	struct silabs_adc_data *data = dev->data;
	uint8_t *sample_ptr = data->buffer;
	ADC_TypeDef *adc = config->base;
	uint32_t flags, err, sample;
	bool sampling_active;

	flags = sl_hal_adc_get_enabled_pending_interrupts(adc);
	if (flags == 0U) {
		return;
	}

	sampling_active = silabs_adc_clear_sampling(data);
	sl_hal_adc_clear_interrupts(adc, flags);

	if (!sampling_active) {
		/* DMA callback has already ended sampling and handled potential errors */
		return;
	}

	err = flags &
	      (ADC_IF_PORTALLOCERR | ADC_IF_POLARITYERR | ADC_IF_SCANFIFOOF | ADC_IF_SCANFIFOUF);

	if (err != 0U) {
		silabs_adc_stop(dev);
		LOG_ERR("ADC error, flags=%08x", err);
		silabs_adc_complete_error(&data->ctx, -EIO);
		return;
	}

	if ((flags & ADC_IF_SCANTABLEDONE) != 0U) {
		while (sl_hal_adc_get_fifo_count(adc) > 0) {
			/*
			 * The FIFO word holds the sample sign extended up to the ID field.
			 * sl_hal_adc_pull() cannot be used as it masks the sample to the configured
			 * resolution, dropping the sign of differential results.
			 */
			sample = adc->SCANFIFODATA;

			memcpy(sample_ptr, &sample, sizeof(uint16_t));
			sample_ptr += sizeof(uint16_t);
		}

		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

static int silabs_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct silabs_adc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, false, NULL);
	error = silabs_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#ifdef CONFIG_ADC_ASYNC
static int silabs_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct silabs_adc_data *data = dev->data;
	int error;

	adc_context_lock(&data->ctx, true, async);
	error = silabs_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif

static int silabs_adc_acquisition_time_to_cycles(const struct device *dev,
						 uint16_t acquisition_time, uint16_t *cycles)
{
	const struct silabs_adc_data *data = dev->data;
	uint64_t acquisition_ns;
	uint64_t cycle_count;

	if (acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		*cycles = 4;
		return 0;
	}

	switch (ADC_ACQ_TIME_UNIT(acquisition_time)) {
	case ADC_ACQ_TIME_TICKS:
		cycle_count = ADC_ACQ_TIME_VALUE(acquisition_time);
		break;
	case ADC_ACQ_TIME_MICROSECONDS:
		acquisition_ns = (uint64_t)ADC_ACQ_TIME_VALUE(acquisition_time) * NSEC_PER_USEC;
		cycle_count = DIV_ROUND_UP(acquisition_ns * data->core_clock_rate, NSEC_PER_SEC);
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		acquisition_ns = ADC_ACQ_TIME_VALUE(acquisition_time);
		cycle_count = DIV_ROUND_UP(acquisition_ns * data->core_clock_rate, NSEC_PER_SEC);
		break;
	default:
		LOG_ERR("Unsupported acquisition time unit: %lu",
			ADC_ACQ_TIME_UNIT(acquisition_time));
		return -EINVAL;
	}

	if (cycle_count > ADC_MAX_ACQUISITION_TIME) {
		LOG_ERR("Acquisition time exceeds maximum of %lu ADC clock cycles",
			ADC_MAX_ACQUISITION_TIME);
		return -EINVAL;
	}

	*cycles = (uint16_t)cycle_count;

	return 0;
}

static int silabs_adc_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	struct silabs_adc_data *data = dev->data;
	struct adc_chan_conf *chan_conf = NULL;
	int ret;

	if (channel_cfg->channel_id < SL_HAL_ADC_CHANNEL_ID_MAX) {
		chan_conf = &data->chan_conf[channel_cfg->channel_id];
	} else {
		LOG_DBG("Requested channel index not available: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	chan_conf->initialized = false;

	ret = silabs_adc_acquisition_time_to_cycles(dev, channel_cfg->acquisition_time,
						    &chan_conf->acquisition_time);
	if (ret < 0) {
		return ret;
	}

	chan_conf->pos_port = (channel_cfg->input_positive & ADC_PORT_MASK) >> 4;
	chan_conf->pos_pin = channel_cfg->input_positive & ADC_PIN_MASK;

	if (channel_cfg->differential) {
		chan_conf->neg_port = (channel_cfg->input_negative & ADC_PORT_MASK) >> 4;
		chan_conf->neg_pin = channel_cfg->input_negative & ADC_PIN_MASK;
	} else {
		chan_conf->neg_port = SL_HAL_ADC_PORT_NEG_GND;
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_2:
		chan_conf->gain = SL_HAL_ADC_ANALOG_GAIN_0_5;
		break;
	case ADC_GAIN_1:
		chan_conf->gain = SL_HAL_ADC_ANALOG_GAIN_1;
		break;
	case ADC_GAIN_2:
		chan_conf->gain = SL_HAL_ADC_ANALOG_GAIN_2;
		break;
	case ADC_GAIN_4:
		chan_conf->gain = SL_HAL_ADC_ANALOG_GAIN_4;
		break;
	default:
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -EINVAL;
	}

	/* Setup reference */
	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		chan_conf->reference = SL_HAL_ADC_REFERENCE_VDDA;
		break;
	case ADC_REF_INTERNAL:
		chan_conf->reference = SL_HAL_ADC_REFERENCE_VREFINT;
		break;
	case ADC_REF_EXTERNAL0:
		chan_conf->reference = SL_HAL_ADC_REFERENCE_VREFPL;
		break;
	case ADC_REF_EXTERNAL1:
		chan_conf->reference = SL_HAL_ADC_REFERENCE_VREFPH;
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'", channel_cfg->reference);
		return -EINVAL;
	}

	chan_conf->initialized = true;

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int silabs_adc_ref_get(const struct device *dev, enum adc_reference ref, uint16_t *vref_mv)
{
	const struct adc_config *config = dev->config;

	switch (ref) {
	case ADC_REF_INTERNAL:
		*vref_mv = (uint16_t)sl_hal_adc_get_internal_reference_voltage(config->base);
		break;
	case ADC_REF_VDD_1:
	case ADC_REF_EXTERNAL0:
	case ADC_REF_EXTERNAL1:
		return -ENODATA;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int adc_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct adc_config *config = dev->config;
	int err;

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
		sl_hal_adc_disable(config->base);
		sl_hal_adc_wait_ready(config->base);

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

static int adc_init(const struct device *dev)
{
	const struct adc_config *config = dev->config;
	struct silabs_adc_data *data = dev->data;
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

	ret = silabs_adc_calculate_prescalers(data->clock_rate, &data->src_prescale,
					      &data->adc_prescale);
	if (ret < 0) {
		return ret;
	}
	data->core_clock_rate =
		data->clock_rate / (data->src_prescale + 1U) / (data->adc_prescale + 1U);

#ifdef CONFIG_ADC_SILABS_ADC_DMA
	ret = silabs_adc_dma_init(dev);
	if (ret < 0) {
		data->dma.dma_dev = NULL;
	}
#endif

	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return pm_device_driver_init(dev, adc_pm_action);
}

static DEVICE_API(adc, adc_api) = {
	.channel_setup = silabs_adc_channel_setup,
	.read = silabs_adc_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = silabs_adc_read_async,
#endif
	.ref_get = silabs_adc_ref_get,
};

#ifdef CONFIG_ADC_SILABS_ADC_DMA
#define ADC_DMA_CHANNEL_INIT(n)                                                                    \
	.dma.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(n)),                                        \
	.dma.dma_cfg.dma_slot = SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_IDX(n, 0, slot)),  \
	.dma.dma_cfg.dma_callback = silabs_adc_dma_cb,
/* clang-format off */
#define ADC_DMA_CHANNEL(n)                                                                         \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas), (ADC_DMA_CHANNEL_INIT(n)), ())
/* clang-format on */
#else
#define ADC_DMA_CHANNEL(n)
#endif

#define ADC_INIT(n)                                                                                \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PM_DEVICE_DT_INST_DEFINE(n, adc_pm_action);                                                \
                                                                                                   \
	static void adc_config_func_##n(void);                                                     \
                                                                                                   \
	static const struct adc_config adc_config_##n = {                                          \
		.base = (ADC_TypeDef *)DT_INST_REG_ADDR(n),                                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(n),                                          \
		.irq_cfg_func = adc_config_func_##n,                                               \
	};                                                                                         \
                                                                                                   \
	static struct silabs_adc_data adc_data_##n = {ADC_CONTEXT_INIT_TIMER(adc_data_##n, ctx),   \
						      ADC_CONTEXT_INIT_LOCK(adc_data_##n, ctx),    \
						      ADC_CONTEXT_INIT_SYNC(adc_data_##n, ctx),    \
						      ADC_DMA_CHANNEL(n)};                         \
                                                                                                   \
	static void adc_config_func_##n(void)                                                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), silabs_adc_isr,             \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &adc_init, PM_DEVICE_DT_INST_GET(n), &adc_data_##n,               \
			      &adc_config_##n, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_INIT)
