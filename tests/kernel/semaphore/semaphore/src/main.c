/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>

/* Macro declarations */
#define SEM_INIT_VAL (0U)
#define SEM_MAX_VAL  (10U)

#define sem_give_from_isr(sema) irq_offload(isr_sem_give, sema)
#define sem_take_from_isr(sema) irq_offload(isr_sem_take, sema)

#define SEM_TIMEOUT (MSEC(100))
#define STACK_SIZE (1024)
#define TOTAL_THREADS_WAITING (5)

struct timeout_info {
	u32_t timeout;
	struct k_sem *sema;
};

/******************************************************************************/
/* Kobject declaration */
K_SEM_DEFINE(simple_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(low_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(mid_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(high_prio_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_SEM_DEFINE(multiple_thread_sem, SEM_INIT_VAL, SEM_MAX_VAL);
K_THREAD_STACK_DEFINE(stack_1, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_2, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_3, STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(multiple_stack, TOTAL_THREADS_WAITING, STACK_SIZE);
K_PIPE_DEFINE(timeout_info_pipe,
	      sizeof(struct timeout_info) * TOTAL_THREADS_WAITING, 4);


__kernel struct k_thread sem_tid, sem_tid_1, sem_tid_2;
__kernel struct k_thread multiple_tid[TOTAL_THREADS_WAITING];

/******************************************************************************/
/* Helper functions */
void isr_sem_give(void *semaphore)
{
	k_sem_give((struct k_sem *)semaphore);
}

void isr_sem_take(void *semaphore)
{
	k_sem_take((struct k_sem *)semaphore, K_NO_WAIT);
}

void sem_give_task(void *p1, void *p2, void *p3)
{
	k_sem_give(&simple_sem);
}

void sem_take_timeout_forever_helper(void *p1, void *p2, void *p3)
{
	k_sleep(MSEC(100));
	k_sem_give(&simple_sem);
}

void sem_take_timeout_isr_helper(void *p1, void *p2, void *p3)
{
	sem_give_from_isr(&simple_sem);
}

void sem_take_multiple_low_prio_helper(void *p1, void *p2, void *p3)
{
	s32_t ret_value;

	ret_value = k_sem_take(&low_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have\n");

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have\n");

	k_sem_give(&low_prio_sem);
}

void sem_take_multiple_mid_prio_helper(void *p1, void *p2, void *p3)
{
	s32_t ret_value;

	ret_value = k_sem_take(&mid_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have\n");

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have\n");

	k_sem_give(&mid_prio_sem);
}

void sem_take_multiple_high_prio_helper(void *p1, void *p2, void *p3)
{
	s32_t ret_value;

	ret_value = k_sem_take(&high_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have\n");

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have\n");

	k_sem_give(&high_prio_sem);
}


/**
 * @ingroup kernel_semaphore_tests
 * @{
 */

/**
 * @brief Test semaphore count when given by an ISR
 * @see k_sem_give()
 */
void test_simple_sem_from_isr(void)
{
	u32_t signal_count;

	/*
	 * Signal the semaphore several times from an ISR.  After each signal,
	 * check the signal count.
	 */

	for (int i = 0; i < 5; i++) {
		sem_give_from_isr(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i + 1),
			     "signal count missmatch Expected %d, got %d\n",
			     (i + 1), signal_count);
	}

}

/**
 * @brief Test semaphore count when given by thread
 * @see k_sem_give()
 */
void test_simple_sem_from_task(void)
{
	u32_t signal_count;

	/*
	 * Signal the semaphore several times from a task.  After each signal,
	 * check the signal count.
	 */

	k_sem_reset(&simple_sem);

	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i + 1),
			     "signal count missmatch Expected %d, got %d\n",
			     (i + 1), signal_count);
	}

}

/**
 * @brief Test if k_sem_take() decreases semaphore count
 * @see k_sem_take()
 */
void test_sem_take_no_wait(void)
{
	u32_t signal_count;
	s32_t ret_value;

	/*
	 * Test the semaphore without wait.  Check the signal count after each
	 * attempt (it should be decrementing by 1 each time).
	 */


	for (int i = 4; i >= 0; i--) {
		ret_value = k_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret_value == 0,
			     "unable to do k_sem_take which returned %d\n",
			     ret_value);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == i,
			     "signal count missmatch Expected %d, got %d\n",
			     i, signal_count);
	}

}

/**
 * @brief Test k_sem_take() when there is no semaphore to take
 * @see k_sem_take()
 */
void test_sem_take_no_wait_fails(void)
{
	u32_t signal_count;
	s32_t ret_value;

	/*
	 * Test the semaphore without wait.  Check the signal count after each
	 * attempt (it should be decrementing by 1 each time).
	 */

	k_sem_reset(&simple_sem);

	for (int i = 4; i >= 0; i--) {
		ret_value = k_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret_value == -EBUSY,
			     "k_sem_take returned when not possible");

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == 0,
			     "signal count missmatch Expected 0, got %d\n",
			     signal_count);
	}

}

/**
 * @brief Test k_sem_take() with timeout expiry
 * @see k_sem_take()
 */
void test_sem_take_timeout_fails(void)
{
	s32_t ret_value;

	/*
	 * Test the semaphore with timeout without a k_sem_give.
	 */

	k_sem_reset(&simple_sem);

	for (int i = 4; i >= 0; i--) {
		ret_value = k_sem_take(&simple_sem, SEM_TIMEOUT);
		zassert_true(ret_value == -EAGAIN,
			     "k_sem_take succeeded when its not possible");
	}

}

/**
 * @brief Test k_sem_take() with timeout
 * @see k_sem_take()
 */
void test_sem_take_timeout(void)
{
	s32_t ret_value;

	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * alternate task (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */
	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_give_task, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_sem_reset(&simple_sem);

	ret_value = k_sem_take(&simple_sem, SEM_TIMEOUT);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have");
	k_thread_abort(&sem_tid);

}

/**
 * @brief Test k_sem_take() with forever timeout
 * @see k_sem_take()
 */
void test_sem_take_timeout_forever(void)
{
	s32_t ret_value;

	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * alternate task (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */
	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_timeout_forever_helper, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_sem_reset(&simple_sem);

	ret_value = k_sem_take(&simple_sem, K_FOREVER);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have");
	k_thread_abort(&sem_tid);

}

/**
 * @brief Test k_sem_take() with timeout in ISR context
 * @see k_sem_take()
 */
void test_sem_take_timeout_isr(void)
{
	s32_t ret_value;

	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * alternate task (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */
	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_timeout_isr_helper, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_sem_reset(&simple_sem);

	ret_value = k_sem_take(&simple_sem, SEM_TIMEOUT);
	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have");
	k_thread_abort(&sem_tid);

}

/**
 * @brief Test multiple semaphore take
 * @see k_sem_take()
 */
void test_sem_take_multiple(void)
{
	u32_t signal_count;

	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * alternate task (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */
	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			sem_take_multiple_low_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_create(&sem_tid_1, stack_2, STACK_SIZE,
			sem_take_multiple_mid_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(2), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_create(&sem_tid_2, stack_3, STACK_SIZE,
			sem_take_multiple_high_prio_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);


	/* time for those 3 threads to complete */
	k_sleep(MSEC(20));

	/* Let these threads proceed to take the multiple_sem */
	k_sem_give(&high_prio_sem);
	k_sem_give(&mid_prio_sem);
	k_sem_give(&low_prio_sem);
	k_sleep(MSEC(200));

	/* enable the higher priority thread to run. */
	k_sem_give(&multiple_thread_sem);
	k_sleep(MSEC(200));
	/* check which threads completed. */
	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1,
		     "Higher priority threads didn't execute");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 0,
		     "Medium priority threads shouldn't have executed");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0,
		     "low priority threads shouldn't have executed");

	/* enable the Medium priority thread to run. */
	k_sem_give(&multiple_thread_sem);
	k_sleep(MSEC(200));
	/* check which threads completed. */
	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1,
		     "Higher priority thread executed again");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 1,
		     "Medium priority thread didn't get executed");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0,
		     "low priority thread shouldn't have executed");

	/* enable the low priority thread to run. */
	k_sem_give(&multiple_thread_sem);
	k_sleep(MSEC(200));
	/* check which threads completed. */
	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1,
		     "Higher priority thread executed again");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 1,
		     "Medium priority thread executed again");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 1,
		     "low priority thread didn't get executed");

}

/**
 * @brief Test semaphore give and take and its count from ISR
 * @see k_sem_give()
 */
void test_sem_give_take_from_isr(void)
{
	u32_t signal_count;

	k_sem_reset(&simple_sem);

	/* Give semaphore from an isr and do a check for the count */
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		sem_give_from_isr(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == i + 1,
			     "signal count missmatch Expected %d, got %d\n",
			     i + 1, signal_count);
	}

	/* Take semaphore from an isr and do a check for the count */
	for (int i = SEM_MAX_VAL; i > 0; i--) {
		sem_take_from_isr(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i - 1),
			     "signal count missmatch Expected %d, got %d\n",
			     (i - 1), signal_count);
	}
}

/**
 * @}
 */

void test_sem_multiple_threads_wait_helper(void *p1, void *p2, void *p3)
{
	/* get blocked until the test thread gives the semaphore */
	k_sem_take(&multiple_thread_sem, K_FOREVER);

	/* Inform the test thread that this thread has got multiple_thread_sem*/
	k_sem_give(&simple_sem);
}


/**
 * @brief Test multiple semaphore take and give with wait
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take(), k_sem_give()
 */
void test_sem_multiple_threads_wait(void)
{
	u32_t signal_count;
	s32_t ret_value;
	u32_t repeat_count = 0;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);


	do {
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_thread_create(&multiple_tid[i],
					multiple_stack[i], STACK_SIZE,
					test_sem_multiple_threads_wait_helper,
					NULL, NULL, NULL,
					K_PRIO_PREEMPT(1),
					K_USER | K_INHERIT_PERMS, K_NO_WAIT);
		}

		/* giving time for the other threads to execute  */
		k_sleep(MSEC(500));

		/* Give the semaphores */
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			k_sem_give(&multiple_thread_sem);
		}

		/* giving time for the other threads to execute  */
		k_sleep(MSEC(500));

		/* check if all the threads are done. */
		for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
			ret_value = k_sem_take(&simple_sem, K_FOREVER);
			zassert_true(ret_value == 0,
				     "Some of the threads didn't get multiple_thread_sem\n"
				     );
		}

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == 0,
			     "signal count missmatch Expected 0, got %d\n",
			     signal_count);

		signal_count = k_sem_count_get(&multiple_thread_sem);
		zassert_true(signal_count == 0,
			     "signal count missmatch Expected 0, got %d\n",
			     signal_count);

		/* Verify a wait q that has been emptied / reset
		 * correctly by running again.
		 */
		repeat_count++;
	} while (repeat_count < 2);
}

/**
 * @brief Test semaphore timeout period
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take(), k_sem_give(), k_sem_reset()
 */
void test_sem_measure_timeouts(void)
{
	s32_t ret_value;
	u32_t start_ticks, end_ticks;

	k_sem_reset(&simple_sem);

	/* With timeout of 1 sec */
	start_ticks = k_uptime_get();

	ret_value = k_sem_take(&simple_sem, SECONDS(1));

	end_ticks = k_uptime_get();

	zassert_true(ret_value == -EAGAIN,
		     "k_sem_take failed when its shouldn't have");

	zassert_true((end_ticks - start_ticks >= SECONDS(1)),
		     "time missmatch expected %d ,got %d\n",
		     SECONDS(1), end_ticks - start_ticks);

	/* With 0 as the timeout */
	start_ticks = k_uptime_get();

	ret_value = k_sem_take(&simple_sem, 0);

	end_ticks = k_uptime_get();

	zassert_true(ret_value == -EBUSY,
		     "k_sem_take failed when its shouldn't have");

	zassert_true((end_ticks - start_ticks < 1),
		     "time missmatch expected %d ,got %d\n",
		     1, end_ticks - start_ticks);

}

void test_sem_measure_timeout_from_thread_helper(void *p1, void *p2, void *p3)
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
	s32_t ret_value;
	u32_t start_ticks, end_ticks;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);

	/* Give a semaphore from a thread and calcualte the time taken.*/
	k_thread_create(&sem_tid, stack_1, STACK_SIZE,
			test_sem_measure_timeout_from_thread_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), 0, K_NO_WAIT);


	/* first sync the 2 threads */
	k_sem_take(&simple_sem, K_FOREVER);

	/* With timeout of 1 sec */
	start_ticks = k_uptime_get();

	ret_value = k_sem_take(&multiple_thread_sem, SECONDS(1));

	end_ticks = k_uptime_get();

	zassert_true(ret_value == 0,
		     "k_sem_take failed when its shouldn't have");

	zassert_true((end_ticks - start_ticks <= SECONDS(1)),
		     "time missmatch. expected less than%d ,got %d\n",
		     SECONDS(1), end_ticks - start_ticks);

}

void test_sem_multiple_take_and_timeouts_helper(void *timeout,
						void *p2,
						void *p3)
{
	u32_t start_ticks, end_ticks;
	size_t bytes_written;

	start_ticks = k_uptime_get();

	k_sem_take(&simple_sem, (int)timeout);

	end_ticks = k_uptime_get();

	zassert_true((end_ticks - start_ticks >= (int)timeout),
		     "time missmatch. expected less than%d ,got %d\n",
		     timeout, end_ticks - start_ticks);


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
	u32_t timeout;
	size_t bytes_read;

	k_sem_reset(&simple_sem);

	/* Multiple threads timeout and the sequence in which it times out
	 * is pushed into a pipe and checked later on.
	 */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i],
				multiple_stack[i], STACK_SIZE,
				test_sem_multiple_take_and_timeouts_helper,
				(void *)SECONDS(i + 1), NULL, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_pipe_get(&timeout_info_pipe, &timeout, sizeof(int),
			   &bytes_read, sizeof(int), K_FOREVER);
		zassert_true(timeout == SECONDS(i + 1),
			     "timeout didn't occur properly");
	}

	/* cleanup */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
	}

}

void test_sem_multi_take_timeout_diff_sem_helper(void *timeout,
						 void *sema,
						 void *p3)
{
	u32_t start_ticks, end_ticks;
	s32_t ret_value;
	size_t bytes_written;
	struct timeout_info info = {
		.timeout = (u32_t) timeout,
		.sema    = (struct k_sem *)sema
	};

	start_ticks = k_uptime_get();

	ret_value = k_sem_take((struct k_sem *)sema, (int)timeout);

	end_ticks = k_uptime_get();

	zassert_true((end_ticks - start_ticks >= (int)timeout),
		     "time missmatch. expected less than%d ,got %d\n",
		     timeout, end_ticks - start_ticks);


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
		{ SECONDS(2), &simple_sem },
		{ SECONDS(1), &multiple_thread_sem },
		{ SECONDS(3), &simple_sem },
		{ SECONDS(5), &multiple_thread_sem },
		{ SECONDS(4), &simple_sem },
	};

	struct timeout_info retrieved_info;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);

	/* Multiple threads timeout on different semaphores and the sequence
	 * in which it times out is pushed into a pipe and checked later on.
	 */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {

		k_thread_create(&multiple_tid[i],
				multiple_stack[i], STACK_SIZE,
				test_sem_multi_take_timeout_diff_sem_helper,
				(void *)seq_info[i].timeout,
				(void *)seq_info[i].sema,
				NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_pipe_get(&timeout_info_pipe,
			   &retrieved_info,
			   sizeof(struct timeout_info),
			   &bytes_read,
			   sizeof(struct timeout_info),
			   K_FOREVER);


		zassert_true(retrieved_info.timeout == SECONDS(i + 1),
			     "timeout didn't occur properly");
	}

}

/* ztest main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(),
			      &simple_sem, &multiple_thread_sem,
			      &low_prio_sem, &mid_prio_sem, &high_prio_sem,
			      &stack_1, &stack_2, &stack_3, &timeout_info_pipe,
			      &sem_tid, &sem_tid_1, &sem_tid_2,
			      NULL);

	ztest_test_suite(test_semaphore,
			 ztest_unit_test(test_simple_sem_from_isr),
			 ztest_user_unit_test(test_simple_sem_from_task),
			 ztest_user_unit_test(test_sem_take_no_wait),
			 ztest_user_unit_test(test_sem_take_no_wait_fails),
			 ztest_user_unit_test(test_sem_take_timeout_fails),
			 ztest_user_unit_test(test_sem_take_timeout),
			 ztest_user_unit_test(test_sem_take_timeout_forever),
			 ztest_unit_test(test_sem_take_timeout_isr),
			 ztest_user_unit_test(test_sem_take_multiple),
			 ztest_unit_test(test_sem_give_take_from_isr),
			 ztest_unit_test(test_sem_multiple_threads_wait),
			 ztest_unit_test(test_sem_measure_timeouts),
			 ztest_unit_test(test_sem_measure_timeout_from_thread),
			 ztest_unit_test(test_sem_multiple_take_and_timeouts),
			 ztest_unit_test(test_sem_multi_take_timeout_diff_sem));
	ztest_run_test_suite(test_semaphore);
}
/******************************************************************************/
