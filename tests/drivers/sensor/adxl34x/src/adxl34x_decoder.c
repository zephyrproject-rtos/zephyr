/*
 * Copyright (c) 2024 Chaim Zax
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include "adxl34x_test.h"

/* The test fixture type must have the same name as the test suite. Because the same fixture type is
 * shared between all test suites a define is used to make them 'compatible'.
 */
#define adxl34x_decoder_fixture adxl34x_fixture

LOG_MODULE_DECLARE(adxl34x_test, CONFIG_SENSOR_LOG_LEVEL);

static void adxl34x_decoder_suite_before(void *fixture)
{
	/* The tests in this test suite are only used by the build(s) specific for these test-cases,
	 * which is defined in the testcase.yaml file.
	 */
	Z_TEST_SKIP_IFNDEF(ADXL34X_TEST_DECODER);
	/* Setup all i2c and spi devices available. */
	adxl34x_suite_before(fixture);
}

/**
 * @brief Test sensor initialisation.
 *
 * @details All devices defined in the device tree are initialised at startup. Some devices should
 * succeed initialisation, some should not, and some are not used in this test suite at all. This
 * test verifies the devices which support the decoder functionality are available.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see device_is_ready()
 */
ZTEST_USER_F(adxl34x_decoder, test_device_is_ready_for_decoder_tests)
{
	/* The devices below should be able to be used in these tests. */
	adxl34x_is_ready(fixture, ADXL34X_TEST_SPI_0);
	adxl34x_is_ready(fixture, ADXL34X_TEST_SPI_1);
	adxl34x_is_ready(fixture, ADXL34X_TEST_SPI_2);
	adxl34x_is_ready(fixture, ADXL34X_TEST_I2C_53);
	adxl34x_is_ready(fixture, ADXL34X_TEST_I2C_54);
	adxl34x_is_ready(fixture, ADXL34X_TEST_I2C_55);
}

ZTEST_SUITE(adxl34x_decoder, NULL, adxl34x_suite_setup, adxl34x_decoder_suite_before, NULL, NULL);
