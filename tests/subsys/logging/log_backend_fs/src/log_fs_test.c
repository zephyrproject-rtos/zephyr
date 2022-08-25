/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Test logging to file system
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/fs/fs.h>

#define DT_DRV_COMPAT zephyr_fstab_littlefs
#define TEST_AUTOMOUNT DT_PROP(DT_DRV_INST(0), automount)
#if !TEST_AUTOMOUNT
#include <zephyr/fs/littlefs.h>
#define PARTITION_NODE DT_NODELABEL(lfs1)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#endif

#define MAX_PATH_LEN (256 + 7)

static const char *log_prefix = CONFIG_LOG_BACKEND_FS_FILE_PREFIX;

int write_log_to_file(uint8_t *data, size_t length, void *ctx);


ZTEST(test_log_backend_fs, test_fs_nonexist)
{
	#if TEST_AUTOMOUNT
	ztest_test_skip();
	#else
	uint8_t to_log[] = "Log to left behind";
	int rc;

	rc = write_log_to_file(to_log, sizeof(to_log), NULL);
	zassert_equal(rc, sizeof(to_log), "Unexpected rteval.");
	struct fs_mount_t *mp = &FS_FSTAB_ENTRY(PARTITION_NODE);

	rc = fs_mount(mp);
	zassert_equal(rc, 0, "Can not mount FS.");
	#endif
}

ZTEST(test_log_backend_fs, test_wipe_fs_logs)
{
	int rc;
	struct fs_dir_t dir;
	struct fs_file_t file;
	char fname[MAX_PATH_LEN];

	fs_dir_t_init(&dir);
	fs_file_t_init(&file);

	rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);
	if (rc) {
		/* log directory might not exist jet */
		return;
	}

	/* Iterate over logging directory. */
	while (1) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		zassert_equal(rc, 0, "Can not read directory.");
		if ((rc < 0) || (ent.name[0] == 0)) {
			break;
		}
		if (ent.type == FS_DIR_ENTRY_FILE &&
		    strncmp(ent.name, log_prefix, strlen(log_prefix)) == 0) {
			sprintf(fname, "%s/%s", CONFIG_LOG_BACKEND_FS_DIR,
				ent.name);
			rc = fs_unlink(fname);
			zassert_equal(rc, 0, "Can not remove file %s.", fname);
			TC_PRINT("removed: %s\n", fname);
		}
	}

	(void)fs_closedir(&dir);
}

ZTEST(test_log_backend_fs, test_log_fs_file_content)
{
	int rc;
	struct fs_file_t file;
	char log_read[MAX_PATH_LEN];
	uint8_t to_log[] = "Correct Log 1";
	static char fname[MAX_PATH_LEN];

	fs_file_t_init(&file);

	rc = write_log_to_file(to_log, sizeof(to_log), NULL);

	sprintf(fname, "%s/%s0000", CONFIG_LOG_BACKEND_FS_DIR, log_prefix);

	zassert_equal(fs_open(&file, fname, FS_O_READ), 0,
		      "Can not open log file.");

	zassert_true(fs_read(&file, log_read, MAX_PATH_LEN) >= 0,
		     "Can not read log file.");

	rc = strncmp(log_read, to_log, sizeof(log_read));
	zassert_equal(rc, 0, "Text inside log file is not correct.");

	zassert_equal(fs_close(&file), 0, "Can not close log file.");

	to_log[sizeof(to_log)-2] = '2';
	rc = write_log_to_file(to_log, sizeof(to_log), NULL);

	zassert_equal(fs_open(&file, fname, FS_O_READ), 0,
		      "Can not open log file.");

	zassert_equal(fs_seek(&file, sizeof(to_log), FS_SEEK_SET), 0,
		      "Bad file size");

	zassert_true(fs_read(&file, log_read, MAX_PATH_LEN) >= 0,
		     "Can not read log file.");

	rc = strncmp(log_read, to_log, sizeof(log_read));
	zassert_equal(rc, 0, "Text inside log file is not correct.");

	zassert_equal(fs_close(&file), 0, "Can not close log file.");
}

ZTEST(test_log_backend_fs, test_log_fs_file_size)
{
	int rc;
	int i;
	struct fs_dir_t dir;
	int file_ctr = 0;
	static char fname[MAX_PATH_LEN];
	uint8_t to_log[] = "Text Log";
	struct fs_dirent entry;

	fs_dir_t_init(&dir);

	sprintf(fname, "%s/%s0000", CONFIG_LOG_BACKEND_FS_DIR, log_prefix);
	zassert_equal(fs_stat(fname, &entry), 0, "Can not get file info.");

	/* Fill in log file over size limit. */
	for (i = 0;
	     i <= (CONFIG_LOG_BACKEND_FS_FILE_SIZE - entry.size) /
		  sizeof(to_log);
	     i++) {
		rc = write_log_to_file(to_log, sizeof(to_log), NULL);
		/* Written length not tracked here. */
		ARG_UNUSED(rc);
	}

	zassert_equal(fs_stat(fname, &entry), 0, "Can not get file info.");
	size_t exp_size = CONFIG_LOG_BACKEND_FS_FILE_SIZE -
			  (CONFIG_LOG_BACKEND_FS_FILE_SIZE - entry.size) %
			  sizeof(to_log);
	zassert_equal(entry.size, exp_size, "Unexpected %s file size (%d B)",
		      fname, entry.size);

	sprintf(fname, "%s/%s0001", CONFIG_LOG_BACKEND_FS_DIR, log_prefix);
	zassert_equal(fs_stat(fname, &entry), 0, "Can not get file info.");

	zassert_equal(entry.size, sizeof(to_log),
		      "Unexpected %s file size (%d B)",
		      fname, entry.size);

	rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);
	zassert_equal(rc, 0, "Can not open directory.");
	/* Count number of log files. */
	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if ((rc < 0) || (ent.name[0] == 0)) {
			break;
		}
		if (strstr(ent.name, log_prefix) != NULL) {
			++file_ctr;
		}
	}
	(void)fs_closedir(&dir);
	zassert_equal(file_ctr, 2, "File changing failed");
}

ZTEST(test_log_backend_fs, test_log_fs_files_max)
{
	int rc;
	int i;
	struct fs_dir_t dir;
	int file_ctr = 0;
	uint8_t to_log[] = "Text Log";
	struct fs_dirent ent;
	uint32_t test_mask = 0;

	fs_dir_t_init(&dir);

	/* Fill in log files over files count limit. */
	for (i = 0;
	     i <= CONFIG_LOG_BACKEND_FS_FILE_SIZE /
		  sizeof(to_log) * (CONFIG_LOG_BACKEND_FS_FILES_LIMIT - 1);
	     i++) {
		rc = write_log_to_file(to_log, sizeof(to_log), NULL);
		/* Written length not tracked here. */
		ARG_UNUSED(rc);
	}

	rc = fs_opendir(&dir, CONFIG_LOG_BACKEND_FS_DIR);
	zassert_equal(rc, 0, "Can not open directory.");
	/* Count log files. */
	while (rc >= 0) {

		rc = fs_readdir(&dir, &ent);
		if ((rc < 0) || (ent.name[0] == 0)) {
			break;
		}
		if (strstr(ent.name, log_prefix) != NULL) {
			++file_ctr;
			test_mask |= 1 << atoi(&ent.name[strlen(log_prefix)]);
		}
	}
	(void)fs_closedir(&dir);
	zassert_equal(file_ctr, CONFIG_LOG_BACKEND_FS_FILES_LIMIT,
		      "Bad files count: expected %d, got %d ",
		      CONFIG_LOG_BACKEND_FS_FILES_LIMIT, file_ctr);
	/*expected files: log.0001, log.0002, log.0003, log.0004 */
	zassert_equal(test_mask, 0b11110, "Unexpected file numeration");
}

ZTEST_SUITE(test_log_backend_fs, NULL, NULL, NULL, NULL, NULL);
