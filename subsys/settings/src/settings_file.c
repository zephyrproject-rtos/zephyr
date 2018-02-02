/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <assert.h>
#include <zephyr.h>

#include <fs.h>

#include "settings/settings.h"
#include "settings/settings_file.h"
#include "settings_priv.h"

static int settings_file_load(struct settings_store *cs, load_cb cb,
			      void *cb_arg);
static int settings_file_save(struct settings_store *cs, const char *name,
			      const char *value);

static struct settings_store_itf settings_file_itf = {
	.csi_load = settings_file_load,
	.csi_save = settings_file_save,
};

/*
 * Register a file to be a source of configuration.
 */
int settings_file_src(struct settings_file *cf)
{
	if (!cf->cf_name) {
		return -EINVAL;
	}
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_src_register(&cf->cf_store);

	return 0;
}

int settings_file_dst(struct settings_file *cf)
{
	if (!cf->cf_name) {
		return -EINVAL;
	}
	cf->cf_store.cs_itf = &settings_file_itf;
	settings_dst_register(&cf->cf_store);

	return 0;
}

int settings_getnext_line(struct fs_file_t  *file, char *buf, int blen,
			  off_t *loc)
{
	int rc;
	char *end;

	rc = fs_seek(file, *loc, FS_SEEK_SET);
	if (rc < 0) {
		*loc = 0;
		return -1;
	}

	rc = fs_read(file, buf, blen);
	if (rc <= 0) {
		*loc = 0;
		return -1;
	}

	if (rc == blen) {
		rc--;
	}
	buf[rc] = '\0';

	end = strchr(buf, '\n');
	if (end) {
		*end = '\0';
	} else {
		end = strchr(buf, '\0');
	}
	blen = end - buf;
	*loc += (blen + 1);
	return blen;
}

/*
 * Called to load configuration items. cb must be called for every configuration
 * item found.
 */
static int settings_file_load(struct settings_store *cs, load_cb cb,
			      void *cb_arg)
{
	struct settings_file *cf = (struct settings_file *)cs;
	struct fs_file_t  file;
	off_t loc;
	char tmpbuf[SETTINGS_MAX_NAME_LEN + SETTINGS_MAX_VAL_LEN +
		    SETTINGS_EXTRA_LEN];
	char *name_str;
	char *val_str;
	int rc;
	int lines;

	rc = fs_open(&file, cf->cf_name);
	if (rc != 0) {
		return -EINVAL;
	}

	loc = 0;
	lines = 0;
	while (1) {
		rc = settings_getnext_line(&file, tmpbuf, sizeof(tmpbuf), &loc);
		if (loc == 0) {
			break;
		}
		if (rc < 0) {
			continue;
		}
		rc = settings_line_parse(tmpbuf, &name_str, &val_str);
		if (rc != 0) {
			continue;
		}
		lines++;
		cb(name_str, val_str, cb_arg);
	}
	rc = fs_close(&file);
	cf->cf_lines = lines;

	return rc;
}

static void settings_tmpfile(char *dst, const char *src, char *pfx)
{
	int len;
	int pfx_len;

	len = strlen(src);
	pfx_len = strlen(pfx);
	if (len + pfx_len >= SETTINGS_FILE_NAME_MAX) {
		len = SETTINGS_FILE_NAME_MAX - pfx_len - 1;
	}
	memcpy(dst, src, len);
	memcpy(dst + len, pfx, pfx_len);
	dst[len + pfx_len] = '\0';
}

static int settings_file_create_or_replace(struct fs_file_t *zfp,
					   const char *file_name)
{
	struct fs_dirent entry;

	if (fs_stat(file_name, &entry) == 0) {
		if (entry.type == FS_DIR_ENTRY_FILE) {
			if (fs_unlink(file_name)) {
				return -EIO;
			}
		} else {
			return -EISDIR;
		}
	}

	return fs_open(zfp, file_name);
}
/*
 * Try to compress configuration file by keeping unique names only.
 */
void settings_file_compress(struct settings_file *cf)
{
	int rc;
	struct fs_file_t rf;
	struct fs_file_t wf;
	char tmp_file[SETTINGS_FILE_NAME_MAX];
	char buf1[SETTINGS_MAX_NAME_LEN + SETTINGS_MAX_VAL_LEN +
		  SETTINGS_EXTRA_LEN];
	char buf2[SETTINGS_MAX_NAME_LEN + SETTINGS_MAX_VAL_LEN +
		  SETTINGS_EXTRA_LEN];
	off_t loc1, loc2;
	char *name1, *val1;
	char *name2, *val2;
	int copy;
	int len, len2;
	int lines;

	if (fs_open(&rf, cf->cf_name) != 0) {
		return;
	}

	settings_tmpfile(tmp_file, cf->cf_name, ".cmp");

	if (settings_file_create_or_replace(&wf, tmp_file)) {
		fs_close(&rf);
		return;
	}

	loc1 = 0;
	lines = 0;
	while (1) {
		len = settings_getnext_line(&rf, buf1, sizeof(buf1), &loc1);
		if (loc1 == 0 || len < 0) {
			break;
		}
		rc = settings_line_parse(buf1, &name1, &val1);
		if (rc) {
			continue;
		}
		loc2 = loc1;
		copy = 1;
		while ((len2 = settings_getnext_line(&rf, buf2, sizeof(buf2),
						 &loc2)) > 0) {
			rc = settings_line_parse(buf2, &name2, &val2);
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
		len = settings_line_make(buf2, sizeof(buf2), name1, val1);
		if (len < 0 || len + 2 > sizeof(buf2)) {
			continue;
		}
		buf2[len++] = '\n';
		if (fs_write(&wf, buf2, len) != len) {
			ARG_UNUSED(fs_close(&rf));
			ARG_UNUSED(fs_close(&wf));
			return;
		}
	lines++;
	}

	len = fs_close(&wf);
	len2 = fs_close(&rf);
	if (len == 0 && len2 == 0 && fs_unlink(cf->cf_name) == 0) {
		ARG_UNUSED(fs_rename(tmp_file, cf->cf_name));
		cf->cf_lines = lines;
	/*
	 * XXX at settings_file_load(), look for .cmp if actual file does not
	 * exist.
	 */
	}
}

/*
 * Called to save configuration.
 */
static int settings_file_save(struct settings_store *cs, const char *name,
			      const char *value)
{
	struct settings_file *cf = (struct settings_file *)cs;
	struct fs_file_t  file;
	char buf[SETTINGS_MAX_NAME_LEN + SETTINGS_MAX_VAL_LEN +
		 SETTINGS_EXTRA_LEN];
	int len;
	int rc2;
	int rc;

	if (!name) {
		return -EINVAL;
	}

	if (cf->cf_maxlines && (cf->cf_lines + 1 >= cf->cf_maxlines)) {
		/*
		 * Compress before config file size exceeds
		 * the max number of lines.
		 */
		settings_file_compress(cf);
	}
	len = settings_line_make(buf, sizeof(buf), name, value);
	if (len < 0 || len + 2 > sizeof(buf)) {
		return -EINVAL;
	}
	buf[len++] = '\n';

	/*
	 * Open the file to add this one value.
	 */
	rc = fs_open(&file, cf->cf_name);
	if (rc == 0) {
		rc = fs_seek(&file, 0, FS_SEEK_END);
		if (rc == 0) {
			rc2 = fs_write(&file, buf, len);
			if (rc2 == len) {
				cf->cf_lines++;
			}
		}

		rc2 = fs_close(&file);
		if (rc == 0) {
			rc = rc2;
		}
	}

	return rc;
}
