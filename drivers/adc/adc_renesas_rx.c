/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>
#include "r_s12ad_rx_if.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_rx, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_RX_MAX_RESOLUTION  12
#define CHANNELS_OVER_8_OFFSET 8

typedef struct {
	adc_mode_t mode;
	void (*p_callback)(void *p_args);
} adc_instance_t;

/**
 * @brief RX ADC config
 *
 * This structure contains constant data for given instance of RX ADC.
 */
struct adc_rx_config {
	/** Number of supported channels */
	uint8_t num_channels;
	/** Mask for channels existed in each unit S12AD */
	uint32_t channel_available_mask;
	/** pinctrl configs */
	const struct pinctrl_dev_config *pcfg;
	/** function pointer to irq setup */
	void (*irq_configure)(void);
};

/**
 * @brief RX ADC data
 *
 * This structure contains data structures used by a RX ADC.
 */
struct adc_rx_data {
	/** Structure that handle state of ongoing read operation */
	struct adc_context ctx;
	/** Index of the device in devicetree */
	uint8_t unit_id;
	/** ADC registers struct */
	volatile struct st_s12ad *p_regs;
	/** Pointer to RX ADC own device structure */
	const struct device *dev;
	/** Struct that stores ADC status and callback */
	adc_instance_t adc;
	/** Structure that handle rdp adc config */
	adc_cfg_t adc_config;
	/** Structure that store adc channel config */
	adc_ch_cfg_t adc_chnl_cfg;
	/** Pointer to memory where next sample will be written */
	uint16_t *buf;
	/** Buffer id */
	uint16_t buf_id;
	/** Mask of channels that have been configured through setup API */
	uint32_t configured_channels;
	/** Mask of channels that will be sampled */
	uint32_t channels;
};

static void adc_rx_scanend_isr(const struct device *dev)
{
	struct adc_rx_data *data = dev->data;
	adc_cb_evt_t event = ADC_EVT_SCAN_COMPLETE;

	/* presence of callback function verified in Open() */
	if (data->adc.p_callback != NULL) {
		data->adc.p_callback(&event);
	}
}

static int adc_rx_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg)
{
	adc_err_t err;
	struct adc_rx_data *data = dev->data;
	const struct adc_rx_config *config = dev->config;

	if (!((config->channel_available_mask & (1 << channel_cfg->channel_id)) != 0)) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential mode is not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid");
		return -EINVAL;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_INTERNAL:
		data->p_regs->ADHVREFCNT.BIT.HVSEL = 0;
		break;
	case ADC_REF_EXTERNAL0:
		data->p_regs->ADHVREFCNT.BIT.HVSEL = 1;
		break;
	default:
		LOG_ERR("Invalid reference. (valid: INTERNAL, EXTERNAL0)");
		return -EINVAL;
	}

	if (channel_cfg->channel_id < 8) {
		data->configured_channels |= (1U << channel_cfg->channel_id);
	} else {
		data->configured_channels |=
			(1U << (channel_cfg->channel_id - CHANNELS_OVER_8_OFFSET));
	}

	data->adc_chnl_cfg.chan_mask |= (1U << channel_cfg->channel_id);
	data->adc_chnl_cfg.add_mask |= (1U << channel_cfg->channel_id);
	err = R_ADC_Control(data->unit_id, ADC_CMD_ENABLE_CHANS, (void *)&data->adc_chnl_cfg);
	if (err != ADC_SUCCESS) {
		return -EINVAL;
	}

	return 0;
}

static void adc_rx_isr(const struct device *dev)
{
	struct adc_rx_data *data = dev->data;
	adc_err_t err;
	adc_reg_t channel_id = 0;
	uint32_t channels = 0;
	int16_t *sample_buffer = (int16_t *)data->buf;

	channels = data->channels;
	for (channel_id = 0; channels > 0; channel_id++) {
		/* Check if it is right channel id */
		if ((channels & 0x01) != 0) {
			err = R_ADC_Read(data->unit_id, channel_id, &sample_buffer[data->buf_id]);
			if (err != ADC_SUCCESS) {
				break;
			}
			data->buf_id = data->buf_id + 1;
		}

		channels = channels >> 1;
	}
	adc_rx_scanend_isr(dev);
	adc_context_on_sampling_done(&data->ctx, dev);
}

static int adc_rx_check_buffer_size(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_rx_config *config = dev->config;
	uint8_t channels = 0;
	size_t needed;

	for (uint32_t mask = BIT(config->num_channels - 1); mask != 0; mask >>= 1) {
		if (mask & sequence->channels) {
			channels++;
		}
	}

	needed = channels * sizeof(uint16_t);
	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

static int validate_read_channels(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_rx_data *data = dev->data;

	uint32_t read_channels = sequence->channels;
	uint32_t configured_channels = data->configured_channels;

	while (read_channels > 0) {
		if (((read_channels & 0x1) == 1) && ((configured_channels & 0x1) == 0)) {
			return -EINVAL;
		}

		read_channels = read_channels >> 1;
		configured_channels = configured_channels >> 1;
	}

	return 0;
}

static int adc_rx_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_rx_config *config = dev->config;
	struct adc_rx_data *data = dev->data;
	int err;

	if (sequence->channels == 0) {
		LOG_ERR("No channel to read");
		return -EINVAL;
	}

	if (sequence->resolution > ADC_RX_MAX_RESOLUTION || sequence->resolution == 0) {
		LOG_ERR("Unsupported resolution %d", sequence->resolution);
		return -EINVAL;
	}

	if (find_msb_set(sequence->channels) > config->num_channels) {
		LOG_ERR("Unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	err = validate_read_channels(dev, sequence);
	if (err) {
		LOG_ERR("One or more channels are not setup");
		return err;
	}

	err = adc_rx_check_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("Buffer size too small");
		return err;
	}

	/** Sample times = 2^oversampling */
	switch (sequence->oversampling) {
	case 0:
		data->p_regs->ADADC.BIT.ADC = 0x0;
		break;
	case 1:
		data->p_regs->ADADC.BIT.ADC = 0x1;
		break;
	case 2:
		data->p_regs->ADADC.BIT.ADC = 0x3;
		break;
	case 4:
		data->p_regs->ADADC.BIT.ADC = 0x5;
		break;
	default:
		LOG_ERR("Invalid oversampling time (valid value: 0, 1, 2, 4)");
		return -EINVAL;
	}

	/** Select AVERAGE for addtion/average mode */
	data->p_regs->ADADC.BIT.AVEE = 1;

	data->buf_id = 0;
	data->buf = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	adc_context_wait_for_completion(&data->ctx);

	return 0;
}

static int adc_rx_read_async(const struct device *dev, const struct adc_sequence *sequence,
			     struct k_poll_signal *async)
{
	struct adc_rx_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_rx_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc_rx_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_rx_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_rx_data *data = CONTAINER_OF(ctx, struct adc_rx_data, ctx);

	data->channels = ctx->sequence.channels;

	R_ADC_Control(data->unit_id, ADC_CMD_SCAN_NOW, NULL);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_rx_data *data = CONTAINER_OF(ctx, struct adc_rx_data, ctx);

	if (repeat_sampling) {
		data->buf_id = 0;
	}
}

static int adc_rx_init(const struct device *dev)
{
	const struct adc_rx_config *config = dev->config;
	struct adc_rx_data *data = dev->data;
	adc_err_t err;
	int ret = 0;

	/* Set pinctrl */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("ADC: Failed to init pinctrl");
		return ret;
	}

	err = R_ADC_Open(data->unit_id, ADC_MODE_SS_MULTI_CH, &data->adc_config, NULL);
	if (err != ADC_SUCCESS) {
		LOG_ERR("ADC: Failed to open module");
		return -EIO;
	}

	data->adc.mode = ADC_MODE_SS_MULTI_CH;
	data->adc.p_callback = NULL;

	/* Config IRQ */
	config->irq_configure();

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

#define IRQ_CONFIGURE_FUNC(idx)                                                                    \
	static void adc_rx_configure_func_##idx(void)                                              \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, s12adi0, irq),                                \
			    DT_INST_IRQ_BY_NAME(idx, s12adi0, priority), adc_rx_isr,               \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, s12adi0, irq));                                \
	}

#define IRQ_CONFIGURE_DEFINE(idx) .irq_configure = adc_rx_configure_func_##idx

#ifdef CONFIG_ADC_ASYNC
#define ASSIGN_READ_ASYNC .read_async = adc_rx_read_async
#else
#define ASSIGN_READ_ASYNC
#endif

#define ADC_RX_INIT(idx)                                                                           \
	IRQ_CONFIGURE_FUNC(idx)                                                                    \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
                                                                                                   \
	static DEVICE_API(adc,                                                                     \
			  adc_driver_rx_api_##idx) = {.channel_setup = adc_rx_channel_setup,       \
						      .read = adc_rx_read,                         \
						      .ref_internal = DT_INST_PROP(idx, vref_mv),  \
						      ASSIGN_READ_ASYNC};                          \
                                                                                                   \
	static const struct adc_rx_config adc_rx_config_##idx = {                                  \
		.num_channels = DT_INST_PROP(idx, channel_count),                                  \
		.channel_available_mask = DT_INST_PROP(idx, channel_available_mask),               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		IRQ_CONFIGURE_DEFINE(idx),                                                         \
	};                                                                                         \
                                                                                                   \
	static struct adc_rx_data adc_rx_data_##idx = {                                            \
		ADC_CONTEXT_INIT_TIMER(adc_rx_data_##idx, ctx),                                    \
		ADC_CONTEXT_INIT_LOCK(adc_rx_data_##idx, ctx),                                     \
		ADC_CONTEXT_INIT_SYNC(adc_rx_data_##idx, ctx),                                     \
		.unit_id = idx,                                                                    \
		.p_regs = (struct st_s12ad *)DT_INST_REG_ADDR(idx),                                \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.adc_config =                                                                      \
			{                                                                          \
				.conv_speed = ADC_CONVERT_SPEED_DEFAULT,                           \
				.alignment = ADC_ALIGN_RIGHT,                                      \
				.add_cnt = ADC_ADD_OFF,                                            \
				.clearing = ADC_CLEAR_AFTER_READ_OFF,                              \
				.trigger = ADC_TRIG_NONE,                                          \
				.trigger_groupb = ADC_TRIG_NONE,                                   \
				.priority = 0,                                                     \
				.priority_groupb = 0,                                              \
			},                                                                         \
		.adc_chnl_cfg =                                                                    \
			{                                                                          \
				.add_mask = 0,                                                     \
				.chan_mask = 0,                                                    \
				.chan_mask_groupb = 0,                                             \
				.diag_method = ADC_DIAG_OFF,                                       \
				.priority_groupa = 0,                                              \
			},                                                                         \
		.configured_channels = 0,                                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, adc_rx_init, NULL, &adc_rx_data_##idx, &adc_rx_config_##idx,    \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_driver_rx_api_##idx)

DT_INST_FOREACH_STATUS_OKAY(ADC_RX_INIT);
