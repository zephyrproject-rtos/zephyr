/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include "settings_nvs.h"
#include "settings_priv.h"

struct settings_nvs_read_fn_arg {
	struct nvs_fs *fs;
	u16_t id;
};

static int settings_nvs_load(struct settings_store *cs);
static int settings_nvs_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len);

static struct settings_store_itf settings_nvs_itf = {
	.csi_load = settings_nvs_load,
	.csi_save = settings_nvs_save,
};

static ssize_t settings_nvs_read_fn(void *data, size_t len, void *read_fn_arg)
{
	struct settings_nvs_read_fn_arg *rd_fn_arg;

	rd_fn_arg = (struct settings_nvs_read_fn_arg *)read_fn_arg;

	return nvs_read(rd_fn_arg->fs, rd_fn_arg->id, data, len);
}

int settings_nvs_src(struct settings_nvs *cf)
{
	cf->cf_store.cs_itf = &settings_nvs_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_nvs_dst(struct settings_nvs *cf)
{
	cf->cf_store.cs_itf = &settings_nvs_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

static int settings_nvs_load(struct settings_store *cs)
{
	struct settings_nvs *cf = (struct settings_nvs *)cs;
	struct settings_nvs_read_fn_arg read_fn_arg;
	struct settings_handler *ch;
	char name[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	char buf, *name_argv[SETTINGS_MAX_DIR_DEPTH];
	int name_argc;
	ssize_t rc1, rc2;
	u16_t name_id = NVS_NAMECNT_ID;

	/* In nvs the settings data is stored as:
	 *	a. name at id starting from NVS_NAMECNT_ID (0 not included)
	 *	b. value at id of name + NVS_NAME_ID_OFFSET
	 * Deleted records will not be found, only the last record will be
	 * read.
	 */

	name_id = cf->last_name_id + 1;

	while (1) {

		name_id--;
		if (name_id == NVS_NAMECNT_ID) {
			break;
		}
		rc1 = nvs_read(&cf->cf_nvs, name_id, &name, sizeof(name));
		rc2 = nvs_read(&cf->cf_nvs, name_id + NVS_NAME_ID_OFFSET,
			       &buf, sizeof(buf));

		if ((rc1 <= 0) && (rc2 <= 0)) {
			continue;
		}

		if ((rc1 <= 0) || (rc2 <= 0)) {
			/* NVS is dirty, clean it */
			if (name_id == cf->last_name_id) {
				cf->last_name_id--;
				nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID,
					  &cf->last_name_id, sizeof(u16_t));
			}
			nvs_delete(&cf->cf_nvs, name_id);
			nvs_delete(&cf->cf_nvs, name_id + NVS_NAME_ID_OFFSET);
			continue;
		}

		/* Found a name, this might not include a trailing \0 */
		name[rc1] = '\0';
		ch = settings_parse_and_lookup(name, &name_argc, name_argv);
		if (!ch) {
			continue;
		}

		/* Found a settings_handler for the name */
		read_fn_arg.fs = &cf->cf_nvs;
		read_fn_arg.id = name_id + NVS_NAME_ID_OFFSET;
		ch->h_set(name_argc - 1, &name_argv[1], rc2,
			  settings_nvs_read_fn, (void *) &read_fn_arg);
	}
	return 0;
}

/* ::csi_save implementation */
static int settings_nvs_save(struct settings_store *cs, const char *name,
			     const char *value, size_t val_len)
{
	struct settings_nvs *cf = (struct settings_nvs *)cs;
	char rdname[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	u16_t name_id, write_name_id;
	bool delete, write_name;
	int rc = 0;

	if (!name) {
		return -EINVAL;
	}

	write_name = true;
	/* Find out if we are doing a delete */
	delete = false;
	if ((value == NULL) || (val_len == 0)) {
		delete = true;
	}

	name_id = cf->last_name_id + 1;
	write_name_id = cf->last_name_id + 1;

	while (1) {

		name_id--;
		if (name_id == NVS_NAMECNT_ID) {
			break;
		}

		rc = nvs_read(&cf->cf_nvs, name_id, &rdname, sizeof(rdname));

		if (rc < 0) {
			/* Error or entry not found */
			if (rc == -ENOENT) {
				write_name_id = name_id;
			}
			continue;
		}

		rdname[rc] = '\0';
		if (strncmp(name, rdname, strlen(name))) {
			continue;
		}

		if ((delete) && (name_id == cf->last_name_id)) {
			cf->last_name_id--;
			rc = nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID,
				       &cf->last_name_id, sizeof(u16_t));
		}

		if (delete) {
			rc = nvs_delete(&cf->cf_nvs, name_id);
			rc = nvs_delete(&cf->cf_nvs, name_id +
					NVS_NAME_ID_OFFSET);
			continue;
		}
		write_name_id = name_id;
		write_name = false;
		break;
	}

	if (delete) {
		return rc;
	}

	/* write the value */
	rc = nvs_write(&cf->cf_nvs, write_name_id + NVS_NAME_ID_OFFSET,
		       value, val_len);

	/* write the name if required */
	if (write_name) {
		rc = nvs_write(&cf->cf_nvs, write_name_id, name, strlen(name));
	}

	/* update the last_name_id and write to flash if required*/
	if (write_name_id > cf->last_name_id) {
		cf->last_name_id = write_name_id;
		rc = nvs_write(&cf->cf_nvs, NVS_NAMECNT_ID, &cf->last_name_id,
			       sizeof(u16_t));
	}

	return rc;
}
