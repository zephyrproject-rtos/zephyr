/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* command to configure port direction of NXP PCA95xx */
#define REG_CONF_PORT0 0x06U

/* test data used to write into registers */
uint8_t test_data[2] = {0xAA, 0xAA};

/**
 * @brief Test i2c api by communicating with pca95xx
 * @details
 * - get i2c mainline device
 * - write data into pca95xx
 * - read data from pca95xx
 * - check read data whether correct
 */
ZTEST(boards_mec172x_pca95xx, test_i2c_pca95xx)
{
	int32_t ret;
	uint8_t datas[3];
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	/* get i2c device */
	const struct i2c_dt_spec i2c = I2C_DT_SPEC_GET(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pca9555));

	zassert_true(device_is_ready(i2c.bus), "I2C controller device is not ready");

	/* configure i2c device */
	ret = i2c_configure(i2c.bus, i2c_cfg);
	zassert_true(ret == 0, "Failed to configure i2c device");

	/* write configuration to register 6 and 7 of PCA95XX*/
	(void)memset(datas, 0, 3);
	datas[0] = REG_CONF_PORT0;
	datas[1] = test_data[0];
	datas[2] = test_data[1];
	ret = i2c_write_dt(&i2c, datas, 3);
	zassert_true(ret == 0, "Failed to write data to i2c device");

	/* read configuration from register 6 and 7 */
	(void)memset(datas, 0, 3);
	datas[0] = REG_CONF_PORT0;
	ret = i2c_write_dt(&i2c, datas, 1);
	zassert_true(ret == 0, "Failed to write data to i2c device");

	(void)memset(datas, 0, 3);
	ret = i2c_read_dt(&i2c, datas, 2);
	zassert_true(ret == 0, "Failed to read data from i2c device");

	/* check read data whether correct */
	ret = memcmp(datas, test_data, 2);
	zassert_true(ret == 0, "Read data is different to write data");
}

ZTEST_SUITE(boards_mec172x_pca95xx, NULL, NULL, NULL, NULL, NULL);
