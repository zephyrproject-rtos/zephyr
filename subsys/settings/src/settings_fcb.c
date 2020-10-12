/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <fs/fcb.h>
#include <string.h>

#include "settings/settings.h"
#include "settings/settings_fcb.h"
#include "settings_priv.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(settings, CONFIG_SETTINGS_LOG_LEVEL);

#define SETTINGS_FCB_VERS		1

int settings_backend_init(void);
void settings_mount_fcb_backend(struct settings_fcb *cf);

static int settings_fcb_load(struct settings_store *cs,
			     const struct settings_load_arg *arg);
static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len);

static const struct settings_store_itf settings_fcb_itf = {
	.csi_load = settings_fcb_load,
	.csi_save = settings_fcb_save,
};

int settings_fcb_src(struct settings_fcb *cf)
{
	int rc;

	cf->cf_fcb.f_version = SETTINGS_FCB_VERS;
	cf->cf_fcb.f_scratch_cnt = 1;

	while (1) {
		rc = fcb_init(FLASH_AREA_ID(storage), &cf->cf_fcb);
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

/**
 * @brief Check if there is any duplicate of the current setting
 *
 * This function checks if there is any duplicated data further in the buffer.
 *
 * @param cf        FCB handler
 * @param entry_ctx Current entry context
 * @param name      The name of the current entry
 *
 * @retval false No duplicates found
 * @retval true  Duplicate found
 */
static bool settings_fcb_check_duplicate(struct settings_fcb *cf,
					const struct fcb_entry_ctx *entry_ctx,
					const char * const name)
{
	struct fcb_entry_ctx entry2_ctx = *entry_ctx;

	while (fcb_getnext(&cf->cf_fcb, &entry2_ctx.loc) == 0) {
		char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
		size_t name2_len;

		if (settings_line_name_read(name2, sizeof(name2), &name2_len,
					    &entry2_ctx)) {
			LOG_ERR("failed to load line");
			continue;
		}
		name2[name2_len] = '\0';
		if (!strcmp(name, name2)) {
			return true;
		}
	}
	return false;
}

static int read_entry_len(const struct fcb_entry_ctx *entry_ctx, off_t off)
{
	if (off >= entry_ctx->loc.fe_data_len) {
		return 0;
	}
	return entry_ctx->loc.fe_data_len - off;
}

static int settings_fcb_load_priv(struct settings_store *cs,
				  line_load_cb cb,
				  void *cb_arg,
				  bool filter_duplicates)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	struct fcb_entry_ctx entry_ctx = {
		{.fe_sector = NULL, .fe_elem_off = 0},
		.fap = cf->cf_fcb.fap
	};
	int rc;

	while ((rc = fcb_getnext(&cf->cf_fcb, &entry_ctx.loc)) == 0) {
		char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
		size_t name_len;
		int rc;
		bool pass_entry = true;

		rc = settings_line_name_read(name, sizeof(name), &name_len,
					     (void *)&entry_ctx);
		if (rc) {
			LOG_ERR("Failed to load line name: %d", rc);
			continue;
		}
		name[name_len] = '\0';

		if (filter_duplicates &&
		    (!read_entry_len(&entry_ctx, name_len+1) ||
		     settings_fcb_check_duplicate(cf, &entry_ctx, name))) {
			pass_entry = false;
		}
		/*name, val-read_cb-ctx, val-off*/
		/* take into account '=' separator after the name */
		if (pass_entry) {
			cb(name, &entry_ctx, name_len + 1, cb_arg);
		}
	}
	if (rc == -ENOTSUP) {
		rc = 0;
	}
	return 0;
}

static int settings_fcb_load(struct settings_store *cs,
			     const struct settings_load_arg *arg)
{
	return settings_fcb_load_priv(
		cs,
		settings_line_load_cb,
		(void *)arg,
		true);
}

static int read_handler(void *ctx, off_t off, char *buf, size_t *len)
{
	struct fcb_entry_ctx *entry_ctx = ctx;

	if (off >= entry_ctx->loc.fe_data_len) {
		*len = 0;
		return 0;
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
	uint8_t rbs;

	rc = fcb_append_to_scratch(&cf->cf_fcb);
	if (rc) {
		return; /* XXX */
	}

	rbs = flash_area_align(cf->cf_fcb.fap);

	loc1.fap = cf->cf_fcb.fap;

	loc1.loc.fe_sector = NULL;
	loc1.loc.fe_elem_off = 0U;

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

		rc = settings_line_entry_copy(&loc2, 0, &loc1, 0,
					      loc1.loc.fe_data_len);
		if (rc) {
			continue;
		}
		rc = fcb_append_finish(&cf->cf_fcb, &loc2.loc);

		if (rc != 0) {
			LOG_ERR("Failed to finish fcb_append (%d)", rc);
		}
	}
	rc = fcb_rotate(&cf->cf_fcb);

	if (rc != 0) {
		LOG_ERR("Failed to fcb rotate (%d)", rc);
	}
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
static int settings_fcb_save_priv(struct settings_store *cs, const char *name,
				  const char *value, size_t val_len)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	struct fcb_entry_ctx loc;
	int len;
	int rc = -EINVAL;
	int i;
	uint8_t wbs;

	if (!name) {
		return -EINVAL;
	}

	wbs = cf->cf_fcb.f_align;
	len = settings_line_len_calc(name, val_len);

	for (i = 0; i < cf->cf_fcb.f_sector_cnt; i++) {
		rc = fcb_append(&cf->cf_fcb, len, &loc.loc);
		if (rc != -ENOSPC) {
			break;
		}

		/* FCB can compress up to cf->cf_fcb.f_sector_cnt - 1 times. */
		if (i < (cf->cf_fcb.f_sector_cnt - 1)) {
			settings_fcb_compress(cf);
		}
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

static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len)
{
	struct settings_line_dup_check_arg cdca;

	if (val_len > 0 && value == NULL) {
		return -EINVAL;
	}

	/*
	 * Check if we're writing the same value again.
	 */
	cdca.name = name;
	cdca.val = (char *)value;
	cdca.is_dup = 0;
	cdca.val_len = val_len;
	settings_fcb_load_priv(cs, settings_line_dup_check_cb, &cdca, false);
	if (cdca.is_dup == 1) {
		return 0;
	}
	return settings_fcb_save_priv(cs, name, (char *)value, val_len);
}

void settings_mount_fcb_backend(struct settings_fcb *cf)
{
	uint8_t rbs;

	rbs = cf->cf_fcb.f_align;

	settings_line_io_init(read_handler, write_handler, get_len_cb, rbs);
}

int settings_backend_init(void)
{
	static struct flash_sector
		settings_fcb_area[CONFIG_SETTINGS_FCB_NUM_AREAS + 1];
	static struct settings_fcb config_init_settings_fcb = {
		.cf_fcb.f_magic = CONFIG_SETTINGS_FCB_MAGIC,
		.cf_fcb.f_sectors = settings_fcb_area,
	};
	uint32_t cnt = sizeof(settings_fcb_area) /
		    sizeof(settings_fcb_area[0]);
	int rc;
	const struct flash_area *fap;

	rc = flash_area_get_sectors(FLASH_AREA_ID(storage), &cnt,
				    settings_fcb_area);
	if (rc == -ENODEV) {
		return rc;
	} else if (rc != 0 && rc != -ENOMEM) {
		k_panic();
	}

	config_init_settings_fcb.cf_fcb.f_sector_cnt = cnt;

	rc = settings_fcb_src(&config_init_settings_fcb);

	if (rc != 0) {
		rc = flash_area_open(FLASH_AREA_ID(storage), &fap);

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
