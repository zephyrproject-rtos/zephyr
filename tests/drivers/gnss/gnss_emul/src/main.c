/*
 * Copyright 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_emul.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

static void gnss_data_callback(const struct device *dev, const struct gnss_data *data);

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_callback);
static K_SEM_DEFINE(gnss_data_published, 0, 1);
static struct gnss_data gnss_published_data;

static void expected_pm_state(const struct device *dev, enum pm_device_state expected)
{
	enum pm_device_state state;

	zassert_equal(0, pm_device_state_get(dev, &state));
	zassert_equal(expected, state);
}

static void gnss_data_callback(const struct device *dev, const struct gnss_data *data)
{
	gnss_published_data = *data;
	k_sem_give(&gnss_data_published);
}

static void print_time(const struct gnss_time *utc)
{
	printk("TIME: %02d/%02d/%02d %02d:%02d:%02d.%03d\n", utc->century_year, utc->month,
	       utc->month_day, utc->hour, utc->minute, utc->millisecond / 1000,
	       utc->millisecond % 1000);
}

ZTEST(gnss_emul, test_config_functions)
{
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));
	enum gnss_navigation_mode mode;
	gnss_systems_t systems;
	uint32_t fix_rate;

	/* Booted into suspend mode */
	expected_pm_state(dev, PM_DEVICE_STATE_SUSPENDED);

	/* Configuration get API functions fail when suspended */
	zassert_equal(-ENODEV, gnss_get_enabled_systems(dev, &systems));
	zassert_equal(-ENODEV, gnss_get_navigation_mode(dev, &mode));
	zassert_equal(-ENODEV, gnss_get_fix_rate(dev, &fix_rate));

	/* Configuration can be queried when enabled */
	zassert_equal(0, pm_device_runtime_get(dev));
	zassert_equal(0, gnss_set_enabled_systems(dev, GNSS_SYSTEM_GPS | GNSS_SYSTEM_GALILEO));
	zassert_equal(0, gnss_set_navigation_mode(dev, GNSS_NAVIGATION_MODE_HIGH_DYNAMICS));
	zassert_equal(0, gnss_set_fix_rate(dev, 1500));

	zassert_equal(0, gnss_get_enabled_systems(dev, &systems));
	zassert_equal(0, gnss_get_navigation_mode(dev, &mode));
	zassert_equal(0, gnss_get_fix_rate(dev, &fix_rate));
	zassert_equal(GNSS_SYSTEM_GPS | GNSS_SYSTEM_GALILEO, systems);
	zassert_equal(GNSS_NAVIGATION_MODE_HIGH_DYNAMICS, mode);
	zassert_equal(1500, fix_rate);

	zassert_equal(0, pm_device_runtime_put(dev));

	/* Fails again when suspended */
	zassert_equal(-ENODEV, gnss_get_enabled_systems(dev, &systems));
	zassert_equal(-ENODEV, gnss_get_navigation_mode(dev, &mode));
	zassert_equal(-ENODEV, gnss_get_fix_rate(dev, &fix_rate));

	/* But escape hatches work */
	systems = 0;
	mode = 0;
	fix_rate = 0;
	zassert_equal(0, gnss_emul_get_enabled_systems(dev, &systems));
	zassert_equal(0, gnss_emul_get_navigation_mode(dev, &mode));
	zassert_equal(0, gnss_emul_get_fix_rate(dev, &fix_rate));
	zassert_equal(GNSS_SYSTEM_GPS | GNSS_SYSTEM_GALILEO, systems);
	zassert_equal(GNSS_NAVIGATION_MODE_HIGH_DYNAMICS, mode);
	zassert_equal(1500, fix_rate);
}

ZTEST(gnss_emul, test_callback_behaviour)
{
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));
	const struct navigation_data nav = {
		.latitude = 150000000000,
		.longitude = -15199000000,
		.altitude = 123456,
	};
	const struct gnss_info info = {
		.satellites_cnt = 7,
		.hdop = 1999,
		.geoid_separation = 1000,
		.fix_status = GNSS_FIX_STATUS_GNSS_FIX,
		.fix_quality = GNSS_FIX_QUALITY_GNSS_SPS,
	};
	const struct gnss_time *utc;
	uint32_t timestamp;

	/* Booted into suspend mode */
	expected_pm_state(dev, PM_DEVICE_STATE_SUSPENDED);

	/* No data published while suspended */
	zassert_equal(-EAGAIN, k_sem_take(&gnss_data_published, K_SECONDS(5)));

	/* Power up and configure for 1Hz */
	zassert_equal(0, pm_device_runtime_get(dev));
	zassert_equal(0, gnss_set_fix_rate(dev, 1000));
	timestamp = k_uptime_get_32();

	/* Monitor data for a while */
	for (int i = 0; i < 10; i++) {
		zassert_equal(0, k_sem_take(&gnss_data_published, K_MSEC(1100)));
		zassert_equal(0, gnss_published_data.nav_data.latitude);
		zassert_equal(0, gnss_published_data.nav_data.longitude);
		zassert_equal(0, gnss_published_data.nav_data.altitude);
		zassert_equal(0, gnss_published_data.info.satellites_cnt);
		print_time(&gnss_published_data.utc);
	}

	/* Set a location, approximately 14th July 2017, 02:40:xx am */
	gnss_emul_set_data(dev, &nav, &info, 1500000000000LL);
	for (int i = 0; i < 3; i++) {
		utc = &gnss_published_data.utc;
		/* Published data should match that configured */
		zassert_equal(0, k_sem_take(&gnss_data_published, K_MSEC(1100)));
		zassert_mem_equal(&gnss_published_data.nav_data, &nav, sizeof(nav));
		zassert_mem_equal(&gnss_published_data.info, &info, sizeof(info));
		zassert_equal(17, utc->century_year);
		zassert_equal(7, utc->month);
		zassert_equal(14, utc->month_day);
		zassert_equal(2, utc->hour);
		zassert_equal(40, utc->minute);
		print_time(&gnss_published_data.utc);
	}

	/* Reset back to no location */
	gnss_emul_clear_data(dev);
	for (int i = 0; i < 5; i++) {
		zassert_equal(0, k_sem_take(&gnss_data_published, K_MSEC(1100)));
		zassert_equal(0, gnss_published_data.nav_data.latitude);
		zassert_equal(0, gnss_published_data.nav_data.longitude);
		zassert_equal(0, gnss_published_data.nav_data.altitude);
		zassert_equal(0, gnss_published_data.info.satellites_cnt);
		print_time(&gnss_published_data.utc);
	}

	/* Once again no callbacks once suspended */
	zassert_equal(0, pm_device_runtime_put(dev));
	zassert_equal(-EAGAIN, k_sem_take(&gnss_data_published, K_SECONDS(5)));
}

ZTEST_SUITE(gnss_emul, NULL, NULL, NULL, NULL, NULL);
