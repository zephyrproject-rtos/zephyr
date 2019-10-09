/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>

#define TIMEOUT 100
#define STACK_SIZE 512
#define SEM_INITIAL 0
#define SEM_LIMIT 2
/**TESTPOINT: init via K_SEM_DEFINE*/
K_SEM_DEFINE(ksema, SEM_INITIAL, SEM_LIMIT);
struct k_sem sema;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
struct k_thread tdata;

/*entry of contexts*/
static void tisr_entry(void *p)
{
	k_sem_give((struct k_sem *)p);
}

static void thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_give((struct k_sem *)p1);
}

static void tsema_thread_thread(struct k_sem *psem)
{
	/**TESTPOINT: thread-thread sync via sema*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      thread_entry, psem, NULL, NULL,
				      K_PRIO_PREEMPT(0),
				      K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	zassert_false(k_sem_take(psem, K_FOREVER), NULL);
	/*clean the spawn thread avoid side effect in next TC*/
	k_thread_abort(tid);
}

static void tsema_thread_isr(struct k_sem *psem)
{
	/**TESTPOINT: thread-isr sync via sema*/
	irq_offload(tisr_entry, psem);
	zassert_false(k_sem_take(psem, K_FOREVER), NULL);
}


/**
 * @brief Tests for the Semaphore kernel object
 * @defgroup kernel_semaphore_tests Semaphore
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Test synchronization of threads with semaphore
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 */
void test_sema_thread2thread(void)
{
	/**TESTPOINT: test k_sem_init sema*/
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);

	tsema_thread_thread(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_thread(&ksema);
}

/**
 * @brief Test synchronization between thread and irq
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 */
void test_sema_thread2isr(void)
{
	/**TESTPOINT: test k_sem_init sema*/
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	tsema_thread_isr(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_isr(&ksema);
}

/**
 * @brief Test k_sem_reset() API
 * @see k_sem_reset()
 */
void test_sema_reset(void)
{
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	k_sem_give(&sema);
	k_sem_reset(&sema);
	zassert_false(k_sem_count_get(&sema), NULL);
	/**TESTPOINT: sem take return -EBUSY*/
	zassert_equal(k_sem_take(&sema, K_NO_WAIT), -EBUSY, NULL);
	/**TESTPOINT: sem take return -EAGAIN*/
	zassert_equal(k_sem_take(&sema, TIMEOUT), -EAGAIN, NULL);
	k_sem_give(&sema);
	zassert_false(k_sem_take(&sema, K_FOREVER), NULL);
}

/**
 * @brief Test k_sem_count_get() API
 * @see k_sem_count_get()
 */
void test_sema_count_get(void)
{
	k_sem_init(&sema, SEM_INITIAL, SEM_LIMIT);
	/**TESTPOINT: sem count get upon init*/
	zassert_equal(k_sem_count_get(&sema), SEM_INITIAL, NULL);
	k_sem_give(&sema);
	/**TESTPOINT: sem count get after give*/
	zassert_equal(k_sem_count_get(&sema), SEM_INITIAL + 1, NULL);
	k_sem_take(&sema, K_FOREVER);
	/**TESTPOINT: sem count get after take*/
	for (int i = 0; i < SEM_LIMIT; i++) {
		zassert_equal(k_sem_count_get(&sema), SEM_INITIAL + i, NULL);
		k_sem_give(&sema);
	}
	/**TESTPOINT: sem give above limit*/
	k_sem_give(&sema);
	zassert_equal(k_sem_count_get(&sema), SEM_LIMIT, NULL);
}

/**
 * @}
 */

/*test case main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(), &ksema, &tdata, &sema, &tstack);

	ztest_test_suite(sema_api,
			 ztest_user_unit_test(test_sema_thread2thread),
			 ztest_unit_test(test_sema_thread2isr),
			 ztest_user_unit_test(test_sema_reset),
			 ztest_user_unit_test(test_sema_count_get));
	ztest_run_test_suite(sema_api);
}
