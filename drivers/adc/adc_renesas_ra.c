/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_internal.h>
#include <instances/r_adc.h>

#include "rp_adc.h"

#include <zephyr/irq.h>

LOG_MODULE_REGISTER(adc_ra, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_AVERAGE_1  ADC_ADD_OFF
#define ADC_AVERAGE_2  ADC_ADD_AVERAGE_TWO
#define ADC_AVERAGE_4  ADC_ADD_AVERAGE_FOUR
#define ADC_AVERAGE_8  ADC_ADD_AVERAGE_EIGHT
#define ADC_AVERAGE_16 ADC_ADD_AVERAGE_SIXTEEN

#define ADC_VARIANT_ADC12 12
#define ADC_VARIANT_ADC16 16

#define ADC_CHANNEL_BIT_MASK (0x01U)

enum ra_adc_reference {
	RA_ADC_REF_VDD,
	RA_ADC_REF_INTERNAL,
	RA_ADC_REF_EXTERNAL,
};

extern void adc_scan_end_isr(void);

/**
 * @brief RA ADC config
 *
 * This structure contains constant data for given instance of RA ADC.
 */
struct adc_ra_config {
	/** Mask for channels existed in each board */
	uint32_t channel_available_mask;
	/** pinctrl configs */
	const struct pinctrl_dev_config *pcfg;
	/** Variant support ADC16 or ADC12 */
	uint8_t variant;
	/** Mapping reference voltage */
	uint32_t reference;
	/** Resolution support */
	uint8_t resolution;
	/** Sampling time in nanoseconds */
	uint32_t sampling_time_ns;
	/** function pointer to irq setup */
	void (*irq_configure)(void);
};

/**
 * @brief RA ADC data
 *
 * This structure contains data structures used by a RA ADC.
 */
struct adc_ra_data {
	/** Structure that handle state of ongoing read operation */
	struct adc_context ctx;
	/** Pointer to RA ADC own device structure */
	const struct device *dev;
	/** Structure that handle fsp ADC */
	adc_instance_ctrl_t adc;
	/** Structure that handle fsp ADC config */
	struct st_adc_cfg f_config;
	/** Structure that handle fsp ADC channel config */
	adc_channel_cfg_t f_channel_cfg;
	/** Pointer to memory where next sample will be written */
	uint16_t *buf;
	/** Mask with channels that will be sampled */
	uint32_t channels;
	/** Buffer id */
	uint16_t buf_id;
	/** Calibration process semaphore */
	struct k_sem calibrate_sem;
};

static adc_sample_state_reg_t map_channel_to_sample_state_reg(uint8_t channel_id)
{
	if (channel_id <= 15) {
		return (adc_sample_state_reg_t)channel_id;
	}

	/* Channel IDs 16–31 share the same sample state register */
	return ADC_SAMPLE_STATE_CHANNEL_16_TO_31;
}

static int adc_ra_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel_cfg)
{
	fsp_err_t fsp_err = FSP_SUCCESS;
	struct adc_ra_data *data = dev->data;
	const struct adc_ra_config *config = dev->config;
	adc_sample_state_t sample_state;
	uint32_t sample_states = 0;

	if (!((config->channel_available_mask & (1 << channel_cfg->channel_id)) != 0)) {
		LOG_ERR("unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Acquisition time is not valid");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("unsupported differential mode");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain is not valid");
		return -EINVAL;
	}

	fsp_err = RP_ADC_SampleStateCalculation(config->sampling_time_ns, &sample_states);
	if (FSP_SUCCESS != fsp_err) {
		return -ENOTSUP;
	}

	sample_state.reg_id = map_channel_to_sample_state_reg(channel_cfg->channel_id);
	sample_state.num_states = sample_states;
	fsp_err = R_ADC_SampleStateCountSet(&data->adc, &sample_state);
	if (FSP_SUCCESS != fsp_err) {
		return -ENOTSUP;
	}

	data->f_channel_cfg.scan_mask |= (1U << channel_cfg->channel_id);
	/* Configure ADC channel specific settings */
	fsp_err = R_ADC_ScanCfg(&data->adc, &data->f_channel_cfg);
	if (FSP_SUCCESS != fsp_err) {
		return -ENOTSUP;
	}

	return 0;
}

static void renesas_ra_adc_callback(adc_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	struct adc_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	adc_channel_t channel_id = 0;
	uint32_t channels = 0;
	int16_t *sample_buffer = (int16_t *)data->buf;

	if (p_args->event == ADC_EVENT_SCAN_COMPLETE) {
		channels = data->channels;
		for (channel_id = 0; channels > 0; channel_id++) {
			/* Check if it is right channel id */
			if ((channels & ADC_CHANNEL_BIT_MASK) != 0) {
				fsp_err = R_ADC_Read(&data->adc, channel_id,
						     &sample_buffer[data->buf_id]);
				if (FSP_SUCCESS != fsp_err) {
					break;
				}
				/* Do not return negative value for single-ended configuration */
				if (sample_buffer[data->buf_id] < 0) {
					sample_buffer[data->buf_id] = 0;
				}
				data->buf_id = data->buf_id + 1;

				fsp_err = R_ADC_ScanStop(&data->adc);
				if (FSP_SUCCESS != fsp_err) {
					break;
				}
			}

			channels = channels >> 1;
		}
		adc_context_on_sampling_done(&data->ctx, dev);
	}

	else if (p_args->event == ADC_EVENT_CALIBRATION_COMPLETE) {
		k_sem_give(&data->calibrate_sem);
	}
}

static int adc_map_vref(const struct adc_ra_config *cfg, adc_extended_cfg_t *extend)
{
	switch (cfg->variant) {
	case ADC_VARIANT_ADC16:
		switch (cfg->reference) {
		case RA_ADC_REF_INTERNAL:
			extend->adc_vref_control = ADC_VREF_CONTROL_2_5V_OUTPUT;
			return 0;
		case RA_ADC_REF_EXTERNAL:
			extend->adc_vref_control = ADC_VREF_CONTROL_VREFH;
			return 0;
		default:
			LOG_ERR("Reference %d not supported", cfg->reference);
			return -ENOTSUP;
		}

	case ADC_VARIANT_ADC12:
		switch (cfg->reference) {
		case RA_ADC_REF_VDD:
			extend->adc_vref_control = ADC_VREF_CONTROL_AVCC0_AVSS0;
			return 0;
		case RA_ADC_REF_EXTERNAL:
			extend->adc_vref_control = ADC_VREF_CONTROL_VREFH0_VREFL0;
			return 0;
		case RA_ADC_REF_INTERNAL:
			extend->adc_vref_control = ADC_VREF_CONTROL_IVREF_AVSS0;
			return 0;
		default:
			LOG_ERR("Reference %d not supported", cfg->reference);
			return -ENOTSUP;
		}

	default:
		LOG_ERR("Variant %d not supported", cfg->variant);
		return -ENOTSUP;
	}
}

static int adc_ra_check_buffer_size(const struct device *dev, const struct adc_sequence *sequence)
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

static int adc_ra_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_ra_config *config = dev->config;
	struct adc_ra_data *data = dev->data;
	fsp_err_t fsp_err = FSP_SUCCESS;
	int err;

	if (config->variant == ADC_VARIANT_ADC16) {
		uint8_t expected = config->resolution - 1;

		if (sequence->resolution != expected) {
			LOG_ERR("unsupported resolution %d for single-ended mode, must be %d",
				sequence->resolution, expected);
			return -ENOTSUP;
		}
	} else if (sequence->resolution != config->resolution) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if ((sequence->channels & ~config->channel_available_mask) != 0) {
		LOG_ERR("unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	err = adc_ra_check_buffer_size(dev, sequence);
	if (err) {
		LOG_ERR("buffer size too small");
		return err;
	}

	data->buf_id = 0;
	data->buf = sequence->buffer;

	if (config->variant == ADC_VARIANT_ADC16) {
		if (!sequence->calibrate) {
			return -ENOTSUP;
		}

		/* Start calibration process */
		k_sem_reset(&data->calibrate_sem);
		fsp_err = R_ADC_Calibrate(&data->adc, NULL);
		if (FSP_SUCCESS != fsp_err) {
			return -EIO;
		}
		k_sem_take(&data->calibrate_sem, K_FOREVER);
	}

	adc_context_start_read(&data->ctx, sequence);

	adc_context_wait_for_completion(&data->ctx);

	return 0;
}

static int adc_ra_read_async(const struct device *dev, const struct adc_sequence *sequence,
			     struct k_poll_signal *async)
{
	struct adc_ra_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_ra_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

static int adc_ra_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_ra_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ra_data *data = CONTAINER_OF(ctx, struct adc_ra_data, ctx);

	data->channels = ctx->sequence.channels;

	R_ADC_ScanStart(&data->adc);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ra_data *data = CONTAINER_OF(ctx, struct adc_ra_data, ctx);

	if (repeat_sampling) {
		data->buf_id = 0;
	}
}

static int adc_ra_init(const struct device *dev)
{
	const struct adc_ra_config *config = dev->config;
	struct adc_ra_data *data = dev->data;
	int ret;
	fsp_err_t fsp_err = FSP_SUCCESS;
	adc_extended_cfg_t *extend = (adc_extended_cfg_t *)data->f_config.p_extend;

	/* Override reference voltage */
	ret = adc_map_vref(config, extend);
	if (ret < 0) {
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->calibrate_sem, 0, 1);

	/* Open ADC module */
	fsp_err = R_ADC_Open(&data->adc, &data->f_config);
	if (FSP_SUCCESS != fsp_err) {
		return -EIO;
	}

	config->irq_configure();

	if (config->variant == ADC_VARIANT_ADC16) {
		/* Start calibration process */
		fsp_err = R_ADC_Calibrate(&data->adc, NULL);
		if (FSP_SUCCESS != fsp_err) {
			return -EIO;
		}
		k_sem_take(&data->calibrate_sem, K_FOREVER);
	}

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

#define EVENT_ADC_SCAN_END(unit) BSP_PRV_IELS_ENUM(CONCAT(EVENT_ADC, unit, _SCAN_END))

#define IRQ_CONFIGURE_FUNC(idx)                                                                    \
	static void adc_ra_configure_func_##idx(void)                                              \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(idx, scanend, irq)] =                             \
			EVENT_ADC_SCAN_END(DT_PROP(DT_DRV_INST(idx), unit));                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, scanend, irq),                                \
			    DT_INST_IRQ_BY_NAME(idx, scanend, priority), adc_scan_end_isr, NULL,   \
			    0);                                                                    \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, scanend, irq));                                \
	}

#define IRQ_CONFIGURE_DEFINE(idx) .irq_configure = adc_ra_configure_func_##idx

#define ADC_RA_INIT_VARIANT(idx, VARIANT, RES_NUM, RES_ENUM)                                       \
	IRQ_CONFIGURE_FUNC(idx)                                                                    \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static adc_extended_cfg_t g_adc_cfg_extend_##idx = {                                       \
		.add_average_count = UTIL_CAT(ADC_AVERAGE_, DT_INST_PROP(idx, average_count)),     \
		.clearing = ADC_CLEAR_AFTER_READ_ON,                                               \
		.trigger_group_b = ADC_START_SOURCE_DISABLED,                                      \
		.double_trigger_mode = ADC_DOUBLE_TRIGGER_DISABLED,                                \
		.adc_vref_control = ADC_VREF_CONTROL_VREFH,                                        \
		.enable_adbuf = 0,                                                                 \
		.window_a_irq = FSP_INVALID_VECTOR,                                                \
		.window_a_ipl = (1),                                                               \
		.window_b_irq = FSP_INVALID_VECTOR,                                                \
		.window_b_ipl = (BSP_IRQ_DISABLED),                                                \
		.trigger = ADC_START_SOURCE_DISABLED,                                              \
	};                                                                                         \
	static DEVICE_API(adc, adc_ra_api_##idx) = {                                               \
		.channel_setup = adc_ra_channel_setup,                                             \
		.read = adc_ra_read,                                                               \
		.ref_internal = DT_INST_PROP(idx, vref_mv),                                        \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_ra_read_async))};                  \
	static const struct adc_ra_config adc_ra_config_##idx = {                                  \
		.channel_available_mask = DT_INST_PROP(idx, channel_available_mask),               \
		.variant = VARIANT,                                                                \
		.reference = DT_INST_ENUM_IDX(idx, reference),                                     \
		.resolution = RES_NUM,                                                             \
		.sampling_time_ns = DT_INST_PROP_OR(idx, sampling_time_ns, UNSPECIFIED),           \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		IRQ_CONFIGURE_DEFINE(idx),                                                         \
	};                                                                                         \
	static struct adc_ra_data adc_ra_data_##idx = {                                            \
		ADC_CONTEXT_INIT_TIMER(adc_ra_data_##idx, ctx),                                    \
		ADC_CONTEXT_INIT_LOCK(adc_ra_data_##idx, ctx),                                     \
		ADC_CONTEXT_INIT_SYNC(adc_ra_data_##idx, ctx),                                     \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
		.f_config =                                                                        \
			{                                                                          \
				.unit = DT_INST_PROP(idx, unit),                                   \
				.mode = ADC_MODE_SINGLE_SCAN,                                      \
				.resolution = RES_ENUM,                                            \
				.alignment = (adc_alignment_t)ADC_ALIGNMENT_RIGHT,                 \
				.trigger = 0,                                                      \
				.p_callback = renesas_ra_adc_callback,                             \
				.p_context = (void *)DEVICE_DT_GET(DT_DRV_INST(idx)),              \
				.p_extend = &g_adc_cfg_extend_##idx,                               \
				.scan_end_irq = DT_INST_IRQ_BY_NAME(idx, scanend, irq),            \
				.scan_end_ipl = DT_INST_IRQ_BY_NAME(idx, scanend, priority),       \
				.scan_end_b_irq = FSP_INVALID_VECTOR,                              \
				.scan_end_b_ipl = (BSP_IRQ_DISABLED),                              \
			},                                                                         \
		.f_channel_cfg =                                                                   \
			{                                                                          \
				.scan_mask = 0,                                                    \
				.scan_mask_group_b = 0,                                            \
				.priority_group_a = ADC_GROUP_A_PRIORITY_OFF,                      \
				.add_mask = UINT16_MAX,                                            \
				.sample_hold_mask = 0,                                             \
				.sample_hold_states = 24,                                          \
				.p_window_cfg = NULL,                                              \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, adc_ra_init, NULL, &adc_ra_data_##idx, &adc_ra_config_##idx,    \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_ra_api_##idx)

#define DT_DRV_COMPAT renesas_ra_adc12
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_RA_INIT_VARIANT, ADC_VARIANT_ADC12, 12, ADC_RESOLUTION_12_BIT)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT renesas_ra_adc16
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADC_RA_INIT_VARIANT, ADC_VARIANT_ADC16, 16, ADC_RESOLUTION_16_BIT)
#undef DT_DRV_COMPAT
