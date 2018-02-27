/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <device.h>
#include <flash.h>
#include <zephyr/types.h>
#include <ztest_assert.h>

static u8_t rambuf[FLASH_AREA_NFFS_SIZE];

static int test_ram_flash_init(struct device *dev)
{
	return 0;
}

static int test_flash_ram_write_protection(struct device *dev, bool enable)
{
	return 0;
}

static int test_flash_ram_erase(struct device *dev, off_t offset, size_t len)
{
	struct flash_pages_info info;
	off_t end_offset = offset + len;

	zassert_true(offset >= 0, "invalid offset");
	zassert_true(offset + len <= FLASH_AREA_NFFS_SIZE,
		     "flash address out of bounds");

	while (offset < end_offset) {
		flash_get_page_info_by_offs(dev, offset, &info);
		memset(rambuf + info.start_offset, 0xff, info.size);
		offset = info.start_offset + info.size;
	}

	return 0;
}

static int test_flash_ram_write(struct device *dev, off_t offset,
						const void *data, size_t len)
{
	zassert_true(offset >= 0, "invalid offset");
	zassert_true(offset + len <= FLASH_AREA_NFFS_SIZE,
		     "flash address out of bounds");

	memcpy(rambuf + offset, data, len);

	return 0;
}

static int test_flash_ram_read(struct device *dev, off_t offset, void *data,
								size_t len)
{
	zassert_true(offset >= 0, "invalid offset");
	zassert_true(offset + len <= FLASH_AREA_NFFS_SIZE,
		     "flash address out of bounds");

	memcpy(data, rambuf + offset, len);

	return 0;
}

static void test_flash_ram_pages_layout(struct device *dev,
				const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	/* Same as used in Mynewt native "flash" backend */
	static struct flash_pages_layout dev_layout[] = {
		{ 4, 16 * 1024 },
		{ 1, 64 * 1024 },
		{ 7, 128 * 1024 },
	};

	*layout = dev_layout;
	*layout_size = ARRAY_SIZE(dev_layout);
}

static const struct flash_driver_api flash_ram_api = {
	.write_protection = test_flash_ram_write_protection,
	.erase = test_flash_ram_erase,
	.write = test_flash_ram_write,
	.read = test_flash_ram_read,
	.page_layout = test_flash_ram_pages_layout,
};

DEVICE_AND_API_INIT(flash_ram_test, "ram_flash_test_drv", test_ram_flash_init,
					NULL, NULL, POST_KERNEL,
					CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
					&flash_ram_api);
