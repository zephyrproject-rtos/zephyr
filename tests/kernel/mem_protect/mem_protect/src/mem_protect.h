/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel_structs.h>
#include <string.h>
#include <stdlib.h>

extern void test_permission_inheritance(void);
extern void test_mem_domain_valid_access(void);
extern void test_mem_domain_invalid_access(void);
extern void test_mem_domain_partitions_user_rw(void);
extern void test_mem_domain_partitions_user_ro(void);
extern void test_mem_domain_partitions_supervisor_rw(void);
extern void test_mem_domain_add_partitions_invalid(void);
extern void test_mem_domain_add_partitions_simple(void);
extern void test_mem_domain_remove_partitions_simple(void);
extern void test_mem_domain_remove_partitions(void);
extern void test_kobject_access_grant(void);
extern void test_syscall_invalid_kobject(void);
extern void test_thread_without_kobject_permission(void);
extern void test_kobject_revoke_access(void);
extern void test_kobject_grant_access_kobj(void);
extern void test_kobject_grant_access_kobj_invalid(void);
extern void test_kobject_release_from_user(void);
extern void test_kobject_access_all_grant(void);
extern void test_thread_has_residual_permissions(void);
extern void test_kobject_access_grant_to_invalid_thread(void);
extern void test_kobject_access_invalid_kobject(void);
extern void test_access_kobject_without_init_access(void);
extern void test_access_kobject_without_init_with_access(void);
extern void test_kobject_reinitialize_thread_kobj(void);
extern void test_create_new_thread_from_user(void);
extern void test_new_user_thread_with_in_use_stack_obj(void);
extern void test_create_new_thread_from_user_no_access_stack(void);
extern void test_create_new_thread_from_user_invalid_stacksize(void);
extern void test_create_new_thread_from_user_huge_stacksize(void);
extern void test_create_new_supervisor_thread_from_user(void);
extern void test_create_new_essential_thread_from_user(void);
extern void test_create_new_higher_prio_thread_from_user(void);
extern void test_create_new_invalid_prio_thread_from_user(void);
extern void test_inherit_resource_pool(void);
extern void test_mark_thread_exit_uninitialized(void);

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
#define MSG_Q_SIZE (10)
#define MSG_Q_MAX_NUM_MSGS (10)
#define MSG_Q_ALIGN (2)
#define PRIORITY 5
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
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

#if defined(CONFIG_X86)
#define MEM_REGION_ALLOC (4096)
#elif defined(CONFIG_ARC)
#define MEM_REGION_ALLOC (Z_ARC_MPU_ALIGN)
#elif defined(CONFIG_ARM)
#define MEM_REGION_ALLOC (Z_THREAD_MIN_STACK_ALIGN)
#else
#error "Test suite not compatible for the given architecture"
#endif
#define MEM_DOMAIN_ALIGNMENT __aligned(MEM_REGION_ALLOC)

/* for kobject.c */
#define KOBJECT_STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)

#ifndef _TEST_SYSCALLS_H_
#define _TEST_SYSCALLS_H_

__syscall struct k_mem_pool *ret_resource_pool_ptr(void);

#include <syscalls/mem_protect.h>

#endif /* _TEST_SYSCALLS_H_ */
