/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Interactive shell test suite
 *
 */

#include <zephyr.h>
#include <ztest.h>

#include "test_fs_shell.h"
#include <shell/shell.h>

static void test_shell_exec(const char *line, int result)
{
	int ret;

	ret = shell_execute_cmd(NULL, line);

	TC_PRINT("shell_execute_cmd(%s): %d\n", line, ret);

	zassert_true(ret == result, line);
}

void test_fs_help(void)
{
#ifdef CONFIG_FILE_SYSTEM_SHELL
	test_shell_exec("help", 0);
	test_shell_exec("help fs", 0);
	test_shell_exec("fs mount fat", -EINVAL);
	test_shell_exec("fs mount nffs", -EINVAL);
#else
	ztest_test_skip();
#endif
}

void test_fs_fat_mount(void)
{
	test_shell_exec("fs mount fat /RAM:", 0);
}
void test_fs_nffs_mount(void)
{
	test_shell_exec("fs mount nffs /nffs", 0);
}
