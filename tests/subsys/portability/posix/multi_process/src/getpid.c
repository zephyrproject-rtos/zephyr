/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <zephyr/ztest.h>

ZTEST(posix_multi_process, test_getpid)
{
	pid_t pid = getpid();

	zexpect_true(pid > 0, "invalid pid: %d", pid);
}
