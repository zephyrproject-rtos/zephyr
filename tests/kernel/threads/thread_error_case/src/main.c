/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_TEST_PRIORITY 5

/* selected negative test case, shared with the worker thread */
static ZTEST_DMEM int case_type;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(test_stack, STACK_SIZE);
static struct k_thread tdata;
static struct k_thread test_tdata;

/* identifiers for each negative test scenario */
enum {
	THREAD_START,
	FLOAT_DISABLE,
	TIMEOUT_REMAINING_TICKS,
	TIMEOUT_EXPIRES_TICKS,
	THREAD_CREATE_NEWTHREAD_NULL,
	THREAD_CREATE_STACK_NULL,
	THREAD_CREATE_STACK_SIZE_OVERFLOW,
	THREAD_SUSPEND_NULL,
	THREAD_RESUME_NULL,
	THREAD_PRIORITY_SET_NULL,
	THREAD_PRIORITY_GET_NULL,
	THREAD_WAKEUP_NULL,
	THREAD_CREATE_SUPERVISOR,
	THREAD_CREATE_ESSENTIAL
} neg_case;

static void test_thread(void *p1, void *p2, void *p3)
{
	/* intentionally empty target thread */
}

static void tThread_entry_negative(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	int choice = *((int *)p1);
	uint32_t perm = K_INHERIT_PERMS;

	/* Arm the fatal-error hook before calling the API under test.
	 * Each case must not return; control resumes after the switch
	 * only if the error was not detected, which fails the test.
	 */
	switch (choice) {
	case THREAD_START:
		ztest_set_fault_valid(true);
		k_thread_start(NULL);
		break;
	case FLOAT_DISABLE:
		ztest_set_fault_valid(true);
		k_float_disable(NULL);
		break;
	case TIMEOUT_REMAINING_TICKS:
		ztest_set_fault_valid(true);
		k_thread_timeout_remaining_ticks(NULL);
		break;
	case TIMEOUT_EXPIRES_TICKS:
		ztest_set_fault_valid(true);
		k_thread_timeout_expires_ticks(NULL);
		break;
	case THREAD_CREATE_NEWTHREAD_NULL:
		ztest_set_fault_valid(true);
		if (k_is_user_context()) {
			perm = perm | K_USER;
		}

		k_thread_create((struct k_thread *)NULL, test_stack, STACK_SIZE,
			test_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);
		break;
	case THREAD_CREATE_STACK_NULL:
		ztest_set_fault_valid(true);
		if (k_is_user_context()) {
			perm = perm | K_USER;
		}

		k_thread_create(&test_tdata, NULL, STACK_SIZE,
			test_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);
		break;
	case THREAD_CREATE_STACK_SIZE_OVERFLOW:
		ztest_set_fault_valid(true);
		if (k_is_user_context()) {
			perm = perm | K_USER;
		}
		k_thread_create(&test_tdata, test_stack, -1,
			test_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);
		break;
	case THREAD_SUSPEND_NULL:
		ztest_set_fault_valid(true);
		k_thread_suspend(NULL);
		break;
	case THREAD_RESUME_NULL:
		ztest_set_fault_valid(true);
		k_thread_resume(NULL);
		break;
	case THREAD_PRIORITY_SET_NULL:
		ztest_set_fault_valid(true);
		k_thread_priority_set(NULL, K_PRIO_PREEMPT(THREAD_TEST_PRIORITY));
		break;
	case THREAD_PRIORITY_GET_NULL:
		ztest_set_fault_valid(true);
		k_thread_priority_get(NULL);
		break;
	case THREAD_WAKEUP_NULL:
		ztest_set_fault_valid(true);
		k_wakeup(NULL);
		break;
	case THREAD_CREATE_SUPERVISOR:
		/* Creating a supervisor thread (K_USER flag absent) is
		 * only rejected in user context; skip otherwise.
		 */
		if (!k_is_user_context()) {
			ztest_test_skip();
			break;
		}
		ztest_set_fault_valid(true);
		k_thread_create(&test_tdata, test_stack, STACK_SIZE,
			test_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_INHERIT_PERMS, K_NO_WAIT);
		break;
	case THREAD_CREATE_ESSENTIAL:
		/* Creating an essential thread (K_ESSENTIAL flag) is
		 * only rejected in user context; skip otherwise.
		 */
		if (!k_is_user_context()) {
			ztest_test_skip();
			break;
		}
		ztest_set_fault_valid(true);
		k_thread_create(&test_tdata, test_stack, STACK_SIZE,
			test_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_ESSENTIAL, K_NO_WAIT);
		break;
	default:
		TC_PRINT("should not be here!\n");
		break;
	}

	/* Reaching here means the expected error was not detected. */
	ztest_test_fail();
}

static void create_negative_test_thread(int choice)
{
	uint32_t perm = K_INHERIT_PERMS;

	if (k_is_user_context()) {
		perm = perm | K_USER;
	}

	case_type = choice;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			tThread_entry_negative,
			(void *)&case_type, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);

	(void)k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test that k_thread_start() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_thread_start() triggers
 * the expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_start()
 */
ZTEST_USER(thread_error_case, test_thread_start)
{
	create_negative_test_thread(THREAD_START);
}

/**
 * @brief Test that k_float_disable() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_float_disable() triggers
 * the expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_float_disable()
 */
ZTEST_USER(thread_error_case, test_float_disable)
{
	create_negative_test_thread(FLOAT_DISABLE);
}

/**
 * @brief Test that k_thread_timeout_remaining_ticks() rejects a NULL pointer
 *
 * Verifies that passing a NULL thread pointer to
 * k_thread_timeout_remaining_ticks() triggers the expected fatal error
 * in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_timeout_remaining_ticks()
 */
ZTEST_USER(thread_error_case, test_timeout_remaining_ticks)
{
	create_negative_test_thread(TIMEOUT_REMAINING_TICKS);
}

/**
 * @brief Test that k_thread_timeout_expires_ticks() rejects a NULL pointer
 *
 * Verifies that passing a NULL thread pointer to
 * k_thread_timeout_expires_ticks() triggers the expected fatal error
 * in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_timeout_expires_ticks()
 */
ZTEST_USER(thread_error_case, test_timeout_expires_ticks)
{
	create_negative_test_thread(TIMEOUT_EXPIRES_TICKS);
}

/**
 * @brief Test that k_thread_create() rejects a NULL new-thread pointer
 *
 * Verifies that passing NULL as the new thread object to k_thread_create()
 * triggers the expected fatal error. The syscall verifier requires the
 * thread object to be a valid, uninitialized kernel object.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_create()
 */
ZTEST_USER(thread_error_case, test_thread_create_uninit)
{
	create_negative_test_thread(THREAD_CREATE_NEWTHREAD_NULL);
}

/**
 * @brief Test that k_thread_create() rejects a NULL stack pointer
 *
 * Verifies that passing NULL as the stack argument to k_thread_create()
 * triggers the expected fatal error. The syscall verifier requires the
 * stack to be a valid, uninitialized kernel stack object.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_create()
 */
ZTEST_USER(thread_error_case, test_thread_create_stack_null)
{
	create_negative_test_thread(THREAD_CREATE_STACK_NULL);
}

/**
 * @brief Test that k_thread_create() rejects an overflowing stack size
 *
 * Verifies that passing SIZE_MAX (-1 cast to size_t) as the stack size
 * to k_thread_create() triggers the expected fatal error. The syscall
 * verifier detects the overflow when computing
 * K_THREAD_STACK_RESERVED + stack_size.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_create()
 */
ZTEST_USER(thread_error_case, test_thread_create_stack_overflow)
{
	create_negative_test_thread(THREAD_CREATE_STACK_SIZE_OVERFLOW);
}

/**
 * @brief Test that k_thread_suspend() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_thread_suspend() triggers
 * the expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_suspend()
 */
ZTEST_USER(thread_error_case, test_thread_suspend_null)
{
	create_negative_test_thread(THREAD_SUSPEND_NULL);
}

/**
 * @brief Test that k_thread_resume() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_thread_resume() triggers
 * the expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_resume()
 */
ZTEST_USER(thread_error_case, test_thread_resume_null)
{
	create_negative_test_thread(THREAD_RESUME_NULL);
}

/**
 * @brief Test that k_thread_priority_set() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_thread_priority_set() triggers
 * the expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_priority_set()
 */
ZTEST_USER(thread_error_case, test_thread_priority_set_null)
{
	create_negative_test_thread(THREAD_PRIORITY_SET_NULL);
}

/**
 * @brief Test that k_thread_priority_get() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_thread_priority_get() triggers
 * the expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_priority_get()
 */
ZTEST_USER(thread_error_case, test_thread_priority_get_null)
{
	create_negative_test_thread(THREAD_PRIORITY_GET_NULL);
}

/**
 * @brief Test that k_wakeup() rejects a NULL thread pointer
 *
 * Verifies that passing a NULL pointer to k_wakeup() triggers the
 * expected fatal error in the syscall validation layer.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_wakeup()
 */
ZTEST_USER(thread_error_case, test_thread_wakeup_null)
{
	create_negative_test_thread(THREAD_WAKEUP_NULL);
}

/**
 * @brief Test that k_thread_create() forbids creating supervisor threads
 *        from user context
 *
 * Verifies that a user thread cannot create a supervisor thread by omitting
 * the K_USER option flag. The syscall verifier in z_vrfy_k_thread_create()
 * enforces that the K_USER flag must be set for all threads spawned from
 * user context.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_create()
 */
ZTEST_USER(thread_error_case, test_thread_create_supervisor)
{
	create_negative_test_thread(THREAD_CREATE_SUPERVISOR);
}

/**
 * @brief Test that k_thread_create() forbids creating essential threads
 *        from user context
 *
 * Verifies that a user thread cannot create an essential thread by setting
 * the K_ESSENTIAL option flag. The syscall verifier in
 * z_vrfy_k_thread_create() rejects this to prevent unprivileged code from
 * creating threads whose abort would trigger a kernel panic.
 *
 * @ingroup kernel_thread_tests
 *
 * @see k_thread_create()
 */
ZTEST_USER(thread_error_case, test_thread_create_essential)
{
	create_negative_test_thread(THREAD_CREATE_ESSENTIAL);
}

/* Grant the test thread access to all shared kernel objects. */
void *thread_grant_setup(void)
{
	k_thread_access_grant(k_current_get(), &tdata, &tstack, &test_tdata, &test_stack);

	return NULL;
}

ZTEST_SUITE(thread_error_case, NULL, thread_grant_setup, NULL, NULL, NULL);
