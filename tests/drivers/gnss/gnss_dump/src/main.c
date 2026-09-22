/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/device.h>

DEVICE_DEFINE(gnss_dev, "gnss_dev", NULL, NULL, NULL, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

static const struct device *gnss_dev = &DEVICE_NAME_GET(gnss_dev);
static struct gnss_data test_data;
static struct gnss_satellite test_satellite;

static void test_gnss_data(void)
{
	gnss_publish_data(gnss_dev, &test_data);

	/* positive values */
	test_data.nav_data.latitude = 10000000001;
	test_data.nav_data.longitude = 20000000002;
	test_data.nav_data.bearing = 3003;
	test_data.nav_data.speed = 4004;
	test_data.nav_data.altitude = 5005;

	test_data.info.satellites_cnt = 6;
	test_data.info.hdop = 7;
	test_data.info.fix_status = GNSS_FIX_STATUS_GNSS_FIX;
	test_data.info.fix_quality = GNSS_FIX_QUALITY_GNSS_PPS;

	test_data.utc.hour = 1;
	test_data.utc.minute = 2;
	test_data.utc.millisecond = 3;
	test_data.utc.month_day = 4;
	test_data.utc.month = 5;
	test_data.utc.century_year = 6;

	gnss_publish_data(gnss_dev, &test_data);

	/* small positive values */
	test_data.nav_data.latitude = 1;
	test_data.nav_data.longitude = 2;
	test_data.nav_data.bearing = 3;
	test_data.nav_data.speed = 4;
	test_data.nav_data.altitude = 5;

	gnss_publish_data(gnss_dev, &test_data);

	/* negative values */
	test_data.nav_data.latitude = -10000000001;
	test_data.nav_data.longitude = -20000000002;
	test_data.nav_data.altitude = -5005;

	gnss_publish_data(gnss_dev, &test_data);

	/* small negative values */
	test_data.nav_data.latitude = -1;
	test_data.nav_data.longitude = -2;
	test_data.nav_data.altitude = -5;

	gnss_publish_data(gnss_dev, &test_data);
}

static void test_satellites_data(void)
{
	gnss_publish_satellites(gnss_dev, &test_satellite, 1);

	test_satellite.prn = 1;
	test_satellite.snr = 2;
	test_satellite.azimuth = 3;
	test_satellite.system = GNSS_SYSTEM_GALILEO;
	test_satellite.is_tracked = 1;

	gnss_publish_satellites(gnss_dev, &test_satellite, 1);
}
int main(void)
{
	test_gnss_data();
	test_satellites_data();

	return 0;
}
