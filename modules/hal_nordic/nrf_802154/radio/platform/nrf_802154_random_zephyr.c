/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <platform/temperature/nrf_802154_temperature.h>
#include <drivers/entropy.h>

static uint32_t state;

static uint32_t next(void)
{
	uint32_t num = state;

	state = 1664525 * num + 1013904223;
	return num;
}

void nrf_802154_random_init(void)
{
	const struct device *dev;
	int err;

	dev = device_get_binding(DT_CHOSEN_ZEPHYR_ENTROPY_LABEL);
	__ASSERT_NO_MSG(dev != NULL);

	do {
		err = entropy_get_entropy(dev, (uint8_t *)&state, sizeof(state));
		__ASSERT_NO_MSG(err == 0);
	} while (state == 0);
}

void nrf_802154_random_deinit(void)
{
	/* Intentionally empty */
}

uint32_t nrf_802154_random_get(void)
{
	return next();
}
