/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup t_i2c_basic
 * @{
 * @defgroup t_i2c_read_write test_i2c_read_write
 * @brief TestPurpose: verify I2C master can read and write
 * @}
 */

#include <i2c.h>
#include <zephyr.h>
#include <ztest.h>

#ifdef CONFIG_ARC
#define I2C_DEV_NAME CONFIG_I2C_SS_0_NAME
#else
#define I2C_DEV_NAME CONFIG_I2C_0_NAME
#endif

u32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_MASTER;

static int test_gy271(void)
{
	unsigned char datas[6];
	struct device *i2c_dev = device_get_binding(I2C_DEV_NAME);

	if (!i2c_dev) {
		TC_PRINT("Cannot get I2C device\n");
		return TC_FAIL;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	datas[0] = 0x01;
	datas[1] = 0x20;

	/* 2. verify i2c_write() */
	if (i2c_write(i2c_dev, datas, 2, 0x1E)) {
		TC_PRINT("Fail to configure sensor GY271\n");
		return TC_FAIL;
	}

	datas[0] = 0x02;
	datas[1] = 0x00;
	if (i2c_write(i2c_dev, datas, 2, 0x1E)) {
		TC_PRINT("Fail to configure sensor GY271\n");
		return TC_FAIL;
	}

	k_sleep(1);

	datas[0] = 0x03;
	if (i2c_write(i2c_dev, datas, 1, 0x1E)) {
		TC_PRINT("Fail to write to sensor GY271\n");
		return TC_FAIL;
	}

	memset(datas, 0, sizeof(datas));

	/* 3. verify i2c_read() */
	if (i2c_read(i2c_dev, datas, 6, 0x1E)) {
		TC_PRINT("Fail to fetch sample from sensor GY271\n");
		return TC_FAIL;
	}

	TC_PRINT("axis raw data: %d %d %d %d %d %d\n",
				datas[0], datas[1], datas[2],
				datas[3], datas[4], datas[5]);

	return TC_PASS;
}

static int test_burst_gy271(void)
{
	unsigned char datas[6];
	struct device *i2c_dev = device_get_binding(I2C_DEV_NAME);

	if (!i2c_dev) {
		TC_PRINT("Cannot get I2C device\n");
		return TC_FAIL;
	}

	/* 1. verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	datas[0] = 0x01;
	datas[1] = 0x20;
	datas[2] = 0x02;
	datas[3] = 0x00;

	/* 2. verify i2c_burst_write() */
	if (i2c_burst_write(i2c_dev, 0x1E, 0x00, datas, 4)) {
		TC_PRINT("Fail to write to sensor GY271\n");
		return TC_FAIL;
	}

	k_sleep(1);

	memset(datas, 0, sizeof(datas));

	/* 3. verify i2c_burst_read() */
	if (i2c_burst_read(i2c_dev, 0x1E, 0x03, datas, 6)) {
		TC_PRINT("Fail to fetch sample from sensor GY271\n");
		return TC_FAIL;
	}

	TC_PRINT("axis raw data: %d %d %d %d %d %d\n",
				datas[0], datas[1], datas[2],
				datas[3], datas[4], datas[5]);

	return TC_PASS;
}

void test_i2c_gy271(void)
{
	zassert_true(test_gy271() == TC_PASS, NULL);
}

void test_i2c_burst_gy271(void)
{
	zassert_true(test_burst_gy271() == TC_PASS, NULL);
}
