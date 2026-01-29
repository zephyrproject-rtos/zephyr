/*
 * Copyright (c) 2025 Axel Utech <utech@sofiha.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER

/*
 * This requires to be included _after_  `#define ADC_CONTEXT_USES_KERNEL_TIMER`
 */
#include "adc_context.h"

LOG_MODULE_REGISTER(mcp342x, CONFIG_ADC_LOG_LEVEL);

#define ACQ_THREAD_PRIORITY   CONFIG_ADC_MCP342X_ACQUISITION_THREAD_PRIORITY
#define ACQ_THREAD_STACK_SIZE CONFIG_ADC_MCP342X_ACQUISITION_THREAD_STACK_SIZE

#define MAX_CHANNELS 4

struct mcp342x_reg_config {
	union {
		struct {
			uint8_t pga: 2;
			uint8_t sampleRate: 2;
			uint8_t conversionMode: 1;
			uint8_t channelSelection: 2;
			uint8_t notReady: 1;
		};
		uint8_t raw;
	};
};

enum {
	PGA_GAIN1 = 0,
	PGA_GAIN2 = 1,
	PGA_GAIN4 = 2,
	PGA_GAIN8 = 3
};

enum {
	SAMPLERATE_240SPS_12BITS = 0,
	SAMPLERATE_60SPS_14BITS = 1,
	SAMPLERATE_15SPS_16BITS = 2,
};

typedef int16_t mcp342x_reg_data_t;
typedef struct mcp342x_reg_config mcp342x_reg_config_t;

struct mcp342x_config {
	const struct i2c_dt_spec bus;
	uint8_t channel_count;
};

struct mcp342x_data {
	const struct device *dev;
	struct adc_context ctx;
#ifdef CONFIG_ADC_ASYNC
	struct k_sem acq_lock;
#endif
	mcp342x_reg_data_t *buffer;
	mcp342x_reg_data_t *repeat_buffer;
	uint8_t channels;
	uint8_t resolution;

	/*
	 * Shadow register
	 */
	mcp342x_reg_config_t reg_config[MAX_CHANNELS];
};

static int mcp342x_read_register(const struct device *dev, mcp342x_reg_data_t *data,
				 mcp342x_reg_config_t *configData)
{
	int ret;
	const struct mcp342x_config *config = dev->config;
	uint8_t tmp[3];

	ret = i2c_read_dt(&config->bus, tmp, sizeof(tmp));
	if (ret) {
		return ret;
	}

	*data = sys_get_be16(tmp);
	configData->raw = tmp[2];

	return 0;
}

static int mcp342x_write_register(const struct device *dev, mcp342x_reg_config_t value)
{
	int ret;
	const struct mcp342x_config *config = dev->config;

	ret = i2c_write_dt(&config->bus, &value.raw, sizeof(value.raw));
	if (ret) {
		return ret;
	}

	return 0;
}

static int mcp342x_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;

	if (cfg->channel_id >= config->channel_count) {
		LOG_ERR("invalid channel selection %u", cfg->channel_id);
		return -EINVAL;
	}
	mcp342x_reg_config_t *reg_config = &data->reg_config[cfg->channel_id];

	reg_config->channelSelection = cfg->channel_id;

	switch (cfg->gain) {
	case ADC_GAIN_1:
		reg_config->pga = PGA_GAIN1;
		break;
	case ADC_GAIN_2:
		reg_config->pga = PGA_GAIN2;
		break;
	case ADC_GAIN_4:
		reg_config->pga = PGA_GAIN4;
		break;
	case ADC_GAIN_8:
		reg_config->pga = PGA_GAIN8;
		break;
	default:
		LOG_ERR("Invalid gain");
		return -EINVAL;
	}

	if (!cfg->differential) {
		LOG_ERR("Only differential mode is supported by hardware.");
		return -EINVAL;
	}

	if (cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid reference");
		return -EINVAL;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid acquisition time");
		return -EINVAL;
	}

	return 0;
}

static int mcp342x_start_read(const struct device *dev, const struct adc_sequence *seq)
{
	struct mcp342x_data *data = dev->data;
	const struct mcp342x_config *config = dev->config;

	const size_t num_extra_samples = seq->options ? seq->options->extra_samplings : 0;
	const size_t num_samples = (1 + num_extra_samples) * POPCOUNT(seq->channels);

	if (find_msb_set(seq->channels) > config->channel_count) {
		LOG_ERR("Selected channel(s) not supported: %x", seq->channels);
		return -EINVAL;
	}

	if (seq->resolution != 12 && seq->resolution != 14 && seq->resolution != 16) {
		LOG_ERR("Selected resolution not supported: %d", seq->resolution);
		return -EINVAL;
	}

	if (seq->oversampling) {
		LOG_ERR("Oversampling is not supported");
		return -EINVAL;
	}

	if (seq->calibrate) {
		LOG_ERR("Calibration is not supported");
		return -EINVAL;
	}

	if (!seq->buffer) {
		LOG_ERR("Buffer invalid");
		return -EINVAL;
	}

	if (seq->buffer_size < (num_samples * sizeof(mcp342x_reg_data_t))) {
		LOG_ERR("buffer size too small");
		return -EINVAL;
	}

	data->buffer = seq->buffer;

	adc_context_start_read(&data->ctx, seq);

	return adc_context_wait_for_completion(&data->ctx);
}

static int mcp342_read_async(const struct device *dev, const struct adc_sequence *seq,
			     struct k_poll_signal *async)
{
	int ret;

	struct mcp342x_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = mcp342x_start_read(dev, seq);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int mcp342x_read(const struct device *dev, const struct adc_sequence *seq)
{
	return mcp342_read_async(dev, seq, NULL);
}

static int mcp342x_get_conversion_time_us(uint8_t resolution)
{
	switch (resolution) {
	case 12:
		return 1000 * 1000 / 240; /* 240 SPS */
	case 14:
		return 1000 * 1000 / 60; /* 60 SPS */
	case 16:
		return 1000 * 1000 / 15; /* 15 SPS */
	default:
		return 0;
	}
	return 0;
}

static void mcp342x_perform_read(const struct device *dev)
{
	struct mcp342x_data *data = dev->data;
	mcp342x_reg_config_t reg;
	mcp342x_reg_data_t res = 0;
	uint8_t ch;
	int ret;

	while (data->channels) {
		/*
		 * Select correct channel
		 */
		ch = find_lsb_set(data->channels) - 1;
		reg = data->reg_config[ch];

		switch (data->resolution) {
		case 12:
			reg.sampleRate = SAMPLERATE_240SPS_12BITS;
			break;
		case 14:
			reg.sampleRate = SAMPLERATE_60SPS_14BITS;
			break;
		case 16:
			reg.sampleRate = SAMPLERATE_15SPS_16BITS;
			break;
		default: /* ignored, checked in mcp342x_start_read */
		}

		LOG_DBG("reg: %x", reg.raw);

		/*
		 * Start single-shot conversion
		 */
		reg.conversionMode = 0;
		reg.notReady = 1;
		ret = mcp342x_write_register(dev, reg);
		if (ret) {
			LOG_WRN("Failed to start conversion");
		}

		/*
		 * Wait until sampling is done
		 */
		k_usleep(mcp342x_get_conversion_time_us(data->resolution));
		do {
			k_yield();
			ret = mcp342x_read_register(dev, &res, &reg);
			if (ret < 0) {
				adc_context_complete(&data->ctx, ret);
			}
		} while (reg.notReady);

		*data->buffer++ = res;

		LOG_DBG("read channel %d, result = %d", ch, res);
		WRITE_BIT(data->channels, ch, 0);
	}

	adc_context_on_sampling_done(&data->ctx, data->dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcp342x_data *data = CONTAINER_OF(ctx, struct mcp342x_data, ctx);

	data->channels = ctx->sequence.channels;
	data->resolution = ctx->sequence.resolution;
	data->repeat_buffer = data->buffer;

#ifdef CONFIG_ADC_ASYNC
	k_sem_give(&data->acq_lock);
#else
	mcp342x_perform_read(data->dev);
#endif
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct mcp342x_data *data = CONTAINER_OF(ctx, struct mcp342x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

#ifdef CONFIG_ADC_ASYNC
static void mcp342x_acq_thread_fn(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	struct mcp342x_data *data = dev->data;

	while (true) {
		k_sem_take(&data->acq_lock, K_FOREVER);

		mcp342x_perform_read(dev);
	}
}
#endif

static int mcp342x_init(const struct device *dev)
{
	int ret;

	const struct mcp342x_config *config = dev->config;
	struct mcp342x_data *data = dev->data;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("Bus not ready");
		return -EINVAL;
	}

	for (uint8_t ch = 0; ch < MAX_CHANNELS; ch++) {
		data->reg_config[ch].raw = 0;
	}

	ret = mcp342x_write_register(dev, data->reg_config[0]);
	if (ret) {
		LOG_ERR("Device reset failed: %d", ret);
		return ret;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, mcp342x_driver_api) = {
	.channel_setup = mcp342x_channel_setup,
	.read = mcp342x_read,
	.ref_internal = 2048,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcp342_read_async,
#endif
};

#define MCP342X_THREAD_INIT(t, n)                                                                  \
	K_THREAD_DEFINE(adc_##t##_##n##_thread, ACQ_THREAD_STACK_SIZE, mcp342x_acq_thread_fn,      \
			DEVICE_DT_INST_GET(n), NULL, NULL, ACQ_THREAD_PRIORITY, 0, 0);

#define MCP342X_INIT(n, t, channels)                                                               \
	IF_ENABLED(CONFIG_ADC_ASYNC, (MCP342X_THREAD_INIT(t, n))) \
	static const struct mcp342x_config inst_##t##_##n##_config = {                             \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		.channel_count = channels,                                                         \
	};                                                                                         \
	static struct mcp342x_data inst_##t##_##n##_data = {                                       \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
		ADC_CONTEXT_INIT_LOCK(inst_##t##_##n##_data, ctx),                                 \
		ADC_CONTEXT_INIT_TIMER(inst_##t##_##n##_data, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(inst_##t##_##n##_data, ctx),                                 \
		IF_ENABLED(CONFIG_ADC_ASYNC, ( \
			.acq_lock = Z_SEM_INITIALIZER(inst_##t##_##n##_data.acq_lock, 0, 1), \
		))};   \
	DEVICE_DT_INST_DEFINE(n, &mcp342x_init, NULL, &inst_##t##_##n##_data,                      \
			      &inst_##t##_##n##_config, POST_KERNEL,                               \
			      CONFIG_ADC_MCP342X_INIT_PRIORITY, &mcp342x_driver_api);

#define DT_DRV_COMPAT        microchip_mcp3428
#define ADC_MCP3428_CHANNELS 4
DT_INST_FOREACH_STATUS_OKAY_VARGS(MCP342X_INIT, mcp3428, ADC_MCP3428_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT        microchip_mcp3427
#define ADC_MCP3427_CHANNELS 2
DT_INST_FOREACH_STATUS_OKAY_VARGS(MCP342X_INIT, mcp3427, ADC_MCP3427_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT        microchip_mcp3426
#define ADC_MCP3426_CHANNELS 2
DT_INST_FOREACH_STATUS_OKAY_VARGS(MCP342X_INIT, mcp3426, ADC_MCP3426_CHANNELS)
#undef DT_DRV_COMPAT

BUILD_ASSERT(CONFIG_I2C_INIT_PRIORITY < CONFIG_ADC_MCP342X_INIT_PRIORITY);
