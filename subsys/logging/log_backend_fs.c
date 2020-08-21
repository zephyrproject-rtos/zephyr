/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <logging/log_backend.h>
#include <logging/log_backend_std.h>
#include <assert.h>
#include <fs/fs.h>

#define MAX_PATH_LEN 256
#define MAX_FLASH_WRITE_SIZE 256
#define LOG_PREFIX_LEN (sizeof(CONFIG_LOG_BACKEND_FS_FILE_PREFIX) - 1)
#define MAX_FILE_NUMERAL 9999
#define FILE_NUMERAL_LEN 4

enum backend_fs_state {
	BACKEND_FS_NOT_INITIALIZED = 0,
	BACKEND_FS_CORRUPTED,
	BACKEND_FS_OK
};

static struct fs_file_t file;
static enum backend_fs_state backend_state = BACKEND_FS_NOT_INITIALIZED;
static int file_ctr, newest, oldest;

static int allocate_new_file(struct fs_file_t *file);
static int del_oldest_log(void);

static int check_log_dir_available(void)
{
	int index = 0;
	char const *name;
	int rc = 0;

	while (rc == 0) {
		rc = fs_readmount(&index, &name);
		if (rc == 0) {
			if (strncmp(CONFIG_LOG_BACKEND_FS_DIR,
				    name,
				    strlen(CONFIG_LOG_BACKEND_FS_DIR))
			    == 0) {
				return 0;
			}
		}
	}

	return -ENOENT;
}

int write_log_to_file(uint8_t *data, size_t length, void *ctx)
{
	int rc;
	struct fs_file_t *f = &file;

	if (backend_state == BACKEND_FS_NOT_INITIALIZED) {
		if (check_log_dir_available()) {
			return length;
		}
		rc = allocate_new_file(&file);
		backend_state = (rc ? BACKEND_FS_CORRUPTED : BACKEND_FS_OK);
	}

	if (backend_state == BACKEND_FS_OK) {

		/* Check if new data overwrites max file size.
		 * If so, create new log file.
		 */
		int size = fs_tell(f);

		if (size < 0) {
			backend_state = BACKEND_FS_CORRUPTED;

			return length;
		} else if ((size + length) > CONFIG_LOG_BACKEND_FS_FILE_SIZE) {
			rc = allocate_new_file(f);

			if (rc < 0) {
				goto on_error;
			}
		}

		rc = fs_write(f, data, length);
		if (rc == -ENOSPC) {
			if (IS_ENABLED(CONFIG_LOG_BACKEND_FS_OVERWRITE)) {
				del_oldest_log();

				return 0;
			}
			/* If overwrite is disabled, full memory
			 * is equivalent of corrupted backend.
			 */
			goto on_error;
		} else if (rc < 0) {
			return 0;
		}

		fs_sync(f);
	}

	return length;

on_error:
	backend_state = BACKEND_FS_CORRUPTED;
	return length;
}

static int get_log_file_id(struct fs_dirent *ent)
{
	size_t len;
	int num;

	if (ent->type != FS_DIR_ENTRY_FILE) {
		return -1;
	}

	len = strlen(ent->name);

	if (len != LOG_PREFIX_LEN + FILE_NUMERAL_LEN) {
		return -1;
	}

	if (memcmp(ent->name, CONFIG_LOG_BACKEND_FS_FILE_PREFIX, LOG_PREFIX_LEN) != 0) {
		return -1;
	}

	num = atoi(ent->name + LOG_PREFIX_LEN);

	if (num <= MAX_FILE_NUMERAL && num > 0) {
		return num;
	}

	return -1;
}

static int allocate_new_file(struct fs_file_t *file)
{
	/* In case of no log file or current file fills up
	 * create new log file.
	 */
	int rc;
	struct fs_statvfs stat;
	int curr_file_num;
	struct fs_dirent ent;
	char fname[MAX_PATH_LEN];

	assert(file);

	if (backend_state == BACKEND_FS_NOT_INITIALIZED) {
		/* Search for the last used log number. */
		struct fs_dir_t dir;
		int file_num = 0;

		fs_dir_t_init(&dir);
		curr_file_num = 0;
		int max = 0, min = MAX_FILE_NUMERAL;

		rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);

		while (rc >= 0) {
			rc = fs_readdir(&dir, &ent);
			if ((rc < 0) || (ent.name[0] == 0)) {
				break;
			}

			file_num = get_log_file_id(&ent);
			if (file_num > 0) {

				if (file_num > max) {
					max = file_num;
				}

				if (file_num < min) {
					min = file_num;
				}
				++file_ctr;
			}
		}

		oldest = min;

		if ((file_ctr > 1) &&
		    ((max - min) >
		     2 * CONFIG_LOG_BACKEND_FS_FILES_LIMIT)) {
			/* oldest log is in the range around the min */
			newest = min;
			oldest = max;
			(void)fs_closedir(&dir);
			rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);

			while (rc == 0) {
				rc = fs_readdir(&dir, &ent);
				if ((rc < 0) || (ent.name[0] == 0)) {
					break;
				}

				file_num = get_log_file_id(&ent);
				if (file_num < min + CONFIG_LOG_BACKEND_FS_FILES_LIMIT) {
					if (newest < file_num) {
						newest = file_num;
					}
				}

				if (file_num > max - CONFIG_LOG_BACKEND_FS_FILES_LIMIT) {
					if (oldest > file_num) {
						oldest = file_num;
					}
				}
			}
		} else {
			newest = max;
			oldest = min;
		}

		(void)fs_closedir(&dir);
		if (rc < 0) {
			goto out;
		}

		curr_file_num = newest;

		if (file_ctr >= 1) {
			curr_file_num++;
			if (curr_file_num > MAX_FILE_NUMERAL) {
				curr_file_num = 0;
			}
		}

		backend_state = BACKEND_FS_OK;
	} else {
		fs_close(file);

		curr_file_num = newest;
		curr_file_num++;
		if (curr_file_num > MAX_FILE_NUMERAL) {
			curr_file_num = 0;
		}
	}

	rc = fs_statvfs(CONFIG_LOG_BACKEND_FS_DIR, &stat);

	/* Check if there is enough space to write file or max files number
	 * is not exceeded.
	 */
	while ((file_ctr >= CONFIG_LOG_BACKEND_FS_FILES_LIMIT) ||
	       ((stat.f_bfree * stat.f_frsize) <=
		CONFIG_LOG_BACKEND_FS_FILE_SIZE)) {

		if (IS_ENABLED(CONFIG_LOG_BACKEND_FS_OVERWRITE)) {
			rc = del_oldest_log();
			if (rc < 0) {
				goto out;
			}

			rc = fs_statvfs(CONFIG_LOG_BACKEND_FS_DIR,
					&stat);
			if (rc < 0) {
				goto out;
			}
		} else {
			return -ENOSPC;
		}
	}

	snprintf(fname, sizeof(fname), "%s/%s%04d",
		CONFIG_LOG_BACKEND_FS_DIR,
		CONFIG_LOG_BACKEND_FS_FILE_PREFIX, curr_file_num);

	rc = fs_open(file, fname, FS_O_CREATE | FS_O_WRITE);
	if (rc < 0) {
		goto out;
	}
	++file_ctr;
	newest = curr_file_num;

out:
	return rc;
}

static int del_oldest_log(void)
{
	int rc;
	static char dellname[MAX_PATH_LEN];

	while (1) {
		snprintf(dellname, sizeof(dellname), "%s/%s%04d",
			 CONFIG_LOG_BACKEND_FS_DIR,
			 CONFIG_LOG_BACKEND_FS_FILE_PREFIX, oldest);
		rc = fs_unlink(dellname);

		if ((rc == 0) || (rc == -ENOENT)) {
			oldest++;
			if (oldest > MAX_FILE_NUMERAL) {
				oldest = 0;
			}

			if (rc == 0) {
				--file_ctr;
				break;
			}
		} else {
			break;
		}
	}

	return rc;
}

BUILD_ASSERT(!IS_ENABLED(CONFIG_LOG_MODE_IMMEDIATE),
	     "Immediate logging is not supported by LOG FS backend.");

#ifndef CONFIG_LOG_BACKEND_FS_TESTSUITE

static uint8_t __aligned(4) buf[MAX_FLASH_WRITE_SIZE];
LOG_OUTPUT_DEFINE(log_output, write_log_to_file, buf, MAX_FLASH_WRITE_SIZE);

static void put(const struct log_backend *const backend,
		struct log_msg *msg)
{
	log_backend_std_put(&log_output, 0, msg);
}

static void log_backend_fs_init(void)
{
}

static void panic(struct log_backend const *const backend)
{
	/* In case of panic deinitialize backend. It is better to keep
	 * current data rather than log new and risk of failure.
	 */
	log_backend_deactivate(backend);
}

static void dropped(const struct log_backend *const backend, uint32_t cnt)
{
	ARG_UNUSED(backend);

	log_backend_std_dropped(&log_output, cnt);
}

static const struct log_backend_api log_backend_fs_api = {
	.put = put,
	.put_sync_string = NULL,
	.put_sync_hexdump = NULL,
	.panic = panic,
	.init = log_backend_fs_init,
	.dropped = dropped,
};


LOG_BACKEND_DEFINE(log_backend_fs, log_backend_fs_api, true);
#endif
