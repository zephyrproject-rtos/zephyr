/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rz_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include "r_adc_c.h"

LOG_MODULE_REGISTER(adc_renesas_rz, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_RZ_MAX_RESOLUTION 12

void adc_c_scan_end_isr(void);

/**
 * @brief RZ ADC config
 *
 * This structure contains constant config data for given instance of RZ ADC.
 */
struct adc_rz_config {
	/** Mask for channels existed in each board */
	uint32_t channel_available_mask;
	/** Structure that handle FSP API */
	const adc_api_t *fsp_api;
};

/**
 * @brief RZ ADC data
 *
 * This structure contains data structures used by a RZ ADC.
 */
struct adc_rz_data {
	/** Structure that handle state of ongoing read operation */
	struct adc_context ctx;
	/** Pointer to RZ ADC own device structure */
	const struct device *dev;
	/** Structure that handle fsp ADC */
	adc_c_instance_ctrl_t fsp_ctrl;
	/** Structure that handle fsp ADC config */
	struct st_adc_cfg fsp_cfg;
	/** Structure that handle fsp ADC channel config */
	adc_c_channel_cfg_t fsp_channel_cfg;
	/** Pointer to memory where next sample will be written */
	uint16_t *buf;
	/** Mask with channels that will be sampled */
	uint32_t channels;
	/** Buffer id */
	uint16_t buf_id;
};

/**
 * @brief Setup channels before starting to scan ADC
 *
 * @param dev RZ ADC device
 * @param channel_cfg channel configuration (user-defined)
 *
 * @return 0 on success
 * @return -ENOTSUP if channel id or differential is wrong value
 * @return -EINVAL if channel configuration is invalid
 */
static int adc_rz_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg)
{

	fsp_err_t fsp_err = FSP_SUCCESS;
	struct adc_rz_data *data = dev->data;
	const struct adc_rz_config *config = dev->config;

	if (!((config->channel_available_mask & BIT(channel_cfg->channel_id)) != 0)) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference");
		return -ENOTSUP;
	}
	data->fsp_channel_cfg.scan_mask |= (1U << channel_cfg->channel_id);
	/** Enable channels. */
	config->fsp_api->scanCfg(&data->fsp_ctrl, &data->fsp_channel_cfg);

	if (FSP_SUCCESS != fsp_err) {
		return -ENOTSUP;
	}

	return 0;
}

/**
 * Interrupt handler
 */
static void adc_rz_isr(const struct device *dev)
{
	struct adc_rz_data *data = dev->data;
	const struct adc_rz_config *config = dev->config;
	fsp_err_t fsp_err = FSP_SUCCESS;
	adc_channel_t channel_id = 0;
	uint32_t channels = 0;
	int16_t *sample_buffer = (int16_t *)data->buf;

	channels = data->channels;
	for (channel_id = 0; channels > 0; channel_id++) {
		/** Get channel ids from scan mask "channels"  */
		if ((channels & 0x01) != 0) {
			/** Read converted data */
			config->fsp_api->read(&data->fsp_ctrl, channel_id,
					      &sample_buffer[data->buf_id]);
			if (FSP_SUCCESS != fsp_err) {
				break;
			}
			data->buf_id = data->buf_id + 1;
		}
		channels = channels >> 1;
	}
	adc_c_scan_end_isr();
	adc_context_on_sampling_done(&data->ctx, dev);
}

/**
 * @brief Check if buffer in @p sequence is big enough to hold all ADC samples
 *
 * @param dev RZ ADC device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOMEM if buffer is not big enough
 */
static int adc_rz_check_buffer_size(const struct device *dev, const struct adc_sequence *sequence)
{
	uint8_t channels = 0;
	size_t needed;

	channels = POPCOUNT(sequence->channels);
	needed = channels * sizeof(uint16_t);

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief Start processing read request
 *
 * @param dev RZ ADC device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOTSUP if requested resolution or channel is out side of supported
 *         range
 * @return -ENOMEM if buffer is not big enough
 *         (see @ref adc_rz_check_buffer_size)
 * @return other error code returned by adc_context_wait_for_completion
 */
static int adc_rz_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_rz_config *config = dev->config;
	struct adc_rz_data *data = dev->data;
	int err;

	if (sequence->resolution > ADC_RZ_MAX_RESOLUTION || sequence->resolution == 0) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if ((sequence->channels & ~config->channel_available_mask) != 0) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}
	err = adc_rz_check_buffer_size(dev, sequence);

	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}
	data->buf_id = 0;
	data->buf = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	adc_context_wait_for_completion(&data->ctx);

	return 0;
}

/**
 * @brief Start processing read request asynchronously.
 *
 * @param dev RZ ADC device
 * @param sequence ADC sequence description
 * @param async async pointer to asynchronous signal
 *
 * @return 0 on success
 * @return -ENOTSUP if requested resolution or channel is out side of supported
 *         range
 * @return -ENOMEM if buffer is not big enough
 *         (see @ref adc_rz_check_buffer_size)
 * @return other error code returned by adc_context_wait_for_completion
 */
static int adc_rz_read_async(const struct device *dev, const struct adc_sequence *sequence,
			     struct k_poll_signal *async)
{
	struct adc_rz_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_rz_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

/**
 * @brief Start processing read request synchronously.
 *
 * @param dev RZ ADC device
 * @param sequence ADC sequence description
 *
 * @return 0 on success
 * @return -ENOTSUP if requested resolution or channel is out side of supported
 *         range
 * @return -ENOMEM if buffer is not big enough
 *         (see @ref adc_rz_check_buffer_size)
 * @return other error code returned by adc_context_wait_for_completion
 */
static int adc_rz_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_rz_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_rz_data *data = CONTAINER_OF(ctx, struct adc_rz_data, ctx);
	const struct device *dev = data->dev;
	const struct adc_rz_config *config = dev->config;

	data->channels = ctx->sequence.channels;
	/** Start a scan */
	config->fsp_api->scanStart(&data->fsp_ctrl);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_rz_data *data = CONTAINER_OF(ctx, struct adc_rz_data, ctx);

	if (repeat_sampling) {
		data->buf_id = 0;
	}
}

/**
 * @brief Function called on init for each RZ ADC device. It setups all
 *        channels.
 *
 * @param dev RZ ADC device
 *
 * @return -EIO if error
 *
 * @return 0 on success
 */
static int adc_rz_init(const struct device *dev)
{
	const struct adc_rz_config *config = dev->config;
	struct adc_rz_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	/**Open the ADC module */
	config->fsp_api->open(&data->fsp_ctrl, &data->fsp_cfg);

	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}
	/** Release context unconditionally */
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/**
 * ************************* DRIVER REGISTER SECTION ***************************
 */

#define ADC_RZG_IRQ_CONNECT(idx, irq_name, isr)                                                    \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, irq_name, irq),                               \
			    DT_INST_IRQ_BY_NAME(idx, irq_name, priority), isr,                     \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, irq_name, irq));                               \
	} while (0)

#define ADC_RZG_CONFIG_FUNC(idx) ADC_RZG_IRQ_CONNECT(idx, scanend, adc_rz_isr);

#define ADC_RZG_INIT(idx)                                                                          \
	static const adc_c_extended_cfg_t g_adc##idx##_cfg_extend = {                              \
		.trigger_mode = ADC_C_TRIGGER_MODE_SOFTWARE,                                       \
		.trigger_source = ADC_C_ACTIVE_TRIGGER_EXTERNAL,                                   \
		.trigger_edge = ADC_C_TRIGGER_EDGE_FALLING,                                        \
		.input_mode = ADC_C_INPUT_MODE_AUTO,                                               \
		.operating_mode = ADC_C_OPERATING_MODE_SCAN,                                       \
		.buffer_mode = ADC_C_BUFFER_MODE_1,                                                \
		.sampling_time = 100,                                                              \
		.external_trigger_filter = ADC_C_FILTER_STAGE_SETTING_DISABLE,                     \
	};                                                                                         \
	static DEVICE_API(adc, adc_rz_api_##idx) = {                                               \
		.channel_setup = adc_rz_channel_setup,                                             \
		.read = adc_rz_read,                                                               \
		.ref_internal = DT_INST_PROP(idx, vref_mv),                                        \
		IF_ENABLED(CONFIG_ADC_ASYNC,                                                       \
					  (.read_async = adc_rz_read_async))};                     \
	static const struct adc_rz_config adc_rz_config_##idx = {                                  \
		.channel_available_mask = DT_INST_PROP(idx, channel_available_mask),               \
		.fsp_api = &g_adc_on_adc,                                                          \
	};                                                                                         \
	static struct adc_rz_data adc_rz_data_##idx = {                                            \
		ADC_CONTEXT_INIT_TIMER(adc_rz_data_##idx, ctx),                                    \
		ADC_CONTEXT_INIT_LOCK(adc_rz_data_##idx, ctx),                                     \
		ADC_CONTEXT_INIT_SYNC(adc_rz_data_##idx, ctx),                                     \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.mode = ADC_MODE_SINGLE_SCAN,                                      \
				.p_callback = NULL,                                                \
				.p_context = NULL,                                                 \
				.p_extend = &g_adc##idx##_cfg_extend,                              \
				.scan_end_irq = DT_INST_IRQ_BY_NAME(idx, scanend, irq),            \
				.scan_end_ipl = DT_INST_IRQ_BY_NAME(idx, scanend, priority),       \
			},                                                                         \
		.fsp_channel_cfg =                                                                 \
			{                                                                          \
				.scan_mask = 0,                                                    \
				.interrupt_setting = ADC_C_INTERRUPT_CHANNEL_SETTING_ENABLE,       \
			},                                                                         \
	};                                                                                         \
	static int adc_rz_init_##idx(const struct device *dev)                                     \
	{                                                                                          \
		ADC_RZG_CONFIG_FUNC(idx)                                                           \
		return adc_rz_init(dev);                                                           \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, adc_rz_init_##idx, NULL, &adc_rz_data_##idx,                    \
			      &adc_rz_config_##idx, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,         \
			      &adc_rz_api_##idx)

DT_INST_FOREACH_STATUS_OKAY(ADC_RZG_INIT);
