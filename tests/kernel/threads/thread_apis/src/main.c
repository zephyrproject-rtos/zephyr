/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/**
 * @brief Thread Tests
 * @defgroup kernel_thread_tests Threads
 * @ingroup all_tests
 * @{
 * @}
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <string.h>

/* internal kernel APIs */
#include <ksched.h>
#include <kthread.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

struct k_thread tdata;
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
size_t tstack_size = K_THREAD_STACK_SIZEOF(tstack);

/*local variables*/
static K_THREAD_STACK_DEFINE(tstack_custom, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack_name, STACK_SIZE);
static struct k_thread tdata_custom;
static struct k_thread tdata_name;

static int main_prio;
static ZTEST_DMEM int tp = 10;

/**
 * @ingroup kernel_thread_tests
 * @brief Verify main thread
 */
ZTEST(threads_lifecycle, test_systhreads_main)
{
	zassert_true(main_prio == CONFIG_MAIN_THREAD_PRIORITY, "%d", CONFIG_MAIN_THREAD_PRIORITY);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Verify idle thread
 */
ZTEST(threads_lifecycle, test_systhreads_idle)
{
	k_msleep(100);
	/** TESTPOINT: check working thread priority should */
	zassert_true(k_thread_priority_get(k_current_get()) <
		     K_IDLE_PRIO, NULL);
}

static void customdata_entry(void *p1, void *p2, void *p3)
{
	long data = 1U;

	zassert_is_null(k_thread_custom_data_get(), NULL);
	while (1) {
		k_thread_custom_data_set((void *)data);
		/* relinquish cpu for a while */
		k_msleep(50);
		/** TESTPOINT: custom data comparison */
		zassert_equal(data, (long)k_thread_custom_data_get());
		data++;
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test thread custom data get/set from coop thread
 *
 * @see k_thread_custom_data_get(), k_thread_custom_data_set()
 */
ZTEST(threads_lifecycle_1cpu, test_customdata_get_set_coop)
{
	k_tid_t tid = k_thread_create(&tdata_custom, tstack_custom, STACK_SIZE,
				      customdata_entry, NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, K_NO_WAIT);

	k_msleep(500);

	/* cleanup environment */
	k_thread_abort(tid);
}

static void thread_name_entry(void *p1, void *p2, void *p3)
{
	/* Do nothing and exit */
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test thread name get/set from supervisor thread
 * @see k_thread_name_get(), k_thread_name_copy(), k_thread_name_set()
 */
ZTEST(threads_lifecycle, test_thread_name_get_set)
{
	int ret;
	const char *thread_name;
	char thread_buf[CONFIG_THREAD_MAX_NAME_LEN];

	/* Set and get current thread's name */
	ret = k_thread_name_set(NULL, "parent_thread");
	zassert_equal(ret, 0, "k_thread_name_set() failed");
	thread_name = k_thread_name_get(k_current_get());
	zassert_true(thread_name != NULL, "thread name was null");
	ret = strcmp(thread_name, "parent_thread");
	zassert_equal(ret, 0, "parent thread name does not match");

	/* Set and get child thread's name */
	k_tid_t tid = k_thread_create(&tdata_name, tstack_name, STACK_SIZE,
				      thread_name_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	ret = k_thread_name_set(tid, "customdata");
	zassert_equal(ret, 0, "k_thread_name_set() failed");
	ret = k_thread_name_copy(tid, thread_buf, sizeof(thread_buf));
	zassert_equal(ret, 0, "couldn't get copied thread name");
	ret = strcmp(thread_buf, "customdata");
	zassert_equal(ret, 0, "child thread name does not match");

	/* cleanup environment */
	k_thread_abort(tid);
}

#ifdef CONFIG_USERSPACE
static char unreadable_string[64];
static char not_my_buffer[CONFIG_THREAD_MAX_NAME_LEN];
struct k_sem sem;
#endif /* CONFIG_USERSPACE */

/**
 * @ingroup kernel_thread_tests
 * @brief Test thread name get/set from user thread
 * @see k_thread_name_copy(), k_thread_name_set()
 */
ZTEST_USER(threads_lifecycle, test_thread_name_user_get_set)
{
#ifdef CONFIG_USERSPACE
	int ret;
	char thread_name[CONFIG_THREAD_MAX_NAME_LEN];
	char too_small[2];

	/* Some memory-related error cases for k_thread_name_set() */
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Non-Secure images cannot normally access memory outside the image
	 * flash and ram.
	 */
	ret = k_thread_name_set(NULL, (const char *)CONFIG_THREAD_API_UNMAPPED_ADDRESS);
	zassert_equal(ret, -EFAULT, "accepted nonsense string (%d)", ret);
#endif
	ret = k_thread_name_set(NULL, unreadable_string);
	zassert_equal(ret, -EFAULT, "accepted unreadable string");
	ret = k_thread_name_set((struct k_thread *)&sem, "some name");
	zassert_equal(ret, -EINVAL, "accepted non-thread object");
	ret = k_thread_name_set(&z_main_thread, "some name");
	zassert_equal(ret, -EINVAL, "no permission on thread object");

	/* Set and get current thread's name */
	ret = k_thread_name_set(NULL, "parent_thread");
	zassert_equal(ret, 0, "k_thread_name_set() failed");
	ret = k_thread_name_copy(k_current_get(), thread_name,
				     sizeof(thread_name));
	zassert_equal(ret, 0, "k_thread_name_copy() failed");
	ret = strcmp(thread_name, "parent_thread");
	zassert_equal(ret, 0, "parent thread name does not match");

	/* memory-related cases for k_thread_name_get() */
	ret = k_thread_name_copy(k_current_get(), too_small,
				     sizeof(too_small));
	zassert_equal(ret, -ENOSPC, "wrote to too-small buffer");
	ret = k_thread_name_copy(k_current_get(), not_my_buffer,
				     sizeof(not_my_buffer));
	zassert_equal(ret, -EFAULT, "wrote to buffer without permission");
	ret = k_thread_name_copy((struct k_thread *)&sem, thread_name,
				     sizeof(thread_name));
	zassert_equal(ret, -EINVAL, "not a thread object");
	ret = k_thread_name_copy(&z_main_thread, thread_name,
				     sizeof(thread_name));
	zassert_equal(ret, 0, "couldn't get main thread name: %s (%d)", thread_name, ret);
	LOG_DBG("Main thread name is '%s'", thread_name);

	/* Set and get child thread's name */
	k_tid_t tid = k_thread_create(&tdata_name, tstack_name, STACK_SIZE,
				      thread_name_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(1), K_USER, K_NO_WAIT);
	ret = k_thread_name_set(tid, "customdata");
	zassert_equal(ret, 0, "k_thread_name_set() failed");
	ret = k_thread_name_copy(tid, thread_name, sizeof(thread_name));
	zassert_equal(ret, 0, "couldn't get copied thread name");
	ret = strcmp(thread_name, "customdata");
	zassert_equal(ret, 0, "child thread name does not match");

	/* cleanup environment */
	k_thread_abort(tid);
#else
	ztest_test_skip();
#endif /* CONFIG_USERSPACE */
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test thread custom data get/set from preempt thread
 * @see k_thread_custom_data_get(), k_thread_custom_data_set()
 */
ZTEST_USER(threads_lifecycle_1cpu, test_customdata_get_set_preempt)
{
	/** TESTPOINT: custom data of preempt thread */
	k_tid_t tid = k_thread_create(&tdata_custom, tstack_custom, STACK_SIZE,
				      customdata_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0), K_USER, K_NO_WAIT);

	k_msleep(500);

	/* cleanup environment */
	k_thread_abort(tid);
}

static void umode_entry(void *thread_id, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!z_is_thread_essential(_current) &&
	    (k_current_get() == (k_tid_t)thread_id)) {
		ztest_test_pass();
	} else {
		zassert_unreachable("User thread is essential or thread"
				    " structure is corrupted\n");
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test k_thread_user_mode_enter to cover when userspace
 * is not supported/enabled
 * @see k_thread_user_mode_enter()
 */
static void enter_user_mode_entry(void *p1, void *p2, void *p3)
{
	z_thread_essential_set(_current);

	zassert_true(z_is_thread_essential(_current), "Thread isn't set"
		     " as essential\n");

	k_thread_user_mode_enter(umode_entry,
				 k_current_get(), NULL, NULL);
}

ZTEST_USER(threads_lifecycle, test_user_mode)
{
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			      enter_user_mode_entry, NULL, NULL,
			      NULL, main_prio, K_INHERIT_PERMS, K_NO_WAIT);
	k_msleep(100);
	k_thread_abort(tid);
}

struct k_thread join_thread;
K_THREAD_STACK_DEFINE(join_stack, STACK_SIZE);

struct k_thread control_thread;
K_THREAD_STACK_DEFINE(control_stack, STACK_SIZE);

enum control_method {
	TIMEOUT,
	NO_WAIT,
	SELF_ABORT,
	OTHER_ABORT,
	OTHER_ABORT_TIMEOUT,
	ALREADY_EXIT,
	ISR_ALREADY_EXIT,
	ISR_RUNNING
};

static void join_entry(void *p1, void *p2, void *p3)
{
	enum control_method m = (enum control_method)(intptr_t)p1;

	switch (m) {
	case TIMEOUT:
	case NO_WAIT:
	case OTHER_ABORT:
	case OTHER_ABORT_TIMEOUT:
	case ISR_RUNNING:
		LOG_DBG("join_thread: sleeping forever");
		k_sleep(K_FOREVER);
		break;
	case SELF_ABORT:
	case ALREADY_EXIT:
	case ISR_ALREADY_EXIT:
		LOG_DBG("join_thread: self-exiting");
		return;
	}
}

static void control_entry(void *p1, void *p2, void *p3)
{
	LOG_DBG("control_thread: killing join thread");
	k_thread_abort(&join_thread);
}

static void do_join_from_isr(const void *arg)
{
	int *ret = (int *)arg;

	zassert_true(k_is_in_isr());
	LOG_DBG("isr: joining join_thread");
	*ret = k_thread_join(&join_thread, K_NO_WAIT);
	LOG_DBG("isr: k_thread_join() returned with %d", *ret);
}

#define JOIN_TIMEOUT_MS	100

static int join_scenario_interval(enum control_method m, int64_t *interval)
{
	k_timeout_t timeout = K_FOREVER;
	int ret = 0;

	LOG_DBG("ztest_thread: method %d, create join_thread", m);
	k_thread_create(&join_thread, join_stack, STACK_SIZE, join_entry,
			(void *)m, NULL, NULL, K_PRIO_PREEMPT(1),
			K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	switch (m) {
	case ALREADY_EXIT:
	case ISR_ALREADY_EXIT:
		/* Let join_thread run first */
		k_msleep(50);
		break;
	case OTHER_ABORT_TIMEOUT:
		timeout = K_MSEC(JOIN_TIMEOUT_MS);
		__fallthrough;
	case OTHER_ABORT:
		LOG_DBG("ztest_thread: create control_thread");
		k_thread_create(&control_thread, control_stack, STACK_SIZE,
				control_entry, NULL, NULL, NULL,
				K_PRIO_PREEMPT(2),
				K_USER | K_INHERIT_PERMS, K_NO_WAIT);
		break;
	case TIMEOUT:
		timeout = K_MSEC(50);
		break;
	case NO_WAIT:
		timeout = K_NO_WAIT;
		break;
	default:
		break;
	}

	if (m == ISR_ALREADY_EXIT || m == ISR_RUNNING) {
		irq_offload(do_join_from_isr, (const void *)&ret);
	} else {
		LOG_DBG("ztest_thread: joining join_thread");

		if (interval != NULL) {
			*interval = k_uptime_get();
		}

		ret = k_thread_join(&join_thread, timeout);

		if (interval != NULL) {
			*interval = k_uptime_get() - *interval;
		}

		LOG_DBG("ztest_thread: k_thread_join() returned with %d", ret);
	}

	if (ret != 0) {
		k_thread_abort(&join_thread);
	}
	if (m == OTHER_ABORT || m == OTHER_ABORT_TIMEOUT) {
		k_thread_join(&control_thread, K_FOREVER);
	}

	return ret;
}

static inline int join_scenario(enum control_method m)
{
	return join_scenario_interval(m, NULL);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test thread join
 *
 */
ZTEST_USER(threads_lifecycle, test_thread_join)
{
	int64_t interval;

#ifdef CONFIG_USERSPACE
	/* scenario: thread never started */
	zassert_equal(k_thread_join(&join_thread, K_FOREVER), 0,
		      "failed case thread never started");
#endif
	zassert_equal(join_scenario(TIMEOUT), -EAGAIN, "failed timeout case");
	zassert_equal(join_scenario(NO_WAIT), -EBUSY, "failed no-wait case");
	zassert_equal(join_scenario(SELF_ABORT), 0, "failed self-abort case");
	zassert_equal(join_scenario(OTHER_ABORT), 0, "failed other-abort case");

	zassert_equal(join_scenario_interval(OTHER_ABORT_TIMEOUT, &interval),
		      0, "failed other-abort case with timeout");
	zassert_true(interval < JOIN_TIMEOUT_MS, "join took too long (%lld ms)",
		     interval);
	zassert_equal(join_scenario(ALREADY_EXIT), 0,
		      "failed already exit case");

}

/**
 * @ingroup kernel_thread_tests
 * @brief Test thread join from ISR
 *
 * @see k_thread_join()
 * @see k_thread_abort()
 */
ZTEST(threads_lifecycle, test_thread_join_isr)
{
	zassert_equal(join_scenario(ISR_RUNNING), -EBUSY, "failed isr running");
	zassert_equal(join_scenario(ISR_ALREADY_EXIT), 0, "failed isr exited");
}

struct k_thread deadlock1_thread;
K_THREAD_STACK_DEFINE(deadlock1_stack, STACK_SIZE);

struct k_thread deadlock2_thread;
K_THREAD_STACK_DEFINE(deadlock2_stack, STACK_SIZE);

static void deadlock1_entry(void *p1, void *p2, void *p3)
{
	int ret;

	k_msleep(500);

	ret = k_thread_join(&deadlock2_thread, K_FOREVER);
	zassert_equal(ret, -EDEADLK, "failed mutual join case");
}

static void deadlock2_entry(void *p1, void *p2, void *p3)
{
	int ret;

	/* deadlock1_thread is active but currently sleeping */
	ret = k_thread_join(&deadlock1_thread, K_FOREVER);

	zassert_equal(ret, 0, "couldn't join deadlock2_thread");
}



/**
 * @brief Test case for thread join deadlock scenarios.
 *
 * This test verifies the behavior of the `k_thread_join` API in scenarios
 * that could lead to deadlocks. It includes the following checks:
 *
 * - Ensures that a thread cannot join itself, which would result in a
 *   self-deadlock. The API should return `-EDEADLK` in this case.
 * - Creates two threads (`deadlock1_thread` and `deadlock2_thread`) and
 *   verifies that they can be joined successfully without causing a deadlock.
 *
 * @ingroup kernel_thread_tests
 */
ZTEST_USER(threads_lifecycle, test_thread_join_deadlock)
{
	/* Deadlock scenarios */
	zassert_equal(k_thread_join(k_current_get(), K_FOREVER), -EDEADLK,
				    "failed self-deadlock case");

	k_thread_create(&deadlock1_thread, deadlock1_stack, STACK_SIZE,
			deadlock1_entry, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	k_thread_create(&deadlock2_thread, deadlock2_stack, STACK_SIZE,
			deadlock2_entry, NULL, NULL, NULL,
			K_PRIO_PREEMPT(1), K_USER | K_INHERIT_PERMS, K_NO_WAIT);

	zassert_equal(k_thread_join(&deadlock1_thread, K_FOREVER), 0,
		      "couldn't join deadlock1_thread");
	zassert_equal(k_thread_join(&deadlock2_thread, K_FOREVER), 0,
		      "couldn't join deadlock2_thread");
}

#define WAIT_TO_START_MS 100
/*
 * entry for a delayed thread, do nothing. After the thread is created,
 * just check how many ticks expires and how many ticks remain before
 * the thread start
 */
static void user_start_thread(void *p1, void *p2, void *p3)
{
	/* do nothing */
}
/**
 * @brief Test case for verifying thread timeout expiration and remaining time.
 *
 * @ingroup kernel_thread_tests
 */

ZTEST_USER(threads_lifecycle, test_thread_timeout_remaining_expires)
{
	k_ticks_t r, e, r1, ticks, expected_expires_ticks;

	ticks = k_ms_to_ticks_ceil32(WAIT_TO_START_MS);
	expected_expires_ticks = k_uptime_ticks() + ticks;

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      user_start_thread, k_current_get(), NULL,
				      NULL, 0, K_USER,
				      K_MSEC(WAIT_TO_START_MS));

	k_msleep(10);
	e = k_thread_timeout_expires_ticks(tid);
	LOG_DBG("thread_expires_ticks: %d, expect: %d", (int)e,
		(int)expected_expires_ticks);
	zassert_true(e >= expected_expires_ticks);

	k_msleep(10);
	r = k_thread_timeout_remaining_ticks(tid);
	zassert_true(r < ticks);
	r1 = r;

	k_msleep(10);
	r = k_thread_timeout_remaining_ticks(tid);
	zassert_true(r < r1);

	k_thread_abort(tid);
}

static void foreach_callback(const struct k_thread *thread, void *user_data)
{

	k_thread_runtime_stats_t stats;
	int ret;

	if (z_is_idle_thread_object((k_tid_t)thread)) {
		return;
	}

	/* Check NULL parameters */
	ret = k_thread_runtime_stats_get(NULL, &stats);
	zassert_true(ret == -EINVAL);
	ret = k_thread_runtime_stats_get((k_tid_t)thread, NULL);
	zassert_true(ret == -EINVAL);

	k_thread_runtime_stats_get((k_tid_t)thread, &stats);
	((k_thread_runtime_stats_t *)user_data)->execution_cycles +=
		stats.execution_cycles;
}

/**
 * @brief Test case for thread runtime statistics retrieval in Zephyr kernel
 *
 * This case accumulates every thread's execution_cycles first, then
 * get the total execution_cycles from a global
 * k_thread_runtime_stats_t to see that all time is reflected in the
 * total.
 *
 * @ingroup kernel_thread_tests
 * @see k_thread_runtime_stats_get()
 */
ZTEST(threads_lifecycle, test_thread_runtime_stats_get)
{
	k_thread_runtime_stats_t stats, stats_all;
	int ret;

	stats.execution_cycles = 0;

	k_thread_foreach(foreach_callback, &stats);

	/* Check NULL parameters */
	ret = k_thread_runtime_stats_all_get(NULL);
	zassert_true(ret == -EINVAL);

	k_thread_runtime_stats_all_get(&stats_all);

	zassert_true(stats.execution_cycles <= stats_all.execution_cycles);
}


/**
 * @brief Test the behavior of k_busy_wait with thread runtime statistics.
 *
 * This test verifies the accuracy of the `k_busy_wait` function by checking
 * the thread's execution cycle statistics before and after calling the function.
 */
ZTEST(threads_lifecycle, test_k_busy_wait)
{
	uint64_t cycles, dt;
	k_thread_runtime_stats_t test_stats;

	k_thread_runtime_stats_get(k_current_get(), &test_stats);
	cycles = test_stats.execution_cycles;
	k_busy_wait(0);
	k_thread_runtime_stats_get(k_current_get(), &test_stats);

	/* execution_cycles doesn't increase significantly after 0
	 * usec (10ms slop experimentally determined,
	 * non-deterministic software emulators are VERY slow wrt
	 * their cycle rate)
	 */
	dt = test_stats.execution_cycles - cycles;
	zassert_true(dt < k_ms_to_cyc_ceil64(10));

	cycles = test_stats.execution_cycles;
	k_busy_wait(100);
	k_thread_runtime_stats_get(k_current_get(), &test_stats);

	/* execution_cycles increases correctly */
	dt = test_stats.execution_cycles - cycles;

	/* execution cycles may not increase by the full 100µs as the
	 * system may be doing something else during the busy
	 * wait. Experimentally, we see at least 80% of the cycles
	 * consumed in the busy wait loop on current test targets.
	 */
	zassert_true(dt >= k_us_to_cyc_floor64(80));
}

static void tp_entry(void *p1, void *p2, void *p3)
{
	tp = 100;
}

/**
 * @brief Test the behavior of k_busy_wait with thread runtime statistics
 *        in user mode.
 *
 * @ingroup kernel_thread_tests
 */
ZTEST_USER(threads_lifecycle_1cpu, test_k_busy_wait_user)
{

	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				      tp_entry, NULL, NULL,
				      NULL, 0, K_USER, K_NO_WAIT);
	k_busy_wait(1000);
	/* this is a 1cpu test case, the new thread has no chance to be
	 * scheduled and value of tp not changed
	 */
	zassert_false(tp == 100);

	/* give up cpu, the new thread will change value of tp to 100 */
	k_msleep(100);
	zassert_true(tp == 100);
	k_thread_abort(tid);
}

#define INT_ARRAY_SIZE 128
static int large_stack(size_t *space)
{
	/* use "volatile" to protect this variable from being optimized out */
	volatile int a[INT_ARRAY_SIZE];

	/* to avoid unused variable error */
	a[0] = 1;
	return k_thread_stack_space_get(k_current_get(), space);

}

static int small_stack(size_t *space)
{
	return k_thread_stack_space_get(k_current_get(), space);
}

/**
 * @brief Test k_thread_stack_sapce_get
 *
 * Test k_thread_stack_sapce_get unused stack space in large_stack_space()
 * is smaller than that in small_stack() because the former function has a
 * large local variable
 *
 * @ingroup kernel_thread_tests
 */
ZTEST_USER(threads_lifecycle, test_k_thread_stack_space_get_user)
{
	size_t a, b;

	small_stack(&a);
	large_stack(&b);
	/* FIXME: Ideally, the follow condition will assert true:
	 * (a - b) == INT_ARRAY_SIZE * sizeof(int)
	 * but it is not the case in native_sim, qemu_leon3 and
	 * qemu_cortex_a53. Relax check condition here
	 */
	zassert_true(b <= a);
}

static void *thread_test_setup(void)
{
	k_thread_access_grant(k_current_get(), &tdata, tstack,
			      &tdata_custom, tstack_custom,
			      &tdata_name, tstack_name,
			      &join_thread, join_stack,
			      &control_thread, control_stack,
			      &deadlock1_thread, deadlock1_stack,
			      &deadlock2_thread, deadlock2_stack);
	main_prio = k_thread_priority_get(k_current_get());
#ifdef CONFIG_USERSPACE
	strncpy(unreadable_string, "unreadable string",
		sizeof(unreadable_string));
#endif

	return NULL;
}

ZTEST_SUITE(threads_lifecycle, NULL, thread_test_setup, NULL, NULL, NULL);
ZTEST_SUITE(threads_lifecycle_1cpu, NULL, thread_test_setup,
		ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
