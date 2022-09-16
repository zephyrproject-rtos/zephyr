/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_TEST_PRIORITY 5

/* use to pass case type to threads */
static ZTEST_DMEM int case_type;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static K_THREAD_STACK_DEFINE(test_stack, STACK_SIZE);
static struct k_thread tdata;
static struct k_thread test_tdata;

/* enumerate our negative case scenario */
enum {
	THREAD_START,
	FLOAT_DISABLE,
	TIMEOUT_REMAINING_TICKS,
	TIMEOUT_EXPIRES_TICKS,
	THREAD_CREATE_NEWTHREAD_NULL,
	THREAD_CREATE_STACK_NULL,
	THREAD_CTEATE_STACK_SIZE_OVERFLOW
} neg_case;

static void test_thread(void *p1, void *p2, void *p3)
{
	/* do nothing here */
}

static void tThread_entry_negative(void *p1, void *p2, void *p3)
{
	int choice = *((int *)p1);
	uint32_t perm = K_INHERIT_PERMS;

	TC_PRINT("current case is %d\n", choice);

	/* Set up the fault or assert are expected before we call
	 * the target tested function.
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
	case THREAD_CTEATE_STACK_SIZE_OVERFLOW:
		ztest_set_fault_valid(true);
		if (k_is_user_context()) {
			perm = perm | K_USER;
		}
		k_thread_create(&test_tdata, test_stack, -1,
			test_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);
		break;
	default:
		TC_PRINT("should not be here!\n");
		break;
	}

	/* If negative comes here, it means error condition not been
	 * detected.
	 */
	ztest_test_fail();
}

static void create_negative_test_thread(int choice)
{
	int ret;
	uint32_t perm = K_INHERIT_PERMS;

	if (k_is_user_context()) {
		perm = perm | K_USER;
	}

	case_type = choice;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)tThread_entry_negative,
			(void *)&case_type, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);

	ret = k_thread_join(tid, K_FOREVER);
}

/* TESTPOINT: Pass a null pointer into the API k_thread_start() */
static void test_thread_start(void)
{
	create_negative_test_thread(THREAD_START);
}

/* TESTPOINT: Pass a null pointer into the API k_float_disable() */
static void test_float_disable(void)
{
	create_negative_test_thread(FLOAT_DISABLE);
}

/* TESTPOINT: Pass a null pointer into the API */
static void test_timeout_remaining_ticks(void)
{
	create_negative_test_thread(TIMEOUT_REMAINING_TICKS);
}

/* TESTPOINT: Pass a null pointer into the API */
static void test_timeout_expires_ticks(void)
{
	create_negative_test_thread(TIMEOUT_EXPIRES_TICKS);
}

/* TESTPOINT: Pass new thread with NULL into API */
static void test_thread_create_uninit(void)
{
	create_negative_test_thread(THREAD_CREATE_NEWTHREAD_NULL);
}

/* TESTPOINT: Pass a NULL stack into API */
static void test_thread_create_stack_null(void)
{
	create_negative_test_thread(THREAD_CREATE_STACK_NULL);
}

/* TESTPOINT: Pass a overflow stack into API */
static void test_thread_create_stack_overflow(void)
{
	create_negative_test_thread(THREAD_CTEATE_STACK_SIZE_OVERFLOW);
}

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, &tstack, &test_tdata, &test_stack);

	ztest_test_suite(thread_error_case,
		 ztest_user_unit_test(test_float_disable),
		 ztest_user_unit_test(test_timeout_remaining_ticks),
		 ztest_user_unit_test(test_timeout_expires_ticks),
		 ztest_user_unit_test(test_thread_create_uninit),
		 ztest_user_unit_test(test_thread_create_stack_null),
		 ztest_user_unit_test(test_thread_create_stack_overflow),
		 ztest_user_unit_test(test_thread_start)
		 );
	ztest_run_test_suite(thread_error_case);
}
