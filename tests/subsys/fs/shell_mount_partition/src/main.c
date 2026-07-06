/**
 * Copyright (c) 2026 Hongquan Li
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/shell/shell.h>

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

ZTEST(shell_mount_partition, test_littlefs_mount_partition)
{
	test_shell_exec("fs mount littlefs /littlefs test-partition", 0);
}

ZTEST(shell_mount_partition, test_littlefs_mount_repeat_busy)
{
	test_shell_exec("fs mount littlefs /littlefs2", -EBUSY);
}

ZTEST_SUITE(shell_mount_partition, NULL, NULL, NULL, NULL, NULL);
