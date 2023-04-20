/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *   This file implements the thermometer abstraction that uses Zephyr sensor
 *   API for the die thermometer.
 *
 */

#include "platform/nrf_802154_temperature.h"

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/** @brief Default temperature [C] reported if NRF_802154_TEMPERATURE_UPDATE is disabled. */
#define DEFAULT_TEMPERATURE 20

#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE_NAME nrf_802154_temperature
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static int8_t value = DEFAULT_TEMPERATURE;


#if defined(CONFIG_NRF_802154_TEMPERATURE_UPDATE)

static const struct device *const device = DEVICE_DT_GET(DT_NODELABEL(temp));
static struct k_work_delayable dwork;

static void work_handler(struct k_work *work)
{
	struct sensor_value val;
	int err;

	err = sensor_sample_fetch(device);
	if (!err) {
		err = sensor_channel_get(device, SENSOR_CHAN_DIE_TEMP, &val);
	}

	if (!err && (value != val.val1)) {
		value = val.val1;

		nrf_802154_temperature_changed();
	}

	k_work_reschedule(&dwork, K_MSEC(CONFIG_NRF_802154_TEMPERATURE_UPDATE_PERIOD));
}

static int temperature_update_init(void)
{

	__ASSERT_NO_MSG(device_is_ready(device));

	k_work_init_delayable(&dwork, work_handler);
	k_work_schedule(&dwork, K_NO_WAIT);

	return 0;
}

SYS_INIT(temperature_update_init, POST_KERNEL, CONFIG_NRF_802154_TEMPERATURE_UPDATE_INIT_PRIO);
BUILD_ASSERT(CONFIG_SENSOR_INIT_PRIORITY < CONFIG_NRF_802154_TEMPERATURE_UPDATE_INIT_PRIO,
	     "CONFIG_SENSOR_INIT_PRIORITY must be lower than CONFIG_NRF_802154_TEMPERATURE_UPDATE_INIT_PRIO");
#endif /* defined(CONFIG_NRF_802154_TEMPERATURE_UPDATE) */

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
	return value;
}
