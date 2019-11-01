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

#include <ztest.h>
#include <kernel_structs.h>
#include <kernel.h>
#include <kernel_internal.h>
#include <string.h>

extern void test_threads_spawn_params(void);
extern void test_threads_spawn_priority(void);
extern void test_threads_spawn_delay(void);
extern void test_threads_spawn_forever(void);
extern void test_thread_start(void);
extern void test_threads_suspend_resume_cooperative(void);
extern void test_threads_suspend_resume_preemptible(void);
extern void test_threads_abort_self(void);
extern void test_threads_abort_others(void);
extern void test_threads_abort_repeat(void);
extern void test_abort_handler(void);
extern void test_essential_thread_operation(void);
extern void test_threads_priority_set(void);
extern void test_delayed_thread_abort(void);
extern void test_k_thread_foreach(void);
extern void test_threads_cpu_mask(void);

struct k_thread tdata;
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
size_t tstack_size = K_THREAD_STACK_SIZEOF(tstack);

/*local variables*/
static K_THREAD_STACK_DEFINE(tstack_custom, STACK_SIZE);
static K_THREAD_STACK_DEFINE(tstack_name, STACK_SIZE);
static struct k_thread tdata_custom;
static struct k_thread tdata_name;

static int main_prio;

/**
 * @ingroup kernel_thread_tests
 * @brief Verify main thread
 */
void test_systhreads_main(void)
{
	zassert_true(main_prio == CONFIG_MAIN_THREAD_PRIORITY, NULL);
}

/**
 * @ingroup kernel_thread_tests
 * @brief Verify idle thread
 */
void test_systhreads_idle(void)
{
	k_sleep(K_MSEC(100));
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
		/* relinguish cpu for a while */
		k_sleep(K_MSEC(50));
		/** TESTPOINT: custom data comparison */
		zassert_equal(data, (long)k_thread_custom_data_get(), NULL);
		data++;
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief test thread custom data get/set from coop thread
 *
 * @see k_thread_custom_data_get(), k_thread_custom_data_set()
 */
void test_customdata_get_set_coop(void)
{
	k_tid_t tid = k_thread_create(&tdata_custom, tstack_custom, STACK_SIZE,
				      customdata_entry, NULL, NULL, NULL,
				      K_PRIO_COOP(1), 0, K_NO_WAIT);

	k_sleep(K_MSEC(500));

	/* cleanup environment */
	k_thread_abort(tid);
}

static void thread_name_entry(void *p1, void *p2, void *p3)
{
	/* Do nothing and exit */
}

/**
* @ingroup kernel_thread_tests
* @brief test thread name get/set from supervisor thread
* @see k_thread_name_get(), k_thread_name_copy(), k_thread_name_set()
*/
void test_thread_name_get_set(void)
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
 * @brief test thread name get/set from user thread
 * @see k_thread_name_copy(), k_thread_name_set()
 */
void test_thread_name_user_get_set(void)
{
#ifdef CONFIG_USERSPACE
	int ret;
	char thread_name[CONFIG_THREAD_MAX_NAME_LEN];
	char too_small[2];

	/* Some memory-related error cases for k_thread_name_set() */
	ret = k_thread_name_set(NULL, (const char *)0xFFFFFFF0);
	zassert_equal(ret, -EFAULT, "accepted nonsense string (%d)", ret);
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
	zassert_equal(ret, 0, "couldn't get main thread name");
	printk("Main thread name is '%s'\n", thread_name);

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
 * @brief test thread custom data get/set from preempt thread
 * @see k_thread_custom_data_get(), k_thread_custom_data_set()
 */
void test_customdata_get_set_preempt(void)
{
	/** TESTPOINT: custom data of preempt thread */
	k_tid_t tid = k_thread_create(&tdata_custom, tstack_custom, STACK_SIZE,
				      customdata_entry, NULL, NULL, NULL,
				      K_PRIO_PREEMPT(0), K_USER, K_NO_WAIT);

	k_sleep(K_MSEC(500));

	/* cleanup environment */
	k_thread_abort(tid);
}

#ifndef CONFIG_ARCH_HAS_USERSPACE
static void umode_entry(void *thread_id, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	if (!z_is_thread_essential() &&
	    (k_current_get() == (k_tid_t)thread_id)) {
		ztest_test_pass();
	} else {
		zassert_unreachable("User thread is essential or thread"
				    " structure is corrupted\n");
	}
}

/**
 * @ingroup kernel_thread_tests
 * @brief Test k_thread_user_mode_enter() to cover when userspace
 * is not supported/enabled
 * @see k_thread_user_mode_enter()
 */
void test_user_mode(void)
{
	z_thread_essential_set();

	zassert_true(z_is_thread_essential(), "Thread isn't set"
		     " as essential\n");

	k_thread_user_mode_enter((k_thread_entry_t)umode_entry,
				 k_current_get(), NULL, NULL);
}
#else
void test_user_mode(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
	k_thread_access_grant(k_current_get(), &tdata, tstack);
	k_thread_access_grant(k_current_get(), &tdata_custom, tstack_custom);
	k_thread_access_grant(k_current_get(), &tdata_name, tstack_name);
	main_prio = k_thread_priority_get(k_current_get());
#ifdef CONFIG_USERSPACE
	strncpy(unreadable_string, "unreadable string",
		sizeof(unreadable_string));
#endif

	ztest_test_suite(threads_lifecycle,
			 ztest_user_unit_test(test_threads_spawn_params),
			 ztest_unit_test(test_threads_spawn_priority),
			 ztest_user_unit_test(test_threads_spawn_delay),
			 ztest_unit_test(test_threads_spawn_forever),
			 ztest_unit_test(test_thread_start),
			 ztest_1cpu_unit_test(test_threads_suspend_resume_cooperative),
			 ztest_user_unit_test(test_threads_suspend_resume_preemptible),
			 ztest_unit_test(test_threads_priority_set),
			 ztest_user_unit_test(test_threads_abort_self),
			 ztest_user_unit_test(test_threads_abort_others),
			 ztest_1cpu_unit_test(test_threads_abort_repeat),
			 ztest_unit_test(test_abort_handler),
			 ztest_1cpu_unit_test(test_delayed_thread_abort),
			 ztest_unit_test(test_essential_thread_operation),
			 ztest_unit_test(test_systhreads_main),
			 ztest_unit_test(test_systhreads_idle),
			 ztest_1cpu_unit_test(test_customdata_get_set_coop),
			 ztest_1cpu_user_unit_test(test_customdata_get_set_preempt),
			 ztest_1cpu_unit_test(test_k_thread_foreach),
			 ztest_unit_test(test_thread_name_get_set),
			 ztest_user_unit_test(test_thread_name_user_get_set),
			 ztest_unit_test(test_user_mode),
			 ztest_1cpu_unit_test(test_threads_cpu_mask)
			 );

	ztest_run_test_suite(threads_lifecycle);
}
