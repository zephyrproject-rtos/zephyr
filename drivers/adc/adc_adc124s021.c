/*
 * Copyright (c) 2026 RBZ Embedded Logics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the Texas Instruments ADC124S021 SPI ADC.
 */

#define DT_DRV_COMPAT ti_adc124s021

#include <errno.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc124s021, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC124S021_RESOLUTION		12U
#define ADC124S021_NUM_CHANNELS		4U
#define ADC124S021_FRAME_BYTES		2U
#define ADC124S021_CHANNEL_MASK		BIT_MASK(ADC124S021_NUM_CHANNELS)
#define ADC124S021_CHANNEL_SEL_MASK	GENMASK(1, 0)
#define ADC124S021_CHANNEL_SEL_SHIFT	3U

struct adc124s021_config {
	struct spi_dt_spec bus;
};

struct adc124s021_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t configured_channels;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack,
			      CONFIG_ADC_ADC124S021_ACQUISITION_THREAD_STACK_SIZE);
};

static int adc124s021_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	struct adc124s021_data *data = dev->data;

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_EXTERNAL0) {
		LOG_ERR("unsupported channel reference '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'",
			channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->channel_id >= ADC124S021_NUM_CHANNELS) {
		LOG_ERR("unsupported channel id '%u'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential mode is not supported");
		return -ENOTSUP;
	}

	data->configured_channels |= BIT(channel_cfg->channel_id);

	return 0;
}

static int adc124s021_validate_buffer_size(const struct adc_sequence *sequence)
{
	uint8_t channel_count = 0U;
	size_t needed;
	uint32_t mask;

	for (mask = BIT(ADC124S021_NUM_CHANNELS - 1U); mask != 0U; mask >>= 1U) {
		if ((sequence->channels & mask) != 0U) {
			channel_count++;
		}
	}

	needed = channel_count * sizeof(uint16_t);

	if (sequence->options != NULL) {
		needed *= 1U + sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int adc124s021_start_read(const struct device *dev,
				 const struct adc_sequence *sequence)
{
	struct adc124s021_data *data = dev->data;
	int err;

	if (sequence->resolution != ADC124S021_RESOLUTION) {
		LOG_ERR("unsupported resolution %u", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("oversampling is not supported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		LOG_ERR("calibration is not supported");
		return -ENOTSUP;
	}

	if (sequence->buffer == NULL) {
		LOG_ERR("sequence buffer is NULL");
		return -EINVAL;
	}

	if ((sequence->channels == 0U) ||
	    ((sequence->channels & ~ADC124S021_CHANNEL_MASK) != 0U)) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	if ((sequence->channels & ~data->configured_channels) != 0U) {
		LOG_ERR("channel used before adc_channel_setup(): 0x%08x",
			sequence->channels & ~data->configured_channels);
		return -EINVAL;
	}

	err = adc124s021_validate_buffer_size(sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc124s021_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc124s021_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async != NULL, async);
	err = adc124s021_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc124s021_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	return adc124s021_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc124s021_data *data =
		CONTAINER_OF(ctx, struct adc124s021_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc124s021_data *data =
		CONTAINER_OF(ctx, struct adc124s021_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc124s021_transfer(const struct device *dev, uint8_t channel,
			       uint16_t *sample)
{
	const struct adc124s021_config *config = dev->config;
	uint8_t tx_data[ADC124S021_FRAME_BYTES] = {
		(uint8_t)((channel & ADC124S021_CHANNEL_SEL_MASK) <<
			  ADC124S021_CHANNEL_SEL_SHIFT),
		0x00,
	};
	uint8_t rx_data[ADC124S021_FRAME_BYTES] = { 0 };
	const struct spi_buf tx_buf = {
		.buf = tx_data,
		.len = sizeof(tx_data),
	};
	const struct spi_buf rx_buf = {
		.buf = rx_data,
		.len = sizeof(rx_data),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};
	int err;

	err = spi_transceive_dt(&config->bus, &tx, &rx);
	if (err) {
		return err;
	}

	*sample = sys_get_be16(rx_data) & BIT_MASK(ADC124S021_RESOLUTION);

	return 0;
}

static int adc124s021_read_channel(const struct device *dev, uint8_t channel,
				   uint16_t *sample)
{
	uint16_t dummy;
	int err;

	/*
	 * The ADC124S021 channel address sent on DIN during one frame is used
	 * for the next conversion result. Send one frame to select the channel,
	 * then a second frame to receive the selected channel's sample.
	 */
	err = adc124s021_transfer(dev, channel, &dummy);
	if (err) {
		return err;
	}

	return adc124s021_transfer(dev, channel, sample);
}

static void adc124s021_acquisition_thread(void *p1, void *p2, void *p3)
{
	struct adc124s021_data *data = p1;
	uint16_t result = 0U;
	uint8_t channel;
	int err;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		while (data->channels != 0U) {
			channel = find_lsb_set(data->channels) - 1U;

			LOG_DBG("reading channel %u", channel);

			err = adc124s021_read_channel(data->dev, channel, &result);
			if (err) {
				LOG_ERR("failed to read channel %u (err %d)",
					channel, err);
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("read channel %u, result = %u", channel, result);

			*data->buffer++ = result;
			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int adc124s021_init(const struct device *dev)
{
	const struct adc124s021_config *config = dev->config;
	struct adc124s021_data *data = dev->data;
	k_tid_t tid;

	data->dev = dev;

	k_sem_init(&data->sem, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

	tid = k_thread_create(&data->thread, data->stack,
			      K_KERNEL_STACK_SIZEOF(data->stack),
			      adc124s021_acquisition_thread,
			      data, NULL, NULL,
			      CONFIG_ADC_ADC124S021_ACQUISITION_THREAD_PRIO,
			      0, K_NO_WAIT);

	k_thread_name_set(tid, dev->name);

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc124s021_api) = {
	.channel_setup = adc124s021_channel_setup,
	.read = adc124s021_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc124s021_read_async,
#endif
};

#define ADC124S021_DEVICE(inst)						\
	static struct adc124s021_data adc124s021_data_##inst = {	\
		ADC_CONTEXT_INIT_TIMER(adc124s021_data_##inst, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc124s021_data_##inst, ctx),	\
		ADC_CONTEXT_INIT_SYNC(adc124s021_data_##inst, ctx),	\
	};								\
	static const struct adc124s021_config adc124s021_config_##inst = { \
		.bus = SPI_DT_SPEC_INST_GET(inst,			\
					    SPI_OP_MODE_MASTER |	\
					    SPI_TRANSFER_MSB |		\
					    SPI_WORD_SET(8)),	\
	};								\
	DEVICE_DT_INST_DEFINE(inst, adc124s021_init, NULL,		\
			      &adc124s021_data_##inst,			\
			      &adc124s021_config_##inst, POST_KERNEL,	\
			      CONFIG_ADC_INIT_PRIORITY, &adc124s021_api)

DT_INST_FOREACH_STATUS_OKAY(ADC124S021_DEVICE)
