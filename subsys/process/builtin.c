/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/process/builtin.h>

static int process_builtin_load(struct k_process *process, k_process_entry_t *entry)
{
	struct k_process_builtin *builtin;

	builtin = CONTAINER_OF(process, struct k_process_builtin, process);
	*entry = builtin->entry;
	return 0;
}

struct k_process *k_process_builtin_init(struct k_process_builtin *builtin,
					 const char *name,
					 k_process_entry_t entry)
{
	builtin->entry = entry;

	k_process_init(&builtin->process,
		       name,
		       process_builtin_load,
		       NULL);

	return &builtin->process;
}
