/* Copyright (c) 2024 Google Inc
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

ZTEST_SUITE(SensorGenerator, nullptr, nullptr, nullptr, nullptr, nullptr);

#define ASSERT_COMPAT_EXISTS(compat)                                                               \
	ZTEST(SensorGenerator, test_compat##_Exists)                                               \
	{                                                                                          \
		zassert_true(SENSOR_SPEC_COMPAT_EXISTS(compat));                                   \
	}

/*
 * Verify that every compatible value provided by the generated
 * SENSOR_SPEC_FOREACH_COMPAT exists.
 */
SENSOR_SPEC_FOREACH_COMPAT(ASSERT_COMPAT_EXISTS)

ZTEST(SensorGenerator, test_static_counts)
{
	zexpect_equal(2, SENSOR_SPEC_CHAN_COUNT(zephyr_test_sensor));
	zexpect_equal(3, SENSOR_SPEC_ATTR_COUNT(zephyr_test_sensor));
	zexpect_equal(0, SENSOR_SPEC_TRIG_COUNT(zephyr_test_sensor));
}

ZTEST(SensorGenerator, test_channel_bar_static_info)
{
	zassert_equal(2, SENSOR_SPEC_CHAN_INST_COUNT(zephyr_test_sensor, bar));
	zexpect_true(SENSOR_SPEC_CHAN_INST_EXISTS(zephyr_test_sensor, bar, 0));
	zexpect_true(SENSOR_SPEC_CHAN_INST_EXISTS(zephyr_test_sensor, bar, 1));
	zexpect_str_equal("left", SENSOR_SPEC_CHAN_INST_NAME(zephyr_test_sensor, bar, 0));
	zexpect_str_equal("right", SENSOR_SPEC_CHAN_INST_NAME(zephyr_test_sensor, bar, 1));
	zexpect_str_equal("Left side of the bar",
			  SENSOR_SPEC_CHAN_INST_DESC(zephyr_test_sensor, bar, 0));
	zexpect_str_equal("Right side of the bar",
			  SENSOR_SPEC_CHAN_INST_DESC(zephyr_test_sensor, bar, 1));

	const struct sensor_chan_spec bar0_spec =
		SENSOR_SPEC_CHAN_INST_SPEC(zephyr_test_sensor, bar, 0);
	const struct sensor_chan_spec bar1_spec =
		SENSOR_SPEC_CHAN_INST_SPEC(zephyr_test_sensor, bar, 1);

	zexpect_equal(SENSOR_CHAN_BAR, bar0_spec.chan_type);
	zexpect_equal(SENSOR_CHAN_BAR, bar1_spec.chan_type);
	zexpect_equal(0, bar0_spec.chan_idx);
	zexpect_equal(1, bar1_spec.chan_idx);
}

ZTEST(SensorGenerator, test_channel_foo_static_info)
{
	zassert_equal(1, SENSOR_SPEC_CHAN_INST_COUNT(zephyr_test_sensor, foo));
	zexpect_true(SENSOR_SPEC_CHAN_INST_EXISTS(zephyr_test_sensor, foo, 0));
	zexpect_str_equal("foo", SENSOR_SPEC_CHAN_INST_NAME(zephyr_test_sensor, foo, 0));
	zexpect_str_equal("A measurement of 'foo'",
			  SENSOR_SPEC_CHAN_INST_DESC(zephyr_test_sensor, foo, 0));

	const struct sensor_chan_spec foo0_spec =
		SENSOR_SPEC_CHAN_INST_SPEC(zephyr_test_sensor, foo, 0);
	zexpect_equal(SENSOR_CHAN_FOO, foo0_spec.chan_type);
	zexpect_equal(0, foo0_spec.chan_idx);
}

ZTEST(SensorGenerator, test_attribute_static_info)
{
	zexpect_true(SENSOR_SPEC_ATTR_EXISTS(zephyr_test_sensor, attr0, foo));
	zexpect_true(SENSOR_SPEC_ATTR_EXISTS(zephyr_test_sensor, attr1, foo));
	zexpect_true(SENSOR_SPEC_ATTR_EXISTS(zephyr_test_sensor, attr0, bar));
}
