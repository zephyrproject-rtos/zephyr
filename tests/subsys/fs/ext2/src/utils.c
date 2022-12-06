/*
 * Copyright (c) 2022 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>

#include "utils.h"

int wipe_partition(uintptr_t id)
{
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(id, &fap);
	if (rc < 0) {
		TC_PRINT("Error accessing flash area %lu (%d)\n", id, rc);
		return rc;
	}

	rc = flash_area_erase(fap, 0, fap->fa_size);
	(void)flash_area_close(fap);

	if (rc < 0) {
		TC_PRINT("Error wiping flash area %lu (%d)\n", id, rc);
		return rc;
	}
	return 0;
}

size_t get_partition_size(uintptr_t id)
{
	int rc;
	const struct flash_area *fap;

	rc = flash_area_open(id, &fap);
	if (rc < 0) {
		TC_PRINT("Error accessing flash area %lu (%d)\n", id, rc);
		return rc;
	}

	(void)flash_area_close(fap);
	return fap->fa_size;
}
