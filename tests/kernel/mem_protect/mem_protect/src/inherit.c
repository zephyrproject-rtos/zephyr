/*
 * Copyright (c) 2017, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"
#include <zephyr/syscall_handler.h>
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */

/* function prototypes */
static inline void dummy_start(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}
static inline void dummy_end(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}

/* Kernel objects */
K_THREAD_STACK_DEFINE(test_1_stack, INHERIT_STACK_SIZE);
K_THREAD_STACK_DEFINE(parent_thr_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(child_thr_stack, STACK_SIZE);
K_HEAP_DEFINE(heap_mem, BLK_SIZE_MAX * BLK_NUM_MAX);
K_SEM_DEFINE(inherit_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_SEM_DEFINE(sync_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_MUTEX_DEFINE(inherit_mutex);
K_TIMER_DEFINE(inherit_timer, dummy_start, dummy_end);
K_MSGQ_DEFINE(inherit_msgq, MSG_Q_SIZE, MSG_Q_MAX_NUM_MSGS, MSG_Q_ALIGN);
struct k_thread test_1_tid, parent_thr, child_thr;
k_tid_t parent_tid;

uint8_t MEM_DOMAIN_ALIGNMENT inherit_buf[MEM_REGION_ALLOC]; /* for mem domain */

K_MEM_PARTITION_DEFINE(inherit_memory_partition,
		       inherit_buf,
		       sizeof(inherit_buf),
		       K_MEM_PARTITION_P_RW_U_RW);

struct k_mem_partition *inherit_memory_partition_array[] = {
#if Z_LIBC_PARTITION_EXISTS
	&z_libc_partition,
#endif
	&inherit_memory_partition,
	&ztest_mem_partition
};

struct k_mem_domain inherit_mem_domain;

/* generic function to do check the access permissions. */
static void access_test(void)
{
	uint32_t msg_q_data = 0xA5A5;

	/* check for all accesses  */
	k_sem_give(&inherit_sem);
	k_mutex_lock(&inherit_mutex, K_FOREVER);
	(void) k_timer_status_get(&inherit_timer);
	k_msgq_put(&inherit_msgq, (void *)&msg_q_data, K_NO_WAIT);
	k_mutex_unlock(&inherit_mutex);
	inherit_buf[MEM_REGION_ALLOC-1] = 0xA5;
}

static void test_thread_1_for_user(void *p1, void *p2, void *p3)
{
	/* check that child thread inherited permissions */
	access_test();

	set_fault_valid(true);

	/* Check that child thread can't access parent thread object*/
	/* Kernel fault in that place will happen */
	k_thread_priority_get(parent_tid);
}

static void test_thread_1_for_SU(void *p1, void *p2, void *p3)
{
	set_fault_valid(false);

	access_test();

	/* Check if user mode inherit is working if control is passed from SU */
	k_thread_user_mode_enter(test_thread_1_for_user, NULL, NULL, NULL);
}

/**
 * @brief Test object permission inheritance except of the parent thread object
 *
 * @details
 * - To the parent current thread grant permissions on kernel objects.
 * - Create a child thread and check that it inherited permissions on that
 *   kernel objects.
 * - Then check child thread can't access to the parent thread object using API
 *   command k_thread_priority_get()
 * - At the same moment that test verifies that child thread was granted
 *   permission on a kernel objects. That means child user thread caller
 *   already has permission on the thread objects being granted.

 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_init(), k_mem_domain_add_thread(),
 * k_thread_access_grant()
 */
ZTEST(mem_protect, test_permission_inheritance)
{
	parent_tid = k_current_get();
	k_mem_domain_add_thread(&inherit_mem_domain, parent_tid);

	k_thread_access_grant(parent_tid,
			      &inherit_sem,
			      &inherit_mutex,
			      &inherit_timer,
			      &inherit_msgq, &test_1_stack);

	k_thread_create(&test_1_tid,
			test_1_stack,
			INHERIT_STACK_SIZE,
			test_thread_1_for_SU,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(&test_1_tid, K_FOREVER);
}

struct k_heap *z_impl_ret_resource_pool_ptr(void)
{
	return _current->resource_pool;
}

static inline struct k_heap *z_vrfy_ret_resource_pool_ptr(void)
{
	return z_impl_ret_resource_pool_ptr();
}
#include <syscalls/ret_resource_pool_ptr_mrsh.c>
struct k_heap *child_heap_mem_ptr;
struct k_heap *parent_heap_mem_ptr;

void child_handler(void *p1, void *p2, void *p3)
{
	child_heap_mem_ptr = ret_resource_pool_ptr();
	k_sem_give(&sync_sem);
}

void parent_handler(void *p1, void *p2, void *p3)
{
	parent_heap_mem_ptr = ret_resource_pool_ptr();
	k_thread_create(&child_thr, child_thr_stack,
			K_THREAD_STACK_SIZEOF(child_thr_stack),
			child_handler,
			NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);

	k_thread_join(&child_thr, K_FOREVER);
}

/**
 * @brief Test child thread inherits parent's thread resource pool
 *
 * @details
 * - Create a memory heap heap_mem for the parent thread.
 * - Then special system call ret_resource_pool_ptr() returns pointer
 *   to the resource pool of the current thread.
 * - Call it in the parent_handler() and in the child_handler()
 * - Then in the main test function test_inherit_resource_pool()
 *   compare returned addresses
 * - If the addresses are the same, it means that child thread inherited
 *   resource pool of the parent's thread -test passed.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see z_thread_heap_assign()
 */
ZTEST(mem_protect, test_inherit_resource_pool)
{
	k_sem_reset(&sync_sem);
	k_thread_create(&parent_thr, parent_thr_stack,
			K_THREAD_STACK_SIZEOF(parent_thr_stack),
			parent_handler,
			NULL, NULL, NULL,
			PRIORITY, 0, K_FOREVER);
	k_thread_heap_assign(&parent_thr, &heap_mem);
	k_thread_start(&parent_thr);
	k_sem_take(&sync_sem, K_FOREVER);
	zassert_true(parent_heap_mem_ptr == child_heap_mem_ptr,
		     "Resource pool of the parent thread not inherited,"
		     " by child thread");

	k_thread_join(&parent_thr, K_FOREVER);
}

void mem_protect_inhert_setup(void)
{
	int ret;

	ret = k_mem_domain_init(&inherit_mem_domain,
				ARRAY_SIZE(inherit_memory_partition_array),
				inherit_memory_partition_array);
	if (ret != 0) {
		ztest_test_fail();
	}
}


K_HEAP_DEFINE(test_mem_heap, TEST_HEAP_SIZE);

void *mem_protect_setup(void)
{
	k_thread_priority_set(k_current_get(), -1);

	k_thread_heap_assign(k_current_get(), &test_mem_heap);

	mem_protect_inhert_setup();

	return NULL;
}

ZTEST_SUITE(mem_protect, NULL, mem_protect_setup,
		NULL, NULL, NULL);
