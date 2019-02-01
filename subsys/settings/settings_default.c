/*
 * Copyright (c) 2019 Laczen JMS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <assert.h>
#include <zephyr.h>
#include <settings/settings.h>

#ifdef CONFIG_SETTINGS_DEFAULT_FS

static struct settings_file default_settings = {
	.cf_maxlines = CONFIG_SETTINGS_DEFAULT_FS_MAX_LINES,
	.cf_name = CONFIG_SETTINGS_DEFAULT_FS_FILE
};

int settings_backend_init(void)
{
	int rc;

	rc = settings_file_backend_init(&default_settings);
	if (rc) {
		return rc;
	}

	rc = settings_file_src(&default_settings);
	if (rc) {
		return rc;
	}

	rc = settings_file_dst(&default_settings);
	return rc;

}

#elif defined(CONFIG_SETTINGS_DEFAULT_FCB)

static struct flash_sector settings_fcb_area[
		CONFIG_SETTINGS_DEFAULT_FCB_NUM_AREAS + 1];

static struct settings_fcb default_settings = {
	.fa_id = DT_FLASH_AREA_STORAGE_ID,
	.cf_fcb.f_sector_cnt = CONFIG_SETTINGS_DEFAULT_FCB_NUM_AREAS + 1,
	.cf_fcb.f_magic = CONFIG_SETTINGS_DEFAULT_FCB_MAGIC,
	.cf_fcb.f_version = SETTINGS_FCB_VERS,
	.cf_fcb.f_sectors = settings_fcb_area,
};

int settings_backend_init(void)
{
	int rc;

	rc = settings_fcb_backend_init(&default_settings);
	if (rc) {
		return rc;
	}

	rc = settings_fcb_src(&default_settings);

	if (rc) {
		return rc;
	}

	rc = settings_fcb_dst(&default_settings);

	return rc;
}

#elif defined(CONFIG_SETTINGS_DEFAULT_NVS)
#include <device.h>
static struct settings_nvs default_settings = {
	.cf_nvs.sector_size = FLASH_ERASE_BLOCK_SIZE *
		CONFIG_SETTINGS_DEFAULT_NVS_SECTOR_SIZE_MULT,
	.cf_nvs.sector_count = CONFIG_SETTINGS_DEFAULT_NVS_SECTOR_COUNT,
	.cf_nvs.offset = FLASH_AREA_STORAGE_OFFSET + FLASH_ERASE_BLOCK_SIZE *
		CONFIG_SETTINGS_DEFAULT_NVS_OFFSET_MULT,
};

int settings_backend_init(void)
{
	int rc;

	rc = nvs_init(&default_settings.cf_nvs, DT_FLASH_DEV_NAME);
	if (rc) {
		return -EINVAL;
	}

	rc = settings_nvs_backend_init(&default_settings);
	if (rc) {
		return rc;
	}

	rc = settings_nvs_src(&default_settings);

	if (rc) {
		return rc;
	}

	rc = settings_nvs_dst(&default_settings);

	return rc;
}

#else
int settings_backend_init(void)
{
	return 0;
}
#endif
