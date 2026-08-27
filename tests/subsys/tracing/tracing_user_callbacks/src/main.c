/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * With CONFIG_TRACING_USER, every kernel-object hook routes to a __weak
 * sys_trace_<obj>_<event>_user() callback. A downstream integrator overrides
 * only the callbacks it cares about - no header forking, no edits to Zephyr.
 * Here we override two of the (Stage B) newly exposed object callbacks and
 * confirm the kernel drives them.
 */
static unsigned int sem_give_calls;
static unsigned int mutex_lock_calls;

void sys_trace_k_sem_give_enter_user(struct k_sem *sem)
{
	ARG_UNUSED(sem);
	sem_give_calls++;
}

void sys_trace_k_mutex_lock_enter_user(struct k_mutex *mutex, k_timeout_t timeout)
{
	ARG_UNUSED(mutex);
	ARG_UNUSED(timeout);
	mutex_lock_calls++;
}

K_SEM_DEFINE(cb_sem, 0, 1);
K_MUTEX_DEFINE(cb_mutex);

ZTEST(tracing_user_callbacks, test_object_callbacks_fire)
{
	unsigned int sem_before = sem_give_calls;
	unsigned int mtx_before = mutex_lock_calls;

	k_sem_give(&cb_sem);
	zassert_true(sem_give_calls > sem_before,
		     "sys_trace_k_sem_give_enter_user() was not invoked");

	zassert_ok(k_mutex_lock(&cb_mutex, K_FOREVER));
	k_mutex_unlock(&cb_mutex);
	zassert_true(mutex_lock_calls > mtx_before,
		     "sys_trace_k_mutex_lock_enter_user() was not invoked");
}

ZTEST_SUITE(tracing_user_callbacks, NULL, NULL, NULL, NULL, NULL);
