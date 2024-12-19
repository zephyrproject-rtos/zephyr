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
#define TEST_AREA_DEV_NODE	DT_INST(0, nordic_qspi_nor)
#elif defined(CONFIG_SPI_NOR)
#define TEST_AREA_DEV_NODE	DT_INST(0, jedec_spi_nor)
#elif defined(CONFIG_FLASH_MSPI_NOR)
#define TEST_AREA_DEV_NODE	DT_INST(0, jedec_mspi_nor)
#else
#define TEST_AREA	storage_partition
#endif

/* TEST_AREA is only defined for configurations that realy on
 * fixed-partition nodes.
 */
#ifdef TEST_AREA
#define TEST_AREA_OFFSET	FIXED_PARTITION_OFFSET(TEST_AREA)
#define TEST_AREA_SIZE		FIXED_PARTITION_SIZE(TEST_AREA)
#define TEST_AREA_MAX		(TEST_AREA_OFFSET + TEST_AREA_SIZE)
#define TEST_AREA_DEVICE	FIXED_PARTITION_DEVICE(TEST_AREA)

#elif defined(TEST_AREA_DEV_NODE)

#define TEST_AREA_DEVICE	DEVICE_DT_GET(TEST_AREA_DEV_NODE)
#define TEST_AREA_OFFSET	0xff000

#if DT_NODE_HAS_PROP(TEST_AREA_DEV_NODE, size_in_bytes)
#define TEST_AREA_MAX DT_PROP(TEST_AREA_DEV_NODE, size_in_bytes)
#else
#define TEST_AREA_MAX (DT_PROP(TEST_AREA_DEV_NODE, size) / 8)
#endif

#else
#error "Unsupported configuraiton"
#endif

#define EXPECTED_SIZE	512

#if !defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE) &&		\
	!defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
#error There is no flash device enabled or it is missing Kconfig options
#endif

static const struct device *const flash_dev = TEST_AREA_DEVICE;
static struct flash_pages_info page_info;
static uint8_t __aligned(4) expected[EXPECTED_SIZE];
static uint8_t erase_value;
static bool ebw_required;

static void flash_driver_before(void *arg)
{
	int rc;

	ARG_UNUSED(arg);

	TC_PRINT("Test will run on device %s\n", flash_dev->name);
	zassert_true(device_is_ready(flash_dev));

	/* Check for erase is only needed when there is mix of devices */
	if (IS_ENABLED(CONFIG_FLASH_HAS_EXPLICIT_ERASE)) {
		const struct flash_parameters *fparams = flash_get_parameters(flash_dev);

		erase_value = fparams->erase_value;
		ebw_required = flash_params_get_erase_cap(fparams) & FLASH_ERASE_C_EXPLICIT;
		/* For tests purposes use page (in nrf_qspi_nor page = 64 kB) */
		flash_get_page_info_by_offs(flash_dev, TEST_AREA_OFFSET,
					    &page_info);
	} else {
		TC_PRINT("No devices with erase requirement present\n");
		erase_value = 0x55;
		page_info.start_offset = TEST_AREA_OFFSET;
		page_info.size = TEST_AREA_MAX - TEST_AREA_OFFSET;
	}


	/* Check if test region is not empty */
	uint8_t buf[EXPECTED_SIZE];

	rc = flash_read(flash_dev, TEST_AREA_OFFSET,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	/* Fill test buffer with random data */
	for (int i = 0, val = 0; i < EXPECTED_SIZE; i++, val++) {
		/* Skip erase value */
		if (val == erase_value) {
			val++;
		}
		expected[i] = val;
	}

	/* Check if tested region fits in flash */
	zassert_true((TEST_AREA_OFFSET + EXPECTED_SIZE) <= TEST_AREA_MAX,
		     "Test area exceeds flash size");

	/* Check if flash is cleared */
	if (IS_ENABLED(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && ebw_required) {
		bool is_buf_clear = true;

		for (off_t i = 0; i < EXPECTED_SIZE; i++) {
			if (buf[i] != erase_value) {
				is_buf_clear = false;
				break;
			}
		}

		if (!is_buf_clear) {
			/* Erase a nb of pages aligned to the EXPECTED_SIZE */
			rc = flash_erase(flash_dev, page_info.start_offset,
					(page_info.size *
					((EXPECTED_SIZE + page_info.size - 1)
					/ page_info.size)));

			zassert_equal(rc, 0, "Flash memory not properly erased");
		}
	}
}

ZTEST(flash_driver, test_read_unaligned_address)
{
	int rc;
	uint8_t buf[EXPECTED_SIZE];
	const uint8_t canary = erase_value;
	uint32_t start;

	if (IS_ENABLED(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && ebw_required) {
		start = page_info.start_offset;
		/* Erase a nb of pages aligned to the EXPECTED_SIZE */
		rc = flash_erase(flash_dev, page_info.start_offset,
				(page_info.size *
				((EXPECTED_SIZE + page_info.size - 1)
				/ page_info.size)));

		zassert_equal(rc, 0, "Flash memory not properly erased");
	} else {
		start = TEST_AREA_OFFSET;
	}

	rc = flash_write(flash_dev,
			 start,
			 expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot write to flash");

	/* read buffer length*/
	for (off_t len = 0; len < 25; len++) {
		/* address offset */
		for (off_t ad_o = 0; ad_o < 4; ad_o++) {
			/* buffer offset; leave space for buffer guard */
			for (off_t buf_o = 1; buf_o < 5; buf_o++) {
				/* buffer overflow protection */
				buf[buf_o - 1] = canary;
				buf[buf_o + len] = canary;
				memset(buf + buf_o, 0, len);
				rc = flash_read(flash_dev,
						start + ad_o,
						buf + buf_o, len);
				zassert_equal(rc, 0, "Cannot read flash");
				zassert_equal(memcmp(buf + buf_o,
						expected + ad_o,
						len),
					0, "Flash read failed at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
				/* check buffer guards */
				zassert_equal(buf[buf_o - 1], canary,
					"Buffer underflow at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
				zassert_equal(buf[buf_o + len], canary,
					"Buffer overflow at len=%d, "
					"ad_o=%d, buf_o=%d", len, ad_o, buf_o);
			}
		}
	}
}

ZTEST(flash_driver, test_flash_fill)
{
	uint8_t buf[EXPECTED_SIZE];
	int rc;
	off_t i;

	if (IS_ENABLED(CONFIG_FLASH_HAS_EXPLICIT_ERASE) && ebw_required) {
		/* Erase a nb of pages aligned to the EXPECTED_SIZE */
		rc = flash_erase(flash_dev, page_info.start_offset,
				(page_info.size *
				((EXPECTED_SIZE + page_info.size - 1)
				/ page_info.size)));

		zassert_equal(rc, 0, "Flash memory not properly erased");
	} else {
		rc = flash_fill(flash_dev, 0x55, page_info.start_offset,
				(page_info.size *
				((EXPECTED_SIZE + page_info.size - 1)
				/ page_info.size)));
		zassert_equal(rc, 0, "Leveling memory with fill failed\n");
	}

	/* Fill the device with 0xaa */
	rc = flash_fill(flash_dev, 0xaa, page_info.start_offset,
			(page_info.size *
			((EXPECTED_SIZE + page_info.size - 1)
			/ page_info.size)));
	zassert_equal(rc, 0, "Fill failed\n");

	rc = flash_read(flash_dev, TEST_AREA_OFFSET,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	for (i = 0; i < EXPECTED_SIZE; i++) {
		if (buf[i] != 0xaa) {
			break;
		}
	}
	zassert_equal(i, EXPECTED_SIZE, "Expected device to be filled wth 0xaa");
}

ZTEST(flash_driver, test_flash_flatten)
{
	uint8_t buf[EXPECTED_SIZE];
	int rc;
	off_t i;

	rc = flash_flatten(flash_dev, page_info.start_offset,
			   (page_info.size *
			   ((EXPECTED_SIZE + page_info.size - 1)
			   / page_info.size)));

	zassert_equal(rc, 0, "Flash not leveled not properly erased");

	/* Fill the device with 0xaa */
	rc = flash_fill(flash_dev, 0xaa, page_info.start_offset,
			(page_info.size *
			((EXPECTED_SIZE + page_info.size - 1)
			/ page_info.size)));
	zassert_equal(rc, 0, "Fill failed\n");

	rc = flash_read(flash_dev, TEST_AREA_OFFSET,
			buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	for (i = 0; i < EXPECTED_SIZE; i++) {
		if (buf[i] != 0xaa) {
			break;
		}
	}
	zassert_equal(i, EXPECTED_SIZE, "Expected device to be filled wth 0xaa");
}

ZTEST(flash_driver, test_flash_erase)
{
	int rc;
	uint8_t read_buf[EXPECTED_SIZE];
	bool comparison_result;
	const struct flash_parameters *fparams = flash_get_parameters(flash_dev);

	erase_value = fparams->erase_value;

	/* Write test data */
	rc = flash_write(flash_dev, page_info.start_offset, expected, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot write to flash");

	/* Confirm write operation */
	rc = flash_read(flash_dev, page_info.start_offset, read_buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	comparison_result = true;
	for (int i = 0; i < EXPECTED_SIZE; i++) {
		if (read_buf[i] != expected[i]) {
			comparison_result = false;
			TC_PRINT("i=%d:\tread_buf[i]=%d\texpected[i]=%d\n", i, read_buf[i],
				 expected[i]);
		}
	}
	zassert_true(comparison_result, "Write operation failed");
	/* Cross check - confirm that expected data is pseudo-random */
	zassert_not_equal(read_buf[0], expected[1], "These values shall be different");

	/* Erase a nb of pages aligned to the EXPECTED_SIZE */
	rc = flash_erase(
		flash_dev, page_info.start_offset,
		(page_info.size * ((EXPECTED_SIZE + page_info.size - 1) / page_info.size)));
	zassert_equal(rc, 0, "Flash memory not properly erased");

	/* Confirm erase operation */
	rc = flash_read(flash_dev, page_info.start_offset, read_buf, EXPECTED_SIZE);
	zassert_equal(rc, 0, "Cannot read flash");

	comparison_result = true;
	for (int i = 0; i < EXPECTED_SIZE; i++) {
		if (read_buf[i] != erase_value) {
			comparison_result = false;
			TC_PRINT("i=%d:\tread_buf[i]=%d\texpected=%d\n", i, read_buf[i],
				 erase_value);
		}
	}
	zassert_true(comparison_result, "Write operation failed");
	/* Cross check - confirm that expected data
	 * doesn't contain erase_value
	 */
	zassert_not_equal(expected[0], erase_value, "These values shall be different");
}

struct test_cb_data_type {
	uint32_t page_counter; /* used to count how many pages was iterated */
	uint32_t exit_page;    /* terminate iteration when this page is reached */
};

static bool flash_callback(const struct flash_pages_info *info, void *data)
{
	struct test_cb_data_type *cb_data = (struct test_cb_data_type *)data;

	cb_data->page_counter++;

	if (cb_data->page_counter >= cb_data->exit_page) {
		return false;
	}

	return true;
}

ZTEST(flash_driver, test_get_size)
{
#if CONFIG_TEST_DRIVER_FLASH_SIZE != -1
	uint64_t size;

	zassert_ok(flash_get_size(flash_dev, &size));
	zassert_equal(size, (uint64_t)CONFIG_TEST_DRIVER_FLASH_SIZE, "Expected %llu, got %llu\n",
		      (uint64_t)CONFIG_TEST_DRIVER_FLASH_SIZE, size);
#else
	/* The test is sipped only because there is no uniform way to get device size */
	ztest_test_skip();
#endif
}

ZTEST(flash_driver, test_flash_page_layout)
{
	int rc;
	struct flash_pages_info page_info_off = {0};
	struct flash_pages_info page_info_idx = {0};
	size_t page_count;
	struct test_cb_data_type test_cb_data = {0};

#if !defined(CONFIG_FLASH_PAGE_LAYOUT)
	ztest_test_skip();
#endif

	/* Get page info with flash_get_page_info_by_offs() */
	rc = flash_get_page_info_by_offs(flash_dev, TEST_AREA_OFFSET, &page_info_off);
	zassert_true(rc == 0, "flash_get_page_info_by_offs returned %d", rc);
	TC_PRINT("start_offset=0x%lx\tsize=%d\tindex=%d\n", page_info_off.start_offset,
		 (int)page_info_off.size, page_info_off.index);
	zassert_true(page_info_off.start_offset >= 0, "start_offset is %d", rc);
	zassert_true(page_info_off.size > 0, "size is %d", rc);
	zassert_true(page_info_off.index >= 0, "index is %d", rc);

	/* Get info for the same page with flash_get_page_info_by_idx() */
	rc = flash_get_page_info_by_idx(flash_dev, page_info_off.index, &page_info_idx);
	zassert_true(rc == 0, "flash_get_page_info_by_offs returned %d", rc);
	zassert_equal(page_info_off.start_offset, page_info_idx.start_offset);
	zassert_equal(page_info_off.size, page_info_idx.size);
	zassert_equal(page_info_off.index, page_info_idx.index);

	page_count = flash_get_page_count(flash_dev);
	TC_PRINT("page_count=%d\n", (int)page_count);
	zassert_true(page_count > 0, "flash_get_page_count returned %d", rc);
	zassert_true(page_count >= page_info_off.index);

	/* Test that callback is executed for every page */
	test_cb_data.exit_page = page_count + 1;
	flash_page_foreach(flash_dev, flash_callback, &test_cb_data);
	zassert_true(page_count == test_cb_data.page_counter,
		     "page_count = %d not equal to pages counted with cb = %d", page_count,
		     test_cb_data.page_counter);

	/* Test that callback can cancell iteration */
	test_cb_data.page_counter = 0;
	test_cb_data.exit_page = page_count >> 1;
	flash_page_foreach(flash_dev, flash_callback, &test_cb_data);
	zassert_true(test_cb_data.exit_page == test_cb_data.page_counter,
		     "%d pages were iterated while it shall stop on page %d",
		     test_cb_data.page_counter, test_cb_data.exit_page);
}

static void test_flash_copy_inner(const struct device *src_dev, off_t src_offset,
				  const struct device *dst_dev, off_t dst_offset, off_t size,
				  uint8_t *buf, size_t buf_size, int expected_result)
{
	int actual_result;

	if ((expected_result == 0) && (size != 0) && (src_offset != dst_offset)) {
		/* prepare for successful copy */
		zassert_ok(flash_flatten(flash_dev, page_info.start_offset, page_info.size));
		zassert_ok(flash_fill(flash_dev, 0xaa, page_info.start_offset, page_info.size));
		zassert_ok(flash_flatten(flash_dev, page_info.start_offset + page_info.size,
					 page_info.size));
	}

	/* perform copy (if args are valid) */
	actual_result = flash_copy(src_dev, src_offset, dst_dev, dst_offset, size, buf, buf_size);
	zassert_equal(actual_result, expected_result,
		      "flash_copy(%p, %lx, %p, %lx, %zu, %p, %zu) failed: expected: %d actual: %d",
		      src_dev, src_offset, dst_dev, dst_offset, size, buf, buf_size,
		      expected_result, actual_result);

	if ((expected_result == 0) && (size != 0) && (src_offset != dst_offset)) {
		/* verify a successful copy */
		off_t copy_size = MIN(size, EXPECTED_SIZE);

		zassert_ok(flash_read(flash_dev, TEST_AREA_OFFSET, expected, copy_size));
		for (int i = 0; i < copy_size; i++) {
			zassert_equal(buf[i], 0xaa, "incorrect data (%02x) at %d", buf[i], i);
		}
	}
}

ZTEST(flash_driver, test_flash_copy)
{
	uint8_t buf[EXPECTED_SIZE];
	const off_t off_max = (sizeof(off_t) == sizeof(int32_t)) ? INT32_MAX : INT64_MAX;

	/*
	 * Rather than explicitly testing 128+ permutations of input,
	 * merge redundant cases:
	 *  - src_dev or dst_dev are invalid
	 *  - src_offset or dst_offset are invalid
	 *  - src_offset + size or dst_offset + size overflow
	 *  - buf is NULL
	 *  - buf size is invalid
	 */
	test_flash_copy_inner(NULL, -1, NULL, -1, -1, NULL, 0, -EINVAL);
	test_flash_copy_inner(NULL, -1, NULL, -1, -1, NULL, sizeof(buf), -EINVAL);
	test_flash_copy_inner(NULL, -1, NULL, -1, -1, buf, sizeof(buf), -EINVAL);
	test_flash_copy_inner(NULL, -1, NULL, -1, page_info.size, buf, sizeof(buf), -EINVAL);
	test_flash_copy_inner(NULL, -1, NULL, -1, page_info.size, buf, sizeof(buf), -EINVAL);
	test_flash_copy_inner(NULL, page_info.start_offset, NULL,
			      page_info.start_offset + page_info.size, page_info.size, buf,
			      sizeof(buf), -ENODEV);
	test_flash_copy_inner(flash_dev, page_info.start_offset, flash_dev,
			      page_info.start_offset + page_info.size, page_info.size, buf,
			      sizeof(buf), 0);

	/* zero-sized copy should succeed */
	test_flash_copy_inner(flash_dev, page_info.start_offset, flash_dev,
			      page_info.start_offset + page_info.size, 0, buf, sizeof(buf), 0);

	/* copy with same offset should succeed */
	test_flash_copy_inner(flash_dev, page_info.start_offset, flash_dev, page_info.start_offset,
			      page_info.size, buf, sizeof(buf), 0);

	/* copy with integer overflow should fail */
	test_flash_copy_inner(flash_dev, off_max, flash_dev, page_info.start_offset, 42, buf,
			      sizeof(buf), -EINVAL);

	/* copy with overlapping ranges should fail */
	test_flash_copy_inner(flash_dev, page_info.start_offset, flash_dev,
			      page_info.start_offset + 32, page_info.size - 32, buf, sizeof(buf),
			      -EINVAL);
}

ZTEST_SUITE(flash_driver, NULL, NULL, flash_driver_before, NULL, NULL);
