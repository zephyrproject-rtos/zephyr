/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <hal/nrf_saadc.h>
#include <drivers/adc.h>
#include <string.h>
#include <stdio.h>

static struct device *adc_dev;
static s32_t adc_buffer;

int adc_init(void)
{
	adc_dev = device_get_binding(DT_ADC_0_NAME);

	if (adc_dev == NULL) {
		printk("Could not get ADC device\n");
		return -EIO;
	}

	/* for details refer to zephyr/drivers/adc/adc_nrfx_saadc.c */
	const struct adc_channel_cfg ch_cfg = {
		.channel_id = 0,
		.differential = 0,
		.input_positive =
			NRF_SAADC_INPUT_VDD,    /* select VDD as ADC source */
		.input_negative = 0,
		.reference = ADC_REF_INTERNAL,  /* vRef=0.6v */
		.gain = ADC_GAIN_1_6,           /* set 1/6 because of vRef */
		.acquisition_time = ADC_ACQ_TIME_DEFAULT
	};

	int err = adc_channel_setup(adc_dev, &ch_cfg);

	if (err != 0) {
		printk("ADC setup failed: %d", err);
	}
	return err;
}

void battery_requestMeasurements(void)
{
	const static struct adc_sequence seq = { .options = NULL,
						 .channels = BIT(0),
						 .buffer = &adc_buffer,
						 .buffer_size =
							 sizeof(adc_buffer),
						 .resolution = 12,
						 .oversampling = 8,
						 .calibrate = 1 };
	int err = adc_read(adc_dev, &seq);

	if (err != 0) {
		printk("ADC read fail: %d\n", err);
	}
}

void main(void)
{
	printk("Adc batterty sample\n");
	adc_init();
	while (true) {
		battery_requestMeasurements();
		printk("ADC value: %d mV\n", (int)adc_buffer);
		k_sleep(K_SECONDS(2));
		/*
		 * If you want to convert mV into % of CR2032 battery you can
		 * search for function battery_level_in_percent inside
		 * Nordic SDK (in nRF5_SDK_15.2.0_9412b96 it's located in file
		 * components/libraries/util/app_util.h) then you can call it
		 * like
		 * u8_t battLevel = battery_level_in_percent(adc_buffer)
		 */
	}
}
