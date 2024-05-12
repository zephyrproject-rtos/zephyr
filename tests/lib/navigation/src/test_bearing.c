/*
 * Copyright 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/data/navigation.h>

struct test_sample {
	int64_t from_latitude;
	int64_t from_longitude;
	int64_t to_latitude;
	int64_t to_longitude;
	uint32_t bearing;
};

static struct navigation_data from_nav_data;
static struct navigation_data to_nav_data;

static const struct test_sample samples[] = {
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 0,
		.to_longitude = 90000000000,
		.bearing = 90000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 0,
		.to_longitude = -90000000000,
		.bearing = 90000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 44999999999,
		.to_longitude = 77942286340,
		.bearing = 60000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 44999999999,
		.to_longitude = -102057713660,
		.bearing = 60000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 77942286340,
		.to_longitude = 45000000000,
		.bearing = 30000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 77942286340,
		.to_longitude = -135000000000,
		.bearing = 30000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 90000000000,
		.to_longitude = 0,
		.bearing = 0,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 90000000000,
		.to_longitude = 180000000000,
		.bearing = 0,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 77942286340,
		.to_longitude = -44999999999,
		.bearing = 330001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 77942286340,
		.to_longitude = 135000000001,
		.bearing = 330001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 45000000000,
		.to_longitude = -77942286340,
		.bearing = 300001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 45000000000,
		.to_longitude = 102057713660,
		.bearing = 300001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = 0,
		.to_longitude = -90000000000,
		.bearing = 270000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = 0,
		.to_longitude = 90000000000,
		.bearing = 270000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = -44999999999,
		.to_longitude = -77942286340,
		.bearing = 240001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = -44999999999,
		.to_longitude = 102057713660,
		.bearing = 240001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = -77942286340,
		.to_longitude = -45000000000,
		.bearing = 210001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = -77942286340,
		.to_longitude = 135000000000,
		.bearing = 210001,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = -90000000000,
		.to_longitude = 0,
		.bearing = 180000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = -90000000000,
		.to_longitude = 180000000000,
		.bearing = 180000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = -77942286340,
		.to_longitude = 44999999999,
		.bearing = 150000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = -77942286340,
		.to_longitude = -135000000001,
		.bearing = 150000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 0,
		.to_latitude = -45000000000,
		.to_longitude = 77942286340,
		.bearing = 120000,
	},
	{
		.from_latitude = 0,
		.from_longitude = 180000000000,
		.to_latitude = -45000000000,
		.to_longitude = -102057713660,
		.bearing = 120000,
	},
};

static void set_nav_data(const struct test_sample *sample)
{
	from_nav_data.latitude = sample->from_latitude;
	from_nav_data.longitude = sample->from_longitude;
	to_nav_data.latitude = sample->to_latitude;
	to_nav_data.longitude = sample->to_longitude;
}

static bool validate_bearing(uint32_t est, uint32_t real)
{
	int64_t error;

	error = real;
	error -= est;
	PRINT("est: %u, real: %u, error: %lli\n", est, real, error);

	if (real < 180000 || est < 180000) {
		est += 180000;
		real += 180000;
	}

	if (est < (real - 5000)) {
		return false;
	}

	if (est > (real + 5000)) {
		return false;
	}

	return true;
}

ZTEST(navigation, test_bearing)
{
	uint32_t bearing;

	for (size_t i = 0; i < ARRAY_SIZE(samples); i++) {
		set_nav_data(&samples[i]);
		zassert_ok(navigation_bearing(&bearing, &from_nav_data, &to_nav_data));
		zassert_true(validate_bearing(bearing, samples[i].bearing));
	}
}
