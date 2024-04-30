/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_adc

#include <errno.h>
#include <hal/adc_hal.h>
#include <hal/adc_types.h>
#include <esp_adc_cal.h>
#include <esp_private/periph_ctrl.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_esp32, CONFIG_ADC_LOG_LEVEL);

#define ADC_RESOLUTION_MIN	SOC_ADC_DIGI_MIN_BITWIDTH
#define ADC_RESOLUTION_MAX	SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_SOC_SERIES_ESP32
#define ADC_CALI_SCHEME		ESP_ADC_CAL_VAL_EFUSE_VREF
/* Due to significant measurement discrepancy in higher voltage range, we
 * clip the value instead of yet another correction. The IDF implementation
 * for ESP32-S2 is doing it, so we copy that approach in Zephyr driver
 */
#define ADC_CLIP_MVOLT_11DB	2550
#else
#define ADC_CALI_SCHEME		ESP_ADC_CAL_VAL_EFUSE_TP
#endif

/* Convert resolution in bits to esp32 enum values */
#define WIDTH_MASK(r) ((((r) - 9) < ADC_WIDTH_MAX) ? ((r) - 9) : (ADC_WIDTH_MAX - 1))

/* Validate if resolution in bits is within allowed values */
#define VALID_RESOLUTION(r) ((r) >= ADC_RESOLUTION_MIN && (r) <= ADC_RESOLUTION_MAX)
#define INVALID_RESOLUTION(r) (!VALID_RESOLUTION(r))

/* Default internal reference voltage */
#define ADC_ESP32_DEFAULT_VREF_INTERNAL (1100)

struct adc_esp32_conf {
	adc_unit_t unit;
	uint8_t channel_count;
};

struct adc_esp32_data {
	adc_atten_t attenuation[SOC_ADC_MAX_CHANNEL_NUM];
	uint8_t resolution[SOC_ADC_MAX_CHANNEL_NUM];
	esp_adc_cal_characteristics_t chars[SOC_ADC_MAX_CHANNEL_NUM];
	uint16_t meas_ref_internal;
	uint16_t *buffer;
	uint16_t *buffer_repeat;
	bool calibrate;
};

/* Convert zephyr,gain property to the ESP32 attenuation */
static inline int gain_to_atten(enum adc_gain gain, adc_atten_t *atten)
{
	switch (gain) {
	case ADC_GAIN_1:
		*atten = ADC_ATTEN_DB_0;
		break;
	case ADC_GAIN_4_5:
		*atten = ADC_ATTEN_DB_2_5;
		break;
	case ADC_GAIN_1_2:
		*atten = ADC_ATTEN_DB_6;
		break;
	case ADC_GAIN_1_4:
		*atten = ADC_ATTEN_DB_11;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

/* Convert voltage by inverted attenuation to support zephyr gain values */
static void atten_to_gain(adc_atten_t atten, uint32_t *val_mv)
{
	if (!val_mv) {
		return;
	}
	switch (atten) {
	case ADC_ATTEN_DB_2_5:
		*val_mv = (*val_mv * 4) / 5; /* 1/ADC_GAIN_4_5 */
		break;
	case ADC_ATTEN_DB_6:
		*val_mv = *val_mv >> 1; /* 1/ADC_GAIN_1_2 */
		break;
	case ADC_ATTEN_DB_11:
		*val_mv = *val_mv / 4; /* 1/ADC_GAIN_1_4 */
		break;
	case ADC_ATTEN_DB_0: /* 1/ADC_GAIN_1 */
	default:
		break;
	}
}

static bool adc_calibration_init(const struct device *dev)
{
	struct adc_esp32_data *data = dev->data;

	switch (esp_adc_cal_check_efuse(ADC_CALI_SCHEME)) {
	case ESP_ERR_NOT_SUPPORTED:
		LOG_WRN("Skip software calibration - Not supported!");
		break;
	case ESP_ERR_INVALID_VERSION:
		LOG_WRN("Skip software calibration - Invalid version!");
		break;
	case ESP_OK:
		LOG_DBG("Software calibration possible");
		return true;
	default:
		LOG_ERR("Invalid arg");
		break;
	}
	return false;
}

static int adc_esp32_read(const struct device *dev, const struct adc_sequence *seq)
{
	const struct adc_esp32_conf *conf = dev->config;
	struct adc_esp32_data *data = dev->data;
	int reading;
	uint32_t cal, cal_mv;

	uint8_t channel_id = find_lsb_set(seq->channels) - 1;

	if (seq->buffer_size < 2) {
		LOG_ERR("Sequence buffer space too low '%d'", seq->buffer_size);
		return -ENOMEM;
	}

	if (seq->channels > BIT(channel_id)) {
		LOG_ERR("Multi-channel readings not supported");
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

	if (INVALID_RESOLUTION(seq->resolution)) {
		LOG_ERR("unsupported resolution (%d)", seq->resolution);
		return -ENOTSUP;
	}

	if (seq->calibrate) {
		/* TODO: Does this mean actual Vref measurement on selected GPIO ?*/
		LOG_ERR("calibration is not supported");
		return -ENOTSUP;
	}

	data->resolution[channel_id] = seq->resolution;

#if CONFIG_SOC_SERIES_ESP32C3
	/* NOTE: nothing to set on ESP32C3 SoC */
	if (conf->unit == ADC_UNIT_1) {
		adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
	}
#else
	adc_set_data_width(conf->unit, WIDTH_MASK(data->resolution[channel_id]));
#endif /* CONFIG_SOC_SERIES_ESP32C3 */

	/* Read raw value */
	if (conf->unit == ADC_UNIT_1) {
		reading = adc1_get_raw(channel_id);
	}
	if (conf->unit == ADC_UNIT_2) {
		if (adc2_get_raw(channel_id, ADC_WIDTH_BIT_DEFAULT, &reading)) {
			LOG_ERR("Conversion timeout on '%s' channel %d", dev->name, channel_id);
			return -ETIMEDOUT;
		}
	}

	/* Calibration scheme is available */
	if (data->calibrate) {
		data->chars[channel_id].bit_width = WIDTH_MASK(data->resolution[channel_id]);
		/* Get corrected voltage output */
		cal = cal_mv = esp_adc_cal_raw_to_voltage(reading, &data->chars[channel_id]);

#if CONFIG_SOC_SERIES_ESP32
		if (data->attenuation[channel_id] == ADC_ATTEN_DB_11) {
			if (cal > ADC_CLIP_MVOLT_11DB) {
				cal = ADC_CLIP_MVOLT_11DB;
			}
		}
#endif /* CONFIG_SOC_SERIES_ESP32 */

		/* Fit according to selected attenuation */
		atten_to_gain(data->attenuation[channel_id], &cal);
		if (data->meas_ref_internal > 0) {
			cal = (cal << data->resolution[channel_id]) / data->meas_ref_internal;
		}
	} else {
		LOG_DBG("Using uncalibrated values!");
		/* Uncalibrated raw value */
		cal = reading;
	}

	/* Store result */
	data->buffer = (uint16_t *) seq->buffer;
	data->buffer[0] = cal;

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_esp32_read_async(const struct device *dev,
				const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	(void)(dev);
	(void)(sequence);
	(void)(async);

	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_esp32_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	const struct adc_esp32_conf *conf = (const struct adc_esp32_conf *)dev->config;
	struct adc_esp32_data *data = (struct adc_esp32_data *) dev->data;
	int err;

	if (cfg->channel_id >= conf->channel_count) {
		LOG_ERR("Unsupported channel id '%d'", cfg->channel_id);
		return -ENOTSUP;
	}

	if (cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference '%d'", cfg->reference);
		return -ENOTSUP;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported acquisition_time '%d'", cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (gain_to_atten(cfg->gain, &data->attenuation[cfg->channel_id])) {
		LOG_ERR("Unsupported gain value '%d'", cfg->gain);
		return -ENOTSUP;
	}

	/* Prepare channel */
	if (conf->unit == ADC_UNIT_1) {
		adc1_config_channel_atten(cfg->channel_id, data->attenuation[cfg->channel_id]);
	}
	if (conf->unit == ADC_UNIT_2) {
		adc2_config_channel_atten(cfg->channel_id, data->attenuation[cfg->channel_id]);
	}

	if (data->calibrate) {
		esp_adc_cal_value_t cal = esp_adc_cal_characterize(conf->unit,
						data->attenuation[cfg->channel_id],
						WIDTH_MASK(data->resolution[cfg->channel_id]),
						data->meas_ref_internal,
						&data->chars[cfg->channel_id]);
		if (cal >= ESP_ADC_CAL_VAL_NOT_SUPPORTED) {
			LOG_ERR("Calibration error or not supported");
			return -EIO;
		}
		LOG_DBG("Using ADC calibration method %d", cal);
	}

	return 0;
}

static int adc_esp32_init(const struct device *dev)
{
	struct adc_esp32_data *data = (struct adc_esp32_data *) dev->data;

	for (uint8_t i = 0; i < ARRAY_SIZE(data->resolution); i++) {
		data->resolution[i] = ADC_RESOLUTION_MAX;
	}
	for (uint8_t i = 0; i < ARRAY_SIZE(data->attenuation); i++) {
		data->attenuation[i] = ADC_ATTEN_DB_0;
	}

	/* Default reference voltage. This could be calibrated externaly */
	data->meas_ref_internal = ADC_ESP32_DEFAULT_VREF_INTERNAL;

	/* Check if calibration is possible */
	data->calibrate = adc_calibration_init(dev);

	return 0;
}

static const struct adc_driver_api api_esp32_driver_api = {
	.channel_setup = adc_esp32_channel_setup,
	.read          = adc_esp32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_esp32_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal  = ADC_ESP32_DEFAULT_VREF_INTERNAL,
};

#define ESP32_ADC_INIT(inst)							\
										\
	static const struct adc_esp32_conf adc_esp32_conf_##inst = {		\
		.unit = DT_PROP(DT_DRV_INST(inst), unit) - 1,			\
		.channel_count = DT_PROP(DT_DRV_INST(inst), channel_count),	\
	};									\
										\
	static struct adc_esp32_data adc_esp32_data_##inst = {			\
	};									\
										\
DEVICE_DT_INST_DEFINE(inst, &adc_esp32_init, NULL,				\
		&adc_esp32_data_##inst,						\
		&adc_esp32_conf_##inst,						\
		POST_KERNEL,							\
		CONFIG_ADC_INIT_PRIORITY,					\
		&api_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_ADC_INIT)
