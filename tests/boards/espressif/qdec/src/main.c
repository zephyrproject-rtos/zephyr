/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ESP32 PCNT hardware self-test. The overlay wires four PCNT units so
 * that software-driven pulse pads feed back into the PCNT input matrix
 * without external wiring (each pulse pad is configured as both input
 * and output, so the GPIO matrix routes the software-driven level into
 * PCNT).
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/pcnt_esp32.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/rtio/rtio.h>

#define QDEC_NODE DT_ALIAS(qdec0)

BUILD_ASSERT(DT_NODE_EXISTS(QDEC_NODE),
	     "Board overlay must provide a 'qdec0' alias pointing to the "
	     "multi-unit PCNT node.");

#define QDEC_UNIT_CHAN(node_id)                                                                    \
	{.chan_type = SENSOR_CHAN_ENCODER_COUNT, .chan_idx = DT_REG_ADDR(node_id)},

SENSOR_DT_READ_IODEV(qdec_iodev, QDEC_NODE, DT_FOREACH_CHILD(QDEC_NODE, QDEC_UNIT_CHAN));

RTIO_DEFINE_WITH_MEMPOOL(qdec_rtio, 4, 4, 4, 256, sizeof(void *));

/* clang-format off */
static const uint8_t unit_indices[] = {
	DT_FOREACH_CHILD_SEP(QDEC_NODE, DT_REG_ADDR, (,))
};
/* clang-format on */

#define NUM_UNITS ARRAY_SIZE(unit_indices)

static const struct gpio_dt_spec pulse0 = GPIO_DT_SPEC_GET(DT_ALIAS(pulse0), gpios);
static const struct gpio_dt_spec pulse0b = GPIO_DT_SPEC_GET(DT_ALIAS(pulse0b), gpios);
static const struct gpio_dt_spec pulse1 = GPIO_DT_SPEC_GET(DT_ALIAS(pulse1), gpios);
static const struct gpio_dt_spec pulse2 = GPIO_DT_SPEC_GET(DT_ALIAS(pulse2), gpios);
static const struct gpio_dt_spec pulse3 = GPIO_DT_SPEC_GET(DT_ALIAS(pulse3), gpios);

static const struct gpio_dt_spec *all_pulses[] = {
	&pulse0, &pulse0b, &pulse1, &pulse2, &pulse3,
};

static volatile uint32_t trig_hits[4];

static void trig0_handler(const struct device *d, const struct sensor_trigger *t)
{
	ARG_UNUSED(d);
	ARG_UNUSED(t);
	trig_hits[0]++;
}
static void trig1_handler(const struct device *d, const struct sensor_trigger *t)
{
	ARG_UNUSED(d);
	ARG_UNUSED(t);
	trig_hits[1]++;
}
static void trig2_handler(const struct device *d, const struct sensor_trigger *t)
{
	ARG_UNUSED(d);
	ARG_UNUSED(t);
	trig_hits[2]++;
}

static const struct device *dev;
static const struct sensor_decoder_api *decoder;

static int read_counts(int32_t *counts)
{
	uint8_t buf[256];
	int rc = sensor_read(&qdec_iodev, &qdec_rtio, buf, sizeof(buf));

	if (rc != 0) {
		return rc;
	}

	for (size_t i = 0; i < NUM_UNITS; i++) {
		struct sensor_chan_spec spec = {
			.chan_type = SENSOR_CHAN_ENCODER_COUNT,
			.chan_idx = unit_indices[i],
		};
		struct sensor_q31_data data = {0};
		uint32_t fit = 0;

		rc = decoder->decode(buf, spec, &fit, 1, &data);
		if (rc < 1) {
			return -EIO;
		}
		counts[i] = data.readings[0].value;
	}
	return 0;
}

static int read_rotation_q31(uint16_t chan_idx, int32_t *q, int8_t *shift)
{
	uint8_t buf[256];
	int rc = sensor_read(&qdec_iodev, &qdec_rtio, buf, sizeof(buf));

	if (rc != 0) {
		return rc;
	}
	struct sensor_chan_spec spec = {
		.chan_type = SENSOR_CHAN_ROTATION,
		.chan_idx = chan_idx,
	};
	struct sensor_q31_data data = {0};
	uint32_t fit = 0;

	rc = decoder->decode(buf, spec, &fit, 1, &data);
	if (rc < 1) {
		return -EIO;
	}
	*q = data.readings[0].value;
	*shift = data.shift;
	return 0;
}

static void reset_all(void)
{
	struct sensor_value v = {0};

	for (size_t i = 0; i < NUM_UNITS; i++) {
		sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(unit_indices[i]),
				SENSOR_ATTR_OFFSET, &v);
		sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(unit_indices[i]),
				SENSOR_ATTR_PCNT_ESP32_RESET, &v);
	}
}

static void drive_edges(const struct gpio_dt_spec *p, int n_rising, uint32_t period_ms)
{
	for (int i = 0; i < 2 * n_rising; i++) {
		gpio_pin_set_dt(p, i & 1);
		k_msleep(period_ms);
	}
	gpio_pin_set_dt(p, 0);
}

static void *qdec_suite_setup(void)
{
	dev = DEVICE_DT_GET(QDEC_NODE);
	zassert_true(device_is_ready(dev), "PCNT device not ready");
	zassert_equal(sensor_get_decoder(dev, &decoder), 0, "decoder not returned");

	/*
	 * Configure every pulse pad as both output and input. The driver's
	 * pinctrl already routed the pad to the PCNT peripheral via the
	 * input matrix, so keeping the input buffer live lets PCNT observe
	 * the software-driven output level - no external wiring required.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(all_pulses); i++) {
		int rc = gpio_pin_configure_dt(all_pulses[i], GPIO_INPUT | GPIO_OUTPUT_INACTIVE);

		zassert_equal(rc, 0, "gpio pulse%zu configure failed", i);
	}
	reset_all();
	return NULL;
}

static void qdec_before(void *f)
{
	ARG_UNUSED(f);
	reset_all();
	for (size_t i = 0; i < ARRAY_SIZE(trig_hits); i++) {
		trig_hits[i] = 0;
	}
}

ZTEST(qdec_selfloop, test_01_baseline)
{
	int32_t counts[4] = {0};

	zassert_equal(read_counts(counts), 0);
	for (size_t i = 0; i < NUM_UNITS; i++) {
		zassert_equal(counts[i], 0, "unit %u not zero", unit_indices[i]);
	}
}

ZTEST(qdec_selfloop, test_02_per_unit_offset)
{
	const int32_t offsets[] = {111, 222, 333, 444};
	int32_t counts[4] = {0};
	struct sensor_value v = {0};

	for (size_t i = 0; i < NUM_UNITS; i++) {
		v.val1 = offsets[i];
		zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(unit_indices[i]),
					      SENSOR_ATTR_OFFSET, &v),
			      0);
	}
	zassert_equal(read_counts(counts), 0);
	for (size_t i = 0; i < NUM_UNITS; i++) {
		zassert_equal(counts[i], offsets[i], "unit %u: got %d, expected %d",
			      unit_indices[i], counts[i], offsets[i]);
	}
}

ZTEST(qdec_selfloop, test_03_attr_get_roundtrip)
{
	struct sensor_value v = {0};

	v.val1 = 555;
	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(1), SENSOR_ATTR_OFFSET, &v),
		      0);
	v.val1 = 0;
	zassert_equal(sensor_attr_get(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(1), SENSOR_ATTR_OFFSET, &v),
		      0);
	zassert_equal(v.val1, 555, "attr_get returned %d", v.val1);
}

ZTEST(qdec_selfloop, test_04_reset_clears_counter)
{
	int32_t counts[4] = {0};
	struct sensor_value v = {0};

	drive_edges(&pulse0, 5, 2);
	zassert_equal(read_counts(counts), 0);
	zassert_true(counts[0] >= 5, "counter only reached %d", counts[0]);

	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(0),
				      SENSOR_ATTR_PCNT_ESP32_RESET, &v),
		      0);
	zassert_equal(read_counts(counts), 0);
	zassert_equal(counts[0], 0, "counter did not clear (got %d)", counts[0]);
}

ZTEST(qdec_selfloop, test_05_legacy_channel_get)
{
	struct sensor_value v;

	v.val1 = 777;
	v.val2 = 0;
	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(0), SENSOR_ATTR_OFFSET, &v),
		      0);
	zassert_equal(sensor_sample_fetch(dev), 0);
	zassert_equal(sensor_channel_get(dev, SENSOR_CHAN_ENCODER_COUNT, &v), 0);
	zassert_equal(v.val1, 777, "legacy read returned %d", v.val1);
}

ZTEST(qdec_selfloop, test_06_unsupported_channel)
{
	struct sensor_value v = {0};
	int rc = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_X, SENSOR_ATTR_OFFSET, &v);

	zassert_equal(rc, -ENOTSUP, "got rc=%d", rc);
}

ZTEST(qdec_selfloop, test_07_independent_counts)
{
	int32_t counts[4] = {0};
	const int expected[] = {20, 10, 7, 3};

	drive_edges(&pulse0, expected[0], 2);
	drive_edges(&pulse1, expected[1], 2);
	drive_edges(&pulse2, expected[2], 2);
	drive_edges(&pulse3, expected[3], 2);

	zassert_equal(read_counts(counts), 0);
	for (size_t i = 0; i < NUM_UNITS; i++) {
		zassert_equal(counts[i], expected[i], "unit %u: got %d, expected %d",
			      unit_indices[i], counts[i], expected[i]);
	}
}

ZTEST(qdec_selfloop, test_08_upper_thresh_trigger)
{
	static const struct sensor_trigger trig_u0 = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_ROTATION,
	};
	static const struct sensor_trigger trig_u1 = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_PCNT_ESP32_UNIT(1),
	};
	struct sensor_value v;

	v.val1 = 3;
	v.val2 = 0;
	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_ROTATION, SENSOR_ATTR_UPPER_THRESH, &v), 0);
	v.val1 = 2;
	zassert_equal(
		sensor_attr_set(dev, SENSOR_CHAN_PCNT_ESP32_UNIT(1), SENSOR_ATTR_UPPER_THRESH, &v),
		0);

	zassert_equal(sensor_trigger_set(dev, &trig_u0, trig0_handler), 0);
	zassert_equal(sensor_trigger_set(dev, &trig_u1, trig1_handler), 0);

	drive_edges(&pulse0, 5, 2);
	drive_edges(&pulse1, 5, 2);
	k_msleep(10);

	zassert_true(trig_hits[0] >= 1, "u0 hits=%u", trig_hits[0]);
	zassert_true(trig_hits[1] >= 1, "u1 hits=%u", trig_hits[1]);
}

ZTEST(qdec_selfloop, test_10_lower_thresh)
{
	static const struct sensor_trigger trig_u0 = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_ROTATION,
	};
	struct sensor_value v;

	v.val1 = 4;
	v.val2 = 0;
	zassert_equal(sensor_attr_set(dev, SENSOR_CHAN_ROTATION, SENSOR_ATTR_LOWER_THRESH, &v), 0);
	zassert_equal(sensor_trigger_set(dev, &trig_u0, trig0_handler), 0);
	drive_edges(&pulse0, 6, 2);
	k_msleep(10);

	v.val1 = 0;
	zassert_equal(sensor_attr_get(dev, SENSOR_CHAN_ROTATION, SENSOR_ATTR_LOWER_THRESH, &v), 0);
	zassert_equal(v.val1, 4);
	zassert_true(trig_hits[0] >= 1, "u0 hits=%u", trig_hits[0]);
}

ZTEST(qdec_selfloop, test_11_four_units_concurrent)
{
	int32_t counts[4] = {0};

	for (int i = 0; i < 20; i++) {
		int level = i & 1;

		gpio_pin_set_dt(&pulse0, level);
		gpio_pin_set_dt(&pulse1, level);
		gpio_pin_set_dt(&pulse2, level);
		gpio_pin_set_dt(&pulse3, level);
		k_msleep(2);
	}
	zassert_equal(read_counts(counts), 0);
	zassert_equal(counts[0], 10, "u0=%d", counts[0]);
	zassert_equal(counts[1], 10, "u1=%d", counts[1]);
	zassert_true(counts[2] >= 0 && counts[2] <= 10, "u2=%d", counts[2]);
	zassert_equal(counts[3], 10, "u3=%d", counts[3]);
}

ZTEST(qdec_selfloop, test_12_rotation_degrees)
{
	int32_t q;
	int8_t shift;

	drive_edges(&pulse0, 100, 1);
	zassert_equal(read_rotation_q31(0, &q, &shift), 0);
	zassert_equal(shift, 9, "shift=%d", shift);
	/* 100/400 * 360 = 90 deg; q ~= 90 * 2^22 */
	int32_t expected = 90 * (int32_t)(1 << 22);
	int32_t diff = q - expected;

	if (diff < 0) {
		diff = -diff;
	}
	zassert_true(diff < (1 << 22), "q=%d expected ~%d (diff %d)", q, expected, diff);
}

ZTEST(qdec_selfloop, test_13_high_limit_auto_reset)
{
	static const struct sensor_trigger trig_u2 = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_PCNT_ESP32_UNIT(2),
	};
	int32_t counts[4] = {0};

	zassert_equal(sensor_trigger_set(dev, &trig_u2, trig2_handler), 0);
	drive_edges(&pulse2, 25, 2);
	k_msleep(10);

	zassert_equal(read_counts(counts), 0);
	zassert_equal(counts[2], 5, "expected 25 %% 10 = 5 after two auto-resets, got %d",
		      counts[2]);
	zassert_true(trig_hits[2] >= 2, "expected H_LIM >= 2 hits, got %u", trig_hits[2]);
}

ZTEST(qdec_selfloop, test_14_zero_cross_unit_counts)
{
	int32_t counts[4] = {0};

	drive_edges(&pulse3, 8, 2);
	zassert_equal(read_counts(counts), 0);
	zassert_equal(counts[3], 8, "u3=%d", counts[3]);
}

ZTEST(qdec_selfloop, test_15_quadrature_direction)
{
	int32_t counts[4] = {0};

	drive_edges(&pulse0, 10, 2); /* CH0 +1 per rising -> +10 */
	drive_edges(&pulse0b, 7, 2); /* CH1 -1 per rising ->  -7 */

	zassert_equal(read_counts(counts), 0);
	zassert_equal(counts[0], 3, "net should be +3, got %d", counts[0]);
}

ZTEST_SUITE(qdec_selfloop, NULL, qdec_suite_setup, qdec_before, NULL, NULL);
