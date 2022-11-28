/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>

#define LITTLEFS_PARTITION	littlefs_dev
#define LITTLEFS_PARTITION_ID	FIXED_PARTITION_ID(LITTLEFS_PARTITION)

/* NFFS work area strcut */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(cstorage);
static struct fs_mount_t littlefs_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &cstorage,
	.storage_dev = (void *)LITTLEFS_PARTITION_ID,
	.mnt_point = TEST_FS_MPTR,
};

void *config_setup_littlefs(void)
{
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(LITTLEFS_PARTITION_ID, &fap);
	zassume_true(rc == 0, "opening flash area for erase [%d]\n", rc);

	rc = flash_area_erase(fap, fap->fa_off, fap->fa_size);
	zassume_true(rc == 0, "erasing flash area [%d]\n", rc);

	rc = fs_mount(&littlefs_mnt);
	zassume_true(rc == 0, "mounting littlefs [%d]\n", rc);
	settings_config_setup();
	return NULL;
}
