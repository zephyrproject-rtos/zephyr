/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <zephyr.h>
#include <ztest.h>

/*
 * For ADV7513 Programming details, please
 * refer to the following link.
 * https://ez.analog.com/docs/DOC-1986
 */

#define ADV7513_HDMI_I2C_SLAVE_ADDR	0x39

#define ADV7513_CHIP_REVISION_REG	0x0
#define CHIP_REVISION_VAL		0x13

#define ADV7513_MAIN_POWER_REG		0x41
#define POWER_ON_VAL			0x10

#define ADV7513_HPD_CTRL_REG		0xD6
#define HPD_CTRL_VAL			0xC0

#define ADV7513_WRITE_TEST_REG		0x2
#define WRITE_TEST_VAL			0x66

static int powerup_adv7513(const struct device *i2c_dev)
{
	uint8_t data;

	TC_PRINT("Powering up ADV7513\n");
	/* write to HPD control registers */
	if (i2c_reg_write_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				ADV7513_HPD_CTRL_REG, HPD_CTRL_VAL)) {
		TC_PRINT("i2c write fail\n");
		return TC_FAIL;
	}
	if (i2c_reg_read_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				0xD6, &data)) {
		TC_PRINT("failed to read HPD control\n");
		return TC_FAIL;
	}
	TC_PRINT("HPD control 0x%x\n", data);

	/* write to power control registers */
	if (i2c_reg_write_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				ADV7513_MAIN_POWER_REG, POWER_ON_VAL)) {
		TC_PRINT("i2c write fail\n");
		return TC_FAIL;
	}

	if (i2c_reg_read_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				0x41, &data)) {
		TC_PRINT("failed to read Power state\n");
		return TC_FAIL;
	}
	TC_PRINT("Power state 0x%x\n", data);

	return TC_PASS;
}

static int test_i2c_adv7513(void)
{
	const struct device *i2c_dev = device_get_binding(DT_LABEL(DT_INST(0, nios2_i2c)));
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;
	uint8_t data;

	if (!i2c_dev) {
		TC_PRINT("cannot get i2c device\n");
		return TC_FAIL;
	}

	/* Test i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("i2c config failed\n");
		return TC_FAIL;
	}

	/* Power up ADV7513 */
	zassert_true(powerup_adv7513(i2c_dev) == TC_PASS,
					"ADV7513 power up failed");

	TC_PRINT("*** Running i2c read/write tests ***\n");
	/* Test i2c byte read */
	data = 0x0;
	if (i2c_reg_read_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				ADV7513_CHIP_REVISION_REG, &data)) {
		TC_PRINT("failed to read chip revision\n");
		return TC_FAIL;
	}
	if (data != CHIP_REVISION_VAL) {
		TC_PRINT("chip revision does not match 0x%x\n", data);
		return TC_FAIL;
	}
	TC_PRINT("i2c read test passed\n");


	/* Test i2c byte write */
	data = WRITE_TEST_VAL;
	if (i2c_reg_write_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				ADV7513_WRITE_TEST_REG, data)) {
		TC_PRINT("i2c write fail\n");
		return TC_FAIL;
	}
	data = 0x0;
	if (i2c_reg_read_byte(i2c_dev, ADV7513_HDMI_I2C_SLAVE_ADDR,
				ADV7513_WRITE_TEST_REG, &data)) {
		TC_PRINT("i2c read fail\n");
		return TC_FAIL;
	}
	if (data != WRITE_TEST_VAL) {
		TC_PRINT("i2c write test failed 0x%x\n", data);
		return TC_FAIL;
	}
	TC_PRINT("i2c write & verify test passed\n");

	return TC_PASS;
}

void test_i2c_master(void)
{
	zassert_true(test_i2c_adv7513() == TC_PASS, NULL);
}

void test_main(void)
{
	ztest_test_suite(nios2_i2c_master_test,
			 ztest_unit_test(test_i2c_master));
	ztest_run_test_suite(nios2_i2c_master_test);
}
