/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#if K_HEAP_MEM_POOL_SIZE > 0
extern struct k_heap _system_heap;

static int cmd_kernel_heap(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err;
	struct sys_memory_stats stats;

	err = sys_heap_runtime_stats_get(&_system_heap.heap, &stats);
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

#if defined(CONFIG_COMMON_LIBC_MALLOC) && CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE != 0
#include <sys_malloc.h>

static int cmd_kernel_malloc_heap(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct sys_memory_stats stats;
	int err;

	err = malloc_runtime_stats_get(&stats);
	if (err != 0) {
		shell_error(sh, "Failed to read malloc heap statistics (err %d)", err);
		return err;
	}

	shell_print(sh, "free:           %zu", stats.free_bytes);
	shell_print(sh, "allocated:      %zu", stats.allocated_bytes);
	shell_print(sh, "max. allocated: %zu", stats.max_allocated_bytes);

	return 0;
}

KERNEL_CMD_ADD(malloc_heap, NULL, "Common libc malloc heap usage statistics.",
	       cmd_kernel_malloc_heap);
#endif /* CONFIG_COMMON_LIBC_MALLOC && CONFIG_COMMON_LIBC_MALLOC_ARENA_SIZE != 0 */
