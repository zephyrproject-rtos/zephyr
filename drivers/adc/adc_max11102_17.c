/*
 * Copyright (c) 2023 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

LOG_MODULE_REGISTER(max11102_17, CONFIG_ADC_LOG_LEVEL);

struct max11102_17_config {
	struct spi_dt_spec bus;
	const struct gpio_dt_spec gpio_chsel;
	uint8_t resolution;
	uint8_t channel_count;
};

struct max11102_17_data {
	struct adc_context ctx;
	struct k_sem acquire_signal;
	int16_t *buffer;
	int16_t *buffer_ptr;
	uint8_t current_channel_id;
	uint8_t sequence_channel_id;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MAX11102_17_ACQUISITION_THREAD_STACK_SIZE);
#endif /* CONFIG_ADC_ASYNC */
};

static int max11102_17_switch_channel(const struct device *dev)
{
	const struct max11102_17_config *config = dev->config;
	struct max11102_17_data *data = dev->data;
	int result;
	uint8_t buffer_rx[1];
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};
	struct spi_dt_spec bus;

	memcpy(&bus, &config->bus, sizeof(bus));
	bus.config.operation |= SPI_HOLD_ON_CS;

	result = spi_read_dt(&bus, &rx);
	if (result != 0) {
		LOG_ERR("read failed with error %i", result);
		return result;
	}

	gpio_pin_set_dt(&config->gpio_chsel, data->current_channel_id);

	result = spi_read_dt(&config->bus, &rx);
	if (result != 0) {
		LOG_ERR("read failed with error %i", result);
		return result;
	}

	return 0;
}

static int max11102_17_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct max11102_17_config *config = dev->config;

	LOG_DBG("read from ADC channel %i", channel_cfg->channel_id);

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("invalid reference %i", channel_cfg->reference);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("invalid gain %i", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("invalid acquisition time %i", channel_cfg->acquisition_time);
		return -EINVAL;
	}

	if (channel_cfg->differential != 0) {
		LOG_ERR("differential inputs are not supported");
		return -EINVAL;
	}

	if (channel_cfg->channel_id > config->channel_count) {
		LOG_ERR("invalid channel selection %i", channel_cfg->channel_id);
		return -EINVAL;
	}

	return 0;
}

static int max11102_17_validate_buffer_size(const struct adc_sequence *sequence)
{
	size_t necessary = sizeof(int16_t);

	if (sequence->options) {
		necessary *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < necessary) {
		return -ENOMEM;
	}

	return 0;
}

static int max11102_17_validate_sequence(const struct device *dev,
					 const struct adc_sequence *sequence)
{
	const struct max11102_17_config *config = dev->config;
	struct max11102_17_data *data = dev->data;
	size_t sequence_channel_count = 0;
	const size_t channel_maximum = 8*sizeof(sequence->channels);

	if (sequence->resolution != config->resolution) {
		LOG_ERR("invalid resolution");
		return -EINVAL;
	}

	for (size_t i = 0; i < channel_maximum; ++i) {
		if ((BIT(i) & sequence->channels) == 0) {
			continue;
		}

		if (i > config->channel_count) {
			LOG_ERR("invalid channel selection");
			return -EINVAL;
		}

		sequence_channel_count++;
		data->sequence_channel_id = i;
	}

	if (sequence_channel_count == 0) {
		LOG_ERR("no channel selected");
		return -EINVAL;
	}

	if (sequence_channel_count > 1) {
		LOG_ERR("multiple channels selected");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling is not supported");
		return -EINVAL;
	}

	return max11102_17_validate_buffer_size(sequence);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct max11102_17_data *data = CONTAINER_OF(ctx, struct max11102_17_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->buffer_ptr;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct max11102_17_data *data = CONTAINER_OF(ctx, struct max11102_17_data, ctx);

	data->buffer_ptr = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static int max11102_17_adc_start_read(const struct device *dev, const struct adc_sequence *sequence,
				      bool wait)
{
	int result;
	struct max11102_17_data *data = dev->data;

	result = max11102_17_validate_sequence(dev, sequence);

	if (result != 0) {
		LOG_ERR("sequence validation failed");
		return result;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	if (wait) {
		result = adc_context_wait_for_completion(&data->ctx);
	}

	return result;
}

static int max11102_17_read_sample(const struct device *dev, int16_t *sample)
{
	const struct max11102_17_config *config = dev->config;
	int result;
	size_t trailing_bits = 15 - config->resolution;
	uint8_t buffer_rx[2];
	const struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ARRAY_SIZE(buffer_rx),
	}};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(rx_buf),
	};

	result = spi_read_dt(&config->bus, &rx);

	if (result != 0) {
		LOG_ERR("read failed with error %i", result);
		return result;
	}

	*sample = sys_get_be16(buffer_rx);
	LOG_DBG("raw sample: 0x%04X", *sample);

	*sample = *sample >> trailing_bits;
	*sample = *sample & GENMASK(config->resolution, 0);
	LOG_DBG("sample: 0x%04X", *sample);

	return 0;
}

static int max11102_17_adc_perform_read(const struct device *dev)
{
	int result;
	struct max11102_17_data *data = dev->data;

	k_sem_take(&data->acquire_signal, K_FOREVER);

	if (data->sequence_channel_id != data->current_channel_id) {
		LOG_DBG("switch channel selection");
		data->current_channel_id = data->sequence_channel_id;
		max11102_17_switch_channel(dev);
	}

	result = max11102_17_read_sample(dev, data->buffer);
	if (result != 0) {
		LOG_ERR("reading sample failed");
		adc_context_complete(&data->ctx, result);
		return result;
	}

	data->buffer++;

	adc_context_on_sampling_done(&data->ctx, dev);

	return result;
}

#if CONFIG_ADC_ASYNC
static int max11102_17_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				      struct k_poll_signal *async)
{
	int result;
	struct max11102_17_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	result = max11102_17_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, result);

	return result;
}

static int max11102_17_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int result;
	struct max11102_17_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	result = max11102_17_adc_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, result);

	return result;
}

#else
static int max11102_17_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int result;
	struct max11102_17_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	result = max11102_17_adc_start_read(dev, sequence, false);

	while (result == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		result = max11102_17_adc_perform_read(dev);
	}

	adc_context_release(&data->ctx, result);
	return result;
}
#endif

#if CONFIG_ADC_ASYNC
static void max11102_17_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	while (true) {
		max11102_17_adc_perform_read(dev);
	}
}
#endif

static int max11102_17_init(const struct device *dev)
{
	int result;
	const struct max11102_17_config *config = dev->config;
	struct max11102_17_data *data = dev->data;
	int16_t sample;

	adc_context_init(&data->ctx);

	k_sem_init(&data->acquire_signal, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	switch (config->channel_count) {
	case 1:
		if (config->gpio_chsel.port != NULL) {
			LOG_ERR("GPIO for chsel set with only one channel");
			return -EINVAL;
		}
		break;
	case 2:
		if (config->gpio_chsel.port == NULL) {
			LOG_ERR("no GPIO for chsel set with two channels");
			return -EINVAL;
		}

		result = gpio_pin_configure_dt(&config->gpio_chsel, GPIO_OUTPUT_INACTIVE);
		if (result != 0) {
			LOG_ERR("failed to initialize GPIO for chsel");
			return result;
		}
		break;
	default:
		LOG_ERR("invalid number of channels (%i)", config->channel_count);
		return -EINVAL;
	}

	data->current_channel_id = 0;

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(
		&data->thread, data->stack, CONFIG_ADC_MAX11102_17_ACQUISITION_THREAD_STACK_SIZE,
		max11102_17_acquisition_thread, (void *)dev, NULL, NULL,
		CONFIG_ADC_MAX11102_17_ACQUISITION_THREAD_INIT_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_max11102_17");
#endif

	/* power up time is one conversion cycle */
	result = max11102_17_read_sample(dev, &sample);
	if (result != 0) {
		LOG_ERR("unable to read dummy sample for power up timing");
		return result;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return result;
}

static DEVICE_API(adc, api) = {
	.channel_setup = max11102_17_channel_setup,
	.read = max11102_17_read,
	.ref_internal = 0,
#ifdef CONFIG_ADC_ASYNC
	.read_async = max11102_17_adc_read_async,
#endif
};

BUILD_ASSERT(CONFIG_ADC_INIT_PRIORITY > CONFIG_SPI_INIT_PRIORITY,
	     "CONFIG_ADC_INIT_PRIORITY must be higher than CONFIG_SPI_INIT_PRIORITY");

#define ADC_MAX11102_17_INST_DEFINE(index, name, res, channels)                                    \
	static const struct max11102_17_config config_##name##_##index = {                         \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			index,                                                                     \
			SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),  \
		.gpio_chsel = GPIO_DT_SPEC_INST_GET_OR(index, chsel_gpios, {0}),                   \
		.resolution = res,                                                                 \
		.channel_count = channels,                                                         \
	};                                                                                         \
	static struct max11102_17_data data_##name##_##index;                                      \
	DEVICE_DT_INST_DEFINE(index, max11102_17_init, NULL, &data_##name##_##index,               \
			      &config_##name##_##index, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,     \
			      &api);

#define DT_DRV_COMPAT           maxim_max11102
#define ADC_MAX11102_RESOLUTION 12
#define ADC_MAX11102_CHANNELS   2
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11102_RESOLUTION, ADC_MAX11102_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11103
#define ADC_MAX11103_RESOLUTION 12
#define ADC_MAX11103_CHANNELS   2
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11103_RESOLUTION, ADC_MAX11103_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11105
#define ADC_MAX11105_RESOLUTION 12
#define ADC_MAX11105_CHANNELS   1
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11105_RESOLUTION, ADC_MAX11105_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11106
#define ADC_MAX11106_RESOLUTION 10
#define ADC_MAX11106_CHANNELS   2
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11106_RESOLUTION, ADC_MAX11106_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11110
#define ADC_MAX11110_RESOLUTION 10
#define ADC_MAX11110_CHANNELS   1
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11110_RESOLUTION, ADC_MAX11110_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11111
#define ADC_MAX11111_RESOLUTION 8
#define ADC_MAX11111_CHANNELS   2
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11111_RESOLUTION, ADC_MAX11111_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11115
#define ADC_MAX11115_RESOLUTION 8
#define ADC_MAX11115_CHANNELS   1
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11115_RESOLUTION, ADC_MAX11115_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11116
#define ADC_MAX11116_RESOLUTION 8
#define ADC_MAX11116_CHANNELS   1
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11116_RESOLUTION, ADC_MAX11116_CHANNELS)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT           maxim_max11117
#define ADC_MAX11117_RESOLUTION 10
#define ADC_MAX11117_CHANNELS   1
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_MAX11102_17_INST_DEFINE, DT_DRV_COMPAT,
				  ADC_MAX11117_RESOLUTION, ADC_MAX11117_CHANNELS)
#undef DT_DRV_COMPAT
