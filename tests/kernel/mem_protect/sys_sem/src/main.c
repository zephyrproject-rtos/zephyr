/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/sys/sem.h>

/* Macro declarations */
#define SEM_INIT_VAL (0U)
#define SEM_MAX_VAL  (10U)
#define SEM_TIMEOUT (K_MSEC(100))
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define TOTAL_THREADS_WAITING (3)

/******************************************************************************/
/* declaration */
ZTEST_BMEM struct sys_sem simple_sem;
ZTEST_BMEM struct sys_sem low_prio_sem;
ZTEST_BMEM struct sys_sem mid_prio_sem;
ZTEST_DMEM struct sys_sem high_prio_sem;
ZTEST_DMEM SYS_SEM_DEFINE(multiple_thread_sem, SEM_INIT_VAL, SEM_MAX_VAL);

K_THREAD_STACK_DEFINE(stack_1, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_2, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_3, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(multiple_stack, TOTAL_THREADS_WAITING, STACK_SIZE);

struct k_thread sem_tid, sem_tid_1, sem_tid_2;
struct k_thread multiple_tid[TOTAL_THREADS_WAITING];

/******************************************************************************/
/* Helper functions */
static void isr_sem_give(const void *semaphore)
{
	sys_sem_give((struct sys_sem *)semaphore);
}

static void isr_sem_take(const void *semaphore)
{
	sys_sem_take((struct sys_sem *)semaphore, K_NO_WAIT);
}

static void sem_give_from_isr(void *semaphore)
{
	irq_offload(isr_sem_give, (const void *)semaphore);
}

static void sem_take_from_isr(void *semaphore)
{
	irq_offload(isr_sem_take, (const void *)semaphore);
}

static void sem_give_task(void *p1, void *p2, void *p3)
{
	sys_sem_give(&simple_sem);
}

static void sem_take_timeout_forever_helper(void *p1, void *p2, void *p3)
{
	k_sleep(K_MSEC(100));
	sys_sem_give(&simple_sem);
}

static void sem_take_timeout_isr_helper(void *p1, void *p2, void *p3)
{
	sem_give_from_isr(&simple_sem);
}

static void sem_take_multiple_low_prio_helper(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = sys_sem_take(&low_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	ret_value = sys_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	sys_sem_give(&low_prio_sem);
}

static void sem_take_multiple_mid_prio_helper(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = sys_sem_take(&mid_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	ret_value = sys_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	sys_sem_give(&mid_prio_sem);
}

static void sem_take_multiple_high_prio_helper(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = sys_sem_take(&high_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	ret_value = sys_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	sys_sem_give(&high_prio_sem);
}

static void sem_multiple_threads_wait_helper(void *p1, void *p2, void *p3)
{
	int ret_value;

	/* get blocked until the test thread gives the semaphore */
	ret_value = sys_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "sys_sem_take failed");

	/* Inform the test thread that this thread has got multiple_thread_sem*/
	sys_sem_give(&simple_sem);
}

/**
 * @brief System semaphore (sys_sem) API tests
 *
 * @defgroup sys_sem_tests System Semaphore
 *
 * @ingroup all_tests
 *
 * This module tests the userspace-capable system semaphore APIs:
 * sys_sem_init(), sys_sem_give(), sys_sem_take() and sys_sem_count_get().
 * @{
 */

/**
 * @brief Verify sys_sem_init() rejects invalid arguments with -EINVAL.
 *
 * @details
 * sys_sem_init() must validate its arguments and refuse to initialize a
 * semaphore when they are nonsensical, returning -EINVAL rather than
 * producing an unusable object. A valid initialization must then succeed and
 * support a take/give cycle. This case is userspace-only.
 *
 * Test steps:
 * - Call sys_sem_init() with a NULL semaphore pointer.
 * - Call sys_sem_init() with an initial count equal to the max (limit == 0).
 * - Call sys_sem_init() with an initial count above the representable range.
 * - Call sys_sem_init() with a limit above the representable range.
 * - Finally initialize with valid values and perform a take then a give.
 *
 * Expected result:
 * - Every invalid call returns -EINVAL.
 * - The valid initialization succeeds and the take/give cycle completes.
 *
 * @see sys_sem_init()
 */
ZTEST(sys_sem, test_basic_sem_test)
{
	int32_t ret_value;

	if (!IS_ENABLED(CONFIG_USERSPACE)) {
		/*
		 * The argument validation exercised below (returning -EINVAL)
		 * only exists in the userspace sys_sem_init(). The supervisor
		 * build is a thin k_sem wrapper that performs no validation, so
		 * skip rather than compile the case out: keeping it always
		 * compiled lets twister observe a reported result on
		 * non-userspace platforms instead of a missing (None) status.
		 */
		ztest_test_skip();
	}

	ret_value = sys_sem_init(NULL, SEM_INIT_VAL, SEM_MAX_VAL);
	zassert_true(ret_value == -EINVAL,
		     "sys_sem_init returned not equal -EINVAL");

	ret_value = sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_INIT_VAL);
	zassert_true(ret_value == -EINVAL,
		     "sys_sem_init returned not equal -EINVAL");

	ret_value = sys_sem_init(&simple_sem, UINT_MAX, SEM_MAX_VAL);
	zassert_true(ret_value == -EINVAL,
		     "sys_sem_init returned not equal -EINVAL");

	ret_value = sys_sem_init(&simple_sem, SEM_MAX_VAL, UINT_MAX);
	zassert_true(ret_value == -EINVAL,
		     "sys_sem_init returned not equal -EINVAL");

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	sys_sem_take(&simple_sem, SEM_TIMEOUT);
	sys_sem_give(&simple_sem);
}

/**
 * @brief Verify sys_sem_give() from an ISR increments the count.
 *
 * @details
 * A sys_sem given from interrupt context must increase its available count by
 * one per give, observable via sys_sem_count_get().
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Give the semaphore from an ISR five times.
 * - After each give, read the count.
 *
 * Expected result:
 * - The count equals the number of gives performed so far (1..5).
 *
 * @see sys_sem_give()
 * @see sys_sem_count_get()
 */
ZTEST(sys_sem, test_simple_sem_from_isr)
{
	uint32_t signal_count;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	for (int i = 0; i < 5; i++) {
		sem_give_from_isr(&simple_sem);

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i + 1),
			     "signal count mismatch Expected %d, got %d",
			     (i + 1), signal_count);
	}

}

/**
 * @brief Verify sys_sem_give() from a user thread increments the count.
 *
 * @details
 * The same give-increments-count behavior must hold when invoked from a
 * user-mode thread, exercising the userspace path of the API.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Give the semaphore five times from the (user) test thread.
 * - After each give, read the count.
 *
 * Expected result:
 * - The count equals the number of gives performed so far (1..5).
 *
 * @see sys_sem_give()
 * @see sys_sem_count_get()
 */
ZTEST_USER(sys_sem, test_simple_sem_from_task)
{
	uint32_t signal_count;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	for (int i = 0; i < 5; i++) {
		sys_sem_give(&simple_sem);

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i + 1),
			     "signal count mismatch Expected %d, got %d",
			     (i + 1), signal_count);
	}
}

/**
 * @brief Verify sys_sem_take() succeeds and decrements when count is positive.
 *
 * @details
 * When tokens are available, sys_sem_take() with K_NO_WAIT must succeed
 * immediately and reduce the count by one each time.
 *
 * Test steps:
 * - Initialize a semaphore with count 5.
 * - Take with K_NO_WAIT five times.
 * - After each take, read the count.
 *
 * Expected result:
 * - Every take returns 0 and the count decrements 4, 3, 2, 1, 0.
 *
 * @see sys_sem_take()
 * @see sys_sem_count_get()
 */
ZTEST_USER(sys_sem, test_sem_take_no_wait)
{
	uint32_t signal_count;
	int32_t ret_value;

	/* Initial condition */
	sys_sem_init(&simple_sem, 5, SEM_MAX_VAL);

	for (int i = 4; i >= 0; i--) {
		ret_value = sys_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret_value == 0,
			     "unable to do sys_sem_take which returned %d",
			     ret_value);

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == i,
			     "signal count mismatch Expected %d, got %d",
			     i, signal_count);
	}

}

/**
 * @brief Verify sys_sem_take() with K_NO_WAIT fails when count is zero.
 *
 * @details
 * With no tokens available, a non-blocking take must fail with -ETIMEDOUT and
 * must not corrupt or change the count.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Attempt a K_NO_WAIT take five times.
 * - After each attempt, read the count.
 *
 * Expected result:
 * - Every take returns -ETIMEDOUT and the count stays 0.
 *
 * @see sys_sem_take()
 */
ZTEST_USER(sys_sem, test_sem_take_no_wait_fails)
{
	uint32_t signal_count;
	int32_t ret_value;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	for (int i = 4; i >= 0; i--) {
		ret_value = sys_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret_value == -ETIMEDOUT,
			     "sys_sem_take returned when not possible");

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == 0U,
			     "signal count mismatch Expected 0, got %d",
			     signal_count);
	}

}

/**
 * @brief Verify sys_sem_take() returns -ETIMEDOUT when its timeout expires.
 *
 * @details
 * With no token available and no other thread giving, a take with a finite
 * timeout must block for the timeout and then fail with -ETIMEDOUT.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Take with a finite timeout (SEM_TIMEOUT) five times.
 *
 * Expected result:
 * - Every take returns -ETIMEDOUT after the timeout elapses.
 *
 * @see sys_sem_take()
 */
ZTEST_USER(sys_sem_1cpu, test_sem_take_timeout_fails)
{
	int32_t ret_value;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	for (int i = 4; i >= 0; i--) {
		ret_value = sys_sem_take(&simple_sem, SEM_TIMEOUT);
		zassert_true(ret_value == -ETIMEDOUT,
			     "sys_sem_take succeeded when its not possible");
	}

}

/**
 * @brief Verify sys_sem_take() with a timeout succeeds when a give arrives.
 *
 * @details
 * A take blocked on a finite timeout must succeed if another thread gives the
 * semaphore before the timeout expires.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Start a thread that gives the semaphore.
 * - In the test thread, take with a finite timeout (SEM_TIMEOUT).
 *
 * Expected result:
 * - The take returns 0 (the give satisfied it before the timeout).
 *
 * @see sys_sem_take()
 * @see sys_sem_give()
 */
ZTEST_USER(sys_sem, test_sem_take_timeout)
{
	int32_t ret_value;
#ifdef CONFIG_USERSPACE
	int thread_flags = K_USER | K_INHERIT_PERMS;
#else
	int thread_flags = 0;
#endif

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_give_task, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), thread_flags,
			K_NO_WAIT);

	ret_value = sys_sem_take(&simple_sem, SEM_TIMEOUT);
	zassert_true(ret_value == 0,
		     "sys_sem_take failed when its shouldn't have");

	k_thread_join(&sem_tid, K_FOREVER);
}

/**
 * @brief Verify sys_sem_take() with K_FOREVER blocks until a give arrives.
 *
 * @details
 * A take with K_FOREVER must block indefinitely and wake only when another
 * thread gives the semaphore, then succeed.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Start a thread that sleeps briefly and then gives the semaphore.
 * - In the test thread, take with K_FOREVER.
 *
 * Expected result:
 * - The take returns 0 once the helper thread gives the semaphore.
 *
 * @see sys_sem_take()
 * @see sys_sem_give()
 */
ZTEST_USER(sys_sem_1cpu, test_sem_take_timeout_forever)
{
	int32_t ret_value;
#ifdef CONFIG_USERSPACE
	int thread_flags = K_USER | K_INHERIT_PERMS;
#else
	int thread_flags = 0;
#endif

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_timeout_forever_helper, NULL,
			NULL, NULL, K_PRIO_PREEMPT(0), thread_flags,
			K_NO_WAIT);

	ret_value = sys_sem_take(&simple_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "sys_sem_take failed when its shouldn't have");

	k_thread_join(&sem_tid, K_FOREVER);
}

/**
 * @brief Verify a timed sys_sem_take() is satisfied by a give from an ISR.
 *
 * @details
 * A take blocked on a finite timeout must be woken and succeed when the
 * semaphore is given from interrupt context.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Start a thread that gives the semaphore from an ISR.
 * - In the test thread, take with a finite timeout (SEM_TIMEOUT).
 *
 * Expected result:
 * - The take returns 0 (satisfied by the ISR give before the timeout).
 *
 * @see sys_sem_take()
 * @see sys_sem_give()
 */
ZTEST(sys_sem_1cpu, test_sem_take_timeout_isr)
{
	int32_t ret_value;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_timeout_isr_helper, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	ret_value = sys_sem_take(&simple_sem, SEM_TIMEOUT);
	zassert_true(ret_value == 0,
		     "sys_sem_take failed when its shouldn't have");

	k_thread_join(&sem_tid, K_FOREVER);
}

static void sem_take_kernel_user(void *p1, void *p2, void *p3)
{
	int ret_value = sys_sem_take(&simple_sem, SEM_TIMEOUT);

	zassert_ok(ret_value, "sys_sem_take failed: %d", ret_value);

	sys_sem_give(&mid_prio_sem);
}

/**
 * @brief Verify a sys_sem is shared correctly between kernel and user threads.
 *
 * @details
 * A sys_sem given by a kernel thread must be takeable by a user thread and
 * vice versa, demonstrating that the object works across the privilege
 * boundary.
 *
 * Test steps:
 * - Initialize two semaphores with count 0.
 * - Start a user thread that takes the first semaphore then gives the second.
 * - The kernel test thread gives the first semaphore, then takes the second
 *   with a finite timeout.
 *
 * Expected result:
 * - The user thread's take succeeds and the kernel thread's take of the
 *   second semaphore returns 0.
 *
 * @see sys_sem_take()
 * @see sys_sem_give()
 */
ZTEST(sys_sem_1cpu, test_sem_take_kernel_user)
{
#ifdef CONFIG_USERSPACE
	int thread_flags = K_USER;
#else
	int thread_flags = 0;
#endif

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	sys_sem_init(&mid_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_kernel_user, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), thread_flags, K_NO_WAIT);

	sys_sem_give(&simple_sem);

	int ret_value = sys_sem_take(&mid_prio_sem, SEM_TIMEOUT);

	zassert_ok(ret_value, "sys_sem_take kernel failed: %d", ret_value);

	k_thread_join(&sem_tid, K_FOREVER);
}

/**
 * @brief Verify pended sys_sem takers wake in thread-priority order.
 *
 * @details
 * When several threads of different priorities are all blocked taking the same
 * semaphore, each give must release the highest-priority waiter first, so the
 * threads complete strictly in priority order.
 *
 * Test steps:
 * - Create high-, mid-, and low-priority helper threads, each of which takes
 *   its own gate semaphore and then blocks taking the shared semaphore.
 * - Lower the test thread's priority and release all three gates so the
 *   helpers pend on the shared semaphore.
 * - Give the shared semaphore once and check which helper progressed; repeat.
 *
 * Expected result:
 * - The high-priority helper completes on the first give, the mid-priority on
 *   the second, and the low-priority on the third — never out of order.
 *
 * @see sys_sem_take()
 * @see sys_sem_give()
 */
ZTEST_USER(sys_sem_1cpu, test_sem_take_multiple)
{
	uint32_t signal_count;
#ifdef CONFIG_USERSPACE
	int thread_flags = K_USER | K_INHERIT_PERMS;
#else
	int thread_flags = 0;
#endif

	sys_sem_init(&high_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	sys_sem_init(&mid_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	sys_sem_init(&low_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_multiple_low_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), thread_flags,
			K_NO_WAIT);

	k_thread_create(&sem_tid_1, stack_2, STACK_SIZE,
			sem_take_multiple_mid_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(2), thread_flags,
			K_NO_WAIT);

	k_thread_create(&sem_tid_2, stack_3, STACK_SIZE,
			sem_take_multiple_high_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), thread_flags,
			K_NO_WAIT);


	/* Lower the priority */
	k_thread_priority_set(k_current_get(), K_PRIO_PREEMPT(3));

	/* giving time for those 3 threads to complete */
	k_yield();

	/* Let these threads proceed to take the multiple_sem */
	sys_sem_give(&high_prio_sem);
	sys_sem_give(&mid_prio_sem);
	sys_sem_give(&low_prio_sem);
	k_yield();

	/* enable the higher priority thread to run. */
	sys_sem_give(&multiple_thread_sem);
	k_yield();

	/* check which threads completed. */
	signal_count = sys_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1U,
		     "Higher priority threads didn't execute");

	signal_count = sys_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 0U,
		     "Medium priority threads shouldn't have executed");

	signal_count = sys_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0U,
		     "low priority threads shouldn't have executed");

	/* enable the Medium priority thread to run. */
	sys_sem_give(&multiple_thread_sem);
	k_yield();

	/* check which threads completed. */
	signal_count = sys_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1U,
		     "Higher priority thread executed again");

	signal_count = sys_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 1U,
		     "Medium priority thread didn't get executed");

	signal_count = sys_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0U,
		     "low priority thread shouldn't have executed");

	/* enable the low priority thread to run. */
	sys_sem_give(&multiple_thread_sem);
	k_yield();

	/* check which threads completed. */
	signal_count = sys_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1U,
		     "Higher priority thread executed again");

	signal_count = sys_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 1U,
		     "Medium priority thread executed again");

	signal_count = sys_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 1U,
		     "low priority thread didn't get executed");

	k_thread_join(&sem_tid, K_FOREVER);
	k_thread_join(&sem_tid_1, K_FOREVER);
	k_thread_join(&sem_tid_2, K_FOREVER);
}

/**
 * @brief Verify give/take from an ISR track the count up and back down.
 *
 * @details
 * Repeated gives from interrupt context must raise the count to the maximum,
 * and repeated takes from interrupt context must lower it back to zero, with
 * the count accurate at every step.
 *
 * Test steps:
 * - Initialize a semaphore with count 0.
 * - Give from an ISR SEM_MAX_VAL times, checking the count rises 1..MAX.
 * - Take from an ISR SEM_MAX_VAL times, checking the count falls MAX-1..0.
 *
 * Expected result:
 * - The count matches the expected value after every give and take.
 *
 * @see sys_sem_give()
 * @see sys_sem_take()
 * @see sys_sem_count_get()
 */
ZTEST(sys_sem, test_sem_give_take_from_isr)
{
	uint32_t signal_count;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	/* Give semaphore from an isr and do a check for the count */
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		sem_give_from_isr(&simple_sem);

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == i + 1,
			     "signal count mismatch Expected %d, got %d",
			     i + 1, signal_count);
	}

	/* Take semaphore from an isr and do a check for the count */
	for (int i = SEM_MAX_VAL; i > 0; i--) {
		sem_take_from_isr(&simple_sem);

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i - 1),
			     "signal count mismatch Expected %d, got %d",
			     (i - 1), signal_count);
	}
}

/**
 * @brief Verify sys_sem_give() does not raise the count past its maximum.
 *
 * @details
 * The count must saturate at the configured maximum: gives up to the limit
 * succeed, and a give at the limit must report -EAGAIN without exceeding the
 * maximum.
 *
 * Test steps:
 * - Initialize a semaphore with count 0 and max SEM_MAX_VAL.
 * - Give SEM_MAX_VAL times, checking the count rises 1..MAX.
 * - Give once more at the limit and inspect the return value and count.
 *
 * Expected result:
 * - Gives within the limit return 0; a give at the limit returns -EAGAIN and
 *   the count never exceeds SEM_MAX_VAL.
 *
 * @see sys_sem_give()
 * @see sys_sem_count_get()
 */
ZTEST_USER(sys_sem, test_sem_give_limit)
{
	int32_t ret_value;
	uint32_t signal_count;

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	/* Give semaphore and do a check for the count */
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		ret_value = sys_sem_give(&simple_sem);
		zassert_true(ret_value == 0,
			     "sys_sem_give failed when its shouldn't have");

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == i + 1,
			     "signal count mismatch Expected %d, got %d",
			     i + 1, signal_count);
	}

	do {
		ret_value = sys_sem_give(&simple_sem);
		if (ret_value == -EAGAIN) {
			signal_count = sys_sem_count_get(&simple_sem);
			zassert_true(signal_count == SEM_MAX_VAL,
				"signal count mismatch Expected %d, got %d",
				SEM_MAX_VAL, signal_count);

			sys_sem_take(&simple_sem, K_FOREVER);
		} else if (ret_value == 0) {
			signal_count = sys_sem_count_get(&simple_sem);
			zassert_true(signal_count == SEM_MAX_VAL,
				"signal count mismatch Expected %d, got %d",
				SEM_MAX_VAL, signal_count);
		}
	} while (ret_value == -EAGAIN);
}

/**
 * @brief Verify multiple threads blocked on a sys_sem are each released by a give.
 *
 * @details
 * When several threads block taking the same semaphore, one give must release
 * exactly one waiter, so N gives release all N waiters and leave both
 * semaphores drained. Repeated to confirm the behavior is stable across runs.
 *
 * Test steps:
 * - Initialize the semaphore with count 0.
 * - Create TOTAL_THREADS_WAITING helper threads that each block taking the
 *   shared semaphore and then give a signalling semaphore.
 * - Give the shared semaphore once per helper, then collect one signal per
 *   helper and check both semaphore counts.
 * - Repeat the whole sequence twice.
 *
 * Expected result:
 * - Every helper is released (one per give); both counts end at 0 each round.
 *
 * @see sys_sem_take()
 * @see sys_sem_give()
 * @see sys_sem_count_get()
 */
ZTEST_USER(sys_sem_1cpu, test_sem_multiple_threads_wait)
{
	uint32_t signal_count;
	int32_t ret_value;
	uint32_t repeat_count = 0U;
#ifdef CONFIG_USERSPACE
	int thread_flags = K_USER | K_INHERIT_PERMS;
#else
	int thread_flags = 0;
#endif

	sys_sem_init(&simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	do {
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_thread_create(&multiple_tid[i],
					multiple_stack[i], STACK_SIZE,
					sem_multiple_threads_wait_helper,
					NULL, NULL, NULL,
					CONFIG_ZTEST_THREAD_PRIORITY,
					thread_flags, K_NO_WAIT);
		}

		/* giving time for the other threads to execute  */
		k_yield();

		/* Give the semaphores */
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			sys_sem_give(&multiple_thread_sem);
		}

		/* giving time for the other threads to execute  */
		k_yield();

		/* check if all the threads are done. */
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			ret_value = sys_sem_take(&simple_sem, K_FOREVER);
			zassert_true(ret_value == 0,
				     "Some of the threads didn't get multiple_thread_sem"
				     );
		}

		signal_count = sys_sem_count_get(&simple_sem);
		zassert_true(signal_count == 0U,
			     "signal count mismatch Expected 0, got %d",
			     signal_count);

		signal_count = sys_sem_count_get(&multiple_thread_sem);
		zassert_true(signal_count == 0U,
			     "signal count mismatch Expected 0, got %d",
			     signal_count);

		repeat_count++;

		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_thread_join(&multiple_tid[i], K_FOREVER);
		}
	} while (repeat_count < 2);
}

/**
 * @}
 */

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	printk("Caught system error -- reason %d\n", reason);
	printk("Unexpected fault during test\n");
	TC_END_REPORT(TC_FAIL);
	k_fatal_halt(reason);
}

void *sys_sem_setup(void)
{
#ifdef CONFIG_USERSPACE
	k_thread_access_grant(k_current_get(),
			      &stack_1, &stack_2, &stack_3,
			      &sem_tid, &sem_tid_1, &sem_tid_2);

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_access_grant(k_current_get(),
			&multiple_tid[i], &multiple_stack[i]);
	}
#endif

	return NULL;
}

ZTEST_SUITE(sys_sem, NULL, sys_sem_setup, NULL, NULL, NULL);

ZTEST_SUITE(sys_sem_1cpu, NULL, sys_sem_setup, ztest_simple_1cpu_before,
		ztest_simple_1cpu_after, NULL);

/******************************************************************************/
