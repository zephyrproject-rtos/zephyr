/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>
#include <string.h>
#include <stdlib.h>

/* Flag needed to figure out if the fault was expected or not. */
extern volatile bool valid_fault;

static inline void set_fault_valid(bool valid)
{
	valid_fault = valid;
	/* Put a barrier here, such that no instructions get ordered by the
	 * compiler before we set valid_fault. This can happen with expansion
	 * of inline syscall invocation functions.
	 */
	compiler_barrier();
}

/* For inherit.c */
#define INHERIT_STACK_SIZE CONFIG_MAIN_STACK_SIZE
#define SEMAPHORE_MAX_COUNT (10)
#define SEMAPHORE_INIT_COUNT (0)
#define SYNC_SEM_MAX_COUNT (1)
#define SYNC_SEM_INIT_COUNT (0)
#define MSG_Q_SIZE (10)
#define MSG_Q_MAX_NUM_MSGS (10)
#define MSG_Q_ALIGN (2)
#define PRIORITY 5
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define BLK_SIZE_MIN 16
#define BLK_SIZE_MAX 64
#define BLK_NUM_MIN 8
#define BLK_NUM_MAX 2
#define BLK_ALIGN BLK_SIZE_MIN
#define SEM_INIT_VAL (0U)
#define SEM_MAX_VAL (1U)

/* For mem_domain.c  */
#define MEM_DOMAIN_STACK_SIZE CONFIG_MAIN_STACK_SIZE
#define MEM_PARTITION_INIT_NUM (1)
#define BLK_SIZE_MIN_MD 8
#define BLK_SIZE_MAX_MD 16
#define BLK_NUM_MAX_MD 4
#define BLK_ALIGN_MD BLK_SIZE_MIN_MD
#define DESC_SIZE	sizeof(struct sys_mem_pool_block)
#define STACK_SIZE_MD (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define PRIORITY_MD 5

#if defined(CONFIG_X86)
#define MEM_REGION_ALLOC (4096)
#elif defined(CONFIG_ARC)
#define MEM_REGION_ALLOC (Z_ARC_MPU_ALIGN)
#elif defined(CONFIG_ARM64)
#define MEM_REGION_ALLOC (4096)
#elif defined(CONFIG_ARM)
#define MEM_REGION_ALLOC (Z_THREAD_MIN_STACK_ALIGN)
#elif defined(CONFIG_RISCV)
#define MEM_REGION_ALLOC (4)
#else
#error "Test suite not compatible for the given architecture"
#endif
#define MEM_DOMAIN_ALIGNMENT __aligned(MEM_REGION_ALLOC)

/* for kobject.c */
#define KOBJECT_STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)



#if (defined(CONFIG_X86_64) || defined(CONFIG_ARM64) || \
	(defined(CONFIG_RISCV) && defined(CONFIG_64BIT)))
#define TEST_HEAP_SIZE	(2 << CONFIG_MAX_THREAD_BYTES) * 1024
#define MAX_OBJ 512
#else
#define TEST_HEAP_SIZE	(2 << CONFIG_MAX_THREAD_BYTES) * 256
#define MAX_OBJ 256
#endif

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_

__syscall struct k_heap *ret_resource_pool_ptr(void);

#include <syscalls/mem_protect.h>

#endif /* _TEST_SYSCALLS_H_ */
