/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <kernel_structs.h>
#include <irq_offload.h>

#define STACKSIZE 2048
#define MAIN_PRIORITY 7
#define PRIORITY 5

/* This test performs validation of the core object validation code in
 * kernel/object_validation.c, showing that the validation works for
 * semaphore APIs.
 *
 * This test does not ensure that all kernel object APIs are correctly
 * connected to this mechanism. To do this would probably require repeating
 * this test for all kernel object types, for all their APIs. However,
 * in the fullness of time we expect to enable this feature by default
 * for the QEMU targets, and exercise validation of all kernel objects
 * as a part of normal sanitycheck runs.
 */

static K_THREAD_STACK_DEFINE(alt_stack, STACKSIZE);
static __kernel struct k_thread alt_thread;

static volatile int rv = TC_PASS;

/* ARM is a special case, in that k_thread_abort() does indeed return
 * instead of calling _Swap() directly. The PendSV exception is queued
 * and immediately fires upon completing the exception path; the faulting
 * thread is never run again.
 */
#ifndef CONFIG_ARM
FUNC_NORETURN
#endif
void _SysFatalErrorHandler(unsigned int reason, const NANO_ESF *pEsf)
{
	TC_PRINT("Caught system error -- reason %d\n", reason);
	if (reason != _NANO_ERR_KERNEL_OOPS) {
		TC_ERROR("Unexpected fata error type %d\n", reason);
		rv = TC_FAIL;
	}
	TC_PRINT("Got a kernel oops as expected, PASS\n");
	k_thread_abort(_current);
#ifndef CONFIG_ARM
	CODE_UNREACHABLE;
#endif
}


K_SEM_DEFINE(sem1, 0, 1);
static __kernel struct k_sem sem2;
static __kernel char bad_sem[sizeof(struct k_sem)];
#ifdef CONFIG_APPLICATION_MEMORY
static struct k_sem sem3;
#endif

static void bad_mem_range(void)
{
	k_sem_give((struct k_sem *)0xFFFFFFFF);
}

static void bad_kernel_object(void)
{
	k_sem_give((struct k_sem *)bad_sem);
}

#ifdef CONFIG_APPLICATION_MEMORY
static void app_memory_space(void)
{
	k_sem_init(&sem3, 0, 1);
}
#endif

static void thread_wrapper(void *(*func)(void), void *unused1, void *unused2)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	func();

	TC_ERROR("test did not trigger kernel oops as expected\n");
	rv = TC_FAIL;
}


static void thread_offload(void *func)
{
	k_thread_create(&alt_thread, alt_stack,
			K_THREAD_STACK_SIZEOF(alt_stack),
			(k_thread_entry_t)thread_wrapper,
			func, NULL, NULL, K_PRIO_COOP(PRIORITY), 0,
			K_NO_WAIT);
}

void main(void)
{
	TC_START("obj_validation");

	k_thread_priority_set(_current, K_PRIO_PREEMPT(MAIN_PRIORITY));

	TC_PRINT("Test operation on valid kernel object\n");

	/* Should succeed without incident */
	k_sem_give(&sem1);
	k_sem_init(&sem2, 0, 1);
	k_sem_give(&sem2);
	TC_PRINT("Semaphores initialized without incident, PASS\n");

	TC_PRINT("\nTest operation on bad memory range\n");
	thread_offload(bad_mem_range);

	TC_PRINT("\nTest operation on bad kernel object\n");
	thread_offload(bad_kernel_object);

#ifdef CONFIG_APPLICATION_MEMORY
	TC_PRINT("\nTest kernel object in app memory space\n");
	thread_offload(app_memory_space);
#endif

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}

