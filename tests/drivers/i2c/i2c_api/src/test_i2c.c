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

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_STATUS(DT_ALIAS(i2c_0), okay)
#define I2C_DEV_NODE DT_ALIAS(i2c_0)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_1), okay)
#define I2C_DEV_NODE DT_ALIAS(i2c_1)
#elif DT_NODE_HAS_STATUS(DT_ALIAS(i2c_2), okay)
#define I2C_DEV_NODE DT_ALIAS(i2c_2)
#else
#error "Please set the correct I2C device"
#endif

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

#define GY271_HMC_ADDR (0x1E)
#define GY271_QMC_ADDR (0x0D)

#if defined(CONFIG_SENSOR_GY271_QMC)
#define GY271_ADDR GY271_QMC_ADDR
#elif defined(CONFIG_SENSOR_GY271_HMC)
#define GY271_ADDR GY271_HMC_ADDR
#else
#error "No sensor type defined"
#endif

static int test_gy271(void)
{
	unsigned char datas[6];
	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
	uint32_t i2c_cfg_tmp;

	if (!device_is_ready(i2c_dev)) {
		TC_PRINT("I2C device is not ready\n");
		return TC_FAIL;
	}

	/* 1. Verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	/* 2. Verify i2c_get_config() */
	if (i2c_get_config(i2c_dev, &i2c_cfg_tmp)) {
		TC_PRINT("I2C get_config failed\n");
		return TC_FAIL;
	}
	if (i2c_cfg != i2c_cfg_tmp) {
		TC_PRINT("I2C get_config returned invalid config\n");
		return TC_FAIL;
	}

#ifdef CONFIG_SENSOR_GY271_QMC
	datas[0] = 0x09;
	datas[1] = 0x01;

	if (i2c_write(i2c_dev, datas, 2, GY271_ADDR)) {
		TC_PRINT("Fail to configure sensor GY271\n");
		return TC_FAIL;
	}
#else /* GY271 HMC */
	datas[0] = 0x01;
	datas[1] = 0x20;

	/* 3. verify i2c_write() */
	if (i2c_write(i2c_dev, datas, 2, GY271_ADDR)) {
		TC_PRINT("Fail to configure sensor GY271\n");
		return TC_FAIL;
	}

	datas[0] = 0x02;
	datas[1] = 0x00;
	if (i2c_write(i2c_dev, datas, 2, GY271_ADDR)) {
		TC_PRINT("Fail to configure sensor GY271\n");
		return TC_FAIL;
	}
#endif

	k_sleep(K_MSEC(1));

#ifdef CONFIG_SENSOR_GY271_QMC
	/* Sensor data bits start from 0x00 to 0x05 */
	datas[0] = 0x00;
#else /* GY271 HMC */
	datas[0] = 0x03;
#endif

	if (i2c_write(i2c_dev, datas, 1, GY271_ADDR)) {
		TC_PRINT("Fail to write to sensor GY271\n");
		return TC_FAIL;
	}

	(void)memset(datas, 0, sizeof(datas));

	/* 4. verify i2c_read() */
	if (i2c_read(i2c_dev, datas, 6, GY271_ADDR)) {
		TC_PRINT("Fail to fetch sample from sensor GY271\n");
		return TC_FAIL;
	}

	TC_PRINT("axis raw data: %d %d %d %d %d %d\n", datas[0], datas[1], datas[2], datas[3],
		 datas[4], datas[5]);

	return TC_PASS;
}

static int test_burst_gy271(void)
{
	unsigned char datas[6];
	const struct device *const i2c_dev = DEVICE_DT_GET(I2C_DEV_NODE);
	uint32_t i2c_cfg_tmp;

	if (!device_is_ready(i2c_dev)) {
		TC_PRINT("I2C device is not ready\n");
		return TC_FAIL;
	}

	/* 1. verify i2c_configure() */
	if (i2c_configure(i2c_dev, i2c_cfg)) {
		TC_PRINT("I2C config failed\n");
		return TC_FAIL;
	}

	/* 2. Verify i2c_get_config() */
	if (i2c_get_config(i2c_dev, &i2c_cfg_tmp)) {
		TC_PRINT("I2C get_config failed\n");
		return TC_FAIL;
	}
	if (i2c_cfg != i2c_cfg_tmp) {
		TC_PRINT("I2C get_config returned invalid config\n");
		return TC_FAIL;
	}

#ifdef CONFIG_SENSOR_GY271_QMC
	datas[0] = 0x09;
	datas[1] = 0x01;

	if (i2c_burst_write(i2c_dev, GY271_ADDR, 0x00, datas, 2)) {
		TC_PRINT("Fail to configure sensor GY271 QMC\n");
	}
#else
	datas[0] = 0x01;
	datas[1] = 0x20;
	datas[2] = 0x02;
	datas[3] = 0x00;

	/* 3. verify i2c_burst_write() */
	if (i2c_burst_write(i2c_dev, GY271_ADDR, 0x00, datas, 4)) {
		TC_PRINT("Fail to write to sensor GY271\n");
		return TC_FAIL;
	}

	k_sleep(K_MSEC(1));

	(void)memset(datas, 0, sizeof(datas));
#endif

#ifdef CONFIG_SENSOR_GY271_QMC
	/* Sensor data bits start from 0x00 to 0x05 */
	int start_bit = 0x00;
#else /* GY271 HMC */
	int start_bit = 0x03;
#endif
	/* 4. verify i2c_burst_read() */
	if (i2c_burst_read(i2c_dev, GY271_ADDR, start_bit, datas, 6)) {
		TC_PRINT("Fail to fetch sample from sensor GY271\n");
		return TC_FAIL;
	}

	TC_PRINT("axis raw data: %d %d %d %d %d %d\n", datas[0], datas[1], datas[2], datas[3],
		 datas[4], datas[5]);

	return TC_PASS;
}

ZTEST(i2c_gy271, test_i2c_gy271)
{
	zassert_true(test_gy271() == TC_PASS);
}

ZTEST(i2c_gy271, test_i2c_burst_gy271)
{
	zassert_true(test_burst_gy271() == TC_PASS);
}

ZTEST_SUITE(i2c_gy271, NULL, NULL, NULL, NULL, NULL);
