/*
 * Copyright (c) 2024 Chaim Zax
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "adxl34x_test.h"

LOG_MODULE_REGISTER(adxl34x_test, CONFIG_SENSOR_LOG_LEVEL);

#define ADXL34X_DEVICE_DEF(_dev, _path1, _path2)                                                   \
	[_dev] = {                                                                                 \
		.dev = DEVICE_DT_GET(DT_PATH(_path1, _path2)),                                     \
		.emul = EMUL_DT_GET(DT_PATH(_path1, _path2)),                                      \
	}
#define ADXL34X_DEVICE_NONE(_dev) [_dev] = {}
#define ADXL34X_DEVICE_FIXTURE(_dev, _path1, _path2)                                               \
	COND_CODE_1(DT_NODE_EXISTS(DT_PATH(_path1, _path2)),                                       \
		    (ADXL34X_DEVICE_DEF(_dev, _path1, _path2)), (ADXL34X_DEVICE_NONE(_dev)))

static const char unknown[] = "unknown";
static char bus_name[][5] = {
	"i2c",
	"espi",
	"spi",
	"none",
};

void *adxl34x_suite_setup(void)
{
	/* clang-format off */
	static struct adxl34x_fixture fixture = {
		.device = {
			ADXL34X_DEVICE_FIXTURE(ADXL34X_TEST_SPI_0,  spi_200, adxl34x_0),
			ADXL34X_DEVICE_FIXTURE(ADXL34X_TEST_SPI_1,  spi_200, adxl34x_1),
			ADXL34X_DEVICE_FIXTURE(ADXL34X_TEST_SPI_2,  spi_200, adxl34x_2),
			ADXL34X_DEVICE_FIXTURE(ADXL34X_TEST_I2C_53, i2c_100, adxl34x_53),
			ADXL34X_DEVICE_FIXTURE(ADXL34X_TEST_I2C_54, i2c_100, adxl34x_54),
			ADXL34X_DEVICE_FIXTURE(ADXL34X_TEST_I2C_55, i2c_100, adxl34x_55),
		},
	};
	/* clang-format on */
	return &fixture;
}

void adxl34x_suite_before(void *a_fixture)
{
	const struct adxl34x_fixture *fixture = (struct adxl34x_fixture *)a_fixture;

	for (unsigned int i = 0; i < ADXL34X_TEST_NR_OF_DEVICES; i++) {
		if (fixture->device[i].dev != NULL) {
			k_object_access_grant(fixture->device[i].dev, k_current_get());
		}
	}
}

const char *adxl34x_get_name(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device)
{
	if (fixture == NULL || fixture->device[test_device].dev == NULL) {
		return unknown;
	}
	return fixture->device[test_device].dev->name;
}

const char *adxl34x_get_bus_name(struct adxl34x_fixture *fixture,
				 const enum ADXL34X_TEST test_device)
{
	if (fixture == NULL || fixture->device[test_device].emul == NULL) {
		return unknown;
	}
	const enum emul_bus_type bus_type = fixture->device[test_device].emul->bus_type;

	if (bus_type > 3) {
		return unknown;
	}
	return bus_name[bus_type];
}

void adxl34x_is_ready(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device)
{
	zassert_ok(fixture->device[test_device].dev->state->init_res,
		   "Device %s/%s not initialized correctly",
		   adxl34x_get_bus_name(fixture, test_device),
		   adxl34x_get_name(fixture, test_device));
}

void adxl34x_is_not_ready(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device)
{
	zassert_not_ok(fixture->device[test_device].dev->state->init_res,
		       "Device %s/%s was initialized correctly unexpectedly",
		       adxl34x_get_bus_name(fixture, test_device),
		       adxl34x_get_name(fixture, test_device));
}

bool is_equal_sensor_value(const struct sensor_value value_1, const struct sensor_value value_2)
{
	return (value_1.val1 == value_2.val1 && value_1.val2 == value_2.val2);
}

bool is_equal_float(const float value_1, const float value_2, const float error)
{
	return (fabsf(value_1 - value_2) < error);
}

bool is_equal_double(const double value_1, const double value_2, const double error)
{
	return (fabs(value_1 - value_2) < error);
}
