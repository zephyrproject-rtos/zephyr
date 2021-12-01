/*../../include/test_sem_common.h
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>
#include "../../common_files/include/sem_test.h"

/* Macro declarations */
#define SEM_MAX_VAL  (10U)
#define THREAD_TEST_PRIORITY 0

#define sem_give_from_isr(sema) irq_offload(isr_sem_give, (const void *)sema)
#define sem_take_from_isr(sema) irq_offload(isr_sem_take, (const void *)sema)

#define TOTAL_THREADS_WAITING (3)

#define SEC2MS(s) ((s) * 1000)
#define QSEC2MS(s) ((s) * 250)

#define multiple_tid multi_tid_give
#define multiple_stack multi_stack_give

#define sem_tid_1 multi_tid_give[TOTAL_THREADS_WAITING]
#define sem_tid_2 multi_tid_give[TOTAL_THREADS_WAITING+1]

#define stack_1 multi_stack_give[TOTAL_THREADS_WAITING]
#define stack_2 multi_stack_give[TOTAL_THREADS_WAITING+1]

/* global variable for mutual exclusion test */
uint32_t critical_var;

struct timeout_info {
	uint32_t timeout;
	struct k_sem *sema;
};

/******************************************************************************/
/* Kobject declaration */
K_SEM_DEFINE(simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(multiple_thread_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(ksema, SEM_INIT_VAL, SEM_MAX_VAL);

static struct k_sem sema;

K_PIPE_DEFINE(timeout_info_pipe,
	      sizeof(struct timeout_info) * TOTAL_THREADS_WAITING, 2);

extern struct k_sem usage_sem, sync_sem, limit_sem;

extern struct k_thread multi_tid_give[STACK_NUMS];
extern struct k_thread multi_tid_take[STACK_NUMS];
extern K_THREAD_STACK_ARRAY_DEFINE(multi_stack_give, STACK_NUMS, STACK_SIZE);
extern K_THREAD_STACK_ARRAY_DEFINE(multi_stack_take, STACK_NUMS, STACK_SIZE);

extern void test_multiple_thread_sem_usage(void);
extern void test_multi_thread_sem_limit(void);

/******************************************************************************/
/* Helper functions */

static void sem_give_task(void *p1, void *p2, void *p3)
{
	k_sem_give((struct k_sem *)p1);
}

static void isr_sem_give(const void *semaphore)
{
	k_sem_give((struct k_sem *)semaphore);
}

static void tsema_thread_thread(struct k_sem *psem)
{
	/**TESTPOINT: thread-thread sync via sema*/
	k_tid_t tid = k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
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


static void isr_sem_take(const void *semaphore)
{
	int ret = k_sem_take((struct k_sem *)semaphore, K_NO_WAIT);

	if (ret != 0 && ret != -EBUSY) {
		zassert_true(false, "incorrect k_sem_take return: %d", ret);
	}
}


static void sem_take_timeout_isr_helper(void *p1, void *p2, void *p3)
{
	sem_give_from_isr(&simple_sem);
}

/**
 * @ingroup kernel_semaphore_tests
 * @{
 */

/**
 * @brief Test synchronization of threads with semaphore
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 */
void test_sem_thread2thread(void)
{
	/**TESTPOINT: test k_sem_init sema*/
	expect_k_sem_init_nomsg(&sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	tsema_thread_thread(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_thread(&ksema);
}

/**
 * @brief Test synchronization between thread and irq
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 */
void test_sem_thread2isr(void)
{
	/**TESTPOINT: test k_sem_init sema*/
	expect_k_sem_init_nomsg(&sema, SEM_INIT_VAL, SEM_MAX_VAL, 0);

	tsema_thread_isr(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_isr(&ksema);
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
 */
void test_sem_give_from_isr(void)
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
 * @brief Test k_sem_take() with timeout in ISR context
 * @see k_sem_take()
 */
void test_sem_take_timeout_isr(void)
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
 * @brief Test semaphore give and take and its count from ISR
 * @see k_sem_give()
 */
void test_sem_give_take_from_isr(void)
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

/**
 * @brief Test semaphore timeout period
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take(), k_sem_give(), k_sem_reset()
 */
void test_sem_measure_timeouts(void)
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

static void sem_measure_timeout_from_thread_helper(void *p1, void *p2, void *p3)
{
	/* first sync the 2 threads */
	k_sem_give(&simple_sem);

	/* give the semaphore */
	k_sem_give(&multiple_thread_sem);
}


/**
 * @brief Test timeout of semaphore from thread
 * @ingroup kernel_semaphore_tests
 * @see k_sem_give(), k_sem_reset(), k_sem_take()
 */
void test_sem_measure_timeout_from_thread(void)
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
		     diff_ticks, SEC2MS(1));

}

static void sem_multiple_take_and_timeouts_helper(void *p1, void *p2, void *p3)
{
	int timeout = POINTER_TO_INT(p1);
	int64_t start_ticks, end_ticks, diff_ticks;
	size_t bytes_written;

	start_ticks = k_uptime_get();

	expect_k_sem_take_nomsg(&simple_sem, K_MSEC(timeout), -EAGAIN);

	end_ticks = k_uptime_get();

	diff_ticks = end_ticks - start_ticks;

	zassert_true(diff_ticks >= timeout,
		     "time mismatch - expected at least %d, got %lld",
		     timeout, diff_ticks);

	k_pipe_put(&timeout_info_pipe, &timeout, sizeof(int),
		   &bytes_written, sizeof(int), K_FOREVER);

}

/**
 * @brief Test multiple semaphore take with timeouts
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take(), k_sem_reset()
 */
void test_sem_multiple_take_and_timeouts(void)
{
	static uint32_t timeout;
	size_t bytes_read;

	k_sem_reset(&simple_sem);

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
		k_pipe_get(&timeout_info_pipe, &timeout, sizeof(int),
			   &bytes_read, sizeof(int), K_FOREVER);
		zassert_equal(timeout, QSEC2MS(i + 1),
			     "timeout did not occur properly: %d != %d",
				 timeout, QSEC2MS(i + 1));
	}

	/* cleanup */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_join(&multiple_tid[i], K_FOREVER);
	}
}

static void sem_multi_take_timeout_diff_sem_helper(void *p1, void *p2, void *p3)
{
	int timeout = POINTER_TO_INT(p1);
	struct k_sem *sema = p2;
	int64_t start_ticks, end_ticks, diff_ticks;
	size_t bytes_written;
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

	k_pipe_put(&timeout_info_pipe, &info, sizeof(struct timeout_info),
		   &bytes_written, sizeof(struct timeout_info), K_FOREVER);
}

/**
 * @brief Test sequence of multiple semaphore timeouts
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take(), k_sem_reset()
 */
void test_sem_multi_take_timeout_diff_sem(void)
{
	size_t bytes_read;
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
		k_pipe_get(&timeout_info_pipe,
			   &retrieved_info,
			   sizeof(struct timeout_info),
			   &bytes_read,
			   sizeof(struct timeout_info),
			   K_FOREVER);


		zassert_true(retrieved_info.timeout == SEC2MS(i + 1),
			     "timeout did not occur properly");
	}

}

/* First function for mutual exclusion test */
static void sem_queue_mutual_exclusion1(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < 5; i++) {
		expect_k_sem_take_nomsg(&sema, K_FOREVER, 0);

		/* in that function critical section makes critical var +1 */
		critical_var += 1;

		/* Check that common value was not changed by another thread,
		 * when semaphore is taken by current thread, and no other
		 * thread can enter the critical section
		 */
		zassert_true(critical_var == 1, NULL);
		k_sem_give(&sema);
	}
}

/* Second function for mutual exclusion test */
static void sem_queue_mutual_exclusion2(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < 5; i++) {
		expect_k_sem_take_nomsg(&sema, K_FOREVER, 0);

		/* in that function critical section makes critical var 0 */
		critical_var -= 1;

		/* Check that common value was not changed by another thread,
		 * when semaphore is taken by current thread, and no other
		 * thread can enter the critical section
		 */
		zassert_true(critical_var == 0, NULL);
		k_sem_give(&sema);
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
void test_sem_queue_mutual_exclusion(void)
{
	critical_var = 0;

	expect_k_sem_init_nomsg(&sema, 0, 1, 0);

	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_queue_mutual_exclusion1, NULL, NULL,
			NULL, 1, 0,
			K_NO_WAIT);

	k_thread_create(&sem_tid_2, stack_2, STACK_SIZE,
			sem_queue_mutual_exclusion2, NULL, NULL,
			NULL, 1, 0,
			K_NO_WAIT);

	k_sleep(K_MSEC(100));

	k_sem_give(&sema);

	/* cleanup */
	k_thread_join(&sem_tid_1, K_FOREVER);
	k_thread_join(&sem_tid_2, K_FOREVER);

}

/* ztest main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(),
				&simple_sem, &multiple_thread_sem,
				&ksema, &sema, &timeout_info_pipe,
				&usage_sem, &sync_sem, &limit_sem,
				&multi_tid_give[0], &multi_tid_give[1],
				&multi_tid_give[2], &multi_tid_give[3],
				&multi_tid_give[4], &multi_tid_take[4],
				&multi_tid_take[2], &multi_tid_take[3],
				&multi_tid_take[0], &multi_tid_take[1],
				&multi_tid_give[5], &multi_tid_take[5],
				&multi_stack_take[0], &multi_stack_take[1],
				&multi_stack_take[3], &multi_stack_take[4],
				&multi_stack_take[2], &multi_stack_give[0],
				&multi_stack_give[1], &multi_stack_give[2],
				&multi_stack_give[3], &multi_stack_give[4]);

	ztest_test_suite(test_kernel_sys_sem,
			 ztest_user_unit_test(test_sem_thread2thread),
			 ztest_unit_test(test_sem_thread2isr),
			 ztest_unit_test(test_sem_give_from_isr),
			 ztest_unit_test(test_sem_take_timeout_isr),
			 ztest_unit_test(test_sem_give_take_from_isr),
			 ztest_unit_test(test_sem_measure_timeouts),
			 ztest_unit_test(test_sem_measure_timeout_from_thread),
			 ztest_1cpu_unit_test(test_sem_multiple_take_and_timeouts),
			 ztest_unit_test(test_sem_multi_take_timeout_diff_sem),
			 ztest_1cpu_unit_test(test_sem_queue_mutual_exclusion),
			 ztest_1cpu_user_unit_test(test_multiple_thread_sem_usage),
			 ztest_1cpu_user_unit_test(test_multi_thread_sem_limit));
	ztest_run_test_suite(test_kernel_sys_sem);
}
