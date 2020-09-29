/*
 * Copyright (c) 2016, 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <irq_offload.h>

/* Macro declarations */
#define SEM_INIT_VAL (0U)
#define SEM_MAX_VAL  (10U)

#define sem_give_from_isr(sema) irq_offload(isr_sem_give, (const void *)sema)
#define sem_take_from_isr(sema) irq_offload(isr_sem_take, (const void *)sema)

#define SEM_TIMEOUT (K_MSEC(100))
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define TOTAL_THREADS_WAITING (5)

#define SEC2MS(s) ((s) * 1000)

/* global variable for mutual exclusion test */
uint32_t critical_var;

struct timeout_info {
	uint32_t timeout;
	struct k_sem *sema;
};

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
K_PIPE_DEFINE(timeout_info_pipe,
	      sizeof(struct timeout_info) * TOTAL_THREADS_WAITING, 4);

struct k_thread sem_tid_1, sem_tid_2, sem_tid_3, sem_tid_4;
struct k_thread multiple_tid[TOTAL_THREADS_WAITING];

K_SEM_DEFINE(ksema, SEM_INIT_VAL, SEM_MAX_VAL);
struct k_sem sema, mut_sem;
static K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
struct k_thread tdata;

/******************************************************************************/
/* Helper functions */

void sem_give_task(void *p1, void *p2, void *p3)
{
	k_sem_give((struct k_sem *)p1);
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

	zassert_false(k_sem_take(psem, K_FOREVER), NULL);

	/*clean the spawn thread avoid side effect in next TC*/
	k_thread_abort(tid);
}

static void tsema_thread_isr(struct k_sem *psem)
{
	/**TESTPOINT: thread-isr sync via sema*/
	irq_offload(isr_sem_give, (const void *)psem);
	zassert_false(k_sem_take(psem, K_FOREVER), NULL);
}


void isr_sem_take(const void *semaphore)
{
	k_sem_take((struct k_sem *)semaphore, K_NO_WAIT);
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
	int32_t ret_value;

	ret_value = k_sem_take(&low_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	k_sem_give(&low_prio_sem);
}

void sem_take_multiple_mid_prio_helper(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = k_sem_take(&mid_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	k_sem_give(&mid_prio_sem);
}

void sem_take_multiple_high_prio_helper(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = k_sem_take(&high_prio_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	k_sem_give(&high_prio_sem);
}

/* First function for mutual exclusion test */
void sem_queue_mutual_exclusion1(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < 5; i++) {
		k_sem_take(&mut_sem, K_FOREVER);

		/* in that function critical section makes critical var +1 */
		critical_var += 1;

		/* Check that common value was not changed by another thread,
		 * when semaphore is taken by current thread, and no other
		 * thread can enter the critical section
		 */
		zassert_true(critical_var == 1, NULL);
		k_sem_give(&mut_sem);
	}
}

/* Second function for mutual exclusion test */
void sem_queue_mutual_exclusion2(void *p1, void *p2, void *p3)
{
	for (int i = 0; i < 5; i++) {
		k_sem_take(&mut_sem, K_FOREVER);

		/* in that function critical section makes critical var 0 */
		critical_var -= 1;

		/* Check that common value was not changed by another thread,
		 * when semaphore is taken by current thread, and no other
		 * thread can enter the critical section
		 */
		zassert_true(critical_var == 0, NULL);
		k_sem_give(&mut_sem);
	}
}

void sem_take_multiple_high_prio_long_helper(void *p1, void *p2, void *p3)
{
	int32_t ret_value;

	ret_value = k_sem_take(&high_prio_long_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

	ret_value = k_sem_take(&multiple_thread_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
		ret_value);

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
 */
void test_k_sem_define(void)
{
	uint32_t signal_count;

	/* get the semaphore count */
	signal_count = k_sem_count_get(&simple_sem);

	/* verify the semaphore count equals to initialized value */
	zassert_true(signal_count == SEM_INIT_VAL,
		     "semaphore initialized failed at compile time"
		     "- expected count %d, got %d",
		     SEM_INIT_VAL, signal_count);
}

/**
 * @brief Test synchronization of threads with semaphore
 * @see k_sem_init(), #K_SEM_DEFINE(x)
 */
void test_sem_thread2thread(void)
{
	int ret;

	/**TESTPOINT: test k_sem_init sema*/
	ret = k_sem_init(&sema, SEM_INIT_VAL, SEM_MAX_VAL);

	zassert_equal(ret, 0, NULL);

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
	int ret;

	/**TESTPOINT: test k_sem_init sema*/
	ret = k_sem_init(&sema, SEM_INIT_VAL, SEM_MAX_VAL);

	zassert_equal(ret, 0, NULL);
	tsema_thread_isr(&sema);

	/**TESTPOINT: test K_SEM_DEFINE sema*/
	tsema_thread_isr(&ksema);
}

/**
 * @brief Test semaphore initialization at running time
 * @details
 * - Initialize a semaphore with valid count and max limit.
 * - Initialize a semaphore with invalid max limit.
 * - Initialize a semaphore with invalid count.
 * @ingroup kernel_semaphore_tests
 */
void test_k_sem_init(void)
{
	int ret;

	/* initialize a semaphore with valid count and max limit */
	ret = k_sem_init(&sema, SEM_INIT_VAL, SEM_MAX_VAL);
	zassert_equal(ret, 0, "k_sem_init() failed");

	k_sem_reset(&sema);

	/* initialize a semaphore with invalid max limit */
	ret = k_sem_init(&sema, SEM_INIT_VAL, 0);
	zassert_equal(ret, -EINVAL, "k_sem_init() with invalid max limit");

	/* initialize a semaphore with invalid count */
	ret = k_sem_init(&sema, SEM_MAX_VAL + 1, SEM_MAX_VAL);
	zassert_equal(ret, -EINVAL, "k_sem_init with invalid count");

}


/**
 * @brief Test k_sem_reset() API
 * @see k_sem_reset()
 */
void test_sem_reset(void)
{
	int ret;

	ret = k_sem_init(&sema, SEM_INIT_VAL, SEM_MAX_VAL);
	zassert_equal(ret, 0, NULL);

	k_sem_give(&sema);
	k_sem_reset(&sema);
	zassert_false(k_sem_count_get(&sema), NULL);
	/**TESTPOINT: semaphore take return -EBUSY*/
	zassert_equal(k_sem_take(&sema, K_NO_WAIT), -EBUSY, NULL);
	/**TESTPOINT: semaphore take return -EAGAIN*/
	zassert_equal(k_sem_take(&sema, SEM_TIMEOUT), -EAGAIN, NULL);
	k_sem_give(&sema);
	zassert_false(k_sem_take(&sema, K_FOREVER), NULL);
}

/**
 * @brief Test k_sem_count_get() API
 * @see k_sem_count_get()
 */
void test_sem_count_get(void)
{
	int ret;

	ret = k_sem_init(&sema, SEM_INIT_VAL, SEM_MAX_VAL);
	zassert_equal(ret, 0, NULL);

	/**TESTPOINT: semaphore count get upon init*/
	zassert_equal(k_sem_count_get(&sema), SEM_INIT_VAL, NULL);
	k_sem_give(&sema);
	/**TESTPOINT: sem count get after give*/
	zassert_equal(k_sem_count_get(&sema), SEM_INIT_VAL + 1, NULL);
	k_sem_take(&sema, K_FOREVER);
	/**TESTPOINT: sem count get after take*/
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		zassert_equal(k_sem_count_get(&sema), SEM_INIT_VAL + i, NULL);
		k_sem_give(&sema);
	}
	/**TESTPOINT: semaphore give above limit*/
	k_sem_give(&sema);
	zassert_equal(k_sem_count_get(&sema), SEM_MAX_VAL, NULL);
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
	uint32_t signal_count;

	/*
	 * Signal the semaphore several times from an ISR.  After each signal,
	 * check the signal count.
	 */

	k_sem_reset(&simple_sem);

	for (int i = 0; i < 5; i++) {
		sem_give_from_isr(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i + 1),
			     "signal count missmatch - expected %d, got %d",
			     (i + 1), signal_count);
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
 */
void test_sem_give_from_thread(void)
{
	uint32_t signal_count;

	/*
	 * Signal the semaphore several times from a task.  After each signal,
	 * check the signal count.
	 */

	k_sem_reset(&simple_sem);

	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i + 1),
			     "signal count missmatch - expected %d, got %d",
			     (i + 1), signal_count);
	}

}

/**
 * @brief Test if k_sem_take() decreases semaphore count
 * @see k_sem_take()
 */
void test_sem_take_no_wait(void)
{
	uint32_t signal_count;
	int32_t ret_value;

	/*
	 * Test the semaphore without wait.  Check the signal count after each
	 * attempt (it should be decrementing by 1 each time).
	 */

	k_sem_reset(&simple_sem);
	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);
	}

	for (int i = 4; i >= 0; i--) {
		ret_value = k_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret_value == 0,
			     "unable to do k_sem_take which returned %d",
			     ret_value);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == i,
			     "signal count missmatch - expected %d, got %d",
			     i, signal_count);
	}

}

/**
 * @brief Test k_sem_take() when there is no semaphore to take
 * @see k_sem_take()
 */
void test_sem_take_no_wait_fails(void)
{
	uint32_t signal_count;
	int32_t ret_value;

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
		zassert_true(signal_count == 0U,
			     "signal count missmatch - expected 0, got %d",
			     signal_count);
	}

}

/**
 * @brief Test a semaphore take operation with an unavailable semaphore
 * @details
 * - Reset the semaphore's count to zero, let it unavailable.
 * - Take an unavailable semaphore and wait it until timeout.
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 */
void test_sem_take_timeout_fails(void)
{
	/*
	 * Test the semaphore with timeout without a k_sem_give.
	 */

	int32_t ret_value;
	uint32_t signal_count;

	k_sem_reset(&simple_sem);

	signal_count = k_sem_count_get(&simple_sem);
	zassert_true(signal_count == 0U, "k_sem_reset failed");

	/* take an unavailable semaphore and wait it until timeout */
	for (int i = 4; i >= 0; i--) {
		ret_value = k_sem_take(&simple_sem, SEM_TIMEOUT);
		zassert_true(ret_value == -EAGAIN,
				"k_sem_take succeeded when it's not possible");
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
 */
void test_sem_take_timeout(void)
{
	int32_t ret_value;
	uint32_t signal_count;

	/*
	 * Signal the semaphore upon which the other thread is waiting.
	 * The thread (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking up this task.
	 */

	/* create a new thread, it will give semaphore */
	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_give_task, &simple_sem, NULL, NULL,
			K_PRIO_PREEMPT(0), K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_sem_reset(&simple_sem);

	signal_count = k_sem_count_get(&simple_sem);
	zassert_true(signal_count == 0U, "k_sem_reset failed");

	/* Take semaphore and wait it given by other threads
	 * in specified timeout
	 */
	ret_value = k_sem_take(&simple_sem, SEM_TIMEOUT);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
			ret_value);
	k_thread_abort(&sem_tid_1);

}

/**
 * @brief Test the semaphore take operation with forever wait
 * @details
 * - Create a new thread, it will give semaphore.
 * - Reset the semaphore's count to zero.
 * - Take semaphore, wait it given by other thread forever until it's available.
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 */
void test_sem_take_timeout_forever(void)
{
	int32_t ret_value;
	uint32_t signal_count;

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

	signal_count = k_sem_count_get(&simple_sem);
	zassert_true(signal_count == 0U, "k_sem_reset failed");

	/* Take semaphore and wait it given by
	 * other threads forever until it's available
	 */
	ret_value = k_sem_take(&simple_sem, K_FOREVER);
	zassert_true(ret_value == 0, "k_sem_take failed with returned %d",
			ret_value);
	k_thread_abort(&sem_tid_1);

}

/**
 * @brief Test k_sem_take() with timeout in ISR context
 * @see k_sem_take()
 */
void test_sem_take_timeout_isr(void)
{
	int32_t ret_value;

	/*
	 * Signal the semaphore upon which the another thread is waiting.  The
	 * thread (which is at a lower priority) will cause simple_sem
	 * to be signalled, thus waking this task.
	 */

	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_take_timeout_isr_helper, NULL, NULL, NULL,
			K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	k_sem_reset(&simple_sem);

	ret_value = k_sem_take(&simple_sem, SEM_TIMEOUT);

	zassert_true(ret_value == 0, "k_sem_take failed");
}

/**
 * @brief Test semaphore take operation by multiple threads
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take()
 */
void test_sem_take_multiple(void)
{
	uint32_t signal_count;

	k_sem_reset(&multiple_thread_sem);
	signal_count = k_sem_count_get(&multiple_thread_sem);
	zassert_true(signal_count == 0U, "k_sem_reset failed");

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
	signal_count = k_sem_count_get(&high_prio_long_sem);
	zassert_true(signal_count == 1U,
			"High priority and long waiting thread "
			"don't get the sem");

	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 0U,
			"High priority thread shouldn't get the sem");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 0U,
		     "Medium priority threads shouldn't have executed");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0U,
		     "Low priority threads shouldn't have executed");

	/* enable the high prio thread sem_tid_4 to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check which threads completed */
	signal_count = k_sem_count_get(&high_prio_long_sem);
	zassert_true(signal_count == 1U, "High priority and long waiting thread"
			" executed again");

	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1U,
		     "Higher priority thread did not get the sem");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 0U,
		     "Medium priority thread shouldn't get the sem");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0U,
		     "Low priority thread shouldn't get the sem");

	/* enable the mid prio thread sem_tid_2 to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check which threads completed */
	signal_count = k_sem_count_get(&high_prio_long_sem);
	zassert_true(signal_count == 1U, "High priority and long waiting thread"
			" executed again");

	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1U,
		     "High priority thread executed again");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 1U,
		     "Medium priority thread did not get the sem");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 0U,
		     "Low priority thread did not get the sem");

	/* enable the low prio thread(thread_1) to run */
	k_sem_give(&multiple_thread_sem);
	k_sleep(K_MSEC(200));

	/* check the thread completed */
	signal_count = k_sem_count_get(&high_prio_long_sem);
	zassert_true(signal_count == 1U, "High priority and long waiting thread"
			" executed again");

	signal_count = k_sem_count_get(&high_prio_sem);
	zassert_true(signal_count == 1U, "High priority thread executed again");

	signal_count = k_sem_count_get(&mid_prio_sem);
	zassert_true(signal_count == 1U, "Mid priority thread executed again");

	signal_count = k_sem_count_get(&low_prio_sem);
	zassert_true(signal_count == 1U,
		     "Low priority thread did not get the sem");
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
 */
void test_k_sem_correct_count_limit(void)
{
	uint32_t signal_count;
	int32_t ret;

	/* reset an initialized semaphore's count to zero */
	k_sem_reset(&simple_sem);
	signal_count = k_sem_count_get(&simple_sem);
	zassert_true(signal_count == 0U, "k_sem_reset failed");

	/* Give the semaphore by a thread and verify the semaphore's
	 * count is as expected
	 */
	for (int i = 1; i <= SEM_MAX_VAL; i++) {
		k_sem_give(&simple_sem);
		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == i,
			     "semaphore count mismatch - expected %d, got %d",
			     i, signal_count);
	}

	/* Verify the max count a semaphore can reach
	 * continue to run k_sem_give,
	 * the count of simple_sem will not increase anymore
	 */
	for (int i = 0; i < 5; i++) {
		k_sem_give(&simple_sem);
		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == SEM_MAX_VAL,
			     "semaphore count mismatch - expected %d, got %d",
			     SEM_MAX_VAL, signal_count);
	}

	/* Take the semaphore by a thread and verify the semaphore's
	 * count is as expected
	 */
	for (int i = SEM_MAX_VAL - 1; i >= 0; i--) {
		ret = k_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret == 0, "k_sem_take failed with returned %d",
			     ret);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == i,
			     "semaphore count mismatch - expected %d, got %d",
			     i, signal_count);
	}

	/* Verify the max times a semaphore can be taken
	 * continue to run k_sem_take, simple_sem can not be taken and
	 * it's count will be zero
	 */
	for (int i = 0; i < 5; i++) {
		ret = k_sem_take(&simple_sem, K_NO_WAIT);
		zassert_true(ret == -EBUSY,
			     "k_sem_take failed with returned %d", ret);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == 0U,
			     "semaphore count mismatch - expected %d, got %d",
			     0, signal_count);
	}
}

/**
 * @brief Test semaphore give and take and its count from ISR
 * @see k_sem_give()
 */
void test_sem_give_take_from_isr(void)
{
	uint32_t signal_count;

	k_sem_reset(&simple_sem);
	signal_count = k_sem_count_get(&simple_sem);
	zassert_true(signal_count == 0U, "k_sem_reset failed");

	/* give semaphore from an isr and do a check for the count */
	for (int i = 0; i < SEM_MAX_VAL; i++) {
		sem_give_from_isr(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == i + 1,
			     "signal count missmatch - expected %d, got %d",
			     i + 1, signal_count);
	}

	/* take semaphore from an isr and do a check for the count */
	for (int i = SEM_MAX_VAL; i > 0; i--) {
		sem_take_from_isr(&simple_sem);

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == (i - 1),
			     "signal count missmatch - expected %d, got %d",
			     (i - 1), signal_count);
	}
}

/**
 * @}
 */

void sem_multiple_threads_wait_helper(void *p1, void *p2, void *p3)
{
	/* get blocked until the test thread gives the semaphore */
	k_sem_take(&multiple_thread_sem, K_FOREVER);

	/* inform the test thread that this thread has got multiple_thread_sem*/
	k_sem_give(&simple_sem);
}


/**
 * @brief Test multiple semaphore take and give with wait
 * @ingroup kernel_semaphore_tests
 * @see k_sem_take(), k_sem_give()
 */
void test_sem_multiple_threads_wait(void)
{
	uint32_t signal_count;
	int32_t ret_value;
	uint32_t repeat_count = 0U;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);

	do {
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
			ret_value = k_sem_take(&simple_sem, K_FOREVER);
			zassert_true(ret_value == 0,
				     "Some of the threads did not get multiple_thread_sem"
				     );
		}

		signal_count = k_sem_count_get(&simple_sem);
		zassert_true(signal_count == 0U,
			     "signal count missmatch - expected 0, got %d",
			     signal_count);

		signal_count = k_sem_count_get(&multiple_thread_sem);
		zassert_true(signal_count == 0U,
			     "signal count missmatch - expected 0, got %d",
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
	int32_t ret_value;
	uint32_t start_ticks, end_ticks;

	k_sem_reset(&simple_sem);

	/* with timeout of 1 sec */
	start_ticks = k_uptime_get();

	ret_value = k_sem_take(&simple_sem, K_SECONDS(1));

	end_ticks = k_uptime_get();

	zassert_true(ret_value == -EAGAIN, "k_sem_take failed");

	zassert_true((end_ticks - start_ticks >= SEC2MS(1)),
		     "time missmatch - expected %d, got %d",
		     SEC2MS(1), end_ticks - start_ticks);

	/* with 0 as the timeout */
	start_ticks = k_uptime_get();

	ret_value = k_sem_take(&simple_sem, K_NO_WAIT);

	end_ticks = k_uptime_get();

	zassert_true(ret_value == -EBUSY, "k_sem_take failed");

	zassert_true((end_ticks - start_ticks < 1),
		     "time missmatch - expected %d, got %d",
		     1, end_ticks - start_ticks);

}

void sem_measure_timeout_from_thread_helper(void *p1, void *p2, void *p3)
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
	int32_t ret_value;
	uint32_t start_ticks, end_ticks;

	k_sem_reset(&simple_sem);
	k_sem_reset(&multiple_thread_sem);

	/* give a semaphore from a thread and calculate the time taken */
	k_thread_create(&sem_tid_1, stack_1, STACK_SIZE,
			sem_measure_timeout_from_thread_helper,
			NULL, NULL, NULL,
			K_PRIO_PREEMPT(3), 0, K_NO_WAIT);


	/* first sync the 2 threads */
	k_sem_take(&simple_sem, K_FOREVER);

	/* with timeout of 1 sec */
	start_ticks = k_uptime_get();

	ret_value = k_sem_take(&multiple_thread_sem, K_SECONDS(1));

	end_ticks = k_uptime_get();

	zassert_true(ret_value == 0, "k_sem_take failed");

	zassert_true((end_ticks - start_ticks <= SEC2MS(1)),
		     "time missmatch - expected less than%d ,got %d",
		     SEC2MS(1), end_ticks - start_ticks);

}

void sem_multiple_take_and_timeouts_helper(void *p1, void *p2, void *p3)
{
	int timeout = POINTER_TO_INT(p1);
	uint32_t start_ticks, end_ticks;
	size_t bytes_written;

	start_ticks = k_uptime_get();

	k_sem_take(&simple_sem, K_MSEC(timeout));

	end_ticks = k_uptime_get();

	zassert_true((end_ticks - start_ticks >= timeout),
		     "time missmatch - expected less than %d ,got %d",
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
	uint32_t timeout;
	size_t bytes_read;

	k_sem_reset(&simple_sem);

	/* Multiple threads timeout and the sequence in which it times out
	 * is pushed into a pipe and checked later on.
	 */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_create(&multiple_tid[i],
				multiple_stack[i], STACK_SIZE,
				sem_multiple_take_and_timeouts_helper,
				INT_TO_POINTER(SEC2MS(i + 1)), NULL, NULL,
				K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	}

	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_pipe_get(&timeout_info_pipe, &timeout, sizeof(int),
			   &bytes_read, sizeof(int), K_FOREVER);
		zassert_true(timeout == SEC2MS(i + 1),
			     "timeout did not occur properly");
	}

	/* cleanup */
	for (int i = 0; i < TOTAL_THREADS_WAITING; i++) {
		k_thread_abort(&multiple_tid[i]);
	}

}

void sem_multi_take_timeout_diff_sem_helper(void *p1, void *p2, void *p3)
{
	int timeout = POINTER_TO_INT(p1);
	struct k_sem *sema = p2;
	uint32_t start_ticks, end_ticks;
	int32_t ret_value;
	size_t bytes_written;
	struct timeout_info info = {
		.timeout = timeout,
		.sema = sema
	};

	start_ticks = k_uptime_get();

	ret_value = k_sem_take(sema, K_MSEC(timeout));

	end_ticks = k_uptime_get();

	zassert_true((end_ticks - start_ticks >= timeout),
		     "time missmatch - expected less than %d, got %d",
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
		{ SEC2MS(2), &simple_sem },
		{ SEC2MS(1), &multiple_thread_sem },
		{ SEC2MS(3), &simple_sem },
		{ SEC2MS(5), &multiple_thread_sem },
		{ SEC2MS(4), &simple_sem },
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

	k_sem_init(&mut_sem, 0, 1);

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
}

/* ztest main entry*/
void test_main(void)
{
	k_thread_access_grant(k_current_get(),
			      &simple_sem, &multiple_thread_sem, &low_prio_sem,
				  &mid_prio_sem, &high_prio_sem, &ksema, &sema,
				  &high_prio_long_sem, &stack_1, &stack_2,
				  &stack_3, &stack_4, &timeout_info_pipe,
				  &sem_tid_1, &sem_tid_2, &sem_tid_3, &sem_tid_4,
				  &tstack, &tdata, &mut_sem);

	ztest_test_suite(test_semaphore,
			 ztest_user_unit_test(test_k_sem_define),
			 ztest_user_unit_test(test_k_sem_init),
			 ztest_user_unit_test(test_sem_thread2thread),
			 ztest_unit_test(test_sem_thread2isr),
			 ztest_user_unit_test(test_sem_reset),
			 ztest_user_unit_test(test_sem_count_get),
			 ztest_unit_test(test_sem_give_from_isr),
			 ztest_user_unit_test(test_sem_give_from_thread),
			 ztest_user_unit_test(test_sem_take_no_wait),
			 ztest_user_unit_test(test_sem_take_no_wait_fails),
			 ztest_1cpu_user_unit_test(test_sem_take_timeout_fails),
			 ztest_user_unit_test(test_sem_take_timeout),
			 ztest_1cpu_user_unit_test(test_sem_take_timeout_forever),
			 ztest_unit_test(test_sem_take_timeout_isr),
			 ztest_1cpu_user_unit_test(test_sem_take_multiple),
			 ztest_unit_test(test_sem_give_take_from_isr),
			 ztest_user_unit_test(test_k_sem_correct_count_limit),
			 ztest_unit_test(test_sem_multiple_threads_wait),
			 ztest_unit_test(test_sem_measure_timeouts),
			 ztest_unit_test(test_sem_measure_timeout_from_thread),
			 ztest_1cpu_unit_test(test_sem_multiple_take_and_timeouts),
			 ztest_unit_test(test_sem_multi_take_timeout_diff_sem),
			 ztest_1cpu_unit_test(test_sem_queue_mutual_exclusion));
	ztest_run_test_suite(test_semaphore);
}
