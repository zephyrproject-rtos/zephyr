/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 * Copyright (c) 2016-2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Measure time
 *
 */
#include <kernel.h>
#include <zephyr.h>
#include <ksched.h>

#include "footprint.h"

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(footprint_mem_partition);
struct k_mem_domain footprint_mem_domain;
#endif

K_THREAD_STACK_DEFINE(my_stack_area, STACK_SIZE);
K_THREAD_STACK_DEFINE(my_stack_area_0, STACK_SIZE);

struct k_thread my_thread;
struct k_thread my_thread_0;

extern void run_heap_malloc_free(void);
extern void run_libc(void);
extern void run_mutex(void);
extern void run_semaphore(void);
extern void run_thread_system(void);
extern void run_timer(void);
extern void run_userspace(void);
extern void run_workq(void);

void main(void)
{
	printk("Hello from %s!\n", CONFIG_BOARD);

#ifdef CONFIG_USERSPACE
	struct k_mem_partition *mem_parts[] = {
	&footprint_mem_partition
	};

	k_mem_domain_init(&footprint_mem_domain,
			  ARRAY_SIZE(mem_parts), mem_parts);
#endif /* CONFIG_USERSPACE */

	run_thread_system();

	run_heap_malloc_free();

	run_semaphore();

	run_mutex();

	run_timer();

	run_libc();

	run_workq();

#ifdef CONFIG_USERSPACE
	run_userspace();
#endif

	printk("PROJECT EXECUTION SUCCESSFUL\n");
}
