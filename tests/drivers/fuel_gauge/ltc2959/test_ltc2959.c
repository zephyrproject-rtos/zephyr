#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>


struct ltc2959_fixture {
	const struct device *dev;
	const struct fuel_gauge_driver_api *api;
};

static void *ltc2959_setup(void)
{
	static ZTEST_DMEM struct ltc2959_fixture fixture;

	fixture.dev = DEVICE_DT_GET_ANY(adi_ltc2959);
	k_object_access_all_grant(fixture.dev);

	zassume_true(device_is_ready(fixture.dev), "Fuel Gauge not found");

	return &fixture;
}

LOG_MODULE_REGISTER(test_ltc2959, LOG_LEVEL_INF);

ZTEST_F(ltc2959, test_get_props__returns_ok)
{
	fuel_gauge_prop_t props[] = {
		FUEL_GAUGE_STATUS,
		FUEL_GAUGE_VOLTAGE,
		FUEL_GAUGE_CURRENT,
		FUEL_GAUGE_TEMPERATURE,
	};

	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];
	int ret = fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props));

	#if CONFIG_EMUL
	zassert_equal(vals[0].fg_status, 0x01);
	zassert_equal(vals[1].voltage, 0x00);
	zassert_equal(vals[2].current, 0x00);
	zassert_equal(vals[3].temperature, 0x00);
	#else
	zassert_between_inclusive(vals[0].fg_status, 0, 0xFF);
	zassert_between_inclusive(vals[1].voltage, 0x00, 0xFFFF);
	#endif
	zassert_equal(ret, 0, "Getting bad property has a good status.");
}

ZTEST_F(ltc2959, test_set_get_single_prop)
{
	int ret;
	union fuel_gauge_prop_val val = {.voltage = 600000};

	ret = fuel_gauge_set_prop(fixture->dev, FUEL_GAUGE_LOW_VOLTAGE_ALARM, val);

	zassert_equal(ret, 0, "Failed to set high current threshold");

	union fuel_gauge_prop_val val1;

	ret = fuel_gauge_get_prop(fixture->dev, FUEL_GAUGE_LOW_VOLTAGE_ALARM, &val1);

	zassert_equal(ret, 0, "Failed to get low voltage threshold");
	LOG_INF("low voltage threshold: %d µV", val1.voltage);
}

ZTEST_USER_F(ltc2959, test_get_some_props_failed__returns_bad_status)
{
	fuel_gauge_prop_t props[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_VOLTAGE,
	};
	union fuel_gauge_prop_val vals[ARRAY_SIZE(props)];

	int ret = fuel_gauge_get_props(fixture->dev, props, vals, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP, "Getting bad property has a good status.");
}

ZTEST_F(ltc2959, test_set_some_props_failed__returns_err)
{
		fuel_gauge_prop_t prop_types[] = {
		/* First invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Second invalid property */
		FUEL_GAUGE_PROP_MAX,
		/* Valid property */
		FUEL_GAUGE_LOW_VOLTAGE_ALARM,
	};

	union fuel_gauge_prop_val props[] = {
		/* First invalid property */
		{0},
		/* Second invalid property */
		{0},
		/* Valid property */
		{.voltage = 0},
	};

	int ret = fuel_gauge_set_props(fixture->dev, prop_types, props, ARRAY_SIZE(props));

	zassert_equal(ret, -ENOTSUP);
}

ZTEST_SUITE(ltc2959, NULL, ltc2959_setup, NULL, NULL, NULL);
