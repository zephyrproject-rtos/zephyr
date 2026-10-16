/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT waveshare_ch32v003_adc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "adc_waveshare_ch32v003.h"

LOG_MODULE_REGISTER(adc_ch32v003, CONFIG_ADC_LOG_LEVEL);

/* This controller exposes a single fixed 10-bit ADC channel. */
#define CH32V003_ADC_RESOLUTION 10U

struct ch32v003_adc_config {
	const struct device *parent;
};

static int ch32v003_adc_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id != 0U) {
		LOG_ERR("unsupported channel id %u", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("unsupported reference %d", channel_cfg->reference);
		return -ENOTSUP;
	}

	return 0;
}

static int ch32v003_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct ch32v003_adc_config *cfg = dev->config;
	uint16_t *buf = sequence->buffer;
	uint32_t samples = 1U;
	int ret;

	if (sequence->channels != BIT(0)) {
		LOG_ERR("only channel 0 is supported");
		return -ENOTSUP;
	}

	if (sequence->resolution != CH32V003_ADC_RESOLUTION) {
		LOG_ERR("unsupported resolution %u", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->options) {
		samples += sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < samples * sizeof(uint16_t)) {
		LOG_ERR("buffer too small for the requested sequence");
		return -ENOMEM;
	}

	for (uint32_t i = 0; i < samples; i++) {
		ret = ch32v003_get_adc_value(cfg->parent, &buf[i]);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int ch32v003_adc_init(const struct device *dev)
{
	const struct ch32v003_adc_config *cfg = dev->config;

	if (!device_is_ready(cfg->parent)) {
		LOG_ERR("Parent CH32V003 device is not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(adc, ch32v003_adc_api) = {
	.channel_setup = ch32v003_adc_channel_setup,
	.read = ch32v003_adc_read,
};

#define CH32V003_ADC_INST_DEFINE(n)                                                                \
	static const struct ch32v003_adc_config ch32v003_adc_cfg_##n = {                           \
		.parent = DEVICE_DT_GET(DT_INST_PARENT(n)),                                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ch32v003_adc_init, NULL, NULL, &ch32v003_adc_cfg_##n,             \
			      POST_KERNEL, CONFIG_ADC_WAVESHARE_CH32V003_INIT_PRIORITY,            \
			      &ch32v003_adc_api);

DT_INST_FOREACH_STATUS_OKAY(CH32V003_ADC_INST_DEFINE)
