/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/i2c.h>
#include <zephyr.h>
#include <ztest.h>

#define I2C_DEV_NAME DT_LABEL(DT_ALIAS(i2c1))

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
void test_i2c_pca95xx(void)
{
	int32_t ret;
	uint8_t datas[3];
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;

	/* get i2c device */
	const struct device *i2c_dev = device_get_binding(I2C_DEV_NAME);

	zassert_true(i2c_dev, "Cannot get i2c device");

	/* configure i2c device */
	ret = i2c_configure(i2c_dev, i2c_cfg);
	zassert_true(ret == 0, "Failed to configure i2c device");

	/* write configuration to register 6 and 7 of PCA95XX*/
	(void)memset(datas, 0, 3);
	datas[0] = REG_CONF_PORT0;
	datas[1] = test_data[0];
	datas[2] = test_data[1];
	ret = i2c_write(i2c_dev, datas, 3, 0x26);
	zassert_true(ret == 0, "Failed to write data to i2c device");

	/* read configuration from register 6 and 7 */
	(void)memset(datas, 0, 3);
	datas[0] = REG_CONF_PORT0;
	ret = i2c_write(i2c_dev, datas, 1, 0x26);
	zassert_true(ret == 0, "Failed to write data to i2c device");

	(void)memset(datas, 0, 3);
	ret = i2c_read(i2c_dev, datas, 2, 0x26);
	zassert_true(ret == 0, "Failed to read data from i2c device");

	/* check read data whether correct */
	ret = memcmp(datas, test_data, 2);
	zassert_true(ret == 0, "Read data is different to write data");
}

void test_main(void)
{
	ztest_test_suite(i2c_test,
			ztest_unit_test(test_i2c_pca95xx)
			);
	ztest_run_test_suite(i2c_test);
}
