/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mem_protect.h"

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
K_SEM_DEFINE(inherit_sem, SEMAPHORE_INIT_COUNT, SEMAPHORE_MAX_COUNT);
K_MUTEX_DEFINE(inherit_mutex);
K_TIMER_DEFINE(inherit_timer, dummy_start, dummy_end);
K_MSGQ_DEFINE(inherit_msgq, MSG_Q_SIZE, MSG_Q_MAX_NUM_MSGS, MSG_Q_ALIGN);
__kernel struct k_thread test_1_tid;

u8_t MEM_DOMAIN_ALIGNMENT inherit_buf[MEM_REGION_ALLOC]; /* for mem domain */

K_MEM_PARTITION_DEFINE(inherit_memory_partition,
		       inherit_buf,
		       sizeof(inherit_buf),
		       K_MEM_PARTITION_P_RW_U_RW);

struct k_mem_partition *inherit_memory_partition_array[] = {
	&inherit_memory_partition
};

__kernel struct k_mem_domain inherit_mem_domain;

/* generic function to do check the access permissions. */
void access_test(void)
{
	u32_t msg_q_data = 0xA5A5;

	/* check for all accesses  */
	k_sem_give(&inherit_sem);
	k_mutex_lock(&inherit_mutex, K_FOREVER);
	(void) k_timer_status_get(&inherit_timer);
	k_msgq_put(&inherit_msgq, (void *)&msg_q_data, K_NO_WAIT);
	k_mutex_unlock(&inherit_mutex);
	inherit_buf[10] = 0xA5;
}

void test_thread_1_for_user(void *p1, void *p2, void *p3)
{
	access_test();
	ztest_test_pass();
}

void test_thread_1_for_SU(void *p1, void *p2, void *p3)
{
	valid_fault = false;
	USERSPACE_BARRIER;

	access_test();

	/* Check if user mode inherit is working if control is passed from SU */
	k_thread_user_mode_enter(test_thread_1_for_user, NULL, NULL, NULL);
}

/**
 * @brief Test object permission inheritance
 *
 * @ingroup kernel_memprotect_tests
 *
 * @see k_mem_domain_init(), k_mem_domain_add_thread(),
 * k_thread_access_grant()
 */
void test_permission_inheritance(void *p1, void *p2, void *p3)
{

	k_mem_domain_init(&inherit_mem_domain,
			  1,
			  inherit_memory_partition_array);

	k_mem_domain_add_thread(&inherit_mem_domain, k_current_get());

	k_thread_access_grant(k_current_get(),
			      &inherit_sem,
			      &inherit_mutex,
			      &inherit_timer,
			      &inherit_msgq, &test_1_stack, NULL);

	k_thread_create(&test_1_tid,
			test_1_stack,
			INHERIT_STACK_SIZE,
			test_thread_1_for_SU,
			NULL, NULL, NULL,
			0, K_INHERIT_PERMS, K_NO_WAIT);

	k_sem_take(&sync_sem, SYNC_SEM_TIMEOUT);
}
