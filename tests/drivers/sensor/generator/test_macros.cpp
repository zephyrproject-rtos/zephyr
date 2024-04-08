#include <zephyr/drivers/sensor.h>

#include "gtest/gtest.h"
#include "pw_unit_test/framework.h"

#define ASSERT_COMPAT_EXISTS(compat)                                                               \
	TEST(SensorGenerator, compat##_Exists)                                                           \
	{                                                                                                \
		ASSERT_TRUE(SENSOR_SPEC_COMPAT_EXISTS(compat));                                                \
	}

/*
 * Verify that every compatible value provided by the generated
 * SENSOR_SPEC_FOREACH_COMPAT exists.
 */
SENSOR_SPEC_FOREACH_COMPAT(ASSERT_COMPAT_EXISTS)

TEST(SensorGenerator, VerifyTestSensorCounts)
{
	EXPECT_EQ(2, SENSOR_SPEC_CHAN_COUNT(zephyr_test_sensor));
	EXPECT_EQ(3, SENSOR_SPEC_ATTR_COUNT(zephyr_test_sensor));
	EXPECT_EQ(0, SENSOR_SPEC_TRIG_COUNT(zephyr_test_sensor));
}

TEST(SensorGenerator, VerifyTestSensorChannelBar)
{
	ASSERT_EQ(2, SENSOR_SPEC_CHAN_INST_COUNT(zephyr_test_sensor, bar));
	EXPECT_TRUE(SENSOR_SPEC_CHAN_INST_EXISTS(zephyr_test_sensor, bar, 0));
	EXPECT_TRUE(SENSOR_SPEC_CHAN_INST_EXISTS(zephyr_test_sensor, bar, 1));
	EXPECT_STREQ("left", SENSOR_SPEC_CHAN_INST_NAME(zephyr_test_sensor, bar, 0));
	EXPECT_STREQ("right", SENSOR_SPEC_CHAN_INST_NAME(zephyr_test_sensor, bar, 1));
	EXPECT_STREQ("Left side of the bar",
		     SENSOR_SPEC_CHAN_INST_DESC(zephyr_test_sensor, bar, 0));
	EXPECT_STREQ("Right side of the bar",
		     SENSOR_SPEC_CHAN_INST_DESC(zephyr_test_sensor, bar, 1));

	const struct sensor_chan_spec bar0_spec =
		SENSOR_SPEC_CHAN_INST_SPEC(zephyr_test_sensor, bar, 0);
	const struct sensor_chan_spec bar1_spec =
		SENSOR_SPEC_CHAN_INST_SPEC(zephyr_test_sensor, bar, 1);

	EXPECT_EQ(SENSOR_CHAN_BAR, bar0_spec.chan_type);
	EXPECT_EQ(SENSOR_CHAN_BAR, bar1_spec.chan_type);
	EXPECT_EQ(0, bar0_spec.chan_idx);
	EXPECT_EQ(1, bar1_spec.chan_idx);
}

TEST(SensorGenerator, VerifyTestSensorChannelFoo)
{
	ASSERT_EQ(1, SENSOR_SPEC_CHAN_INST_COUNT(zephyr_test_sensor, foo));
	EXPECT_TRUE(SENSOR_SPEC_CHAN_INST_EXISTS(zephyr_test_sensor, foo, 0));
	EXPECT_STREQ("foo", SENSOR_SPEC_CHAN_INST_NAME(zephyr_test_sensor, foo, 0));
	EXPECT_STREQ("A measurement of 'foo'",
		     SENSOR_SPEC_CHAN_INST_DESC(zephyr_test_sensor, foo, 0));

	const struct sensor_chan_spec foo0_spec =
		SENSOR_SPEC_CHAN_INST_SPEC(zephyr_test_sensor, foo, 0);
	EXPECT_EQ(SENSOR_CHAN_FOO, foo0_spec.chan_type);
	EXPECT_EQ(0, foo0_spec.chan_idx);
}

TEST(SensorGenerator, VerifyTestSensorAttributes)
{
	EXPECT_TRUE(SENSOR_SPEC_ATTR_EXISTS(zephyr_test_sensor, attr0, foo));
	EXPECT_TRUE(SENSOR_SPEC_ATTR_EXISTS(zephyr_test_sensor, attr1, foo));
	EXPECT_TRUE(SENSOR_SPEC_ATTR_EXISTS(zephyr_test_sensor, attr0, bar));
}
