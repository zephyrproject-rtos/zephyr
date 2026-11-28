/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

static K_SEM_DEFINE(controller_started, 0, 1);
static K_SEM_DEFINE(start_controller, 0, 1);
static K_SEM_DEFINE(join_returned, 0, 1);
static K_SEM_DEFINE(release_join, 0, 1);
static K_SEM_DEFINE(body_started, 0, 1);
static K_SEM_DEFINE(finish_body, 0, 1);
static K_SEM_DEFINE(exit_published, 0, 1);
static K_SEM_DEFINE(release_exit, 0, 1);
static K_SEM_DEFINE(release_keeper, 0, 1);
static k_tid_t controller_thread;
static k_tid_t joining_thread;
static k_tid_t exiting_thread;
static pthread_t join_target;
static int first_value;
static int second_value;
static int joined_result;
static void *joined_value;
static void (*scenario)(void);
static atomic_t pause_join;
static atomic_t pause_exit;
static atomic_t exit_is_held;

struct scenario_result {
	int error;
	int line;
	int create_result;
	bool freed_early;
	void *replacement_value;
};

static struct scenario_result result;

static bool record_error(int error, int line)
{
	if (result.error == 0 && error != 0) {
		result.error = error;
		result.line = line;
	}
	return error != 0;
}

#define API_FAILED(call) record_error((call), __LINE__)

int __real_z_impl_k_thread_join(struct k_thread *thread, k_timeout_t timeout);
void __real_z_impl_k_thread_abort(k_tid_t thread);
int __real_z_impl_k_thread_stack_free(k_thread_stack_t *stack);

/* Hold the joiner after termination, before pthread_join() consumes retval. */
int __wrap_z_impl_k_thread_join(struct k_thread *thread, k_timeout_t timeout)
{
	int ret = __real_z_impl_k_thread_join(thread, timeout);

	if (ret == 0 && k_current_get() == joining_thread && atomic_cas(&pause_join, 1, 0)) {
		k_sem_give(&join_returned);
		if (k_sem_take(&release_join, K_FOREVER) != 0) {
			k_panic();
		}
	}
	return ret;
}

/* DONE is visible, but the underlying kernel thread has not stopped. */
void __wrap_z_impl_k_thread_abort(k_tid_t thread)
{
	if (thread == exiting_thread && k_current_get() == thread &&
	    atomic_cas(&pause_exit, 1, 0)) {
		atomic_set(&exit_is_held, 1);
		k_sem_give(&exit_published);
		if (k_sem_take(&release_exit, K_FOREVER) != 0) {
			k_panic();
		}
		atomic_clear(&exit_is_held);
	}
	__real_z_impl_k_thread_abort(thread);
}

int __wrap_z_impl_k_thread_stack_free(k_thread_stack_t *stack)
{
	if (atomic_get(&exit_is_held) != 0) {
		/* Record the regression, then let the unfixed code finish safely. */
		result.freed_early = true;
		k_sem_give(&release_exit);
		if (__real_z_impl_k_thread_join(exiting_thread, K_SECONDS(2)) != 0) {
			k_panic();
		}
	}
	return __real_z_impl_k_thread_stack_free(stack);
}

static void *return_argument(void *arg)
{
	return arg;
}

static void *join_entry(void *arg)
{
	ARG_UNUSED(arg);
	joining_thread = k_current_get();
	joined_result = pthread_join(join_target, &joined_value);
	return NULL;
}

static void *held_exit(void *arg)
{
	exiting_thread = k_current_get();
	k_sem_give(&body_started);
	if (k_sem_take(&finish_body, K_FOREVER) != 0) {
		k_panic();
	}
	return arg;
}

static void *keeper_entry(void *arg)
{
	ARG_UNUSED(arg);
	if (k_sem_take(&release_keeper, K_FOREVER) != 0) {
		k_panic();
	}
	return NULL;
}

static void join_scenario(void)
{
	pthread_t joiner = 0;
	pthread_t replacement = 0;
	void *value;

	atomic_set(&pause_join, 1);
	if (API_FAILED(pthread_create(&join_target, NULL, return_argument, &first_value)) ||
	    API_FAILED(pthread_create(&joiner, NULL, join_entry, NULL)) ||
	    API_FAILED(k_sem_take(&join_returned, K_SECONDS(2)))) {
		goto cleanup;
	}

	/* Controller, target and joiner occupy all three slots. */
	result.create_result = pthread_create(&replacement, NULL, return_argument, &second_value);
	if (result.create_result == 0) {
		if (API_FAILED(pthread_join(replacement, &value))) {
			goto cleanup;
		}
		replacement = 0;
	}
	k_sem_give(&release_join);
	if (API_FAILED(pthread_join(joiner, NULL))) {
		goto cleanup;
	}
	joiner = 0;
	join_target = 0;

	if (API_FAILED(pthread_create(&replacement, NULL, return_argument, &second_value)) ||
	    API_FAILED(pthread_join(replacement, &result.replacement_value))) {
		goto cleanup;
	}
	replacement = 0;

cleanup:
	k_sem_give(&release_join);
	if (joiner != 0) {
		record_error(pthread_join(joiner, NULL), __LINE__);
	} else if (join_target != 0) {
		record_error(pthread_join(join_target, NULL), __LINE__);
	}
	if (replacement != 0) {
		record_error(pthread_join(replacement, NULL), __LINE__);
	}
}

static void stack_scenario(void)
{
	pthread_t target = 0;
	pthread_t keeper = 0;
	pthread_t replacement = 0;
	bool detached = false;

	atomic_set(&pause_exit, 1);
	if (API_FAILED(pthread_create(&keeper, NULL, keeper_entry, NULL)) ||
	    API_FAILED(pthread_create(&target, NULL, held_exit, &first_value)) ||
	    API_FAILED(k_sem_take(&body_started, K_SECONDS(2))) ||
	    API_FAILED(pthread_detach(target))) {
		goto cleanup;
	}
	detached = true;
	k_sem_give(&finish_body);
	if (API_FAILED(k_sem_take(&exit_published, K_SECONDS(2)))) {
		goto cleanup;
	}

	/* The live keeper prevents an unrelated free slot from hiding the race. */
	result.create_result = pthread_create(&replacement, NULL, return_argument, &second_value);
	if (result.create_result == 0) {
		if (API_FAILED(pthread_join(replacement, NULL))) {
			goto cleanup;
		}
		replacement = 0;
	}
	k_sem_give(&release_exit);
	if (API_FAILED(k_thread_join(exiting_thread, K_SECONDS(2)))) {
		goto cleanup;
	}
	k_sem_give(&release_keeper);
	if (API_FAILED(pthread_join(keeper, NULL))) {
		goto cleanup;
	}
	keeper = 0;

	if (API_FAILED(pthread_create(&replacement, NULL, return_argument, &second_value)) ||
	    API_FAILED(pthread_join(replacement, &result.replacement_value))) {
		goto cleanup;
	}
	replacement = 0;

cleanup:
	k_sem_give(&finish_body);
	k_sem_give(&release_exit);
	k_sem_give(&release_keeper);
	if (keeper != 0) {
		record_error(pthread_join(keeper, NULL), __LINE__);
	}
	if (target != 0 && !detached) {
		record_error(pthread_join(target, NULL), __LINE__);
	} else if (exiting_thread != NULL) {
		record_error(k_thread_join(exiting_thread, K_SECONDS(2)), __LINE__);
	}
	if (replacement != 0) {
		record_error(pthread_join(replacement, NULL), __LINE__);
	}
}

static void *controller_entry(void *arg)
{
	ARG_UNUSED(arg);
	controller_thread = k_current_get();
	k_sem_give(&controller_started);
	if (k_sem_take(&start_controller, K_FOREVER) != 0) {
		k_panic();
	}
	scenario();
	return NULL;
}

static void run_scenario(void (*function)(void))
{
	pthread_t controller;

	/* Keep pthread_join() callers in pthreads, including the test controller. */
	scenario = function;
	result = (struct scenario_result){0};
	k_sem_reset(&controller_started);
	k_sem_reset(&start_controller);
	k_sem_reset(&join_returned);
	k_sem_reset(&release_join);
	k_sem_reset(&body_started);
	k_sem_reset(&finish_body);
	k_sem_reset(&exit_published);
	k_sem_reset(&release_exit);
	k_sem_reset(&release_keeper);
	atomic_clear(&pause_join);
	atomic_clear(&pause_exit);
	atomic_clear(&exit_is_held);
	join_target = 0;
	joining_thread = NULL;
	exiting_thread = NULL;
	joined_value = NULL;
	joined_result = -1;
	zassert_ok(pthread_create(&controller, NULL, controller_entry, NULL));
	zassert_ok(k_sem_take(&controller_started, K_SECONDS(2)));
	zassert_ok(pthread_detach(controller));
	k_sem_give(&start_controller);
	zassert_ok(k_thread_join(controller_thread, K_SECONDS(5)));
	zassert_ok(result.error, "Unexpected API result %d at line %d", result.error, result.line);
}

ZTEST(pthread_lifecycle, test_join_retains_slot)
{
	run_scenario(join_scenario);
	zassert_equal(result.create_result, EAGAIN,
		      "A joiner must retain its slot until retval is consumed");
	zassert_ok(joined_result);
	zassert_equal_ptr(joined_value, &first_value);
	zassert_equal_ptr(result.replacement_value, &second_value);
}

ZTEST(pthread_lifecycle, test_stack_retained_until_termination)
{
	run_scenario(stack_scenario);
	zassert_false(result.freed_early, "Recycled a stack before kernel termination");
	zassert_equal(result.create_result, EAGAIN,
		      "An exiting thread still owns its stack and slot");
	zassert_equal_ptr(result.replacement_value, &second_value);
}

ZTEST_SUITE(pthread_lifecycle, NULL, NULL, NULL, NULL, NULL);
