/*
 * Copyright (c) 2022, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/location.h>
#include <ztest.h>
#include <string.h>

/***********************************************************************
 *                    Driver instance data structure
 **********************************************************************/
struct test_driver_data {
	struct location_position position;
	struct location_bearing bearing;
	struct location_speed speed;
	struct location_altitude altitude;
};

/***********************************************************************
 *             Driver location provider api implementation
 **********************************************************************/
static int test_position_get(const struct device *dev,
			     struct location_position *position)
{
	struct test_driver_data *data = (struct test_driver_data *)dev->data;

	*position = data->position;
	return 0;
}

static int test_bearing_get(const struct device *dev,
			    struct location_bearing *bearing)
{
	struct test_driver_data *data = (struct test_driver_data *)dev->data;

	*bearing = data->bearing;
	return 0;
}

static int test_speed_get(const struct device *dev,
			  struct location_speed *speed)
{
	struct test_driver_data *data = (struct test_driver_data *)dev->data;

	*speed = data->speed;
	return 0;
}

static int test_altitude_get(const struct device *dev,
			     struct location_altitude *altitude)
{
	struct test_driver_data *data = (struct test_driver_data *)dev->data;

	*altitude = data->altitude;
	return 0;
}

static struct location_provider_api test_location_provider_api = {
	.position_get = test_position_get,
	.bearing_get = test_bearing_get,
	.speed_get = test_speed_get,
	.altitude_get = test_altitude_get,
};

/***********************************************************************
 *                       Driver instance data
 **********************************************************************/
static struct test_driver_data test_driver_data1 = {
	.position = {
		.latitude = 100,
		.longitude = -100,
		.accuracy = 100,
		.uptime_ticks = 10,
	},
	.bearing = {
		.bearing = 200,
		.accuracy = 10,
		.uptime_ticks = 10,
	},
	.speed = {
		.speed = 1000,
		.accuracy = 10,
		.uptime_ticks = 10,
	},
	.altitude = {
		.altitude = 5000,
		.accuracy = 200,
		.uptime_ticks = 20,
	},
};

static struct test_driver_data test_driver_data2 = {
	.position = {
		.latitude = 200,
		.longitude = -200,
		.accuracy = 200,
		.uptime_ticks = 20,
	},
	.bearing = {
		.bearing = 400,
		.accuracy = 20,
		.uptime_ticks = 20,
	},
	.speed = {
		.speed = 1400,
		.accuracy = 20,
		.uptime_ticks = 20,
	},
	.altitude = {
		.altitude = 4000,
		.accuracy = 1000,
		.uptime_ticks = 200,
	},
};

/***********************************************************************
 *                          Driver instances
 **********************************************************************/
const struct device test_driver1 = {
	.name = "TEST1",
	.api = NULL,
	.data = &test_driver_data1,
};

const struct device test_driver2 = {
	.name = "TEST2",
	.api = NULL,
	.data = &test_driver_data2,
};

/***********************************************************************
 *                        Supervisor mode tests
 **********************************************************************/
void test_location_api_register(void)
{
	zassert_true(location_provider_register(&test_driver1,
		&test_location_provider_api) == 0, "Could not register test driver 1");
	zassert_true(location_provider_register(&test_driver2,
		&test_location_provider_api) == 0, "Could not register test driver 2");
	zassert_true(location_provider_register(&test_driver2,
		&test_location_provider_api) == -ENOMEM,
		"Could register a third provider with a max of 2 allowed");
}

void test_location_api_location_providers_get(void)
{
	/* Get providers */
	const struct location_provider *providers;

	zassert_true(location_providers_get(&providers) == 2,
		"Incorrect number of providers returned");

	/* Verify providers */
	zassert_true((void *)providers[0].dev == (void *)&test_driver1,
		"Returned provider 1 dev does not match registered provider 1 dev");
	zassert_true((void *)providers[1].dev == (void *)&test_driver2,
		"Returned provider 2 dev does not match registered provider 2 dev");
	zassert_true((void *)providers[0].api == (void *)&test_location_provider_api,
		"Returned provider 1 api does not match registered provider 1 api");
	zassert_true((void *)providers[1].api == (void *)&test_location_provider_api,
		"Returned provider 2 api does not match registered provider 2 api");
}

/* Try to get and verify position from the two registered location providers */
void test_location_api_position_get(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get position from provider 1 */
	struct location_position position1;
	struct location_position position2;

	zassert_true(location_position_get(&providers[0], &position1) == 0,
		"Could not get position from provider 1");
	zassert_true(location_position_get(&providers[1], &position2) == 0,
		"Could not get position from provider 2");

	/* Verify positions */
	zassert_true(memcmp(&position1, &test_driver_data1.position,
		sizeof(struct location_position)) == 0,
		"Returned position 1 does not match real position 1");
	zassert_true(memcmp(&position2, &test_driver_data2.position,
		sizeof(struct location_position)) == 0,
		"Returned position 2 does not match real position 2");
}

/* Try to get and verify bearing from the two registered location providers */
void test_location_api_bearing_get(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get bearing from provider 1 */
	struct location_bearing bearing1;
	struct location_bearing bearing2;

	zassert_true(location_bearing_get(&providers[0], &bearing1) == 0,
		"Could not get bearing from provider 1");
	zassert_true(location_bearing_get(&providers[1], &bearing2) == 0,
		"Could not get bearing from provider 2");

	/* Verify bearings */
	zassert_true(memcmp(&bearing1, &test_driver_data1.bearing,
		sizeof(struct location_bearing)) == 0,
		"Returned bearing 1 does not match real bearing 1");
	zassert_true(memcmp(&bearing2, &test_driver_data2.bearing,
		sizeof(struct location_bearing)) == 0,
		"Returned bearing 2 does not match real bearing 2");
}

/* Try to get and verify speed from the two registered location providers */
void test_location_api_speed_get(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);
	/* Get speed from provider 1 */
	struct location_speed speed1;
	struct location_speed speed2;

	zassert_true(location_speed_get(&providers[0], &speed1) == 0,
		"Could not get speed from provider 1");
	zassert_true(location_speed_get(&providers[1], &speed2) == 0,
		"Could not get speed from provider 2");

	/* Verify speeds */
	zassert_true(memcmp(&speed1, &test_driver_data1.speed, sizeof(struct location_speed)) == 0,
		"Returned speed 1 does not match real speed 1");
	zassert_true(memcmp(&speed2, &test_driver_data2.speed, sizeof(struct location_speed)) == 0,
		"Returned speed 2 does not match real speed 2");
}

/* Try to get and verify altitude from the two registered location providers */
void test_location_api_altitude_get(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get altitude from provider 1 */
	struct location_altitude altitude1;
	struct location_altitude altitude2;

	zassert_true(location_altitude_get(&providers[0], &altitude1) == 0,
		"Could not get altitude from provider 1");
	zassert_true(location_altitude_get(&providers[1], &altitude2) == 0,
		"Could not get altitude from provider 2");

	/* Verify altitudes */
	zassert_true(memcmp(&altitude1, &test_driver_data1.altitude,
		sizeof(struct location_altitude)) == 0,
		"Returned altitude 1 does not match real altitude 1");
	zassert_true(memcmp(&altitude2, &test_driver_data2.altitude,
		sizeof(struct location_altitude)) == 0,
		"Returned altitude 2 does not match real altitude 2");
}

/***********************************************************************
 *                           Usermode tests
 **********************************************************************/
void test_location_api_location_providers_get_user(void)
{
	/* Get providers */
	const struct location_provider *providers;

	zassert_true(location_providers_get(&providers) == 2,
		"Incorrect number of providers returned");
}

void test_location_api_position_get_user(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get position from provider 1 */
	struct location_position position1;
	struct location_position position2;

	zassert_true(location_position_get(&providers[0], &position1) == 0,
		"Could not get position from provider 1");
	zassert_true(location_position_get(&providers[1], &position2) == 0,
		"Could not get position from provider 2");

	/* Verify positions */
	zassert_true(memcmp(&position1, &test_driver_data1.position,
		sizeof(struct location_position)) == 0,
		"Returned position 1 does not match real position 1");
	zassert_true(memcmp(&position2, &test_driver_data2.position,
		sizeof(struct location_position)) == 0,
		"Returned position 2 does not match real position 2");
}

void test_location_api_bearing_get_user(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get bearing from provider 1 */
	struct location_bearing bearing1;
	struct location_bearing bearing2;

	zassert_true(location_bearing_get(&providers[0], &bearing1) == 0,
		"Could not get bearing from provider 1");
	zassert_true(location_bearing_get(&providers[1], &bearing2) == 0,
		"Could not get bearing from provider 2");

	/* Verify bearings */
	zassert_true(memcmp(&bearing1, &test_driver_data1.bearing,
		sizeof(struct location_bearing)) == 0,
		"Returned bearing 1 does not match real bearing 1");
	zassert_true(memcmp(&bearing2, &test_driver_data2.bearing,
		sizeof(struct location_bearing)) == 0,
		"Returned bearing 2 does not match real bearing 2");
}

void test_location_api_speed_get_user(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get speed from provider 1 */
	struct location_speed speed1;
	struct location_speed speed2;

	zassert_true(location_speed_get(&providers[0], &speed1) == 0,
		"Could not get speed from provider 1");
	zassert_true(location_speed_get(&providers[1], &speed2) == 0,
		"Could not get speed from provider 2");

	/* Verify speeds */
	zassert_true(memcmp(&speed1, &test_driver_data1.speed,
		sizeof(struct location_speed)) == 0,
		"Returned speed 1 does not match real speed 1");
	zassert_true(memcmp(&speed2, &test_driver_data2.speed,
		sizeof(struct location_speed)) == 0,
		"Returned speed 2 does not match real speed 2");
}

void test_location_api_altitude_get_user(void)
{
	/* Get providers */
	const struct location_provider *providers;

	location_providers_get(&providers);

	/* Get altitude from provider 1 */
	struct location_altitude altitude1;
	struct location_altitude altitude2;

	zassert_true(location_altitude_get(&providers[0], &altitude1) == 0,
		"Could not get altitude from provider 1");
	zassert_true(location_altitude_get(&providers[1], &altitude2) == 0,
		"Could not get altitude from provider 2");

	/* Verify altitudes */
	zassert_true(memcmp(&altitude1, &test_driver_data1.altitude,
		sizeof(struct location_altitude)) == 0,
		"Returned altitude 1 does not match real altitude 1");
	zassert_true(memcmp(&altitude2, &test_driver_data2.altitude,
		sizeof(struct location_altitude)) == 0,
		"Returned altitude 2 does not match real altitude 2");
}
