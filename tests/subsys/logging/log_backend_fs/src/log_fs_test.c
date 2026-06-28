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
#include <zephyr/fff.h>
#include <zephyr/logging/log_backend.h>

#define DT_DRV_COMPAT zephyr_fstab_littlefs
#define TEST_AUTOMOUNT DT_PROP(DT_DRV_INST(0), automount)
#if !TEST_AUTOMOUNT
#include <zephyr/fs/littlefs.h>
#define PARTITION_NODE DT_NODELABEL(lfs1)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#endif

#define MAX_PATH_LEN (256 + 7)

static const char *log_prefix = CONFIG_LOG_BACKEND_FS_FILE_PREFIX;
static const struct log_backend *backend;

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(log_output_dropped_process, const struct log_output *, uint32_t);
FAKE_VALUE_FUNC(log_format_func_t, log_format_func_t_get, uint32_t);

int write_log_to_file(uint8_t *data, size_t length, void *ctx);


/**
 * @brief Verify the filesystem backend writes a log before the filesystem is mounted.
 *
 * @details
 * On a non-automounting configuration, writes a log entry through the filesystem backend
 * while the filesystem is not yet mounted, then mounts it. Confirms the backend handles
 * writing to the filesystem resource without a pre-existing mount and that the mount can
 * subsequently be brought up. Skipped when the filesystem auto-mounts.
 *
 * Test steps:
 * - Skip if the filesystem is configured to auto-mount.
 * - Write a log entry to the file via the backend write path.
 * - Mount the filesystem partition.
 *
 * Expected result:
 * - The write reports the expected length and the mount succeeds.
 *
 * @see fs_mount()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-5
 */
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

/**
 * @brief Verify removal of pre-existing log files from the backend log directory.
 *
 * @details
 * Iterates the logging directory and unlinks any files matching the backend log prefix to
 * provide a clean starting state for subsequent backend tests. This is a maintenance step
 * over the filesystem backend's output files rather than a behavioral requirement check.
 *
 * Test steps:
 * - Open the configured log directory (return early if it does not yet exist).
 * - Read each directory entry.
 * - Unlink every regular file matching the log prefix.
 *
 * Expected result:
 * - All prior log files are removed, leaving a clean log directory.
 *
 * @see fs_unlink()
 * @ingroup logging_tests
 */
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

/**
 * @brief Verify that logged text is correctly persisted to the backend log file.
 *
 * @details
 * Writes a known log string through the filesystem backend, signals the backend to flush,
 * then opens the resulting log file and reads it back to confirm the content matches.
 * Repeats with a second entry and a seek to confirm sequential appends. This validates
 * that the backend stores log output to a filesystem resource for later inspection.
 *
 * Test steps:
 * - Write a known string and notify the backend that processing is done.
 * - Open the log file and read back its content, asserting it matches.
 * - Write a second string, flush, seek past the first, and read back the second entry.
 *
 * Expected result:
 * - The file contains exactly the logged strings in the order written.
 *
 * @see fs_read()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-2
 * @verifies ZEP-SRS-11-5
 */
ZTEST(test_log_backend_fs, test_log_fs_file_content)
{
	int rc;
	struct fs_file_t file;
	char log_read[MAX_PATH_LEN];
	uint8_t to_log[] = "Correct Log 1";
	static char fname[MAX_PATH_LEN];

	fs_file_t_init(&file);

	rc = write_log_to_file(to_log, sizeof(to_log), NULL);
	backend->api->notify(backend, LOG_BACKEND_EVT_PROCESS_THREAD_DONE, NULL);

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
	backend->api->notify(backend, LOG_BACKEND_EVT_PROCESS_THREAD_DONE, NULL);

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

/**
 * @brief Verify the backend rotates to a new file when the size limit is reached.
 *
 * @details
 * Writes enough log entries to exceed the configured per-file size limit, flushes the
 * backend, then confirms the first file is capped at the limit and a second file was
 * created to hold the overflow. This validates the filesystem backend's file-rotation
 * mechanics that keep the log resource bounded.
 *
 * Test steps:
 * - Write entries until the first file exceeds the configured size limit and flush.
 * - Stat the first file and assert its size is bounded as expected.
 * - Stat the second file and assert it holds the overflow entry.
 * - Count log files in the directory and assert two exist.
 *
 * Expected result:
 * - The first file is size-bounded and a second file holds the remaining output.
 *
 * @see fs_stat()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-5
 */
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

	backend->api->notify(backend, LOG_BACKEND_EVT_PROCESS_THREAD_DONE, NULL);

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

/**
 * @brief Verify the backend enforces the maximum log file count by rotating oldest files.
 *
 * @details
 * Writes enough log entries to exceed the configured maximum number of log files, flushes
 * the backend, then counts the remaining files and checks their numbering. Confirms the
 * filesystem backend caps the number of stored files and discards the oldest, keeping the
 * log resource bounded on disk.
 *
 * Test steps:
 * - Write entries spanning more than the configured file-count limit and flush.
 * - Enumerate the log directory, counting files and recording their indices.
 * - Assert the file count equals the configured limit and the surviving indices are the newest.
 *
 * Expected result:
 * - Exactly the configured maximum number of files remain, retaining the newest entries.
 *
 * @see fs_readdir()
 * @ingroup logging_tests
 * @verifies ZEP-SRS-11-5
 */
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

	backend->api->notify(backend, LOG_BACKEND_EVT_PROCESS_THREAD_DONE, NULL);

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

static const struct log_backend *backend_find(char const *name)
{
	size_t slen = strlen(name);

	STRUCT_SECTION_FOREACH(log_backend, backend) {
		if (strncmp(name, backend->name, slen) == 0) {
			return backend;
		}
	}

	return NULL;
}


void *suite_setup(void)
{
	backend = backend_find("log_backend_fs");
	zassert_not_null(backend);

	return NULL;
}

ZTEST_SUITE(test_log_backend_fs, NULL, suite_setup, NULL, NULL, NULL);
