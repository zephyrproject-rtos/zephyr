/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

#include "settings/settings.h"
#include "settings/settings_file.h"
#include <zephyr.h>


bool settings_subsys_initialized;

void settings_init(void);

int settings_backend_init(void);

#ifdef CONFIG_SETTINGS_FS
#include <fs/fs.h>

static struct settings_file config_init_settings_file = {
	.cf_name = CONFIG_SETTINGS_FS_FILE,
	.cf_maxlines = CONFIG_SETTINGS_FS_MAX_LINES
};

int settings_backend_init(void)
{
	int rc;

	rc = settings_file_src(&config_init_settings_file);
	if (rc) {
		k_panic();
	}

	rc = settings_file_dst(&config_init_settings_file);
	if (rc) {
		k_panic();
	}

	settings_mount_fs_backend(&config_init_settings_file);

	/*
	 * Must be called after root FS has been initialized.
	 */
	rc = fs_mkdir(CONFIG_SETTINGS_FS_DIR);

	/*
	 * The following lines mask the file exist error.
	 */
	if (rc == -EEXIST) {
		rc = 0;
	}

	return rc;
}

#elif defined(CONFIG_SETTINGS_FCB)
#include <fs/fcb.h>
#include "settings/settings_fcb.h"

static struct flash_sector settings_fcb_area[CONFIG_SETTINGS_FCB_NUM_AREAS + 1];

static struct settings_fcb config_init_settings_fcb = {
	.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC,
	.cf_fcb.f_sectors = settings_fcb_area,
};

int settings_backend_init(void)
{
	u32_t cnt = CONFIG_SETTINGS_FCB_NUM_AREAS + 1;
	int rc;
	const struct flash_area *fap;

	rc = flash_area_get_sectors(DT_FLASH_AREA_STORAGE_ID, &cnt,
				    settings_fcb_area);
	if (rc == -ENODEV) {
		return rc;
	} else if (rc != 0 && rc != -ENOMEM) {
		k_panic();
	}

	config_init_settings_fcb.cf_fcb.f_sector_cnt = cnt;

	rc = settings_fcb_src(&config_init_settings_fcb);

	if (rc != 0) {
		rc = flash_area_open(DT_FLASH_AREA_STORAGE_ID, &fap);

		if (rc == 0) {
			rc = flash_area_erase(fap, 0, fap->fa_size);
			flash_area_close(fap);
		}

		if (rc != 0) {
			k_panic();
		} else {
			rc = settings_fcb_src(&config_init_settings_fcb);
		}
	}

	if (rc != 0) {
		k_panic();
	}

	rc = settings_fcb_dst(&config_init_settings_fcb);

	if (rc != 0) {
		k_panic();
	}

	settings_mount_fcb_backend(&config_init_settings_fcb);

	return rc;
}

#elif defined(CONFIG_SETTINGS_NVS)
#include <device.h>
#include <flash_map.h>
#include "settings/settings_nvs.h"

static struct settings_nvs default_settings_nvs;

int settings_backend_init(void)
{
	int rc;
	u16_t cnt = 0;
	size_t nvs_sector_size, nvs_size = 0;
	const struct flash_area *fa;
	struct flash_sector hw_flash_sector;
	u32_t sector_cnt = 1;

	rc = flash_area_open(DT_FLASH_AREA_STORAGE_ID, &fa);
	if (rc) {
		return rc;
	}

	rc = flash_area_get_sectors(DT_FLASH_AREA_STORAGE_ID, &sector_cnt,
				    &hw_flash_sector);
	if (rc == -ENODEV) {
		return rc;
	} else if (rc != 0 && rc != -ENOMEM) {
		k_panic();
	}

	nvs_sector_size = CONFIG_SETTINGS_NVS_SECTOR_SIZE_MULT *
			  hw_flash_sector.fs_size;

	if (nvs_sector_size > UINT16_MAX) {
		return -EDOM;
	}

	while (cnt < CONFIG_SETTINGS_NVS_SECTOR_COUNT) {
		nvs_size += nvs_sector_size;
		if (nvs_size > fa->fa_size) {
			break;
		}
		cnt++;
	}

	/* define the nvs file system using the page_info */
	default_settings_nvs.cf_nvs.sector_size = nvs_sector_size;
	default_settings_nvs.cf_nvs.sector_count = cnt;
	default_settings_nvs.cf_nvs.offset = fa->fa_off;
	default_settings_nvs.flash_dev_name = fa->fa_dev_name;

	rc = settings_nvs_backend_init(&default_settings_nvs);
	if (rc) {
		return rc;
	}

	rc = settings_nvs_src(&default_settings_nvs);

	if (rc) {
		return rc;
	}

	rc = settings_nvs_dst(&default_settings_nvs);

	return rc;
}

#elif defined(CONFIG_SETTINGS_NONE)
int settings_backend_init(void)
{
	return 0;
}
#endif

int settings_subsys_init(void)
{

	int err = 0;

	if (settings_subsys_initialized) {
		return 0;
	}

	settings_init();

	err = settings_backend_init(); /* func rises kernel panic once error */

	if (!err) {
		settings_subsys_initialized = true;
	}

	return err;
}
