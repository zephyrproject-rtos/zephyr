/*
 * Copyright (c) 2023 Google, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_pec_test_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define I2C_BUS DEVICE_DT_GET(DT_NODELABEL(i2c_bus))
#define EMUL_DEV EMUL_DT_GET(DT_NODELABEL(i2c_pec_test))
#define DEV_ADDR 0x0b

static const uint8_t test_pattern[] = {
	16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15
};
static uint8_t buf[sizeof(test_pattern)];

static const struct i2c_dt_spec dt = {
	.bus = I2C_BUS,
	.addr = DEV_ADDR,
};

/* Validate reset state. */
ZTEST(i2c_pec, test_i2c_pec_reset)
{
	int rc;
	const uint8_t reset_state[] = {
		16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	memset(buf, 0xff, sizeof(buf));
	rc = i2c_pec_burst_read_dt(&dt, 0, buf, sizeof(test_pattern));
	zassert_ok(rc);
	zassert_ok(memcmp(buf, reset_state, sizeof(reset_state)));
	zassert_true(i2c_pec_test_emul_is_idle(EMUL_DEV));
	zassert_false(i2c_pec_test_emul_get_corrupt(EMUL_DEV));
}

/* Validate normal R/W. */
ZTEST(i2c_pec, test_i2c_pec_rw)
{
	int rc;
	/*
	 * PEC data:
	 * 0x16 (I2C addr + write)
	 * 0x00 (start index)
	 * 0x10 (write length)
	 * 0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c
	 * 0x0d 0x0e 0x0f (data)
	 * crc8 result: 0x18
	 */
	rc = i2c_pec_burst_write_dt(&dt, 0, test_pattern, sizeof(test_pattern));
	zassert_ok(rc);
	zassert_true(i2c_pec_test_emul_is_idle(EMUL_DEV));
	zassert_equal(i2c_pec_test_emul_get_last_pec(EMUL_DEV), 0x18);

	/*
	 * PEC data:
	 * 0x16 (I2C addr + write)
	 * 0x00 (start index)
	 * 0x17 (I2C addr + read)
	 * 0x10 (read length)
	 * 0x00 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08 0x09 0x0a 0x0b 0x0c
	 * 0x0d 0x0e 0x0f (data)
	 * crc8 result: 0xff
	 */
	memset(buf, 0, sizeof(buf));
	rc = i2c_pec_burst_read_dt(&dt, 0, buf, sizeof(test_pattern));
	zassert_ok(rc);
	zassert_ok(memcmp(buf, test_pattern, sizeof(test_pattern)));
	zassert_true(i2c_pec_test_emul_is_idle(EMUL_DEV));
	zassert_equal(i2c_pec_test_emul_get_last_pec(EMUL_DEV), 0xff);
}

/* Validate corrupting PEC results in as error. */
ZTEST(i2c_pec, test_i2c_pec_corrupt)
{
	int rc;

	i2c_pec_test_emul_set_corrupt(EMUL_DEV, true);
	zassert_true(i2c_pec_test_emul_get_corrupt(EMUL_DEV));

	rc = i2c_pec_burst_write_dt(&dt, 0, test_pattern,
				    sizeof(test_pattern));
	zassert_equal(rc, -EIO);
	zassert_true(i2c_pec_test_emul_is_idle(EMUL_DEV));

	memset(buf, 0, sizeof(buf));
	rc = i2c_pec_burst_read_dt(&dt, 0, buf,
				   sizeof(test_pattern));
	zassert_equal(rc, -EAGAIN);
	zassert_true(i2c_pec_test_emul_is_idle(EMUL_DEV));
}

ZTEST_SUITE(i2c_pec, NULL, NULL, NULL, NULL, NULL);
