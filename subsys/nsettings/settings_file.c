/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <assert.h>
#include <zephyr.h>


#include "settings_file.h"
#include "settings_priv.h"

struct settings_file_read_fn_arg {
	void *stor_ctx;
	off_t seek; /* offset of value within the file */
};

struct file_entry {
	off_t off_name;   /* offset of the start of name in file */
	size_t len_name;  /* name len including '=' */
	off_t off_value;  /* offset of the start of value in file */
	size_t len_value; /* value length */
};

static int settings_file_load(struct settings_store *cs);
static int settings_file_save(struct settings_store *cs, const char *name,
			      const char *value, size_t val_len);

static struct settings_store_itf settings_file_itf = {
	.csi_load = settings_file_load,
	.csi_save = settings_file_save,
};

static ssize_t settings_file_read_fn(void *data, size_t len, void *read_fn_arg)
{
	struct settings_file_read_fn_arg *rd_fn_arg = read_fn_arg;
	struct fs_file_t *file = rd_fn_arg->stor_ctx;
	int rc;

	rc = fs_seek(file, rd_fn_arg->seek, FS_SEEK_SET);
	if (rc < 0) {
		return rc;
	}

	rc = fs_read(file, data, len);
	return rc;
}
/*
 * Register a file to be a source of configuration.
 */
int settings_file_src(struct settings_file *cf)
{
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_file_dst(struct settings_file *cf)
{
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

/*
 * Called to advance to a next entry in a file
 *
 * Items are stored as: string - binary where
 *   string: "name="
 *   binary: len (u16_t) and value (void*)
 */
static int file_get_next_entry(struct fs_file_t *file, struct file_entry *entry)
{
	char buf[16];
	size_t len, add, val_len;
	int rc;

	/* calculate offset of next entry */
	entry->off_name = entry->off_value + entry->len_value;
	entry->len_name = 0;
	entry->off_value = entry->off_name;
	entry->len_value = 0;

	rc = fs_seek(file, 0, FS_SEEK_END);
	if (rc) {
		return rc;
	}
	if (fs_tell(file) <= entry->off_name) {
		/* At the end of the file */
		rc = -ENOENT;
		return rc;
	}
	/* advance to the next entry */
	rc = fs_seek(file, entry->off_name, FS_SEEK_SET);
	if (rc) {
		return rc;
	}

	while (1) {
		len = sizeof(buf);
		rc = fs_read(file, buf, len);
		if (rc < 0) {
			goto end;
		}
		add = strchr(buf, '=') - buf;
		entry->len_name += add;
		if (add < sizeof(buf)) {
			break;
		}
	}

	entry->off_value += entry->len_name + 1;

	rc = fs_seek(file, entry->off_value, FS_SEEK_SET);
	if (rc < 0) {
		goto end;
	}

	rc = fs_read(file, &val_len, sizeof(u16_t));
	if (rc < 0) {
		goto end;
	}
	entry->off_value += sizeof(u16_t);
	entry->len_value = val_len;
	rc = 0;

end:
	/* reset file to start of entry */
	fs_seek(file, entry->off_name, FS_SEEK_SET);
	return rc;
}
/*
 * Called to load settings items. h_set() will be called for every item that
 * has not been deleted.
 */

static int settings_file_load(struct settings_store *cs)
{
	struct settings_file *cf = (struct settings_file *)cs;
	struct settings_file_read_fn_arg read_fn_arg;
	struct settings_handler *ch;
	char fname[SETTINGS_FILE_NAME_MAX+1];
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN + 1];
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct fs_file_t  file;
	struct file_entry loc1, loc2;
	bool dup_name;
	int rc, lines, name_argc;

	lines = 0;

	if (cf->toggle) {
		strcpy(fname, cf->cf_name);
		strcat(fname, "1");
	} else {
		strcpy(fname, cf->cf_name);
		strcat(fname, "0");
	}
	rc = fs_open(&file, fname);
	if (rc < 0) {
		return -EINVAL;
	}

	loc1.off_name = 0;
	loc1.len_name = 0;
	loc1.off_value = 0;
	loc1.len_value = 0;

	while (file_get_next_entry(&file, &loc1) == 0) {
		lines++;
		dup_name = false;

		rc = fs_read(&file, name1, loc1.len_name);
		if (rc < 0) {
			continue;
		}
		name1[loc1.len_name] = '\0';

		loc2 = loc1;

		while (file_get_next_entry(&file, &loc2) == 0) {
			if (loc1.len_name != loc2.len_name) {
				continue;
			}
			rc = fs_read(&file, name2, loc2.len_name);
			if (rc < 0) {
				continue;
			}
			name2[loc2.len_name] = '\0';
			if (!memcmp(name1, name2, loc2.len_name)) {
				dup_name = true;
				break;
			}
		}

		if ((dup_name) || (loc1.len_value == 0)) {
			continue;
		}

		/* Last entry of specific name found */
		read_fn_arg.stor_ctx = &file;
		read_fn_arg.seek = loc1.off_value;

		ch = settings_parse_and_lookup(name1, &name_argc, name_argv);
		if (!ch) {
			continue;
		}

		ch->h_set(name_argc - 1, &name_argv[1], loc1.len_value,
			  settings_file_read_fn, (void *) &read_fn_arg);
	}

	rc = fs_close(&file);

	cf->cf_lines = lines;

	return rc;
}

/*
 * Try to compress configuration file by keeping unique names only.
 */
int settings_file_compress(struct settings_file *cf)
{
	int rc, rcf, rcw;
	struct fs_file_t rf;
	struct fs_file_t wf;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char fname_0[SETTINGS_FILE_NAME_MAX+1];
	char fname_1[SETTINGS_FILE_NAME_MAX+1];
	char buf[16];
	size_t entry_len;
	struct file_entry loc1, loc2;
	bool dup_name = false;
	off_t rd_off;
	int lines;


	strcpy(fname_0, cf->cf_name);
	strcat(fname_0, "0");
	strcpy(fname_1, cf->cf_name);
	strcat(fname_1, "1");

	if (cf->toggle) {
		rcf = fs_open(&rf, fname_1);
		(void)fs_unlink(fname_0);
		rcw = fs_open(&wf, fname_0);
	} else {
		rcf = fs_open(&rf, fname_0);
		(void)fs_unlink(fname_1);
		rcw = fs_open(&wf, fname_1);
	}
	if (rcf) {
		return rcf;
	}
	if (rcw) {
		return rcw;
	}


	lines = 0;
	loc1.off_name = 0;
	loc1.len_name = 0;
	loc1.off_value = 0;
	loc1.len_value = 0;

	while (file_get_next_entry(&rf, &loc1) == 0) {
		dup_name = false;
		rc = fs_read(&rf, name1, loc1.len_name);
		if (rc < 0) {
			continue;
		}
		name1[loc1.len_name] = '\0';

		loc2 = loc1;

		while (file_get_next_entry(&rf, &loc2) == 0) {
			if (loc1.len_name != loc2.len_name) {
				continue;
			}
			rc = fs_read(&rf, name2, loc2.len_name);
			if (rc < 0) {
				continue;
			}
			name2[loc2.len_name] = '\0';
			if (!memcmp(name1, name2, loc2.len_name)) {
				dup_name = true;
				break;
			}
		}

		if ((dup_name) || (loc1.len_value == 0)) {
			continue;
		}
		/* Copy needed */
		rd_off = loc1.off_name;
		entry_len = loc1.len_name + 1 + sizeof(u16_t) + loc1.len_value;
		while (entry_len) {
			size_t cp_len = min(sizeof(buf), entry_len);

			fs_seek(&rf, rd_off, FS_SEEK_SET);
			rc = fs_read(&rf, buf, cp_len);
			fs_seek(&wf, 0, FS_SEEK_END);
			rc = fs_write(&wf, buf, cp_len);
			if (rc < 0) {
				/* corrupted file */
				goto end_rollback;
			}
			entry_len -= cp_len;
			rd_off += cp_len;
			lines++;
		}
	}

	rc = fs_close(&wf);
	if (rc == 0) {
		cf->cf_lines = lines;
		cf->toggle = 1 - cf->toggle;
	}
	rc = fs_close(&rf);
	if (rc == 0) {
		if (cf->toggle) {
			/* toggle has just changed */
			(void)fs_unlink(fname_0);
		} else {
			(void)fs_unlink(fname_1);
		}
	}
	return rc;

end_rollback:
	(void)fs_close(&wf);
	(void)fs_close(&rf);
	return -EIO;

}

/*
 * Called to save configuration. Avoids writing the same data twice
 */
static int settings_file_save(struct settings_store *cs, const char *name,
			      const char *value, size_t val_len)
{
	struct settings_file *cf = (struct settings_file *)cs;
	struct file_entry loc1, loc2;
	struct fs_file_t  file;
	char name1[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char name2[SETTINGS_MAX_NAME_LEN + SETTINGS_EXTRA_LEN];
	char fname[SETTINGS_FILE_NAME_MAX+1];
	int rc;
	off_t off;
	size_t w_size, len;
	bool delete, grp_delete, save, eofs_reached;
	char w_buf[16];
	u16_t val_len_w;

	if (!name) {
		return -EINVAL;
	}

	if (cf->toggle) {
		strcpy(fname, cf->cf_name);
		strcat(fname, "1");
	} else {
		strcpy(fname, cf->cf_name);
		strcat(fname, "0");
	}
	rc = fs_open(&file, fname);
	if (rc < 0) {
		return rc;
	}
	/* Find out if we are doing a delete */
	delete = false;
	if ((value == NULL) || (val_len == 0)) {
		delete = true;
	}

	grp_delete = false;

	loc1.off_name = 0;
	loc1.len_name = 0;
	loc1.off_value = 0;
	loc1.len_value = 0;

	rc = 0;

	while (1) {

		if (file_get_next_entry(&file, &loc1) == 0) {

			save = true;

			rc = fs_read(&file, name1, loc1.len_name);
			if (rc < 0) {
				continue;
			}
			name1[loc1.len_name] = '\0';

			if (loc1.len_value == 0 && delete) {
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

			while (file_get_next_entry(&file, &loc2) == 0) {

				if (loc1.len_name != loc2.len_name) {
					continue;
				}
				rc = fs_read(&file, name2, loc2.len_name);
				if (rc < 0) {
					continue;
				}
				name2[loc2.len_name] = '\0';
				if (!memcmp(name1, name2, loc2.len_name)) {
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

			/* The value only needs to be compared when the length
			 * is equal, otherwise just continue with save
			 */

			if (loc1.len_value == val_len) {

				len = val_len;
				off = loc1.off_value;
				while (len) {
					save = false;
					w_size = min(len, sizeof(w_buf));
					fs_seek(&file, off, FS_SEEK_SET);
					rc = fs_read(&file, w_buf, w_size);
					if (rc < 0) {
						return rc;
					}
					if (memcmp(w_buf, value + val_len - len,
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
			 * to use name1, if not we can use name. Copy name to
			 * name1 and use name1.
			 */
			if (!grp_delete) {
				strcpy(name1, name);
			}
			fs_seek(&file, 0, FS_SEEK_END);
			len = strlen(name1);
			name1[len] = '=';
			rc = fs_write(&file, name1, len + 1);
			if (rc < 0) {
				return rc;
			}
			val_len_w = (u16_t) val_len;
			rc = fs_write(&file, &val_len_w,
				      sizeof(u16_t));
			if (rc < 0) {
				return rc;
			}
			if (val_len) {
				rc = fs_write(&file, value, val_len);
				if (rc < 0) {
					return rc;
				}
			}
			cf->cf_lines++;
		}

		if ((!grp_delete) || (eofs_reached)) {
			break;
		}
	}

	rc = fs_close(&file);

	if (cf->cf_maxlines && (cf->cf_lines >= cf->cf_maxlines)) {
		/*
		 * Compress before config file size exceeds
		 * the max number of lines.
		 */
		rc = settings_file_compress(cf);
	}
	return rc;
}
