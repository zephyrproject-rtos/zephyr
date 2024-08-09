/*
 * Copyright (c) 2024 Chaim Zax
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <math.h>

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include "adxl34x_test.h"
#include "adxl34x_emul.h"
#include "adxl34x_private.h"
#include "adxl34x_convert.h"

/* The test fixture type must have the same name as the test suite. Because the same fixture type is
 * shared between all test suites a define is used to make them 'compatible'.
 */
#define adxl34x_basic_fixture adxl34x_fixture

LOG_MODULE_DECLARE(adxl34x_test, CONFIG_SENSOR_LOG_LEVEL);

static void adxl34x_basic_suite_before(void *fixture)
{
	/* The tests in this test suite are only used by the build(s) specific for these test-cases,
	 * which is defined in the testcase.yaml file.
	 */
	Z_TEST_SKIP_IFNDEF(ADXL34X_TEST_BASIC);
	/* Setup all i2c and spi devices available. */
	adxl34x_suite_before(fixture);
}

/**
 * @brief Test sensor initialisation.
 *
 * @details All devices defined in the device tree are initialised at startup. Some devices should
 * succeed initialisation, some should not, and some are not used in this test suite at all. This
 * test verifies the devices which support the basic functionality are available.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see device_is_ready()
 */
ZTEST_USER_F(adxl34x_basic, test_device_is_ready_for_basic_tests)
{
	/* The devices below should be able to be used in these tests. */
	adxl34x_is_ready(fixture, ADXL34X_TEST_SPI_0);
	adxl34x_is_ready(fixture, ADXL34X_TEST_SPI_1);
	adxl34x_is_ready(fixture, ADXL34X_TEST_SPI_2);
	adxl34x_is_ready(fixture, ADXL34X_TEST_I2C_53);
	adxl34x_is_ready(fixture, ADXL34X_TEST_I2C_54);
	adxl34x_is_ready(fixture, ADXL34X_TEST_I2C_55);
}

static void double_to_raw(const struct emul *target, const double value, int8_t *out)
{
	struct adxl34x_dev_data *dev_data = target->dev->data;
	const double range_scale =
		(double)adxl34x_range_conv[dev_data->cfg.data_format.range] / 10000;
	const int16_t scaled_value = value / range_scale;

	sys_put_le16(scaled_value, (uint8_t *)out);
}

static void set_simulated_sensor_values(struct adxl34x_fixture *fixture,
					const enum ADXL34X_TEST test_device,
					const double value_in[])
{
	const struct emul *target = fixture->device[test_device].emul;
	struct adxl34x_emul_data *data = (struct adxl34x_emul_data *)target->data;
	uint8_t *reg = data->reg;

	zassert_equal(reg[ADXL34X_REG_DEVID], ADXL344_DEVID,
		      "Device id doesn't match, sanity check failed");
	double_to_raw(target, value_in[0], &reg[ADXL34X_REG_DATA]);
	double_to_raw(target, value_in[1], &reg[ADXL34X_REG_DATA + 2]);
	double_to_raw(target, value_in[2], &reg[ADXL34X_REG_DATA + 4]);
	reg[ADXL34X_REG_FIFO_STATUS] = 1; /* FIFO entries = 1 */
}

static void test_get_value(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device,
			   const bool use_pm)
{
	LOG_DBG("Running test on %s/%s", adxl34x_get_bus_name(fixture, test_device),
		adxl34x_get_name(fixture, test_device));
	struct sensor_value acc[3];

	/* Setup the test-case/driver with pre-defined x, y and z values. */
	zassert_not_null(fixture->device[test_device].emul);
	const struct adxl34x_dev_data *dev_data = fixture->device[test_device].emul->dev->data;
	const uint8_t max_g = adxl34x_max_g_conv[dev_data->cfg.data_format.range];
	const double value_in[] = {-1.0 * max_g, 0.5 * max_g, 1.0 * max_g};

	set_simulated_sensor_values(fixture, test_device, value_in);

#if CONFIG_PM_DEVICE_RUNTIME
	if (use_pm) {
		zassert_ok(pm_device_runtime_get(fixture->device[test_device].dev));
	} else {
		/* Fetching sensor data without a pm-get should fail. */
		zassert_false(sensor_sample_fetch(fixture->device[test_device].dev) >= 0);
		return;
	}
#endif

	/* Use the sensor as normal. */
	zassert_true(sensor_sample_fetch(fixture->device[test_device].dev) >= 0);
	zassert_ok(
		sensor_channel_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ, acc));

	/* Verify the set values are corresponding with the returned values. */
	for (unsigned int i = 0; i < 3; i++) {
		const double value_out = MS2_TO_G(sensor_value_to_double(&acc[i]));

		zassert_true(fabs(value_in[i] - value_out) < 0.05);
	}

#if CONFIG_PM_DEVICE_RUNTIME
	if (use_pm) {
		zassert_ok(
			pm_device_runtime_put_async(fixture->device[test_device].dev, K_NO_WAIT));
	}
#endif
}

/**
 * @brief Test getting basic sensor data using spi.
 *
 * @details Use the default polling mechanism to get sensor data.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see sensor_sample_fetch(), sensor_channel_get()
 */
ZTEST_USER_F(adxl34x_basic, test_get_value_spi)
{
	test_get_value(fixture, ADXL34X_TEST_SPI_0, true);
	test_get_value(fixture, ADXL34X_TEST_SPI_1, true);
	test_get_value(fixture, ADXL34X_TEST_SPI_2, true);
}

/**
 * @brief Test getting basic sensor data using i2c.
 *
 * @details Use the default polling mechanism to get sensor data.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see sensor_sample_fetch(), sensor_channel_get()
 */
ZTEST_USER_F(adxl34x_basic, test_get_value_i2c)
{
	test_get_value(fixture, ADXL34X_TEST_I2C_53, true);
	test_get_value(fixture, ADXL34X_TEST_I2C_54, true);
	test_get_value(fixture, ADXL34X_TEST_I2C_55, true);
}

/**
 * @brief Test getting basic sensor data with power management enabled.
 *
 * @details Use the default polling mechanism to get sensor data without a pm_device_runtime_get.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see sensor_sample_fetch(), sensor_channel_get(), pm_device_runtime_get()
 */
ZTEST_USER_F(adxl34x_basic, test_sample_fetch_pm)
{
	test_get_value(fixture, ADXL34X_TEST_I2C_53, false);
}

static void test_sensor_attr_sampling_frequency(struct adxl34x_fixture *fixture,
						const enum ADXL34X_TEST test_device,
						const struct sensor_value default_value)
{
	static const struct sensor_value value_in = {12, 500000}; /* 12.50 Hz */
	struct sensor_value value_out;
	/* Check default value */
	zassert_ok(sensor_attr_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &value_out));
	zassert_true(is_equal_sensor_value(default_value, value_out));
	/* Check if setting and getting values */
	zassert_ok(sensor_attr_set(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &value_in));
	zassert_ok(sensor_attr_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_SAMPLING_FREQUENCY, &value_out));
	zassert_true(is_equal_sensor_value(value_in, value_out));
}

/**
 * @brief Test changing the sample frequency.
 *
 * @details Depending on the device tree configuration the various sensors are initialised with
 * various frequencies. This test not only tests if these defaults are correct, it also sets a new
 * frequency and verifies this frequency is set correctly.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see sensor_attr_get()
 */
ZTEST_USER_F(adxl34x_basic, test_sensor_attr_sampling_frequency)
{
	/* No value provided in the dts file, verify default (100 Hz) */
	test_sensor_attr_sampling_frequency(fixture, ADXL34X_TEST_I2C_53,
					    (struct sensor_value){100, 0});
	/* Value set explicitly in dts file to 50 Hz. */
	test_sensor_attr_sampling_frequency(fixture, ADXL34X_TEST_I2C_54,
					    (struct sensor_value){50, 0});
	/* Value set explicitly in dts file to 3200 Hz. */
	test_sensor_attr_sampling_frequency(fixture, ADXL34X_TEST_I2C_55,
					    (struct sensor_value){3200, 0});
}

void test_sensor_attr_sampling_frequency_range(struct adxl34x_fixture *fixture,
					       const enum ADXL34X_TEST test_device)
{
	static const double q31_delta_error = 0.0000005;
	static const double frequency[] = {
		0.10, 0.20, 0.39,  0.78,  1.56,  3.13,  6.25,   12.50,
		25.0, 50.0, 100.0, 200.0, 400.0, 800.0, 1600.0, 3200.0,
	};

	for (unsigned int i = 0; i < ARRAY_SIZE(frequency); i++) {
		struct sensor_value value_in, value_out;
		/* Rounding down will be done when setting the sample frequency,
		 * to make sure this doesn't result in setting the value below
		 * the one needed we add a delta error to the set value.
		 */
		sensor_value_from_double(&value_in, frequency[i] + q31_delta_error);

		LOG_DBG("Setting frequency to %d.%d", value_in.val1, value_in.val2);
		zassert_ok(sensor_attr_set(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
					   SENSOR_ATTR_SAMPLING_FREQUENCY, &value_in));
		zassert_ok(sensor_attr_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
					   SENSOR_ATTR_SAMPLING_FREQUENCY, &value_out));
		const double frequency_out = sensor_value_to_double(&value_out);

		zassert_true(fabs(frequency[i] - frequency_out) < q31_delta_error);
	}
}

/**
 * @brief Test the complete range of sampling frequencies.
 *
 * @details Loop though the entire set of supported frequencies and verify each of the can be set
 * correctly (by reading back the result).
 *
 * @ingroup driver_sensor_subsys_tests
 * @see sensor_attr_set(), sensor_attr_get()
 */
ZTEST_USER_F(adxl34x_basic, test_sensor_attr_sampling_frequency_range)
{
	test_sensor_attr_sampling_frequency_range(fixture, ADXL34X_TEST_I2C_53);
}

void test_sensor_attr_offset(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device)
{
	static const int32_t ug_in[] = {-1000000, 500000, 2000000};
	struct sensor_value value_in[3], value_out[3];

	for (unsigned int i = 0; i < 3; i++) {
		sensor_ug_to_ms2(ug_in[i], &value_in[i]);
	}
	zassert_ok(sensor_attr_set(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_OFFSET, value_in));
	zassert_ok(sensor_attr_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_OFFSET, value_out));

	for (unsigned int i = 0; i < 3; i++) {
		const int32_t ug_out = sensor_ms2_to_ug(&value_out[i]);

		zassert_true(fabs((double)ug_in[i] - (double)ug_out) / 1000000 < 0.02);
	}
}

/**
 * @brief Test changing the offset value.
 *
 * @details Verify changing the offset of the sensor value works correctly by using a range of
 * values (both positive and negative).
 *
 * @ingroup driver_sensor_subsys_tests
 * @see sensor_attr_set(), sensor_attr_get()
 */
ZTEST_USER_F(adxl34x_basic, test_sensor_attr_offset)
{
	test_sensor_attr_offset(fixture, ADXL34X_TEST_I2C_55);
}

void test_emul_set_attr_offset(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device)
{
	static const struct sensor_chan_spec channel = {
		.chan_idx = 0,
		.chan_type = SENSOR_CHAN_ACCEL_XYZ,
	};
	static const double offset_lsb_ms2 = 0.152985; /* lsb = 15.6 mg = 0.152985 ms2 */
	static const uint8_t shift = 5; /* Maximum value of offset_in now is 19.6 ms2 (2g) */
	static const double offset_in[] = {offset_lsb_ms2, -offset_lsb_ms2, offset_lsb_ms2 * 100};
	const struct sensor_three_axis_attribute offset_in_q31 = {
		.x = DOUBLE_TO_Q31(offset_in[0], shift),
		.y = DOUBLE_TO_Q31(offset_in[1], shift),
		.z = DOUBLE_TO_Q31(offset_in[2], shift),
		.shift = shift,
	};
	struct sensor_value offset_out[3];

	zassert_ok(emul_sensor_backend_set_attribute(fixture->device[test_device].emul, channel,
						     SENSOR_ATTR_OFFSET, &offset_in_q31));
	zassert_ok(sensor_attr_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_XYZ,
				   SENSOR_ATTR_OFFSET, offset_out));

	for (unsigned int i = 0; i < 3; i++) {
		const double offset_out_ms2 = sensor_value_to_double(&offset_out[i]);

		zassert_true(fabs(offset_out_ms2 - offset_in[i]) < 0.0005);
	}
}

/**
 * @brief Test the sensor emulation api.
 *
 * @details Using the sensor emulation api change the offset of the sensor, and verify by getting
 * the offset using the normal sensor api.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see emul_sensor_backend_set_attribute(), sensor_attr_get()
 */
ZTEST_USER_F(adxl34x_basic, test_emul_set_attr_offset)
{
	test_emul_set_attr_offset(fixture, ADXL34X_TEST_I2C_55);
}

void test_emul_get_sample_range(struct adxl34x_fixture *fixture,
				const enum ADXL34X_TEST test_device)
{
	static const struct sensor_chan_spec channel = {
		.chan_idx = 0,
		.chan_type = SENSOR_CHAN_ACCEL_XYZ,
	};
	static const uint16_t resolution = 512;
	const struct emul *target = fixture->device[test_device].emul;
	const struct adxl34x_dev_data *dev_data = target->dev->data;
	const double max_g = (double)adxl34x_max_g_conv[dev_data->cfg.data_format.range];
	q31_t lower, upper, epsilon;
	int8_t shift;

	zassert_ok(emul_sensor_backend_get_sample_range(fixture->device[test_device].emul, channel,
							&lower, &upper, &epsilon, &shift));

	const double lower_f = MS2_TO_G(Q31_TO_DOUBLE(lower, shift));
	const double upper_f = MS2_TO_G(Q31_TO_DOUBLE(upper, shift));
	const double epsilon_f = MS2_TO_G(Q31_TO_DOUBLE(epsilon, shift));

	zassert_true(fabs(upper_f - max_g) < 0.0001);
	zassert_true(fabs(-lower_f - max_g) < 0.0001);
	zassert_true(fabs(epsilon_f - max_g / resolution) < 0.0001);
}

/**
 * @brief Test the sensor emulation api.
 *
 * @details Using the sensor emulation api get the sensor range and verify using the one set in the
 * device tree.  the offset using the normal sensor api.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see emul_sensor_backend_get_sample_range()
 */
ZTEST_USER_F(adxl34x_basic, test_emul_get_sample_range)
{
	test_emul_get_sample_range(fixture, ADXL34X_TEST_I2C_53);
	test_emul_get_sample_range(fixture, ADXL34X_TEST_I2C_54);
	test_emul_get_sample_range(fixture, ADXL34X_TEST_I2C_55);
}

void test_emul_set_channel(struct adxl34x_fixture *fixture, const enum ADXL34X_TEST test_device,
			   uint16_t chan_type)
{
	static const struct sensor_chan_spec channel_x = {
		.chan_idx = 0,
		.chan_type = SENSOR_CHAN_ACCEL_X,
	};
	static const struct sensor_chan_spec channel_y = {
		.chan_idx = 0,
		.chan_type = SENSOR_CHAN_ACCEL_Y,
	};
	static const struct sensor_chan_spec channel_z = {
		.chan_idx = 0,
		.chan_type = SENSOR_CHAN_ACCEL_Z,
	};
	static const uint8_t shift = 5; /* Maximum value of offset_in now is 19.6 ms2 (2g) */
	static const double value_in[] = {9.80665, -9.80665, 19.6133};
	const struct sensor_three_axis_attribute value = {
		.x = DOUBLE_TO_Q31(value_in[0], shift),
		.y = DOUBLE_TO_Q31(value_in[1], shift),
		.z = DOUBLE_TO_Q31(value_in[2], shift),
		.shift = shift,
	};
	struct sensor_value acc[3];

	zassert_ok(emul_sensor_backend_set_channel(fixture->device[test_device].emul, channel_x,
						   &value.x, shift));
	zassert_ok(emul_sensor_backend_set_channel(fixture->device[test_device].emul, channel_y,
						   &value.y, shift));
	zassert_ok(emul_sensor_backend_set_channel(fixture->device[test_device].emul, channel_z,
						   &value.z, shift));

#if CONFIG_PM_DEVICE_RUNTIME
	zassert_ok(pm_device_runtime_get(fixture->device[test_device].dev));
#endif

	/* Check a non existent channel */
	zassert_false(sensor_sample_fetch_chan(fixture->device[test_device].dev,
					       SENSOR_CHAN_VOLTAGE) >= 0);

	/* Use the sensor as normal. */
	if (chan_type == SENSOR_CHAN_ALL) {
		zassert_true(sensor_sample_fetch(fixture->device[test_device].dev) >= 0);
		zassert_ok(
			sensor_channel_get(fixture->device[test_device].dev, SENSOR_CHAN_ALL, acc));
	} else if (chan_type == SENSOR_CHAN_ACCEL_XYZ) {
		zassert_true(sensor_sample_fetch_chan(fixture->device[test_device].dev,
						      SENSOR_CHAN_ACCEL_XYZ) >= 0);
		zassert_ok(sensor_channel_get(fixture->device[test_device].dev,
					      SENSOR_CHAN_ACCEL_XYZ, acc));
	} else {
		zassert_true(sensor_sample_fetch_chan(fixture->device[test_device].dev,
						      SENSOR_CHAN_ACCEL_X) >= 0);
		zassert_ok(sensor_channel_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_X,
					      &acc[0]));
		zassert_true(sensor_sample_fetch_chan(fixture->device[test_device].dev,
						      SENSOR_CHAN_ACCEL_Y) >= 0);
		zassert_ok(sensor_channel_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_Y,
					      &acc[1]));
		zassert_true(sensor_sample_fetch_chan(fixture->device[test_device].dev,
						      SENSOR_CHAN_ACCEL_Z) >= 0);
		zassert_ok(sensor_channel_get(fixture->device[test_device].dev, SENSOR_CHAN_ACCEL_Z,
					      &acc[2]));
	}

	/* Verify the set values are corresponding with the returned values. */
	for (unsigned int i = 0; i < 3; i++) {
		const double value_out = sensor_value_to_double(&acc[i]);

		zassert_true(fabs(value_in[i] - value_out) < 0.05);
	}

#if CONFIG_PM_DEVICE_RUNTIME
	zassert_ok(pm_device_runtime_put_async(fixture->device[test_device].dev, K_NO_WAIT));
#endif
}

/**
 * @brief Test getting samples values which were set using the emul(ation) api.
 *
 * @details Simular to the previous test (getting basic sensor data) except pre-defined sensor
 * values are set in advance, which are verified after getting the values.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see emul_sensor_backend_set_channel(), sensor_sample_fetch(), sensor_channel_get()
 */
ZTEST_USER_F(adxl34x_basic, test_emul_set_channel)
{
	test_emul_set_channel(fixture, ADXL34X_TEST_I2C_53, SENSOR_CHAN_ACCEL_XYZ);
	test_emul_set_channel(fixture, ADXL34X_TEST_I2C_54, SENSOR_CHAN_ACCEL_XYZ);
	test_emul_set_channel(fixture, ADXL34X_TEST_I2C_55, SENSOR_CHAN_ACCEL_XYZ);
}

/**
 * @brief Test getting samples values which were set using the emul(ation) api.
 *
 * @details Simular to the previous test (getting basic sensor data) except pre-defined sensor
 * values are set in advance, which are verified after getting the values, and different channels
 * are fetched/get.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see emul_sensor_backend_set_channel(), sensor_sample_fetch(), sensor_channel_get()
 */
ZTEST_USER_F(adxl34x_basic, test_emul_set_channel_x)
{
	test_emul_set_channel(fixture, ADXL34X_TEST_I2C_53, SENSOR_CHAN_ACCEL_X);
	test_emul_set_channel(fixture, ADXL34X_TEST_I2C_53, SENSOR_CHAN_ALL);
}

void test_emul_get_attr_offset_metadata(struct adxl34x_fixture *fixture,
					const enum ADXL34X_TEST test_device)
{
	static const struct sensor_chan_spec channel = {
		.chan_idx = 0,
		.chan_type = SENSOR_CHAN_ACCEL_XYZ,
	};
	static const double min_ms2 = -19.58161;     /* -2 g */
	static const double max_ms2 = 19.42863;      /* 2 g */
	static const double increment_ms2 = 0.15298; /* 15.6 mg */
	q31_t min_q31, max_q31, increment_q31;
	int8_t shift;

	zassert_ok(emul_sensor_backend_get_attribute_metadata(fixture->device[test_device].emul,
							      channel, SENSOR_ATTR_OFFSET, &min_q31,
							      &max_q31, &increment_q31, &shift));

	const double min = Q31_TO_DOUBLE(min_q31, shift);
	const double max = Q31_TO_DOUBLE(max_q31, shift);
	const double increment = Q31_TO_DOUBLE(increment_q31, shift);

	zassert_true(fabs(min - min_ms2) < 0.001);
	zassert_true(fabs(max - max_ms2) < 0.001);
	zassert_true(fabs(increment - increment_ms2) < 0.0001);
}

/**
 * @brief Test the sensor emulation api.
 *
 * @details Using the sensor emulation api get the metadata of the sensor offset attribute.
 *
 * @ingroup driver_sensor_subsys_tests
 * @see emul_sensor_backend_get_attribute_metadata()
 */
ZTEST_USER_F(adxl34x_basic, test_emul_get_attr_offset_metadata)
{
	test_emul_get_attr_offset_metadata(fixture, ADXL34X_TEST_I2C_53);
}

ZTEST_SUITE(adxl34x_basic, NULL, adxl34x_suite_setup, adxl34x_basic_suite_before, NULL, NULL);
