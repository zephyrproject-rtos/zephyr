/*
 * Copyright (c) 1997-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <debug/object_tracing_common.h>
#include <init.h>
#include <ksched.h>
#include <wait_q.h>
#include <syscall_handler.h>
#include <stdbool.h>
#include <spinlock.h>

static struct k_spinlock lock;

#ifdef CONFIG_OBJECT_TRACING

struct k_timer *_trace_list_k_timer;

/*
 * Complete initialization of statically defined timers.
 */
static int init_timer_module(const struct device *dev)
{
	ARG_UNUSED(dev);

	Z_STRUCT_SECTION_FOREACH(k_timer, timer) {
		SYS_TRACING_OBJ_INIT(k_timer, timer);
	}
	return 0;
}

SYS_INIT(init_timer_module, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS);

#endif /* CONFIG_OBJECT_TRACING */

/**
 * @brief Handle expiration of a kernel timer object.
 *
 * @param t  Timeout used by the timer.
 *
 * @return N/A
 */
void z_timer_expiration_handler(struct _timeout *t)
{
	struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
	struct k_thread *thread;

	/*
	 * if the timer is periodic, start it again; don't add _TICK_ALIGN
	 * since we're already aligned to a tick boundary
	 */
	if (!K_TIMEOUT_EQ(timer->period, K_NO_WAIT) &&
	    !K_TIMEOUT_EQ(timer->period, K_FOREVER)) {
		z_add_timeout(&timer->timeout, z_timer_expiration_handler,
			     timer->period);
	}

	/* update timer's status */
	timer->status += 1U;

	/* invoke timer expiry function */
	if (timer->expiry_fn != NULL) {
		timer->expiry_fn(timer);
	}

	thread = z_waitq_head(&timer->wait_q);

	if (thread == NULL) {
		return;
	}

	/*
	 * Interrupts _DO NOT_ have to be locked in this specific
	 * instance of thread unpending because a) this is the only
	 * place a thread can be taken off this pend queue, and b) the
	 * only place a thread can be put on the pend queue is at
	 * thread level, which of course cannot interrupt the current
	 * context.
	 */
	z_unpend_thread_no_timeout(thread);

	z_ready_thread(thread);

	arch_thread_return_value_set(thread, 0);
}


void k_timer_init(struct k_timer *timer,
			 k_timer_expiry_t expiry_fn,
			 k_timer_stop_t stop_fn)
{
	timer->expiry_fn = expiry_fn;
	timer->stop_fn = stop_fn;
	timer->status = 0U;

	z_waitq_init(&timer->wait_q);
	z_init_timeout(&timer->timeout);
	SYS_TRACING_OBJ_INIT(k_timer, timer);

	timer->user_data = NULL;

	z_object_init(timer);
}


void z_impl_k_timer_start(struct k_timer *timer, k_timeout_t duration,
			  k_timeout_t period)
{
	if (K_TIMEOUT_EQ(duration, K_FOREVER)) {
		return;
	}

#ifdef CONFIG_LEGACY_TIMEOUT_API
	duration = k_ms_to_ticks_ceil32(duration);
	period = k_ms_to_ticks_ceil32(period);
#else
	/* z_add_timeout() always adds one to the incoming tick count
	 * to round up to the next tick (by convention it waits for
	 * "at least as long as the specified timeout"), but the
	 * period interval is always guaranteed to be reset from
	 * within the timer ISR, so no round up is desired.  Subtract
	 * one.
	 *
	 * Note that the duration (!) value gets the same treatment
	 * for backwards compatibility.  This is unfortunate
	 * (i.e. k_timer_start() doesn't treat its initial sleep
	 * argument the same way k_sleep() does), but historical.  The
	 * timer_api test relies on this behavior.
	 */
	if (period.ticks != 0 && Z_TICK_ABS(period.ticks) < 0) {
		period.ticks = MAX(period.ticks - 1, 1);
	}
	if (Z_TICK_ABS(duration.ticks) < 0) {
		duration.ticks = MAX(duration.ticks - 1, 0);
	}
#endif

	(void)z_abort_timeout(&timer->timeout);
	timer->period = period;
	timer->status = 0U;

	z_add_timeout(&timer->timeout, z_timer_expiration_handler,
		     duration);
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_timer_start(struct k_timer *timer,
					k_timeout_t duration,
					k_timeout_t period)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_start(timer, duration, period);
}
#include <syscalls/k_timer_start_mrsh.c>
#endif

void z_impl_k_timer_stop(struct k_timer *timer)
{
	int inactive = z_abort_timeout(&timer->timeout) != 0;

	if (inactive) {
		return;
	}

	if (timer->stop_fn != NULL) {
		timer->stop_fn(timer);
	}

	struct k_thread *pending_thread = z_unpend1_no_timeout(&timer->wait_q);

	if (pending_thread != NULL) {
		z_ready_thread(pending_thread);
		z_reschedule_unlocked();
	}
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_timer_stop(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_stop(timer);
}
#include <syscalls/k_timer_stop_mrsh.c>
#endif

uint32_t z_impl_k_timer_status_get(struct k_timer *timer)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t result = timer->status;

	timer->status = 0U;
	k_spin_unlock(&lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline uint32_t z_vrfy_k_timer_status_get(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_status_get(timer);
}
#include <syscalls/k_timer_status_get_mrsh.c>
#endif

uint32_t z_impl_k_timer_status_sync(struct k_timer *timer)
{
	__ASSERT(!arch_is_in_isr(), "");

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t result = timer->status;

	if (result == 0U) {
		if (!z_is_inactive_timeout(&timer->timeout)) {
			/* wait for timer to expire or stop */
			(void)z_pend_curr(&lock, key, &timer->wait_q, K_FOREVER);

			/* get updated timer status */
			key = k_spin_lock(&lock);
			result = timer->status;
		} else {
			/* timer is already stopped */
		}
	} else {
		/* timer has already expired at least once */
	}

	timer->status = 0U;
	k_spin_unlock(&lock, key);

	return result;
}

#ifdef CONFIG_USERSPACE
static inline uint32_t z_vrfy_k_timer_status_sync(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_status_sync(timer);
}
#include <syscalls/k_timer_status_sync_mrsh.c>

static inline k_ticks_t z_vrfy_k_timer_remaining_ticks(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_remaining_ticks(timer);
}
#include <syscalls/k_timer_remaining_ticks_mrsh.c>

static inline k_ticks_t z_vrfy_k_timer_expires_ticks(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_expires_ticks(timer);
}
#include <syscalls/k_timer_expires_ticks_mrsh.c>

static inline void *z_vrfy_k_timer_user_data_get(struct k_timer *timer)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	return z_impl_k_timer_user_data_get(timer);
}
#include <syscalls/k_timer_user_data_get_mrsh.c>

static inline void z_vrfy_k_timer_user_data_set(struct k_timer *timer,
						void *user_data)
{
	Z_OOPS(Z_SYSCALL_OBJ(timer, K_OBJ_TIMER));
	z_impl_k_timer_user_data_set(timer, user_data);
}
#include <syscalls/k_timer_user_data_set_mrsh.c>

#endif
