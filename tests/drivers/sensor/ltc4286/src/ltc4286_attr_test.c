/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ltc4286.h>
#include <zephyr/ztest.h>

#include "ltc4286_emul.h"

struct ltc4286_attrs_fixture {
	const struct device *dev;
	const struct emul *mock;
};

static void *ltc4286_attrs_setup(void)
{
	static struct ltc4286_attrs_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(ltc4286_default_test)),
		.mock = EMUL_DT_GET(DT_NODELABEL(ltc4286_default_test)),
	};

	return &fixture;
}

static void ltc4286_attrs_after(void *f)
{
	struct ltc4286_attrs_fixture *fixture = f;

	ltc4286_emul_reset(fixture->mock);
}

ZTEST_F(ltc4286_attrs, test_threshold_voltage_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 12;
	set_val.val2 = 500000; /* 12.5V */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_UPPER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set VOUT upper threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_UPPER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get VOUT upper threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 0.1,
		       "VOUT upper threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));

	set_val.val1 = 10;
	set_val.val2 = 0; /* 10.0V */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set VOUT lower threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get VOUT lower threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 0.1,
		       "VOUT lower threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));

	set_val.val1 = 13;
	set_val.val2 = 200000; /* 13.2V */

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      SENSOR_ATTR_UPPER_THRESH, &set_val);
	zassert_ok(ret, "Failed to set VIN upper threshold");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      SENSOR_ATTR_UPPER_THRESH, &get_val);
	zassert_ok(ret, "Failed to get VIN upper threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 0.1,
		       "VIN upper threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));

	set_val.val1 = 9;
	set_val.val2 = 800000; /* 9.8V */

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      SENSOR_ATTR_LOWER_THRESH, &set_val);
	zassert_ok(ret, "Failed to set VIN lower threshold");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      SENSOR_ATTR_LOWER_THRESH, &get_val);
	zassert_ok(ret, "Failed to get VIN lower threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 0.1,
		       "VIN lower threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));
}

ZTEST_F(ltc4286_attrs, test_threshold_current_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 3;
	set_val.val2 = 500000; /* 3.5A */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set IOUT upper threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get IOUT upper threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 0.1,
		       "IOUT upper threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));
}

ZTEST_F(ltc4286_attrs, test_threshold_power_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 0;
	set_val.val2 = 100000; /* 0.1W */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_POWER, SENSOR_ATTR_UPPER_THRESH, &set_val);
	zassert_ok(ret, "Failed to set PIN upper threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_POWER, SENSOR_ATTR_UPPER_THRESH, &get_val);
	zassert_ok(ret, "Failed to get PIN upper threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 0.01,
		       "PIN upper threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));
}

ZTEST_F(ltc4286_attrs, test_threshold_temperature_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 85;
	set_val.val2 = 0; /* 85C */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set TEMP upper threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get TEMP upper threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 1.0,
		       "TEMP upper threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));

	set_val.val1 = 10;
	set_val.val2 = 0; /* 10°C */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set TEMP lower threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get TEMP lower threshold");

	zexpect_within(sensor_value_to_double(&set_val), sensor_value_to_double(&get_val), 1.0,
		       "TEMP lower threshold mismatch: expected %.3f, got %.3f",
		       sensor_value_to_double(&set_val), sensor_value_to_double(&get_val));
}

ZTEST_F(ltc4286_attrs, test_adc_config_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 1;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_VDS_SELECT, &set_val);
	zassert_ok(ret, "Failed to set ADC_VDS_SELECT");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_VDS_SELECT, &get_val);
	zassert_ok(ret, "Failed to get ADC_VDS_SELECT");

	zassert_equal(set_val.val1, get_val.val1, "ADC_VDS_SELECT mismatch");

	set_val.val1 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_VIN_VOUT_SELECT,
			      &set_val);
	zassert_ok(ret, "Failed to set ADC_VIN_VOUT_SELECT");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_VIN_VOUT_SELECT,
			      &get_val);
	zassert_ok(ret, "Failed to get ADC_VIN_VOUT_SELECT");

	zassert_equal(set_val.val1, get_val.val1, "ADC_VIN_VOUT_SELECT mismatch");

	set_val.val1 = 1;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_DISP_AVG, &set_val);
	zassert_ok(ret, "Failed to set ADC_DISP_AVG");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_DISP_AVG, &get_val);
	zassert_ok(ret, "Failed to get ADC_DISP_AVG");

	zassert_equal(set_val.val1, get_val.val1, "ADC_DISP_AVG mismatch");

	set_val.val1 = 8;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_AVG_SELECT, &set_val);
	zassert_ok(ret, "Failed to set ADC_AVG_SELECT");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_ADC,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_ADC_AVG_SELECT, &get_val);
	zassert_ok(ret, "Failed to get ADC_AVG_SELECT");

	zassert_equal(set_val.val1, get_val.val1, "ADC_AVG_SELECT mismatch");
}

ZTEST_F(ltc4286_attrs, test_config_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 0x80;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_OPERATION,
			      &set_val);
	zassert_ok(ret, "Failed to set CFG_OPERATION");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_OPERATION,
			      &get_val);
	zassert_ok(ret, "Failed to get CFG_OPERATION");

	zassert_equal(set_val.val1, get_val.val1, "CNG_OPERATION mismatch");

	/* Test CLEAR_FAULTS (send-only command, no get) */
	set_val.val1 = 0;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_CLEAR_FAULTS,
			      &set_val);
	zassert_ok(ret, "Failed to set CFG_CLEAR_FAULTS");

	/* Test WRITE_PROTECT get after set */
	set_val.val1 = 0x80;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &set_val);
	zassert_ok(ret, "Failed to enable write protection");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &get_val);
	zassert_ok(ret, "Failed to get CFG_WRITE_PROTECT");

	zassert_equal(get_val.val1, 1, "WRITE_PROTECT should be enabled");

	/* Disable write protection and verify get returns 0 */
	set_val.val1 = 0x00;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			&set_val);
	zassert_ok(ret, "Failed to disable write protection");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &get_val);
	zassert_ok(ret, "Failed to get CFG_WRITE_PROTECT after disable");

	zassert_equal(get_val.val1, 0, "WRITE_PROTECT should be disabled");
}

ZTEST_F(ltc4286_attrs, test_mfr_config_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 10;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM, &set_val);
	zassert_ok(ret, "Failed to set MFR_CONFIG_ILIM");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM, &get_val);
	zassert_ok(ret, "Failed to get MFR_CONFIG_ILIM");

	zassert_equal(set_val.val1, get_val.val1, "MFR_CONFIG_ILIM mismatch");

	set_val.val1 = 1;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL,
			      &set_val);
	zassert_ok(ret, "Failed to set MFR_CONFIG_VRANGE_SEL");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL,
			      &get_val);
	zassert_ok(ret, "Failed to get MFR_CONFIG_VRANGE_SEL");

	zassert_equal(set_val.val1, get_val.val1, "MFR_CONFIG_VRANGE_SEL mismatch");

	set_val.val1 = 1;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT,
			      &set_val);
	zassert_ok(ret, "Failed to set MFR_CONFIG_PMBUS_1MBIT");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT,
			      &get_val);
	zassert_ok(ret, "Failed to get MFR_CONFIG_PMBUS_1MBIT");

	zassert_equal(set_val.val1, get_val.val1, "MFR_CONFIG_PMBUS_1MBIT mismatch");

	set_val.val1 = 1;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP,
			      &set_val);
	zassert_ok(ret, "Failed to set MFR_CONFIG_EXT_TEMP");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP,
			      &get_val);
	zassert_ok(ret, "Failed to get MFR_CONFIG_EXT_TEMP");

	zassert_equal(set_val.val1, get_val.val1, "MFR_CONFIG_EXT_TEMP mismatch");

	set_val.val1 = 1;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_DEBOUNCE_TIMER,
			      &set_val);
	zassert_ok(ret, "Failed to set MFR_CONFIG_DEBOUNCE_TIMER");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_MFR_CONFIG_DEBOUNCE_TIMER,
			      &get_val);
	zassert_ok(ret, "Failed to get MFR_CONFIG_DEBOUNCE_TIMER");

	zassert_equal(set_val.val1, get_val.val1, "MFR_CONFIG_DEBOUNCE_TIMER mismatch");
}

ZTEST_F(ltc4286_attrs, test_fault_response_iout_oc_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 2;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_CURRENT,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RESPONSE, &set_val);
	zassert_ok(ret, "Failed to set IOUT_OC FAULT_RESPONSE");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_CURRENT,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RESPONSE, &get_val);
	zassert_ok(ret, "Failed to get IOUT_OC FAULT_RESPONSE");

	zassert_equal(set_val.val1, get_val.val1, "IOUT_OC FAULT_RESPONSE mismatch");

	set_val.val1 = 5;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_CURRENT,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RETRY, &set_val);
	zassert_ok(ret, "Failed to set IOUT_OC FAULT_RETRY");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_CURRENT,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RETRY, &get_val);
	zassert_ok(ret, "Failed to get IOUT_OC FAULT_RETRY");

	zassert_equal(set_val.val1, get_val.val1, "IOUT_OC FAULT_RETRY mismatch");
}

ZTEST_F(ltc4286_attrs, test_fault_response_vin_ov_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 1;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_OV_RESPONSE,
			      &set_val);
	zassert_ok(ret, "Failed to set VIN_OV FAULT_RESPONSE");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_OV_RESPONSE,
			      &get_val);
	zassert_ok(ret, "Failed to get VIN_OV FAULT_RESPONSE");

	zassert_equal(set_val.val1, get_val.val1, "VIN_OV FAULT_RESPONSE mismatch");

	set_val.val1 = 3;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_OV_RETRY, &set_val);
	zassert_ok(ret, "Failed to set VIN_OV FAULT_RETRY");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_OV_RETRY, &get_val);
	zassert_ok(ret, "Failed to get VIN_OV FAULT_RETRY");

	zassert_equal(set_val.val1, get_val.val1, "VIN_OV FAULT_RETRY mismatch");
}

ZTEST_F(ltc4286_attrs, test_fault_response_vin_uv_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 3;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_UV_RESPONSE,
			      &set_val);
	zassert_ok(ret, "Failed to set VIN_UV FAULT_RESPONSE");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_UV_RESPONSE,
			      &get_val);
	zassert_ok(ret, "Failed to get VIN_UV FAULT_RESPONSE");

	zassert_equal(set_val.val1, get_val.val1, "VIN_UV FAULT_RESPONSE mismatch");

	set_val.val1 = 7;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_UV_RETRY, &set_val);
	zassert_ok(ret, "Failed to set VIN_UV FAULT_RETRY");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_UV_RETRY, &get_val);
	zassert_ok(ret, "Failed to get VIN_UV FAULT_RETRY");

	zassert_equal(set_val.val1, get_val.val1, "VIN_UV FAULT_RETRY mismatch");
}

ZTEST_F(ltc4286_attrs, test_fault_response_fet_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 2;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_FAULT_RESP_FET,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RESPONSE, &set_val);
	zassert_ok(ret, "Failed to set FET FAULT_RESPONSE");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_FAULT_RESP_FET,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RESPONSE, &get_val);
	zassert_ok(ret, "Failed to get FET FAULT_RESPONSE");

	zassert_equal(set_val.val1, get_val.val1, "FET FAULT_RESPONSE mismatch");

	set_val.val1 = 4;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_FAULT_RESP_FET,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RETRY, &set_val);
	zassert_ok(ret, "Failed to set FET FAULT_RETRY");

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_FAULT_RESP_FET,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RETRY, &get_val);
	zassert_ok(ret, "Failed to get FET FAULT_RETRY");

	zassert_equal(set_val.val1, get_val.val1, "FET FAULT_RETRY mismatch");
}

ZTEST_F(ltc4286_attrs, test_fault_response_pin_op_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 1;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_POWER,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RESPONSE,
			      &set_val);
	zassert_ok(ret, "Failed to set PIN_OP FAULT_RESPONSE");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_POWER,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RESPONSE,
			      &get_val);
	zassert_ok(ret, "Failed to get PIN_OP FAULT_RESPONSE");

	zassert_equal(set_val.val1, get_val.val1, "PIN_OP FAULT_RESPONSE mismatch");

	set_val.val1 = 6;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_POWER,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RETRY,
			      &set_val);
	zassert_ok(ret, "Failed to set PIN_OP FAULT_RETRY");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_POWER,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_RETRY,
			      &get_val);
	zassert_ok(ret, "Failed to get PIN_OP FAULT_RETRY");

	zassert_equal(set_val.val1, get_val.val1, "PIN_OP FAULT_RETRY mismatch");

	set_val.val1 = 100;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_POWER,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_OP_TIMER,
			      &set_val);
	zassert_ok(ret, "Failed to set PIN_OP FAULT_OP_TIMER");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_POWER,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_FAULT_OP_TIMER,
			      &get_val);
	zassert_ok(ret, "Failed to get PIN_OP FAULT_OP_TIMER");

	zassert_equal(set_val.val1, get_val.val1, "PIN_OP FAULT_OP_TIMER mismatch");
}

ZTEST_F(ltc4286_attrs, test_reboot_control_attrs)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 3;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev,
			      (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_REBOOT_CONTROL,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_REBOOT_CONTROL_DELAY,
			      &set_val);
	zassert_ok(ret, "Failed to set REBOOT_CONTROL_DELAY");

	ret = sensor_attr_get(fixture->dev,
			      (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_REBOOT_CONTROL,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_REBOOT_CONTROL_DELAY,
			      &get_val);
	zassert_ok(ret, "Failed to get REBOOT_CONTROL_DELAY");

	zassert_equal(set_val.val1, get_val.val1, "REBOOT_CONTROL_DELAY mismatch");
}

ZTEST_F(ltc4286_attrs, test_read_only_mfr_attrs)
{
	struct sensor_value get_val;
	int ret;

	ret = sensor_attr_get(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_COMMON,
			      SENSOR_ATTR_CONFIGURATION, &get_val);
	zassert_ok(ret, "Failed to get MFR_COMMON");

	ret = sensor_attr_get(fixture->dev,
			      (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_PADS_LIVE_STATUS,
			      SENSOR_ATTR_CONFIGURATION, &get_val);
	zassert_ok(ret, "Failed to get MFR_PADS_LIVE_STATUS");

	ret = sensor_attr_get(fixture->dev,
			      (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_SHUTDOWN_CAUSE,
			      SENSOR_ATTR_CONFIGURATION, &get_val);
	zassert_ok(ret, "Failed to get MFR_SHUTDOWN_CAUSE");
}

ZTEST_F(ltc4286_attrs, test_invalid_attrs)
{
	struct sensor_value val;
	int ret;

	val.val1 = 0;
	val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_COMMON,
			      SENSOR_ATTR_CONFIGURATION, &val);
	zassert_equal(ret, -ENOTSUP, "Should fail to set read-only MFR_COMMON");

	ret = sensor_attr_set(fixture->dev,
			      (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_PADS_LIVE_STATUS,
			      SENSOR_ATTR_CONFIGURATION, &val);
	zassert_equal(ret, -ENOTSUP, "Should fail to set read-only MFR_PADS_LIVE_STATUS");

	ret = sensor_attr_set(fixture->dev,
			      (enum sensor_channel)SENSOR_CHAN_LTC4286_MFR_SHUTDOWN_CAUSE,
			      SENSOR_ATTR_CONFIGURATION, &val);
	zassert_equal(ret, -ENOTSUP, "Should fail to set read-only MFR_SHUTDOWN_CAUSE");

	ret = sensor_attr_set(fixture->dev, 9999, SENSOR_ATTR_CONFIGURATION, &val);
	zassert_equal(ret, -EINVAL, "Should fail for invalid channel");

	ret = sensor_attr_get(fixture->dev, 9999, SENSOR_ATTR_CONFIGURATION, &val);
	zassert_equal(ret, -EINVAL, "Should fail for invalid channel");
}

ZTEST_F(ltc4286_attrs, test_threshold_boundary_values)
{
	struct sensor_value set_val, get_val;
	int ret;

	set_val.val1 = 0;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set minimum voltage threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_VOLTAGE, SENSOR_ATTR_LOWER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get minimum voltage threshold");

	set_val.val1 = 125;
	set_val.val2 = 0; /* 125C */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set maximum temperature threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get maximum temperature threshold");

	set_val.val1 = 5;
	set_val.val2 = 0; /* 5C */

	ret = sensor_attr_set(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH,
			      &set_val);
	zassert_ok(ret, "Failed to set low temperature threshold");

	ret = sensor_attr_get(fixture->dev, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH,
			      &get_val);
	zassert_ok(ret, "Failed to get low temperature threshold");
}

ZTEST_F(ltc4286_attrs, test_write_protect_attrs)
{
	struct sensor_value set_val;
	uint16_t reg_val;
	int ret;

	/* Enable write protection: 0x80 (BIT 7) protects all writeable registers */
	set_val.val1 = 0x80;
	set_val.val2 = 0;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &set_val);
	zassert_ok(ret, "Failed to enable write protection");

	/* Verify the WRITE_PROTECT register (cmd 0x10) was written in the emulator */
	ret = ltc4286_emul_get_register(fixture->mock->data, 0x10, &reg_val);
	zassert_ok(ret, "Failed to read WRITE_PROTECT register from emulator");
	zassert_equal((uint8_t)reg_val, 0x80, "WRITE_PROTECT register not set");

	/* Disable write protection */
	set_val.val1 = 0x00;

	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &set_val);
	zassert_ok(ret, "Failed to disable write protection");

	ret = ltc4286_emul_get_register(fixture->mock->data, 0x10, &reg_val);
	zassert_ok(ret, "Failed to read WRITE_PROTECT register from emulator");
	zassert_equal((uint8_t)reg_val, 0x00, "WRITE_PROTECT register not cleared");
}

ZTEST_F(ltc4286_attrs, test_write_protect_logic)
{
	struct sensor_value set_val;
	int ret;

	/* Enable write protection */
	set_val.val1 = 0x80;
	set_val.val2 = 0;
	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &set_val);
	zassert_ok(ret, "Failed to enable write protection");

	/* Any write to a non-write-protect attribute must be blocked with -EACCES */
	set_val.val1 = 12;
	set_val.val2 = 0;
	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      SENSOR_ATTR_UPPER_THRESH, &set_val);
	zassert_equal(ret, -EACCES, "Write to threshold should be blocked when protected");

	/* Disabling write protection must still be allowed */
	set_val.val1 = 0x00;
	set_val.val2 = 0;
	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_CONFIG,
			      (enum sensor_attribute)SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT,
			      &set_val);
	zassert_ok(ret, "Disabling write protection should always be allowed");

	/* After disabling, the same write must succeed */
	set_val.val1 = 12;
	set_val.val2 = 0;
	ret = sensor_attr_set(fixture->dev, (enum sensor_channel)SENSOR_CHAN_LTC4286_VIN,
			      SENSOR_ATTR_UPPER_THRESH, &set_val);
	zassert_ok(ret, "Write to threshold should succeed after write protection is cleared");
}

ZTEST_SUITE(ltc4286_attrs, NULL, ltc4286_attrs_setup, NULL, ltc4286_attrs_after, NULL);
