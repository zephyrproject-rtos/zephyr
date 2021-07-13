/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements the thermometer abstraction that uses Zephyr sensor
 *   API for the die termomether.
 *
 */

#include "platform/nrf_802154_temperature.h"

#include <device.h>
#include <drivers/sensor.h>
#include <kernel.h>
#include <logging/log.h>
#include <stdbool.h>

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME nrf_802154_temperature
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/** @brief Value of the last temperature measurement in degree Celsius. */
static int8_t temperature;

void nrf_802154_temperature_init(void)
{
	/* Intentionally empty. */
}

void nrf_802154_temperature_deinit(void)
{
	/* Intentionally empty. */
}

int8_t nrf_802154_temperature_get(void)
{
	return temperature;
}

/** @brief Thread handler for temperature update. */
static void temperature_update_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	static struct sensor_value  temperature_value;
	static const struct device *temperature_dev;

	temperature_dev = DEVICE_DT_GET(DT_NODELABEL(temp));

	if (!device_is_ready(temperature_dev)) {
		LOG_ERR("Setup of temperature sensor for nRF 802.15.4 driver failed");
		__ASSERT_NO_MSG(false);
	}

	while (true) {
		sensor_sample_fetch(temperature_dev);
		sensor_channel_get(temperature_dev, SENSOR_CHAN_DIE_TEMP, &temperature_value);

		if (temperature != temperature_value.val1) {
			temperature = temperature_value.val1;

			nrf_802154_temperature_changed();
		}

		k_sleep(K_MSEC(CONFIG_NRF_802154_TEMPERATURE_UPDATE_PERIOD));
	}
}

K_THREAD_DEFINE(temperature_update_tid,
		CONFIG_NRF_802154_TEMPERATURE_UPDATE_STACK_SIZE,
		temperature_update_thread,
		NULL,
		NULL,
		NULL,
		CONFIG_NRF_802154_TEMPERATURE_UPDATE_PRIO,
		0,
		0);
