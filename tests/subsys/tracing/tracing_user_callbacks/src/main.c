/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
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
static atomic_t isr_enter_calls;
static atomic_t isr_exit_calls;
static atomic_t work_seq;
static atomic_t work_enter_seq;
static atomic_t work_handler_seq;
static atomic_t work_exit_seq;

static void cb_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	atomic_set(&work_handler_seq, atomic_inc(&work_seq));
}

static K_WORK_DEFINE(cb_work, cb_work_handler);

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

void sys_trace_isr_enter_user(void)
{
	atomic_inc(&isr_enter_calls);
}

void sys_trace_isr_exit_user(void)
{
	atomic_inc(&isr_exit_calls);
}

void sys_trace_k_work_handler_enter_user(struct k_work *work)
{
	if (work == &cb_work) {
		atomic_set(&work_enter_seq, atomic_inc(&work_seq));
	}
}

void sys_trace_k_work_handler_exit_user(struct k_work *work)
{
	if (work == &cb_work) {
		atomic_set(&work_exit_seq, atomic_inc(&work_seq));
	}
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

/*
 * The ISR hooks live in each architecture's interrupt entry path rather than in
 * generic code, so an arch that never emits them looks exactly like an arch
 * with no interrupts. Sleeping guarantees at least one system timer interrupt.
 */
ZTEST(tracing_user_callbacks, test_isr_callbacks_fire)
{
	atomic_val_t enter_before = atomic_get(&isr_enter_calls);
	atomic_val_t exit_before = atomic_get(&isr_exit_calls);

	k_sleep(K_MSEC(50));

	zassert_true(atomic_get(&isr_enter_calls) > enter_before,
		     "sys_trace_isr_enter_user() was not invoked");
	zassert_true(atomic_get(&isr_exit_calls) > exit_before,
		     "sys_trace_isr_exit_user() was not invoked");
}

ZTEST(tracing_user_callbacks, test_work_handler_callbacks_fire)
{
	struct k_work_sync sync;

	atomic_set(&work_seq, 0);
	atomic_set(&work_enter_seq, -1);
	atomic_set(&work_handler_seq, -1);
	atomic_set(&work_exit_seq, -1);

	zassert_equal(k_work_submit(&cb_work), 1, "k_work_submit() did not queue cb_work");
	(void)k_work_flush(&cb_work, &sync);

	zassert_equal(atomic_get(&work_enter_seq), 0,
		      "sys_trace_k_work_handler_enter_user() did not run before handler");
	zassert_equal(atomic_get(&work_handler_seq), 1,
		      "cb_work_handler() did not run between enter and exit");
	zassert_equal(atomic_get(&work_exit_seq), 2,
		      "sys_trace_k_work_handler_exit_user() did not run after handler");
	zassert_equal(atomic_get(&work_seq), 3, "unexpected k_work handler trace callback count");
}

ZTEST_SUITE(tracing_user_callbacks, NULL, NULL, NULL, NULL, NULL);
