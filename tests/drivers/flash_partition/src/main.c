/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/flash.h>
/* For direct access to flash partition device object structure */
#include <../../../../drivers/flash/flash_partition_device_priv.h>
#include <device.h>

/* Offset between pages */
#define FLASH_PARTITION_OFFSET(label) DT_REG_ADDR(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define FLASH_PARTITION_SIZE(label) DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define FLASH_PARTITION_REAL_DEV_NODE(label) \
	DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODE_BY_FIXED_PARTITION_LABEL(label)))

#define FLASH_PARTITION_GET(label) DT_LABEL(DT_NODE_BY_FIXED_PARTITION_LABEL(label))

#define TEST_PARTITION image_1

#define REAL_DEV_PARTITION_OFFSET(label, offset) (FLASH_PARTITION_OFFSET(label) + offset)

#define MAX_POSSIBLE_PAGE_SIZE 8192

const struct device *flash_dev;
uint8_t __aligned(4) buffer[MAX_POSSIBLE_PAGE_SIZE];

#define HELLO "Hello world"
#define HELLO_PREV "Hello world PREV Hello world PREV"
#define HELLO_NEXT "Hello world NEXT Hello world NEXT"

#define SIZE_ALIGN(what, how) ((sizeof(what) / how) * how)

/* Get access to the device and erase it ready for testing*/
static void test_init(void)
{
	const struct flash_partition_device *fpd;
	const struct flash_parameters *fp;
	const struct flash_parameters *rfp;
	const struct device *real_dev = DEVICE_DT_GET(DT_MTD_FROM_FIXED_PARTITION(DT_NODE_BY_FIXED_PARTITION_LABEL(TEST_PARTITION)));

	zassert_true(real_dev != NULL, "Failed to get real device by DT");
	flash_dev = device_get_binding(FLASH_PARTITION_GET(TEST_PARTITION));

	zassert_true(flash_dev != NULL, "Failed to get partition_device ");
	fpd = flash_dev->config;
	zassert_equal(fpd->real_dev, real_dev, "Expected devices to match");

	rfp = flash_get_parameters(fpd->real_dev);
	fp = flash_get_parameters(flash_dev);

	zassert_true(rfp->max_page_size <= MAX_POSSIBLE_PAGE_SIZE,
		"Read/write buffer too small for the test");
	zassert_true(fp->max_page_size <= MAX_POSSIBLE_PAGE_SIZE,
		"Read/write buffer too small for the test");
}


static void test_partition(void)
{
	struct flash_page_info pi;
	const struct flash_parameters *fp;
	const struct flash_parameters *rfp;
	/* These are only accessible via priv header */
	const struct flash_partition_device *fpd;
	const struct flash_partition_device_priv *fpdp;
	int rc = 0;

	fpd = flash_dev->config;
	fpdp = flash_dev->data;

	zassert_equal(FLASH_PARTITION_OFFSET(TEST_PARTITION), fpd->offset, "Bad start offset");
	zassert_equal(FLASH_PARTITION_SIZE(TEST_PARTITION), fpd->size, "Bad start size");

	rfp = flash_get_parameters(fpd->real_dev);
	fp = flash_get_parameters(flash_dev);

	zassert_equal(rfp->erase_value, fp->erase_value,
		"Erase value is different for partition device from the real device");

	/* Erase page via real device access, then write via partition, check if written in proper
	 * position.
	 */
	zassert_true(flash_get_page_info(fpd->real_dev, FLASH_PARTITION_OFFSET(TEST_PARTITION), &pi) == 0,
		"Failed to get info on real device");
	zassert_true(flash_erase(fpd->real_dev, pi.offset, pi.size) == 0,
		"Failed to erase real dev");
	zassert_true(flash_read(flash_dev, 0, buffer, sizeof(buffer)) == 0,
		"Failed to read partition");
	for (size_t i = 0; i < pi.size; i++) {
		if (buffer[i] != fp->erase_value) {
			rc = 1;
			break;
		}
	};

	/* Write to partition */
	zassert_equal(rc, 0, "Erase failed or partition device reads wrong offset");
	zassert_equal(0, flash_write(flash_dev, 0, HELLO, SIZE_ALIGN(HELLO, 4)),
		"Failed to write test pattern to partition device");
	/* But read via real */
	zassert_equal(0,
		flash_read(fpd->real_dev,
			REAL_DEV_PARTITION_OFFSET(TEST_PARTITION, 0), buffer, sizeof(buffer)),
		"Failed to read test pattern from real device");
	zassert_equal(0, memcmp(HELLO, buffer, SIZE_ALIGN(HELLO, 4)), "Expected matched pattern");

	/* For test erase page that precedes the just written page, so precedes the partition,
	 * and follows it; then write beginnings of them with data and erase the middle page
	 * (the first page of partition).The preceding page and the following page should be
	 * left with data.
	 */


	/* Erase prev page and next page via real_dev */
	zassert_equal(0,
		flash_get_page_info(fpd->real_dev,
			REAL_DEV_PARTITION_OFFSET(TEST_PARTITION, -1),
		&pi),
		"Failed to get info on prev page via real device");
	zassert_equal(0,
		flash_erase(fpd->real_dev, pi.offset, pi.size),
		"Failed to erase prev page via real dev");
	zassert_equal(0,
		flash_write(fpd->real_dev, pi.offset, HELLO_PREV, SIZE_ALIGN(HELLO_PREV, 4)),
		"Failed to write to prev page via real dev");

	zassert_equal(0,
		flash_get_page_info(fpd->real_dev,
			REAL_DEV_PARTITION_OFFSET(TEST_PARTITION, 0), &pi),
		"Failed to get info on first page via real device");
	zassert_equal(0,
		flash_get_page_info(fpd->real_dev,
			REAL_DEV_PARTITION_OFFSET(TEST_PARTITION, pi.size), &pi),
		"Failed to get info on nest page via real device");
	zassert_equal(0,
		flash_erase(fpd->real_dev, pi.offset, pi.size),
		"Failed to erase next page via real dev");
	zassert_equal(0,
		flash_write(fpd->real_dev, pi.offset, HELLO_NEXT, SIZE_ALIGN(HELLO_NEXT, 4)),
		"Failed to write to next page via real dev");


	/* Erase first page via partition device */
	zassert_true(flash_get_page_info(flash_dev, 0, &pi) == 0,
		"Failed to get info on partition device");
	zassert_equal(0, flash_erase(flash_dev, 0, pi.size), "Failed to erase partition device");

	/* Check if erased on real dev */
	zassert_equal(0,
		flash_get_page_info(fpd->real_dev, FLASH_PARTITION_OFFSET(TEST_PARTITION),
			&pi),
		"Failed to get info on real device");
	zassert_equal(0,
		flash_read(fpd->real_dev, pi.offset, buffer, pi.size),
		"Failed to read partition");
	for (size_t i = 0; i < pi.size; i++) {
		if (buffer[i] != fp->erase_value) {
			rc = 1;
			break;
		}
	};

	zassert_equal(rc, 0, "Erase failed or partition device reads wrong offset");

	/* Check previous page and next page via real_dev */
	zassert_equal(0,
		flash_get_page_info(fpd->real_dev,
			REAL_DEV_PARTITION_OFFSET(TEST_PARTITION, -1), &pi),
		"Failed to get info on partition device");
	zassert_equal(0,
		flash_read(fpd->real_dev, pi.offset, buffer, SIZE_ALIGN(HELLO_PREV, 4)),
		"Failed to read real device");

	zassert_equal(0,
		memcmp(HELLO_PREV, buffer, SIZE_ALIGN(HELLO_PREV, 4)),
		"Previous page erased");

	/* Check next page using real device */
	zassert_equal(0,
		flash_get_page_info(fpd->real_dev,
			REAL_DEV_PARTITION_OFFSET(TEST_PARTITION, pi.size), &pi),
		"Failed to get info on partition device");
	zassert_equal(0,
		flash_read(fpd->real_dev, pi.offset, buffer, SIZE_ALIGN(HELLO_NEXT, 4)),
		"Failed to read real device");

	zassert_equal(0,
		memcmp(HELLO_NEXT, buffer, SIZE_ALIGN(HELLO_NEXT, 4)),
		"Next page erased");

	/* Check next page using partition device */
	zassert_equal(0,
		flash_get_page_info(flash_dev, 0, &pi),
		"Failed to get info from partition device");
	/* Second page */
	zassert_equal(0,
		flash_get_page_info(flash_dev, pi.size, &pi),
		"Failed to get info from partition device");
	zassert_equal(0,
		flash_read(flash_dev, pi.offset, buffer, SIZE_ALIGN(HELLO_NEXT, 4)),
		"Failed to read real device");

	zassert_equal(0,
		memcmp(HELLO_NEXT, buffer, SIZE_ALIGN(HELLO_NEXT, 4)),
		"Next page erased");

	/* Try write past partition device */
	zassert_equal(-EINVAL,
		flash_write(flash_dev, flash_get_size(flash_dev), HELLO, SIZE_ALIGN(HELLO, 4)),
		"Write past partition device should cause error");
}

void test_main(void)
{
	ztest_test_suite(flash_partition_device,
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_partition)
			);

	ztest_run_test_suite(flash_partition_device);
}
