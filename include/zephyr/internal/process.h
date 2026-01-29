/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INTERNAL_PROCESS_H_
#define ZEPHYR_INCLUDE_INTERNAL_PROCESS_H_

#include <zephyr/process/process.h>

const char *k_process_get_name(struct k_process *process);

k_tid_t k_process_get_thread(struct k_process *process);

k_thread_stack_t *k_process_get_thread_stack(struct k_process *process);

struct k_mem_domain *k_process_get_domain(struct k_process *process);

#endif /* ZEPHYR_INCLUDE_INTERNAL_PROCESS_H_ */
