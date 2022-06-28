/*
 * Copyright (c) 2022 Wolter HV <wolterhv@gmx.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * adc_stm32.c and adc_mcp320x have been used as a reference for this
 * implementation.
 */

#define DT_DRV_COMPAT espressif_esp32_adc

#include <errno.h>

#include <drivers/adc.h>
#include <device.h>
#include <kernel.h>
#include <logging/log.h>

#include <hal/adc_hal.h>
#include <esp_adc_cal.h>

#include "drivers/adc/adc_esp32.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
LOG_MODULE_REGISTER(adc_esp32);


/* Macro constants */

#define ADC_ESP32_RESOLUTION_OFFSET        (9)
#define ADC_ESP32_DEFAULT_VREF_INTERNAL (1100)  /* this is the default used in
						   espressif:adc1_example_main.c */


/* Macro functions */

#define MIN(a, b)          (((a) < (b)) ? (a) : (b))
#define MAX(a, b)          (((a) > (b)) ? (a) : (b))
#define CLIP(min, max, x)  (MIN(MAX((min), (x)), (max)))
#define ELEM_COUNT(arr)    (sizeof(arr)/sizeof(arr[0]))

#define LINTERP(x0, y0, x1, y1, x)					\
((y0) + (((y1)-(y0))*((x)-(x0)))/((x1)-(x0)))

#define DECLARE_DEVCONF(dev, devconf)					\
const struct adc_esp32_conf *(devconf) =				\
	(const struct adc_esp32_conf *) (dev)->config;

#define DECLARE_DEVDATA(dev, devdata)					\
struct adc_esp32_data *(devdata) =					\
	(struct adc_esp32_data *) (dev)->data;

/* Declarations */

struct adc_esp32_conf;
struct adc_esp32_data;

static int8_t adc_esp32_atten_map[][2] = {
	{ (int8_t) ADC_ESP32_ATTEN_0, (int8_t) ADC_ATTEN_DB_0   },
	{ (int8_t) ADC_ESP32_ATTEN_1, (int8_t) ADC_ATTEN_DB_2_5 },
	{ (int8_t) ADC_ESP32_ATTEN_2, (int8_t) ADC_ATTEN_DB_6   },
	{ (int8_t) ADC_ESP32_ATTEN_3, (int8_t) ADC_ATTEN_DB_11  },
};

static uint8_t adc_esp32_resolution_map[][2] = {
#if CONFIG_IDF_TARGET_ESP32
	{(uint8_t) ADC_WIDTH_BIT_9,   9},
	{(uint8_t) ADC_WIDTH_BIT_10, 10},
	{(uint8_t) ADC_WIDTH_BIT_11, 11},
	{(uint8_t) ADC_WIDTH_BIT_12, 12},
#elif SOC_ADC_MAX_BITWIDTH == 12
	{(uint8_t) ADC_WIDTH_BIT_12, 12},
#elif SOC_ADC_MAX_BITWIDTH == 13
	{(uint8_t) ADC_WIDTH_BIT_13, 13},
#endif
};

/* API functions */

static int adc_esp32_init		(const struct device *dev);
static int adc_esp32_channel_setup	(const struct device *dev,
					 const struct adc_channel_cfg *channel_cfg);
static int adc_esp32_read		(const struct device *dev,
					 const struct adc_sequence *sequence);
#ifdef CONFIG_ADC_ASYNC
static int adc_esp32_read_async		(const struct device *dev,
					 const struct adc_sequence *sequence,
					 struct k_poll_signal *async);
#endif /* CONFIG_ADC_ASYNC */

/* Helpers */

static int validate_channel_id		(const struct device  *dev,
					 const uint8_t         channel_id);
static int validate_resolution		(const uint8_t resolution);
static int encode_resolution		(const uint8_t      resolution,
					 adc_bits_width_t  *esp32_resolution);
static int decode_resolution		(const adc_bits_width_t   esp32_resolution,
					 uint8_t                 *resolution);
static int validate_attenuation		(const adc_atten_t  attenuation);
static int encode_attenuation		(const enum adc_esp32_atten_e   atten,
					 adc_atten_t                   *esp32_atten);
static int decode_attenuation		(const adc_atten_t        esp32_atten,
					 enum adc_esp32_atten_e  *atten);


/* Definitions */

static const struct adc_driver_api api_esp32_driver_api = {
	.channel_setup = adc_esp32_channel_setup,
	.read          = adc_esp32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_esp32_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal  = ADC_ESP32_DEFAULT_VREF_INTERNAL,
};

/*
 * Things to configure via devicetree
 * ----------------------------------
 * - channel arrays on each ADC (channel count)
 * - supported resolutions (widths in bits)
 * - reference voltage
 * - supported gains
 * - supported acquisition times
 * - whether differential is supported
 */
struct adc_esp32_conf {
	adc_ll_num_t  adc_num;
	uint8_t       channel_count; /* maps to adc1_channel_t or
					adc2_channel_t, must be set to the
					relevant ADCn_CHANNEL_MAX from the
					devicetree configuration. */
	/* TODO instead of channel count, we can have an array in the devicetree
	 * description, and take its length */
};

struct adc_esp32_data {
	uint16_t                        meas_ref_internal; /* mV, measured,
							     update on
							     calibration */
	uint16_t                       *buffer;
	adc_atten_t                     atten[ADC_CHANNEL_MAX];
	esp_adc_cal_characteristics_t   chars;
};

/*
 * According to espressif:adc1_example_main.c, adc2_vref_to_gpio is a function
 * used to measure the internal voltage reference.
 *
 * According to the espressif:esp_adc_cal_characterize function, the function
 * read_efuse_vref can be used to obtained a reading as well, but apparently
 * the so-called efuse is not always available, this can be checked with
 * check_efuse_vref.
 */

/* Extra ESP32 API functions */

int adc_esp32_get_atten(const struct device     *dev,
			const uint8_t            channel_id,
			enum adc_esp32_atten_e  *atten)
{
	DECLARE_DEVDATA(dev, devdata);
	adc_atten_t esp32_atten;
	int         err;

	err = validate_channel_id(dev, channel_id);
	if (err < 0) {
		return -EINVAL;
	}

	atten = NULL;
	esp32_atten = devdata->atten[channel_id];

	for (int i = 0; i < ELEM_COUNT(adc_esp32_atten_map); i++) {
		if (esp32_atten == adc_esp32_atten_map[i][1]) {
			*atten = adc_esp32_atten_map[i][0];
		}
	}
	if (atten == NULL) {
		LOG_ERR("invalid attenuation");
		return -EINVAL;
	}

	return 0;
}

int adc_esp32_set_atten(const struct device           *dev,
			const uint8_t                  channel_id,
			const enum adc_esp32_atten_e   atten)
{
	DECLARE_DEVCONF(dev, devconf);
	DECLARE_DEVDATA(dev, devdata);
	int         err;
	adc_atten_t esp32_atten;

	err = validate_channel_id(dev, channel_id);
	if (err < 0) {
		return -EINVAL;
	}

	esp32_atten = ADC_ATTEN_MAX;
	for (int i = 0; i < ELEM_COUNT(adc_esp32_atten_map); i++) {
		if (atten == adc_esp32_atten_map[i][0]) {
			esp32_atten = adc_esp32_atten_map[i][1];
		}
	}
	if (esp32_atten == ADC_ATTEN_MAX) {
		LOG_ERR("invalid attenuation");
		return -EINVAL;
	}

	devdata->atten[channel_id] = esp32_atten;
	adc_hal_set_atten(devconf->adc_num,
			  (adc_channel_t) channel_id,
			  devdata->atten[channel_id]);

	return 0;
}

int adc_esp32_update_meas_ref_internal(const struct device *dev)
{
	DECLARE_DEVDATA(dev, devdata);

	/* TODO add logic to actually update the measurement */
	devdata->meas_ref_internal = ADC_ESP32_DEFAULT_VREF_INTERNAL;

	return 0;
}

int adc_esp32_get_meas_ref_internal(const struct device  *dev,
				    uint16_t             *meas_ref_internal)
{
	DECLARE_DEVDATA(dev, devdata);

	*meas_ref_internal = devdata->meas_ref_internal;

	return 0;
}

int adc_esp32_characterize_by_atten(const struct device           *dev,
			            const uint8_t                  resolution,
			            const enum adc_esp32_atten_e   atten)
{
	DECLARE_DEVCONF(dev, devconf);
	DECLARE_DEVDATA(dev, devdata);
	int			err;
	adc_atten_t		esp32_atten;
	adc_bits_width_t	esp32_resolution;

	err = encode_attenuation(atten, &esp32_atten);
	if (err < 0) {
		return -EINVAL;
	}

	err = encode_resolution(resolution, &esp32_resolution);
	if (err < 0) {
		return -ENOTSUP;
	}

	esp_adc_cal_characterize( devconf->adc_num,
				  esp32_atten,
				  esp32_resolution,
				  devdata->meas_ref_internal,
				 &devdata->chars);
	/* Instead of ADC_ESP32_DEFAULT_VREF_INTERNAL use adc2_vref_to_gpio for
	 * a better estimate */

	return 0;
}

int adc_esp32_characterize_by_channel(const struct device  *dev,
				      const uint8_t         resolution,
				      const uint8_t         channel_id)
{
	DECLARE_DEVDATA(dev, devdata);
	int			err;
	enum adc_esp32_atten_e  atten;

	err = validate_channel_id(dev, channel_id);
	if (err < 0) {
		return -ENOTSUP;
	}

	err = decode_attenuation(devdata->atten[channel_id], &atten);
	if (err < 0) {
		return -EINVAL;
	}

	return adc_esp32_characterize_by_atten(dev, resolution, atten);
}

int adc_esp32_raw_to_millivolts(const struct device  *dev,
			        int32_t              *value)
{
	DECLARE_DEVDATA(dev, devdata);

	*value = esp_adc_cal_raw_to_voltage(*value, &devdata->chars);

	return 0;
}


/* Driver implementation functions */

static int adc_esp32_init(const struct device *dev)
{
	DECLARE_DEVCONF(dev, devconf);
	DECLARE_DEVDATA(dev, devdata);

	LOG_DBG("initialising");

	for (uint8_t ch_id = 0; ch_id < ELEM_COUNT(devdata->atten); ch_id++) {
		devdata->atten[ch_id] = ADC_ATTEN_DB_0;
	}

	devdata->meas_ref_internal = ADC_ESP32_DEFAULT_VREF_INTERNAL;

	adc_hal_init();

	/* TODO this depends on the selected clock. Inverted seems to be the
	 * default behaviour in ESP-IDF */
	adc_ll_rtc_output_invert  (devconf->adc_num, true);
	adc_ll_digi_output_invert (devconf->adc_num, true);

	return 0;
}

static int
adc_esp32_channel_setup	(const struct device *dev,
			 const struct adc_channel_cfg *channel_cfg)
{
	DECLARE_DEVCONF(dev, devconf);
	DECLARE_DEVDATA(dev, devdata);
	int err;

	err = validate_channel_id(dev, channel_cfg->channel_id);
	if (err < 0) {
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("unsupported channel gain '%d'", channel_cfg->gain);
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("unsupported channel reference '%d'",
			channel_cfg->reference);
		return -ENOTSUP;
	}

	/* TODO Find whether other acquisition times are supported */
	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("unsupported acquisition_time '%d'",
			channel_cfg->acquisition_time);
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		LOG_ERR("differential channels are not supported");
		return -ENOTSUP;
	}

	adc_hal_set_atten(devconf->adc_num,
			  (adc_channel_t) channel_cfg->channel_id,
			  devdata->atten[channel_cfg->channel_id]);
	/* Resolution is set in read call */

	return 0;
}

/*
 * Channels, buffer and resolution are in the sequence param.
 * Other device-specific stuff is retrieved from the dev param.
 * Reads samples for the channels specified in the sequence struct.
 * Stores one sample per channel on the sequence struct buffer.
 */
static int
adc_esp32_read	(const struct device *dev,
		 const struct adc_sequence *sequence)
{
	DECLARE_DEVCONF(dev, devconf);
	DECLARE_DEVDATA(dev, devdata);
	adc_bits_width_t  esp32_resolution;
	int err;
	int reading;

	if ((size_t) devconf->channel_count > sequence->buffer_size) {
		LOG_ERR("sequence buffer only has space for %u channel values, "
			"but device has %u channels",
			(uint16_t) sequence->buffer_size,
			(uint16_t) devconf->channel_count);
		return -ENOMEM;
	}

	/* the adc_sequence struct member "channels" is a 32-bit bitfield with
	 * the channels to get a reading for. Possibly in this function we will
	 * have a loop instead, that will get a reading for each channel. */
	/* The STM32 returns an error when multiple channels are selected, and
	 * that's how it gets away with doing just single channel reads. */
	/* TODO: Find out whether the ESP32 supports reading multiple channels
	 * at a time.  In the meantime, do what STM32 does. */
	uint8_t channel_id = find_lsb_set(sequence->channels) - 1;
	if (sequence->channels > BIT(channel_id)) {
		LOG_ERR("multichannel readings unsupported");
		return -ENOTSUP;
	}

	if (sequence->calibrate) {
		/* TODO: Find out how to calibrate. Possibly, resolution needs
		 * to be applied before calibration. */
		LOG_ERR("calibration is not supported");
		return -ENOTSUP;
	}

	err = encode_resolution(sequence->resolution, &esp32_resolution);
	if (err < 0) {
		return -ENOTSUP;
	}
	adc_hal_rtc_set_output_format(devconf->adc_num, esp32_resolution);
	/* adc_ll_digi_set_output_format() */ /* TODO support later */

#ifdef CONFIG_IDF_TARGET_ESP32
	adc_hal_hall_disable();
	adc_hal_amp_disable();
#endif /* ifdef CONFIG_IDF_TARGET_ESP32 */
	adc_hal_set_controller(devconf->adc_num, ADC_CTRL_RTC);
	adc_hal_convert(devconf->adc_num, channel_id, &reading);
#if !CONFIG_IDF_TARGET_ESP32
	adc_hal_rtc_reset();
#endif /* if !CONFIG_IDF_TARGET_ESP32 */

	devdata->buffer             = (uint16_t *) sequence->buffer;
	devdata->buffer[channel_id] = (uint16_t)   reading;

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int
adc_esp32_read_async	(const struct device *dev,
			 const struct adc_sequence *sequence,
			 struct k_poll_signal *async)
{
	(void)(dev);
	(void)(sequence);
	(void)(async);

	/*
	 * TODO Look into the adc_ll_rtc_convert_is_done function, maybe this is
	 * supported.
	 */

	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */


/* Utility functions */

static int validate_channel_id(const struct device  *dev,
					 const uint8_t         channel_id)
{
	DECLARE_DEVCONF(dev, devconf);
	if (channel_id >= devconf->channel_count) {
		LOG_ERR("unsupported channel id '%u'", channel_id);
		return -ENOTSUP;
	}
	return 0;
}

static int validate_resolution(const uint8_t resolution)
{
	for (int i = 0; i < ELEM_COUNT(adc_esp32_resolution_map); i++) {
		if (resolution == adc_esp32_resolution_map[i][1]) {
			return 0;
		}
	}
	LOG_ERR("resolution not supported");
	return -ENOTSUP;
}

static int validate_attenuation(const adc_atten_t esp32_atten)
{
	for (int i = 0; i < ELEM_COUNT(adc_esp32_atten_map); i++) {
		if (esp32_atten == (adc_atten_t) adc_esp32_atten_map[i][1]) {
			return 0;
		}
	}
	LOG_ERR("attenuation is invalid");
	return -EINVAL;
}

static int encode_attenuation	(const enum adc_esp32_atten_e   atten,
				 adc_atten_t                   *esp32_atten)
{
	for (int i = 0; i < ELEM_COUNT(adc_esp32_atten_map); i++) {
		if ((int8_t) atten == adc_esp32_atten_map[i][0]) {
			*esp32_atten = (adc_atten_t) adc_esp32_atten_map[i][1];
			return 0;
		}
	}
	LOG_ERR("attenuation is invalid");
	return -EINVAL;
}

static int encode_resolution(const uint8_t      resolution,
			     adc_bits_width_t  *esp32_resolution)
{
	for (int i = 0; i < ELEM_COUNT(adc_esp32_resolution_map); i++) {
		if (resolution == adc_esp32_resolution_map[i][1]) {
			*esp32_resolution =
				(adc_bits_width_t) adc_esp32_resolution_map[i][0];
			return 0;
		}
	}
	LOG_ERR("resolution not supported");
	return -ENOTSUP;
}

static int decode_resolution(const adc_bits_width_t   esp32_resolution,
				       uint8_t                 *resolution)
{
	resolution = NULL;
	for (int i = 0; i < ELEM_COUNT(adc_esp32_resolution_map); i++) {
		if ((uint8_t) esp32_resolution == adc_esp32_resolution_map[i][0]) {
			*resolution = adc_esp32_resolution_map[i][1];
			return 0;
		}
	}
	LOG_ERR("resolution not supported");
	return -ENOTSUP;
}

static int decode_attenuation	(const adc_atten_t        esp32_atten,
				 enum adc_esp32_atten_e  *atten)
{
	for (int i = 0; i < ELEM_COUNT(adc_esp32_atten_map); i++) {
		if (esp32_atten == (adc_atten_t) adc_esp32_atten_map[i][1]) {
			*atten = (enum adc_esp32_atten_e) adc_esp32_atten_map[i][0];
			return 0;
		}
	}
	LOG_ERR("attenuation is invalid");
	return -EINVAL;
}

/* Footer */

#define ADC_ESP32_CONFIG(index)						\
static const struct adc_esp32_conf adc_esp32_conf_##index = {		\
	.adc_num       = DT_PROP(DT_DRV_INST(index), adc_num),		\
	.channel_count = DT_PROP(DT_DRV_INST(index), channel_count),	\
};

#define ESP32_ADC_INIT(index)						\
									\
ADC_ESP32_CONFIG(index)							\
									\
static struct adc_esp32_data adc_esp32_data_##index = {			\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		      &adc_esp32_init, NULL,				\
		      &adc_esp32_data_##index, &adc_esp32_conf_##index,	\
		      POST_KERNEL,			\
		      CONFIG_ADC_INIT_PRIORITY,				\
		      &api_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_ADC_INIT)
