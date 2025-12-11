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
 * @ingroup sys_sem_tests
 * @{
 */

#ifdef CONFIG_USERSPACE
ZTEST(sys_sem, test_basic_sem_test)
{
	int32_t ret_value;

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
#endif

/**
 * @brief Test semaphore count when given by an ISR
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
 * @brief Test semaphore count when given by thread
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
 * @brief Test if sys_sem_take() decreases semaphore count
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
 * @brief Test sys_sem_take() when there is no semaphore to take
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
 * @brief Test sys_sem_take() with timeout expiry
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
 * @brief Test sys_sem_take() with timeout
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
 * @brief Test sys_sem_take() with forever timeout
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
 * @brief Test sys_sem_take() with timeout in ISR context
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

/**
 * @brief Test multiple semaphore take
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
 * @brief Test semaphore give and take and its count from ISR
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
 * @brief Test semaphore give limit count
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
 * @brief Test multiple semaphore take and give with wait
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
