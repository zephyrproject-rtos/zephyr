/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

#include <nsettings/settings.h>
#include <zephyr.h>

void settings_init(void);
static int settings_default_backend_init(void);

int settings_subsys_init(void)
{
	static bool settings_initialized;
	int rc;

	if (settings_initialized) {
		return 0;
	}

	settings_init();
	rc = settings_default_backend_init();

	if (!rc) {
		settings_initialized = true;
	}

	return rc;
}

#ifdef CONFIG_NSETTINGS_DEFAULT_FS
#include "settings_file.h"

static struct settings_file default_settings = {
	.cf_maxlines = CONFIG_NSETTINGS_DEFAULT_FS_MAX_LINES,
	.cf_name = CONFIG_NSETTINGS_DEFAULT_FS_FILE
};

static int settings_default_backend_init(void)
{
	struct fs_dirent entry;
	char fname_0[SETTINGS_FILE_NAME_MAX+1];
	char fname_1[SETTINGS_FILE_NAME_MAX+1];
	size_t fsize_0, fsize_1;
	int rc;

	/*
	 * Must be called after root FS has been initialized.
	 */
	rc = fs_stat(CONFIG_NSETTINGS_DEFAULT_FS_DIR, &entry);
	if (rc) {
		if (rc == -ENOENT) {
			fs_mkdir(CONFIG_NSETTINGS_DEFAULT_FS_DIR);
		} else {
			return rc;
		}
	}
	/*
	 * Directory is created.
	 * There might be 2 settings files in use (when reset during copy),
	 * set the largest one as the one to use and delete the smallest
	 * if available
	 */
	if (!default_settings.cf_name) {
		return -EINVAL;
	}

	strcpy(fname_0, default_settings.cf_name);
	strcat(fname_0, "0");
	rc = fs_stat(fname_0, &entry);
	if (rc) {
		if (rc == -ENOENT) {
			/* File 0 not there */
			fsize_0 = 0;
		} else {
			return rc;
		}
	} else {
		fsize_0 = entry.size;
	}

	strcpy(fname_1, default_settings.cf_name);
	strcat(fname_1, "1");
	rc = fs_stat(fname_1, &entry);
	if (rc) {
		if (rc == -ENOENT) {
			/* File 1 not there */
			fsize_1 = 0;
		} else {
			return rc;
		}
	} else {
		fsize_1 = entry.size;
	}
	if (fsize_0 >= fsize_1) {
		default_settings.toggle = 0;
		if (fsize_1 > 0) {
			rc = fs_unlink(fname_1);
			if (rc) {
				return rc;
			}
		}
	} else {
		default_settings.toggle = 1;
		if (fsize_0 > 0) {
			rc = fs_unlink(fname_0);
			if (rc) {
				return rc;
			}
		}
	}

	rc = settings_file_src(&default_settings);
	if (rc) {
		return rc;
	}

	rc = settings_file_dst(&default_settings);
	return rc;
}

#elif defined(CONFIG_NSETTINGS_DEFAULT_FCB)
#include "settings_fcb.h"

#define SETTINGS_FCB_VERS		1

static struct flash_sector settings_fcb_area[
		CONFIG_NSETTINGS_DEFAULT_FCB_NUM_AREAS + 1];

static struct settings_fcb default_settings = {
	.cf_fcb.f_magic = CONFIG_NSETTINGS_DEFAULT_FCB_MAGIC,
	.cf_fcb.f_sectors = settings_fcb_area,
};

static int settings_default_backend_init(void)
{
	u32_t cnt = CONFIG_NSETTINGS_DEFAULT_FCB_NUM_AREAS + 1;
	int rc;
	const struct flash_area *fap;

	rc = flash_area_get_sectors(CONFIG_NSETTINGS_DEFAULT_FCB_FLASH_AREA,
				    &cnt, settings_fcb_area);
	if (rc != 0 && rc != -ENOMEM) {
		return rc;
	}

	default_settings.cf_fcb.f_sector_cnt = cnt;
	default_settings.cf_fcb.f_version = SETTINGS_FCB_VERS;
	default_settings.cf_fcb.f_scratch_cnt = 1;


	while (1) {
		rc = fcb_init(CONFIG_NSETTINGS_DEFAULT_FCB_FLASH_AREA,
				&default_settings.cf_fcb);
		if (rc) {
			return rc;
		}

		/*
		 * Check if system was reset in middle of emptying a sector.
		 * This situation is recognized by checking if the scratch block
		 * is missing.
		 */
		if (fcb_free_sector_cnt(&default_settings.cf_fcb) < 1) {

			rc = flash_area_erase(default_settings.cf_fcb.fap,
			default_settings.cf_fcb.f_active.fe_sector->fs_off,
			default_settings.cf_fcb.f_active.fe_sector->fs_size);

			if (rc) {
				return rc;
			}

		} else {

			break;

		}
	}

	if (rc != 0) {
		rc = flash_area_open(CONFIG_NSETTINGS_DEFAULT_FCB_FLASH_AREA,
				     &fap);

		if (rc == 0) {
			rc = flash_area_erase(fap, 0, fap->fa_size);
			flash_area_close(fap);
		}

		if (rc) {
			return rc;
		}
	}

	rc = settings_fcb_src(&default_settings);

	if (rc) {
		return rc;
	}

	rc = settings_fcb_dst(&default_settings);

	return rc;
}
#elif defined(CONFIG_NSETTINGS_DEFAULT_NVS)
#include <device.h>
#include "settings_nvs.h"
static struct settings_nvs default_settings = {
	.cf_nvs.sector_size = FLASH_ERASE_BLOCK_SIZE *
		CONFIG_NSETTINGS_DEFAULT_NVS_SECTOR_SIZE_MULT,
	.cf_nvs.sector_count = CONFIG_NSETTINGS_DEFAULT_NVS_SECTOR_COUNT,
	.cf_nvs.offset = FLASH_AREA_STORAGE_OFFSET + FLASH_ERASE_BLOCK_SIZE *
		CONFIG_NSETTINGS_DEFAULT_NVS_OFFSET_MULT,
};

static int settings_default_backend_init(void)
{
	int rc;
	u16_t last_name_id;

	rc = nvs_init(&default_settings.cf_nvs, DT_FLASH_DEV_NAME);
	if (rc) {
		return -EINVAL;
	}

	rc = nvs_read(&default_settings.cf_nvs, NVS_NAMECNT_ID, &last_name_id,
		      sizeof(last_name_id));
	if (rc < 0) {
		default_settings.last_name_id = NVS_NAMECNT_ID;
	} else {
		default_settings.last_name_id = last_name_id;
	}

	rc = settings_nvs_src(&default_settings);

	if (rc) {
		return rc;
	}

	rc = settings_nvs_dst(&default_settings);

	return rc;
}

#elif defined(CONFIG_NSETTINGS_DEFAULT_NONE)
static int settings_default_backend_init(void)
{
	return 0;
}
#endif
