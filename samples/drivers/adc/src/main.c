/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <drivers/adc.h>
#include <logging/log.h>

#ifdef CONFIG_ADC_ESP32
#include <drivers/adc/adc_esp32.h>
#endif /* #ifdef CONFIG_ADC_ESP32 */

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define ADC_NUM_CHANNELS	DT_PROP_LEN(DT_PATH(zephyr_user), io_channels)

#if ADC_NUM_CHANNELS > 2
#error "Currently only 1 or 2 channels supported in this sample"
#endif

#if ADC_NUM_CHANNELS == 2 && !DT_SAME_NODE( \
	DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), io_channels, 0), \
	DT_PHANDLE_BY_IDX(DT_PATH(zephyr_user), io_channels, 1))
#error "Channels have to use the same ADC."
#endif

#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT

#ifdef CONFIG_ADC_NRFX_SAADC
#define ADC_INPUT_POS_OFFSET SAADC_CH_PSELP_PSELP_AnalogInput0
#else
#define ADC_INPUT_POS_OFFSET 0
#endif

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
LOG_MODULE_REGISTER(adc_sample);

/* Get the numbers of up to two channels */
static uint8_t channel_ids[ADC_NUM_CHANNELS] = {
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 0),
#if ADC_NUM_CHANNELS == 2
	DT_IO_CHANNELS_INPUT_BY_IDX(DT_PATH(zephyr_user), 1)
#endif
};

/* static int16_t sample_buffer[ADC_NUM_CHANNELS]; */
static int16_t sample_buffer[256];

struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	/* channel ID will be overwritten below */
	.channel_id =   0,
	.differential = 0
};

struct adc_sequence sequence = {
	/* individual channels will be added below */
	.channels    = 0,
	.buffer      = sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer),
	.resolution  = ADC_RESOLUTION,
};

void main(void)
{
	int err;
	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);

	struct adc_esp32_data *devdata =
		(struct adc_esp32_data *) dev_adc->data;

	LOG_DBG("starting example application\n");

	if (!device_is_ready(dev_adc)) {
		printk("ADC device not found\n");
		return;
	}

	LOG_DBG("you have chosen %s as your ADC", dev_adc->name);

	/*
	 * Configure channels individually prior to sampling
	 */
	for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
		channel_cfg.channel_id = channel_ids[i];
#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
		channel_cfg.input_positive = ADC_INPUT_POS_OFFSET + channel_ids[i];
#endif

		err = adc_channel_setup(dev_adc, &channel_cfg);
		if (err != 0) {
			LOG_ERR("failed setting up channel %d (err: %d)",
				channel_ids[i], err);
		} else {
			LOG_DBG("succeeded setting up channel %d", channel_ids[i]);
		}
		adc_esp32_set_atten(dev_adc, channel_ids[i], ADC_ESP32_ATTEN_3);

		/* sequence.channels |= BIT(channel_ids[i]); */
		sequence.channels = BIT(channel_ids[i]);
	}

	int32_t adc_vref = adc_ref_internal(dev_adc);
	adc_esp32_update_meas_ref_internal(dev_adc);
	adc_esp32_get_meas_ref_internal(dev_adc, (uint16_t *) &adc_vref);
	adc_esp32_characterize_by_atten(dev_adc,
					sequence.resolution,
					ADC_ESP32_ATTEN_3);

	LOG_DBG("detected internal reference voltage: %d mV", adc_vref);

	while (1) {
		/*
		 * Read sequence of channels (fails if not supported by MCU)
		 */
		err = adc_read(dev_adc, &sequence);
		if (err != 0) {
			printk("ADC reading failed with error %d.\n", err);
			return;
		}

		printk("ADC reading:");
		for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
			int32_t raw_value = sample_buffer[channel_ids[i]];

			printk(" %d", raw_value);
			if (adc_vref > 0) {
				/*
				 * Convert raw reading to millivolts if driver
				 * supports reading of ADC reference voltage
				 */
				int32_t mv_value = raw_value;

				/* adc_raw_to_millivolts(adc_vref, ADC_GAIN, */
				/* 	ADC_RESOLUTION, &mv_value); */
				/* adc_raw_to_millivolts(adc_vref, */
				/* 		      channel_cfg.gain, */
				/* 		      sequence.resolution, */
				/* 		      &mv_value); */
				adc_esp32_raw_to_millivolts(dev_adc, &mv_value);
				printk(" = %d mV  ", mv_value);
			}
		}
		printk("\n");

		k_sleep(K_MSEC(500));
	}
}
