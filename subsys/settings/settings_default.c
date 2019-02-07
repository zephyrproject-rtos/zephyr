/*
 * Copyright (c) 2019 Laczen JMS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <settings/settings.h>

#ifdef CONFIG_SETTINGS_DEFAULT_FS

#define FS_NAME (CONFIG_SETTINGS_DEFAULT_FS_MNT \
		 CONFIG_SETTINGS_DEFAULT_FS_FILE)

static struct settings_file default_settings = {
	.cf_maxlines = CONFIG_SETTINGS_DEFAULT_FS_MAX_LINES,
	.cf_name = FS_NAME
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
#include <flash.h>
static struct settings_nvs default_settings;

int settings_backend_init(void)
{
	int rc;
	struct flash_pages_info info;

	rc = flash_get_page_info_by_offs(device_get_binding(DT_FLASH_DEV_NAME),
					 DT_FLASH_AREA_STORAGE_OFFSET, &info);
	if (rc) {
		return rc;
	}
	/* define the nvs file system using the page_info */
	default_settings.cf_nvs.sector_size = info.size *
		CONFIG_SETTINGS_DEFAULT_NVS_SECTOR_SIZE_MULT;
	default_settings.cf_nvs.sector_count =
		CONFIG_SETTINGS_DEFAULT_NVS_SECTOR_COUNT;
	default_settings.cf_nvs.offset = DT_FLASH_AREA_STORAGE_OFFSET +
		CONFIG_SETTINGS_DEFAULT_NVS_OFFSET_MULT * info.size;

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
