/*
 * Copyright (c) 2018, 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <kswap.h>
#include <ksched.h>
#include <ipi.h>

static int slice_ticks = DIV_ROUND_UP(CONFIG_TIMESLICE_SIZE * Z_HZ_ticks, Z_HZ_ms);
static int slice_max_prio = CONFIG_TIMESLICE_PRIORITY;

#ifdef CONFIG_SWAP_NONATOMIC
/* If z_swap() isn't atomic, then it's possible for a timer interrupt
 * to try to timeslice away _current after it has already pended
 * itself but before the corresponding context switch.  Treat that as
 * a noop condition in z_time_slice().
 */
struct k_thread *pending_current;
#endif

static inline int slice_time(struct k_thread *thread)
{
	int ret = slice_ticks;

#ifdef CONFIG_TIMESLICE_PER_THREAD
	if (thread->base.slice_ticks != 0) {
		ret = thread->base.slice_ticks;
	}
#else
	ARG_UNUSED(thread);
#endif
	return ret;
}

static int z_time_slice_size(struct k_thread *thread)
{
	if (z_is_thread_prevented_from_running(thread) ||
	    z_is_idle_thread_object(thread) ||
	    (slice_time(thread) == 0)) {
		return 0;
	}

#ifdef CONFIG_TIMESLICE_PER_THREAD
	if (thread->base.slice_ticks != 0) {
		return thread->base.slice_ticks;
	}
#endif

	if (thread_is_preemptible(thread) &&
	    !z_is_prio_higher(thread->base.prio, slice_max_prio)) {
		return slice_ticks;
	}

	return 0;
}

static void slice_timeout(struct _timeout *timeout)
{
	struct _timeslice *ts;
	struct _cpu *cpu;

	ts = CONTAINER_OF(timeout, struct _timeslice, timeout);
	cpu = CONTAINER_OF(ts, struct _cpu, timeslice);

	ts->expired = true;

	/* We need an IPI if we just handled a timeslice expiration
	 * for a different CPU.
	 */
	if (cpu->id != _current_cpu->id) {
		flag_ipi(IPI_CPU_MASK(cpu->id));
	}
}

void z_reset_time_slice(unsigned int cpu, struct k_thread *thread)
{
	int slice_size = z_time_slice_size(thread);
	struct _timeslice *ts = &_kernel.cpus[cpu].timeslice;

	z_abort_timeout(&ts->timeout);
	ts->expired = false;
	if (slice_size != 0) {
		z_add_timeout(&ts->timeout, slice_timeout,
			      K_TICKS(slice_size - 1));
	}
}

void k_sched_time_slice_set(int32_t slice, int prio)
{
	unsigned int cpu = 0;

	K_SPINLOCK(&_sched_spinlock) {
		slice_ticks = k_ms_to_ticks_ceil32(slice);
		slice_max_prio = prio;

#ifdef CONFIG_SMP
		cpu = _current_cpu->id;
#endif
		z_reset_time_slice(cpu, _kernel.cpus[cpu].current);
	}
}

#ifdef CONFIG_TIMESLICE_PER_THREAD
void k_thread_time_slice_set(struct k_thread *thread, int32_t thread_slice_ticks,
			     k_thread_timeslice_fn_t expired, void *data)
{
	unsigned int cpu = 0;

	K_SPINLOCK(&_sched_spinlock) {
		thread->base.slice_ticks = thread_slice_ticks;
		thread->base.slice_expired = expired;
		thread->base.slice_data = data;
#ifdef CONFIG_SMP
		cpu = thread->base.cpu;
#endif
		if (_kernel.cpus[cpu].current == thread) {
			z_reset_time_slice(cpu, thread);
		}
	}
}
#endif

/* Called out of each timer and IPI interrupt */
void z_time_slice(void)
{
	k_spinlock_key_t key = k_spin_lock(&_sched_spinlock);
	struct k_thread *curr = _current;

#ifdef CONFIG_SWAP_NONATOMIC
	if (pending_current == curr) {
		z_reset_time_slice(0, curr);
		k_spin_unlock(&_sched_spinlock, key);
		return;
	}
	pending_current = NULL;
#endif

	if (_current_cpu->timeslice.expired && (z_time_slice_size(curr) != 0)) {
#ifdef CONFIG_TIMESLICE_PER_THREAD
		if (curr->base.slice_expired) {
			k_spin_unlock(&_sched_spinlock, key);
			curr->base.slice_expired(curr, curr->base.slice_data);
			key = k_spin_lock(&_sched_spinlock);
		}
#endif
		if (!z_is_thread_prevented_from_running(curr)) {
			move_thread_to_end_of_prio_q(curr);
		}
		z_reset_time_slice(_current_cpu->id, curr);
	}
	k_spin_unlock(&_sched_spinlock, key);
}
