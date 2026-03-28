/*
 * Copyright 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define TEST_SEARCH_PERIOD K_SECONDS(CONFIG_TEST_SEARCH_PERIOD)

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));
static const gnss_systems_t enabled_systems_array[] = {
	CONFIG_TEST_ENABLED_SYSTEMS_0,
	CONFIG_TEST_ENABLED_SYSTEMS_1,
	CONFIG_TEST_ENABLED_SYSTEMS_2,
	CONFIG_TEST_ENABLED_SYSTEMS_3,
};

static bool test_reported_are_expected(gnss_systems_t reported, gnss_systems_t expected)
{
	return ((~expected) & reported) == 0;
}

static void test_set_enabled_systems(gnss_systems_t enabled_systems)
{
	int ret;

	ret = gnss_set_enabled_systems(dev, enabled_systems);
	zassert_ok(ret, "failed to set enabled systems (%i)", ret);
}

static void test_get_system_enabled(gnss_systems_t expected_systems)
{
	int ret;
	gnss_systems_t enabled_systems;

	ret = gnss_get_enabled_systems(dev, &enabled_systems);
	if (ret == -ENOSYS) {
		return;
	}
	zassert_ok(ret, "failed to get enabled systems (%i)", ret);

	zassert_equal(enabled_systems, expected_systems,
		      "invalid enabled systems (%u != %u)",
		      enabled_systems, expected_systems);
}

#ifdef CONFIG_GNSS_SATELLITES
static atomic_t reported_systems_atom = ATOMIC_INIT(0);

static void gnss_satellites_cb(const struct device *dev,
			       const struct gnss_satellite *satellites,
			       uint16_t size)
{
	for (uint16_t i = 0; i < size; i++) {
		atomic_or(&reported_systems_atom, satellites[i].system);
	}
}

GNSS_SATELLITES_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_satellites_cb);

static void test_validate_satellites(gnss_systems_t expected_systems)
{
	gnss_systems_t reported_systems;
	bool expected;

	atomic_set(&reported_systems_atom, 0);
	PRINT("searching with enabled system %u\n", expected_systems);
	k_sleep(TEST_SEARCH_PERIOD);

	reported_systems = atomic_get(&reported_systems_atom);
	if (reported_systems == 0) {
		PRINT("found no satellites\n");
	} else {
		PRINT("found satellites\n");
	}

	expected = test_reported_are_expected(reported_systems, expected_systems);
	zassert_true(expected, "unexpected systems reported (%u != %u)",
		     reported_systems, expected_systems);
}
#endif

static void test_validate_enabled_systems(void)
{
	gnss_systems_t enabled_systems;

	for (uint8_t i = 0; i < ARRAY_SIZE(enabled_systems_array); i++) {
		enabled_systems = enabled_systems_array[i];
		if (enabled_systems == 0) {
			continue;
		}

		test_set_enabled_systems(enabled_systems);
		test_get_system_enabled(enabled_systems);
#ifdef CONFIG_GNSS_SATELLITES
		test_validate_satellites(enabled_systems);
#endif
	}
}

static bool test_all_enabled_systems_are_disabled(void)
{
	gnss_systems_t enabled_systems;

	for (uint8_t i = 0; i < ARRAY_SIZE(enabled_systems_array); i++) {
		enabled_systems = enabled_systems_array[i];
		if (enabled_systems != 0) {
			return false;
		}
	}

	return true;
}

static void test_validate_supported_systems(void)
{
	gnss_systems_t supported_systems;
	int ret;
	gnss_systems_t enabled_systems;
	bool supported;

	ret = gnss_get_supported_systems(dev, &supported_systems);
	if (ret == -ENOSYS) {
		return;
	}
	zassert_ok(ret, "failed to get supported systems (%i)", ret);

	for (uint8_t i = 0; i < ARRAY_SIZE(enabled_systems_array); i++) {
		enabled_systems = enabled_systems_array[0];
		supported = test_reported_are_expected(enabled_systems, supported_systems);
		zassert_true(supported, "enabled systems %u is not supported", i);
	}
}

ZTEST(gnss_api, test_enabled_systems)
{
	if (test_all_enabled_systems_are_disabled()) {
		ztest_test_skip();
	}

	test_validate_supported_systems();
	test_validate_enabled_systems();
}
