/*
 * Copyright 2023 Cirrus Logic, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

struct sbs_charger_fixture {
	const struct device *dev;
	const struct charger_driver_api *api;
};

static void *sbs_charger_setup(void)
{
	static ZTEST_DMEM struct sbs_charger_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(sbs_sbs_charger);

	k_object_access_all_grant(fixture.dev);

	zassert_true(device_is_ready(fixture.dev), "Charger not found");

	return &fixture;
}

ZTEST_USER_F(sbs_charger, test_get_prop_failed_returns_negative)
{
	/* Grab a bogus property */
	charger_prop_t prop = CHARGER_PROP_MAX;
	union charger_propval val = {0};

	int ret = charger_get_prop(fixture->dev, prop, &val);

	zassert_equal(ret, -ENOTSUP, "Getting bad property %d has a good status.", prop);
}

ZTEST_USER_F(sbs_charger, test_get_prop_success_returns_zero)
{
	/* Validate what props are supported by the driver */
	charger_prop_t prop = CHARGER_PROP_ONLINE;
	union charger_propval val = {0};

	int ret = charger_get_prop(fixture->dev, prop, &val);

	zassert_equal(ret, 0, "Getting good property %d has a good status.", prop);
}

ZTEST_USER_F(sbs_charger, test_set_prop_failed_returns_negative)
{
	/* Set a bogus property */
	charger_prop_t prop = CHARGER_PROP_MAX;
	union charger_propval val = {0};

	int ret = charger_set_prop(fixture->dev, prop, &val);

	zassert_equal(ret, -ENOTSUP, "Setting bad property %d has a good status.", prop);
}

ZTEST_USER_F(sbs_charger, test_charge_enable_success_returns_zero)
{
	int ret = charger_charge_enable(fixture->dev, true);

	zassert_equal(ret, 0, "Enabling charge has a good status.");
}

ZTEST_SUITE(sbs_charger, NULL, sbs_charger_setup, NULL, NULL, NULL);
