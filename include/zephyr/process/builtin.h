/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_PROCESS_BUILTIN_H_
#define ZEPHYR_INCLUDE_PROCESS_BUILTIN_H_

#include <zephyr/process/process.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_process_builtin {
	struct k_process process;
	k_process_entry_t entry;
};

struct k_process *k_process_builtin_init(struct k_process_builtin *builtin,
					 const char *name,
					 k_process_entry_t entry);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PROCESS_BUILTIN_H_ */
