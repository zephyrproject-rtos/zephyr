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

struct ltc4286_alert_fixture {
	const struct device *dev;
	const struct emul *mock;
};

static void *ltc4286_alert_setup(void)
{
	static struct ltc4286_alert_fixture fixture = {
		.dev = DEVICE_DT_GET(DT_NODELABEL(ltc4286_default_test)),
		.mock = EMUL_DT_GET(DT_NODELABEL(ltc4286_default_test)),
	};

	return &fixture;
}

static void ltc4286_alert_after(void *f)
{
	struct ltc4286_alert_fixture *fixture = f;

	ltc4286_emul_reset(fixture->mock);
}

ZTEST_F(ltc4286_alert, test_disable_vout_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_VOUT_MASK,
					 LTC4286_ALERT_VOUT_OV_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable VOUT OV warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_VOUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read VOUT alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_VOUT_OV_WARNING_ALERT_BIT, reg_val),
		     "VOUT OV warning alert bit should be set after disable");

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_VOUT_MASK,
					 LTC4286_ALERT_VOUT_UV_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable VOUT UV warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_VOUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read VOUT alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_VOUT_UV_WARNING_ALERT_BIT, reg_val),
		     "VOUT UV warning alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_vout_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	/* First set the bits via disable, then clear them via enable */
	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_VOUT_MASK,
					 LTC4286_ALERT_VOUT_OV_WARNING_ALERT_BIT |
						 LTC4286_ALERT_VOUT_UV_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable VOUT alerts");

	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_VOUT_MASK,
					LTC4286_ALERT_VOUT_OV_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to enable VOUT OV warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_VOUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read VOUT alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_VOUT_OV_WARNING_ALERT_BIT, reg_val),
		      "VOUT OV warning alert bit should be clear after enable");

	/* UV bit must still be set (not affected by enabling OV) */
	zassert_true(FIELD_GET(LTC4286_ALERT_VOUT_UV_WARNING_ALERT_BIT, reg_val),
		     "VOUT UV warning alert bit must remain set");
}

ZTEST_F(ltc4286_alert, test_disable_iout_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_IOUT_MASK,
					 LTC4286_ALERT_IOUT_OC_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to disable IOUT OC fault alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_IOUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read IOUT alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_IOUT_OC_FAULT_ALERT_BIT, reg_val),
		     "IOUT OC fault alert bit should be set after disable");

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_IOUT_MASK,
					 LTC4286_ALERT_IOUT_OC_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable IOUT OC warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_IOUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read IOUT alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_IOUT_OC_WARNING_ALERT_BIT, reg_val),
		     "IOUT OC warning alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_iout_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_IOUT_MASK,
					 LTC4286_ALERT_IOUT_OC_FAULT_ALERT_BIT |
						 LTC4286_ALERT_IOUT_OC_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable IOUT alerts");

	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_IOUT_MASK,
					LTC4286_ALERT_IOUT_OC_FAULT_ALERT_BIT |
						LTC4286_ALERT_IOUT_OC_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to enable IOUT alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_IOUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read IOUT alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_IOUT_OC_FAULT_ALERT_BIT, reg_val),
		      "IOUT OC fault alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_IOUT_OC_WARNING_ALERT_BIT, reg_val),
		      "IOUT OC warning alert bit should be clear after enable");
}

ZTEST_F(ltc4286_alert, test_disable_input_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_INPUT_MASK,
					 LTC4286_ALERT_VIN_OV_FAULT_ALERT_BIT |
						 LTC4286_ALERT_VIN_OV_WARNING_ALERT_BIT |
						 LTC4286_ALERT_VIN_UV_WARNING_ALERT_BIT |
						 LTC4286_ALERT_VIN_UV_FAULT_ALERT_BIT |
						 LTC4286_ALERT_PIN_OP_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable INPUT alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_INPUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read INPUT alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_VIN_OV_FAULT_ALERT_BIT, reg_val),
		     "VIN OV fault alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_VIN_OV_WARNING_ALERT_BIT, reg_val),
		     "VIN OV warning alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_VIN_UV_WARNING_ALERT_BIT, reg_val),
		     "VIN UV warning alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_VIN_UV_FAULT_ALERT_BIT, reg_val),
		     "VIN UV fault alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_PIN_OP_WARNING_ALERT_BIT, reg_val),
		     "PIN OP warning alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_input_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_INPUT_MASK,
					 LTC4286_ALERT_VIN_OV_FAULT_ALERT_BIT |
						 LTC4286_ALERT_PIN_OP_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable INPUT alerts");

	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_INPUT_MASK,
					LTC4286_ALERT_VIN_OV_FAULT_ALERT_BIT |
						LTC4286_ALERT_PIN_OP_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to enable INPUT alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_INPUT_MASK, &reg_val);
	zassert_ok(ret, "Failed to read INPUT alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_VIN_OV_FAULT_ALERT_BIT, reg_val),
		      "VIN OV fault alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_PIN_OP_WARNING_ALERT_BIT, reg_val),
		      "PIN OP warning alert bit should be clear after enable");
}

ZTEST_F(ltc4286_alert, test_disable_temp_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					 LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT |
						 LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT |
						 LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable TEMP alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_TEMP_MASK, &reg_val);
	zassert_ok(ret, "Failed to read TEMP alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT, reg_val),
		     "TEMP OT fault alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT, reg_val),
		     "TEMP OT warning alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT, reg_val),
		     "TEMP UT warning alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_temp_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					 LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT |
						 LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT |
						 LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable TEMP alerts");

	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT |
						LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT |
						LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to enable TEMP alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_TEMP_MASK, &reg_val);
	zassert_ok(ret, "Failed to read TEMP alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT, reg_val),
		      "TEMP OT fault alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT, reg_val),
		      "TEMP OT warning alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT, reg_val),
		      "TEMP UT warning alert bit should be clear after enable");
}

ZTEST_F(ltc4286_alert, test_disable_comms_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_COMMS_MASK,
		LTC4286_ALERT_BAD_CMD_ALERT_BIT | LTC4286_ALERT_BAD_DATA_ALERT_BIT |
			LTC4286_ALERT_PEC_FAILED_ALERT_BIT | LTC4286_ALERT_MISC_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to disable COMMS alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_COMMS_MASK, &reg_val);
	zassert_ok(ret, "Failed to read COMMS alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_BAD_CMD_ALERT_BIT, reg_val),
		     "BAD_CMD alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_BAD_DATA_ALERT_BIT, reg_val),
		     "BAD_DATA alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_PEC_FAILED_ALERT_BIT, reg_val),
		     "PEC_FAILED alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_MISC_FAULT_ALERT_BIT, reg_val),
		     "MISC_FAULT alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_comms_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_COMMS_MASK,
		LTC4286_ALERT_BAD_CMD_ALERT_BIT | LTC4286_ALERT_BAD_DATA_ALERT_BIT |
			LTC4286_ALERT_PEC_FAILED_ALERT_BIT | LTC4286_ALERT_MISC_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to disable COMMS alerts");

	ret = ltc4286_enable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_COMMS_MASK,
		LTC4286_ALERT_BAD_CMD_ALERT_BIT | LTC4286_ALERT_BAD_DATA_ALERT_BIT |
			LTC4286_ALERT_PEC_FAILED_ALERT_BIT | LTC4286_ALERT_MISC_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to enable COMMS alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_COMMS_MASK, &reg_val);
	zassert_ok(ret, "Failed to read COMMS alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_BAD_CMD_ALERT_BIT, reg_val),
		      "BAD_CMD alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_BAD_DATA_ALERT_BIT, reg_val),
		      "BAD_DATA alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_PEC_FAILED_ALERT_BIT, reg_val),
		      "PEC_FAILED alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MISC_FAULT_ALERT_BIT, reg_val),
		      "MISC_FAULT alert bit should be clear after enable");
}

ZTEST_F(ltc4286_alert, test_disable_mfr_spec_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_SPEC_MASK,
					 LTC4286_ALERT_MFR_SPEC_EN_CHANGED_ALERT_BIT |
						 LTC4286_ALERT_MFR_SPEC_TSD_FAULT_ALERT_BIT |
						 LTC4286_ALERT_MFR_SPEC_FET_BAD_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to disable MFR_SPEC alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_SPEC_MASK, &reg_val);
	zassert_ok(ret, "Failed to read MFR_SPEC alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SPEC_EN_CHANGED_ALERT_BIT, reg_val),
		     "EN_CHANGED alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SPEC_TSD_FAULT_ALERT_BIT, reg_val),
		     "TSD_FAULT alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SPEC_FET_BAD_FAULT_ALERT_BIT, reg_val),
		     "FET_BAD_FAULT alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_mfr_spec_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_SPEC_MASK,
					 LTC4286_ALERT_MFR_SPEC_EN_CHANGED_ALERT_BIT |
						 LTC4286_ALERT_MFR_SPEC_TSD_FAULT_ALERT_BIT |
						 LTC4286_ALERT_MFR_SPEC_FET_BAD_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to disable MFR_SPEC alerts");

	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_SPEC_MASK,
					LTC4286_ALERT_MFR_SPEC_EN_CHANGED_ALERT_BIT |
						LTC4286_ALERT_MFR_SPEC_TSD_FAULT_ALERT_BIT |
						LTC4286_ALERT_MFR_SPEC_FET_BAD_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to enable MFR_SPEC alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_SPEC_MASK, &reg_val);
	zassert_ok(ret, "Failed to read MFR_SPEC alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SPEC_EN_CHANGED_ALERT_BIT, reg_val),
		      "EN_CHANGED alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SPEC_TSD_FAULT_ALERT_BIT, reg_val),
		      "TSD_FAULT alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SPEC_FET_BAD_FAULT_ALERT_BIT, reg_val),
		      "FET_BAD_FAULT alert bit should be clear after enable");
}

/* ---------------------------------------------------------------------------
 * 2-byte alert mask registers (STATUS WORD, SYS STATUS 1, SYS STATUS 2)
 * ---------------------------------------------------------------------------
 */

ZTEST_F(ltc4286_alert, test_disable_status_word_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_WORD_MASK,
					 LTC4286_ALERT_STATUS_BUSY_ALERT_BIT);
	zassert_ok(ret, "Failed to disable STATUS_WORD busy alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_WORD_MASK, &reg_val);
	zassert_ok(ret, "Failed to read STATUS_WORD alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_STATUS_BUSY_ALERT_BIT, reg_val),
		     "STATUS_WORD busy alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_status_word_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_WORD_MASK,
					 LTC4286_ALERT_STATUS_BUSY_ALERT_BIT);
	zassert_ok(ret, "Failed to disable STATUS_WORD busy alert");

	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_WORD_MASK,
					LTC4286_ALERT_STATUS_BUSY_ALERT_BIT);
	zassert_ok(ret, "Failed to enable STATUS_WORD busy alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_WORD_MASK, &reg_val);
	zassert_ok(ret, "Failed to read STATUS_WORD alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_STATUS_BUSY_ALERT_BIT, reg_val),
		      "STATUS_WORD busy alert bit should be clear after enable");
}

ZTEST_F(ltc4286_alert, test_disable_sys1_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_SYS1_MASK,
		LTC4286_ALERT_MFR_SYS_STATUS1_RESET_DONE_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_POWER_LOSS_ALERT_BIT);
	zassert_ok(ret, "Failed to disable SYS STATUS 1 alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_SYS1_MASK, &reg_val);
	zassert_ok(ret, "Failed to read SYS STATUS 1 alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS1_RESET_DONE_ALERT_BIT, reg_val),
		     "SYS STATUS1 RESET_DONE alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS1_POWER_LOSS_ALERT_BIT, reg_val),
		     "SYS STATUS1 POWER_LOSS alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_sys1_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_SYS1_MASK,
		LTC4286_ALERT_MFR_SYS_STATUS1_ADC_CONV_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_AVERAGE_DONE_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_RESET_DONE_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_POWER_LOSS_ALERT_BIT);
	zassert_ok(ret, "Failed to disable SYS STATUS 1 alerts");

	ret = ltc4286_enable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_SYS1_MASK,
		LTC4286_ALERT_MFR_SYS_STATUS1_ADC_CONV_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_AVERAGE_DONE_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_RESET_DONE_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS1_POWER_LOSS_ALERT_BIT);
	zassert_ok(ret, "Failed to enable SYS STATUS 1 alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_SYS1_MASK, &reg_val);
	zassert_ok(ret, "Failed to read SYS STATUS 1 alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS1_ADC_CONV_ALERT_BIT, reg_val),
		      "SYS STATUS1 ADC_CONV alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS1_AVERAGE_DONE_ALERT_BIT, reg_val),
		      "SYS STATUS1 AVERAGE_DONE alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS1_RESET_DONE_ALERT_BIT, reg_val),
		      "SYS STATUS1 RESET_DONE alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS1_POWER_LOSS_ALERT_BIT, reg_val),
		      "SYS STATUS1 POWER_LOSS alert bit should be clear after enable");
}

ZTEST_F(ltc4286_alert, test_disable_sys2_alert_sets_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_SYS2_MASK,
		LTC4286_ALERT_MFR_SYS_STATUS2_FET_SHORT_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_POWER_FAILED_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable SYS STATUS 2 alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_SYS2_MASK, &reg_val);
	zassert_ok(ret, "Failed to read SYS STATUS 2 alert mask register");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_FET_SHORT_WARNING_ALERT_BIT, reg_val),
		     "SYS STATUS2 FET_SHORT alert bit should be set after disable");
	zassert_true(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_POWER_FAILED_WARNING_ALERT_BIT,
			       reg_val),
		     "SYS STATUS2 POWER_FAILED alert bit should be set after disable");
}

ZTEST_F(ltc4286_alert, test_enable_sys2_alert_clears_bit)
{
	uint16_t reg_val;
	int ret;

	ret = ltc4286_disable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_SYS2_MASK,
		LTC4286_ALERT_MFR_SYS_STATUS2_PIN_UP_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_IOUT_UC_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_VDS_UV_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_VDS_OV_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_FET_SHORT_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_POWER_FAILED_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable SYS STATUS 2 alerts");

	ret = ltc4286_enable_alert_mask(
		fixture->dev, LTC4286_ALERT_STATUS_SYS2_MASK,
		LTC4286_ALERT_MFR_SYS_STATUS2_PIN_UP_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_IOUT_UC_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_VDS_UV_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_VDS_OV_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_FET_SHORT_WARNING_ALERT_BIT |
			LTC4286_ALERT_MFR_SYS_STATUS2_POWER_FAILED_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to enable SYS STATUS 2 alerts");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_SYS2_MASK, &reg_val);
	zassert_ok(ret, "Failed to read SYS STATUS 2 alert mask register");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_PIN_UP_WARNING_ALERT_BIT, reg_val),
		      "SYS STATUS2 PIN_UP alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_IOUT_UC_WARNING_ALERT_BIT, reg_val),
		      "SYS STATUS2 IOUT_UC alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_VDS_UV_WARNING_ALERT_BIT, reg_val),
		      "SYS STATUS2 VDS_UV alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_VDS_OV_WARNING_ALERT_BIT, reg_val),
		      "SYS STATUS2 VDS_OV alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_FET_SHORT_WARNING_ALERT_BIT, reg_val),
		      "SYS STATUS2 FET_SHORT alert bit should be clear after enable");
	zassert_false(FIELD_GET(LTC4286_ALERT_MFR_SYS_STATUS2_POWER_FAILED_WARNING_ALERT_BIT,
				reg_val),
		      "SYS STATUS2 POWER_FAILED alert bit should be clear after enable");
}

/* ---------------------------------------------------------------------------
 * Toggle test: disable → enable → disable, verify each transition
 * ---------------------------------------------------------------------------
 */

ZTEST_F(ltc4286_alert, test_alert_toggle)
{
	uint16_t reg_val;
	int ret;

	/* Step 1: disable (mask) — bit must become 1 */
	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					 LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Step 1: Failed to disable TEMP OT warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_TEMP_MASK, &reg_val);
	zassert_ok(ret, "Failed to read TEMP alert mask register (step 1)");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT, reg_val),
		     "Step 1: TEMP OT warning alert bit should be set");

	/* Step 2: enable (unmask) — bit must go back to 0 */
	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Step 2: Failed to enable TEMP OT warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_TEMP_MASK, &reg_val);
	zassert_ok(ret, "Failed to read TEMP alert mask register (step 2)");
	zassert_false(FIELD_GET(LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT, reg_val),
		      "Step 2: TEMP OT warning alert bit should be clear");

	/* Step 3: disable again — bit must be set once more */
	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					 LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Step 3: Failed to disable TEMP OT warning alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_TEMP_MASK, &reg_val);
	zassert_ok(ret, "Failed to read TEMP alert mask register (step 3)");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT, reg_val),
		     "Step 3: TEMP OT warning alert bit should be set again");
}

/* ---------------------------------------------------------------------------
 * Independence test: operating on one bit must not affect other bits
 * ---------------------------------------------------------------------------
 */

ZTEST_F(ltc4286_alert, test_alert_bit_independence)
{
	uint16_t reg_val;
	int ret;

	/* Disable all three TEMP alert bits */
	ret = ltc4286_disable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					 LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT |
						 LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT |
						 LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT);
	zassert_ok(ret, "Failed to disable all TEMP alerts");

	/* Enable only OT_FAULT — the other two must remain set */
	ret = ltc4286_enable_alert_mask(fixture->dev, LTC4286_ALERT_STATUS_TEMP_MASK,
					LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT);
	zassert_ok(ret, "Failed to enable TEMP OT fault alert");

	ret = ltc4286_emul_get_register(fixture->mock->data,
					(uint8_t)LTC4286_ALERT_STATUS_TEMP_MASK, &reg_val);
	zassert_ok(ret, "Failed to read TEMP alert mask register");

	zassert_false(FIELD_GET(LTC4286_ALERT_TEMP_OT_FAULT_ALERT_BIT, reg_val),
		      "OT_FAULT bit should be clear after selective enable");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_OT_WARNING_ALERT_BIT, reg_val),
		     "OT_WARNING bit must remain set (not touched)");
	zassert_true(FIELD_GET(LTC4286_ALERT_TEMP_UT_WARNING_ALERT_BIT, reg_val),
		     "UT_WARNING bit must remain set (not touched)");
}

ZTEST_F(ltc4286_alert, test_invalid_alert_mask_returns_einval)
{
	int ret;

	ret = ltc4286_enable_alert_mask(fixture->dev, (enum ltc4286_trig_alert_mask)0xFF, BIT(0));
	zassert_equal(ret, -EINVAL, "enable with invalid alert_reg should return -EINVAL, got %d",
		      ret);

	ret = ltc4286_disable_alert_mask(fixture->dev, (enum ltc4286_trig_alert_mask)0xFF, BIT(0));
	zassert_equal(ret, -EINVAL, "disable with invalid alert_reg should return -EINVAL, got %d",
		      ret);
}

ZTEST_SUITE(ltc4286_alert, NULL, ltc4286_alert_setup, NULL, ltc4286_alert_after, NULL);
