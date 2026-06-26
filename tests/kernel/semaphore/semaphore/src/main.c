/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/ztest_error_hook.h>


/**
 * @defgroup kernel_semaphore_tests Semaphores
 * @ingroup all_tests
 * @{
 * @}
 */

/* Macro declarations */
#define SEM_INIT_VAL (0U)
#define SEM_MAX_VAL  (10U)
#define THREAD_TEST_PRIORITY 0

#define sem_give_from_isr(sema) irq_offload(isr_sem_give, (const void *)sema)
#define sem_take_from_isr(sema) irq_offload(isr_sem_take, (const void *)sema)

#define SEM_TIMEOUT (K_MSEC(100))
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TOTAL_THREADS_WAITING (5)

#define SEC2MS(s) ((s) * 1000)
#define QSEC2MS(s) ((s) * 250)

#define expect_k_sem_take(sem, timeout, exp, str) do { \
	int _act = k_sem_take((sem), (timeout)); \
	int _exp = (exp); \
	zassert_equal(_act, _exp, (str), _act, _exp); \
} while (0)

#define expect_k_sem_init(sem, init, max, exp, str) do { \
	int _act = k_sem_init((sem), (init), (max)); \
	int _exp = (exp); \
	zassert_equal(_act, _exp, (str), _act, _exp); \
} while (0)

#define expect_k_sem_count_get(sem, exp, str) do { \
	unsigned int _act = k_sem_count_get(sem); \
	unsigned int _exp = (exp); \
	zassert_equal(_act, _exp, (str), _act, _exp); \
} while (0)

#define expect_k_sem_take_nomsg(sem, timeout, exp) \
	expect_k_sem_take((sem), (timeout), (exp), "k_sem_take incorrect return value: %d != %d")
#define expect_k_sem_init_nomsg(sem, init, max, exp) \
	expect_k_sem_init((sem), (init), (max), (exp), \
					  "k_sem_init incorrect return value: %d != %d")
#define expect_k_sem_count_get_nomsg(sem, exp) \
	expect_k_sem_count_get((sem), (exp), "k_sem_count_get incorrect return value: %u != %u")

/* global variable for mutual exclusion test */
uint32_t critical_var;

/** Per-thread timeout descriptor used by the multi-timeout test helpers. */
struct timeout_info {
	uint32_t timeout;    /**< Requested take timeout in milliseconds. */
	struct k_sem *sema;  /**< Semaphore the thread waits on. */
};

/******************************************************************************/
/* Kobject declaration */
K_SEM_DEFINE(statically_defined_sem, SEM_INIT_VAL, SEM_MAX_VAL);
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
K_PIPE_DEFINE(timeout_info_pipe,
	      sizeof(struct timeout_info) * TOTAL_THREADS_WAITING, 4);

struct k_thread sem_tid_1, sem_tid_2, sem_tid_3, sem_tid_4;
struct k_thread multiple_tid[TOTAL_THREADS_WAITING];

K_SEM_DEFINE(ksema, SEM_INIT_VAL, SEM_MAX_VAL);
struct k_sem msg_sema, mut_sem;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
struct k_thread tdata;

/******************************************************************************/
/* Helper functions */

void sem_give_task(void *p1, void *p2, void *p3)
{
	k_sem_give((struct k_sem *)p1);
}

void sem_reset_take_task(void *p1, void *p2, void *p3)
{
	k_sem_reset((struct k_sem *)p1);
	zassert_false(k_sem_take((struct k_sem *)p1, K_FOREVER));
}

void isr_sem_give(const void *semaphore)
{
	k_sem_give((struct k_sem *)semaphore);
}

static void tsema_thread_thread(struct k_sem *psem)
{
	/**TESTPOINT: thread-thread sync via sema*/
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      sem_give_task, psem, NULL, NULL,
				      K_PRIO_PREEMPT(0),
				      K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	expect_k_sem_take_nomsg(psem, K_FOREVER, 0);

	/*clean the spawn thread avoid side effect in next TC*/
	k_thread_join(tid, K_FOREVER);
}

static void tsema_thread_isr(struct k_sem *psem)
{
	/**TESTPOINT: thread-isr sync via sema*/
	irq_offload(isr_sem_give, (const void *)psem);

	expect_k_sem_take_nomsg(psem, K_FOREVER, 0);
}


void isr_sem_take(const void *semaphore)
{
	int ret = k_sem_take((struct k_sem *)semaphore, K_NO_WAIT);

	if (ret != 0 && ret != -EBUSY) {
		zassert_true(false, "incorrect k_sem_take return: %d", ret);
	}
}



void sem_take_timeout_forever_helper(void *p1, void *p2, void *p3)
{
	k_sleep(K_MSEC(100));
	k_sem_give(&simple_sem);
}

void sem_take_timeout_isr_helper(void *p1, void *p2, void *p3)
{
	sem_give_from_isr(&simple_sem);
}

void sem_take_multiple_low_prio_helper(void *p1, void *p2, void *p3)
{
	expect_k_sem_take_nomsg(&low_prio_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&low_prio_sem);
}

void sem_take_multiple_mid_prio_helper(void *p1, void *p2, void *p3)
{
	expect_k_sem_take_nomsg(&mid_prio_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&mid_prio_sem);
}

void sem_take_multiple_high_prio_helper(void *p1, void *p2, void *p3)
{

	expect_k_sem_take_nomsg(&high_prio_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&high_prio_sem);
}

/* First function for mutual exclusion test */
void sem_queue_mutual_exclusion1(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < 1000; i++) {
		expect_k_sem_take_nomsg(&mut_sem, K_FOREVER, 0);

		/* in that function critical section makes critical var +1 */
		uint32_t tmp = critical_var;
		critical_var += 1;

		/* Check that common value was not changed by another thread,
		 * when semaphore is taken by current thread, and no other
		 * thread can enter the critical section
		 */
		zassert_true(critical_var == tmp + 1);
		k_sem_give(&mut_sem);
	}
}

/* Second function for mutual exclusion test */
void sem_queue_mutual_exclusion2(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < 1000; i++) {
		expect_k_sem_take_nomsg(&mut_sem, K_FOREVER, 0);

		/* in that function critical section makes critical var 0 */
		uint32_t tmp = critical_var;
		critical_var -= 1;

		/* Check that common value was not changed by another thread,
		 * when semaphore is taken by current thread, and no other
		 * thread can enter the critical section
		 */
		zassert_true(critical_var == tmp - 1);
		k_sem_give(&mut_sem);
	}
}

void sem_take_multiple_high_prio_long_helper(void *p1, void *p2, void *p3)
{
	expect_k_sem_take_nomsg(&high_prio_long_sem, K_FOREVER, 0);
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	k_sem_give(&high_prio_long_sem);
}

/**
 * @ingroup kernel_semaphore_tests
 * @{
 */


/**
 * @brief Test semaphore defined at compile time
 * @details
 * - Get the semaphore count.
 * - Verify the semaphore count equals to initialized value.
 * @ingroup kernel_semaphore_tests
 * @see k_sem_count_get()
 * @verifies ZEP-SRS-5-1
 * @verifies ZEP-SRS-5-5
 */
ZTEST_USER(semaphore, test_k_sem_define)
{
	/* verify the semaphore count equals to initialized value */
	expect_k_sem_count_get(&statically_defined_sem, SEM_INIT_VAL,
		     "semaphore initialized failed at compile time"
		     "- got %u, expected %u");
}

/**
 * @brief Verify a semaphore synchronizes two threads.
 *
 * @details
 * A semaphore is used to hand off execution between two threads, exercising both
 * a run-time initialized semaphore (k_sem_init()) and a compile-time defined one
 * (K_SEM_DEFINE()). One thread gives the semaphore while the other takes it,
 * confirming that a run-time defined semaphore is usable for synchronization.
 *
 * Test steps:
 * - Initialize a semaphore at run time with k_sem_init().
 * - Hand off between two threads via k_sem_give()/k_sem_take().
 * - Repeat the hand off using a compile-time K_SEM_DEFINE() semaphore.
 *
 * Expected result:
 * - Both threads synchronize correctly using either semaphore.
 *
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 * @verifies ZEP-SRS-5-2
 */
ZTEST_USER(semaphore, test_sem_thread2thread)
{
	/**TESTPOINT: test k_sem_init sema*/
	expect_k_sem_init_nomsg(&msg_sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	tsema_thread_thread(&msg_sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_thread(&ksema);
}

/**
 * @brief Verify a semaphore synchronizes a thread with an ISR.
 *
 * @details
 * A semaphore is given from interrupt context and taken from thread context,
 * exercising both a run-time initialized semaphore and a compile-time defined
 * one, to confirm a semaphore can be used to synchronize across the thread/ISR
 * boundary.
 *
 * Test steps:
 * - Initialize a semaphore at run time and give it from an ISR.
 * - Take it from the thread and verify synchronization.
 * - Repeat with a compile-time K_SEM_DEFINE() semaphore.
 *
 * Expected result:
 * - The thread and ISR synchronize correctly using either semaphore.
 *
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 * @verifies ZEP-SRS-5-20
 */
ZTEST(semaphore, test_sem_thread2isr)
{
	/**TESTPOINT: test k_sem_init sema*/
	expect_k_sem_init_nomsg(&msg_sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	tsema_thread_isr(&msg_sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_isr(&ksema);
}

/** Parameter set for the parameterized test_sem_init_validity test. */
struct sem_init_case {
	uint32_t init_val;     /**< Initial semaphore count passed to k_sem_init(). */
	uint32_t max_val;      /**< Maximum semaphore count passed to k_sem_init(). */
	int      expected_ret; /**< Expected k_sem_init() return value. */
};

/**
 * @brief Parameterized k_sem_init() validity test.
 *
 * @details
 * The existing test_k_sem_init() checks 3 hardcoded (init, max) combinations
 * in a single test body.  Here each combination is a separate named invocation
 * so that Twister output immediately identifies which specific combination
 * caused a failure (e.g. "test_sem_init_validity[cases/3]").
 *
 * Valid combinations must return 0; invalid ones must return -EINVAL:
 *  - max == 0                          -> -EINVAL
 *  - init > max                        -> -EINVAL
 *  - init == max (boundary)            -> 0
 *  - init < max                        -> 0
 *  - large but valid max               -> 0
 *
 * @ingroup kernel_semaphore_tests
 * @see k_sem_init()
 * @verifies ZEP-SRS-5-2
 * @verifies ZEP-SRS-5-4
 * @verifies ZEP-SRS-5-18
 */
ZTEST_USER_P(semaphore, test_sem_init_validity)
{
	const struct sem_init_case *tc =
		ZTEST_GET_PARAM_PTR(struct sem_init_case);

	expect_k_sem_init_nomsg(&msg_sema, tc->init_val, tc->max_val,
				tc->expected_ret);

	if (tc->expected_ret == 0) {
		/* Verify the count was set as requested */
		expect_k_sem_count_get_nomsg(&msg_sema, tc->init_val);
		k_sem_reset(&msg_sema);
	}
}

static const struct sem_init_case sem_init_cases[] = {
	/* valid: init = 0, various max values */
	{0U,              1U,          0},
	{0U,              SEM_MAX_VAL, 0},
	{0U,              UINT_MAX,    0},
	/* valid: init == max (boundary) */
	{1U,              1U,          0},
	{SEM_MAX_VAL,     SEM_MAX_VAL, 0},
	/* valid: 0 < init < max */
	{SEM_MAX_VAL / 2, SEM_MAX_VAL, 0},
	/* invalid: max == 0 */
	{0U,              0U,          -EINVAL},
	/* invalid: init > max */
	{SEM_MAX_VAL + 1, SEM_MAX_VAL, -EINVAL},
	{2U,              1U,          -EINVAL},
};

ZTEST_DEFINE_PARAM_VALUES_ARRAY(sem_init_cases_vals, sem_init_cases);
ZTEST_INSTANTIATE_TEST_SUITE_P(cases, semaphore,
			       test_sem_init_validity, sem_init_cases_vals);

/**
 * @brief Verify k_sem_reset() sets the semaphore count back to zero.
 *
 * @details
 * After giving a semaphore so its count is non-zero, k_sem_reset() must return
 * the count to zero. A subsequent non-blocking take then fails with -EBUSY and a
 * timed take fails with -EAGAIN, and the semaphore remains usable afterwards.
 *
 * Test steps:
 * - Initialize a semaphore and give it once (count becomes 1).
 * - Call k_sem_reset() and read the count.
 * - Attempt k_sem_take() with K_NO_WAIT and with a finite timeout.
 *
 * Expected result:
 * - Count is zero after reset; take returns -EBUSY (no wait) and -EAGAIN (timeout).
 *
 * @see k_sem_reset()
 * @verifies ZEP-SRS-5-16
 */
ZTEST_USER(semaphore, test_sem_reset)
{
	expect_k_sem_init_nomsg(&msg_sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);
	expect_k_sem_count_get_nomsg(&msg_sema, 0);

	k_sem_give(&msg_sema);
	expect_k_sem_count_get_nomsg(&msg_sema, 1);
	k_sem_reset(&msg_sema);
	expect_k_sem_count_get_nomsg(&msg_sema, 0);

	/**TESTPOINT: semaphore take return -EBUSY*/
	expect_k_sem_take_nomsg(&msg_sema, K_NO_WAIT, -EBUSY);
	expect_k_sem_count_get_nomsg(&msg_sema, 0);

	/**TESTPOINT: semaphore take return -EAGAIN*/
	expect_k_sem_take_nomsg(&msg_sema, SEM_TIMEOUT, -EAGAIN);
	expect_k_sem_count_get_nomsg(&msg_sema, 0);

	k_sem_give(&msg_sema);
	expect_k_sem_count_get_nomsg(&msg_sema, 1);

	expect_k_sem_take_nomsg(&msg_sema, K_FOREVER, 0);
	expect_k_sem_count_get_nomsg(&msg_sema, 0);
}

/**
 * @brief Verify that resetting a semaphore aborts pending acquisitions.
 *
 * @details
 * A waiter blocks on k_sem_take() with K_FOREVER while another thread resets
 * the semaphore. The pending acquisition must be aborted and return -EAGAIN,
 * and the semaphore must remain functional afterwards.
 *
 * @ingroup kernel_semaphore_tests
 * @see k_sem_reset(), k_sem_take()
 * @verifies ZEP-SRS-5-17
 */
ZTEST_USER(semaphore, test_sem_reset_waiting)
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
 * @brief Verify k_sem_count_get() reports the count without acquiring.
 *
 * @details
 * k_sem_count_get() must return the current semaphore count without consuming
 * it, and the reported count must track give/take operations: it increments on
 * each give (saturating at the maximum) and decrements on each take.
 *
 * Test steps:
 * - Read the count right after initialization.
 * - Give the semaphore and read the count again.
 * - Repeatedly give up to (and beyond) the maximum and read the count.
 *
 * Expected result:
 * - The returned count matches the expected value at each step and never exceeds
 *   the configured maximum.
 *
 * @see k_sem_count_get()
 * @verifies ZEP-SRS-5-13
 * @verifies ZEP-SRS-5-15
 */
ZTEST_USER(semaphore, test_sem_count_get)
{
	expect_k_sem_init_nomsg(&msg_sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	/**TESTPOINT: semaphore count get upon init*/
	expect_k_sem_count_get_nomsg(&msg_sema, SEM_INIT_VAL);
	k_sem_give(&msg_sema);
	/**TESTPOINT: sem count get after give*/
	expect_k_sem_count_get_nomsg(&msg_sema, SEM_INIT_VAL + 1);
	expect_k_sem_take_nomsg(&msg_sema, K_FOREVER, 0);
	/**TESTPOINT: sem count get after take*/
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		expect_k_sem_count_get_nomsg(&msg_sema, SEM_INIT_VAL + i);
		k_sem_give(&msg_sema);
	}
	/**TESTPOINT: semaphore give above limit*/
	k_sem_give(&msg_sema);
	expect_k_sem_count_get_nomsg(&msg_sema, SEM_MAX_VAL);
}


/**
 * @brief Test whether a semaphore can be given by an ISR
 * @details
 * - Reset an initialized semaphore's count to zero
 * - Create a loop, in each loop, do follow steps
 * - Give the semaphore from an ISR
 * - Get the semaphore's count
 * - Verify whether the semaphore's count as expected
 * @ingroup kernel_semaphore_tests
 * @see k_sem_give()
 * @verifies ZEP-SRS-5-12
 * @verifies ZEP-SRS-5-20
 */
ZTEST(semaphore, test_sem_give_from_isr)
{
	/*
	 * Signal the semaphore several times from an ISR.  After each signal,
	 * check the signal count.
	 */

	k_sem_reset(&simple_sem);
	expect_k_sem_count_get_nomsg(&simple_sem, 0);

	for (int i = 0; i < 5; i++) {
		sem_give_from_isr(&simple_sem);

		expect_k_sem_count_get_nomsg(&simple_sem, i + 1);
	}
}

/**
 * @brief Test semaphore count when given by thread
 * @details
 * - Reset an initialized semaphore's count to zero
 * - Create a loop, in each loop, do follow steps
 * - Give the semaphore from a thread
 * - Get the semaphore's count
 * - Verify whether the semaphore's count as expected
 * @ingroup kernel_semaphore_tests
 * @see k_sem_give()
 * @verifies ZEP-SRS-5-12
 * @verifies ZEP-SRS-5-13
 */
ZTEST_USER(semaphore, test_sem_give_from_thread)
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
 * @brief Verify k_sem_take() succeeds and decrements when the count is positive.
 *
 * @details
 * While the count is greater than zero, a non-blocking k_sem_take() must acquire
 * the semaphore immediately and decrement the count by one on each call.
 *
 * Test steps:
 * - Give the semaphore several times to raise its count.
 * - Repeatedly call k_sem_take() with K_NO_WAIT.
 * - Read the count after each take.
 *
 * Expected result:
 * - Each take returns 0 and the count decreases by one each time.
 *
 * @see k_sem_take()
 * @verifies ZEP-SRS-5-6
 * @verifies ZEP-SRS-5-7
 */
ZTEST_USER(semaphore, test_sem_take_no_wait)
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
 * @brief Verify k_sem_take() with no wait fails when the count is zero.
 *
 * @details
 * When the count is zero and no timeout is provided (K_NO_WAIT), k_sem_take()
 * must not block and must return -EBUSY, leaving the count unchanged at zero.
 *
 * Test steps:
 * - Reset the semaphore so its count is zero.
 * - Call k_sem_take() with K_NO_WAIT repeatedly.
 * - Read the count after each attempt.
 *
 * Expected result:
 * - Each take returns -EBUSY and the count remains zero.
 *
 * @see k_sem_take()
 * @verifies ZEP-SRS-5-11
 */
ZTEST_USER(semaphore, test_sem_take_no_wait_fails)
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
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 * @verifies ZEP-SRS-5-10
 */
ZTEST_USER(semaphore, test_sem_take_timeout_fails)
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
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 * @verifies ZEP-SRS-5-8
 * @verifies ZEP-SRS-5-9
 */
ZTEST_USER(semaphore, test_sem_take_timeout)
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
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 * @verifies ZEP-SRS-5-8
 */
ZTEST_USER(semaphore, test_sem_take_timeout_forever)
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
 * @brief Verify k_sem_take() with a timeout is satisfied by an ISR give.
 *
 * @details
 * A thread blocks on k_sem_take() with a finite timeout while a helper gives the
 * semaphore from interrupt context before the timeout elapses, so the take must
 * succeed.
 *
 * Test steps:
 * - Reset the semaphore so its count is zero.
 * - Start a helper that gives the semaphore from an ISR.
 * - Call k_sem_take() with a finite timeout.
 *
 * Expected result:
 * - k_sem_take() returns 0 (acquired before the timeout expires).
 *
 * @see k_sem_take()
 */
ZTEST(semaphore_1cpu, test_sem_take_timeout_isr)
{
	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * thread (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */

	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_take_timeout_isr_helper, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_sem_reset(&simple_sem);

	expect_k_sem_take_nomsg(&simple_sem, SEM_TIMEOUT, 0);

	k_thread_join(&sem_tid_1, K_FOREVER);
}

/**
 * @brief Test semaphore take operation by multiple threads
 * @details
 * Several threads of differing priorities wait on the same semaphore. When the
 * semaphore is given, the highest-priority (and, among equal priorities, the
 * longest-waiting) thread must be the one that acquires it.
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 * @verifies ZEP-SRS-5-14
 */
ZTEST_USER(semaphore, test_sem_take_multiple)
{
	k_sem_reset(&multiple_thread_sem);
	k_sem_reset(&high_prio_long_sem);
	k_sem_reset(&mid_prio_sem);
	k_sem_reset(&low_prio_sem);
	k_sem_reset(&high_prio_sem);
	expect_k_sem_count_get_nomsg(&multiple_thread_sem, 0);
	expect_k_sem_count_get_nomsg(&high_prio_long_sem, 0);
	expect_k_sem_count_get_nomsg(&mid_prio_sem, 0);
	expect_k_sem_count_get_nomsg(&low_prio_sem, 0);
	expect_k_sem_count_get_nomsg(&high_prio_sem, 0);
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

	k_thread_join(&sem_tid_1, K_FOREVER);
	k_thread_join(&sem_tid_2, K_FOREVER);
	k_thread_join(&sem_tid_3, K_FOREVER);
	k_thread_join(&sem_tid_4, K_FOREVER);
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
 * @ingroup kernel_semaphore_tests
 * @see k_sem_count_get(), k_sem_give()
 * @verifies ZEP-SRS-5-3
 * @verifies ZEP-SRS-5-4
 * @verifies ZEP-SRS-5-19
 */
ZTEST_USER(semaphore, test_k_sem_correct_count_limit)
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
 * @brief Verify a semaphore can be given and taken from ISR context.
 *
 * @details
 * Give the semaphore from an ISR up to its maximum, then take it from an ISR back
 * down to zero, checking the count after every operation to confirm give/take
 * work correctly from interrupt context.
 *
 * Test steps:
 * - Reset the semaphore so its count is zero.
 * - Give from an ISR until the maximum count, checking the count each time.
 * - Take from an ISR until zero, checking the count each time.
 *
 * Expected result:
 * - The count tracks every give/take and ends at zero.
 *
 * @see k_sem_give()
 * @verifies ZEP-SRS-5-20
 */
ZTEST(semaphore, test_sem_give_take_from_isr)
{
	k_sem_reset(&simple_sem);
	expect_k_sem_count_get_nomsg(&simple_sem, 0U);

	/* give semaphore from an isr and do a check for the count */
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		sem_give_from_isr(&simple_sem);
		expect_k_sem_count_get_nomsg(&simple_sem, i + 1);
	}

	/* take semaphore from an isr and do a check for the count */
	for (int i = SEM_MAX_VAL; i > 0; i--) {
		sem_take_from_isr(&simple_sem);

		expect_k_sem_count_get_nomsg(&simple_sem, i - 1);
	}
}

/**
 * @}
 */

void sem_multiple_threads_wait_helper(void *p1, void *p2, void *p3)
{
	/* get blocked until the test thread gives the semaphore */
	expect_k_sem_take_nomsg(&multiple_thread_sem, K_FOREVER, 0);

	/* inform the test thread that this thread has got multiple_thread_sem*/
	k_sem_give(&simple_sem);
}


/**
 * @brief Verify all waiters are released as the semaphore is given repeatedly.
 *
 * @details
 * Several threads block on the same semaphore. Giving the semaphore once per
 * waiter must release each of them exactly once. The scenario is run twice to
 * confirm the wait queue is left empty and reusable after the first round.
 *
 * Test steps:
 * - Start TOTAL_THREADS_WAITING threads, each blocking on the semaphore.
 * - Give the semaphore once per waiting thread.
 * - Confirm every thread acquired it and the counts return to zero.
 * - Repeat the whole sequence a second time.
 *
 * Expected result:
 * - All waiters are released each round and the wait queue ends empty.
 *
 * @see k_sem_take(), k_sem_give()
 */
ZTEST(semaphore, test_sem_multiple_threads_wait)
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

		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_thread_join(&multiple_tid[i], K_FOREVER);
		}
	}
}

/**
 * @brief Verify k_sem_take() honors the requested timeout duration.
 *
 * @details
 * On an unavailable semaphore, k_sem_take() with a one-second timeout must block
 * for at least that long before returning -EAGAIN, confirming the timeout period
 * is actually observed (and a K_NO_WAIT take returns promptly).
 *
 * Test steps:
 * - Reset the semaphore so its count is zero.
 * - Record uptime, call k_sem_take() with a 1-second timeout, record uptime.
 * - Verify the elapsed time is at least one second and the return is -EAGAIN.
 *
 * Expected result:
 * - The timed take blocks for >= 1 second and returns -EAGAIN.
 *
 * @see k_sem_take(), k_sem_give(), k_sem_reset()
 */
ZTEST(semaphore, test_sem_measure_timeouts)
{
	int64_t start_ticks, end_ticks, diff_ticks;

	k_sem_reset(&simple_sem);

	/* with timeout of 1 sec */
	start_ticks = k_uptime_get();

	expect_k_sem_take_nomsg(&simple_sem, K_SECONDS(1), -EAGAIN);

	end_ticks = k_uptime_get();

	diff_ticks = end_ticks - start_ticks;

	zassert_true((diff_ticks >= SEC2MS(1)),
		     "k_sem_take returned too early: %lld < %d",
		     diff_ticks, SEC2MS(1));

	/* This subtest could fail spuriously if we happened to run the below right as
	 * a tick occurred. Unfortunately, we cannot actually fix this, because on some
	 * emulated platforms time does not advance while running the cpu, so
	 * if we spin and wait for a tick boundary we'll spin forever.
	 * The best we can do is hope that k_busy_wait finishes just after a tick boundary.
	 */
	k_busy_wait(1);
	start_ticks = k_uptime_get();

	expect_k_sem_take_nomsg(&simple_sem, K_NO_WAIT, -EBUSY);

	end_ticks = k_uptime_get();

	diff_ticks = end_ticks - start_ticks;

	zassert_true(end_ticks >= start_ticks,
		     "time went backwards: %lld -> %lld",
		     start_ticks, end_ticks);

	zassert_true(diff_ticks >= 0,
		     "k_sem_take took too long: %lld != 0",
		     diff_ticks);
}

void sem_measure_timeout_from_thread_helper(void *p1, void *p2, void *p3)
{
	/* first sync the 2 threads */
	k_sem_give(&simple_sem);

	/* give the semaphore */
	k_sem_give(&multiple_thread_sem);
}


/**
 * @brief Verify a timed k_sem_take() returns early when given by a thread.
 *
 * @details
 * A thread blocks on k_sem_take() with a one-second timeout while a lower
 * priority thread gives the semaphore well before the timeout. The take must
 * therefore return success in less than the full timeout period.
 *
 * Test steps:
 * - Reset the semaphores and start a helper thread that gives the semaphore.
 * - Synchronize, then call k_sem_take() with a 1-second timeout.
 * - Measure the elapsed time.
 *
 * Expected result:
 * - k_sem_take() returns 0 and the elapsed time is well under one second.
 *
 * @see k_sem_give(), k_sem_reset(), k_sem_take()
 */
ZTEST(semaphore, test_sem_measure_timeout_from_thread)
{
	int64_t start_ticks, end_ticks, diff_ticks;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);

	/* give a semaphore from a thread and calculate the time taken */
	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_measure_timeout_from_thread_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), 0, K_NO_WAIT);


	/* first sync the 2 threads */
	expect_k_sem_take_nomsg(&simple_sem, K_FOREVER, 0);

	/* with timeout of 1 sec */
	start_ticks = k_uptime_get();

	expect_k_sem_take_nomsg(&multiple_thread_sem, K_SECONDS(1), 0);

	end_ticks = k_uptime_get();

	diff_ticks = end_ticks - start_ticks;

	zassert_true((diff_ticks < SEC2MS(1)),
		     "k_sem_take took too long: %d >= %d",
		     (int)diff_ticks, SEC2MS(1));
	k_thread_join(&sem_tid_1, K_FOREVER);
}

void sem_multiple_take_and_timeouts_helper(void *p1, void *p2, void *p3)
{
	int timeout = POINTER_TO_INT(p1);
	int64_t start_ticks, end_ticks, diff_ticks;

	start_ticks = k_uptime_get();

	expect_k_sem_take_nomsg(&simple_sem, K_MSEC(timeout), -EAGAIN);

	end_ticks = k_uptime_get();

	diff_ticks = end_ticks - start_ticks;

	zassert_true(diff_ticks >= timeout,
		     "time mismatch - expected at least %d, got %lld",
		     timeout, diff_ticks);

	k_pipe_write(&timeout_info_pipe, (uint8_t *)&timeout, sizeof(int), K_FOREVER);

}

/**
 * @brief Verify threads waiting on one semaphore time out in timeout order.
 *
 * @details
 * Several threads block on the same semaphore with different timeouts. With none
 * of them ever given the semaphore, they must time out (-EAGAIN) in ascending
 * order of their timeout values; the observed order is recorded via a pipe and
 * checked.
 *
 * Test steps:
 * - Start threads that each take the semaphore with a distinct timeout.
 * - Record each thread's timeout value as it expires.
 * - Compare the expiry order against the expected ascending order.
 *
 * Expected result:
 * - Threads time out with -EAGAIN in increasing timeout order.
 *
 * @see k_sem_take(), k_sem_reset()
 */
ZTEST(semaphore_1cpu, test_sem_multiple_take_and_timeouts)
{
	if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		ztest_test_skip();
	}

	static uint32_t timeout;

	k_sem_reset(&simple_sem);
	k_pipe_reset(&timeout_info_pipe);

	/* Multiple threads timeout and the sequence in which it times out
	 * is pushed into a pipe and checked later on.
	 */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i],
				multiple_stack[i], STACK_SIZE,
				sem_multiple_take_and_timeouts_helper,
				INT_TO_POINTER(QSEC2MS(i + 1)), NULL, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_pipe_read(&timeout_info_pipe, (uint8_t *)&timeout, sizeof(int), K_FOREVER);
		zassert_equal(timeout, QSEC2MS(i + 1),
			     "timeout did not occur properly: %d != %d",
				 timeout, QSEC2MS(i + 1));
	}

	/* cleanup */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_join(&multiple_tid[i], K_FOREVER);
	}
}

void sem_multi_take_timeout_diff_sem_helper(void *p1, void *p2, void *p3)
{
	int rc;
	int timeout = POINTER_TO_INT(p1);
	struct k_sem *sema = p2;
	int64_t start_ticks, end_ticks, diff_ticks;
	struct timeout_info info = {
		.timeout = timeout,
		.sema = sema
	};

	start_ticks = k_uptime_get();

	expect_k_sem_take_nomsg(sema, K_MSEC(timeout), -EAGAIN);

	end_ticks = k_uptime_get();

	diff_ticks = end_ticks - start_ticks;

	zassert_true(diff_ticks >= timeout,
		     "time mismatch - expected at least %d, got %lld",
		     timeout, diff_ticks);

	rc = k_pipe_write(&timeout_info_pipe, (uint8_t *)&info, sizeof(struct timeout_info),
		   K_FOREVER);
	zassert_true(rc == sizeof(struct timeout_info),
		     "k_pipe_write failed: %d", rc);
}

/**
 * @brief Verify timeout ordering across waiters on different semaphores.
 *
 * @details
 * Threads block with different timeouts on two different semaphores. None are
 * given, so every thread must time out (-EAGAIN), and the global expiry order
 * must follow the ascending timeout values regardless of which semaphore each
 * thread waited on.
 *
 * Test steps:
 * - Start threads waiting on two semaphores, each with a distinct timeout.
 * - Record each expiry as it happens.
 * - Verify the expiries occur in ascending timeout order.
 *
 * Expected result:
 * - All takes return -EAGAIN and expire in ascending timeout order.
 *
 * @see k_sem_take(), k_sem_reset()
 */
ZTEST(semaphore, test_sem_multi_take_timeout_diff_sem)
{
	int rc;
	if (IS_ENABLED(CONFIG_KERNEL_COHERENCE)) {
		ztest_test_skip();
	}

	struct timeout_info seq_info[] = {
		{ SEC2MS(2), &simple_sem },
		{ SEC2MS(1), &multiple_thread_sem },
		{ SEC2MS(3), &simple_sem },
		{ SEC2MS(5), &multiple_thread_sem },
		{ SEC2MS(4), &simple_sem },
	};

	static struct timeout_info retrieved_info;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);
	k_pipe_reset(&timeout_info_pipe);
	memset(&retrieved_info, 0, sizeof(struct timeout_info));

	/* Multiple threads timeout on different semaphores and the sequence
	 * in which it times out is pushed into a pipe and checked later on.
	 */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i],
				multiple_stack[i], STACK_SIZE,
				sem_multi_take_timeout_diff_sem_helper,
				INT_TO_POINTER(seq_info[i].timeout),
				seq_info[i].sema, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		rc = k_pipe_read(&timeout_info_pipe, (uint8_t *)&retrieved_info,
			sizeof(struct timeout_info), K_FOREVER);
		zassert_true(rc == sizeof(struct timeout_info),
			     "k_pipe_read failed: %d", rc);

		zassert_true(retrieved_info.timeout == SEC2MS(i + 1),
			     "timeout did not occur properly");
	}
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_join(&multiple_tid[i], K_FOREVER);
	}
}

/**
 * @brief Test thread mutual exclusion by semaphore
 * @details Test is using to see how mutual exclusion is made by semaphore
 * Made two threads, with two functions which use common variable.
 * That variable is a critical section and can't be changed by two threads
 * at the same time.
 * @ingroup kernel_semaphore_tests
 */
ZTEST(semaphore_1cpu, test_sem_queue_mutual_exclusion)
{
	critical_var = 0;

	expect_k_sem_init_nomsg(&mut_sem, 0, 1, 0);

	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_queue_mutual_exclusion1, NULL, NULL,
			NULL, 1, 0,
			K_NO_WAIT);

	k_thread_create(&sem_tid_2, stack_2, STACK_SIZE,
			sem_queue_mutual_exclusion2, NULL, NULL,
			NULL, 1, 0,
			K_NO_WAIT);

	k_sleep(K_MSEC(100));

	k_sem_give(&mut_sem);
	k_thread_join(&sem_tid_1, K_FOREVER);
	k_thread_join(&sem_tid_2, K_FOREVER);
}

#ifdef CONFIG_USERSPACE
static void thread_sem_give_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_sem_give(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_give() API
 *
 * @details Create a thread and set k_sem_give() input to NULL
 *
 * @ingroup kernel_semaphore_tests
 *
 * @see k_sem_give()
 */
ZTEST_USER(semaphore_null_case, test_sem_give_null)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread_sem_give_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_sem_init_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_sem_init(NULL, 0, 1);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_init() API
 *
 * @details Create a thread and set k_sem_init() input to NULL
 *
 * @ingroup kernel_semaphore_tests
 *
 * @see k_sem_init()
 */
ZTEST_USER(semaphore_null_case, test_sem_init_null)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread_sem_init_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_sem_take_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_sem_take(NULL, K_MSEC(1));

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_take() API
 *
 * @details Create a thread and set k_sem_take() input to NULL
 *
 * @ingroup kernel_semaphore_tests
 *
 * @see k_sem_take()
 */
ZTEST_USER(semaphore_null_case, test_sem_take_null)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread_sem_take_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_sem_reset_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_sem_reset(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_reset() API
 *
 * @details Create a thread and set k_sem_reset() input to NULL
 *
 * @ingroup kernel_semaphore_tests
 *
 * @see k_sem_reset()
 */
ZTEST_USER(semaphore_null_case, test_sem_reset_null)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread_sem_reset_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#endif

#ifdef CONFIG_USERSPACE
static void thread_sem_count_get_null(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ztest_set_fault_valid(true);
	k_sem_count_get(NULL);

	/* should not go here*/
	ztest_test_fail();
}

/**
 * @brief Test k_sem_count_get() API
 *
 * @details Create a thread and set k_sem_count_get() input to NULL
 *
 * @ingroup kernel_semaphore_tests
 *
 * @see k_sem_count_get()
 */
ZTEST_USER(semaphore_null_case, test_sem_count_get_null)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			thread_sem_count_get_null,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(THREAD_TEST_PRIORITY),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	k_thread_join(tid, K_FOREVER);
}
#endif

void *test_init(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(),
			      &simple_sem, &multiple_thread_sem, &low_prio_sem,
				  &mid_prio_sem, &high_prio_sem, &ksema, &msg_sema,
				  &high_prio_long_sem, &stack_1, &stack_2,
				  &stack_3, &stack_4, &timeout_info_pipe,
				  &sem_tid_1, &sem_tid_2, &sem_tid_3,
				  &sem_tid_4, &tstack, &tdata, &mut_sem,
				  &statically_defined_sem);
#endif
	return NULL;
}

ZTEST_SUITE(semaphore, NULL, test_init, NULL, NULL, NULL);
ZTEST_SUITE(semaphore_1cpu, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
ZTEST_SUITE(semaphore_null_case, NULL, test_init, NULL, NULL, NULL);
