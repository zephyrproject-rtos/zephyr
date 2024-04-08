/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad559x_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/mfd/ad559x.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_ad559x, CONFIG_ADC_LOG_LEVEL);

#define AD559X_ADC_RD_POINTER_SIZE 1
#define AD559X_ADC_RD_POINTER      0x40

#define AD559X_ADC_RESOLUTION 12U
#define AD559X_ADC_VREF_MV 2500U

struct adc_ad559x_config {
	const struct device *mfd_dev;
};

struct adc_ad559x_data {
	struct adc_context ctx;
	const struct device *dev;
	uint8_t adc_conf;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t channels;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_AD559X_ACQUISITION_THREAD_STACK_SIZE);
};

static int adc_ad559x_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_ad559x_config *config = dev->config;
	struct adc_ad559x_data *data = dev->data;

	if (channel_cfg->channel_id >= AD559X_PIN_MAX) {
		LOG_ERR("invalid channel id %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	data->adc_conf |= BIT(channel_cfg->channel_id);

	return mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_ADC_CONFIG, data->adc_conf);
}

static int adc_ad559x_validate_buffer_size(const struct device *dev,
					   const struct adc_sequence *sequence)
{
	uint8_t channels;
	size_t needed;

	channels = POPCOUNT(sequence->channels);
	needed = channels * sizeof(uint16_t);

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int adc_ad559x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_ad559x_data *data = dev->data;
	int ret;

	if (sequence->resolution != AD559X_ADC_RESOLUTION) {
		LOG_ERR("invalid resolution %d", sequence->resolution);
		return -EINVAL;
	}

	if (find_msb_set(sequence->channels) > AD559X_PIN_MAX) {
		LOG_ERR("invalid channels in mask: 0x%08x", sequence->channels);
		return -EINVAL;
	}

	ret = adc_ad559x_validate_buffer_size(dev, sequence);
	if (ret < 0) {
		LOG_ERR("insufficient buffer size");
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_ad559x_read_channel(const struct device *dev, uint8_t channel, uint16_t *result)
{
	const struct adc_ad559x_config *config = dev->config;
	uint16_t val;
	int ret;

	/* Select channel */
	ret = mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_SEQ_ADC, BIT(channel));
	if (ret < 0) {
		return ret;
	}

	if (mfd_ad559x_has_pointer_byte_map(config->mfd_dev)) {
		/* Start readback */
		val = AD559X_ADC_RD_POINTER;
		ret = mfd_ad559x_write_raw(config->mfd_dev, (uint8_t *)&val,
					   AD559X_ADC_RD_POINTER_SIZE);
		if (ret < 0) {
			return ret;
		}

		/* Read channel */
		ret = mfd_ad559x_read_raw(config->mfd_dev, (uint8_t *)&val, sizeof(val));
		if (ret < 0) {
			return ret;
		}

		*result = sys_get_be16((uint8_t *)&val);

	} else {
		/*
		 * Invalid data:
		 * See Figure 46. Single-Channel ADC Conversion Sequence.
		 * The first conversion result always returns invalid data.
		 */
		(void)mfd_ad559x_read_raw(config->mfd_dev, (uint8_t *)&val, sizeof(val));

		ret = mfd_ad559x_read_raw(config->mfd_dev, (uint8_t *)&val, sizeof(val));
		if (ret < 0) {
			return ret;
		}

		val = sys_be16_to_cpu(val);
		if (channel >= 1) {
			val -= channel * BIT(AD559X_ADC_RESOLUTION);
		}

		*result = val;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ad559x_data *data = CONTAINER_OF(ctx, struct adc_ad559x_data, ctx);

	data->channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ad559x_data *data = CONTAINER_OF(ctx, struct adc_ad559x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_ad559x_acquisition_thread(struct adc_ad559x_data *data)
{
	uint16_t result;
	uint8_t channel;
	int ret;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		while (data->channels != 0) {
			channel = find_lsb_set(data->channels) - 1;

			ret = adc_ad559x_read_channel(data->dev, channel, &result);
			if (ret < 0) {
				LOG_ERR("failed to read channel %d (ret %d)", channel, ret);
				adc_context_complete(&data->ctx, ret);
				break;
			}

			*data->buffer++ = result;
			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int adc_ad559x_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_ad559x_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, async ? true : false, async);
	ret = adc_ad559x_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_ad559x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_ad559x_read_async(dev, sequence, NULL);
}

static int adc_ad559x_init(const struct device *dev)
{
	const struct adc_ad559x_config *config = dev->config;
	struct adc_ad559x_data *data = dev->data;
	k_tid_t tid;
	int ret;

	if (!device_is_ready(config->mfd_dev)) {
		return -ENODEV;
	}

	ret = mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_PD_REF_CTRL, AD559X_EN_REF);
	if (ret < 0) {
		return ret;
	}

	data->dev = dev;

	k_sem_init(&data->sem, 0, 1);
	adc_context_init(&data->ctx);

	tid = k_thread_create(&data->thread, data->stack,
			K_KERNEL_STACK_SIZEOF(data->stack),
			(k_thread_entry_t)adc_ad559x_acquisition_thread, data, NULL, NULL,
			CONFIG_ADC_AD559X_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		ret = k_thread_name_set(tid, "adc_ad559x");
		if (ret < 0) {
			return ret;
		}
	}

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api adc_ad559x_api = {
	.channel_setup = adc_ad559x_channel_setup,
	.read = adc_ad559x_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_ad559x_read_async,
#endif
	.ref_internal = AD559X_ADC_VREF_MV,
};

#define ADC_AD559X_DEFINE(inst)                                                                    \
	static const struct adc_ad559x_config adc_ad559x_config##inst = {                          \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
	};                                                                                         \
                                                                                                   \
	static struct adc_ad559x_data adc_ad559x_data##inst;                                       \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, adc_ad559x_init, NULL, &adc_ad559x_data##inst,                 \
			      &adc_ad559x_config##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,     \
			      &adc_ad559x_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_AD559X_DEFINE)
