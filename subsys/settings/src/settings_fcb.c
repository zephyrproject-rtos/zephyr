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

/* Version 2: Add a status byte to tell if the settings is valid */
#define SETTINGS_FCB_VERS		2

/* Add an extra byte for the status of the settings entry */
#define FCB_SETTINGS_EXTRA_LEN          1

#define FCB_SETTINGS_BUFFER_LEN (FCB_SETTINGS_EXTRA_LEN + \
	SETTINGS_MAX_NAME_LEN + SETTINGS_MAX_VAL_LEN + SETTINGS_EXTRA_LEN)

#define SETTINGS_ENTRY_VALID   0xFF
#define SETTINGS_ENTRY_INVALID 0x00

struct settings_fcb_load_cb_arg {
	load_cb cb;
	void *cb_arg;
};

static int settings_fcb_load(struct settings_store *cs, load_cb cb,
			     void *cb_arg);
static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value);

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
	char buf[FCB_SETTINGS_BUFFER_LEN];
	char *name_str;
	char *val_str;
	int rc;
	int len;

	argp = (struct settings_fcb_load_cb_arg *)arg;

	len = entry_ctx->loc.fe_data_len;
	if (len >= sizeof(buf)) {
		len = sizeof(buf) - 1;
	}

	rc = flash_area_read(entry_ctx->fap,
			     FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc), buf, len);

	if (rc) {
		return 0;
	}

	/* Check if the setting entry is valid */
	if (buf[0] == SETTINGS_ENTRY_INVALID) {
		return 0;
	}

	buf[len] = '\0';

	rc = settings_line_parse(buf + FCB_SETTINGS_EXTRA_LEN, &name_str,
				 &val_str);
	if (rc) {
		return 0;
	}
	argp->cb(name_str, val_str, argp->cb_arg);
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

static int settings_fcb_var_read(struct fcb_entry_ctx *entry_ctx, char *buf,
				 char **name, char **val)
{
	int rc;

	rc = flash_area_read(entry_ctx->fap,
			     FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc), buf,
			     entry_ctx->loc.fe_data_len);
	if (rc) {
		return rc;
	}

	/* Check if the setting entry is valid */
	if (buf[0] == SETTINGS_ENTRY_INVALID) {
		return 0;
	}

	buf[entry_ctx->loc.fe_data_len] = '\0';

	rc = settings_line_parse(buf + FCB_SETTINGS_EXTRA_LEN, name, val);
	return rc;
}

static void settings_fcb_compress(struct settings_fcb *cf)
{
	int rc;
	char buf1[FCB_SETTINGS_BUFFER_LEN];
	char buf2[FCB_SETTINGS_BUFFER_LEN];
	struct fcb_entry_ctx loc1;
	struct fcb_entry_ctx loc2;
	char *name1, *val1;
	char *name2, *val2;
	int copy;

	rc = fcb_append_to_scratch(&cf->cf_fcb);
	if (rc) {
		return; /* XXX */
	}

	loc1.fap = cf->cf_fcb.fap;

	loc1.loc.fe_sector = NULL;
	loc1.loc.fe_elem_off = 0;
	while (fcb_getnext(&cf->cf_fcb, &loc1.loc) == 0) {
		if (loc1.loc.fe_sector != cf->cf_fcb.f_oldest) {
			break;
		}
		rc = settings_fcb_var_read(&loc1, buf1, &name1, &val1);
		if (rc) {
			continue;
		}
		if (!val1) {
			/* No sense to copy empty entry from the oldest sector*/
			continue;
		}
		loc2 = loc1;
		copy = 1;
		while (fcb_getnext(&cf->cf_fcb, &loc2.loc) == 0) {
			rc = settings_fcb_var_read(&loc2, buf2, &name2, &val2);
			if (rc) {
				continue;
			}
			if (!strcmp(name1, name2)) {
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
		rc = flash_area_read(loc1.fap, FCB_ENTRY_FA_DATA_OFF(loc1.loc),
				     buf1, loc1.loc.fe_data_len);
		if (rc) {
			continue;
		}
		rc = fcb_append(&cf->cf_fcb, loc1.loc.fe_data_len, &loc2.loc);
		if (rc) {
			continue;
		}
		rc = flash_area_write(loc2.fap, FCB_ENTRY_FA_DATA_OFF(loc2.loc),
				      buf1, loc1.loc.fe_data_len);
		if (rc) {
			continue;
		}
		rc = fcb_append_finish(&cf->cf_fcb, &loc2.loc);
		__ASSERT(rc == 0, "Failed to finish fcb_append.\n");
	}
	rc = fcb_rotate(&cf->cf_fcb);

	__ASSERT(rc == 0, "Failed to fcb rotate.\n");
}

static int settings_fcb_invalid_entry(struct fcb_entry_ctx *entry_ctx,
				      void *arg)
{
	const char *entry_name = arg;
	char buf[FCB_SETTINGS_BUFFER_LEN];
	size_t len;
	char *name_str;
	char *val_str;
	int rc;

	len = entry_ctx->loc.fe_data_len;
	if (len >= sizeof(buf)) {
		len = sizeof(buf) - 1;
	}

	rc = flash_area_read(entry_ctx->fap,
			     FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc),
			     buf, len);
	if (rc) {
		return rc;
	}

	/* Check if the setting entry is valid */
	if (buf[0] == SETTINGS_ENTRY_INVALID) {
		return 0;
	}

	buf[len] = '\0';

	rc = settings_line_parse(buf + FCB_SETTINGS_EXTRA_LEN, &name_str,
				 &val_str);
	if (rc) {
		return 0;
	}

	if (strcmp(entry_name, name_str)) {
		return 0;
	}

	buf[0] = SETTINGS_ENTRY_INVALID;
	return flash_area_write(entry_ctx->fap,
				FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc),
				buf, FCB_SETTINGS_EXTRA_LEN);
}

static int settings_fcb_append(struct settings_fcb *cf, char *buf, int len)
{
	int rc;
	int i;
	struct fcb_entry loc;
	u8_t entry_status = SETTINGS_ENTRY_VALID;

	for (i = 0; i < 10; i++) {
		rc = fcb_append(&cf->cf_fcb, len + FCB_SETTINGS_EXTRA_LEN,
				&loc);
		if (rc != FCB_ERR_NOSPACE) {
			break;
		}
		settings_fcb_compress(cf);
	}
	if (rc) {
		return -EINVAL;
	}

	rc = flash_area_write(cf->cf_fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc),
			      &entry_status, sizeof(entry_status));
	if (rc) {
		return -EINVAL;
	}

	rc = flash_area_write(cf->cf_fcb.fap,
		      FCB_ENTRY_FA_DATA_OFF(loc) + FCB_SETTINGS_EXTRA_LEN,
		      buf, len);
	if (rc) {
		return -EINVAL;
	}
	return fcb_append_finish(&cf->cf_fcb, &loc);
}

static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	char buf[FCB_SETTINGS_BUFFER_LEN];
	int len;
	int rc;

	if (!name) {
		return -EINVAL;
	}

	/* Invalid entry with the same key */
	rc = fcb_walk(&cf->cf_fcb, 0, settings_fcb_invalid_entry,
		      (char *)name);
	if (rc) {
		return rc;
	}

	len = settings_line_make(buf, sizeof(buf), name, value);
	if (len < 0 || len + 2 > sizeof(buf)) {
		return -EINVAL;
	}
	return settings_fcb_append(cf, buf, len);
}
