/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FOOTPRINT_H_
#define _FOOTPRINT_H_

#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>

#define STACK_SIZE	512

K_THREAD_STACK_DECLARE(my_stack_area, STACK_SIZE);
K_THREAD_STACK_DECLARE(my_stack_area_0, STACK_SIZE);
extern struct k_thread my_thread;
extern struct k_thread my_thread_0;

#ifdef CONFIG_USERSPACE
#define FP_DMEM		K_APP_DMEM(footprint_mem_partition)
#define FP_BMEM		K_APP_BMEM(footprint_mem_partition)
extern struct k_mem_partition footprint_mem_partition;
extern struct k_mem_domain footprint_mem_domain;
#else
#define FP_DMEM
#define FP_BMEM
#endif

#endif /* _FOOTPRINT_H_ */
