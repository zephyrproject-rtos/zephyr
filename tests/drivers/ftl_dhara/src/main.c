/*
 * Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/storage/disk_access.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT   zephyr_ftl_dhara
#define FTL_DISK_NAME   DT_INST_PROP(0, disk_name)
#define FTL_BUFFER_SIZE DT_INST_PROP(0, buffer_size)

static const struct device *const dev = DEVICE_DT_GET(DT_DRV_INST(0));

static void *ftl_dhara_setup(void)
{
	zassert_true(device_is_ready(dev), "FTL device is not ready");

	return NULL;
}

ZTEST(ftl_dhara, test_init_via_ioctl)
{
	int ret;

	ret = disk_access_status(FTL_DISK_NAME);
	zassert_equal(ret, DISK_STATUS_NOMEDIA, "Disk status before initialisation is %d", ret);

	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_CTRL_INIT, NULL);
	zassert_equal(ret, 0, "Disk initialisation failed with %d", ret);

	ret = disk_access_status(FTL_DISK_NAME);
	zassert_equal(ret, DISK_STATUS_OK, "Disk status after initialisation is %d", ret);
}

ZTEST(ftl_dhara, test_ioctl)
{
	int ret;
	uint32_t value;

	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_RESERVED, NULL);
	zassert_equal(ret, -ENOTSUP, "Disk ioctl returned %d", ret);

	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &value);
	zassert_equal(ret, 0, "Getting sector count failed with %d", ret);
	zassert_equal(value, 0x544, "Sector count is %X", value);

	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &value);
	zassert_equal(ret, 0, "Getting sector size failed with %d", ret);
	zassert_equal(value, 0x200, "Sector size is %X", value);

	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_GET_ERASE_BLOCK_SZ, &value);
	zassert_equal(ret, 0, "Getting erase block size failed with %d", ret);
	zassert_equal(value, 0x1000, "Erase block size is %X", value);

	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_CTRL_SYNC, NULL);
	zassert_equal(ret, 0, "Disk synchronisation failed with %d", ret);
	ret = disk_access_ioctl(FTL_DISK_NAME, DISK_IOCTL_CTRL_DEINIT, NULL);
	zassert_equal(ret, 0, "Disk synchronisation failed with %d", ret);
}

ZTEST(ftl_dhara, test_read_bounds)
{
	int ret;
	uint8_t buffer[FTL_BUFFER_SIZE * 2];

	ret = disk_access_read(FTL_DISK_NAME, buffer, 0, 1);
	zassert_equal(ret, 0, "Disk read failed with %d", ret);

	ret = disk_access_read(FTL_DISK_NAME, buffer, 0x800, 1);
	zassert_equal(ret, -EINVAL, "Disk read returned %d", ret);

	ret = disk_access_read(FTL_DISK_NAME, buffer, 0x07FE, 2);
	zassert_equal(ret, 0, "Disk read failed with %d", ret);

	ret = disk_access_read(FTL_DISK_NAME, buffer, 0x07FF, 2);
	zassert_equal(ret, -EINVAL, "Disk read returned %d", ret);
}

ZTEST(ftl_dhara, test_write_bounds)
{
	int ret;
	uint8_t buffer[FTL_BUFFER_SIZE * 2] = {0};

	ret = disk_access_write(FTL_DISK_NAME, buffer, 0, 1);
	zassert_equal(ret, 0, "Disk write failed with %d", ret);

	ret = disk_access_write(FTL_DISK_NAME, buffer, 0x800, 1);
	zassert_equal(ret, -EINVAL, "Disk write returned %d", ret);

	ret = disk_access_write(FTL_DISK_NAME, buffer, 0x07FE, 2);
	zassert_equal(ret, 0, "Disk write failed with %d", ret);

	ret = disk_access_write(FTL_DISK_NAME, buffer, 0x07FF, 2);
	zassert_equal(ret, -EINVAL, "Disk write returned %d", ret);
}

ZTEST(ftl_dhara, test_read_write)
{
	int ret;
	uint8_t read_buffer[FTL_BUFFER_SIZE * 2] = {0x00};
	uint8_t write_buffer[FTL_BUFFER_SIZE * 2] = {0x00};
	uint8_t empty_sector[FTL_BUFFER_SIZE];
	uint8_t unread_sector[FTL_BUFFER_SIZE] = {0x00};

	for (size_t i = 0; i < sizeof(write_buffer); i++) {
		write_buffer[i] = (uint8_t)(i & 0xFF);
	}
	memset(empty_sector, 0xFF, sizeof(empty_sector));

	ret = disk_access_write(FTL_DISK_NAME, write_buffer, 0, 2);
	zassert_equal(ret, 0, "Disk write failed with %d", ret);

	ret = disk_access_read(FTL_DISK_NAME, read_buffer, 0, 1);
	zassert_equal(ret, 0, "Disk read failed with %d", ret);
	zassert_mem_equal(read_buffer, write_buffer, FTL_BUFFER_SIZE, "Read data is wrong");
	zassert_mem_equal(read_buffer + FTL_BUFFER_SIZE, unread_sector, FTL_BUFFER_SIZE,
			  "Read data is wrong");

	ret = disk_access_read(FTL_DISK_NAME, read_buffer, 0, 2);
	zassert_equal(ret, 0, "Disk read failed with %d", ret);
	zassert_mem_equal(read_buffer, write_buffer, FTL_BUFFER_SIZE * 2, "Read data is wrong");

	ret = disk_access_read(FTL_DISK_NAME, read_buffer, 0x10, 1);
	zassert_equal(ret, 0, "Disk read failed with %d", ret);
	zassert_mem_equal(read_buffer, empty_sector, FTL_BUFFER_SIZE, "Read data is wrong");
}

ZTEST_SUITE(ftl_dhara, NULL, ftl_dhara_setup, NULL, NULL, NULL);
