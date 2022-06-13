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
};

enum adc_esp32_devid_e {
	ADC1 = '1',
	ADC2 = '2',
	ADC_ID_INVALID
};

/* To house ESP32-specific stuff */
struct adc_esp32_devconf {
	adc_atten_t       atten; /* adc-specific (adc1_config_width) */
	adc_bits_width_t  width; /* channel-specific (adc1_config_channel_atten) */
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

static enum adc_esp32_devid helper_get_devid(const struct device  *dev)
{
	/* TODO: find a better way of identifying the device that doesn't rely
	 * on strings. */
	/* dev->name is either "ADC1" or "ADC2".  To check which ADC the device
	 * corresponds to, it is only necessary to check character 3. */
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

static int adc_esp32_init(const struct device *dev)
{
	adc_hal_init();
	return 0;
}

static int adc_esp32_channel_setup (const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_esp32_devconf  *devconf = dev->config;
	enum adc_esp32_devid             id      = helper_get_devid(dev);

	if (channel_cfg->channel_id >= devconf->channel_count) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	switch (id) {
	case ADC1:
		adc1_config_width(devconf->width);
		adc1_config_channel_atten((adc1_channel_t) channel_cfg->channel_id,
					  devconf->atten);
		break;
	case ADC2:
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
static int adc_esp32_read          (const struct device *dev,
			            const struct adc_sequence *sequence)
{
	enum adc_esp32_devid id = helper_get_devid(dev);

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

	if (sequence->calibrate) {
		/* TODO: Find out how to calibrate. */
		adc_esp32_calibrate();
	}

	/* TODO: Find out if there are equivalent HAL functions for the
	 * readings.  For now, use the ESP-IDF functions. */
	int reading;
	switch (id) {
	case ADC1:
		reading = adc1_get_raw((adc1_channel_t) channel_cfg->channel_id);
		break;
	case ADC2:
		reading = adc2_get_raw((adc2_channel_t) channel_cfg->channel_id);
		break;
	default:
		/* Error already handled by helper_get_devid */
		break;
	}
	/* Set resolution, according to channel */
	sequence->buffer[chan_idx] = raw_value;

	return 0;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_esp32_read_async    (const struct device *dev,
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
