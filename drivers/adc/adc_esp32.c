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

#include "drivers/adc/adc_esp32.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
LOG_MODULE_REGISTER(adc_esp32);

#define ADC_ESP32_RESOLUTION_OFFSET     (9)

#define MIN(a, b)          (((a) < (b)) ? (a) : (b))
#define MAX(a, b)          (((a) > (b)) ? (a) : (b))
#define CLIP(min, max, x)  (MIN(MAX((min), (x)), (max)))
#define ELEM_COUNT(arr)    (sizeof(arr)/sizeof(arr[0]))

#define ADC_ESP32_LINTERP(x0, y0, x1, y1, x)				\
((y0) + (((y1)-(y0))*((x)-(x0)))/((x1)-(x0)))

#define ADC_ESP32_DECLARE_CONST_DEVCONF(dev, devconf)			\
const struct adc_esp32_conf *(devconf) =				\
	(const struct adc_esp32_conf *) (dev)->config;

#define ADC_ESP32_DECLARE_DEVDATA(dev, devdata)				\
struct adc_esp32_data *(devdata) =					\
	(struct adc_esp32_data *) (dev)->data;

struct adc_esp32_conf;
struct adc_esp32_data;

static int adc_esp32_init          (const struct device *dev);
static int adc_esp32_channel_setup (const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg);
static int adc_esp32_read          (const struct device *dev,
			            const struct adc_sequence *sequence);
#ifdef CONFIG_ADC_ASYNC
static int adc_esp32_read_async    (const struct device *dev,
			            const struct adc_sequence *sequence,
			            struct k_poll_signal *async);
#endif /* CONFIG_ADC_ASYNC */

static int adc_esp32_validate_channel_id(const struct device  *dev,
					 const uint8_t         channel_id);

static const struct adc_driver_api api_esp32_driver_api = {
	.channel_setup = adc_esp32_channel_setup,
	.read          = adc_esp32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_esp32_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal  = 1100, /* this is the default used in
			          espressif:adc1_example_main.c */
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

/* To house ESP32-specific stuff */
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
	uint16_t      mes_ref_internal; /* mV, measured, update on calibration */
	uint16_t     *buffer;
	adc_atten_t   atten[ADC_CHANNEL_MAX];
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


/* Exposed functions */

int adc_esp32_set_atten(const struct device           *dev,
			const uint8_t                  channel_id,
			const enum adc_esp32_atten_e   atten)
{
	ADC_ESP32_DECLARE_CONST_DEVCONF(dev, devconf);
	ADC_ESP32_DECLARE_DEVDATA(dev, devdata);
	int         err;
	adc_atten_t esp32_atten;

	err = adc_esp32_validate_channel_id(dev, channel_id);
	if (err < 0) {
		return err;
	}

	switch (atten) {
	case ADC_ESP32_ATTEN_0:
		esp32_atten = ADC_ATTEN_DB_0;
		break;
	case ADC_ESP32_ATTEN_1:
		esp32_atten = ADC_ATTEN_DB_2_5;
		break;
	case ADC_ESP32_ATTEN_2:
		esp32_atten = ADC_ATTEN_DB_6;
		break;
	case ADC_ESP32_ATTEN_3:
		esp32_atten = ADC_ATTEN_DB_11;
		break;
	default:
		LOG_ERR("invalid attenuation");
		return -EINVAL;
		break;
	}

	devdata->atten[channel_id] = esp32_atten;

	adc_hal_set_atten(devconf->adc_num,
			  (adc_channel_t) channel_id,
			  devdata->atten[channel_id]);

	return 0;
}

int adc_esp32_raw_to_millivolts(const struct device  *dev,
				const uint8_t         channel_id,
				const uint8_t         resolution,
				const int32_t         adc_ref_voltage,
			        int32_t              *valp)
{
	ADC_ESP32_DECLARE_DEVDATA(dev, devdata);
	int err;

	err = adc_esp32_validate_channel_id(dev, channel_id);
	if (err < 0) {
		return err;
	}

	const int x0 =                 0; int y0;
	const int x1 = (1 << resolution); int y1;

	switch (devdata->atten[channel_id]) {
	case ADC_ATTEN_DB_0:
		y0 =  100;
		y1 =  950;
		break;
	case ADC_ATTEN_DB_2_5:
		y0 =  100;
		y1 = 1250;
		break;
	case ADC_ATTEN_DB_6:
		y0 =  150;
		y1 = 1750;
		break;
	case ADC_ATTEN_DB_11:
		y0 =  150;
		y1 = 2450;
		break;
	default:
		goto invalid_atten;
		break;
	}

	*valp = MIN(INT32_MAX,
		    (adc_ref_voltage * ADC_ESP32_LINTERP(x0, y0, x1, y1, *valp)))
		/ 1100;

	return 0;

invalid_atten:
	return -ENOTSUP;
}


/* Driver implementation functions */

static int adc_esp32_init(const struct device *dev)
{
	ADC_ESP32_DECLARE_DEVDATA(dev, devdata);

	LOG_DBG("initialising");

	for (uint8_t ch_id = 0; ch_id < ELEM_COUNT(devdata->atten); ch_id++) {
		devdata->atten[ch_id] = ADC_ATTEN_DB_0;
	}

	/* adc_hal_init(); */
	return 0;
}

static int
adc_esp32_channel_setup	(const struct device *dev,
			 const struct adc_channel_cfg *channel_cfg)
{
	ADC_ESP32_DECLARE_CONST_DEVCONF(dev, devconf);
	ADC_ESP32_DECLARE_DEVDATA(dev, devdata);
	int err;

	err = adc_esp32_validate_channel_id(dev, channel_cfg->channel_id);
	if (err < 0) {
		return err;
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
	ADC_ESP32_DECLARE_CONST_DEVCONF(dev, devconf);
	ADC_ESP32_DECLARE_DEVDATA(dev, devdata);

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

	/* resolution in Zephyr is width in Espressif. */
	/* ESP32 supports only resolutions of 9, 10, 11 and 12 bits, which are
	 * mapped as follows:
	 * zephyr_resolution : espressif_resolution
	 * -           9 bit : 0
	 * -          10 bit : 1
	 * -          11 bit : 2
	 * -          12 bit : 3
	 * i.e., zephyr_resolution - 9 = espressif_resolution
	 * Other ESP32XX versions may support up to 13 bits of resolution.
	 */
	int8_t offset_resolution =   (int8_t) sequence->resolution
				   - ADC_ESP32_RESOLUTION_OFFSET;
	adc_bits_width_t esp32_resolution =
		(adc_bits_width_t) CLIP((int8_t) ADC_WIDTH_BIT_9,
				        (int8_t) SOC_ADC_MAX_BITWIDTH,
				        (int8_t) offset_resolution);
	if (offset_resolution != (int8_t) esp32_resolution) {
		LOG_ERR("resolution not supported, using nearest: %u bits",
			(uint8_t) esp32_resolution);
	}
	adc_hal_rtc_set_output_format(devconf->adc_num,
				      esp32_resolution);
	/* adc_ll_digi_set_output_format() */ /* TODO support later */

	if (sequence->calibrate) {
		/* TODO: Find out how to calibrate. Possibly, resolution needs
		 * to be applied before calibration. */
		LOG_ERR("calibration is not supported");
	}

	/* TODO: Find out if there are equivalent HAL functions for the
	 * readings.  For now, use the ESP-IDF functions. */
	int reading;

#ifdef CONFIG_IDF_TARGET_ESP32
	adc_hal_hall_disable();
	adc_hal_amp_disable();
#endif /* CONFIG_IDF_TARGET_ESP32 */
	adc_hal_set_controller(devconf->adc_num, ADC_CTRL_RTC);
	adc_hal_convert(devconf->adc_num, channel_id, &reading);
#if !CONFIG_IDF_TARGET_ESP32
	adc_hal_rtc_reset();
#endif /* !CONFIG_IDF_TARGET_ESP32 */

	devdata->buffer = (uint16_t *) sequence->buffer;
	devdata->buffer[channel_id] = reading;

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

	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */


/* Utility functions */

static int adc_esp32_validate_channel_id(const struct device  *dev,
					 const uint8_t         channel_id)
{
	ADC_ESP32_DECLARE_CONST_DEVCONF(dev, devconf);
	if (channel_id >= devconf->channel_count) {
		LOG_ERR("unsupported channel id '%u'", channel_id);
		return -ENOTSUP;
	}
	return 0;
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

#ifndef _SYS_INIT_LEVEL_POST_KERNEL
#	error "_SYS_INIT_LEVEL_POST_KERNEL is not defined"
#endif

#ifndef CONFIG_ADC_INIT_PRIORITY
#	error "CONFIG_ADC_INIT_PRIORITY is not defined"
#endif

DT_INST_FOREACH_STATUS_OKAY(ESP32_ADC_INIT)
