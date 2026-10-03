/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_adc

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_ameba, CONFIG_ADC_LOG_LEVEL);

/* reference voltage for the ADC */
#define AMEBA_ADC_VREF_MV DT_INST_PROP(0, vref_mv)

/* Validate resolution in bits */
#define ADC_RESOLUTION_MIN    (12)
#define VALID_RESOLUTION(r)   ((r) >= ADC_RESOLUTION_MIN)
#define INVALID_RESOLUTION(r) (!VALID_RESOLUTION(r))
#define AMEBA_ADC_MAX_CHANNELS 16U

BUILD_ASSERT(DT_INST_PROP(0, channel_count) <= AMEBA_ADC_MAX_CHANNELS,
	     "Ameba ADC supports at most 16 channel IDs");

struct adc_ameba_config {
	const uint8_t channel_count;
	const struct device *clock;
	const clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
};

struct adc_ameba_data {
	uint16_t meas_ref_internal;
	uint16_t *buffer;
	struct k_mutex lock;
};

static uint8_t adc_ameba_get_channel_id(uint32_t reading)
{
#if defined(BIT_MASK_DAT_CHID)
	return FIELD_GET(BIT_MASK_DAT_CHID, reading);
#else
	return ADC_GET_CH_NUM_GLOBAL(reading);
#endif
}

static int adc_ameba_read(const struct device *dev, const struct adc_sequence *seq)
{
	const struct adc_ameba_config *conf = dev->config;
	struct adc_ameba_data *data = dev->data;
	uint32_t readings[AMEBA_ADC_MAX_CHANNELS];
	uint16_t values[AMEBA_ADC_MAX_CHANNELS] = {0};
	uint8_t channel_ids[AMEBA_ADC_MAX_CHANNELS];
	uint32_t seen_channels = 0U;
	uint8_t channel_count = 0U;
	uint8_t output_index = 0U;
	int ret = 0;

	if (seq->buffer == NULL) {
		LOG_ERR("Sequence buffer is NULL");
		return -EINVAL;
	}

	if (seq->channels == 0U) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (find_msb_set(seq->channels) > conf->channel_count) {
		LOG_ERR("Unsupported channel mask '0x%x'", seq->channels);
		return -ENOTSUP;
	}

	channel_count = POPCOUNT(seq->channels);
	if (seq->buffer_size < channel_count * sizeof(uint16_t)) {
		LOG_ERR("Sequence buffer space too low '%zu'", seq->buffer_size);
		return -ENOMEM;
	}

	if (INVALID_RESOLUTION(seq->resolution)) {
		LOG_ERR("unsupported resolution (%d)", seq->resolution);
		return -ENOTSUP;
	}

	if (seq->options) {
		if (seq->options->extra_samplings) {
			LOG_ERR("Extra samplings not supported");
			return -ENOTSUP;
		}

		if (seq->options->interval_us) {
			LOG_ERR("Interval between samplings not supported");
			return -ENOTSUP;
		}
	}

	if (seq->calibrate) {
		LOG_ERR("calibration is not supported");
		return -ENOTSUP;
	}

	for (uint8_t channel_id = 0U;
	     channel_id < conf->channel_count;
	     channel_id++) {
		if ((seq->channels & BIT(channel_id)) != 0U) {
			channel_ids[output_index++] = channel_id;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ADC_SetChList(channel_ids, channel_count);
	ADC_ReceiveBuf(readings, channel_count);

	for (uint8_t sample_index = 0U; sample_index < channel_count; sample_index++) {
		uint8_t channel_id =
			adc_ameba_get_channel_id(readings[sample_index]);
		int cal;

		if (channel_id >= conf->channel_count ||
		    (seq->channels & BIT(channel_id)) == 0U ||
		    (seen_channels & BIT(channel_id)) != 0U) {
			LOG_ERR("Unexpected channel id '%u'", channel_id);
			ret = -EIO;
			goto out;
		}

		cal = ADC_GetVoltage(ADC_GET_DATA_GLOBAL(readings[sample_index]));
		if (data->meas_ref_internal > 0) {
			cal = (cal << seq->resolution) / data->meas_ref_internal;
		}
		values[channel_id] = cal < 0 ? 0 : cal;
		seen_channels |= BIT(channel_id);
	}

	if (seen_channels != seq->channels) {
		LOG_ERR("ADC channel list incomplete");
		ret = -EIO;
		goto out;
	}

	data->buffer = (uint16_t *)seq->buffer;
	output_index = 0U;
	for (uint8_t channel_id = 0U;
	     channel_id < conf->channel_count;
	     channel_id++) {
		if ((seq->channels & BIT(channel_id)) != 0U) {
			data->buffer[output_index++] = values[channel_id];
		}
	}

out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adc_ameba_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	const struct adc_ameba_config *conf = dev->config;

	if (cfg->channel_id >= conf->channel_count) {
		LOG_ERR("Unsupported channel id '%d'", cfg->channel_id);
		return -ENOTSUP;
	}

	if (cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain '%d'", cfg->gain);
		return -EINVAL;
	}

	if (cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference '%d'", cfg->reference);
		return -ENOTSUP;
	}

	if (cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	LOG_DBG("Channel setup succeeded!");

	return 0;
}

static int adc_ameba_init(const struct device *dev)
{
	const struct adc_ameba_config *config = dev->config;
	ADC_InitTypeDef adc_init_struct;

	if (!device_is_ready(config->clock)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(config->clock, config->clock_subsys)) {
		LOG_ERR("Could not enable ADC clock");
		return -EIO;
	}

	int ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		return ret;
	}

	ADC_StructInit(&adc_init_struct);
	adc_init_struct.ADC_OpMode = ADC_AUTO_MODE;
	ADC_Init(&adc_init_struct);
	ADC_Cmd(ENABLE);
	k_mutex_init(&((struct adc_ameba_data *)dev->data)->lock);

	return 0;
}

static DEVICE_API(adc, api_ameba_driver_api) = {
	.channel_setup = adc_ameba_channel_setup,
	.read = adc_ameba_read,
	.ref_internal = AMEBA_ADC_VREF_MV,
};

PINCTRL_DT_INST_DEFINE(0);

static const struct adc_ameba_config adc_config = {
	.channel_count = DT_PROP(DT_DRV_INST(0), channel_count),
	.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, idx),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct adc_ameba_data adc_data = {
	.meas_ref_internal = AMEBA_ADC_VREF_MV,
};

DEVICE_DT_INST_DEFINE(0, &adc_ameba_init, NULL, &adc_data, &adc_config, POST_KERNEL,
		      CONFIG_ADC_INIT_PRIORITY, &api_ameba_driver_api);
