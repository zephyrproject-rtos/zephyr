/*
 * Copyright (c) 2025 Koppel Electronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/flash_simulator.h>
#include <zephyr/device.h>

/* Test the flash simulator callbacks to modify the behaviour of the
 * flash simulator on the fly.
 */

/* configuration derived from DT */
#ifdef CONFIG_ARCH_POSIX
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_0)
#else
#define SOC_NV_FLASH_NODE DT_CHILD(DT_INST(0, zephyr_sim_flash), flash_sim_0)
#endif /* CONFIG_ARCH_POSIX */
#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIMULATOR_ERASE_UNIT DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_SIMULATOR_PROG_UNIT DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_SIMULATOR_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_SIMULATOR_ERASE_VALUE \
		DT_PROP(DT_PARENT(SOC_NV_FLASH_NODE), erase_value)

#if (defined(CONFIG_ARCH_POSIX) || defined(CONFIG_BOARD_QEMU_X86))
static const struct device *const flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
#else
static const struct device *const flash_dev = DEVICE_DT_GET(DT_NODELABEL(sim_flash_controller));
#endif


/* We are simulating the broken FLASH memory.
 * Simulate different types of errors depending on the erase page.
 * Page 0: behaves normal
 * Page 1: erase works as expected, all writes fails (no write, -EIO returned)
 * Page 2: erase works as expected, all writes silently corrupt data (bits 0, 1, 20 in sticks to 1)
 * Page 3: erase fails (no erase, -EIO returned), all writes fails (-EIO returned)
 * Page 4: erase fails silently (erases writes 0x55 to all bytes), all writes fails (-EIO returned)
 */
enum test_page_type {
	TEST_PAGE_TYPE_NORMAL,
	TEST_PAGE_TYPE_ERASE_OK_WRITE_FAIL,
	TEST_PAGE_TYPE_ERASE_OK_WRITE_CORRUPT,
	TEST_PAGE_TYPE_ERASE_FAIL_WRITE_FAIL,
	TEST_PAGE_TYPE_ERASE_CORRUPT_WRITE_FAIL,
};

/* Corruption pattern for write operations: bits 0, 1, and 20 in 32-bit word */
static const uint32_t test_write_corruption_pattern = 0x00100003;

static int test_write_byte_callback(const struct device *dev, off_t offset, uint8_t data)
{
	const struct flash_simulator_params *params = flash_simulator_get_params(dev);
	enum test_page_type page = offset / params->erase_unit;

	switch (page) {
	case TEST_PAGE_TYPE_ERASE_OK_WRITE_FAIL:
	case TEST_PAGE_TYPE_ERASE_FAIL_WRITE_FAIL:
	case TEST_PAGE_TYPE_ERASE_CORRUPT_WRITE_FAIL:
		return -EIO;
	case TEST_PAGE_TYPE_ERASE_OK_WRITE_CORRUPT: {
		/* Apply corruption pattern based on byte position within word */
		uint8_t byte_in_word = offset & 0x3;
		uint8_t corruption_byte =
			(test_write_corruption_pattern >> (byte_in_word * 8)) & 0xFF;

		return data | corruption_byte;
	}
	case TEST_PAGE_TYPE_NORMAL:
	default:
		return data;
	}
}

static int test_erase_unit_callback(const struct device *dev, off_t offset)
{
	const struct flash_simulator_params *params = flash_simulator_get_params(dev);
	enum test_page_type page = offset / params->erase_unit;
	size_t flash_size;
	uint8_t *flash_mock = flash_simulator_get_memory(dev, &flash_size);

	zassert_true(offset < flash_size, "Offset 0x%lx out of range (flash size 0x%zx)",
		    (long)offset, flash_size);

	switch (page) {
	case TEST_PAGE_TYPE_ERASE_FAIL_WRITE_FAIL:
		return -EIO;
	case TEST_PAGE_TYPE_ERASE_CORRUPT_WRITE_FAIL:
		/* Corrupt erase by writing 0x55 to all bytes */
		memset(flash_mock + offset, 0x55, params->erase_unit);
		return 0;
	case TEST_PAGE_TYPE_NORMAL:
	case TEST_PAGE_TYPE_ERASE_OK_WRITE_FAIL:
	case TEST_PAGE_TYPE_ERASE_OK_WRITE_CORRUPT:
	default:
		/* Normal erase behavior - callback must perform the erase */
		memset(flash_mock + offset, params->erase_value, params->erase_unit);
		return 0;
	}
}

static struct flash_simulator_cb test_flash_sim_cbs = {
	.write_byte = test_write_byte_callback,
	.erase_unit = test_erase_unit_callback,
};

void *flash_sim_cbs_setup(void)
{
	zassert_true(device_is_ready(flash_dev),
		     "Simulated flash device not ready");

	flash_simulator_set_callbacks(flash_dev, &test_flash_sim_cbs);

	return NULL;
}

/* Disable callbacks after tests to restore default simulator behavior */
static void flash_sim_cbs_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	flash_simulator_set_callbacks(flash_dev, NULL);
}

ZTEST(flash_sim_cbs, test_page_behaviors)
{
	int ret;
	uint32_t write_buf[FLASH_SIMULATOR_ERASE_UNIT / sizeof(uint32_t)];
	uint32_t read_buf[FLASH_SIMULATOR_ERASE_UNIT / sizeof(uint32_t)];
	off_t page_offset;
	size_t buf_size = sizeof(write_buf);

	/* Initialize write buffer with pseudo-random pattern */
	srand(0x12345678);
	for (size_t i = 0; i < ARRAY_SIZE(write_buf); i++) {
		write_buf[i] = ((uint32_t)rand() << 16) | (uint32_t)rand();
	}

	/* Test Page 0: TEST_PAGE_TYPE_NORMAL - behaves normal */
	page_offset = TEST_PAGE_TYPE_NORMAL * FLASH_SIMULATOR_ERASE_UNIT;

	ret = flash_erase(flash_dev, page_offset, FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(ret, 0, "Page 0: Erase should succeed");

	ret = flash_write(flash_dev, page_offset, write_buf, buf_size);
	zassert_equal(ret, 0, "Page 0: Write should succeed");

	ret = flash_read(flash_dev, page_offset, read_buf, buf_size);
	zassert_equal(ret, 0, "Page 0: Read should succeed");
	zassert_mem_equal(read_buf, write_buf, buf_size,
			  "Page 0: Data should match written data");

	/* Test Page 1: TEST_PAGE_TYPE_ERASE_OK_WRITE_FAIL - erase OK, write fails */
	page_offset = TEST_PAGE_TYPE_ERASE_OK_WRITE_FAIL * FLASH_SIMULATOR_ERASE_UNIT;

	ret = flash_erase(flash_dev, page_offset, FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(ret, 0, "Page 1: Erase should succeed");

	ret = flash_read(flash_dev, page_offset, read_buf, buf_size);
	zassert_equal(ret, 0, "Page 1: Read after erase should succeed");
	for (size_t i = 0; i < ARRAY_SIZE(read_buf); i++) {
		uint32_t expected = FLASH_SIMULATOR_ERASE_VALUE;

		expected |= (expected << 8) | (expected << 16) | (expected << 24);
		zassert_equal(read_buf[i], expected,
			      "Page 1: After erase, data should be erase value");
	}

	ret = flash_write(flash_dev, page_offset, write_buf, buf_size);
	zassert_equal(ret, -EIO, "Page 1: Write should fail with -EIO");

	/* Test Page 2: TEST_PAGE_TYPE_ERASE_OK_WRITE_CORRUPT - erase OK, write corrupts */
	page_offset = TEST_PAGE_TYPE_ERASE_OK_WRITE_CORRUPT * FLASH_SIMULATOR_ERASE_UNIT;

	ret = flash_erase(flash_dev, page_offset, FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(ret, 0, "Page 2: Erase should succeed");

	ret = flash_write(flash_dev, page_offset, write_buf, buf_size);
	zassert_equal(ret, 0, "Page 2: Write should succeed (but corrupt data)");

	ret = flash_read(flash_dev, page_offset, read_buf, buf_size);
	zassert_equal(ret, 0, "Page 2: Read should succeed");

	/* Verify corruption pattern: bits 0, 1, and 20 should be set */
	for (size_t i = 0; i < ARRAY_SIZE(read_buf); i++) {
		uint32_t expected = write_buf[i] | test_write_corruption_pattern;

		zassert_equal(read_buf[i], expected,
			      "Page 2: Data should be corrupted with pattern 0x%08x",
			      test_write_corruption_pattern);
	}

	/* Test Page 3: TEST_PAGE_TYPE_ERASE_FAIL_WRITE_FAIL - both fail */
	page_offset = TEST_PAGE_TYPE_ERASE_FAIL_WRITE_FAIL * FLASH_SIMULATOR_ERASE_UNIT;

	ret = flash_erase(flash_dev, page_offset, FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(ret, -EIO, "Page 3: Erase should fail with -EIO");

	ret = flash_write(flash_dev, page_offset, write_buf, buf_size);
	zassert_equal(ret, -EIO, "Page 3: Write should fail with -EIO");

	/* Test Page 4: TEST_PAGE_TYPE_ERASE_CORRUPT_WRITE_FAIL - erase corrupts, write fails */
	page_offset = TEST_PAGE_TYPE_ERASE_CORRUPT_WRITE_FAIL * FLASH_SIMULATOR_ERASE_UNIT;

	ret = flash_erase(flash_dev, page_offset, FLASH_SIMULATOR_ERASE_UNIT);
	zassert_equal(ret, 0, "Page 4: Erase should succeed (but corrupt)");

	ret = flash_read(flash_dev, page_offset, read_buf, buf_size);
	zassert_equal(ret, 0, "Page 4: Read after erase should succeed");
	for (size_t i = 0; i < ARRAY_SIZE(read_buf); i++) {
		zassert_equal(read_buf[i], 0x55555555,
			      "Page 4: After erase, data should be 0x55555555 (corrupted erase)");
	}

	ret = flash_write(flash_dev, page_offset, write_buf, buf_size);
	zassert_equal(ret, -EIO, "Page 4: Write should fail with -EIO");
}

ZTEST_SUITE(flash_sim_cbs, NULL, flash_sim_cbs_setup, NULL, NULL, flash_sim_cbs_teardown);
