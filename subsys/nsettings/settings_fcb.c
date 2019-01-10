/* Copyright (c) 2019 Laczen
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcb.h>
#include <string.h>

#include <nsettings/settings.h>
#include "settings_fcb.h"
#include "settings_priv.h"

<<<<<<< HEAD
#define SETTINGS_FCB_VERS		1

=======
>>>>>>> subsys/settings: Reworked settings module
struct settings_fcb_read_fn_arg {
	struct fcb_entry_ctx *entry_ctx;
	off_t off;
};

static int settings_fcb_load(struct settings_store *cs);
static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len);

static struct settings_store_itf settings_fcb_itf = {
	.csi_load = settings_fcb_load,
	.csi_save = settings_fcb_save,
};

<<<<<<< HEAD
int settings_fcb_src(struct settings_fcb *cf)
{
	int rc;

	cf->cf_fcb.f_version = SETTINGS_FCB_VERS;
	cf->cf_fcb.f_scratch_cnt = 1;

	while (1) {
		rc = fcb_init(CONFIG_NSETTINGS_FCB_FLASH_AREA, &cf->cf_fcb);
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
=======
int settings_fcb_dst(struct settings_fcb *cf)
{
	cf->cf_store.cs_itf = &settings_fcb_itf;
	settings_dst_register(&cf->cf_store);
>>>>>>> subsys/settings: Reworked settings module

	return 0;
}

<<<<<<< HEAD
int settings_fcb_dst(struct settings_fcb *cf)
{
	cf->cf_store.cs_itf = &settings_fcb_itf;
	settings_dst_register(&cf->cf_store);
=======
int settings_fcb_src(struct settings_fcb *cf)
{
	cf->cf_store.cs_itf = &settings_fcb_itf;
	settings_src_register(&cf->cf_store);
>>>>>>> subsys/settings: Reworked settings module

	return 0;
}

static int settings_fcb_read(struct fcb_entry_ctx *entry_ctx, off_t off,
			     char *buf, size_t *len)
{
	if (off >= entry_ctx->loc.fe_data_len) {
		return -EINVAL;
	}

	if ((off + *len) > entry_ctx->loc.fe_data_len) {
		*len = entry_ctx->loc.fe_data_len - off;
	}

	return flash_area_read(entry_ctx->fap,
			       FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc) + off,
			       buf, *len);
}

static ssize_t settings_fcb_read_fn(void *data, size_t len, void *read_fn_arg)
{
	int rc, len_read;
	struct settings_fcb_read_fn_arg *rd_fn_arg;

	len_read = len;
	rd_fn_arg = (struct settings_fcb_read_fn_arg *)read_fn_arg;
	rc = settings_fcb_read(rd_fn_arg->entry_ctx, rd_fn_arg->off,
			       data, &len_read);
	if (rc < 0) {
		return rc;
	}
<<<<<<< HEAD
=======

>>>>>>> subsys/settings: Reworked settings module
	return len_read;
}

static int settings_fcb_load_cb(struct fcb_entry_ctx *entry_ctx)
{
	struct settings_fcb_read_fn_arg read_fn_arg;
	struct settings_handler *ch;
	char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	int rc, name_argc;

	size_t len_read, len_data;

	len_read = min(SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1,
				   entry_ctx->loc.fe_data_len);

	rc = settings_fcb_read(entry_ctx, 0, name, &len_read);
	if (rc) {
		return 0;
	}

	len_read = strchr(name, '=') - name;
	name[len_read] = '\0';

	len_data = entry_ctx->loc.fe_data_len - (len_read + 1);
	read_fn_arg.entry_ctx = entry_ctx;
	read_fn_arg.off = len_read + 1;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_set(name_argc - 1, &name_argv[1], len_data,
			 settings_fcb_read_fn, (void *) &read_fn_arg);
}

static int settings_fcb_load(struct settings_store *cs)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	struct fcb_entry_ctx loc;
	int rc;

	loc.fap = cf->cf_fcb.fap;
	loc.loc.fe_sector = NULL;
	loc.loc.fe_elem_off = 0;

	while (fcb_getnext(&cf->cf_fcb, &loc.loc) == 0) {
		rc = settings_fcb_load_cb(&loc);
	}
	return 0;
}

static int settings_fcb_write(struct fcb_entry_ctx *entry_ctx, off_t off,
			      char const *buf, size_t len)
{
	return flash_area_write(entry_ctx->fap,
				FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc) + off,
				buf, len);
}

static void settings_fcb_compress(struct settings_fcb *cf)
{
	int rc;
	struct fcb_entry_ctx loc1;
	struct fcb_entry_ctx loc2;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char buf[16];
	int copy;
	size_t chunk_size, len1_read, len2_read;
	off_t src_off, dst_off;
	size_t len;
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

		len1_read = min(SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1,
				loc1.loc.fe_data_len);

		rc = settings_fcb_read(&loc1, 0, name1, &len1_read);

		if (rc) {
			continue;
<<<<<<< HEAD
=======
		}
>>>>>>> subsys/settings: Reworked settings module

		len1_read = strchr(name1, '=') - name1;

		if (len1_read + 1 == loc1.loc.fe_data_len) {
			/* Lack of a value so the record is a deletion-record */
			/* No sense to copy empty entry from */
			/* the oldest sector */
			continue;
		}

		loc2 = loc1;
		copy = 1;

		while (fcb_getnext(&cf->cf_fcb, &loc2.loc) == 0) {

			len2_read = min(SETTINGS_MAX_NAME_LEN +
					SETTINGS_EXTRA_LEN + 1,
					loc2.loc.fe_data_len);

			rc = settings_fcb_read(&loc2, 0, name2, &len2_read);

			if (rc) {
				continue;
			}

			len2_read = strchr(name2, '=') - name2;

			if ((len1_read == len2_read) &&
			    !memcmp(name1, name2, len1_read)) {
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

		len = loc1.loc.fe_data_len;
		src_off = 0;
		dst_off = 0;

		while (len) {
			chunk_size = min(len, sizeof(buf));

			rc = settings_fcb_read(&loc1, src_off, buf,
					       &chunk_size);
			if (rc) {
				break;
			}

			rc = settings_fcb_write(&loc2, dst_off, buf,
						chunk_size);
			if (rc) {
				break;
			}

			src_off += chunk_size;
			dst_off += chunk_size;
			len -= chunk_size;
		}

		if (rc) {
			continue;
		}
		rc = fcb_append_finish(&cf->cf_fcb, &loc2.loc);
		__ASSERT(rc == 0, "Failed to finish fcb_append.\n");
	}
	rc = fcb_rotate(&cf->cf_fcb);

	__ASSERT(rc == 0, "Failed to fcb rotate.\n");
}

static int settings_fcb_save_record(struct settings_fcb *cf,
				    const char *name, const char *value,
				    size_t val_len)
{
	struct fcb_entry_ctx loc;
	int rc, i;
	size_t len, rem, w_size, add;
	off_t off = 0U;
	bool done;
	char w_buf[16];
	u8_t wbs;

	wbs = flash_area_align(cf->cf_fcb.fap);
	len = strlen(name) + 1 + val_len;
	rem = len % wbs;
	if (rem) {
		 /*additional \0 for meet flash alignment */
		len += wbs - rem;
	}

<<<<<<< HEAD
	for (i = 0; i < CONFIG_NSETTINGS_FCB_NUM_AREAS; i++) {
=======
	for (i = 0; i < cf->cf_fcb.f_sector_cnt; i++) {
>>>>>>> subsys/settings: Reworked settings module
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
	rem = strlen(name);

	w_size = rem - rem % wbs;
	rem %= wbs;
	rc = settings_fcb_write(&loc, off, name, w_size);
	if (rc) {
		return -EIO;
	}
	off += w_size;
	w_size = rem;

	if (rem) {
		memcpy(w_buf, name + strlen(name) - rem, rem);
	}

	w_buf[rem] = '=';
	w_size++;

	rem = val_len;
	done = false;

	while (1) {
		while (w_size < sizeof(w_buf)) {
			if (rem) {
				add = min(rem, sizeof(w_buf) - w_size);
				memcpy(&w_buf[w_size], value, add);
				value += add;
				rem -= add;
				w_size += add;
			} else {
				add = (w_size) % wbs;
				if (add) {
					add = wbs - add;
					memset(&w_buf[w_size], '\0', add);
					w_size += add;
				}
				done = true;
				break;
			}
		}
		rc = settings_fcb_write(&loc, off, w_buf, w_size);
		if (rc) {
			return -EIO;
		}

		if (done) {
			break;
		}
		off += w_size;
		w_size = 0;
	}

	if (rc != -EIO) {
		i = fcb_append_finish(&cf->cf_fcb, &loc.loc);
		if (!rc) {
			rc = i;
		}
	}
	return rc;
}

/* ::csi_save implementation */
static int settings_fcb_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len)
{
	struct settings_fcb *cf = (struct settings_fcb *)cs;
	struct fcb_entry_ctx loc1, loc2;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	int len, rc;
	off_t off;
	size_t w_size, len1_read, len2_read;
	bool delete, grp_delete, save, eofs_reached;
	char w_buf[16];

	if (!name) {
		return -EINVAL;
	}

	/* Find out if we are doing a delete */
	delete = false;
	if ((value == NULL) || (val_len == 0)) {
		delete = true;
	}

	grp_delete = false;

	loc1.fap = cf->cf_fcb.fap;
	loc1.loc.fe_sector = NULL;
	loc1.loc.fe_elem_off = 0;

	rc = 0;

	while (1) {

		if (fcb_getnext(&cf->cf_fcb, &loc1.loc) == 0) {

			save = true;

			len1_read = min(SETTINGS_MAX_NAME_LEN +
					SETTINGS_EXTRA_LEN + 1,
					loc1.loc.fe_data_len);

			rc = settings_fcb_read(&loc1, 0, name1, &len1_read);
			if (rc) {
				continue;
			}

			len1_read = strchr(name1, '=') - name1;
			name1[len1_read] = '\0';

			if ((len1_read + 1 == loc1.loc.fe_data_len) && delete) {
				/* Lack of a value so the record is a
				 * deletion-record
				 * No sense to delete a deleted record again
				 */
				continue;
			}

			if (strncmp(name, name1, strlen(name))) {
				/* name1 is different from name or does not
				 * contain name, nothing to do
				 */
				continue;
			}

			if (delete) {
				/* doing a delete and found a name match, this
				 * is part of a group delete if name is shorter
				 * than name1
				 */
				if (strlen(name) < strlen(name1)) {
					grp_delete = true;
				}
			}

			/* A possible candidate for saving was found
			 * Lets see if there is another candidate with the same
			 * name further in the fs
			 */

			loc2 = loc1;

			while (fcb_getnext(&cf->cf_fcb, &loc2.loc) == 0) {

				len2_read = min(SETTINGS_MAX_NAME_LEN +
						SETTINGS_EXTRA_LEN + 1,
						loc2.loc.fe_data_len);

				rc = settings_fcb_read(&loc2, 0, name2,
						       &len2_read);

				if (rc) {
					continue;
				}

				len2_read = strchr(name2, '=') - name2;
				name2[len2_read] = '\0';

				if ((len1_read == len2_read) &&
				    !memcmp(name1, name2, len1_read)) {
					save = false;
					break;
				}
			}

			if (!save) {
				continue;
			}

			/* The last entry that matches the name has been found,
			 * data should only be saved if the value is different,
			 * loc1 contains the old data
			 */
			len = loc1.loc.fe_data_len - len1_read - 1;

			/* The value only needs to be compared when the length
			 * is equal, otherwise just continue with save
			 */

			if (len == val_len) {

				off = len1_read;
				while (len) {
					save = false;
					w_size = min(len, sizeof(w_buf));
					rc = settings_fcb_read(&loc1, off,
						w_buf, &w_size);
					if (rc) {
						return -EIO;
					}
					if (memcmp(&w_buf[w_size],
						value + val_len - len,
						w_size)) {
						save = true;
						break;
					}
					len -= w_size;
					off += w_size;
				}
			}
			eofs_reached = false;
		} else {
			save = true;
			if (grp_delete) {
				save = false;
			}
			eofs_reached = true;
		}

		if (save) {

			/* Storage is needed, when it is a group delete we have
			 * to use name1, if not we can use name.
			 */

			if (grp_delete) {
				rc = settings_fcb_save_record(cf, name1, value,
							      val_len);
				if (rc) {
					return -EIO;
				}
			} else {
				rc = settings_fcb_save_record(cf, name, value,
							      val_len);
				if (rc) {
					return -EIO;
				}
			}
		}

		if ((!grp_delete) || (eofs_reached)) {
			break;
		}
	}
	return rc;
}
<<<<<<< HEAD

void settings_mount_fcb_backend(struct settings_fcb *cf)
{
}
=======
>>>>>>> subsys/settings: Reworked settings module
