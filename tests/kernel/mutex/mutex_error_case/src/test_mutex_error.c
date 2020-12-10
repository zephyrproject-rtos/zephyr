/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <irq_offload.h>

#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define THREAD_TEST_PRIORITY 5

/* use to pass case type to threads */
static ZTEST_DMEM int case_type;

static struct k_mutex mutex;

static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

/* enumerate our negative case scenario */
enum {
	MUTEX_INIT_NULL,
	MUTEX_LOCK_NULL,
	MUTEX_UNLOCK_NULL,
	MUTEX_LOCK_IN_ISR,
	MUTEX_UNLOCK_IN_ISR,
	MUTEX_UNLOCK_COUNT_UNMET,
	NOT_DEFINE
} neg_case;

/* This is a semaphore using inside irq_offload */
extern struct k_sem offload_sem;


/* action after self-defined fatal handler. */
static ZTEST_BMEM volatile bool valid_fault;

static inline void set_fault_valid(bool valid)
{
	valid_fault = valid;
}

static void action_after_assert_fail(void *param)
{
	int choice = *((int *)param);

	switch (choice) {
	case MUTEX_LOCK_IN_ISR:
	case MUTEX_UNLOCK_IN_ISR:
		/* Semaphore used inside irq_offload need to be
		 * released after assert or fault happened.
		 */
		k_sem_give(&offload_sem);
		ztest_test_pass();
		break;

	case MUTEX_UNLOCK_COUNT_UNMET:
		ztest_test_pass();
		break;

	default:
		ztest_test_fail();
		break;
	}
}

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	printk("Caught system error -- reason %d %d\n", reason, valid_fault);
	if (valid_fault) {
		printk("Fatal error expected as part of test case.\n");
		valid_fault = false; /* reset back to normal */
	} else {
		printk("Fatal error was unexpected, aborting...\n");
		k_fatal_halt(reason);
	}
}
/* end of self-defined fatal handler */

/* action for self-defined assert handler. */
static ZTEST_DMEM volatile bool valid_assert;

static inline void set_assert_valid(bool valid)
{
	valid_assert = valid;
}

#ifdef CONFIG_ASSERT_NO_FILE_INFO
void assert_post_action(void)
#else
void assert_post_action(const char *file, unsigned int line)
#endif
{
#ifndef CONFIG_ASSERT_NO_FILE_INFO
	ARG_UNUSED(file);
	ARG_UNUSED(line);
#endif

	printk("Caught assert failed\n");

	if (valid_assert) {
		valid_assert = false; /* reset back to normal */
		printk("Assert error expected as part of test case.\n");

		/* do some action after fatal error happened */
		action_after_assert_fail(&case_type);
	} else {
		printk("Assert failed was unexpected, aborting...\n");
#ifdef CONFIG_USERSPACE
	/* User threads aren't allowed to induce kernel panics; generate
	 * an oops instead.
	 */
		if (_is_user_context()) {
			k_oops();
		}
#endif
		k_panic();
	}
}
/* end of self-defined assert handler */

static void tIsr_entry_lock(const void *p)
{
	k_mutex_lock((struct k_mutex *)p, K_NO_WAIT);
}

static void tIsr_entry_unlock(const void *p)
{
	k_mutex_unlock((struct k_mutex *)p);
}

static void tThread_entry_negative(void *p1, void *p2, void *p3)
{
	int choice = *((int *)p2);

	TC_PRINT("current case is %d\n", choice);

	/* Set up the fault or assert are expected before we call
	 * the target tested funciton.
	 */
	switch (choice) {
	case MUTEX_INIT_NULL:
		set_fault_valid(true);
		k_mutex_init(NULL);
		break;
	case MUTEX_LOCK_NULL:
		set_fault_valid(true);
		k_mutex_lock(NULL, K_NO_WAIT);
		break;
	case MUTEX_UNLOCK_NULL:
		set_fault_valid(true);
		k_mutex_unlock(NULL);
		break;
	case MUTEX_LOCK_IN_ISR:
		k_mutex_init(&mutex);
		set_assert_valid(true);
		irq_offload(tIsr_entry_lock, (void *)p1);
		break;
	case MUTEX_UNLOCK_IN_ISR:
		k_mutex_init(&mutex);
		set_assert_valid(true);
		irq_offload(tIsr_entry_unlock, (void *)p1);
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

	if (_is_user_context()) {
		perm = perm | K_USER;
	}

	case_type = choice;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)tThread_entry_negative,
			&mutex, (void *)&case_type, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			perm, K_NO_WAIT);

	ret = k_thread_join(tid, K_FOREVER);

	return ret;
}

/* TESTPOINT: Pass a null pointer into the API k_mutex_init */
static void test_mutex_init_null(void)
{
	create_negative_test_thread(MUTEX_INIT_NULL);
}
/* TESTPOINT: Pass a null pointer into the API k_mutex_lock */
static void test_mutex_lock_null(void)
{
	create_negative_test_thread(MUTEX_LOCK_NULL);
}

/* TESTPOINT: Pass a null pointer into the API k_mutex_unlock */
static void test_mutex_unlock_null(void)
{
	create_negative_test_thread(MUTEX_UNLOCK_NULL);
}

/* TESTPOINT: Try to lock mutex in isr context */
static void test_mutex_lock_in_isr(void)
{
	create_negative_test_thread(MUTEX_LOCK_IN_ISR);
}

/* TESTPOINT: Try to unlock mutex in isr context */
static void test_mutex_unlock_in_isr(void)
{
	create_negative_test_thread(MUTEX_UNLOCK_IN_ISR);
}

static void test_mutex_unlock_count_unmet(void)
{
	struct k_mutex tmutex;

	case_type = MUTEX_UNLOCK_COUNT_UNMET;

	k_mutex_init(&tmutex);
	zassert_true(k_mutex_lock(&tmutex, K_FOREVER) == 0,
			"current thread failed to lock the mutex");
	/* Verify an assertion will be trigger if lock_count is zero while
	 * the lock owner try to unlock mutex.
	 */
	tmutex.lock_count = 0;
	set_assert_valid(true);
	k_mutex_unlock(&tmutex);
}


/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, &tstack, &mutex);

	ztest_test_suite(mutex_api,
		 ztest_unit_test(test_mutex_lock_in_isr),
		 ztest_unit_test(test_mutex_unlock_in_isr),
		 ztest_user_unit_test(test_mutex_init_null),
		 ztest_user_unit_test(test_mutex_lock_null),
		 ztest_user_unit_test(test_mutex_unlock_null),
		 ztest_unit_test(test_mutex_unlock_count_unmet)
		 );
	ztest_run_test_suite(mutex_api);
}
