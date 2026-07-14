/*
 * Copyright (c) 2026 Leica Geosystems AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/mfd/tla2528.h>
#include <zephyr/logging/log.h>
#include <zephyr/arch/common/ffs.h>
#include <zephyr/toolchain.h>
#include <stdint.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DT_DRV_COMPAT ti_tla2528_adc

LOG_MODULE_DECLARE(TLA2528, CONFIG_ADC_LOG_LEVEL);

#define TLA2528_NUM_CHANNELS            8
#define TLA2528_REF_INTERNAL            0
#define TLA2528_RESOLUTION_DEFAULT      12
#define TLA2528_RESOLUTION_OVERSAMPLING 16
#define TLA2528_MAX_OVERSAMPLING        7
#define TLA2528_CHANNEL_MASK            BIT_MASK(TLA2528_NUM_CHANNELS)

struct adc_tla2528_config {
	const struct device *parent;
};

struct adc_tla2528_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	int channels;
	bool avg_en;
};

static void tla2528_perform_read(const struct device *dev)
{
	const struct adc_tla2528_config *cfg = dev->config;
	struct adc_tla2528_data *data = dev->data;
	int channels = data->channels;

	while (channels > 0) {
		const uint8_t channel = find_lsb_set(channels) - 1;

		int err = tla2528_register_write(cfg->parent, TLA2528_CHANNEL_SEL, channel);

		if (err != 0) {
			LOG_ERR("failed to select channel, %d", err);
			return;
		}

		/* start conversion */
		err = tla2528_register_set_bits(cfg->parent, TLA2528_GENERAL_CFG, TLA2528_CNVST);
		if (err != 0) {
			LOG_ERR("failed to start conversion, %d", err);
			return;
		}

		err = tla2528_read_adc_data(cfg->parent, data->buffer);
		if (err != 0) {
			LOG_ERR("failed to read adc data, %d", err);
			return;
		}

		if (!data->avg_en) {
			/*
			 * when oversampling is disabled, data is returned in 12-bit format, with
			 * the lower 4 bits unused
			 */
			*data->buffer = *data->buffer >> 4u;
		}

		WRITE_BIT(channels, channel, 0);

		++data->buffer;
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_tla2528_data *data = CONTAINER_OF(ctx, struct adc_tla2528_data, ctx);

	data->repeat_buffer = data->buffer;

	tla2528_perform_read(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_tla2528_data *data = CONTAINER_OF(ctx, struct adc_tla2528_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static size_t tla2528_get_min_buffersize(const struct adc_sequence *sequence)
{
	const uint8_t num_channels = POPCOUNT(sequence->channels);

	size_t min_size = sizeof(*((struct adc_tla2528_data){}).buffer) * num_channels;

	if (sequence->options) {
		min_size *= (1 + sequence->options->extra_samplings);
	}

	return min_size;
}

static int tla2528_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_tla2528_config *cfg = dev->config;
	struct adc_tla2528_data *data = dev->data;

	if (sequence->channels & ~TLA2528_CHANNEL_MASK) {
		LOG_ERR("unsupported channel selection: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}
	data->channels = sequence->channels;

	if (sequence->buffer_size < tla2528_get_min_buffersize(sequence)) {
		LOG_ERR("buffer too small");
		return -ENOMEM;
	}

	if (sequence->resolution != (sequence->oversampling ? TLA2528_RESOLUTION_OVERSAMPLING
							    : TLA2528_RESOLUTION_DEFAULT)) {
		LOG_ERR("unsupported resolution: %u", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->oversampling > TLA2528_MAX_OVERSAMPLING) {
		LOG_ERR("unsupported oversampling setting: %u", sequence->oversampling);
		return -ENOTSUP;
	}

	const int err =
		tla2528_register_write(cfg->parent, TLA2528_OSR_CFG, sequence->oversampling);
	if (err != 0) {
		LOG_ERR("failed to configure oversampling, %d", err);
		return err;
	}

	data->avg_en = sequence->oversampling != 0;

	if (sequence->calibrate) {
		LOG_ERR("calibration not implemented");
		return -ENOSYS;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return 0;
}

static int tla2528_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_tla2528_config *cfg = dev->config;

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("unsupported reference: %u", channel_cfg->reference);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential mode not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported gain: %u", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->channel_id >= TLA2528_NUM_CHANNELS) {
		LOG_ERR("unsupported channel_id: %u (device has only 8 channels)",
			channel_cfg->channel_id);
		return -EINVAL;
	}

	const uint8_t pin_config = BIT(channel_cfg->channel_id);

	return tla2528_register_clear_bits(cfg->parent, TLA2528_PIN_CFG, pin_config);
}

static int tla2528_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int ret;
	struct adc_tla2528_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);

	ret = tla2528_start_read(dev, sequence);

	adc_context_release(&data->ctx, ret);

	return ret;
}

static int tla2528_init(const struct device *dev)
{
	const struct adc_tla2528_config *cfg = dev->config;
	struct adc_tla2528_data *data = dev->data;

	if (!device_is_ready(cfg->parent)) {
		return -ENODEV;
	}

	adc_context_init(&data->ctx);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_tla2528_api) = {
	.channel_setup = tla2528_channel_setup,
	.read = tla2528_read,
	.ref_internal = TLA2528_REF_INTERNAL,
};

#define ADC_TLA2528_INST_DEFINE(n)                                                                 \
	static const struct adc_tla2528_config config_##n = {                                      \
		.parent = DEVICE_DT_GET(DT_INST_BUS(n))};                                          \
	static struct adc_tla2528_data data_##n = {.dev = DEVICE_DT_INST_GET(n)};                  \
	DEVICE_DT_INST_DEFINE(n, tla2528_init, NULL, &data_##n, &config_##n, POST_KERNEL,          \
			      CONFIG_ADC_TLA2528_INIT_PRIO, &adc_tla2528_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_TLA2528_INST_DEFINE);
