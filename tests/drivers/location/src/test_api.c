/*
 * Copyright (c) 2022, Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/location.h>
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
	location_event_handler_t handler; 
	void *handler_user_data;
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

static int test_event_handler_set(const struct device *dev,
			location_event_handler_t handler, void *user_data)
{
	struct test_driver_data *data = (struct test_driver_data *)dev->data;

	data->handler = handler;
	data->handler_user_data = user_data;
	return 0;
}

static void test_location_event_handler(const struct device *dev, uint8_t events,
			void *user_data)
{

}

static struct location_api test_location_api = {
	.position_get = test_position_get,
	.bearing_get = test_bearing_get,
	.speed_get = test_speed_get,
	.altitude_get = test_altitude_get,
	.event_handler_set = test_event_handler_set
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
 *                             User data
 **********************************************************************/
static int32_t user_data_1 = 320;
static int32_t user_data_2 = 455;

/***********************************************************************
 *                          Driver instances
 **********************************************************************/
const struct device test_driver1 = {
	.name = "TEST1",
	.api = &test_location_api,
	.data = &test_driver_data1,
};

const struct device test_driver2 = {
	.name = "TEST2",
	.api = &test_location_api,
	.data = &test_driver_data2,
};

/***********************************************************************
 *                        Supervisor mode tests
 **********************************************************************/
void test_location_api_position_get(void)
{
	/* Get position from provider 1 */
	struct location_position position1;
	struct location_position position2;

	zassert_true(location_position_get(&test_driver1, &position1) == 0,
		"Could not get position from driver 1");
	zassert_true(location_position_get(&test_driver2, &position2) == 0,
		"Could not get position from driver 2");

	/* Verify positions */
	zassert_true(memcmp(&position1, &test_driver_data1.position,
		sizeof(struct location_position)) == 0,
		"Returned position 1 does not match real position 1");
	zassert_true(memcmp(&position2, &test_driver_data2.position,
		sizeof(struct location_position)) == 0,
		"Returned position 2 does not match real position 2");
}

void test_location_api_bearing_get(void)
{
	/* Get bearing from provider 1 */
	struct location_bearing bearing1;
	struct location_bearing bearing2;

	zassert_true(location_bearing_get(&test_driver1, &bearing1) == 0,
		"Could not get bearing from driver 1");
	zassert_true(location_bearing_get(&test_driver2, &bearing2) == 0,
		"Could not get bearing from driver 2");

	/* Verify bearings */
	zassert_true(memcmp(&bearing1, &test_driver_data1.bearing,
		sizeof(struct location_bearing)) == 0,
		"Returned bearing 1 does not match real bearing 1");
	zassert_true(memcmp(&bearing2, &test_driver_data2.bearing,
		sizeof(struct location_bearing)) == 0,
		"Returned bearing 2 does not match real bearing 2");
}

void test_location_api_speed_get(void)
{
	/* Get speed from provider 1 */
	struct location_speed speed1;
	struct location_speed speed2;

	zassert_true(location_speed_get(&test_driver1, &speed1) == 0,
		"Could not get speed from driver 1");
	zassert_true(location_speed_get(&test_driver2, &speed2) == 0,
		"Could not get speed from driver 2");

	/* Verify speeds */
	zassert_true(memcmp(&speed1, &test_driver_data1.speed, sizeof(struct location_speed)) == 0,
		"Returned speed 1 does not match real speed 1");
	zassert_true(memcmp(&speed2, &test_driver_data2.speed, sizeof(struct location_speed)) == 0,
		"Returned speed 2 does not match real speed 2");
}

/* Try to get and verify altitude from the two registered location providers */
void test_location_api_altitude_get(void)
{
	/* Get altitude from provider 1 */
	struct location_altitude altitude1;
	struct location_altitude altitude2;

	zassert_true(location_altitude_get(&test_driver1, &altitude1) == 0,
		"Could not get altitude from driver 1");
	zassert_true(location_altitude_get(&test_driver2, &altitude2) == 0,
		"Could not get altitude from driver 2");

	/* Verify altitudes */
	zassert_true(memcmp(&altitude1, &test_driver_data1.altitude,
		sizeof(struct location_altitude)) == 0,
		"Returned altitude 1 does not match real altitude 1");
	zassert_true(memcmp(&altitude2, &test_driver_data2.altitude,
		sizeof(struct location_altitude)) == 0,
		"Returned altitude 2 does not match real altitude 2");
}

void test_location_api_event_handler_set(void)
{
	zassert_ok(location_event_handler_set(&test_driver1, test_location_event_handler,
				&user_data_1), "Failed to set event handler for driver 1");

	zassert_true(((struct test_driver_data *)test_driver1.data)->handler ==
				test_location_event_handler, "Incorrect event handler set for driver 1");

	zassert_true(((struct test_driver_data *)test_driver1.data)->handler_user_data ==
				&user_data_1, "Incorrect event handler user data set for driver 1");

	zassert_ok(location_event_handler_set(&test_driver2, test_location_event_handler,
				&user_data_2), "Failed to set event handler for driver 2");

	zassert_true(((struct test_driver_data *)test_driver2.data)->handler ==
				test_location_event_handler, "Incorrect event handler set for driver 2");

	zassert_true(((struct test_driver_data *)test_driver2.data)->handler_user_data ==
				&user_data_2, "Incorrect event handler user data set for driver 2");
}

/***********************************************************************
 *                           Usermode tests
 **********************************************************************/
void test_location_api_position_get_user(void)
{
	test_location_api_position_get();
}

void test_location_api_bearing_get_user(void)
{
	test_location_api_bearing_get();
}

void test_location_api_speed_get_user(void)
{
	test_location_api_speed_get();
}

void test_location_api_altitude_get_user(void)
{
	test_location_api_altitude_get();
}

void test_location_api_event_handler_set_user(void)
{
	test_location_api_event_handler_set();
}
