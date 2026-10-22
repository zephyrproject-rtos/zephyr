/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/util.h>

#include <smbus_utils.h>

ZTEST(smbus_pec, test_smbus_pec)
{
	uint8_t addr = 0x42;
	/* Write Block with PEC (SMBus spec v3.1, Section 6.5.7) */
	uint8_t write_data[] = {
		0x73, /* command */
		4, /* len */
		0xde, 0xad, 0xbe, 0xef, /* data */
	};
	struct i2c_msg msgs[] = {
		{
			.buf = write_data,
			.len = sizeof(write_data),
			.flags = I2C_MSG_WRITE | I2C_MSG_STOP,
		},
		/* driver would add a PEC message here */
	};

	uint8_t actual_pec = smbus_pec(addr, msgs, ARRAY_SIZE(msgs));
	uint8_t expected_pec = 0x12;

	zexpect_equal(expected_pec, actual_pec, "expected: %02x actual: %02x",
		      expected_pec, actual_pec);
}

ZTEST(smbus_pec, test_smbus_read_check_pec)
{
	uint8_t addr = 0xa;

	{
		/* Read Byte with PEC (SMBus spec v3.1, Section 6.5.5) */
		uint8_t data[] = {
			0x10, /* command */
			0x05, /* data */
			0x90, /* PEC */
		};
		struct i2c_msg msgs[] = {
			{
				.buf = &data[0], /* command */
				.len = 1,
				.flags = I2C_MSG_WRITE,
			},
			{
				.buf = &data[1], /* data */
				.len = 1,
				.flags = I2C_MSG_READ,
			},
			{
				.buf = &data[2], /* PEC */
				.len = 1,
				.flags = I2C_MSG_READ,
			},
		};

		zexpect_ok(smbus_read_check_pec(SMBUS_MODE_PEC, addr, msgs, ARRAY_SIZE(msgs)));
	}

	{
		/* Read Word with PEC (SMBus spec v3.1, Section 6.5.5) */
		uint8_t data[] = {
			0x10, /* command */
			0x05, /* data byte (low) */
			0x0a, /* data byte (high) */
			0xcf, /* PEC */
		};
		struct i2c_msg msgs[] = {
			{
				.buf = &data[0], /* command */
				.len = 1,
				.flags = I2C_MSG_WRITE,
			},
			{
				.buf = &data[1], /* data */
				.len = 2,
				.flags = I2C_MSG_READ,
			},
			{
				.buf = &data[3], /* PEC */
				.len = 1,
				.flags = I2C_MSG_READ,
			},
		};

		zexpect_ok(smbus_read_check_pec(SMBUS_MODE_PEC, addr, msgs, ARRAY_SIZE(msgs)));
	}

	{
		/* Block read (SMBus spec v3.1, Section 6.5.7) */
		uint8_t data[] = {
			0x10,                               /* command */
			0x06,                               /* block count */
			0x05, 0x00, 0x00, 0x00, 0x00, 0x00, /* data */
			0x99,                               /* PEC */
		};
		struct i2c_msg msgs[] = {
			{
				.buf = &data[0], /* command */
				.len = 1,
				.flags = I2C_MSG_WRITE,
			},
			{
				.buf = &data[1], /* block count */
				.len = 1,
				.flags = I2C_MSG_READ | I2C_MSG_RESTART,
			},
			{
				.buf = &data[2], /* data */
				.len = data[1],
				.flags = I2C_MSG_READ,
			},
			{
				.buf = &data[2] + data[1], /* PEC */
				.len = 1,
				.flags = I2C_MSG_READ,
			},
		};

		zexpect_ok(smbus_read_check_pec(SMBUS_MODE_PEC, addr, msgs, ARRAY_SIZE(msgs)));
	}
}

ZTEST_SUITE(smbus_pec, NULL, NULL, NULL, NULL, NULL);
