/*
 * Copyright (c) 2026 Pierrick Curt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ADC driver for the MCP3221 ADC.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_mcp3221, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define MCP3221_RESOLUTION 12U
#define MCP3221_DATA_SIZE  2U

struct mcp3221_config {
	struct i2c_dt_spec i2c;
};

struct mcp3221_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	struct k_thread thread;
	struct k_sem sem;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_MCP3221_ACQUISITION_THREAD_STACK_SIZE);
};

static int mcp3221_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct mcp3221_data *data = dev->data;
	size_t needed_size = MCP3221_DATA_SIZE;

	if (sequence->resolution != MCP3221_RESOLUTION) {
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (sequence->channels != BIT(0)) {
		LOG_ERR("Only one channel is supported by the MCP3221");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("Oversampling not supported");
		return -EINVAL;
	}

	if (sequence->options) {
		needed_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_size) {
		LOG_ERR("Buffer size too small for the requested sequence");
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int mcp3221_read_async(const struct device *dev, const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct mcp3221_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = mcp3221_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int mcp3221_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return mcp3221_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct mcp3221_data *data = CONTAINER_OF(ctx, struct mcp3221_data, ctx);

	data->repeat_buffer = data->buffer;

	k_sem_give(&data->sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct mcp3221_data *data = CONTAINER_OF(ctx, struct mcp3221_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int mcp3221_read_channel(const struct device *dev, uint16_t *result)
{
	const struct mcp3221_config *config = dev->config;
	uint8_t raw_data[MCP3221_DATA_SIZE];
	int ret = i2c_read_dt(&config->i2c, raw_data, MCP3221_DATA_SIZE);

	if (ret != 0) {
		LOG_ERR("Error reading mcp3221 on I2C bus");
		return ret;
	}
	*result = (raw_data[0] << 8) | raw_data[1];

	return ret;
}

static int mcp3221_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	if (channel_cfg->channel_id != 0) {
		LOG_ERR("Unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_VDD_1) {
		LOG_ERR("Unsupported channel reference '%d'", channel_cfg->reference);
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported acquisition_time '%d'", channel_cfg->acquisition_time);
		return -ENOTSUP;
	}
	return 0;
}

static DEVICE_API(adc, mcp3221_adc_api) = {
	.channel_setup = mcp3221_channel_setup,
	.read = mcp3221_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = mcp3221_read_async,
#endif
};

static void mcp3221_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct mcp3221_data *data = p1;
	uint16_t result = 0;
	int err;

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);

		err = mcp3221_read_channel(data->dev, &result);
		if (err) {
			LOG_ERR("Failed to read channel %d (err %d)", 0, err);
			adc_context_complete(&data->ctx, err);
			break;
		}
		LOG_DBG("Read channel %d, result = %d", 0, result);
		*data->buffer++ = result;

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int mcp3221_init(const struct device *dev)
{
	const struct mcp3221_config *config = dev->config;
	struct mcp3221_data *data = dev->data;

	data->dev = dev;

	k_sem_init(&data->sem, 0, 1);

	LOG_DBG("Initializing MCP3221 ADC at I2C address 0x%02X", config->i2c.addr);

	if (!device_is_ready(config->i2c.bus)) {
		return -ENODEV;
	}

	k_tid_t tid =
		k_thread_create(&data->thread, data->stack, K_KERNEL_STACK_SIZEOF(data->stack),
				mcp3221_acquisition_thread, data, NULL, NULL,
				CONFIG_ADC_MCP3221_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);

	k_thread_name_set(tid, dev->name);

	adc_context_unlock_unconditionally(&data->ctx);

	LOG_INF("MCP3221 ADC initialized successfully");

	return 0;
}

#define DT_DRV_COMPAT microchip_mcp3221

#define MCP3221_DEVICE(n)                                                                          \
	static struct mcp3221_data mcp3221_data_##n = {                                            \
		ADC_CONTEXT_INIT_TIMER(mcp3221_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_LOCK(mcp3221_data_##n, ctx),                                      \
		ADC_CONTEXT_INIT_SYNC(mcp3221_data_##n, ctx),                                      \
	};                                                                                         \
	static const struct mcp3221_config mcp3221_config_##n = {                                  \
		.i2c = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, &mcp3221_init, NULL, &mcp3221_data_##n, &mcp3221_config_##n,      \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &mcp3221_adc_api)

DT_INST_FOREACH_STATUS_OKAY(MCP3221_DEVICE)
