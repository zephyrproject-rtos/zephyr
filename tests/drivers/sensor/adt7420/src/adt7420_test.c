/*
 * Copyright (c) 2026, Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor/adt7420.h>
#include <zephyr/ztest.h>

#include "adt7420_emul.h"

#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME test_adt7420_regs
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define ADT7420_TEST_CONFIG_FAULT_QUEUE_MASK	GENMASK(1, 0)
#define ADT7420_TEST_RESOLUTION_13_BIT_MICRO_C	7813 /* 0.0078125 */
#define ADT7420_TEST_RESOLUTION_16_BIT_MICRO_C	62500 /* 0.0625 */


struct adt7420_fixture {
	const struct device *dev;
	const struct emul *emul;
};

static void adt7420_after(void *data)
{
	const struct adt7420_fixture *fixture = data;

	adt7420_emul_reset(fixture->emul);
}

static void *adt7420_setup(void)
{
	static struct adt7420_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_ALIAS(adt7420)),
		.emul = EMUL_DT_GET(DT_ALIAS(adt7420)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.emul);
	zassert_true(device_is_ready(fixture.dev));

	return &fixture;
}

ZTEST_SUITE(adt7420, NULL, adt7420_setup, NULL, adt7420_after, NULL);

/* Attribute test */

/* test configuration via sensor_attr_set and sensor_attr_get */
ZTEST_F(adt7420, test_resolution)
{
	struct sensor_value sensor_val = {0};
	uint8_t target_val = 16;

	sensor_val.val1 = 1;
	zassert_equal(0,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_RESOLUTION, &sensor_val),
		      "Could not set sensor resolution");

	zassert_equal(0,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_RESOLUTION, &sensor_val),
		      "Could not get sensor resolution");

	zassert_equal(target_val, sensor_val.val1, "Expected %d, Received %d", target_val,
		      sensor_val.val1);
}

/* test all valid operation modes via SENSOR_ATTR_SAMPLING_FREQUENCY */
ZTEST_F(adt7420, test_sampling_frequency)
{
	struct sensor_value sensor_val = {0};

	for (uint8_t mode = ADT7420_OP_MODE_CONT_CONV; mode <= ADT7420_OP_MODE_SHUTDOWN; mode++) {
		sensor_val.val1 = mode;
		zassert_equal(0,
			      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
					      SENSOR_ATTR_SAMPLING_FREQUENCY, &sensor_val),
			      "Could not set sampling frequency mode %d", mode);

		zassert_equal(0,
			      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
					      SENSOR_ATTR_SAMPLING_FREQUENCY, &sensor_val),
			      "Could not get sampling frequency");

		zassert_equal(mode, sensor_val.val1, "Expected op mode %d, got %d", mode,
			      sensor_val.val1);
	}
}

/* test upper temperature threshold via SENSOR_ATTR_UPPER_THRESH */
ZTEST_F(adt7420, test_upper_threshold)
{
	struct sensor_value sensor_val = {0};

	sensor_val.val1 = 25;
	sensor_val.val2 = 0;
	zassert_equal(0,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_UPPER_THRESH, &sensor_val),
		      "Could not set upper threshold");

	zassert_equal(0,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_UPPER_THRESH, &sensor_val),
		      "Could not get upper threshold");

	zassert_equal(25, sensor_val.val1, "Expected 25 C, got %d", sensor_val.val1);
	zassert_equal(0, sensor_val.val2, "Expected val2 = 0, got %d", sensor_val.val2);
}

/* test lower temperature threshold via SENSOR_ATTR_LOWER_THRESH */
ZTEST_F(adt7420, test_lower_threshold)
{
	struct sensor_value sensor_val = {0};
	uint8_t target_val = 10;

	sensor_val.val1 = target_val;
	sensor_val.val2 = 0;
	zassert_equal(0,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_LOWER_THRESH, &sensor_val),
		      "Could not set lower threshold");

	zassert_equal(0,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_LOWER_THRESH, &sensor_val),
		      "Could not get lower threshold");

	zassert_equal(target_val, sensor_val.val1, "Expected 10 C, got %d", sensor_val.val1);
	zassert_equal(0, sensor_val.val2, "Expected val2 = 0, got %d", sensor_val.val2);
}

/* test lower temperature threshold via SENSOR_ATTR_LOWER_THRESH */
ZTEST_F(adt7420, test_lower_threshold_negative)
{
	struct sensor_value sensor_val = {0};
	int8_t target_val = -10;

	sensor_val.val1 = target_val;
	sensor_val.val2 = 0;
	zassert_equal(0,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_LOWER_THRESH, &sensor_val),
		      "Could not set lower threshold");

	zassert_equal(0,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_LOWER_THRESH, &sensor_val),
		      "Could not get lower threshold");

	zassert_equal(target_val, sensor_val.val1, "Expected 10 C, got %d", sensor_val.val1);
	zassert_equal(0, sensor_val.val2, "Expected val2 = 0, got %d", sensor_val.val2);
}

/* test hysteresis temperature via SENSOR_ATTR_HYSTERESIS */
ZTEST_F(adt7420, test_hysteresis)
{
	struct sensor_value sensor_val = {0};

	sensor_val.val1 = 5;
	zassert_equal(0,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_HYSTERESIS, &sensor_val),
		      "Could not set hysteresis");

	zassert_equal(0,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_HYSTERESIS, &sensor_val),
		      "Could not get hysteresis");

	zassert_equal(5, sensor_val.val1, "Expected hysteresis 5, got %d", sensor_val.val1);
}

/* test fault queue count via SENSOR_ATTR_FEATURE_MASK */
ZTEST_F(adt7420, test_fault_queue)
{
	struct sensor_value sensor_val = {0};
	uint8_t reg_val;

	sensor_val.val1 = ADT7420_FAULT_QUEUE_4_FAULTS;
	zassert_equal(0,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_FEATURE_MASK, &sensor_val),
		      "Could not set fault queue");

	adt7420_emul_get_reg(fixture->emul, ADT7420_REG_CONFIG, &reg_val);
	zassert_equal(ADT7420_FAULT_QUEUE_4_FAULTS,
		      FIELD_GET(ADT7420_TEST_CONFIG_FAULT_QUEUE_MASK, reg_val),
		      "Expected fault queue %d in CONFIG reg, got %d", ADT7420_FAULT_QUEUE_4_FAULTS,
		      FIELD_GET(ADT7420_TEST_CONFIG_FAULT_QUEUE_MASK, reg_val));

	zassert_equal(-ENOTSUP,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_FEATURE_MASK, &sensor_val),
		      "Expected -ENOTSUP for SENSOR_ATTR_FEATURE_MASK get");
}

/* test that an unsupported attribute returns -ENOTSUP for both set and get */
ZTEST_F(adt7420, test_unsupported_attr)
{
	struct sensor_value sensor_val = {0};

	zassert_equal(-ENOTSUP,
		      sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_OVERSAMPLING, &sensor_val),
		      "Expected -ENOTSUP for unsupported attr set");

	zassert_equal(-ENOTSUP,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP,
				      SENSOR_ATTR_OVERSAMPLING, &sensor_val),
		      "Expected -ENOTSUP for unsupported attr get");
}

/* test alert status flags via SENSOR_ATTR_ALERT */
ZTEST_F(adt7420, test_alert)
{
	struct sensor_value sensor_val = {0};
	uint8_t status = ADT7420_STATUS_T_HIGH | ADT7420_STATUS_T_CRIT;

	adt7420_emul_set_reg(fixture->emul, ADT7420_REG_STATUS, &status);

	zassert_equal(0,
		      sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_ALERT,
				      &sensor_val),
		      "Could not get alert status");

	zassert_equal(status, (uint8_t)sensor_val.val1, "Expected status 0x%02x, got 0x%02x",
		      status, sensor_val.val1);
}

/* Channel set and sample fetch tests */

ZTEST_F(adt7420, test_fetch_temp_13bit)
{
	struct sensor_value sensor_val = {0};
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP, .chan_idx = 0};
	q31_t lower, upper, epsilon;
	int8_t shift;

	/* Get the sample range to know valid shift/epsilon */
	zassert_ok(emul_sensor_backend_get_sample_range(fixture->emul, ch, &lower, &upper, &epsilon,
							&shift));

	/* Set 25.0C: q31 = (25 * (INT32_MAX+1)) >> shift */
	q31_t temp_25c = ((int64_t)25 * BIT(31)) >> shift;

	zassert_ok(emul_sensor_backend_set_channel(fixture->emul, ch, &temp_25c, shift));

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &sensor_val));

	/* 13-bit resolution: 1/16 °C = 0.0625 °C granularity */
	zassert_equal(25, sensor_val.val1, "Expected 25 C, got %d", sensor_val.val1);
	zassert_within(0, sensor_val.val2, ADT7420_TEST_RESOLUTION_16_BIT_MICRO_C,
		       "Expected val2 near 0, got %d", sensor_val.val2);
}

ZTEST_F(adt7420, test_fetch_temp_16bit)
{
	struct sensor_value sensor_val = {0};
	struct sensor_value resolution = {.val1 = 1, .val2 = 0};
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP, .chan_idx = 0};
	q31_t lower, upper, epsilon;
	int8_t shift;

	/* Switch to 16-bit resolution */
	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_RESOLUTION,
				   &resolution));

	zassert_ok(emul_sensor_backend_get_sample_range(fixture->emul, ch, &lower, &upper, &epsilon,
							&shift));

	/* Set 25.5C */
	q31_t temp_25_5c = ((int64_t)25500 * BIT(31) / 1000) >> shift;

	zassert_ok(emul_sensor_backend_set_channel(fixture->emul, ch, &temp_25_5c, shift));

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &sensor_val));

	/* 16-bit resolution: 1/128 °C = 0.0078125 °C granularity */
	zassert_equal(25, sensor_val.val1, "Expected 25 C, got %d", sensor_val.val1);
	zassert_within(500000, sensor_val.val2, ADT7420_TEST_RESOLUTION_13_BIT_MICRO_C,
		       "Expected val2 near 500000, got %d", sensor_val.val2);
}

ZTEST_F(adt7420, test_fetch_negative_temp_13_bit)
{
	struct sensor_value sensor_val = {0};
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP, .chan_idx = 0};
	q31_t lower, upper, epsilon;
	int8_t shift;

	zassert_ok(emul_sensor_backend_get_sample_range(fixture->emul, ch, &lower, &upper, &epsilon,
							&shift));

	/* Set -10.0C */
	q31_t temp_neg10c = ((int64_t)(-10) * BIT(31)) >> shift;

	zassert_ok(emul_sensor_backend_set_channel(fixture->emul, ch, &temp_neg10c, shift));

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &sensor_val));

	zassert_equal(-10, sensor_val.val1, "Expected -10 C, got %d", sensor_val.val1);
	zassert_within(0, sensor_val.val2, ADT7420_TEST_RESOLUTION_13_BIT_MICRO_C,
		       "Expected val2 near 0, got %d", sensor_val.val2);
}

ZTEST_F(adt7420, test_fetch_negative_temp_16_bit)
{
	struct sensor_value sensor_val = {0};
	struct sensor_value resolution = {.val1 = 1, .val2 = 0};
	struct sensor_chan_spec ch = {.chan_type = SENSOR_CHAN_AMBIENT_TEMP, .chan_idx = 0};
	q31_t lower, upper, epsilon;
	int8_t shift;

	/* Switch to 16-bit resolution */
	zassert_ok(sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_RESOLUTION,
				   &resolution));

	zassert_ok(emul_sensor_backend_get_sample_range(fixture->emul, ch, &lower, &upper, &epsilon,
							&shift));

	/* Set -25.5C */
	q31_t temp_25_5c = ((int64_t)-25500 * BIT(31) / 1000) >> shift;

	zassert_ok(emul_sensor_backend_set_channel(fixture->emul, ch, &temp_25_5c, shift));

	zassert_ok(sensor_sample_fetch(fixture->dev));
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, &sensor_val));

	zassert_equal(-25, sensor_val.val1, "Expected -25 C, got %d", sensor_val.val1);
	zassert_within(-500000, sensor_val.val2, ADT7420_TEST_RESOLUTION_16_BIT_MICRO_C,
		       "Expected val2 near 500000, got %d", sensor_val.val2);
}
