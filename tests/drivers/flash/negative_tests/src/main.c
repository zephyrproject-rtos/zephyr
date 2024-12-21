/*
 * Copyright (c) 2020-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <zephyr/storage/flash_map.h>

#if defined(CONFIG_NORDIC_QSPI_NOR)
#define TEST_AREA_DEV_NODE DT_INST(0, nordic_qspi_nor)
#elif defined(CONFIG_SPI_NOR)
#define TEST_AREA_DEV_NODE DT_INST(0, jedec_spi_nor)
#else
#define TEST_AREA storage_partition
#endif

/* TEST_AREA is only defined for configurations that rely on
 * fixed-partition nodes.
 */
#if defined(TEST_AREA)
#define TEST_AREA_OFFSET FIXED_PARTITION_OFFSET(TEST_AREA)
#define TEST_AREA_SIZE   FIXED_PARTITION_SIZE(TEST_AREA)
#define TEST_AREA_DEVICE FIXED_PARTITION_DEVICE(TEST_AREA)

#if defined(CONFIG_SOC_NRF54L15)
#define TEST_FLASH_START (DT_REG_ADDR(DT_MEM_FROM_FIXED_PARTITION(DT_NODELABEL(TEST_AREA))))
#define TEST_FLASH_SIZE  (DT_REG_SIZE(DT_MEM_FROM_FIXED_PARTITION(DT_NODELABEL(TEST_AREA))))
#elif defined(CONFIG_SOC_NRF54H20)
#define TEST_FLASH_START (DT_REG_ADDR(DT_PARENT(DT_PARENT(DT_NODELABEL(TEST_AREA)))))
#define TEST_FLASH_SIZE  (DT_REG_SIZE(DT_PARENT(DT_PARENT(DT_NODELABEL(TEST_AREA)))))
#else
#error "Missing definition of TEST_FLASH_START and TEST_FLASH_SIZE for this target"
#endif

#else
#error "Unsupported configuraiton"
#endif

#define EXPECTED_SIZE 512

#if !defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && !defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#error There is no flash device enabled or it is missing Kconfig options
#endif

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static struct flash_pages_info page_info;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];

static void *flash_driver_setup(void)
{
	int rc;

	zassert_true(device_is_ready(flash_dev));

	TC_PRINT("Test will run on device %s\n", flash_dev->name);
	TC_PRINT("TEST_AREA_OFFSET = 0x%lx\n", (unsigned long)TEST_AREA_OFFSET);
	TC_PRINT("TEST_AREA_SIZE   = 0x%lx\n", (unsigned long)TEST_AREA_SIZE);
	TC_PRINT("TEST_FLASH_START = 0x%lx\n", (unsigned long)TEST_FLASH_START);
	TC_PRINT("TEST_FLASH_SIZE  = 0x%lx\n", (unsigned long)TEST_FLASH_SIZE);

	rc = flash_get_page_info_by_offs(flash_dev, TEST_AREA_OFFSET, &page_info);
	zassert_true(rc == 0, "flash_get_page_info_by_offs returned %d", rc);
	TC_PRINT("Test Page Info:\n");
	TC_PRINT("start_offset = 0x%lx\n", (unsigned long)page_info.start_offset);
	TC_PRINT("size         = 0x%lx\n", (unsigned long)page_info.size);
	TC_PRINT("index        = %d\n", page_info.index);
	TC_PRINT("===================================================================\n");

	return NULL;
}

ZTEST(flash_driver_negative, test_negative_flash_erase)
{
	int rc;

#if !defined(TEST_AREA)
	/* Flash memory boundaries are correctly calculated
	 * only for storage_partition.
	 */
	ztest_test_skip();
#endif

	/*  Acceptable values of erase size and offset are subject to
	 *  hardware-specific multiples of page size and offset.
	 */

	/* Check error returned when erasing memory at wrong address (too low) */
	rc = flash_erase(flash_dev, (TEST_FLASH_START - page_info.size), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_erase returned %d", rc);

	/* Check error returned when erasing memory at wrong address (too high) */
	rc = flash_erase(flash_dev, (TEST_FLASH_START + TEST_FLASH_SIZE), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_erase returned %d", rc);

	/* Check error returned when erasing unaligned memory or too large chunk of memory */
	rc = flash_erase(flash_dev, (TEST_AREA_OFFSET + 1), (TEST_FLASH_SIZE + 1));
	zassert_true(rc < 0, "Invalid use of flash_erase returned %d", rc);

	/* Erasing 0 bytes shall succeed */
	rc = flash_erase(flash_dev, TEST_AREA_OFFSET, 0);
	zassert_true(rc == 0, "flash_erase 0 bytes returned %d", rc);
}

ZTEST(flash_driver_negative, test_negative_flash_fill)
{
	int rc;
	uint8_t fill_val = 0xA; /* Dummy value */

#if !defined(TEST_AREA)
	/* Flash memory boundaries are correctly calculated
	 * only for storage_partition.
	 */
	ztest_test_skip();
#endif

	/*  Erase page offset and size are constrains of paged, explicit erase devices,
	 *  but can be relaxed with devices without such requirement.
	 */

	/* Check error returned when filling memory at wrong address (too low) */
	rc = flash_fill(flash_dev, fill_val, (TEST_FLASH_START - page_info.size), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_fill returned %d", rc);

	/* Check error returned when filling memory at wrong address (too high) */
	rc = flash_fill(flash_dev, fill_val, (TEST_FLASH_START + TEST_FLASH_SIZE), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_fill returned %d", rc);

	/* Check error returned when filling unaligned memory */
	rc = flash_fill(flash_dev, fill_val, (TEST_AREA_OFFSET + 1), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_fill returned %d", rc);
	rc = flash_fill(flash_dev, fill_val, TEST_AREA_OFFSET, (page_info.size + 1));
	zassert_true(rc < 0, "Invalid use of flash_fill returned %d", rc);

	/* Filling 0 bytes shall succeed */
	rc = flash_fill(flash_dev, fill_val, TEST_AREA_OFFSET, 0);
	zassert_true(rc == 0, "flash_fill 0 bytes returned %d", rc);
}

ZTEST(flash_driver_negative, test_negative_flash_flatten)
{
	int rc;

#if !defined(TEST_AREA)
	/* Flash memory boundaries are correctly calculated
	 * only for storage_partition.
	 */
	ztest_test_skip();
#endif

	/*  Erase page offset and size are constrains of paged, explicit erase devices,
	 *  but can be relaxed with devices without such requirement.
	 */

	/* Check error returned when flatten memory at wrong address (too low) */
	rc = flash_flatten(flash_dev, (TEST_FLASH_START - page_info.size), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_flatten returned %d", rc);

	/* Check error returned when flatten memory at wrong address (too high) */
	rc = flash_flatten(flash_dev, (TEST_FLASH_START + TEST_FLASH_SIZE), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_flatten returned %d", rc);

	/* Check error returned when flatten unaligned memory */
	rc = flash_flatten(flash_dev, (TEST_AREA_OFFSET + 1), page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_flatten returned %d", rc);
	rc = flash_flatten(flash_dev, TEST_AREA_OFFSET, (page_info.size + 1));
	zassert_true(rc < 0, "Invalid use of flash_flatten returned %d", rc);

	/* Flatten 0 bytes shall succeed */
	rc = flash_flatten(flash_dev, TEST_AREA_OFFSET, 0);
	zassert_true(rc == 0, "flash_flatten 0 bytes returned %d", rc);
}

ZTEST(flash_driver_negative, test_negative_flash_read)
{
	int rc;
	uint8_t read_buf[EXPECTED_SIZE];

#if !defined(TEST_AREA)
	/* Flash memory boundaries are correctly calculated
	 * only for storage_partition.
	 */
	ztest_test_skip();
#endif

	/*  All flash drivers support reads without alignment restrictions on
	 *  the read offset, the read size, or the destination address.
	 */

	/* Check error returned when reading from a wrong address (too low) */
	rc = flash_read(flash_dev, (TEST_FLASH_START - page_info.size), read_buf, EXPECTED_SIZE);
	zassert_true(rc < 0, "Invalid use of flash_read returned %d", rc);

	/* Check error returned when reading from a wrong address (too high) */
	rc = flash_read(flash_dev, (TEST_FLASH_START + TEST_FLASH_SIZE), read_buf, EXPECTED_SIZE);
	zassert_true(rc < 0, "Invalid use of flash_read returned %d", rc);

	/* Check error returned when reading too many data */
	rc = flash_read(flash_dev, TEST_AREA_OFFSET, read_buf, (TEST_FLASH_SIZE + page_info.size));
	zassert_true(rc < 0, "Invalid use of flash_read returned %d", rc);

	/* Reading 0 bytes shall succeed */
	rc = flash_read(flash_dev, TEST_AREA_OFFSET, read_buf, 0);
	zassert_true(rc == 0, "flash_read 0 bytes returned %d", rc);
}

ZTEST(flash_driver_negative, test_negative_flash_write)
{
	int rc;

#if !defined(TEST_AREA)
	/* Flash memory boundaries are correctly calculated
	 * only for storage_partition.
	 */
	ztest_test_skip();
#endif

	/*  Write size and offset must be multiples of the minimum write block size
	 *  supported by the driver.
	 */

	/* Check error returned when writing to a wrong address (too low) */
	rc = flash_write(flash_dev, (TEST_FLASH_START - page_info.size), expected, page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_write returned %d", rc);

	/* Check error returned when writing to a wrong address (too high) */
	rc = flash_write(flash_dev, (TEST_FLASH_START + TEST_FLASH_SIZE), expected, page_info.size);
	zassert_true(rc < 0, "Invalid use of flash_write returned %d", rc);

	/* Check error returned when writing at unaligned memory or too large chunk of memory */
	rc = flash_write(flash_dev, (TEST_AREA_OFFSET + 1), expected, (TEST_FLASH_SIZE + 1));
	zassert_true(rc < 0, "Invalid use of flash_write returned %d", rc);

	/* Writing 0 bytes shall succeed */
	rc = flash_write(flash_dev, TEST_AREA_OFFSET, expected, 0);
	zassert_true(rc == 0, "flash_write 0 bytes returned %d", rc);
}

ZTEST_SUITE(flash_driver_negative, NULL, flash_driver_setup, NULL, NULL, NULL);
