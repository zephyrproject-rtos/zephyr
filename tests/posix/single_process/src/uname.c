/*
 * Copyright (c) 2023 meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/sys/utsname.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/ztest.h>

ZTEST(posix_single_process, test_uname)
{
	struct utsname info;

	zassert_ok(uname(&info));
	zassert_ok(strncmp(info.sysname, "Zephyr", sizeof(info.sysname)));
	zassert_ok(strncmp(info.machine, CONFIG_ARCH, sizeof(info.machine)));
}
