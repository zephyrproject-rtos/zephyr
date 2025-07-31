/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_sample, LOG_LEVEL_INF);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#define ADC_NODE		DT_NODELABEL(adc0)
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_2ND_CHANNEL_ID	4
#define ADC_3RD_CHANNEL_ID	5
#else
#define ADC_NODE		DT_PHANDLE(DT_PATH(zephyr_user), io_channels)
#define ADC_RESOLUTION		DT_PROP(DT_PATH(zephyr_user), io_channels_resolution)
#define ADC_GAIN		DT_PROP(DT_PATH(zephyr_user), io_channels_gain)
#define ADC_REFERENCE		DT_PROP(DT_PATH(zephyr_user), io_channels_reference)
#define ADC_ACQUISITION_TIME	DT_PROP(DT_PATH(zephyr_user), io_channels_acquisition_time)
#define ADC_1ST_CHANNEL_ID	DT_PROP(DT_PATH(zephyr_user), io_channels_input_positive)
#endif

static const struct device *adc_dev = DEVICE_DT_GET(ADC_NODE);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static int16_t sample_buffer[3];

static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.differential = 0,
};

static struct adc_sequence sequence = {
	.buffer = sample_buffer,
	.buffer_size = sizeof(sample_buffer),
	.resolution = ADC_RESOLUTION,
};

static int adc_sample_channel(uint8_t channel)
{
	int ret;

	channel_cfg.channel_id = channel;

	ret = adc_channel_setup(adc_dev, &channel_cfg);
	if (ret) {
		LOG_ERR("Setting up ADC channel %d failed with code %d", channel, ret);
		return ret;
	}

	sequence.channels = BIT(channel);

	ret = adc_read(adc_dev, &sequence);
	if (ret) {
		LOG_ERR("ADC reading failed with code %d", ret);
		return ret;
	}

	return 0;
}

int main(void)
{
	int ret;
	int32_t mv_value;

	LOG_INF("ADC Sample Application");
	LOG_INF("Board: %s", CONFIG_BOARD);

	/* Configure LED */
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure LED pin");
		return ret;
	}

	/* Check if ADC is ready */
	if (!device_is_ready(adc_dev)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	LOG_INF("ADC device is ready");

	/* Configure ADC channels */
	LOG_INF("Configuring ADC channels...");
	
	/* Main loop */
	while (1) {
		/* Toggle LED to show we're alive */
		gpio_pin_toggle_dt(&led);

		LOG_INF("ADC sampling:");

		/* Sample channel 0 */
		ret = adc_sample_channel(ADC_1ST_CHANNEL_ID);
		if (ret == 0) {
			mv_value = sample_buffer[0];
			LOG_INF("- Channel %d: %d raw", ADC_1ST_CHANNEL_ID, mv_value);
			
			/* Convert to millivolts (assuming 3.3V reference) */
			mv_value = (mv_value * 3300) >> ADC_RESOLUTION;
			LOG_INF("  %d mV", mv_value);
		}

		/* Sample channel 4 */
		ret = adc_sample_channel(ADC_2ND_CHANNEL_ID);
		if (ret == 0) {
			mv_value = sample_buffer[0];
			LOG_INF("- Channel %d: %d raw", ADC_2ND_CHANNEL_ID, mv_value);
			
			/* Convert to millivolts */
			mv_value = (mv_value * 3300) >> ADC_RESOLUTION;
			LOG_INF("  %d mV", mv_value);
		}

		/* Sample channel 5 */
		ret = adc_sample_channel(ADC_3RD_CHANNEL_ID);
		if (ret == 0) {
			mv_value = sample_buffer[0];
			LOG_INF("- Channel %d: %d raw", ADC_3RD_CHANNEL_ID, mv_value);
			
			/* Convert to millivolts */
			mv_value = (mv_value * 3300) >> ADC_RESOLUTION;
			LOG_INF("  %d mV", mv_value);
		}

		LOG_INF("---");
		k_sleep(K_SECONDS(2));
	}

	return 0;
}