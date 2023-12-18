/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/ztest.h>

/* There is no obvious way to pass this to tests, so use a global */
ZTEST_BMEM static const struct device *eeprom;

/* Test retrieval of eeprom size */
ZTEST_USER(eeprom, test_size)
{
	size_t size;

	size = eeprom_get_size(eeprom);
	zassert_not_equal(0, size, "Unexpected size of zero bytes");
}

/* Test write outside eeprom area */
ZTEST_USER(eeprom, test_out_of_bounds)
{
	const uint8_t data[4] = { 0x01, 0x02, 0x03, 0x03 };
	size_t size;
	int rc;

	size = eeprom_get_size(eeprom);

	rc = eeprom_write(eeprom, size - 1, data, sizeof(data));
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

/* Test write and rewrite */
ZTEST_USER(eeprom, test_write_rewrite)
{
	const uint8_t wr_buf1[4] = { 0xFF, 0xEE, 0xDD, 0xCC };
	const uint8_t wr_buf2[sizeof(wr_buf1)] = { 0xAA, 0xBB, 0xCC, 0xDD };
	uint8_t rd_buf[sizeof(wr_buf1)];
	size_t size;
	off_t address;
	int rc;

	size = eeprom_get_size(eeprom);

	address = 0;
	while (address < MIN(size, 16)) {
		rc = eeprom_write(eeprom, address, wr_buf1, sizeof(wr_buf1));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = eeprom_read(eeprom, address, rd_buf, sizeof(rd_buf));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = memcmp(wr_buf1, rd_buf, sizeof(wr_buf1));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		address += sizeof(wr_buf1);
	}

	address = 0;
	while (address < MIN(size, 16)) {
		rc = eeprom_write(eeprom, address, wr_buf2, sizeof(wr_buf2));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = eeprom_read(eeprom, address, rd_buf, sizeof(rd_buf));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = memcmp(wr_buf2, rd_buf, sizeof(wr_buf2));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		address += sizeof(wr_buf2);
	}
}

/* Test write at fixed address */
ZTEST_USER(eeprom, test_write_at_fixed_address)
{
	const uint8_t wr_buf1[4] = { 0xFF, 0xEE, 0xDD, 0xCC };
	uint8_t rd_buf[sizeof(wr_buf1)];
	size_t size;
	const off_t address = 0;
	int rc;

	size = eeprom_get_size(eeprom);

	for (int i = 0; i < 16; i++) {
		rc = eeprom_write(eeprom, address, wr_buf1, sizeof(wr_buf1));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = eeprom_read(eeprom, address, rd_buf, sizeof(rd_buf));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = memcmp(wr_buf1, rd_buf, sizeof(wr_buf1));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	}
}

/* Test write one byte at a time */
ZTEST_USER(eeprom, test_write_byte)
{
	const uint8_t wr = 0x00;
	uint8_t rd = 0xff;
	int rc;

	for (off_t address = 0; address < 16; address++) {
		rc = eeprom_write(eeprom, address, &wr, 1);
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = eeprom_read(eeprom, address, &rd, 1);
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		zassert_equal(wr - rd, rc, "Unexpected error code (%d)", rc);
	}
}

/* Test write a pattern of bytes at increasing address */
ZTEST_USER(eeprom, test_write_at_increasing_address)
{
	const uint8_t wr_buf1[8] = {0xEE, 0xDD, 0xCC, 0xBB, 0xFF, 0xEE, 0xDD,
				    0xCC };
	uint8_t rd_buf[sizeof(wr_buf1)];
	int rc;

	for (off_t address = 0; address < 4; address++) {
		rc = eeprom_write(eeprom, address, wr_buf1, sizeof(wr_buf1));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = eeprom_read(eeprom, address, rd_buf, sizeof(rd_buf));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);

		rc = memcmp(wr_buf1, rd_buf, sizeof(wr_buf1));
		zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	}
}

/* Test writing zero length data */
ZTEST_USER(eeprom, test_zero_length_write)
{
	const uint8_t wr_buf1[4] = { 0x10, 0x20, 0x30, 0x40 };
	const uint8_t wr_buf2[sizeof(wr_buf1)] = { 0xAA, 0xBB, 0xCC, 0xDD };
	uint8_t rd_buf[sizeof(wr_buf1)];
	int rc;

	rc = eeprom_write(eeprom, 0, wr_buf1, sizeof(wr_buf1));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = eeprom_read(eeprom, 0, rd_buf, sizeof(rd_buf));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = memcmp(wr_buf1, rd_buf, sizeof(wr_buf1));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = eeprom_write(eeprom, 0, wr_buf2, 0);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = eeprom_read(eeprom, 0, rd_buf, sizeof(rd_buf));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = memcmp(wr_buf1, rd_buf, sizeof(wr_buf1));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
}

static void *eeprom_setup(void)
{
	zassert_true(device_is_ready(eeprom), "EEPROM device not ready");

	return NULL;
}

ZTEST_SUITE(eeprom, NULL, eeprom_setup, NULL, NULL, NULL);

/* Run all of our tests on the given EEPROM device */
static void run_tests_on_eeprom(const struct device *dev)
{
	eeprom = dev;

	printk("Running tests on device \"%s\"\n", eeprom->name);
	k_object_access_grant(eeprom, k_current_get());
	ztest_run_all(NULL, false, 1, 1);
}

void test_main(void)
{
	run_tests_on_eeprom(DEVICE_DT_GET(DT_ALIAS(eeprom_0)));

#if DT_NODE_HAS_STATUS(DT_ALIAS(eeprom_1), okay)
	run_tests_on_eeprom(DEVICE_DT_GET(DT_ALIAS(eeprom_1)));
#endif /* DT_NODE_HAS_STATUS(DT_ALIAS(eeprom_1), okay) */

	ztest_verify_all_test_suites_ran();
}
