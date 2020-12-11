/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FOOTPRINT_H_
#define _FOOTPRINT_H_

#include <kernel.h>
#include <zephyr.h>
#include <app_memory/app_memdomain.h>

K_THREAD_STACK_EXTERN(my_stack_area);
K_THREAD_STACK_EXTERN(my_stack_area_0);
extern struct k_thread my_thread;
extern struct k_thread my_thread_0;

#define STACK_SIZE	512

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
