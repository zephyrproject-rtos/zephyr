/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "adltc2990.h"
#include "adltc2990_emul.h"
#include "adltc2990_reg.h"

/* Colllection of common assertion macros */
#define CHECK_SINGLE_ENDED_VOLTAGE(sensor_val, index, pin_voltage, r1, r2)                         \
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_VOLTAGE));                   \
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLTAGE, sensor_val));             \
	zassert_between_inclusive(                                                                 \
		sensor_val[index].val1 + (float)sensor_val[index].val2 / 1000000,                  \
		(pin_voltage - 0.01f) * ((r1 + r2) / (float)r2),                                   \
		(pin_voltage + 0.01f) * ((r1 + r2) / (float)r2),                                   \
		"%f Out of Range [%f,%f] input %f, [%dmΩ, %dmΩ] "                                \
		"\nCheck if the sensor node is configured correctly",                              \
		(double)(sensor_val[index].val1 + (float)sensor_val[index].val2 / 1000000),        \
		(double)((pin_voltage - 0.01f) * ((r1 + r2) / (float)r2)),                         \
		(double)((pin_voltage + 0.01f) * ((r1 + r2) / (float)r2)), (double)pin_voltage,    \
		r1, r2);

#define CHECK_CURRENT(sensor_val, index, pin_voltage, r_microohms)                                 \
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_CURRENT));                   \
	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, sensor_val));             \
	zassert_between_inclusive(                                                                 \
		sensor_val[index].val1 + (float)sensor_val[index].val2 / 1000000,                  \
		(pin_voltage - 0.01f) * ADLTC2990_MICROOHM_CONVERSION_FACTOR / r_microohms,        \
		(pin_voltage + 0.01f) * ADLTC2990_MICROOHM_CONVERSION_FACTOR / r_microohms,        \
		"%f Out of Range [%f,%f] input %f, current-resistor: %dµΩ\nCheck if the sensor " \
		"node is configured correctly",                                                    \
		(double)(sensor_val[index].val1 + (float)sensor_val[index].val2 / 1000000),        \
		(double)((pin_voltage - 0.001f) * ADLTC2990_MICROOHM_CONVERSION_FACTOR /           \
			 r_microohms),                                                             \
		(double)((pin_voltage + 0.001f) * ADLTC2990_MICROOHM_CONVERSION_FACTOR /           \
			 r_microohms),                                                             \
		(double)pin_voltage, r_microohms);

#define CHECK_TEMPERATURE(sensor_val, index, expected_temperature, temperature_type)               \
	zassert_ok(sensor_sample_fetch_chan(fixture->dev, temperature_type));                      \
	zassert_ok(sensor_channel_get(fixture->dev, temperature_type, sensor_val));                \
	zassert_equal(expected_temperature,                                                        \
		      sensor_val[index].val1 + (float)sensor_val[index].val2 / 1000000,            \
		      "expected %f, got %f", (double)expected_temperature,                         \
		      (double)(sensor_val[index].val1 + (float)sensor_val[index].val2 / 1000000));

/*** TEST-SUITE: ADLTC2990 Measurement Mode 0 0***/

struct adltc2990_0_0_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_0_0_setup(void)
{
	static struct adltc2990_0_0_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_0_0)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_0_0)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_0_0_before(void *f)
{
	struct adltc2990_0_0_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_0_0, NULL, adltc2990_0_0_setup, adltc2990_0_0_before, NULL, NULL);

ZTEST_F(adltc2990_0_0, test_measure_mode_internal_temperature_only)
{
	struct sensor_value value[1];

	zassert_equal(-ENOTSUP, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_MAGN_X));
	zassert_equal(-ENOTSUP, sensor_channel_get(fixture->dev, SENSOR_CHAN_MAGN_Z, value));
	zassert_equal(-EINVAL, sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, value));
	zassert_equal(-EINVAL, sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, value));
}

/*** TEST-SUITE: ADLTC2990 Measurement Mode 4 3***/

struct adltc2990_4_3_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_4_3_setup(void)
{
	static struct adltc2990_4_3_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_4_3)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_4_3)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_4_3_before(void *f)
{
	struct adltc2990_4_3_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_4_3, NULL, adltc2990_4_3_setup, adltc2990_4_3_before, NULL, NULL);

ZTEST_F(adltc2990_4_3, test_available_channels)
{
	struct sensor_value value[3];

	zassert_equal(0, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_VOLTAGE));
	zassert_equal(0, sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLTAGE, value));
	zassert_equal(0, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP));
	zassert_equal(0, sensor_channel_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, value));
	zassert_equal(0, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_CURRENT));
	zassert_equal(0, sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, value));
}

/*** TEST-SUITE: ADLTC2990 Measurement Mode 1 3***/

struct adltc2990_1_3_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_1_3_setup(void)
{
	static struct adltc2990_1_3_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_1_3)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_1_3)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_1_3_before(void *f)
{
	struct adltc2990_1_3_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_1_3, NULL, adltc2990_1_3_setup, adltc2990_1_3_before, NULL, NULL);

ZTEST_F(adltc2990_1_3, test_die_temperature)
{
	/* The following values are taken from datasheet and should translate to 125°c */
	uint8_t msb = 0b00000111, lsb = 0b11010000;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_LSB, &lsb);

	struct sensor_value temp_value[1];

	CHECK_TEMPERATURE(temp_value, 0, 125.00f, SENSOR_CHAN_DIE_TEMP);

	/*0b00011101 0b10000000 –40.0000*/
	msb = 0b00011101;
	lsb = 0b10000000;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_LSB, &lsb);

	CHECK_TEMPERATURE(temp_value, 0, -40.00f, SENSOR_CHAN_DIE_TEMP);
}

ZTEST_F(adltc2990_1_3, test_ambient_temperature)
{
	/* 0b00000001 0b10010001 +25.0625 */
	uint8_t msb = 0b00000001, lsb = 0b10010001;
	struct sensor_value temp_ambient[1];

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_LSB, &lsb);

	CHECK_TEMPERATURE(temp_ambient, 0, 25.06250f, SENSOR_CHAN_AMBIENT_TEMP);
}

ZTEST_F(adltc2990_1_3, test_current)
{
	/* 0b00111100 0b01011000 +0.300 */
	uint8_t msb_reg_value = 0b00111100, lsb_reg_value = 0b01011000;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_LSB, &lsb_reg_value);

	struct sensor_value current_values[1];
	const struct adltc2990_config *dev_config = fixture->target->dev->config;

	CHECK_CURRENT(current_values, 0, 0.3f, dev_config->pins_v1_v2.pins_current_resistor);

	/* 0b00100000 0b00000000 +0.159 */
	msb_reg_value = 0b00100000, lsb_reg_value = 0b00000000;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_LSB, &lsb_reg_value);
	CHECK_CURRENT(current_values, 0, 0.159f, dev_config->pins_v1_v2.pins_current_resistor);
}

ZTEST_F(adltc2990_1_3, test_V1_MINUS_V2_VCC)
{
	uint8_t msb = 0b01100000, lsb = 0b00000000;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_LSB, &lsb);

	msb = 0b00000010;
	lsb = 0b10001111;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_VCC_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_VCC_LSB, &lsb);

	zassert_ok(sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_VOLTAGE));

	struct sensor_value voltage_values[2];

	zassert_ok(sensor_channel_get(fixture->dev, SENSOR_CHAN_VOLTAGE, voltage_values));

	float test_value = voltage_values[0].val1 + (float)voltage_values[0].val2 / 1000000;

	zassert_between_inclusive(test_value, -0.16f, -0.159f, "Out of Range [-0.16,-0.159]%.6f",
				  (double)test_value);

	test_value = voltage_values[1].val1 + (float)voltage_values[1].val2 / 1000000;
	zassert_between_inclusive(test_value, 2.69f, 2.7f, "Out of Range [2.69, 2.7]%.6f",
				  (double)test_value);
}

/*** TEST-SUITE: ADLTC2990 Measurement Mode 5 3***/

struct adltc2990_5_3_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_5_3_setup(void)
{
	static struct adltc2990_5_3_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_5_3)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_5_3)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_5_3_before(void *f)
{
	struct adltc2990_5_3_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_5_3, NULL, adltc2990_5_3_setup, adltc2990_5_3_before, NULL, NULL);

ZTEST_F(adltc2990_5_3, test_ambient_temperature)
{
	/*Kelvin 0b00010001 0b00010010 273.1250*/
	uint8_t msb = 0b00010001, lsb = 0b00010010;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_LSB, &lsb);

	struct sensor_value temp_value[2];

	CHECK_TEMPERATURE(temp_value, 0, 273.1250f, SENSOR_CHAN_AMBIENT_TEMP);

	/*Kelvin 0b00001110 0b10010010 233.125*/
	msb = 0b00001110;
	lsb = 0b10010010;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_LSB, &lsb);
	CHECK_TEMPERATURE(temp_value, 1, 233.1250f, SENSOR_CHAN_AMBIENT_TEMP);
}

ZTEST_F(adltc2990_5_3, test_die_temperature)
{
	/*0b00011000 0b11100010 398.1250*/
	uint8_t msb = 0b00011000, lsb = 0b11100010;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_LSB, &lsb);

	struct sensor_value temp_value[1];

	CHECK_TEMPERATURE(temp_value, 0, 398.1250f, SENSOR_CHAN_DIE_TEMP);
}

/*** TEST-SUITE: ADLTC2990 Measurement Mode 7 3***/

struct adltc2990_6_3_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_6_3_setup(void)
{
	static struct adltc2990_6_3_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_6_3)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_6_3)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_6_3_before(void *f)
{
	struct adltc2990_6_3_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_6_3, NULL, adltc2990_6_3_setup, adltc2990_6_3_before, NULL, NULL);

ZTEST_F(adltc2990_6_3, test_current)
{
	/* 0b00111100 0b01011000 +0.300 */
	uint8_t msb_reg_value = 0b00111100, lsb_reg_value = 0b01011000;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_LSB, &lsb_reg_value);

	struct sensor_value current_values[2];
	const struct adltc2990_config *dev_config = fixture->target->dev->config;

	CHECK_CURRENT(current_values, 0, 0.3f, dev_config->pins_v1_v2.pins_current_resistor);

	/* 0b00100000 0b00000000 +0.159 */
	msb_reg_value = 0b00100000, lsb_reg_value = 0b00000000;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_LSB, &lsb_reg_value);
	CHECK_CURRENT(current_values, 1, 0.159f, dev_config->pins_v3_v4.pins_current_resistor);
}

/*** TEST-SUITE: ADLTC2990 Measurement Mode 7 3***/
struct adltc2990_7_3_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_7_3_setup(void)
{
	static struct adltc2990_7_3_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_7_3)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_7_3)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_7_3_before(void *f)
{
	struct adltc2990_7_3_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_7_3, NULL, adltc2990_7_3_setup, adltc2990_7_3_before, NULL, NULL);

ZTEST_F(adltc2990_7_3, test_available_channels)
{
	zassert_equal(-EINVAL, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP));
	zassert_equal(-EINVAL, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_CURRENT));
}

ZTEST_F(adltc2990_7_3, test_is_device_busy)
{
	uint8_t is_busy = BIT(0);

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_STATUS, &is_busy);
	zassert_equal(-EBUSY, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_ALL));
	is_busy = 0;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_STATUS, &is_busy);
	zassert_equal(0, sensor_sample_fetch_chan(fixture->dev, SENSOR_CHAN_ALL));
}

ZTEST_F(adltc2990_7_3, test_die_temperature)
{
	/* The following values are taken from datasheet and should translate to 398.1250K */

	uint8_t msb = 0b00011000, lsb = 0b11100010;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_MSB, &msb);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_INTERNAL_TEMP_LSB, &lsb);

	struct sensor_value *die_temp_value_null = (struct sensor_value *)NULL;

	zassert_equal(-EINVAL,
		      sensor_channel_get(fixture->dev, SENSOR_CHAN_ALL, die_temp_value_null));

	struct sensor_value die_temp_value[1];

	CHECK_TEMPERATURE(die_temp_value, 0, 398.1250f, SENSOR_CHAN_DIE_TEMP);
}

ZTEST_F(adltc2990_7_3, test_V1_V2_V3_V4_VCC)
{
	/* 0b00111111 0b11111111 >5 */
	uint8_t msb_reg_value = 0b00111111, lsb_reg_value = 0b11111111;

	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V1_LSB, &lsb_reg_value);

	/* 0b00101100 0b11001101 3.500 */
	msb_reg_value = 0b00101100;
	lsb_reg_value = 0b11001101;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V2_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V2_LSB, &lsb_reg_value);

	/* 0b00011111 0b11111111 2.500 */
	msb_reg_value = 0b00011111;
	lsb_reg_value = 0b11111111;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V3_LSB, &lsb_reg_value);

	/* 0b01111100 0b00101001 –0.300 */
	msb_reg_value = 0b01111100;
	lsb_reg_value = 0b00101001;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V4_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_V4_LSB, &lsb_reg_value);

	/* VCC = 6V */
	msb_reg_value = 0b00101100;
	lsb_reg_value = 0b11001101;
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_VCC_MSB, &msb_reg_value);
	adltc2990_emul_set_reg(fixture->target, ADLTC2990_REG_VCC_LSB, &lsb_reg_value);

	struct sensor_value voltage_values[5];

	const struct adltc2990_config *dev_config = fixture->dev->config;

	CHECK_SINGLE_ENDED_VOLTAGE(voltage_values, 0, 5.0f,
				   dev_config->pins_v1_v2.voltage_divider_resistors.v1_r1_r2[0],
				   dev_config->pins_v1_v2.voltage_divider_resistors.v1_r1_r2[1]);

	CHECK_SINGLE_ENDED_VOLTAGE(voltage_values, 1, 3.5f,
				   dev_config->pins_v1_v2.voltage_divider_resistors.v2_r1_r2[0],
				   dev_config->pins_v1_v2.voltage_divider_resistors.v2_r1_r2[1]);

	CHECK_SINGLE_ENDED_VOLTAGE(voltage_values, 2, 2.5f,
				   dev_config->pins_v3_v4.voltage_divider_resistors.v3_r1_r2[0],
				   dev_config->pins_v3_v4.voltage_divider_resistors.v3_r1_r2[1]);

	CHECK_SINGLE_ENDED_VOLTAGE(voltage_values, 3, -0.3f,
				   dev_config->pins_v3_v4.voltage_divider_resistors.v4_r1_r2[0],
				   dev_config->pins_v3_v4.voltage_divider_resistors.v4_r1_r2[1]);

	double test_value = voltage_values[4].val1 + (double)voltage_values[4].val2 / 1000000;

	zassert_between_inclusive(test_value, 6.0, 6.1, "Out of Range [6.0,6.1] %.6f",
				  (double)test_value);

	zassert_equal(6, voltage_values[4].val1);
}

/*** TEST-SUITE: ADLTC2990 Measurement Mode Incorrect***/
struct adltc2990_incorrect_fixture {
	const struct device *dev;
	const struct emul *target;
};

static void *adltc2990_incorrect_setup(void)
{
	static struct adltc2990_incorrect_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(adltc2990_incorrect)),
		.target = EMUL_DT_GET(DT_NODELABEL(adltc2990_incorrect)),
	};

	zassert_not_null(fixture.dev);
	zassert_not_null(fixture.target);
	return &fixture;
}

static void adltc2990_incorrect_before(void *f)
{
	struct adltc2990_incorrect_fixture *fixture = f;

	adltc2990_emul_reset(fixture->target);
}

ZTEST_SUITE(adltc2990_incorrect, NULL, adltc2990_incorrect_setup, adltc2990_incorrect_before, NULL,
	    NULL);

ZTEST_F(adltc2990_incorrect, test_current_cannot_be_measured)
{
	struct sensor_value current[1];

	zassert_equal(-EINVAL, sensor_channel_get(fixture->dev, SENSOR_CHAN_CURRENT, current));
}
