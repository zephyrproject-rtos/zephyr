/*
 * Copyright (c) 2025 Jonas Berg
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

#include "ina237.h"
#include "emul_ina228.h"

struct ina228_fixture {
	const struct device *dev_basic;
	const struct emul *target_basic;
};

/**
 * @brief Convert the 20 lowest bits to a signed value (from two's complement)
 *
 * @param input      Input value
 *
 * @retval converted value
 */
static int32_t ina228_convert_20bits_to_signed(uint32_t input)
{
	if (input <= INA228_INT20_MAX) {
		return (int32_t)input;
	}

	uint32_t intermediate = (~input & INA228_UINT20_MAX) + 1;

	return -((int32_t)intermediate);
}

/**
 * @brief Convert the 40 lowest bits to a signed value (from two's complement)
 *
 * @param input      Input value
 *
 * @retval converted value
 */
static int64_t ina228_convert_40bits_to_signed(uint64_t input)
{
	if (input <= INA228_INT40_MAX) {
		return (int64_t)input;
	}

	uint64_t intermediate = (~input & INA228_UINT40_MAX) + 1;

	return -((int64_t)intermediate);
}

static void *ina228_setup(void)
{
	static struct ina228_fixture fixture = {
		.dev_basic = DEVICE_DT_GET(DT_NODELABEL(ina228_default_test)),
		.target_basic = EMUL_DT_GET(DT_NODELABEL(ina228_default_test)),
	};

	zassert_not_null(fixture.dev_basic);
	zassert_not_null(fixture.target_basic);

	return &fixture;
}

static void ina228_before(void *f)
{
	struct ina228_fixture *fixture = (struct ina228_fixture *)f;

	zassert_true(device_is_ready(fixture->dev_basic), "I2C device %s is not ready",
		     fixture->dev_basic->name);
}

ZTEST_SUITE(ina228, NULL, ina228_setup, ina228_before, NULL, NULL);

/**
 * @brief Test converting 20 bits to signed value
 */
ZTEST(ina228, test_convert_20bits)
{
	zassert_equal(0x00000000, ina228_convert_20bits_to_signed(0x00000000));
	zassert_equal(0x00000001, ina228_convert_20bits_to_signed(0x00000001));
	zassert_equal(0x00000002, ina228_convert_20bits_to_signed(0x00000002));
	zassert_equal(0x00000003, ina228_convert_20bits_to_signed(0x00000003));
	zassert_equal(0x0007FFFD, ina228_convert_20bits_to_signed(0x0007FFFD));
	zassert_equal(0x0007FFFE, ina228_convert_20bits_to_signed(0x0007FFFE));
	zassert_equal(0x0007FFFF, ina228_convert_20bits_to_signed(0x0007FFFF));
	zassert_equal(-0x00080000, ina228_convert_20bits_to_signed(0x00080000));
	zassert_equal(-0x0007FFFF, ina228_convert_20bits_to_signed(0x00080001));
	zassert_equal(-0x0007FFFE, ina228_convert_20bits_to_signed(0x00080002));
	zassert_equal(-0x0007FFFD, ina228_convert_20bits_to_signed(0x00080003));
	zassert_equal(-0x00000003, ina228_convert_20bits_to_signed(0x000FFFFD));
	zassert_equal(-0x00000002, ina228_convert_20bits_to_signed(0x000FFFFE));
	zassert_equal(-0x00000001, ina228_convert_20bits_to_signed(0x000FFFFF));
}

/**
 * @brief Test converting 40 bits to signed value
 */
ZTEST(ina228, test_convert_40bits)
{
	zassert_equal(0x0000000000000000, ina228_convert_40bits_to_signed(0x0000000000000000));
	zassert_equal(0x0000000000000001, ina228_convert_40bits_to_signed(0x0000000000000001));
	zassert_equal(0x0000000000000002, ina228_convert_40bits_to_signed(0x0000000000000002));
	zassert_equal(0x0000000000000003, ina228_convert_40bits_to_signed(0x0000000000000003));
	zassert_equal(0x0000007FFFFFFFFD, ina228_convert_40bits_to_signed(0x0000007FFFFFFFFD));
	zassert_equal(0x0000007FFFFFFFFE, ina228_convert_40bits_to_signed(0x0000007FFFFFFFFE));
	zassert_equal(0x0000007FFFFFFFFF, ina228_convert_40bits_to_signed(0x0000007FFFFFFFFF));
	zassert_equal(-0x0000008000000000, ina228_convert_40bits_to_signed(0x0000008000000000));
	zassert_equal(-0x0000007FFFFFFFFF, ina228_convert_40bits_to_signed(0x0000008000000001));
	zassert_equal(-0x0000007FFFFFFFFE, ina228_convert_40bits_to_signed(0x0000008000000002));
	zassert_equal(-0x0000007FFFFFFFFD, ina228_convert_40bits_to_signed(0x0000008000000003));
	zassert_equal(-0x0000000000000003, ina228_convert_40bits_to_signed(0x000000FFFFFFFFFD));
	zassert_equal(-0x0000000000000002, ina228_convert_40bits_to_signed(0x000000FFFFFFFFFE));
	zassert_equal(-0x0000000000000001, ina228_convert_40bits_to_signed(0x000000FFFFFFFFFF));
}

/**
 * @brief Test reading out values
 *
 * The values are from the example in the data sheet
 */
ZTEST_F(ina228, test_example_from_data_sheet)
{
	uint16_t resulting_registervalue;
	uint16_t resulting_shunt_cal;
	const uint16_t expected_shunt_cal = 4034;
	struct sensor_value sensor_val;
	double reading;

	ina228_emul_get_reg_16(fixture->target_basic, INA237_REG_CONFIG, &resulting_registervalue);
	/*
	 * RST		0	0... .... .... ....
	 * RSTACC	0	.0.. .... .... ....
	 * CONVDLY	0	..00 0000 00.. ....
	 * TEMPCOMP	0	.... .... ..0. ....
	 * ADCRANGE	0	.... .... ...0 ....
	 */
	zexpect_equal(resulting_registervalue, 0x0000, "CONFIG: got 0x%04X",
		      resulting_registervalue);

	ina228_emul_get_reg_16(fixture->target_basic, INA237_REG_ADC_CONFIG,
			       &resulting_registervalue);
	/*
	 * MODE		15	1111 .... .... ....
	 * VBUSCT	5	.... 101. .... ....
	 * VSHCT	5	.... ...1 01.. ....
	 * VTCT		5	.... .... ..10 1...
	 * AVG		0	.... .... .... .000
	 */
	zexpect_equal(resulting_registervalue, 0xFB68, "ADC_CONFIG: got 0x%04X",
		      resulting_registervalue);

	ina228_emul_get_reg_16(fixture->target_basic, INA237_REG_CALIB,
			       &resulting_registervalue);
	resulting_shunt_cal = resulting_registervalue & 0x7FFF;
	zexpect_within(resulting_shunt_cal, expected_shunt_cal, 2,
		       "Expected shunt calib setting 0x%04X, got 0x%04X", expected_shunt_cal,
		       resulting_shunt_cal);

	ina228_emul_get_reg_16(fixture->target_basic, INA228_REG_SHUNT_TEMPCO,
			       &resulting_registervalue);
	/*
	 * TEMPCO	0	..00 0000 0000 0000
	 */
	zexpect_equal(resulting_registervalue, 0x0000, "SHUNT_TEMPCO: got 0x%04X",
		      resulting_registervalue);

	/* Datasheet value 311040 decimal */
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_SHUNT_VOLT, 0x4BF00 << 4);

	/* Datasheet value 314572 decimal */
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_CURRENT, 0x4CCCC << 4);

	/* Datasheet value 245760 decimal */
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_BUS_VOLT, 0x3C000 << 4);

	/* Datasheet value 4718604 decimal */
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_POWER, 0x48000C);

	/* Datasheet value 1061683200 decimal */
	ina228_emul_set_reg_40(fixture->target_basic, INA228_REG_ENERGY, 0x003F480000);

	/* Datasheet value 1132462080 decimal */
	ina228_emul_set_reg_40(fixture->target_basic, INA228_REG_CHARGE, 0x0043800000);

	/* Datasheet value 3200 decimal */
	ina228_emul_set_reg_16(fixture->target_basic, INA237_REG_DIETEMP, 0x0C80);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VOLTAGE, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(48.0, reading, 1.0e-3, "Got %.6f V", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(6.0, reading, 0.1, "Got %.6f A", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_POWER, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	/* Some difference due to limited resolution in devicetree LSB setting */
	zexpect_within(287, reading, 1, "Got %.6f W", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic,
				      (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	/* Some difference due to limited resolution in devicetree LSB setting */
	zexpect_within(21500, reading, 20, "Got %.6f C", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic,
				      (enum sensor_channel)SENSOR_CHAN_INA2XX_ENERGY, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	/* Some difference due to limited resolution in devicetree LSB setting */
	zexpect_within(1032800, reading, 50, "Got %.6f J", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(25, reading, 1.0e-3, "Got %.3f deg C", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VSHUNT, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(0.0972, reading, 1.0e-3, "Got %.6f V", reading);
}

/**
 * @brief Test negative values
 */
ZTEST_F(ina228, test_negative_values)
{
	struct sensor_value sensor_val;
	double reading;

	/* Current LSB = 19000 nA according to devicetree settings.
	 * -100 = 0xFFF9C in 20 bit two's complement
	 * This corresponds to -1.9 mA
	 */
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_CURRENT, 0xFFF9C << 4);

	/* Charge LSB = 19000 nC (same as devicetree setting for current LSB).
	 * -100 = 0xFFFFFFFF9C in 40 bit two's complement
	 * This corresponds to -1.9 mC
	 */
	ina228_emul_set_reg_40(fixture->target_basic, INA228_REG_CHARGE, 0xFFFFFFFF9C);

	/* Shunt voltage LSB = 312.5 nV for this ADCRANGE.
	 * -100 = 0xFFF9C in 20 bit two's complement
	 * This corresponds to -31.55 uV
	 */
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_SHUNT_VOLT, 0xFFF9C << 4);

	/* Temperature LSB = 7.8125 mdegC
	 * -100 = 0xFF9C in 20 bit two's complement
	 * This corresponds to -0.78125 deg C
	 */
	ina228_emul_set_reg_16(fixture->target_basic, INA237_REG_DIETEMP, 0xFF9C);

	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_BUS_VOLT, 0x000000);
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_POWER, 0x000000);
	ina228_emul_set_reg_40(fixture->target_basic, INA228_REG_ENERGY, 0x0000000000);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_CURRENT, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(-1.907e-3, reading, 1.0e-5, "Got %.6f A", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic,
				      (enum sensor_channel)SENSOR_CHAN_INA2XX_CHARGE, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(-1.907e-3, reading, 1.0e-5, "Got %.6f C", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_DIE_TEMP, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(-0.78125, reading, 1.0e-3, "Got %.3f deg C", reading);

	zassert_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_VSHUNT, &sensor_val));
	reading = sensor_value_to_double(&sensor_val);
	zexpect_within(-3.155e-5, reading, 1.0e-6, "Got %.6f V", reading);
}

/**
 * @brief Test invalid channel
 */
ZTEST_F(ina228, test_invalid_channel)
{
	struct sensor_value sensor_val;

	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_CURRENT, 0x0000);
	ina228_emul_set_reg_40(fixture->target_basic, INA228_REG_CHARGE, 0x0000000000);
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_SHUNT_VOLT, 0x00000);
	ina228_emul_set_reg_16(fixture->target_basic, INA237_REG_DIETEMP, 0x0000);
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_BUS_VOLT, 0x000000);
	ina228_emul_set_reg_24(fixture->target_basic, INA237_REG_POWER, 0x000000);
	ina228_emul_set_reg_40(fixture->target_basic, INA228_REG_ENERGY, 0x0000000000);

	zassert_ok(sensor_sample_fetch(fixture->dev_basic));

	zassert_not_ok(sensor_channel_get(fixture->dev_basic, SENSOR_CHAN_ALTITUDE, &sensor_val));
}
