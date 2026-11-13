/*
 * Copyright (c) 2026 Chingbin Li
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "kernel_shell.h"
#include <zephyr/kernel.h>
#include <zephyr/app_memory/mem_domain.h>
#include <stdlib.h>
#include <inttypes.h>

static int cmd_kernel_thread_memparts(const struct shell *sh, size_t argc, char **argv)
{
	k_tid_t thread_id = (k_tid_t)strtoul(argv[1], NULL, 16);
	struct k_mem_domain *domain;
	struct k_mem_partition *partition;
	char attr_str[ARCH_MEM_PARTITION_ATTR_STR_MAX];

	if (!z_thread_is_valid(thread_id)) {
		shell_error(sh, "Thread ID %p is not valid", thread_id);
		return -EINVAL;
	}

	domain = thread_id->mem_domain_info.mem_domain;
	if (domain == NULL) {
		shell_error(sh, "Thread %p does not belong to any memory domain.", thread_id);
		return -EINVAL;
	}

	shell_print(sh, "Domain[%p]", domain);
	shell_print(sh, "Number Of Partitions: %d", domain->num_partitions);

	for (int i = 0; i < domain->num_partitions; i++) {
		partition = &domain->partitions[i];
		shell_print(sh, "Partition [%d]:", i);
		shell_print(sh, " - Range: 0x%" PRIxPTR " ~ 0x%" PRIxPTR " (%zu bytes)",
			    partition->start, (uintptr_t)(partition->start + partition->size),
			    partition->size);
		shell_print(sh, " - Permissions: %s",
			    arch_mem_partition_attr_str(&attr_str[0], &partition->attr));
	}

	return 0;
}

KERNEL_THREAD_CMD_ARG_ADD(memparts, NULL, "kernel thread memparts <thread_id>",
			  cmd_kernel_thread_memparts, 2, 0);
