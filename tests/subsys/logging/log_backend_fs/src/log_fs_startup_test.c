/*
 * Copyright (c) 2026 Jakub Kwintal
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/fff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/ztest.h>

#define PARTITION_NODE DT_NODELABEL(lfs1)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(log_output_dropped_process, const struct log_output *, uint32_t);
FAKE_VALUE_FUNC(log_format_func_t, log_format_func_t_get, uint32_t);

int write_log_to_file(uint8_t *data, size_t length, void *ctx);

/** @brief Resume existing log and rotate before exceeding the file size limit. */
ZTEST(test_log_backend_fs_startup, test_append_to_existing_file)
{
	const char old_name[] =
		CONFIG_LOG_BACKEND_FS_DIR "/" CONFIG_LOG_BACKEND_FS_FILE_PREFIX "0000";
	const char new_name[] =
		CONFIG_LOG_BACKEND_FS_DIR "/" CONFIG_LOG_BACKEND_FS_FILE_PREFIX "0001";
	uint8_t existing[CONFIG_LOG_BACKEND_FS_FILE_SIZE - TEST_REMAINING_SPACE] = {0};
	uint8_t readback[CONFIG_LOG_BACKEND_FS_FILE_SIZE];
	uint8_t message[] = {'O', 'K'};
	const bool rotate = sizeof(message) > TEST_REMAINING_SPACE;
	struct fs_dirent entry;
	struct fs_file_t file;
	int rc;

#if !DT_PROP(PARTITION_NODE, automount)
	zassert_ok(fs_mount(&FS_FSTAB_ENTRY(PARTITION_NODE)), "Cannot mount LittleFS");
#endif

	rc = fs_mkdir(CONFIG_LOG_BACKEND_FS_DIR);
	zassert_true(rc == 0 || rc == -EEXIST, "Failed to create log directory: %d", rc);
	rc = fs_unlink(new_name);
	zassert_true(rc == 0 || rc == -ENOENT, "Failed to remove previous log: %d", rc);

	/* Seed the filesystem before the backend's first write, as after a reboot. */
	fs_file_t_init(&file);
	zassert_ok(fs_open(&file, old_name, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC));
	zassert_equal(fs_write(&file, existing, sizeof(existing)), sizeof(existing));
	zassert_ok(fs_close(&file));

	zassert_equal(write_log_to_file(message, sizeof(message), NULL), sizeof(message));
	STRUCT_SECTION_FOREACH(log_backend, backend) {
		log_backend_notify(backend, LOG_BACKEND_EVT_PROCESS_THREAD_DONE, NULL);
	}

	zassert_ok(fs_stat(old_name, &entry));
	zassert_equal(entry.size, sizeof(existing) + (rotate ? 0 : sizeof(message)),
		      "Unexpected size of the pre-existing log");
	zassert_ok(fs_open(&file, old_name, FS_O_READ));
	zassert_equal(fs_read(&file, readback, sizeof(existing)), sizeof(existing));
	zassert_mem_equal(readback, existing, sizeof(existing), "Existing log was modified");

	if (rotate) {
		zassert_ok(fs_close(&file));
		zassert_ok(fs_stat(new_name, &entry), "Log did not rotate");
		zassert_equal(entry.size, sizeof(message), "Unexpected size of the new log");
		zassert_ok(fs_open(&file, new_name, FS_O_READ));
	} else {
		zassert_equal(fs_stat(new_name, &entry), -ENOENT, "Log rotated prematurely");
	}

	zassert_equal(fs_read(&file, readback, sizeof(message)), sizeof(message));
	zassert_mem_equal(readback, message, sizeof(message), "Log message was lost");
	zassert_ok(fs_close(&file));
}

ZTEST_SUITE(test_log_backend_fs_startup, NULL, NULL, NULL, NULL, NULL);
