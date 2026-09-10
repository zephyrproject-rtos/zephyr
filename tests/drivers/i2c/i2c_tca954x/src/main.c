/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(i2c_channel_0))
#define I2C_0_CTRL_NODE_ID      DT_ALIAS(i2c_channel_0)
#else
#error "I2C 0 controller device not found"
#endif

#if DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(i2c_channel_1))
#define I2C_1_CTRL_NODE_ID      DT_ALIAS(i2c_channel_1)
#else
#error "I2C 1 controller device not found"
#endif




/**
 * @brief Test Asserts
 *
 * This test verifies various assert macros provided by ztest.
 *
 */
ZTEST(i2c_tca954x, test_tca954x)
{
	uint8_t buff[1];

	const struct device *const i2c0 = DEVICE_DT_GET(I2C_0_CTRL_NODE_ID);
	const struct device *const i2c1 = DEVICE_DT_GET(I2C_1_CTRL_NODE_ID);

	zassert_true(device_is_ready(i2c0), "I2C 0 not ready");
	zassert_true(device_is_ready(i2c1), "I2C 1 not ready");

	i2c_read(i2c0, buff, 1, 0x42);
	i2c_read(i2c1, buff, 1, 0x42);
}

ZTEST_SUITE(i2c_tca954x, NULL, NULL, NULL, NULL, NULL);
