/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <misc/printk.h>

#include <zephyr.h>
#include <init.h>
#include <fs.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/storage.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_HCI_CORE)
#include "common/log.h"

#define STORAGE_ROOT          "/bt"

/* Required file name length for full storage support. If the maximum
 * file name length supported by the chosen file system is less than
 * this value, then only local keys are supported (/bt/abcd).
 */
#define STORAGE_FILE_NAME_LEN  13

#if MAX_FILE_NAME >= STORAGE_FILE_NAME_LEN
/* /bt/aabbccddeeff0/abcd */
#define STORAGE_PATH_MAX       23
#else
/* /bt/abcd */
#define STORAGE_PATH_MAX       9
#endif

enum storage_access {
	STORAGE_READ,
	STORAGE_WRITE
};

static int storage_open(const bt_addr_le_t *addr, u16_t key,
			enum storage_access access, fs_file_t *file)
{
	char path[STORAGE_PATH_MAX];

	if (addr) {
#if MAX_FILE_NAME >= STORAGE_FILE_NAME_LEN
		int len;

		len = snprintk(path, sizeof(path),
			       STORAGE_ROOT "/%02X%02X%02X%02X%02X%02X%u",
			       addr->a.val[5], addr->a.val[4], addr->a.val[3],
			       addr->a.val[2], addr->a.val[1], addr->a.val[0],
			       addr->type);

		/* Create the subdirectory if necessary */
		if (access == STORAGE_WRITE) {
			struct fs_dirent entry;
			int err;

			err = fs_stat(path, &entry);
			if (err) {
				err = fs_mkdir(path);
				if (err) {
					return err;
				}
			}
		}

		snprintk(path + len, sizeof(path) - len, "/%04x", key);
#else
		return -ENAMETOOLONG;
#endif
	} else {
		snprintk(path, sizeof(path), STORAGE_ROOT "/%04x", key);
	}

	return fs_open(file, path);
}

static ssize_t storage_read(const bt_addr_le_t *addr, u16_t key, void *data,
			    size_t length)
{
	fs_file_t file;
	ssize_t ret;

	ret = storage_open(addr, key, STORAGE_READ, &file);
	if (ret) {
		return ret;
	}

	ret = fs_read(&file, data, length);
	fs_close(&file);

	return ret;
}

static ssize_t storage_write(const bt_addr_le_t *addr, u16_t key,
			     const void *data, size_t length)
{
	fs_file_t file;
	ssize_t ret;

	ret = storage_open(addr, key, STORAGE_WRITE, &file);
	if (ret) {
		return ret;
	}

	ret = fs_write(&file, data, length);
	fs_close(&file);

	return ret;
}

static int unlink_recursive(char path[STORAGE_PATH_MAX])
{
	size_t path_len;
	fs_dir_t dir;
	int err;

	err = fs_opendir(&dir, path);
	if (err) {
		return err;
	}

	/* We calculate this up-front so we can keep reusing the same
	 * buffer for the path when recursing.
	 */
	path_len = strlen(path);

	while (1) {
		struct fs_dirent entry;

		err = fs_readdir(&dir, &entry);
		if (err) {
			break;
		}

		if (entry.name[0] == '\0') {
			break;
		}

		snprintk(path + path_len, STORAGE_PATH_MAX - path_len, "/%s",
			 entry.name);

		if (entry.type == FS_DIR_ENTRY_DIR) {
			err = unlink_recursive(path);
		} else {
			err = fs_unlink(path);
		}

		if (err) {
			break;
		}
	}

	fs_closedir(&dir);

	/* Return to the original value */
	path[path_len] = '\0';

	fs_unlink(path);

	return err;
}

static int storage_clear(const bt_addr_le_t *addr)
{
	char path[STORAGE_PATH_MAX];
	int err;

	if (addr) {
#if MAX_FILE_NAME >= STORAGE_FILE_NAME_LEN
		snprintk(path, STORAGE_PATH_MAX,
			 STORAGE_ROOT "/%02X%02X%02X%02X%02X%02X%u",
			 addr->a.val[5], addr->a.val[4], addr->a.val[3],
			 addr->a.val[2], addr->a.val[1], addr->a.val[0],
			 addr->type);

		return unlink_recursive(path);
#else
		return -ENAMETOOLONG;
#endif
	}

	/* unlink_recursive() uses the given path as a buffer for
	 * constructing sub-paths, so we can't give it a string literal
	 * such as STORAGE_ROOT directly.
	 */
	strcpy(path, STORAGE_ROOT);

	err = unlink_recursive(path);
	if (err) {
		return err;
	}

	return fs_mkdir(STORAGE_ROOT);
}

static int storage_init(struct device *unused)
{
	static const struct bt_storage storage = {
		.read  = storage_read,
		.write = storage_write,
		.clear = storage_clear
	};
	struct fs_dirent entry;
	int err;

	err = fs_stat(STORAGE_ROOT, &entry);
	if (err) {
		BT_WARN("%s doesn't seem to exist (err %d). Creating it.",
			STORAGE_ROOT, err);

		err = fs_mkdir(STORAGE_ROOT);
		if (err) {
			BT_ERR("Unable to create %s (err %d)",
			       STORAGE_ROOT, err);
			return err;
		}
	}

	bt_storage_register(&storage);

	return 0;
}

SYS_INIT(storage_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
