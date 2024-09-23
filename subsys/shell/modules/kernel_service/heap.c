/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if K_HEAP_MEM_POOL_SIZE > 0
#include "kernel_shell.h"

#include <zephyr/sys/sys_heap.h>

extern struct sys_heap _system_heap;

static int cmd_kernel_heap(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;
	struct sys_memory_stats stats;

	err = sys_heap_runtime_stats_get(&_system_heap, &stats);
	if (err) {
		shell_error(sh, "Failed to read kernel system heap statistics (err %d)", err);
		return -ENOEXEC;
	}

	shell_print(sh, "free:           %zu", stats.free_bytes);
	shell_print(sh, "allocated:      %zu", stats.allocated_bytes);
	shell_print(sh, "max. allocated: %zu", stats.max_allocated_bytes);

	return 0;
}

KERNEL_CMD_ADD(heap, NULL, "System heap usage statistics.", cmd_kernel_heap);

#endif /* K_HEAP_MEM_POOL_SIZE > 0 */
