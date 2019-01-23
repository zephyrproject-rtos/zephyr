/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcb.h>
#include <string.h>

#include "settings/settings.h"
#include "settings/settings_fcb.h"
#include "settings_priv.h"

#define SETTINGS_FCB_VERS		1

struct settings_fcb_load_cb_arg {
	load_cb cb;
	void *cb_arg;
};

static int settings_fcb_load(struct settings_store *cs, load_cb cb,
			     void *cb_arg);
static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len);

static struct settings_store_itf settings_fcb_itf = {
	.csi_load = settings_fcb_load,
	.csi_save = settings_fcb_save,
};

int settings_fcb_src(struct settings_fcb *cf)
{
	int rc;

	cf->cf_fcb.f_version = SETTINGS_FCB_VERS;
	cf->cf_fcb.f_scratch_cnt = 1;

	while (1) {
		rc = fcb_init(CONFIG_SETTINGS_FCB_FLASH_AREA, &cf->cf_fcb);
		if (rc) {
			return -EINVAL;
		}

		/*
		 * Check if system was reset in middle of emptying a sector.
		 * This situation is recognized by checking if the scratch block
		 * is missing.
		 */
		if (fcb_free_sector_cnt(&cf->cf_fcb) < 1) {

			rc = flash_area_erase(cf->cf_fcb.fap,
					cf->cf_fcb.f_active.fe_sector->fs_off,
					cf->cf_fcb.f_active.fe_sector->fs_size);

			if (rc) {
				return -EIO;
			}
		} else {
			break;
		}
	}

	cf->cf_store.cs_itf = &settings_fcb_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_fcb_dst(struct settings_fcb *cf)
{
	cf->cf_store.cs_itf = &settings_fcb_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

static int settings_fcb_load_cb(struct fcb_entry_ctx *entry_ctx, void *arg)
{
	struct settings_fcb_load_cb_arg *argp;
	char buf[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	int rc;

	argp = (struct settings_fcb_load_cb_arg *)arg;

	size_t len_read;

	rc = settings_line_name_read(buf, sizeof(buf), &len_read,
				     (void *)&entry_ctx->loc);
	if (rc) {
		return 0;
	}
	buf[len_read] = '\0';

	/*name, val-read_cb-ctx, val-off*/
	/* take into account '=' separator after the name */
	argp->cb(buf, (void *)&entry_ctx->loc, len_read + 1, argp->cb_arg);
	return 0;
}

static int settings_fcb_load(struct settings_store *cs, load_cb cb,
			     void *cb_arg)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	struct settings_fcb_load_cb_arg arg;
	int rc;

	arg.cb = cb;
	arg.cb_arg = cb_arg;
	rc = fcb_walk(&cf->cf_fcb, 0, settings_fcb_load_cb, &arg);
	if (rc) {
		return -EINVAL;
	}
	return 0;
}

static int read_handler(void *ctx, off_t off, char *buf, size_t *len)
{
	struct fcb_entry_ctx *entry_ctx = ctx;

	if (off >= entry_ctx->loc.fe_data_len) {
		return -EINVAL;
	}

	if ((off + *len) > entry_ctx->loc.fe_data_len) {
		*len = entry_ctx->loc.fe_data_len - off;
	}

	return flash_area_read(entry_ctx->fap,
			       FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc) + off, buf,
			       *len);
}

static void settings_fcb_compress(struct settings_fcb *cf)
{
	int rc;
	struct fcb_entry_ctx loc1;
	struct fcb_entry_ctx loc2;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	int copy;
	u8_t rbs;

	rc = fcb_append_to_scratch(&cf->cf_fcb);
	if (rc) {
		return; /* XXX */
	}

	rbs = flash_area_align(cf->cf_fcb.fap);

	loc1.fap = cf->cf_fcb.fap;

	loc1.loc.fe_sector = NULL;
	loc1.loc.fe_elem_off = 0;

	while (fcb_getnext(&cf->cf_fcb, &loc1.loc) == 0) {
		if (loc1.loc.fe_sector != cf->cf_fcb.f_oldest) {
			break;
		}

		size_t val1_off;

		rc = settings_line_name_read(name1, sizeof(name1), &val1_off,
					     &loc1);
		if (rc) {
			continue;
		}

		if (val1_off + 1 == loc1.loc.fe_data_len) {
			/* Lack of a value so the record is a deletion-record */
			/* No sense to copy empty entry from */
			/* the oldest sector */
			continue;
		}

		loc2 = loc1;
		copy = 1;

		while (fcb_getnext(&cf->cf_fcb, &loc2.loc) == 0) {
			size_t val2_off;

			rc = settings_line_name_read(name2, sizeof(name2),
						     &val2_off, &loc2);
			if (rc) {
				continue;
			}

			if ((val1_off == val2_off) &&
			    !memcmp(name1, name2, val1_off)) {
				copy = 0;
				break;
			}
		}
		if (!copy) {
			continue;
		}

		/*
		 * Can't find one. Must copy.
		 */
		rc = fcb_append(&cf->cf_fcb, loc1.loc.fe_data_len, &loc2.loc);
		if (rc) {
			continue;
		}

		rc = settings_entry_copy(&loc2, 0, &loc1, 0,
					 loc1.loc.fe_data_len);
		if (rc) {
			continue;
		}
		rc = fcb_append_finish(&cf->cf_fcb, &loc2.loc);
		__ASSERT(rc == 0, "Failed to finish fcb_append.\n");
	}
	rc = fcb_rotate(&cf->cf_fcb);

	__ASSERT(rc == 0, "Failed to fcb rotate.\n");
}

static size_t get_len_cb(void *ctx)
{
	struct fcb_entry_ctx *entry_ctx = ctx;

	return entry_ctx->loc.fe_data_len;
}

static int write_handler(void *ctx, off_t off, char const *buf, size_t len)
{
	struct fcb_entry_ctx *entry_ctx = ctx;

	return flash_area_write(entry_ctx->fap,
				FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc) + off,
				buf, len);
}

/* ::csi_save implementation */
static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	struct fcb_entry_ctx loc;
	int len;
	int rc;
	int i;
	u8_t wbs;

	if (!name) {
		return -EINVAL;
	}

	wbs = flash_area_align(cf->cf_fcb.fap);
	len = settings_line_len_calc(name, val_len);

	for (i = 0; i < cf->cf_fcb.f_sector_cnt - 1; i++) {
		rc = fcb_append(&cf->cf_fcb, len, &loc.loc);
		if (rc != FCB_ERR_NOSPACE) {
			break;
		}
		settings_fcb_compress(cf);
	}
	if (rc) {
		return -EINVAL;
	}

	loc.fap = cf->cf_fcb.fap;

	rc = settings_line_write(name, value, val_len, 0, (void *)&loc);

	if (rc != -EIO) {
		i = fcb_append_finish(&cf->cf_fcb, &loc.loc);
		if (!rc) {
			rc = i;
		}
	}
	return rc;
}

void settings_mount_fcb_backend(struct settings_fcb *cf)
{
	u8_t rbs;

	rbs = cf->cf_fcb.f_align;

	settings_line_io_init(read_handler, write_handler, get_len_cb, rbs);
}
