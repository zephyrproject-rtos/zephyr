/*
 * SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/ztest.h>

#include "fake_i2c_dev_bottom.h"

/* The fake adapters are provided by fake_i2c_dev_bottom.c */
static const struct device *const i2c = DEVICE_DT_GET(DT_NODELABEL(fake_i2c));
static const struct device *const smbus = DEVICE_DT_GET(DT_NODELABEL(smbus_i2c));
static const struct device *const missing = DEVICE_DT_GET(DT_NODELABEL(missing_i2c));

#define ADDR FAKE_I2C_DEV_ADDR

static void *i2c_native_linux_setup(void)
{
	zassert_true(device_is_ready(i2c), "I2C controller not ready");

	return NULL;
}

static void i2c_native_linux_before(void *fixture)
{
	ARG_UNUSED(fixture);

	fake_i2c_dev_reset();
}

static void assert_msg(int transfer, int msg, bool read, int len)
{
	zassert_equal(fake_i2c_dev_msg_read(transfer, msg), read ? 1 : 0,
		      "transfer %d message %d direction", transfer, msg);
	zassert_equal(fake_i2c_dev_msg_len(transfer, msg), len, "transfer %d message %d length",
		      transfer, msg);
}

ZTEST(i2c_native_linux, test_init_errors)
{
	zassert_false(device_is_ready(smbus), "SMBus only adapter accepted");
	zassert_false(device_is_ready(missing), "missing i2c-dev node accepted");
}

ZTEST(i2c_native_linux, test_config)
{
	uint32_t config;

	zassert_ok(i2c_get_config(i2c, &config));
	zassert_equal(config, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_FAST));

	zassert_ok(i2c_configure(i2c, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD)));
	zassert_ok(i2c_get_config(i2c, &config));
	zassert_equal(config, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD));

	zassert_equal(i2c_configure(i2c, I2C_SPEED_SET(I2C_SPEED_STANDARD)), -ENOTSUP);
}

ZTEST(i2c_native_linux, test_burst_write_read)
{
	const uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
	uint8_t buf[sizeof(data)] = {0};

	zassert_ok(i2c_burst_write(i2c, ADDR, 0x10, data, sizeof(data)));
	zassert_ok(i2c_burst_read(i2c, ADDR, 0x10, buf, sizeof(buf)));
	zassert_mem_equal(buf, data, sizeof(data));

	zassert_equal(fake_i2c_dev_transfers(), 2);

	/* The register address and the data are joined in one message */
	zassert_equal(fake_i2c_dev_msgs(0), 1);
	assert_msg(0, 0, false, 1 + sizeof(data));

	zassert_equal(fake_i2c_dev_msgs(1), 2);
	assert_msg(1, 0, false, 1);
	assert_msg(1, 1, true, sizeof(data));
}

ZTEST(i2c_native_linux, test_continued_read)
{
	const uint8_t data[] = {0xa1, 0xa2, 0xa3, 0xa4, 0xa5};
	uint8_t reg = 0x40;
	uint8_t head[2] = {0};
	uint8_t tail[3] = {0};
	struct i2c_msg msgs[] = {
		{.buf = &reg, .len = 1, .flags = I2C_MSG_WRITE},
		{.buf = head, .len = sizeof(head), .flags = I2C_MSG_READ | I2C_MSG_RESTART},
		{.buf = tail, .len = sizeof(tail), .flags = I2C_MSG_READ | I2C_MSG_STOP},
	};

	zassert_ok(i2c_burst_write(i2c, ADDR, reg, data, sizeof(data)));
	fake_i2c_dev_reset();

	zassert_ok(i2c_transfer(i2c, msgs, ARRAY_SIZE(msgs), ADDR));
	zassert_mem_equal(head, &data[0], sizeof(head));
	zassert_mem_equal(tail, &data[sizeof(head)], sizeof(tail));

	/* Both reads are received as one message */
	zassert_equal(fake_i2c_dev_transfers(), 1);
	zassert_equal(fake_i2c_dev_msgs(0), 2);
	assert_msg(0, 0, false, 1);
	assert_msg(0, 1, true, sizeof(head) + sizeof(tail));
}

ZTEST(i2c_native_linux, test_stop_splits_transfer)
{
	uint8_t wr[] = {0x20, 0x5a};
	uint8_t reg = 0x20;
	uint8_t rd = 0;
	struct i2c_msg msgs[] = {
		{.buf = wr, .len = sizeof(wr), .flags = I2C_MSG_WRITE | I2C_MSG_STOP},
		{.buf = &reg, .len = 1, .flags = I2C_MSG_WRITE},
		{.buf = &rd, .len = 1, .flags = I2C_MSG_READ | I2C_MSG_RESTART | I2C_MSG_STOP},
	};

	zassert_ok(i2c_transfer(i2c, msgs, ARRAY_SIZE(msgs), ADDR));
	zassert_equal(rd, 0x5a);

	zassert_equal(fake_i2c_dev_transfers(), 2);
	zassert_equal(fake_i2c_dev_msgs(0), 1);
	assert_msg(0, 0, false, sizeof(wr));
	zassert_equal(fake_i2c_dev_msgs(1), 2);
	assert_msg(1, 0, false, 1);
	assert_msg(1, 1, true, 1);
}

ZTEST(i2c_native_linux, test_ten_bit_address)
{
	uint8_t wr[] = {0x00, 0x01};
	struct i2c_msg msg = {
		.buf = wr,
		.len = sizeof(wr),
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP | I2C_MSG_ADDR_10_BITS,
	};

	zassert_ok(i2c_transfer(i2c, &msg, 1, ADDR));
	zassert_equal(fake_i2c_dev_msg_ten(0, 0), 1);
}

ZTEST(i2c_native_linux, test_nack)
{
	uint8_t byte = 0;

	zassert_equal(i2c_write(i2c, &byte, 1, ADDR + 1), -EIO);
	zassert_equal(i2c_read(i2c, &byte, 1, ADDR + 1), -EIO);
}

ZTEST(i2c_native_linux, test_bounce_overflow)
{
	static uint8_t data[CONFIG_I2C_NATIVE_LINUX_BOUNCE_SIZE];

	/* The register address does not fit next to a full buffer of data */
	zassert_equal(i2c_burst_write(i2c, ADDR, 0x00, data, sizeof(data)), -ENOMEM);
	zassert_equal(fake_i2c_dev_transfers(), 0);

	/* A single message is passed without copying, whatever its length */
	zassert_ok(i2c_write(i2c, data, sizeof(data), ADDR));
}

ZTEST_SUITE(i2c_native_linux, NULL, i2c_native_linux_setup, i2c_native_linux_before, NULL, NULL);
