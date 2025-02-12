/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/emul_fuel_gauge.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include "test_sbs_gauge.h"

ZTEST_F(sbs_gauge_new_api, test_cutoff_disabled)
{
	bool is_cutoff;

	/* Initially there should be no cutoff */
	zassert_ok(emul_fuel_gauge_is_battery_cutoff(fixture->sbs_fuel_gauge, &is_cutoff));
	zassert_false(is_cutoff);

	zassert_not_equal(fuel_gauge_battery_cutoff(fixture->dev), 0);

	/* We confirm there was no cutoff */
	zassert_ok(emul_fuel_gauge_is_battery_cutoff(fixture->sbs_fuel_gauge, &is_cutoff));
	zassert_false(is_cutoff);
}
