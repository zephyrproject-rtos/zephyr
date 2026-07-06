/**
 * Copyright (c) 2026 Hongquan Li
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/shell/shell.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/ztest.h>

static void test_shell_exec(const char *line, int result)
{
	int ret;

	ret = shell_execute_cmd(NULL, line);

	TC_PRINT("shell_execute_cmd(%s): %d\n", line, ret);

	zassert_true(ret == result, "%s", line);
}

ZTEST(shell_mount_partition, test_littlefs_mount_bad_label)
{
	test_shell_exec("fs mount littlefs /littlefs nonexistent", -ENOENT);
}

ZTEST(shell_mount_partition, test_littlefs_mount_no_format)
{
	const struct flash_area *fa;
	uint8_t expected[16];
	uint8_t actual[sizeof(expected)];

	memset(expected, 0x5a, sizeof(expected));
	zassert_ok(flash_area_open(PARTITION_ID(test_partition), &fa));
	zassert_ok(flash_area_flatten(fa, 0, fa->fa_size));
	zassert_ok(flash_area_write(fa, 0, expected, sizeof(expected)));

	test_shell_exec("fs mount littlefs /littlefs test-partition", -EIO);

	zassert_ok(flash_area_read(fa, 0, actual, sizeof(actual)));
	flash_area_close(fa);
	zassert_mem_equal(actual, expected, sizeof(expected), "Partition contents changed");
}

ZTEST(shell_mount_partition, test_littlefs_mount_partition)
{
	zassert_ok(fs_mkfs(FS_LITTLEFS, PARTITION_ID(test_partition), NULL, 0));
	test_shell_exec("fs mount littlefs /littlefs test-partition", 0);
}

ZTEST(shell_mount_partition, test_littlefs_mount_repeat_busy)
{
	test_shell_exec("fs mount littlefs /littlefs2", -EBUSY);
}

ZTEST_SUITE(shell_mount_partition, NULL, NULL, NULL, NULL, NULL);
