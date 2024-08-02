/*
 * Copyright (c) 2022  The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_VBUS_ADC_PRIV_H_
#define ZEPHYR_DRIVERS_USBC_VBUS_ADC_PRIV_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>

/**
 * @brief Driver config
 */
struct usbc_vbus_config {
	uint32_t output_ohm;
	uint32_t full_ohm;
	struct adc_dt_spec adc_channel;
	const struct gpio_dt_spec power_gpios;
	const struct gpio_dt_spec discharge_gpios;
};

/**
 * @brief Driver data
 */
struct usbc_vbus_data {
	int sample;
	struct adc_sequence sequence;
};

#endif /* ZEPHYR_DRIVERS_USBC_VBUS_ADC_PRIV_H_ */
