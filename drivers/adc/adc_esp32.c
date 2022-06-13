/*
 * Copyright (c) 2022 Wolter HELLMUND VEGA <wolterhv@gmx.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Zephyr - Espressif mapping
 * adc_channel_setup - adc1_config_width, adc1_config_channel_atten
 * adc_read - adc1_get_raw, adc2_get_raw
 * adc_read_async - (not implemented)
 * adc_atten_t - adc_gain
 *
 * Step 1: implement with non-hal calls.
 * Step 2: implement with hal calls.
 *
 * The channel attenuation is not completely compatible with the concept of ADC
 * gain, apparently. It seems to be something that gets passed on to the
 * hardware.
 */

/*
 * adc_stm32.c has been used as a reference for this implementation.
 */

#define DT_DRV_COMPAT espressif_esp32_adc

#include <errno.h>

#include <drivers/adc.h>

/* From espressif */
#include <hal/adc_hal.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_esp32);

#define MIN(a, b)         (((a) < (b)) ? (a) : (b))
#define MAX(a, b)         (((a) > (b)) ? (a) : (b))
#define CLIP(min, max, x) (MIN(MAX((min), (x)), (max)))

#define ADC_ESP32_RESOLUTION_OFFSET (9)

struct adc_esp32_cfg;

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

static const struct adc_driver_api api_esp32_driver_api = {
	.channel_setup = adc_esp32_channel_setup,
	.read          = adc_esp32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_esp32_read_async,
#endif /* CONFIG_ADC_ASYNC */
	.ref_internal  = 1100; /* this is the default used in
			          espressif:adc1_example_main.c */
};
/* TODO add support for the ref_internal member required for the adc_driver_api.
 * This value should probably be updated on initialisation and on calibration.
 *
 * According to espressif:adc1_example_main.c, adc2_vref_to_gpio is a function
 * used to measure the internal voltage reference.
 *
 * According to the espressif:esp_adc_cal_characterize function, the function
 * read_efuse_vref can be used to obtained a reading as well, but apparently
 * the so-called efuse is not always available, this can be checked with
 * check_efuse_vref.
 */

enum adc_esp32_devid_e {
	ADC1 = '1',
	ADC2 = '2',
	ADC_ID_INVALID
};

/* To house ESP32-specific stuff */
struct adc_esp32_devconf {
	adc_bits_width_t  width; /* adc-specific (adc1_config_width) */
	adc_atten_t       atten; /* channel-specific (adc1_config_channel_atten) */
	uint8_t channel_count; /* maps to adc1_channel_t or adc2_channel_t, must
				  be set to the relevant ADCn_CHANNEL_MAX from
				  the devicetree configuration. */
	/* From single_read/adc/adc1_example_main.c */
        /*
	 * adc_bits_width_t width = ADC_WIDTH_BIT_12;
	 * adc_channel_t channel = ADC_CHANNEL_6;
	 * adc_atten_t atten = ADC_ATTEN_DB_11;
         */
};

/* Implementation */

/*
 * Get the ADC device.
 *
 * Returns the ADC device which dev is associated to.
 */
static enum adc_esp32_devid
adc_esp32_get_devid(const struct device  *dev)
{
	/* TODO: find a better way of identifying the device that doesn't rely
	 * on strings. */
	/* dev->name is either "ADC1" or "ADC2".  To check which ADC the device
	 * corresponds to, it is only necessary to check character 3. */
	/* TODO Note that Espressif already defines an enum for this,
	 * adc_ll_num_t.  We don't currently use that one because it doesn't
	 * have an entry for an invalid ADC device. */
	switch (dev->name[3]) {
	case '1':
		return ADC1;
		break;
	case '2':
		return ADC2;
		break;
	default:
		return ADC_ID_INVALID;
		break;
	}
}

static int
adc_esp32_init(const struct device *dev)
{
	adc_hal_init();
	return 0;
}

static int
adc_esp32_channel_setup	(const struct device *dev,
			 const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_esp32_devconf  *devconf = dev->config;
	enum adc_esp32_devid             id      = adc_esp32_get_devid(dev);

	if (channel_cfg->channel_id >= devconf->channel_count) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	switch (id) {
	case ADC1:
		adc1_config_width(devconf->width);
		adc1_config_channel_atten((adc1_channel_t) channel_cfg->channel_id,
					  devconf->atten);
		break;
	case ADC2:
		/* Width / resolution for the ADC2 is configured with the
		 * adc2_get_raw function, i.e. at read-time. */
		adc2_config_channel_atten((adc2_channel_t) channel_cfg->channel_id,
					  devconf->atten);
		break;
	default:
		/* The device name is invalid for some reason. */
		return -EINVAL;
		break;
	}
	/* At some point, a "characterisation" of the ADC may be needed. In the
	 * example it is called with
	 * esp_adc_cal_characterize. */
	/* After this, all that is needed is calling adc1_get_raw or
	 * adc2_get_raw. */
	/* To convert a raw value to a physical quantity, use
	 * esp_adc_cal_raw_to_voltage. */
	/* In the adc_hal.h, the functions
	 * adc_ll_set_atten and adc_ll_rtc_enable_channel are used, which
	 * suggests that these may the HAL ways of setting up a channel. */
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
	enum adc_esp32_devid id = adc_esp32_get_devid(dev);

	if (id == ADC_ID_INVALID) {
		return -EINVAL;
	}

	/* the adc_sequence struct member "channels" is a 32-bit bitfield with
	 * the channels to get a reading for. Possibly in this function we will
	 * have a loop instead, that will get a reading for each channel. */
	/* The STM32 returns an error when multiple channels are selected, and
	 * that's how it gets away with doing just single channel reads. */
	/* TODO: Find out whether the ESP32 supports reading multiple channels
	 * at a time.  In the meantime, do what STM32 does. */
	uint8_t index = find_lsb_set(sequence->channels) - 1;
	if (sequence->channels > BIT(index)) {
		LOG_ERR("Only single channel supported");
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
	int16_t offset_resolution = (int16_t) sequence->resolution - ADC_ESP32_RESOLUTION_OFFSET;
	uint8_t esp32_resolution  = (uint8_t) CLIP((int16_t) ADC_WIDTH_BIT_9,
				                   (int16_t) SOC_ADC_MAX_BITWIDTH,
				                   (int16_t) offset_resolution);
	if (offset_resolution != esp32_resolution) {
		LOG_ERR("Resolution not supported, using nearest: %d bits",
			esp32_resolution);
	}

	if (sequence->calibrate) {
		/* TODO: Find out how to calibrate. Possibly, resolution needs
		 * to be applied before calibration. */
		LOG_ERR("Calibration not supported yet");
	}

	/* TODO: Find out if there are equivalent HAL functions for the
	 * readings.  For now, use the ESP-IDF functions. */
	int reading;
	switch (id) {
	case ADC1:
		adc1_config_width((adc_bits_width_t) esp32_resolution);
		reading = adc1_get_raw((adc1_channel_t) channel_cfg->channel_id);
		break;
	case ADC2:
		adc2_get_raw((adc2_channel_t)   channel_cfg->channel_id,
			     (adc_bits_width_t) esp32_resolution,
			     &reading);
		break;
	default:
		/* Error already handled by adc_esp32_get_devid */
		break;
	}
	sequence->buffer[chan_idx] = raw_value;

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

	/* TODO: Find out whether this is supported. */

	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */


/* Footer */

#define ADC_ESP32_CONFIG(index)						\
static const struct adc_esp32_cfg adc_esp32_cfg_##index = {		\
};

#define ESP32_ADC_INIT(index)						\
									\
PINCTRL_DT_INST_DEFINE(index);						\
									\
ADC_ESP32_CONFIG(index)							\
									\
static struct adc_esp32_data adc_esp32_data_##index = {			\
	ADC_CONTEXT_INIT_TIMER (adc_esp32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_LOCK  (adc_esp32_data_##index, ctx),		\
	ADC_CONTEXT_INIT_SYNC  (adc_esp32_data_##index, ctx),		\
};									\
									\
DEVICE_DT_INST_DEFINE(index,						\
		      &adc_esp32_init, NULL,				\
		      &adc_esp32_data_##index, &adc_esp32_cfg_##index	\
		      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,		\
		      &api_esp32_driver_api);				\

DT_INST_FOREACH_STATUS_OKAY(ESP32_ADC_INIT)
