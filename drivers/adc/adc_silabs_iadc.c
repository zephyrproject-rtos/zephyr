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

/* Comptibility section for IADC IP version*/
#if (_IADC_IPVERSION_RESETVALUE == 0x00000000UL)

#define IADC_NO_DIGAVG 1

#define _IADC_CFG_DIGAVG_AVG1  -1
#define _IADC_CFG_DIGAVG_AVG2  -1
#define _IADC_CFG_DIGAVG_AVG4  -1
#define _IADC_CFG_DIGAVG_AVG8  -1
#define _IADC_CFG_DIGAVG_AVG16 -1

#define IADC_NO_EXTENDED_ALIGN 1

#define _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT16 -1
#define _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT20 -1
#define _IADC_SCANFIFOCFG_ALIGNMENT_LEFT16  -1
#define _IADC_SCANFIFOCFG_ALIGNMENT_LEFT20  -1

#define IADC_EXPLICIT_NEG_PIN 1

#endif

#define IADC_PORT_MASK 0xF0
#define IADC_PIN_MASK  0x0F

struct iadc_dma_channel {
	const struct device *dma_dev;
	struct dma_block_config blk_cfg;
	struct dma_config dma_cfg;
	int dma_channel;
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
	struct iadc_chan_conf chan_conf[SL_HAL_IADC_CHANNEL_ID_MAX];
	struct iadc_dma_channel dma;
	uint8_t adc_config_count; /* Number of ADC configs created (max 2) */
	uint32_t clock_rate;
	uint32_t channels;
	uint16_t active_channels;
	uint8_t alignment;
	uint8_t oversampling;
	uint8_t digital_averaging;
	size_t data_size;
	uint8_t *buffer;
};

struct iadc_config {
	sl_hal_iadc_config_t config;
	IADC_TypeDef *base;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	struct silabs_clock_control_cmu_config clock_cfg;
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

	if (data->adc_config_count >= ARRAY_SIZE(init->configs)) {
		LOG_ERR("Maximum of 2 different ADC configs supported");
		return -EINVAL;
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

	if (!dma->dma_dev) {
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
	dma->dma_cfg.complete_callback_en = 1;
	dma->dma_cfg.channel_priority = 3;
	dma->dma_cfg.channel_direction = PERIPHERAL_TO_MEMORY;
	dma->dma_cfg.head_block = &dma->blk_cfg;
	dma->dma_cfg.user_data = data;

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
		return -EBUSY;
	}

	ret = dma_config(dma->dma_dev, dma->dma_channel, &dma->dma_cfg);
	if (ret) {
		LOG_ERR("DMA config error: %d", ret);
		return ret;
	}

	dma->enabled = true;

	ret = dma_start(dma->dma_dev, dma->dma_channel);
	if (ret) {
		LOG_ERR("DMA start error: %d", ret);
		dma->enabled = false;
		return ret;
	}

	return 0;
}

static void iadc_dma_stop(const struct device *dev)
{
	struct iadc_data *data = dev->data;
	struct iadc_dma_channel *dma = &data->dma;

	if (!dma->enabled) {
		return;
	}

	dma_stop(dma->dma_dev, dma->dma_channel);

	dma->enabled = false;
}

static void iadc_dma_cb(const struct device *dma_dev, void *user_data, uint32_t channel, int status)
{
	struct iadc_data *data = user_data;
	const struct device *dev = data->dev;

	if (status < 0) {
		LOG_ERR("DMA transfer error: %d", status);
		adc_context_complete(&data->ctx, status);
		return;
	}

	iadc_dma_stop(dev);

	adc_context_on_sampling_done(&data->ctx, dev);
}
#endif /* CONFIG_ADC_SILABS_IADC_DMA */

/* Oversampling and resolution are common for both ADC configs
 * because they are not configurable per channel inside a ADC
 * sequence and are common for a sequence.
 */
static int iadc_set_config(const struct device *dev)
{
	const struct iadc_config *config = dev->config;
	IADC_TypeDef *iadc = config->base;
	struct iadc_data *data = dev->data;
	sl_hal_iadc_scan_table_t scan_table = {};
	sl_hal_iadc_init_t adc_init_config = {
		.configs[0].analog_gain = _IADC_CFG_ANALOGGAIN_ANAGAIN1,
		.configs[1].analog_gain = _IADC_CFG_ANALOGGAIN_ANAGAIN1,
		.configs[0].vref = SL_HAL_IADC_DEFAULT_VREF,
		.configs[1].vref = SL_HAL_IADC_DEFAULT_VREF,
		.configs[0].osr_high_speed = data->oversampling,
		.configs[1].osr_high_speed = data->oversampling,
#ifndef IADC_NO_DIGAVG
		.configs[0].dig_avg = data->digital_averaging,
		.configs[1].dig_avg = data->digital_averaging,
#endif
	};
	sl_hal_iadc_init_scan_t scan_init = {
		.data_valid_level = _IADC_SCANFIFOCFG_DVL_VALID4,
		.alignment = data->alignment,
	};
	struct iadc_chan_conf *chan_conf;
	uint32_t channels;
	int res;

	if (data->dma.dma_dev) {
		scan_init.data_valid_level = _IADC_SCANFIFOCFG_DVL_VALID1;
		/* Only needed to wake up DMA if EM is 2/3 */
		scan_init.fifo_dma_wakeup = true;
	}

	data->adc_config_count = 0;

	if (data->dma.dma_dev) {
		if (data->alignment == _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT20) {
			data->dma.dma_cfg.source_data_size = 4;
			data->dma.dma_cfg.dest_data_size = 4;
			data->dma.dma_cfg.source_burst_length = 4;
			data->dma.dma_cfg.dest_burst_length = 4;
		} else {
			data->dma.dma_cfg.source_data_size = 2;
			data->dma.dma_cfg.dest_data_size = 2;
			data->dma.dma_cfg.source_burst_length = 2;
			data->dma.dma_cfg.dest_burst_length = 2;
		}
	}

	channels = data->channels;

	/*
	 * Process each channel configuration and set up ADC scan sequence.
	 * The IADC hardware supports only 2 different ADC configurations
	 * (gain + reference combinations + oversampling), so we need to map
	 * multiple channel configs to these 2 available ADC configs.
	 */
	ARRAY_FOR_EACH(data->chan_conf, i) {
		chan_conf = &data->chan_conf[i];

		if (!chan_conf->initialized || (i != find_lsb_set(channels) - 1)) {
			continue;
		}

		res = iadc_find_or_create_adc_config(data, &adc_init_config, chan_conf);
		if (res < 0) {
			LOG_DBG("IADC: too many different ADC configurations");
			return res;
		}

		chan_conf->iadc_conf_id = res;

		iadc_configure_scan_table_entry(&scan_table.entries[i], chan_conf);

		channels &= ~BIT(i);
	}

	sl_hal_iadc_init(iadc, &adc_init_config, data->clock_rate);
	sl_hal_iadc_init_scan(iadc, &scan_init, &scan_table);
	sl_hal_iadc_set_scan_mask_multiple_entries(iadc, &scan_table);

	return 0;
}

static int iadc_check_buffer_size(const struct adc_sequence *sequence, uint16_t active_channels,
				  size_t data_size)
{
	size_t needed_buffer_size = active_channels * data_size;

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

/*
 * Goal of this function is to have concensius between wanted resolution, oversampling,
 * the IADC Alignment Table, analog oversampling and digital averaging.
 *
 * Formulas:
 * Output Resolution = 11 + log2(OversamplingRatio × DigitalAveraging)
 * +------------+-------------+---------------+---------------+-------------+
 * | Alignment  | Oversample  | Digital Avg   | Num Samples   | Output Res  |
 * | Setting    | Ratio       |               | Averaged      |             |
 * +------------+-------------+---------------+---------------+-------------+
 * | 16-bit     | 2x          | 1x            | 2             | 12 bits     |
 * | 16-bit     | 8x          | 2x            | 16            | 15 bits     |
 * | 20-bit     | 2x          | 1x            | 2             | 12 bits     |
 * | 20-bit     | 16x         | 4x            | 64            | 17 bits     |
 * +------------+-------------+---------------+---------------+-------------+
 */
static int iadc_check_oversampling_and_resolution(const struct adc_sequence *sequence,
						  struct iadc_data *data)
{
	int res = sequence->resolution;
	int ospl;

	const static struct oversampling_table {
		uint8_t analog_oversampling;
		uint8_t digital_averaging;
	} ospl_table[] = {
		[0]  = { _IADC_CFG_OSRHS_HISPD2,  _IADC_CFG_DIGAVG_AVG1  }, /* 2x oversampling */
		[1]  = { _IADC_CFG_OSRHS_HISPD2,  _IADC_CFG_DIGAVG_AVG1  }, /* 2x oversampling */
		[2]  = { _IADC_CFG_OSRHS_HISPD4,  _IADC_CFG_DIGAVG_AVG1  }, /* 4x oversampling */
		[3]  = { _IADC_CFG_OSRHS_HISPD8,  _IADC_CFG_DIGAVG_AVG1  }, /* 8x oversampling */
		[4]  = { _IADC_CFG_OSRHS_HISPD16, _IADC_CFG_DIGAVG_AVG1  }, /* 16x oversampling */
		[5]  = { _IADC_CFG_OSRHS_HISPD32, _IADC_CFG_DIGAVG_AVG1  }, /* 32x oversampling */
		[6]  = { _IADC_CFG_OSRHS_HISPD64, _IADC_CFG_DIGAVG_AVG1  }, /* 64x oversampling */
		[7]  = { _IADC_CFG_OSRHS_HISPD64, _IADC_CFG_DIGAVG_AVG2  }, /* 128x oversampling */
		[8]  = { _IADC_CFG_OSRHS_HISPD64, _IADC_CFG_DIGAVG_AVG4  }, /* 256x oversampling */
		[9]  = { _IADC_CFG_OSRHS_HISPD64, _IADC_CFG_DIGAVG_AVG8  }, /* 512x oversampling */
		[10] = { _IADC_CFG_OSRHS_HISPD64, _IADC_CFG_DIGAVG_AVG16 }, /* 1024x oversampling */
	};

	if (!sequence->oversampling) {
		ospl = 1;
	} else {
		ospl = sequence->oversampling;
	}

	if (ospl > ARRAY_SIZE(ospl_table) - 1) {
		LOG_ERR("Unsupported oversampling %d", sequence->oversampling);
		return -EINVAL;
	}

	if (ospl > 6 && IS_ENABLED(IADC_NO_DIGAVG)) {
		LOG_ERR("Unsupported oversampling %d", ospl);
		return -EINVAL;
	}

	if (res > 12 && IS_ENABLED(IADC_NO_EXTENDED_ALIGN)) {
		LOG_ERR("Unsupported resolution %d", res);
		return -EINVAL;
	}

	switch (res) {
	case 12:
		data->alignment = _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT12;
		break;
	case 16:
		data->alignment = _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT16;
		break;
	case 20:
		data->alignment = _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT20;
		break;
	default:
		LOG_ERR("Unsupported resolution %d", res);
		return -EINVAL;
	}

	data->oversampling = ospl_table[ospl].analog_oversampling;
	data->digital_averaging = ospl_table[ospl].digital_averaging;

	return 0;
}

static int start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct iadc_data *data = dev->data;
	uint32_t channels;
	uint16_t channel_count;
	uint16_t index;
	int res;

	if (sequence->channels == 0) {
		LOG_DBG("No channel requested");
		return -EINVAL;
	}

	res = iadc_check_oversampling_and_resolution(sequence, data);
	if (res < 0) {
		return res;
	}

	if (data->alignment == _IADC_SCANFIFOCFG_ALIGNMENT_RIGHT20) {
		data->data_size = sizeof(uint32_t);
	} else {
		data->data_size = sizeof(uint16_t);
	}

	if (sequence->calibrate) {
		/* TODO: Implement runtime calibration */
		LOG_DBG("Hardware have hardcoded calibration value but runtime calibration is not "
			"supported");
	}

	channels = sequence->channels;
	channel_count = 0;
	while (channels) {
		index = find_lsb_set(channels) - 1;
		if (index >= SL_HAL_IADC_CHANNEL_ID_MAX) {
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

	res = iadc_check_buffer_size(sequence, channel_count, data->data_size);
	if (res < 0) {
		return res;
	}

	data->buffer = sequence->buffer;
	data->active_channels = channel_count;

	if (data->dma.dma_dev) {
		data->dma.blk_cfg.dest_address = (uintptr_t)data->buffer;
		data->dma.blk_cfg.block_size = channel_count * data->data_size;
	}

	data->channels = sequence->channels;

	res = iadc_set_config(data->dev);
	if (res < 0) {
		return res;
	}

	adc_context_start_read(&data->ctx, sequence);

	res = adc_context_wait_for_completion(&data->ctx);

	return res;
}

static void iadc_start_scan(const struct device *dev)
{
	const struct iadc_config *config = dev->config;
	__maybe_unused struct iadc_data *data = dev->data;
	IADC_TypeDef *iadc = (IADC_TypeDef *)config->base;

#ifdef CONFIG_ADC_SILABS_IADC_DMA
	if (data->dma.dma_dev) {
		data->dma.blk_cfg.dest_address = (uintptr_t)data->buffer;
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

	iadc_start_scan(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct iadc_data *data = CONTAINER_OF(ctx, struct iadc_data, ctx);

	if (!repeat_sampling) {
		data->buffer += data->active_channels * data->data_size;
	}
}

static void iadc_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	const struct iadc_config *config = dev->config;
	struct iadc_data *data = dev->data;
	uint8_t *sample_ptr = data->buffer;
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
			memcpy(sample_ptr, &sample.data, data->data_size);
			sample_ptr += data->data_size;
		}

		adc_context_on_sampling_done(&data->ctx, dev);
	}

	if (err) {
		LOG_ERR("IADC error, flags=%08x", err);
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

	if (channel_cfg->channel_id < SL_HAL_IADC_CHANNEL_ID_MAX) {
		chan_conf = &data->chan_conf[channel_cfg->channel_id];
	} else {
		LOG_DBG("Requested channel index not available: %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	chan_conf->initialized = false;

	chan_conf->pos_port = (channel_cfg->input_positive & IADC_PORT_MASK) >> 4;
	chan_conf->pos_pin = channel_cfg->input_positive & IADC_PIN_MASK;

	if (channel_cfg->differential) {
		chan_conf->neg_port = (channel_cfg->input_negative & IADC_PORT_MASK) >> 4;
		chan_conf->neg_pin = channel_cfg->input_negative & IADC_PIN_MASK;
	} else {
		chan_conf->neg_port = _IADC_SCAN_PORTNEG_GND;
		if (chan_conf->pos_port == _IADC_SCAN_PORTPOS_SUPPLY &&
		    IS_ENABLED(IADC_EXPLICIT_NEG_PIN)) {
			chan_conf->neg_pin = 1;
		}
	}

	switch (channel_cfg->gain) {
	case ADC_GAIN_1_2:
		chan_conf->gain = _IADC_CFG_ANALOGGAIN_ANAGAIN0P5;
		break;
	case ADC_GAIN_1:
		chan_conf->gain = _IADC_CFG_ANALOGGAIN_ANAGAIN1;
		break;
	case ADC_GAIN_2:
		chan_conf->gain = _IADC_CFG_ANALOGGAIN_ANAGAIN2;
		break;
	case ADC_GAIN_3:
		chan_conf->gain = _IADC_CFG_ANALOGGAIN_ANAGAIN3;
		break;
	case ADC_GAIN_4:
		chan_conf->gain = _IADC_CFG_ANALOGGAIN_ANAGAIN4;
		break;
	default:
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -EINVAL;
	}

	/* Setup reference */
	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		chan_conf->reference = _IADC_CFG_REFSEL_VDDX;
		break;
	case ADC_REF_INTERNAL:
		chan_conf->reference = _IADC_CFG_REFSEL_VBGR;
		break;
	case ADC_REF_EXTERNAL0:
		chan_conf->reference = _IADC_CFG_REFSEL_VREF;
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'", channel_cfg->reference);
		return -EINVAL;
	}

	chan_conf->initialized = true;

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int iadc_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct iadc_config *config = dev->config;
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
	.dma.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR(n)),                                        \
	.dma.dma_cfg.dma_slot = SILABS_LDMA_REQSEL_TO_SLOT(DT_INST_DMAS_CELL_BY_IDX(n, 0, slot)),  \
	.dma.dma_cfg.dma_callback = iadc_dma_cb,
#define IADC_DMA_CHANNEL(n)                                                                        \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, dmas), (IADC_DMA_CHANNEL_INIT(n)), ())
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
	static struct iadc_data iadc_data_##n = {                                                  \
		ADC_CONTEXT_INIT_TIMER(iadc_data_##n, ctx),                                        \
		ADC_CONTEXT_INIT_LOCK(iadc_data_##n, ctx),                                         \
		ADC_CONTEXT_INIT_SYNC(iadc_data_##n, ctx),                                         \
		IADC_DMA_CHANNEL(n)                                                                \
	};                                                                                         \
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
