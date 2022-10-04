/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <ztest.h>
#include <zephyr/storage/flash_map.h>

int check_file_dir_exists(const char *fpath)
{
	int res;
	struct fs_dirent entry;

	res = fs_stat(fpath, &entry);

	return !res;
}

void test_clear_flash(void)
{
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(FLASH_AREA_ID(storage), &fap);
	zassert_equal(rc, 0, "Opening flash area for erase [%d]\n", rc);

	rc = flash_area_erase(fap, 0, fap->fa_size);
	zassert_equal(rc, 0, "Erasing flash area [%d]\n", rc);
}
