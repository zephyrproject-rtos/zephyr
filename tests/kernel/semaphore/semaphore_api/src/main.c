/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>

#include "../../common_files/include/sem_test.h"

/* Macro declarations */
#define SEM_MAX_VAL  (10U)
#define THREAD_TEST_PRIORITY 0

#define TOTAL_THREADS_WAITING (5)

/**
 * @brief Tests for the Semaphore kernel object
 * @defgroup kernel_semaphore_tests Semaphore
 * @ingroup all_tests
 * @{
 * @}
 */

/**
 * @brief Moudle test cases of kernel semaphore
 * @defgroup semaphore_module_tests Semaphore Module Tests
 * @ingroup kernel_semaphore_tests
 * @{
 * @}
 */

/******************************************************************************/
/* Kobject declaration */
K_SEM_DEFINE(simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(low_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(mid_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(high_prio_long_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(high_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(multiple_thread_sem, SEM_INIT_VAL, SEM_MAX_VAL);

K_THREAD_STACK_DEFINE(stack_1, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_2, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_3, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_4, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(multiple_stack, TOTAL_THREADS_WAITING, STACK_SIZE);

struct k_thread sem_tid_1, sem_tid_2, sem_tid_3, sem_tid_4;
struct k_thread multiple_tid[TOTAL_THREADS_WAITING];

struct k_sem sema;

/******************************************************************************/
/* Helper functions */

static void sem_give_task(void *p1, void *p2, void *p3)
{
	k_sem_give((struct k_sem *)p1);
}

static void sem_reset_take_task(void *p1, void *p2, void *p3)
{
	k_sem_reset((struct k_sem *)p1);
	zassert_false(k_sem_take((struct k_sem *)p1, K_FOREVER), NULL);
}

static void sem_take_timeout_forever_helper(void *p1, void *p2, void *p3)
{
	k_sleep(K_MSEC(100));
	k_sem_give(&simple_sem);
}

static void sem_take_multiple_low_prio_helper(void *p1, void *p2, void *p3)
{
	expect_k_sem_take_nomsg(&low_prio_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&low_prio_sem);
}

static void sem_take_multiple_mid_prio_helper(void *p1, void *p2, void *p3)
{
	expect_k_sem_take_nomsg(&mid_prio_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&mid_prio_sem);
}

static void sem_take_multiple_high_prio_helper(void *p1, void *p2, void *p3)
{

	expect_k_sem_take_nomsg(&high_prio_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&high_prio_sem);
}

static void sem_take_multiple_high_prio_long_helper(void *p1, void *p2, void *p3)
{
	expect_k_sem_take_nomsg(&high_prio_long_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&high_prio_long_sem);
}


static void sem_multiple_threads_wait_helper(void *p1, void *p2, void *p3)
{
	/* get blocked until the test thread gives the semaphore */
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	/* inform the test thread that this thread has got multiple_thread_sem*/
	k_sem_give(&simple_sem);
}

/**
 * @brief Test semaphore defined at compile time
 * @details
 * - Get the semaphore count.
 * - Verify the semaphore count equals to initialized value.
 * @ingroup semaphore_module_tests
 * @see k_sem_count_get()
 */
void test_k_sem_define(void)
{
	/* verify the semaphore count equals to initialized value */
	expect_k_sem_count_get(&simple_sem, SEM_INIT_VAL,
		     "semaphore initialized failed at compile time"
		     "- got %u, expected %u");
}

/**
 * @brief Test semaphore initialization at running time
 * @details
 * - Initialize a semaphore with valid count and max limit.
 * - Initialize a semaphore with invalid max limit.
 * - Initialize a semaphore with invalid count.
 * @ingroup semaphore_module_tests
 */
void test_k_sem_init(void)
{
	/* initialize a semaphore with valid count and max limit */
	expect_k_sem_init_nomsg(&sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	k_sem_reset(&sema);

	/* initialize a semaphore with invalid max limit */
	expect_k_sem_init_nomsg(&sema, SEM_INIT_VAL, 0, -EINVAL);

	/* initialize a semaphore with invalid count */
	expect_k_sem_init_nomsg(&sema, SEM_MAX_VAL + 1, SEM_MAX_VAL, -EINVAL);
}

/**
 * @brief Test k_sem_reset() API
 * @see k_sem_reset()
 */
void test_sem_reset(void)
{
	expect_k_sem_init_nomsg(&sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);
	expect_k_sem_count_get_nomsg(&sema, 0);

	k_sem_give(&sema);
	expect_k_sem_count_get_nomsg(&sema, 1);
	k_sem_reset(&sema);
	expect_k_sem_count_get_nomsg(&sema, 0);

	/**TESTPOINT: semaphore take return -EBUSY*/
	expect_k_sem_take_nomsg(&sema, K_NO_WAIT, -EBUSY);
	expect_k_sem_count_get_nomsg(&sema, 0);

	/**TESTPOINT: semaphore take return -EAGAIN*/
	expect_k_sem_take_nomsg(&sema, SEM_TIMEOUT, -EAGAIN);
	expect_k_sem_count_get_nomsg(&sema, 0);

	k_sem_give(&sema);
	expect_k_sem_count_get_nomsg(&sema, 1);

	expect_k_sem_take_nomsg(&sema, K_FOREVER, 0);
	expect_k_sem_count_get_nomsg(&sema, 0);
}

void test_sem_reset_waiting(void)
{
	int32_t ret_value;

	k_sem_reset(&simple_sem);

	/* create a new thread. It will reset the semaphore in 1ms
	 * then wait for us.
	 */
	k_tid_t tid = k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_reset_take_task, &simple_sem, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_MSEC(1));

	/* Take semaphore and wait for the abort. */
	ret_value = k_sem_take(&simple_sem, K_FOREVER);
	zassert_true(ret_value == -EAGAIN, "k_sem_take not aborted: %d",
			ret_value);

	/* ensure the semaphore is still functional after. */
	k_sem_give(&simple_sem);

	k_thread_join(tid, K_FOREVER);
}

/**
 * @brief Test k_sem_count_get() API
 * @see k_sem_count_get()
 */
void test_sem_count_get(void)
{
	expect_k_sem_init_nomsg(&sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	/**TESTPOINT: semaphore count get upon init*/
	expect_k_sem_count_get_nomsg(&sema, SEM_INIT_VAL);
	k_sem_give(&sema);
	/**TESTPOINT: sem count get after give*/
	expect_k_sem_count_get_nomsg(&sema, SEM_INIT_VAL + 1);
	expect_k_sem_take_nomsg(&sema, K_FOREVER, 0);
	/**TESTPOINT: sem count get after take*/
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		expect_k_sem_count_get_nomsg(&sema, SEM_INIT_VAL + i);
		k_sem_give(&sema);
	}
	/**TESTPOINT: semaphore give above limit*/
	k_sem_give(&sema);
	expect_k_sem_count_get_nomsg(&sema, SEM_MAX_VAL);
}

/**
 * @brief Test semaphore count when given by thread
 * @details
 * - Reset an initialized semaphore's count to zero
 * - Create a loop, in each loop, do follow steps
 * - Give the semaphore from a thread
 * - Get the semaphore's count
 * - Verify whether the semaphore's count as expected
 * @ingroup semaphore_module_tests
 * @see k_sem_give()
 */
void test_sem_give_from_thread(void)
{
	/*
	 * Signal the semaphore several times from a task.  After each signal,
	 * check the signal count.
	 */

	k_sem_reset(&simple_sem);
	expect_k_sem_count_get_nomsg(&simple_sem, 0);

	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);

		expect_k_sem_count_get_nomsg(&simple_sem, i + 1);
	}

}

/**
 * @brief Test if k_sem_take() decreases semaphore count
 * @see k_sem_take()
 */
void test_sem_take_no_wait(void)
{
	/*
	 * Test the semaphore without wait.  Check the signal count after each
	 * attempt (it should be decrementing by 1 each time).
	 */

	k_sem_reset(&simple_sem);
	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);
	}

	for (int i = 4; i >= 0; i--) {
		expect_k_sem_take_nomsg(&simple_sem, K_NO_WAIT, 0);
		expect_k_sem_count_get_nomsg(&simple_sem, i);
	}
}

/**
 * @brief Test k_sem_take() when there is no semaphore to take
 * @see k_sem_take()
 */
void test_sem_take_no_wait_fails(void)
{
	/*
	 * Test the semaphore without wait.  Check the signal count after each
	 * attempt (it should be always zero).
	 */

	k_sem_reset(&simple_sem);

	for (int i = 4; i >= 0; i--) {
		expect_k_sem_take_nomsg(&simple_sem, K_NO_WAIT, -EBUSY);
		expect_k_sem_count_get_nomsg(&simple_sem, 0);
	}

}

/**
 * @brief Test a semaphore take operation with an unavailable semaphore
 * @details
 * - Reset the semaphore's count to zero, let it unavailable.
 * - Take an unavailable semaphore and wait it until timeout.
 * @ingroup semaphore_module_tests
 * @see k_sem_take()
 */
void test_sem_take_timeout_fails(void)
{
	/*
	 * Test the semaphore with timeout without a k_sem_give.
	 */
	k_sem_reset(&simple_sem);
	expect_k_sem_count_get_nomsg(&simple_sem, 0);

	/* take an unavailable semaphore and wait it until timeout */
	for (int i = 4; i >= 0; i--) {
		expect_k_sem_take_nomsg(&simple_sem, SEM_TIMEOUT, -EAGAIN);
	}
}

/**
 * @brief Test the semaphore take operation with specified timeout
 * @details
 * - Create a new thread, it will give semaphore.
 * - Reset the semaphore's count to zero.
 * - Take semaphore and wait it given by other threads in specified timeout.
 * @ingroup semaphore_module_tests
 * @see k_sem_take()
 */
void test_sem_take_timeout(void)
{
	/*
	 * Signal the semaphore upon which the other thread is waiting.
	 * The thread (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking up this task.
	 */

	/* create a new thread, it will give semaphore */
	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_give_task, &simple_sem, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_FOREVER);

	k_sem_reset(&simple_sem);

	expect_k_sem_count_get_nomsg(&simple_sem, 0);

	k_thread_start(&sem_tid_1);
	/* Take semaphore and wait it given by other threads
	 * in specified timeout
	 */
	expect_k_sem_take_nomsg(&simple_sem, SEM_TIMEOUT, 0);
	k_thread_join(&sem_tid_1, K_FOREVER);

}

/**
 * @brief Test the semaphore take operation with forever wait
 * @details
 * - Create a new thread, it will give semaphore.
 * - Reset the semaphore's count to zero.
 * - Take semaphore, wait it given by other thread forever until it's available.
 * @ingroup semaphore_module_tests
 * @see k_sem_take()
 */
void test_sem_take_timeout_forever(void)
{
	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * thread (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */

	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_take_timeout_forever_helper, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_sem_reset(&simple_sem);

	expect_k_sem_count_get_nomsg(&simple_sem, 0);

	/* Take semaphore and wait it given by
	 * other threads forever until it's available
	 */
	expect_k_sem_take_nomsg(&simple_sem, K_FOREVER, 0);
	k_thread_join(&sem_tid_1, K_FOREVER);
}

/**
 * @brief Test semaphore take operation by multiple threads
 * @ingroup semaphore_module_tests
 * @see k_sem_take()
 */
void test_sem_take_multiple(void)
{
	k_sem_reset(&multiple_thread_sem);
	expect_k_sem_count_get_nomsg(&multiple_thread_sem, 0);

	/*
	 * Signal the semaphore upon which the another thread is waiting.
	 * The thread (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */

	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_take_multiple_low_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_create(&sem_tid_2, stack_2, STACK_SIZE,
			sem_take_multiple_mid_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(2), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_create(&sem_tid_3, stack_3, STACK_SIZE,
			sem_take_multiple_high_prio_long_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* Create another high priority thread, the same priority with sem_tid_3
	 * sem_tid_3 and sem_tid_4 are the same highest priority,
	 * but the waiting time of sem_tid_3 is longer than sem_tid_4.
	 * If some threads are the same priority, the sem given operation
	 * should be decided according to waiting time.
	 * That thread is necessary to test if a sem is available,
	 * it should be given to the highest priority and longest waiting thread
	 */
	k_thread_create(&sem_tid_4, stack_4, STACK_SIZE,
			sem_take_multiple_high_prio_helper, NULL, NULL,
			NULL, K_PRIO_PREEMPT(1), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	/* time for those 4 threads to complete */
	k_sleep(K_MSEC(20));

	/* Let these threads proceed to take the multiple_sem
	 * make thread 1 to 3 waiting on multiple_thread_sem
	 */
	k_sem_give(&high_prio_long_sem);
	k_sem_give(&mid_prio_sem);
	k_sem_give(&low_prio_sem);

	/* Delay 100ms to make sem_tid_4 waiting on multiple_thread_sem,
	 * then waiting time of sem_tid_4 is shorter than sem_tid_3
	 */
	k_sleep(K_MSEC(100));
	k_sem_give(&high_prio_sem);

	k_sleep(K_MSEC(20));

	/* enable the high prio and long waiting thread sem_tid_3 to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check which threads completed */
	expect_k_sem_count_get(&high_prio_long_sem, 1,
			"High priority and long waiting thread "
			"don't get the sem: %u != %u");

	expect_k_sem_count_get(&high_prio_sem, 0U,
			"High priority thread shouldn't get the sem: %u != %u");

	expect_k_sem_count_get(&mid_prio_sem, 0U,
		"Medium priority threads shouldn't have executed: %u != %u");

	expect_k_sem_count_get(&low_prio_sem, 0U,
		"Low priority threads shouldn't have executed: %u != %u");

	/* enable the high prio thread sem_tid_4 to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check which threads completed */
	expect_k_sem_count_get(&high_prio_long_sem, 1U,
		"High priority and long waiting thread executed again: %u != %u");

	expect_k_sem_count_get(&high_prio_sem, 1U,
		"Higher priority thread did not get the sem: %u != %u");

	expect_k_sem_count_get(&mid_prio_sem, 0U,
		"Medium priority thread shouldn't get the sem: %u != %u");

	expect_k_sem_count_get(&low_prio_sem, 0U,
		"Low priority thread shouldn't get the sem: %u != %u");

	/* enable the mid prio thread sem_tid_2 to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check which threads completed */
	expect_k_sem_count_get(&high_prio_long_sem, 1U,
		"High priority and long waiting thread executed again: %u != %u");

	expect_k_sem_count_get(&high_prio_sem, 1U,
		"High priority thread executed again: %u != %u");

	expect_k_sem_count_get(&mid_prio_sem, 1U,
		"Medium priority thread did not get the sem: %u != %u");

	expect_k_sem_count_get(&low_prio_sem, 0U,
		"Low priority thread did not get the sem: %u != %u");

	/* enable the low prio thread(thread_1) to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check the thread completed */
	expect_k_sem_count_get(&high_prio_long_sem, 1U,
		"High priority and long waiting thread executed again: %u != %u");

	expect_k_sem_count_get(&high_prio_sem, 1U,
		"High priority thread executed again: %u != %u");

	expect_k_sem_count_get(&mid_prio_sem, 1U,
		"Mid priority thread executed again: %u != %u");

	expect_k_sem_count_get(&low_prio_sem, 1U,
		"Low priority thread did not get the sem: %u != %u");
}

/**
 * @brief Test the max value a semaphore can be given and taken
 * @details
 * - Reset an initialized semaphore's count to zero.
 * - Give the semaphore by a thread and verify the semaphore's count is
 *   as expected.
 * - Verify the max count a semaphore can reach.
 * - Take the semaphore by a thread and verify the semaphore's count is
 *   as expected.
 * - Verify the max times a semaphore can be taken.
 * @ingroup semaphore_module_tests
 * @see k_sem_count_get(), k_sem_give()
 */
void test_k_sem_correct_count_limit(void)
{

	/* reset an initialized semaphore's count to zero */
	k_sem_reset(&simple_sem);
	expect_k_sem_count_get(&simple_sem, 0U, "k_sem_reset failed: %u != %u");

	/* Give the semaphore by a thread and verify the semaphore's
	 * count is as expected
	 */
	for (int i = 1; i <= SEM_MAX_VAL; i++) {
		k_sem_give(&simple_sem);
		expect_k_sem_count_get_nomsg(&simple_sem, i);
	}

	/* Verify the max count a semaphore can reach
	 * continue to run k_sem_give,
	 * the count of simple_sem will not increase anymore
	 */
	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);
		expect_k_sem_count_get_nomsg(&simple_sem, SEM_MAX_VAL);
	}

	/* Take the semaphore by a thread and verify the semaphore's
	 * count is as expected
	 */
	for (int i = SEM_MAX_VAL - 1; i >= 0; i--) {
		expect_k_sem_take_nomsg(&simple_sem, K_NO_WAIT, 0);
		expect_k_sem_count_get_nomsg(&simple_sem, i);
	}

	/* Verify the max times a semaphore can be taken
	 * continue to run k_sem_take, simple_sem can not be taken and
	 * it's count will be zero
	 */
	for (int i = 0; i < 5; i++) {
		expect_k_sem_take_nomsg(&simple_sem, K_NO_WAIT, -EBUSY);

		expect_k_sem_count_get_nomsg(&simple_sem, 0U);
	}
}

/**
 * @brief Test multiple semaphore take and give with wait
 * @ingroup semaphore_module_tests
 * @see k_sem_take(), k_sem_give()
 */
void test_sem_multiple_threads_wait(void)
{
	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);

	/* Verify a wait q that has been emptied / reset
	 * correctly by running twice.
	 */
	for (int repeat_count = 0; repeat_count < 2; repeat_count++) {
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_thread_create(&multiple_tid[i],
					multiple_stack[i], STACK_SIZE,
					sem_multiple_threads_wait_helper,
					NULL, NULL, NULL,
					K_PRIO_PREEMPT(1),
					K_USER | K_INHERIT_PERMS, K_NO_WAIT);
		}

		/* giving time for the other threads to execute  */
		k_sleep(K_MSEC(500));

		/* give the semaphores */
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_sem_give(&multiple_thread_sem);
		}

		/* giving time for the other threads to execute  */
		k_sleep(K_MSEC(500));

		/* check if all the threads are done. */
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			expect_k_sem_take(&simple_sem, K_FOREVER, 0,
				"Some of the threads did not get multiple_thread_sem: %d != %d");
		}

		expect_k_sem_count_get_nomsg(&simple_sem, 0U);
		expect_k_sem_count_get_nomsg(&multiple_thread_sem, 0U);

	}
}

/* ztest main entry */
void test_main(void)
{
	k_thread_access_grant(k_current_get(),
			      &simple_sem, &multiple_thread_sem, &low_prio_sem,
				  &mid_prio_sem, &high_prio_sem, &sema,
				  &high_prio_long_sem, &stack_1, &stack_2,
				  &stack_3, &stack_4, &sem_tid_1, &sem_tid_2,
				  &sem_tid_3, &sem_tid_4);

	ztest_test_suite(test_semaphore_api,
			 ztest_user_unit_test(test_k_sem_define),
			 ztest_user_unit_test(test_k_sem_init),
			 ztest_user_unit_test(test_sem_reset),
			 ztest_user_unit_test(test_sem_reset_waiting),
			 ztest_user_unit_test(test_sem_count_get),
			 ztest_user_unit_test(test_sem_give_from_thread),
			 ztest_user_unit_test(test_sem_take_no_wait),
			 ztest_user_unit_test(test_sem_take_no_wait_fails),
			 ztest_user_unit_test(test_sem_take_timeout_fails),
			 ztest_user_unit_test(test_sem_take_timeout),
			 ztest_user_unit_test(test_sem_take_timeout_forever),
			 ztest_user_unit_test(test_sem_take_multiple),
			 ztest_user_unit_test(test_k_sem_correct_count_limit),
			 ztest_unit_test(test_sem_multiple_threads_wait));
	ztest_run_test_suite(test_semaphore_api);
}
