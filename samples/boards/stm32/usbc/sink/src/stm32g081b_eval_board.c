/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(board, LOG_LEVEL_DBG);

#define VBUS DT_PATH(vbus)
#define DISCHARGE_VBUS_NODE    DT_PATH(discharge_vbus2_config, discharge_vbus2)
#define LED0_NODE              DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec discharge_vbus = GPIO_DT_SPEC_GET(DISCHARGE_VBUS_NODE, gpios);

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION          12
#define ADC_GAIN                ADC_GAIN_1
#define ADC_REFERENCE           ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME    ADC_ACQ_TIME_DEFAULT
#define ADC_REF_MV              3300

static const struct device *dev_adc;
static int16_t sample_buffer;

static const uint32_t output_ohm = DT_PROP(VBUS, output_ohms);
static const uint32_t full_ohm = DT_PROP(VBUS, full_ohms);

struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.differential = 0
};

struct adc_sequence sequence = {
	.channels = BIT(3),
	.buffer = &sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer),
	.resolution = ADC_RESOLUTION,
};

/**
 * @brief This function is used to measure the VBUS in millivolts
 */
int board_vbus_meas(int *mv)
{
	int32_t value;
	int ret;

	ret = adc_read(dev_adc, &sequence);
	if (ret != 0) {
		printk("ADC reading failed with error %d.\n", ret);
		return ret;
	}

	value = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, &value);
	if (ret != 0) {
		printk("Scaling ADC failed with error %d.\n", ret);
		return ret;
	}

	/* VBUS is scaled down though a voltage divider */
	*mv = (value * 1000) / ((output_ohm * 1000) / full_ohm);

	return 0;
}

/**
 * @brief This function discharges VBUS
 */
int board_discharge_vbus(int en)
{
	gpio_pin_set_dt(&discharge_vbus, en);

	return 0;
}

void board_led(int val)
{
	gpio_pin_set_dt(&led, val);
}

int board_config(void)
{
	int ret;

	dev_adc = DEVICE_DT_GET(DT_IO_CHANNELS_CTLR(VBUS));
	if (!device_is_ready(dev_adc)) {
		printk("ADC device not found\n");
		return -ENODEV;
	}

	if (!device_is_ready(led.port)) {
		LOG_ERR("LED device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(discharge_vbus.port)) {
		LOG_ERR("Discharge VBUS device not ready");
		return -ENODEV;
	}

	adc_channel_setup(dev_adc, &channel_cfg);

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		LOG_ERR("Could not configure LED pin");
		return ret;
	}

	ret = gpio_pin_configure_dt(&discharge_vbus, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		LOG_ERR("Could not configure VBUS Discharge pin");
		return ret;
	}

	return 0;
}
