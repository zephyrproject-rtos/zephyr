/*
 * Copyright (c) 2023 Kickmaker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/eeprom/eeprom_st25dv.h>
#include <zephyr/ztest.h>

/* There is no obvious way to pass this to tests, so use a global */
ZTEST_BMEM static const struct device *eeprom;

static void create_user_area_and_check_end_zone(uint16_t zone1_length, uint16_t zone2_length,
						uint16_t zone3_length, uint16_t zone4_length)
{
	int rc;
	uint8_t end_zone_addr = 0;
	uint32_t last_byte_area;
	uint32_t expected_last_byte_area;

	rc = eeprom_st25dv_create_user_zone(eeprom, zone1_length, zone2_length, zone3_length,
					    zone4_length);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);

	/* For each area end, compute last byte and check value doing a read of end zone */
	rc = eeprom_st25dv_read_end_zone(eeprom, EEPROM_ST25DV_END_ZONE1, &end_zone_addr);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	last_byte_area = EEPROM_ST25DV_CONVERT_END_ZONE_AREA_IN_BYTES(end_zone_addr);
	expected_last_byte_area = zone1_length - 1;
	zassert_equal(expected_last_byte_area, last_byte_area,
		      "Zone 1 area did not end as expected.");

	rc = eeprom_st25dv_read_end_zone(eeprom, EEPROM_ST25DV_END_ZONE2, &end_zone_addr);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	last_byte_area = EEPROM_ST25DV_CONVERT_END_ZONE_AREA_IN_BYTES(end_zone_addr);
	expected_last_byte_area += zone2_length;
	zassert_equal(expected_last_byte_area, last_byte_area,
		      "Zone 2 area did not end as expected.");

	rc = eeprom_st25dv_read_end_zone(eeprom, EEPROM_ST25DV_END_ZONE3, &end_zone_addr);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	last_byte_area = EEPROM_ST25DV_CONVERT_END_ZONE_AREA_IN_BYTES(end_zone_addr);
	expected_last_byte_area += zone3_length;
	zassert_equal(expected_last_byte_area, last_byte_area,
		      "Zone 3 area did not end as expected.");
}

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
	const uint8_t data[4] = {0x01, 0x02, 0x03, 0x03};
	size_t size;
	int rc;

	size = eeprom_get_size(eeprom);

	rc = eeprom_write(eeprom, size - 1, data, sizeof(data));
	zassert_equal(-EINVAL, rc, "Unexpected error code (%d)", rc);
}

/* Test write and rewrite */
ZTEST_USER(eeprom, test_write_rewrite)
{
	const uint8_t wr_buf1[4] = {0xFF, 0xEE, 0xDD, 0xCC};
	const uint8_t wr_buf2[sizeof(wr_buf1)] = {0xAA, 0xBB, 0xCC, 0xDD};
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
	const uint8_t wr_buf1[4] = {0xFF, 0xEE, 0xDD, 0xCC};
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
	const uint8_t wr_buf1[8] = {0xEE, 0xDD, 0xCC, 0xBB, 0xFF, 0xEE, 0xDD, 0xCC};
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
	const uint8_t wr_buf1[4] = {0x10, 0x20, 0x30, 0x40};
	const uint8_t wr_buf2[sizeof(wr_buf1)] = {0xAA, 0xBB, 0xCC, 0xDD};
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

/* Test read UUID and check manufacturer is STMicroelectronics */
ZTEST_USER(eeprom, test_read_uuid_check_manufacturer_stmicro)
{
	int rc;
	uint8_t uuid[8] = {0};

	rc = eeprom_st25dv_read_uuid(eeprom, uuid);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	zassert_true((uuid[6] == 0x02) && (uuid[7] == 0xe0),
		     "Manufacturer was not STMicroelectronics");
}

/* Test read IC rev */
ZTEST_USER(eeprom, test_read_ic_rev)
{
	int rc;
	uint8_t ic_rev = 0;

	rc = eeprom_st25dv_read_ic_rev(eeprom, &ic_rev);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
}

/* Test read IC ref and check value */
ZTEST_USER(eeprom, test_read_ic_ref)
{
	int rc;
	uint8_t ic_ref = 0;

	rc = eeprom_st25dv_read_ic_ref(eeprom, &ic_ref);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
	/* Some references use the same ID. Only two different values are possible */
	zassert_true((ic_ref == EEPROM_ST25DV_IC_REF_ST25DV04K_IE) ||
			     (ic_ref == EEPROM_ST25DV_IC_REF_ST25DV16K_IE),
		     "Unknown IC reference");
}

/* Test read memory size */
ZTEST_USER(eeprom, test_read_mem_size)
{
	int rc;
	eeprom_st25dv_mem_size_t memsize = {0};

	rc = eeprom_st25dv_read_mem_size(eeprom, &memsize);
	zassert_equal(0, rc, "Unexpected error code (%d)", rc);
}

/* Test create user areas, 128Bytes each */
ZTEST_USER(eeprom, test_create_user_area_128B_each)
{
	uint16_t zone1_length = 128;
	uint16_t zone2_length = 128;
	uint16_t zone3_length = 128;
	uint16_t zone4_length = 128;

	create_user_area_and_check_end_zone(zone1_length, zone2_length, zone3_length, zone4_length);
}

/* Test create user areas, zone2 at 64 Bytes, zone 3 at 192 Bytes */
ZTEST_USER(eeprom, test_create_user_area_zone2_64B_zone3_192B)
{
	uint16_t zone1_length = 128;
	uint16_t zone2_length = 64;
	uint16_t zone3_length = 192;
	uint16_t zone4_length = 128;

	create_user_area_and_check_end_zone(zone1_length, zone2_length, zone3_length, zone4_length);
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
	ztest_run_all(NULL);
}

void test_main(void)
{
	run_tests_on_eeprom(DEVICE_DT_GET(DT_ALIAS(eeprom_0)));

#if DT_NODE_HAS_STATUS(DT_ALIAS(eeprom_1), okay)
	run_tests_on_eeprom(DEVICE_DT_GET(DT_ALIAS(eeprom_1)));
#endif /* DT_NODE_HAS_STATUS(DT_ALIAS(eeprom_1), okay) */

	ztest_verify_all_test_suites_ran();
}
