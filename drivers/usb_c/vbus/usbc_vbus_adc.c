/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usb_c_vbus_adc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbc_vbus_adc, CONFIG_USBC_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/usb_c/usbc_pd.h>
#include <zephyr/drivers/usb_c/usbc_vbus.h>
#include <soc.h>
#include <stddef.h>

#include "usbc_vbus_adc_priv.h"

/**
 * @brief Reads and returns VBUS measured in mV
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int adc_vbus_measure(const struct device *dev, int *meas)
{
	const struct usbc_vbus_config *const config = dev->config;
	struct usbc_vbus_data *data = dev->data;
	int value;
	int ret;

	__ASSERT(meas != NULL, "ADC VBUS meas must not be NULL");

	ret = adc_read(config->adc_channel.dev, &data->sequence);
	if (ret != 0) {
		LOG_INF("ADC reading failed with error %d.", ret);
		return ret;
	}

	value = data->sample;
	ret = adc_raw_to_millivolts_dt(&config->adc_channel, &value);
	if (ret != 0) {
		LOG_INF("Scaling ADC failed with error %d.", ret);
		return ret;
	}

	if (config->full_ohm > 0) {
		/* VBUS is scaled down though a voltage divider */
		value = (value * 1000) / ((config->output_ohm * 1000) / config->full_ohm);
	}
	*meas = value;

	return 0;
}

/**
 * @brief Checks if VBUS is at a particular level
 *
 * @retval true if VBUS is at the level voltage, else false
 */
static bool adc_vbus_check_level(const struct device *dev,
				 enum tc_vbus_level level)
{
	int meas;
	int ret;

	ret = adc_vbus_measure(dev, &meas);
	if (ret) {
		return false;
	}

	switch (level) {
	case TC_VBUS_SAFE0V:
		return (meas < PD_V_SAFE_0V_MAX_MV);
	case TC_VBUS_PRESENT:
		return (meas >= PD_V_SAFE_5V_MIN_MV);
	case TC_VBUS_REMOVED:
		return (meas < TC_V_SINK_DISCONNECT_MAX_MV);
	}

	return false;
}

/**
 * @brief Sets pin to discharge VBUS
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOENT if enable pin isn't defined
 */
static int adc_vbus_discharge(const struct device *dev,
			      bool enable)
{
	const struct usbc_vbus_config *const config = dev->config;
	const struct gpio_dt_spec *gcd = &config->discharge_gpios;
	int ret = -ENOENT;

	if (gcd->port) {
		ret = gpio_pin_set_dt(gcd, enable);
	}
	return ret;
}

/**
 * @brief Sets pin to enable VBUS measurments
 *
 * @retval 0 on success
 * @retval -EIO on failure
 * @retval -ENOENT if enable pin isn't defined
 */
static int adc_vbus_enable(const struct device *dev,
			   bool enable)
{
	const struct usbc_vbus_config *const config = dev->config;
	const struct gpio_dt_spec *gcp = &config->power_gpios;
	int ret = -ENOENT;

	if (gcp->port) {
		ret = gpio_pin_set_dt(gcp, enable);
	}
	return ret;
}

/**
 * @brief Initializes the ADC VBUS Driver
 *
 * @retval 0 on success
 * @retval -EIO on failure
 */
static int adc_vbus_init(const struct device *dev)
{
	const struct usbc_vbus_config *const config = dev->config;
	struct usbc_vbus_data *data = dev->data;
	const struct gpio_dt_spec *gcp = &config->power_gpios;
	const struct gpio_dt_spec *gcd = &config->discharge_gpios;
	int ret;

	if (!adc_is_ready_dt(&config->adc_channel)) {
		LOG_ERR("ADC controller device is not ready");
		return -ENODEV;
	}

	/* Configure VBUS Measurement enable pin if defined */
	if (gcp->port) {
		if (!device_is_ready(gcp->port)) {
			LOG_ERR("%s: device not ready", gcp->port->name);
			return -EIO;
		}
		ret = gpio_pin_configure_dt(gcp, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gcp->port->name, gcp->pin, ret);
			return ret;
		}
	}

	/* Configure VBUS Discharge pin if defined */
	if (gcd->port) {
		if (!device_is_ready(gcd->port)) {
			LOG_ERR("%s: device not ready", gcd->port->name);
			return -EIO;
		}
		ret = gpio_pin_configure_dt(gcd, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				gcd->port->name, gcd->pin, ret);
			return ret;
		}

	}

	data->sequence.buffer = &data->sample;
	data->sequence.buffer_size = sizeof(data->sample);

	ret = adc_channel_setup_dt(&config->adc_channel);
	if (ret != 0) {
		LOG_INF("Could not setup channel (%d)\n", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->adc_channel, &data->sequence);
	if (ret != 0) {
		LOG_INF("Could not init sequence (%d)\n", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(usbc_vbus, driver_api) = {
	.measure = adc_vbus_measure,
	.check_level = adc_vbus_check_level,
	.discharge = adc_vbus_discharge,
	.enable = adc_vbus_enable
};

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0,
	     "No compatible USB-C VBUS Measurement instance found");

#define DRIVER_INIT(inst)								\
	static struct usbc_vbus_data drv_data_##inst;					\
	static const struct usbc_vbus_config drv_config_##inst = {			\
		.output_ohm = DT_INST_PROP(inst, output_ohms),				\
		.full_ohm = DT_INST_PROP_OR(inst, full_ohms, 0),			\
		.adc_channel = ADC_DT_SPEC_INST_GET(inst),				\
		.discharge_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, discharge_gpios, {}), \
		.power_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {}),		\
	};										\
	DEVICE_DT_INST_DEFINE(inst,							\
			      &adc_vbus_init,						\
			      NULL,							\
			      &drv_data_##inst,						\
			      &drv_config_##inst,					\
			      POST_KERNEL,						\
			      CONFIG_USBC_VBUS_INIT_PRIORITY,				\
			      &driver_api);

DT_INST_FOREACH_STATUS_OKAY(DRIVER_INIT)
