/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bmc_fs, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/kvss/nvs.h>

#include "fs.h"

/*
 * Some STM32 internal flash seems to have an issue where writes randomly fail.
 * Noticed with the smaller sectors at the beginning of the flash space used
 * for app storage.  Try to work around it by retrying write EIO fails.
 */
#define FLASH_WRITE_RETRY_COUNT 3

#if !defined(PARTITION_EXISTS)
/* Zephyr pre-4.4 compat */
#define PARTITION_EXISTS FIXED_PARTITION_EXISTS
#define PARTITION_ID FIXED_PARTITION_ID
#define PARTITION_DEVICE FIXED_PARTITION_DEVICE
#define PARTITION_OFFSET FIXED_PARTITION_OFFSET
#define PARTITION_SIZE FIXED_PARTITION_SIZE
#endif

#if defined(CONFIG_BMC_PERSISTENT_STORAGE) && PARTITION_EXISTS(STORAGE_PARTITION_LABEL)
#define STORAGE_PARTITION_ID		PARTITION_ID(STORAGE_PARTITION_LABEL)
#define STORAGE_PARTITION_DEVICE	PARTITION_DEVICE(STORAGE_PARTITION_LABEL)
#define STORAGE_PARTITION_OFFSET	PARTITION_OFFSET(STORAGE_PARTITION_LABEL)
#define STORAGE_PARTITION_SIZE		PARTITION_SIZE(STORAGE_PARTITION_LABEL)

static bool fs_is_enabled;

bool fs_enabled(void)
{
	return fs_is_enabled;
}

static struct nvs_fs nvs_fs = {
	.flash_device = STORAGE_PARTITION_DEVICE,
	.offset = STORAGE_PARTITION_OFFSET,
};

static int mount_fs(void)
{
	struct flash_pages_info info;
	int rc;

	LOG_INF("Mounting NVS...");

	rc = flash_get_page_info_by_offs(nvs_fs.flash_device, nvs_fs.offset, &info);
	if (rc) {
		LOG_ERR("Unable to get flash info (err=%d)", rc);
		return rc;
	}

	nvs_fs.sector_size = info.size;
	nvs_fs.sector_count = STORAGE_PARTITION_SIZE / nvs_fs.sector_size;
	LOG_INF("  NVS using flash at 0x%08lx sector size 0x%08x sector count %u",
		nvs_fs.offset, nvs_fs.sector_size, nvs_fs.sector_count);

	/*
	 * CONFIG_NVS_INIT_BAD_MEMORY_REGION is set, so this will try to init
	 * storage if there was no NVS filesystem there.
	 */
	rc = nvs_mount(&nvs_fs);
	if (rc) {
		LOG_DBG("Mount failed (err=%d)", rc);
		return rc;
	}

	fs_is_enabled = true;

	LOG_INF("NVS mounted");

	return 0;
}

static int umount_fs(void)
{
	fs_is_enabled = false;

	return 0;
}

int fs_clear(void)
{
	int rc;
	int retry_count = 0;

	if (!fs_enabled())
		return -ENODEV;

again:
	rc = nvs_clear(&nvs_fs);
	if (rc) {
		if (rc == -EIO && retry_count < FLASH_WRITE_RETRY_COUNT) {
			retry_count++;
			goto again;
		}
		LOG_ERR("Could not clear config (err=%d)", rc);
		return rc;
	}

	return 0;
}

ssize_t fs_key_read(uint16_t id, void *buf, size_t size)
{
	ssize_t rc;

	if (!fs_enabled())
		return -ENODEV;

	rc = nvs_read(&nvs_fs, id, buf, size);
	if (rc < 0) {
		if (rc != -ENOENT)
			LOG_ERR("Could not read config id %u (err=%zd)", id, rc);
		return rc;
	}

	if ((size_t)rc > size) {
		LOG_INF("Read config id %u object is larger than size (read %zd)", id, rc);
		rc = -EINVAL; /* Fail */
	}

	return rc;
}

ssize_t fs_key_write(uint16_t id, const void *buf, size_t size)
{
	ssize_t rc;
	int retry_count = 0;

	if (!fs_enabled())
		return -ENODEV;

again:
	rc = nvs_write(&nvs_fs, id, buf, size);
	if (rc < 0) {
		if (rc == -EIO && retry_count < FLASH_WRITE_RETRY_COUNT) {
			retry_count++;
			goto again;
		}
		LOG_ERR("Could not write config id %u (err=%zd)", id, rc);
		return rc;
	}

	if (rc == 0) {
		LOG_DBG("Writing unchanged data config id %u", id);
		return size;
	}

	return rc;
}

int fs_init(void)
{
	int rc;

	if (!device_is_ready(STORAGE_PARTITION_DEVICE)) {
		LOG_ERR("Flash device not ready");
		return -ENODEV;
	}

	rc = mount_fs();
	if (rc)
		return rc;

	return 0;
}

int fs_exit(void)
{
	return umount_fs();
}
#endif /* defined(CONFIG_BMC_PERSISTENT_STORAGE) && PARTITION_EXISTS(STORAGE_PARTITION_LABEL) */
