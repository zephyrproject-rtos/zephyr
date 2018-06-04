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

#include "settings/settings.h"
#include "settings/settings_file.h"
#include <zephyr.h>

void settings_init(void);

#ifdef CONFIG_SETTINGS_FS
#include <fs.h>

static struct settings_file config_init_settings_file = {
	.cf_name = CONFIG_SETTINGS_FS_FILE,
	.cf_maxlines = CONFIG_SETTINGS_FS_MAX_LINES
};

static void settings_init_fs(void)
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
}

#elif defined(CONFIG_SETTINGS_FCB)
#include "fcb.h"
#include "settings/settings_fcb.h"

static struct flash_sector settings_fcb_area[CONFIG_SETTINGS_FCB_NUM_AREAS + 1];

static struct settings_fcb config_init_settings_fcb = {
	.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC,
	.cf_fcb.f_sectors = settings_fcb_area,
};

static void settings_init_fcb(void)
{
	u32_t cnt = CONFIG_SETTINGS_FCB_NUM_AREAS + 1;
	int rc;
	const struct flash_area *fap;

	rc = flash_area_get_sectors(CONFIG_SETTINGS_FCB_FLASH_AREA, &cnt,
				    settings_fcb_area);
	if (rc != 0 && rc != -ENOMEM) {
		k_panic();
	}

	config_init_settings_fcb.cf_fcb.f_sector_cnt = cnt;

	rc = settings_fcb_src(&config_init_settings_fcb);

	if (rc != 0) {
		rc = flash_area_open(CONFIG_SETTINGS_FCB_FLASH_AREA, &fap);

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
}

#endif

int settings_subsys_init(void)
{
	static bool settings_initialized;
	int err;

	if (settings_initialized) {
		return 0;
	}

	settings_init();

#ifdef CONFIG_SETTINGS_FS
	settings_init_fs(); /* func rises kernel panic once error */

	/*
	 * Must be called after root FS has been initialized.
	 */
	err = fs_mkdir(CONFIG_SETTINGS_FS_DIR);
	/*
	 * The following lines mask the file exist error.
	 */
	if (err == -EEXIST) {
		err = 0;
	}
#elif defined(CONFIG_SETTINGS_FCB)
	settings_init_fcb(); /* func rises kernel panic once error */
	err = 0;
#endif

	if (!err) {
		settings_initialized = true;
	}

	return err;
}
