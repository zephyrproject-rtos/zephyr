/*
 * Copyright 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>

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

#if IS_ENABLED(CONFIG_GNSS_ACCURACY)
static K_SEM_DEFINE(gnss_accuracy_published, 0, 1);
static struct gnss_accuracy gnss_published_accuracy;
static uint32_t gnss_accuracy_publish_count;

static void gnss_accuracy_callback(const struct device *dev,
				   const struct gnss_accuracy *accuracy)
{
	ARG_UNUSED(dev);
	gnss_published_accuracy = *accuracy;
	gnss_accuracy_publish_count++;
	k_sem_give(&gnss_accuracy_published);
}

GNSS_ACCURACY_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_accuracy_callback);
#endif /* CONFIG_GNSS_ACCURACY */

#if IS_ENABLED(CONFIG_GNSS_RAW_NMEA)
#define RAW_CAPTURE_MAX 128
static K_SEM_DEFINE(gnss_raw_published, 0, 1);
static uint8_t gnss_published_raw[RAW_CAPTURE_MAX];
static size_t gnss_published_raw_len;
static uint32_t gnss_raw_publish_count;

static void gnss_raw_callback(const struct device *dev, const char *sentence, size_t len)
{
	ARG_UNUSED(dev);
	gnss_published_raw_len = MIN(len, sizeof(gnss_published_raw));
	memcpy(gnss_published_raw, sentence, gnss_published_raw_len);
	gnss_raw_publish_count++;
	k_sem_give(&gnss_raw_published);
}

GNSS_RAW_NMEA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_raw_callback);
#endif /* CONFIG_GNSS_RAW_NMEA */

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

#if IS_ENABLED(CONFIG_GNSS_ACCURACY)
ZTEST(gnss_emul, test_accuracy_callback)
{
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));
	const struct gnss_accuracy injected = {
		.utc_ms = 66101000,
		.rms_total_mm = 15500,
		.err_ellipse_major_mm = 15300,
		.err_ellipse_minor_mm = 7200,
		.err_ellipse_orientation_cdeg = 2180,
		.lat_err_mm = 900,
		.lon_err_mm = 500,
		.alt_err_mm = 800,
	};

	expected_pm_state(dev, PM_DEVICE_STATE_SUSPENDED);

	zassert_equal(0, pm_device_runtime_get(dev));
	zassert_equal(0, gnss_set_fix_rate(dev, 1000));

	/* No accuracy is published until the test calls set_accuracy. */
	gnss_accuracy_publish_count = 0;
	k_sem_reset(&gnss_accuracy_published);

	/* Sanity: a fix epoch arrives but no accuracy event yet */
	zassert_equal(0, k_sem_take(&gnss_data_published, K_MSEC(1500)));
	zassert_equal(-EAGAIN, k_sem_take(&gnss_accuracy_published, K_MSEC(100)),
		      "accuracy must NOT publish before set_accuracy is called");

	/* Inject and verify the next epoch publishes the cached values */
	gnss_emul_set_accuracy(dev, &injected);
	zassert_equal(0, k_sem_take(&gnss_accuracy_published, K_MSEC(1500)));
	zassert_mem_equal(&gnss_published_accuracy, &injected, sizeof(injected));

	/* Sticky: a second epoch (no new set_accuracy) re-publishes the same
	 * values — matches the "chip emits GST every fix" semantics. */
	zassert_equal(0, k_sem_take(&gnss_accuracy_published, K_MSEC(1500)));
	zassert_mem_equal(&gnss_published_accuracy, &injected, sizeof(injected));
	zassert_true(gnss_accuracy_publish_count >= 2,
		     "sticky cache should keep publishing on every tick");

	/* clear_data() must reset has_accuracy so subsequent ticks stop
	 * publishing accuracy until the next set_accuracy call. */
	gnss_emul_clear_data(dev);
	k_sem_reset(&gnss_accuracy_published);
	gnss_accuracy_publish_count = 0;

	/* Drain any stale fix sem so the next take is fresh */
	k_sem_reset(&gnss_data_published);

	zassert_equal(0, k_sem_take(&gnss_data_published, K_MSEC(1500)));
	zassert_equal(-EAGAIN, k_sem_take(&gnss_accuracy_published, K_MSEC(100)),
		      "accuracy must NOT publish after clear_data");
	zassert_equal(0U, gnss_accuracy_publish_count);

	zassert_equal(0, pm_device_runtime_put(dev));
}
#endif /* CONFIG_GNSS_ACCURACY */

#if IS_ENABLED(CONFIG_GNSS_RAW_NMEA)
ZTEST(gnss_emul, test_raw_sentence_injection)
{
	const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));
	const char first[] = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
	const char second[] = "$GPGST,182141.000,15.5,15.3,7.2,21.8,0.9,0.5,0.8*54";

	gnss_raw_publish_count = 0;
	k_sem_reset(&gnss_raw_published);

	/* Synchronous publish: callback fires before inject_raw_sentence
	 * returns. No fix tick required. */
	gnss_emul_inject_raw_sentence(dev, first, sizeof(first) - 1);

	zassert_equal(1U, gnss_raw_publish_count, "raw callback must fire once per inject");
	zassert_equal(0, k_sem_take(&gnss_raw_published, K_NO_WAIT),
		      "synchronous publish leaves the sem signalled");
	zassert_equal(sizeof(first) - 1, gnss_published_raw_len);
	zassert_mem_equal(gnss_published_raw, first, sizeof(first) - 1,
			  "byte-faithful capture of the injected sentence");

	gnss_emul_inject_raw_sentence(dev, second, sizeof(second) - 1);
	zassert_equal(2U, gnss_raw_publish_count);
	zassert_equal(sizeof(second) - 1, gnss_published_raw_len);
	zassert_mem_equal(gnss_published_raw, second, sizeof(second) - 1);
}
#endif /* CONFIG_GNSS_RAW_NMEA */

ZTEST_SUITE(gnss_emul, NULL, NULL, NULL, NULL, NULL);
