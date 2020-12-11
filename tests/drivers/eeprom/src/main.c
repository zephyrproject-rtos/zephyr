/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <ztest_test.h>
#include <drivers/eeprom.h>
#include <device.h>

/* There is no obvious way to pass this to tests, so use a global */
ZTEST_BMEM static const char *eeprom_label;

static void test_size(void)
{
	const struct device *eeprom;
	size_t size;

	eeprom = device_get_binding(eeprom_label);

	size = eeprom_get_size(eeprom);
	zassert_not_equal(0, size, "Unexpected size of zero bytes");
}

static void test_out_of_bounds(void)
{
	const uint8_t data[4] = { 0x01, 0x02, 0x03, 0x03 };
	const struct device *eeprom;
	size_t size;
	int rc;

	eeprom = device_get_binding(eeprom_label);
	size = eeprom_get_size(eeprom);

	rc = eeprom_write(eeprom, size - 1, data, sizeof(data));
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

static void test_write_and_verify(void)
{
	const uint8_t wr_buf1[4] = { 0xFF, 0xEE, 0xDD, 0xCC };
	const uint8_t wr_buf2[sizeof(wr_buf1)] = { 0xAA, 0xBB, 0xCC, 0xDD };
	uint8_t rd_buf[sizeof(wr_buf1)];
	const struct device *eeprom;
	int rc;

	eeprom = device_get_binding(eeprom_label);

	rc = eeprom_write(eeprom, 0, wr_buf1, sizeof(wr_buf1));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = eeprom_read(eeprom, 0, rd_buf, sizeof(rd_buf));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = memcmp(wr_buf1, rd_buf, sizeof(wr_buf1));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = eeprom_write(eeprom, 0, wr_buf2, sizeof(wr_buf2));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = eeprom_read(eeprom, 0, rd_buf, sizeof(rd_buf));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	rc = memcmp(wr_buf2, rd_buf, sizeof(wr_buf2));
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
}

static void test_zero_length_write(void)
{
	const uint8_t wr_buf1[4] = { 0x10, 0x20, 0x30, 0x40 };
	const uint8_t wr_buf2[sizeof(wr_buf1)] = { 0xAA, 0xBB, 0xCC, 0xDD };
	uint8_t rd_buf[sizeof(wr_buf1)];
	const struct device *eeprom;
	int rc;

	eeprom = device_get_binding(eeprom_label);

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

/* Run all of our tests on EEPROM device with the given label */
static void run_tests_on_eeprom(const char *label)
{
	const struct device *eeprom = device_get_binding(label);

	zassert_not_null(eeprom, "Unable to get EEPROM device");
	k_object_access_grant(eeprom, k_current_get());
	eeprom_label = label;
	ztest_test_suite(eeprom_api,
			 ztest_user_unit_test(test_size),
			 ztest_user_unit_test(test_out_of_bounds),
			 ztest_user_unit_test(test_write_and_verify),
			 ztest_user_unit_test(test_zero_length_write));
	ztest_run_test_suite(eeprom_api);
}

void test_main(void)
{
	run_tests_on_eeprom(DT_LABEL(DT_ALIAS(eeprom_0)));

#ifdef DT_N_ALIAS_eeprom_1
	run_tests_on_eeprom(DT_LABEL(DT_ALIAS(eeprom_1)));
#endif

}
