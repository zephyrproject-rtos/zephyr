/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/ztest.h>

/* List of known EEPROM devicetree aliases */
#define EEPROM_ALIASES eeprom_0, eeprom_1

/* Test retrieval of eeprom size */
static void test_size(const struct device *eeprom)
{
	size_t size;

	size = eeprom_get_size(eeprom);
	zassert_not_equal(0, size, "Unexpected size of zero bytes");
}

/* Test write outside eeprom area */
static void test_out_of_bounds(const struct device *eeprom)
{
	const uint8_t data[4] = { 0x01, 0x02, 0x03, 0x03 };
	size_t size;
	int rc;

	size = eeprom_get_size(eeprom);

	rc = eeprom_write(eeprom, size - 1, data, sizeof(data));
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

/* Test write and rewrite */
static void test_write_rewrite(const struct device *eeprom)
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
static void test_write_at_fixed_address(const struct device *eeprom)
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
static void test_write_byte(const struct device *eeprom)
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
static void test_write_at_increasing_address(const struct device *eeprom)
{
	const uint8_t wr_buf1[8] = { 0xEE, 0xDD, 0xCC, 0xBB, 0xFF, 0xEE, 0xDD, 0xCC };
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
static void test_zero_length_write(const struct device *eeprom)
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

/* Get the device for an EEPROM devicetree alias */
#define EEPROM_ALIAS_DEVICE_GET(alias) DEVICE_DT_GET(DT_ALIAS(alias))

/* Expand macro F for each EEPROM known devicetree alias */
#define FOR_EACH_EEPROM_ALIAS(F) FOR_EACH(F, (), EEPROM_ALIASES)

/* Generate a test case function for an EEPROM devicetree alias */
#define TEST_EEPROM_FUNCTION(alias, name)                                                          \
	ZTEST_USER(eeprom, test_##alias##_##name) { test_##name(EEPROM_ALIAS_DEVICE_GET(alias)); }

/* Generate test cases for an EEPROM devicetree alias */
#define TEST_EEPROM(alias)                                                                         \
	TEST_EEPROM_FUNCTION(alias, size)                                                          \
	TEST_EEPROM_FUNCTION(alias, out_of_bounds)                                                 \
	TEST_EEPROM_FUNCTION(alias, write_rewrite)                                                 \
	TEST_EEPROM_FUNCTION(alias, write_at_fixed_address)                                        \
	TEST_EEPROM_FUNCTION(alias, write_byte)                                                    \
	TEST_EEPROM_FUNCTION(alias, write_at_increasing_address)                                   \
	TEST_EEPROM_FUNCTION(alias, zero_length_write)

/* Conditionally generate test cases for an EEPROM devicetree alias */
#define TEST_EEPROM_COND(alias)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_ALIAS(alias), okay), (TEST_EEPROM(alias)), ())

/* Try generating test cases for all known EEPROM devicetree aliases */
FOR_EACH_EEPROM_ALIAS(TEST_EEPROM_COND)

/* Initialize an EEPROM device obtained from an EEPROM devicetree alias */
#define INIT_EEPROM(alias)                                                                         \
	zassume_true(device_is_ready(EEPROM_ALIAS_DEVICE_GET(alias)), "device not ready");         \
	k_object_access_grant(EEPROM_ALIAS_DEVICE_GET(alias), k_current_get());

/* Conditionally initialize an EEPROM device obtained from an EEPROM devicetree alias */
#define INIT_EEPROM_COND(alias)                                                                    \
	COND_CODE_1(DT_NODE_HAS_STATUS(DT_ALIAS(alias), okay), (INIT_EEPROM(alias)), ())

static void *eeprom_setup(void)
{
	FOR_EACH_EEPROM_ALIAS(INIT_EEPROM_COND);

	return NULL;
}

ZTEST_SUITE(eeprom, NULL, eeprom_setup, NULL, NULL, NULL);
