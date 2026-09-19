/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/d7s.h>
#include <zephyr/ztest.h>

#include "d7s.h"

#define D7S_NODE DT_ALIAS(d7s)

#define INT1_PIN DT_GPIO_PIN(D7S_NODE, int1_gpios)
#define INT2_PIN DT_GPIO_PIN(D7S_NODE, int2_gpios)

static const struct device *const dev = DEVICE_DT_GET(D7S_NODE);
static const struct emul *const target = EMUL_DT_GET(D7S_NODE);
static const struct device *const gpio = DEVICE_DT_GET(DT_GPIO_CTLR(D7S_NODE, int1_gpios));

static struct {
	int threshold;
	int drdy;
} fired;

/* Sampled before any test mutates CTRL, so it still reflects what init wrote. */
static uint8_t init_ctrl;

static void trigger_handler(const struct device *sensor, const struct sensor_trigger *trig)
{
	ARG_UNUSED(sensor);

	switch (trig->type) {
	case SENSOR_TRIG_THRESHOLD:
		fired.threshold++;
		break;
	case SENSOR_TRIG_DATA_READY:
		fired.drdy++;
		break;
	default:
		break;
	}
}

static void *d7s_setup(void)
{
	zassert_true(device_is_ready(dev), "device not ready");
	zassert_true(device_is_ready(gpio), "gpio not ready");

	init_ctrl = d7s_emul_get_ctrl(target);

	return NULL;
}

static void d7s_before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&fired, 0, sizeof(fired));

	d7s_emul_set_state(target, D7S_STATE_NORMAL_STANDBY);
	d7s_emul_set_event(target, 0);
	d7s_emul_set_record(target, 0, 0, 0, 0);

	/* Both pins are active low, so idle is physically high. */
	zassert_ok(gpio_emul_input_set(gpio, INT1_PIN, 1));
	zassert_ok(gpio_emul_input_set(gpio, INT2_PIN, 1));
}

ZTEST(d7s, test_init_applies_devicetree_ctrl)
{
	/* The binding defaults are "switch-at-install" and threshold "high". */
	zassert_equal(FIELD_GET(D7S_CTRL_AXIS_MASK, init_ctrl), D7S_AXIS_MODE_SWITCH_AT_INSTALL);
	zassert_equal(init_ctrl & D7S_CTRL_THRESHOLD, 0);
}

ZTEST(d7s, test_fetch_scales_to_tenths)
{
	struct sensor_value val;

	/* 123 -> 12.3 kine, 4567 -> 456.7 gal, -251 -> -25.1 degrees C. */
	d7s_emul_set_record(target, 0, 123, 4567, -251);

	zassert_ok(sensor_sample_fetch(dev));

	zassert_ok(sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_D7S_SI, &val));
	zassert_equal(val.val1, 12);
	zassert_equal(val.val2, 300000);

	zassert_ok(sensor_channel_get(dev, (enum sensor_channel)SENSOR_CHAN_D7S_PGA, &val));
	zassert_equal(val.val1, 456);
	zassert_equal(val.val2, 700000);

	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_DIE_TEMP, &val));
	zassert_equal(val.val1, -25);
	zassert_equal(val.val2, -100000);
}

ZTEST(d7s, test_unsupported_channel)
{
	struct sensor_value val;

	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &val), -ENOTSUP);
	zassert_equal(sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_X), -ENOTSUP);
}

ZTEST(d7s, test_event_read_clears)
{
	struct sensor_value val;

	d7s_emul_set_event(target, D7S_EVENT_SHUTOFF | D7S_EVENT_COLLAPSE);

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_EVENT, &val));
	zassert_equal(val.val1, D7S_EVENT_SHUTOFF | D7S_EVENT_COLLAPSE);

	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_EVENT, &val));
	zassert_equal(val.val1, 0, "EVENT must be cleared by the read that reports it");
}

ZTEST(d7s, test_attr_axis_mode_roundtrip)
{
	struct sensor_value val = {.val1 = D7S_AXIS_MODE_XZ};

	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_AXIS_MODE, &val));

	val.val1 = 0;
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_AXIS_MODE, &val));
	zassert_equal(val.val1, D7S_AXIS_MODE_XZ);

	val.val1 = D7S_AXIS_MODE_SWITCH_AT_INSTALL + 1;
	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_ALL,
				      (enum sensor_attribute)SENSOR_ATTR_D7S_AXIS_MODE, &val),
		      -EINVAL);
}

ZTEST(d7s, test_attr_shutoff_threshold_roundtrip)
{
	struct sensor_value val = {.val1 = D7S_SHUTOFF_THRESHOLD_LOW};

	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_SHUTOFF_THRESHOLD, &val));
	zassert_not_equal(d7s_emul_get_ctrl(target) & D7S_CTRL_THRESHOLD, 0);

	val.val1 = 0;
	zassert_ok(sensor_attr_get(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_SHUTOFF_THRESHOLD, &val));
	zassert_equal(val.val1, D7S_SHUTOFF_THRESHOLD_LOW);
}

ZTEST(d7s, test_mode_change_rejected_while_busy)
{
	struct sensor_value val = {0};

	d7s_emul_set_state(target, D7S_STATE_SELF_DIAGNOSTIC);

	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_ALL,
				      (enum sensor_attribute)SENSOR_ATTR_D7S_SELFTEST, &val),
		      -EBUSY);

	d7s_emul_set_state(target, D7S_STATE_NORMAL_STANDBY);
	zassert_ok(sensor_attr_set(dev, SENSOR_CHAN_ALL,
				   (enum sensor_attribute)SENSOR_ATTR_D7S_SELFTEST, &val));
}

ZTEST(d7s, test_trigger_threshold_on_int1_assert)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_ALL,
	};

	zassert_ok(sensor_trigger_set(dev, &trig, trigger_handler));

	zassert_ok(gpio_emul_input_set(gpio, INT1_PIN, 0));
	k_sleep(K_MSEC(10));

	zassert_equal(fired.threshold, 1);
	zassert_equal(fired.drdy, 0);

	zassert_ok(sensor_trigger_set(dev, &trig, NULL));
}

ZTEST(d7s, test_trigger_drdy_on_int2_release)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	zassert_ok(sensor_trigger_set(dev, &trig, trigger_handler));

	/* INT2 is asserted while the sensor calculates. */
	zassert_ok(gpio_emul_input_set(gpio, INT2_PIN, 0));
	k_sleep(K_MSEC(10));
	zassert_equal(fired.drdy, 0, "data must not be ready while the sensor is busy");

	zassert_ok(gpio_emul_input_set(gpio, INT2_PIN, 1));
	k_sleep(K_MSEC(10));
	zassert_equal(fired.drdy, 1);

	zassert_ok(sensor_trigger_set(dev, &trig, NULL));
}

ZTEST(d7s, test_trigger_unsupported)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	zassert_equal(sensor_trigger_set(dev, &trig, trigger_handler), -ENOTSUP);
}

ZTEST_SUITE(d7s, NULL, d7s_setup, d7s_before, NULL, NULL);
