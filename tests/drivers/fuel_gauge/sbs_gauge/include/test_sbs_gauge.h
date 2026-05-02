/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>

struct sbs_gauge_new_api_fixture {
	const struct device *dev;
	const struct emul *sbs_fuel_gauge;
	const struct fuel_gauge_driver_api *api;
};
