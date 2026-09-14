/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

K_SEM_DEFINE(static_preem_start_sema, 0, 1);
K_SEM_DEFINE(preem_start_sema, 0, 1);
K_SEM_DEFINE(static_coop_start_sema, 0, 1);
K_SEM_DEFINE(coop_start_sema, 0, 1);

K_SEM_DEFINE(end_sema, 0, 1);
/*macro definition*/
#define INIT_COOP_PRIO -2
#define INIT_COOP_STACK_SIZE (500 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define INIT_COOP_P1			((void *)0xFFFF0000)
#define INIT_COOP_P2			((void *)0xCDEF)
#define INIT_COOP_P3_static		((void *)&static_coop_start_sema)
#define INIT_COOP_P3			((void *)&coop_start_sema)
#define INIT_COOP_OPTION (K_USER | K_INHERIT_PERMS)
#define INIT_COOP_DELAY 2000
#define INIT_PREEMPT_PRIO 1
#define INIT_PREEMPT_STACK_SIZE (499 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define INIT_PREEMPT_P1			((void *)5)
#define INIT_PREEMPT_P2			((void *)6)
#define INIT_PREEMPT_P3_static	((void *)&static_preem_start_sema)
#define INIT_PREEMPT_P3			((void *)&preem_start_sema)
#define INIT_PREEMPT_OPTION (K_USER | K_INHERIT_PERMS)
#define INIT_PREEMPT_DELAY 0

static void thread_entry(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(T_KDEFINE_COOP_THREAD, INIT_COOP_STACK_SIZE,
		thread_entry, INIT_COOP_P1, INIT_COOP_P2, INIT_COOP_P3_static,
		INIT_COOP_PRIO, INIT_COOP_OPTION, INIT_COOP_DELAY);

K_THREAD_DEFINE(T_KDEFINE_PREEMPT_THREAD, INIT_PREEMPT_STACK_SIZE,
		thread_entry, INIT_PREEMPT_P1, INIT_PREEMPT_P2, INIT_PREEMPT_P3_static,
		INIT_PREEMPT_PRIO, INIT_PREEMPT_OPTION, INIT_PREEMPT_DELAY);

K_THREAD_ACCESS_GRANT(T_KDEFINE_COOP_THREAD, &static_preem_start_sema,
	 &preem_start_sema, &static_coop_start_sema, &coop_start_sema, &end_sema);
K_THREAD_ACCESS_GRANT(T_KDEFINE_PREEMPT_THREAD, &static_preem_start_sema,
	 &preem_start_sema, &static_coop_start_sema, &coop_start_sema, &end_sema);

/*local variables*/
static K_THREAD_STACK_DEFINE(stack_coop, INIT_COOP_STACK_SIZE);
static K_THREAD_STACK_DEFINE(stack_preempt, INIT_PREEMPT_STACK_SIZE);
static struct k_thread thread_coop;
static struct k_thread thread_preempt;
static ZTEST_BMEM uint64_t t_create;
static ZTEST_BMEM struct thread_data {
	int init_prio;
	int32_t init_delay;
	void *init_p1;
	void *init_p2;
	void *init_p3;
} expected;

/*entry routines*/
static void thread_entry(void *p1, void *p2, void *p3)
{
	/* Sample the moment this thread was first scheduled before
	 * synchronising, so the start delay is still measured from creation to
	 * first execution rather than from the release of the semaphore.
	 */
	uint64_t t_start = k_uptime_get();

	/* Wait to be released by the test case that owns this thread before
	 * looking at any of the shared expectations. A statically defined
	 * thread becomes ready at boot, or once its own start delay expires,
	 * and is then scheduled during whichever test case happens to block
	 * first, which need not be its own, so reading them earlier would race
	 * with that case.
	 */
	k_sem_take(p3, K_FOREVER);

	if (t_create) {
		uint64_t t_delay = t_start - t_create;
		/**TESTPOINT: check delay start*/
		zassert_true(t_delay >= expected.init_delay,
			     "k_thread_create delay start failed");
	}

	k_tid_t tid = k_current_get();
	/**TESTPOINT: check priority and params*/
	zassert_equal(k_thread_priority_get(tid), expected.init_prio);
	zassert_equal(p1, expected.init_p1);
	zassert_equal(p2, expected.init_p2);
	zassert_equal(p3, expected.init_p3);
	/*option, stack size, not checked, no public API to get these values*/

	k_sem_give(&end_sema);
}

/**
 * @addtogroup kernel_thread_tests
 * @{
 */

/**
 * @brief Verify that K_THREAD_DEFINE() initializes a preemptible thread with
 *        the values it was given.
 *
 * @details
 * A statically defined thread has to reach its entry point with exactly the
 * priority and the three parameters named in its definition, the same way a
 * thread created at run time does. The thread itself performs the checks, so
 * what is validated is the state the kernel actually handed it rather than
 * what the definition asked for. The start delay is not checked here because
 * a statically defined thread has no observable creation timestamp.
 *
 * Test steps:
 * - Record the priority and parameters the thread was defined with.
 * - Release the statically defined preemptible thread from its start
 *   semaphore.
 * - In the thread, compare its own priority and its p1, p2 and p3 against the
 *   recorded values, then signal completion.
 * - Wait for the completion semaphore.
 *
 * Expected result:
 * - The thread runs at the defined priority and receives the defined
 *   parameters.
 *
 * @see K_THREAD_DEFINE()
 * @see k_thread_priority_get()
 */
ZTEST_USER(thread_init, test_thread_init_kdefine_preempt)
{
	/*static thread created time unknown, skip it*/
	t_create = 0U;
	expected.init_p1 = INIT_PREEMPT_P1;
	expected.init_p2 = INIT_PREEMPT_P2;
	expected.init_p3 = INIT_PREEMPT_P3_static;
	expected.init_prio = INIT_PREEMPT_PRIO;
	expected.init_delay = INIT_PREEMPT_DELAY;
	k_sem_reset(&end_sema);

	/*signal thread to start*/
	k_sem_give(&static_preem_start_sema);
	/*wait for thread to exit*/
	k_sem_take(&end_sema, K_FOREVER);
}

/**
 * @brief Verify that K_THREAD_DEFINE() initializes a cooperative thread with
 *        the values it was given.
 *
 * @details
 * The cooperative counterpart of the preemptible case: a negative priority
 * must survive static definition just as a positive one does, so the same
 * checks are repeated against a thread defined at a cooperative priority.
 *
 * Test steps:
 * - Record the priority and parameters the thread was defined with.
 * - Release the statically defined cooperative thread from its start
 *   semaphore.
 * - In the thread, compare its own priority and its p1, p2 and p3 against the
 *   recorded values, then signal completion.
 * - Wait for the completion semaphore.
 *
 * Expected result:
 * - The thread runs at the defined cooperative priority and receives the
 *   defined parameters.
 *
 * @see K_THREAD_DEFINE()
 * @see k_thread_priority_get()
 */
ZTEST_USER(thread_init, test_thread_init_kdefine_coop)
{
	/*static thread creation time unknown, skip it*/
	t_create = 0U;
	expected.init_p1 = INIT_COOP_P1;
	expected.init_p2 = INIT_COOP_P2;
	expected.init_p3 = INIT_COOP_P3_static;
	expected.init_prio = INIT_COOP_PRIO;
	expected.init_delay = INIT_COOP_DELAY;
	k_sem_reset(&end_sema);

	/*signal thread to start*/
	k_sem_give(&static_coop_start_sema);
	/*wait for thread to exit*/
	k_sem_take(&end_sema, K_FOREVER);
}

/**
 * @brief Verify that k_thread_create() honours priority, parameters and start
 *        delay for a preemptible thread.
 *
 * @details
 * Creating a thread at run time must apply everything the call specifies: the
 * priority, the three parameters and the delay before the thread is made
 * ready. This case creates the thread with a delay of zero, so it covers
 * immediate start; the cooperative case below carries the non-zero delay.
 *
 * Test steps:
 * - Record the creation timestamp, then create a preemptible thread with a
 *   zero start delay.
 * - Release it from its start semaphore.
 * - In the thread, check that the time from creation to its first execution
 *   is at least the requested delay, then compare its priority and its p1, p2
 *   and p3 against the requested values and signal completion.
 * - Wait for the completion semaphore.
 *
 * Expected result:
 * - Creation succeeds and the thread runs at the requested priority with the
 *   requested parameters.
 *
 * @see k_thread_create()
 * @see k_thread_priority_get()
 */
ZTEST_USER(thread_init, test_thread_init_create_preempt)
{
	/*record time stamp before thread creation*/
	t_create = k_uptime_get();

	/*create preempt thread*/
	k_tid_t pthread = k_thread_create(&thread_preempt, stack_preempt,
					  INIT_PREEMPT_STACK_SIZE, thread_entry, INIT_PREEMPT_P1,
					  INIT_PREEMPT_P2, INIT_PREEMPT_P3, INIT_PREEMPT_PRIO,
					  INIT_PREEMPT_OPTION,
					  K_MSEC(INIT_PREEMPT_DELAY));

	zassert_not_null(pthread, "thread creation failed");

	expected.init_p1 = INIT_PREEMPT_P1;
	expected.init_p2 = INIT_PREEMPT_P2;
	expected.init_p3 = INIT_PREEMPT_P3;
	expected.init_prio = INIT_PREEMPT_PRIO;
	expected.init_delay = INIT_PREEMPT_DELAY;
	k_sem_reset(&end_sema);

	/*signal thread to start*/
	k_sem_give(&preem_start_sema);
	/*wait for thread to exit*/
	k_sem_take(&end_sema, K_FOREVER);
}

/**
 * @brief Verify that k_thread_create() honours priority, parameters and start
 *        delay for a cooperative thread.
 *
 * @details
 * The cooperative counterpart of the run-time creation case, covering a
 * negative priority and the K_USER option. It is also the case that exercises
 * the start delay: the thread is created with a two second delay and knows
 * when it was created, so it can confirm it was not made ready early. Skipped
 * when userspace is not enabled, as the thread is created with the user option
 * that configuration does not provide.
 *
 * Test steps:
 * - Record the creation timestamp, then create a cooperative thread with a
 *   two second start delay.
 * - Release it from its start semaphore.
 * - In the thread, check that the time from creation to its first execution
 *   is at least the requested delay, then compare its priority and its p1, p2
 *   and p3 against the requested values and signal completion.
 * - Wait for the completion semaphore.
 *
 * Expected result:
 * - Creation succeeds, the thread starts no earlier than the requested delay,
 *   and it runs at the requested cooperative priority with the requested
 *   parameters.
 *
 * @see k_thread_create()
 * @see k_thread_priority_get()
 */
ZTEST(thread_init, test_thread_init_create_coop)
{
	if (!(IS_ENABLED(CONFIG_USERSPACE))) {
		ztest_test_skip();
	}

	/*record time stamp before thread creation*/
	t_create = k_uptime_get();

	/*create coop thread*/
	k_tid_t pthread = k_thread_create(&thread_coop, stack_coop,
			  INIT_COOP_STACK_SIZE, thread_entry, INIT_COOP_P1,
			  INIT_COOP_P2, INIT_COOP_P3, INIT_COOP_PRIO,
			  INIT_COOP_OPTION, K_MSEC(INIT_COOP_DELAY));

	zassert_not_null(pthread, "thread spawn failed");

	expected.init_p1 = INIT_COOP_P1;
	expected.init_p2 = INIT_COOP_P2;
	expected.init_p3 = INIT_COOP_P3;
	expected.init_prio = INIT_COOP_PRIO;
	expected.init_delay = INIT_COOP_DELAY;
	k_sem_reset(&end_sema);

	/*signal thread to start*/
	k_sem_give(&coop_start_sema);
	/*wait for thread to exit*/
	k_sem_take(&end_sema, K_FOREVER);
}


/**
 * @}
 */

/*test case main entry*/
void *thread_init_setup(void)
{
	k_thread_access_grant(k_current_get(), &thread_preempt, &stack_preempt,
				 &static_preem_start_sema, &preem_start_sema,
				 &static_coop_start_sema, &coop_start_sema, &end_sema);
#ifdef CONFIG_USERSPACE
	k_mem_domain_add_thread(&k_mem_domain_default, T_KDEFINE_COOP_THREAD);
	k_mem_domain_add_thread(&k_mem_domain_default, T_KDEFINE_PREEMPT_THREAD);
#endif

	return NULL;
}

ZTEST_SUITE(thread_init, NULL, thread_init_setup, NULL, NULL, NULL);
