/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest_error_hook.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define THREAD_TEST_PRIORITY 5

/* use to pass case type to threads */
static ZTEST_DMEM int case_type;

static struct k_mutex mutex;
static struct k_sem sem;
static struct k_pipe pipe;
static struct k_queue queue;


static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

/* enumerate our negative case scenario */
enum {
	MUTEX_INIT_NULL,
	MUTEX_INIT_INVALID_OBJ,
	MUTEX_LOCK_NULL,
	MUTEX_LOCK_INVALID_OBJ,
	MUTEX_UNLOCK_NULL,
	MUTEX_UNLOCK_INVALID_OBJ,
	NOT_DEFINE
} neg_case;

/* This is a semaphore using inside irq_offload */
extern struct k_sem offload_sem;

/* A call back function which is hooked in default assert handler. */
void ztest_post_fatal_error_hook(unsigned int reason,
		const struct arch_esf *pEsf)

{
	/* check if expected error */
	zassert_equal(reason, K_ERR_KERNEL_OOPS);
}

static void tThread_entry_negative(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p3);

	int choice = *((int *)p2);

	TC_PRINT("current case is %d\n", choice);

	/* Set up the fault or assert are expected before we call
	 * the target tested function.
	 */
	switch (choice) {
	case MUTEX_INIT_NULL:
		ztest_set_fault_valid(true);
		k_mutex_init(NULL);
		break;
	case MUTEX_INIT_INVALID_OBJ:
		ztest_set_fault_valid(true);
		k_mutex_init((struct k_mutex *)&sem);
		break;
	case MUTEX_LOCK_NULL:
		ztest_set_fault_valid(true);
		k_mutex_lock(NULL, K_NO_WAIT);
		break;
	case MUTEX_LOCK_INVALID_OBJ:
		ztest_set_fault_valid(true);
		k_mutex_lock((struct k_mutex *)&pipe, K_NO_WAIT);
		break;
	case MUTEX_UNLOCK_NULL:
		ztest_set_fault_valid(true);
		k_mutex_unlock(NULL);
		break;
	case MUTEX_UNLOCK_INVALID_OBJ:
		ztest_set_fault_valid(true);
		k_mutex_unlock((struct k_mutex *)&queue);
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

static int create_negative_test_thread(int choice)
{
	int ret;
	uint32_t perm = K_INHERIT_PERMS;

	if (k_is_user_context()) {
		perm = perm | K_USER;
	}

	case_type = choice;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			tThread_entry_negative,
			&mutex, (void *)&case_type, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);

	ret = k_thread_join(tid, K_FOREVER);

	return ret;
}


/**
 * @brief Test initializing mutex with a NULL pointer
 *
 * @details Pass a null pointer as parameter, then see if the
 * expected error happens.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_init()
 */
ZTEST_USER(mutex_api_error, test_mutex_init_null)
{
	create_negative_test_thread(MUTEX_INIT_NULL);
}

/**
 * @brief Test initialize mutex with a invalid kernel object
 *
 * @details Pass a invalid kobject as parameter, then see if the
 * expected error happens.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_init()
 */
ZTEST_USER(mutex_api_error, test_mutex_init_invalid_obj)
{
	create_negative_test_thread(MUTEX_INIT_INVALID_OBJ);
}

/**
 * @brief Test locking mutex with a NULL pointer
 *
 * @details Pass a null pointer as parameter, then see if the
 * expected error happens.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_lock()
 */
ZTEST_USER(mutex_api_error, test_mutex_lock_null)
{
	create_negative_test_thread(MUTEX_LOCK_NULL);
}

/**
 * @brief Test locking mutex with a invalid kernel object
 *
 * @details Pass a invalid kobject as parameter, then see if the
 * expected error happens.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_lock()
 */
/* TESTPOINT: Pass a invalid kobject into the API k_mutex_lock */
ZTEST_USER(mutex_api_error, test_mutex_lock_invalid_obj)
{
	create_negative_test_thread(MUTEX_LOCK_INVALID_OBJ);
}

/**
 * @brief Test unlocking mutex with a NULL pointer
 *
 * @details Pass a null pointer as parameter, then see if the
 * expected error happens.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_unlock()
 */
ZTEST_USER(mutex_api_error, test_mutex_unlock_null)
{
	create_negative_test_thread(MUTEX_UNLOCK_NULL);
}

/**
 * @brief Test unlocking mutex with a invalid kernel object
 *
 * @details Pass a invalid kobject as parameter, then see if the
 * expected error happens.
 *
 * @ingroup kernel_mutex_tests
 *
 * @see k_mutex_unlock()
 */
/* TESTPOINT: Pass a invalid kobject into the API k_mutex_unlock */
ZTEST_USER(mutex_api_error, test_mutex_unlock_invalid_obj)
{
	create_negative_test_thread(MUTEX_UNLOCK_INVALID_OBJ);
}

static void *mutex_api_tests_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(), &tdata, &tstack,
		       &mutex, &sem, &pipe, &queue);
#endif
	return NULL;
}

ZTEST_SUITE(mutex_api_error, NULL, mutex_api_tests_setup, NULL, NULL, NULL);
