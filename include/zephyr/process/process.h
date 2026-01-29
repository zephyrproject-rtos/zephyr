/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROCESS_PROCESS_H_
#define ZEPHYR_INCLUDE_PROCESS_PROCESS_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_process;

typedef int (*k_process_entry_t)(size_t argc,
				 const char **argv,
				 struct k_pipe *input,
				 struct k_pipe *output);

typedef int (*k_process_load_t)(struct k_process *process,
				k_process_entry_t *entry);

typedef void (*k_process_unload_t)(struct k_process *process);

struct k_process {
	const char *name;
	k_process_load_t load;
	k_process_unload_t unload;
	sys_snode_t node;
	uint16_t pid;
#if CONFIG_USERSPACE
	struct k_mem_domain domain;
#endif
};

void k_process_init(struct k_process *process,
		    const char *name,
		    k_process_load_t load,
		    k_process_unload_t unload);

int k_process_register(struct k_process *process);

int k_process_unregister(struct k_process *process);

__syscall struct k_process *k_process_get(const char *name);

__syscall int k_process_start(struct k_process *process,
			      size_t argc,
			      const char **argv,
			      struct k_pipe *input,
			      struct k_pipe *output);

__syscall int k_process_stop(struct k_process *process);

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/process.h>

#endif /* ZEPHYR_INCLUDE_PROCESS_PROCESS_H_ */
