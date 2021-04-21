/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings_test.h"
#include "settings/settings_fcb.h"
#include <sys/printk.h>

#define NAME_DELETABLE "4/deletable"

struct flash_sector fcb_small_sectors[2] = {
	[0] = {
		.fs_off = 0x00000000,
		.fs_size = 4 * 1024
	},
	[1] = {
		.fs_off = 0x00001000,
		.fs_size = 4 * 1024
	}
};

struct deletable_s {
	bool valid;
	uint32_t val32;
} deletable_val;

uint32_t val4v2;

int c4_handle_export(int (*cb)(const char *name, const void *value, size_t val_len));

struct settings_handler c4_test_handler = {
	.name = "4",
	.h_get = NULL,
	.h_set = NULL,
	.h_commit = NULL,
	.h_export = c4_handle_export
};

int c4_handle_export(int (*cb)(const char *name, const void *value, size_t val_len))
{
	if (deletable_val.valid) {
		(void)cb(NAME_DELETABLE, &deletable_val.val32,
			 sizeof(deletable_val.val32));
	} else {
		(void)cb(NAME_DELETABLE, NULL, 0);
	}

	(void)cb("4/dummy", &val4v2, sizeof(val4v2));

	return 0;
}

static int check_compressed_cb(struct fcb_entry_ctx *entry_ctx, void *arg)
{
	char buf[SETTINGS_MAX_NAME_LEN + SETTINGS_MAX_VAL_LEN +
		 SETTINGS_EXTRA_LEN];
	int rc;
	int len;

	len = entry_ctx->loc.fe_data_len;
	if (len >= sizeof(buf)) {
		len = sizeof(buf) - 1;
	}

	rc = flash_area_read(entry_ctx->fap,
			     FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc), buf, len);

	if (rc) {
		return 0;
	}
	buf[len] = '\0';

	rc = strncmp(buf, NAME_DELETABLE, sizeof(NAME_DELETABLE)-1);
	zassert_true(rc != 0, "The deleted settings shouldn be compressed.");

	return 0;
}

void test_config_compress_deleted(void)
{
	int rc;
	struct settings_fcb cf;
	int i;

	config_wipe_srcs();
	config_wipe_fcb(fcb_small_sectors, ARRAY_SIZE(fcb_small_sectors));

	cf.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC;
	cf.cf_fcb.f_sectors = fcb_small_sectors;
	cf.cf_fcb.f_sector_cnt = ARRAY_SIZE(fcb_small_sectors);

	rc = settings_fcb_src(&cf);
	zassert_true(rc == 0, "can't register FCB as configuration source");

	rc = settings_fcb_dst(&cf);
	zassert_true(rc == 0,
		     "can't register FCB as configuration destination");

	rc = settings_register(&c4_test_handler);
	zassert_true(rc == 0, "settings_register fail");

	deletable_val.valid = true;
	deletable_val.val32 = 2018U;
	val4v2 = 0U;

	rc = settings_save();
	zassert_true(rc == 0, "fcb write error");

	deletable_val.valid = false;

	for (i = 0; ; i++) {
		val4v2++;

		rc = settings_save();
		zassert_true(rc == 0, "fcb write error");

		if (cf.cf_fcb.f_active.fe_sector == &fcb_small_sectors[1]) {
			/*
			 * The commpresion should happened while the active
			 * sector was changing.
			 */
			break;
		}
	}

	rc = fcb_walk(&cf.cf_fcb, &fcb_small_sectors[1], check_compressed_cb,
		      NULL);
}
