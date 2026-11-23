/*
 * Copyright (c) 2026 Nicolas Belin <nbelin@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

/* The overlay sets pulses-per-liter = 1000, which makes the math easy:
 * 1000 pulses = 1 liter = 1000 mL.
 */
#define PULSES_PER_LITER 1000

#define FLOW_METER      DT_NODELABEL(flow_meter_0)
#define PULSE_GPIO_CTLR DT_GPIO_CTLR(FLOW_METER, pulse_gpios)
#define PULSE_GPIO_PIN  DT_GPIO_PIN(FLOW_METER, pulse_gpios)

struct flow_meter_fixture {
	const struct device *dev;
	const struct device *gpio_dev;
	bool volume_trigger_fired;
};

static struct flow_meter_fixture *current_fixture;

static void generate_pulses_us(const struct device *gpio_dev, uint32_t pin, uint32_t count,
			       uint32_t half_period_us)
{
	for (uint32_t i = 0; i < count; i++) {
		gpio_emul_input_set(gpio_dev, pin, 1);
		k_busy_wait(half_period_us);
		gpio_emul_input_set(gpio_dev, pin, 0);
		k_busy_wait(half_period_us);
	}
}

static void generate_pulses(const struct device *gpio_dev, uint32_t pin, uint32_t count)
{
	generate_pulses_us(gpio_dev, pin, count, 50);
}

static uint32_t get_volume_mL(const struct device *dev)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(dev));
	zassert_ok(sensor_channel_get(dev, SENSOR_CHAN_VOLUME, &val));
	/* Convert sensor_value (liters) back to mL for easy comparison.
	 * Sub-milliliter remainder is truncated.
	 */
	return (uint32_t)(val.val1 * 1000 + val.val2 / 1000);
}

static void *flow_meter_setup(void)
{
	static struct flow_meter_fixture fixture = {
		.dev = DEVICE_DT_GET(FLOW_METER),
		.gpio_dev = DEVICE_DT_GET(PULSE_GPIO_CTLR),
	};

	zassert_true(device_is_ready(fixture.dev), "flow meter device not ready");
	zassert_true(device_is_ready(fixture.gpio_dev), "GPIO device not ready");

	return &fixture;
}

static void flow_meter_before(void *f)
{
	struct flow_meter_fixture *fixture = f;

	current_fixture = fixture;
	fixture->volume_trigger_fired = false;

	/* Restore default calibration. */
	struct sensor_value cal_val = {.val1 = PULSES_PER_LITER};

	sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CALIBRATION, &cal_val);
}

static void flow_meter_after(void *f)
{
	struct flow_meter_fixture *fixture = f;
	static const struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	/* Disarm any trigger left armed by a test. */
	sensor_trigger_set(fixture->dev, &trig, NULL);
}

ZTEST_SUITE(flow_meter, NULL, flow_meter_setup, flow_meter_before, flow_meter_after, NULL);

ZTEST_F(flow_meter, test_wrong_channel_returns_enotsup)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_equal(-ENOTSUP, sensor_channel_get(fixture->dev, SENSOR_CHAN_ACCEL_X, &val));
}

ZTEST_F(flow_meter, test_pulse_counting_volume)
{
	/* SENSOR_CHAN_VOLUME is cumulative — measure the delta across this test. */
	uint32_t before = get_volume_mL(fixture->dev);

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 1000);

	uint32_t after = get_volume_mL(fixture->dev);

	zassert_equal(1000, after - before, "expected +1000 mL, got +%u", after - before);
}

ZTEST_F(flow_meter, test_fractional_liter_conversion)
{
	/* 1750 pulses * 1000 µL = 1,750,000 µL = 1.75 L
	 * Verifies val1 and val2 are both set correctly.
	 */
	struct sensor_value before_val, after_val;

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLUME, &before_val));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 1750);

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLUME, &after_val));

	uint64_t before_uL = (uint64_t)before_val.val1 * 1000000 + before_val.val2;
	uint64_t after_uL = (uint64_t)after_val.val1 * 1000000 + after_val.val2;
	uint64_t delta_uL = after_uL - before_uL;

	zassert_equal(1, (int32_t)(delta_uL / 1000000), "expected 1 L, got %d",
		      (int32_t)(delta_uL / 1000000));
	zassert_equal(750000, (int32_t)(delta_uL % 1000000), "expected 750000 µL, got %d",
		      (int32_t)(delta_uL % 1000000));
}

ZTEST_F(flow_meter, test_calibration_change)
{
	/* Switch to 500 p/L so 500 pulses = 1 L = 1000 mL */
	struct sensor_value attr_val = {.val1 = 500};

	zassert_ok(
		sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CALIBRATION, &attr_val));

	uint32_t before = get_volume_mL(fixture->dev);

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 500);

	uint32_t after = get_volume_mL(fixture->dev);

	zassert_equal(1000, after - before, "expected +1000 mL with 500 p/L, got +%u",
		      after - before);
}

ZTEST_F(flow_meter, test_invalid_calibration_returns_einval)
{
	struct sensor_value attr_val = {.val1 = 0};

	zassert_equal(-EINVAL, sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL,
					       SENSOR_ATTR_CALIBRATION, &attr_val));
}

ZTEST_F(flow_meter, test_attr_get_pulses_per_liter)
{
	struct sensor_value set_val = {.val1 = 300};
	struct sensor_value get_val;

	zassert_ok(
		sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CALIBRATION, &set_val));
	zassert_ok(
		sensor_attr_get(fixture->dev, SENSOR_CHAN_ALL, SENSOR_ATTR_CALIBRATION, &get_val));
	zassert_equal(300, get_val.val1);
}

ZTEST_F(flow_meter, test_rate_zero_without_flow)
{
	struct sensor_value val;

	zassert_ok(sensor_sample_fetch(fixture->dev));
	k_sleep(K_MSEC(50));
	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_FLOW_RATE, &val));
	zassert_equal(0, val.val1);
	zassert_equal(0, val.val2);
}

ZTEST_F(flow_meter, test_rate_nonzero_with_flow)
{
	struct sensor_value val;

	/* Establish a baseline fetch. */
	zassert_ok(sensor_sample_fetch(fixture->dev));

	/* 100 pulses * 2 * 500µs = 100ms. fetch immediately after — no sleep.
	 * Expected rate: 100 * 1000µL / 100ms = 1.0 L/s = 60 L/min.
	 */
	generate_pulses_us(fixture->gpio_dev, PULSE_GPIO_PIN, 100, 500);

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_FLOW_RATE, &val));

	int32_t rate_mL_min = val.val1 * 1000 + val.val2 / 1000;

	zassert_between_inclusive(rate_mL_min, 57000, 63000,
				  "rate %d mL/min outside expected range [57000, 63000]",
				  rate_mL_min);
}

static void volume_trigger_cb(const struct device *dev, const struct sensor_trigger *trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(trig);
	current_fixture->volume_trigger_fired = true;
}

ZTEST_F(flow_meter, test_volume_threshold_trigger)
{
	struct sensor_value threshold = {.val1 = 0, .val2 = 500000}; /* 0.5 L */
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 400);
	k_sleep(K_MSEC(20));
	zassert_false(fixture->volume_trigger_fired, "trigger fired too early");

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 150);
	k_sleep(K_MSEC(20));
	zassert_true(fixture->volume_trigger_fired, "trigger did not fire");
}

ZTEST_F(flow_meter, test_volume_threshold_one_shot)
{
	struct sensor_value threshold = {.val1 = 0, .val2 = 100000}; /* 0.1 L */
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 150);
	k_sleep(K_MSEC(20));
	zassert_true(fixture->volume_trigger_fired, "trigger did not fire");

	fixture->volume_trigger_fired = false;
	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 200);
	k_sleep(K_MSEC(20));
	zassert_false(fixture->volume_trigger_fired, "one-shot trigger fired twice");
}

ZTEST_F(flow_meter, test_volume_threshold_rearm)
{
	struct sensor_value threshold = {.val1 = 0, .val2 = 100000}; /* 0.1 L */
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 150);
	k_sleep(K_MSEC(20));
	zassert_true(fixture->volume_trigger_fired, "trigger did not fire");

	/* Re-arm with the handler still registered. */
	fixture->volume_trigger_fired = false;
	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 150);
	k_sleep(K_MSEC(20));
	zassert_true(fixture->volume_trigger_fired, "re-armed trigger did not fire");
}

ZTEST_F(flow_meter, test_volume_threshold_disarm)
{
	struct sensor_value threshold = {.val1 = 0, .val2 = 100000}; /* 0.1 L */
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));

	/* Disarm by registering a NULL handler. */
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, NULL));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 200);
	k_sleep(K_MSEC(20));
	zassert_false(fixture->volume_trigger_fired, "disarmed trigger fired");
}

ZTEST_F(flow_meter, test_attr_get_upper_thresh)
{
	struct sensor_value threshold = {.val1 = 0, .val2 = 500000}; /* 0.5 L */
	struct sensor_value readback;
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));
	zassert_ok(sensor_attr_get(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &readback));
	zassert_equal(0, readback.val1);
	zassert_equal(500000, readback.val2);

	/* After firing, the one-shot threshold reads back as disarmed. */
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));
	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 600);
	k_sleep(K_MSEC(20));
	zassert_true(fixture->volume_trigger_fired, "trigger did not fire");

	zassert_ok(sensor_attr_get(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &readback));
	zassert_equal(0, readback.val1);
	zassert_equal(0, readback.val2);
}

ZTEST_F(flow_meter, test_sub_pulse_threshold_clamped_to_one_pulse)
{
	/* 500 µL is half a pulse at 1000 p/L — clamped to one pulse. */
	struct sensor_value threshold = {.val1 = 0, .val2 = 500};
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLUME,
	};

	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME, SENSOR_ATTR_UPPER_THRESH,
				   &threshold));
	zassert_ok(sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));

	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 1);
	k_sleep(K_MSEC(20));
	zassert_true(fixture->volume_trigger_fired, "single pulse did not fire trigger");
}

ZTEST_F(flow_meter, test_invalid_threshold_returns_einval)
{
	struct sensor_value bad;

	bad.val1 = -1;
	bad.val2 = 0;
	zassert_equal(-EINVAL, sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME,
					       SENSOR_ATTR_UPPER_THRESH, &bad));

	bad.val1 = 0;
	bad.val2 = -1;
	zassert_equal(-EINVAL, sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME,
					       SENSOR_ATTR_UPPER_THRESH, &bad));

	bad.val1 = 0;
	bad.val2 = 1000000;
	zassert_equal(-EINVAL, sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLUME,
					       SENSOR_ATTR_UPPER_THRESH, &bad));
}

ZTEST_F(flow_meter, test_attr_wrong_channel_returns_enotsup)
{
	struct sensor_value val = {.val1 = 100};

	zassert_equal(-ENOTSUP, sensor_attr_set(fixture->dev, SENSOR_CHAN_ACCEL_X,
						SENSOR_ATTR_CALIBRATION, &val));
	zassert_equal(-ENOTSUP, sensor_attr_get(fixture->dev, SENSOR_CHAN_ACCEL_X,
						SENSOR_ATTR_CALIBRATION, &val));
	zassert_equal(-ENOTSUP, sensor_attr_set(fixture->dev, SENSOR_CHAN_ALL,
						SENSOR_ATTR_UPPER_THRESH, &val));
}

ZTEST_F(flow_meter, test_fetch_wrong_channel_returns_enotsup)
{
	zassert_equal(-ENOTSUP, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_ACCEL_X));
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_FLOW_RATE));
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_VOLUME));
}

ZTEST_F(flow_meter, test_unsupported_trigger_returns_enotsup)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};

	zassert_equal(-ENOTSUP, sensor_trigger_set(fixture->dev, &trig, volume_trigger_cb));
}

ZTEST(flow_meter, test_power_gpio_enabled_at_init)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(flow_meter_1));
	const struct device *gpio_dev =
		DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(flow_meter_1), power_gpios));

	zassert_true(device_is_ready(dev), "flow meter with power-gpios not ready");
	zassert_equal(1,
		      gpio_emul_output_get(gpio_dev,
					   DT_GPIO_PIN(DT_NODELABEL(flow_meter_1), power_gpios)),
		      "power GPIO not asserted after init");
}

ZTEST_F(flow_meter, test_instances_count_independently)
{
	const struct device *dev1 = DEVICE_DT_GET(DT_NODELABEL(flow_meter_1));
	uint32_t pin1 = DT_GPIO_PIN(DT_NODELABEL(flow_meter_1), pulse_gpios);

	uint32_t before0 = get_volume_mL(fixture->dev);
	uint32_t before1 = get_volume_mL(dev1);

	/* Pulse only the second instance. */
	generate_pulses(fixture->gpio_dev, pin1, 1000);

	zassert_equal(0, get_volume_mL(fixture->dev) - before0,
		      "instance 0 counted pulses meant for instance 1");
	zassert_equal(1000, get_volume_mL(dev1) - before1,
		      "instance 1 did not count its own pulses");

	/* Pulse only the first instance. */
	generate_pulses(fixture->gpio_dev, PULSE_GPIO_PIN, 500);

	zassert_equal(500, get_volume_mL(fixture->dev) - before0,
		      "instance 0 did not count its own pulses");
	zassert_equal(1000, get_volume_mL(dev1) - before1,
		      "instance 1 counted pulses meant for instance 0");
}
