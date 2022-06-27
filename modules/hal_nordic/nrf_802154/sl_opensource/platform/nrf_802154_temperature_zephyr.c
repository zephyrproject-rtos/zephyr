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

/** @brief Structure describing the temperature measurement module. */
struct temperature {
	const struct device *dev;	/**< Temperature device pointer. */
	int8_t value;			/**< Last temperature measurement in degree Celsius. */

	struct k_work_delayable work;
};

static struct temperature temperature = {
	.value = DEFAULT_TEMPERATURE,
	.dev = DEVICE_DT_GET(DT_NODELABEL(temp))
};

#if defined(CONFIG_NRF_802154_TEMPERATURE_UPDATE)
static void work_handler(struct k_work *work)
{
	struct sensor_value val;
	int err;

	err = sensor_sample_fetch(temperature.dev);
	if (!err) {
		err = sensor_channel_get(temperature.dev, SENSOR_CHAN_DIE_TEMP, &val);
	}

	if (!err && (temperature.value != val.val1)) {
		temperature.value = val.val1;

		nrf_802154_temperature_changed();
	}

	k_work_reschedule(&temperature.work, K_MSEC(CONFIG_NRF_802154_TEMPERATURE_UPDATE_PERIOD));
}

static int temperature_update_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	__ASSERT_NO_MSG(device_is_ready(temperature.dev));

	k_work_init_delayable(&temperature.work, work_handler);
	k_work_schedule(&temperature.work, K_NO_WAIT);

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
	return temperature.value;
}
