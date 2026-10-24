/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * SMP side of the scheduler: selecting the next thread, exchanging switch
 * handles and halting threads that run on other CPUs. The common scheduler
 * in kernel/sched.c reaches this code through the hooks in ksmp.h.
 */

#include <zephyr/kernel.h>
#include <kspinlock.h>
#include <ksched.h>
#include <ksmp.h>
#include <kswap.h>
#include <kthread.h>
#include <ipi.h>
#include <metairq.h>
#include <run_q.h>
#include <timeout_q.h>
#include <timeslicing.h>
#include <usage.h>
#include <kernel_arch_func.h>

static ALWAYS_INLINE struct k_thread *next_up(void)
{
	bool ipi_idle_target_rebound = false;
	struct k_thread *ipi_idle_target = ipi_idle_reserved_take();

	if (z_is_thread_halting(_current)) {
		/* NULL key: scheduler context, no retry possible. _current
		 * cannot have an in-flight timeout (a running thread's
		 * timeout already fired and its handler returned), so the
		 * abort inside the halt won't see -EAGAIN.
		 */
		z_sched_halt_thread_locked(_current, z_is_thread_aborting(_current) ?
						     _THREAD_DEAD : _THREAD_SUSPENDED, NULL);
	}

	struct k_thread *thread = runq_best();

	thread = metairq_preempt_recover(thread);

	/* Under SMP, the "cache" mechanism for selecting the next
	 * thread doesn't work, so we have more work to do to test
	 * _current against the best choice from the queue.  Here, the
	 * thread selected above represents "the best thread that is
	 * not current".
	 *
	 * Subtle note on "queued": in SMP mode, neither _current nor
	 * metairq_premepted live in the queue, so this isn't exactly the
	 * same thing as "ready", it means "the thread already been
	 * added back to the queue such that we don't want to re-add it".
	 */
	bool queued = z_is_thread_queued(_current);
	bool active = z_is_thread_ready(_current);

	if (thread == NULL) {
		thread = _current_cpu->idle_thread;
	}

	if (active) {
		int32_t cmp = z_sched_prio_cmp(_current, thread);

		/* Ties only switch if state says we yielded */
		if ((cmp > 0) || ((cmp == 0) && !_current_cpu->swap_ok)) {
			thread = _current;
		}

		if (!should_preempt(thread, _current_cpu->swap_ok)) {
			thread = _current;
		}
	}

	if (thread != _current) {
		update_metairq_preempt(thread);
		/*
		 * Put _current back into the queue unless it is ..
		 * 1. not active (i.e., blocked, suspended, dead), or
		 * 2. already queued, or
		 * 3. the idle thread, or
		 * 4. preempted by a MetaIRQ thread
		 */
		if (active && !queued && !z_is_idle_thread_object(_current)
		    && metairq_current_requeue_allowed()) {
			queue_thread(_current);
		}
	}

	/* Take the new _current out of the queue */
	if (z_is_thread_queued(thread)) {
		/* Remove or transfer the selected thread's idle CPU coverage. */
		if (ipi_idle_target != NULL &&
		    thread != ipi_idle_target &&
		    z_is_thread_queued(ipi_idle_target)) {
			ipi_idle_target_rebound =
				ipi_idle_thread_rebind(thread, ipi_idle_target);
		} else {
			ipi_idle_thread_unreserve(thread);
		}
		dequeue_thread(thread);
	}

	_current_cpu->swap_ok = false;

	/* If this CPU consumed a different thread, preserve coverage for the
	 * runnable thread covered by this CPU's reservation.
	 */
	if (!ipi_idle_target_rebound &&
	    ipi_idle_target != NULL && z_is_thread_queued(ipi_idle_target)) {
		flag_ipi(ipi_mask_create(ipi_idle_target));
	}
	return thread;
}

struct k_thread *z_swap_next_thread(void)
{
	struct k_thread *ret = next_up();

	if (ret == _current) {
		/* When not swapping, have to signal IPIs here.  In
		 * the context switch case it must happen later, after
		 * _current gets requeued.
		 */
		signal_pending_ipi();
	}
	return ret;
}

void *z_get_next_switch_handle(void *interrupted)
{
	void *ret = NULL;

	z_check_stack_sentinel();

	Z_SCHED_SPINLOCK {
		struct k_thread *old_thread = _current, *new_thread;

		__ASSERT(old_thread->switch_handle == NULL || is_thread_dummy(old_thread),
			"old thread handle should be null.");

		new_thread = next_up();

		z_sched_usage_switch(new_thread);

		if (old_thread != new_thread) {
			uint8_t  cpu_id;

			z_sched_switch_spin(new_thread);
			arch_cohere_stacks(old_thread, interrupted, new_thread);

			_current_cpu->swap_ok = 0;
			cpu_id = arch_curr_cpu()->id;
			new_thread->base.cpu = cpu_id;
			z_sched_set_current(new_thread);

#ifdef CONFIG_TIMESLICING
			z_time_slice_reset(new_thread);
#endif /* CONFIG_TIMESLICING */

			/* Changed _current!  Update the scheduler's spinlock
			 * bookkeeping so the validation doesn't get
			 * confused when the "wrong" thread tries to
			 * release the lock.
			 */
			z_sched_spinlock_transfer_owner(new_thread);

			/* A queued (runnable) old/current thread
			 * needs to be added back to the run queue
			 * here, and atomically with its switch handle
			 * being set below.  This is safe now, as we
			 * will not return into it.
			 */
			if (z_is_thread_queued(old_thread)) {
#ifdef CONFIG_SCHED_IPI_CASCADE
				if ((new_thread->base.cpu_mask != -1) &&
				    (old_thread->base.cpu_mask != BIT(cpu_id))) {
					flag_ipi(ipi_mask_create(old_thread));
				}
#endif
				runq_add(old_thread);
			}
		}
		old_thread->switch_handle = interrupted;
		ret = new_thread->switch_handle;
		/* Active threads MUST have a null here */
		new_thread->switch_handle = NULL;

		/* Check for IPIs under the lock to avoid silently consuming a
		 * rescheduling IPI flagged by another CPU for ourselves.
		 */
		signal_pending_ipi();
	}
	return ret;
}

void z_smp_switch_requeue(struct k_thread *old_thread)
{
	/* Now add _current back to the run queue, once we are
	 * guaranteed to reach the context switch in finite time.  See
	 * z_sched_switch_spin().  The current thread can never live in
	 * the run queue until we are inexorably on the context switch
	 * path on SMP, otherwise there is a deadlock condition where a
	 * set of CPUs pick a cycle of threads to run and wait for them
	 * all to context switch forever.
	 */
	if (z_is_thread_queued(old_thread)) {
		runq_add(old_thread);
	}
	signal_pending_ipi();
}

/* Spins in ISR context, waiting for a thread known to be running on
 * another CPU to catch the IPI we sent and halt.  Note that we check
 * for ourselves being asynchronously halted first to prevent simple
 * deadlocks (but not complex ones involving cycles of 3+ threads!).
 * Acts to release the provided lock before returning.
 */
static void thread_halt_spin(struct k_thread *thread, k_spinlock_key_t key)
{
	if (z_is_thread_halting(_current)) {
		z_sched_halt_thread_locked(_current,
					   z_is_thread_aborting(_current) ? _THREAD_DEAD
									  : _THREAD_SUSPENDED,
					   &key);
	}
	z_sched_spinlock_unlock(key);
	while (z_is_thread_halting(thread)) {
		unsigned int k = arch_irq_lock();

		arch_spin_relax(); /* Requires interrupts be masked */
		arch_irq_unlock(k);
	}
}

void z_smp_halt_remote(struct k_thread *thread, struct _cpu *cpu,
		       k_spinlock_key_t key, bool terminate)
{
	_wait_q_t *wq = terminate ? &thread->join_queue : &thread->halt_queue;

	thread->base.thread_state |= (terminate ? _THREAD_ABORTING
				      : _THREAD_SUSPENDING);
#ifdef CONFIG_SCHED_IPI_SUPPORTED
#ifdef CONFIG_ARCH_HAS_DIRECTED_IPIS
	arch_sched_directed_ipi(IPI_CPU_MASK(cpu->id));
#else
	ARG_UNUSED(cpu);
	arch_sched_broadcast_ipi();
#endif /* CONFIG_ARCH_HAS_DIRECTED_IPIS */
#else
	ARG_UNUSED(cpu);
#endif /* CONFIG_SCHED_IPI_SUPPORTED */
	if (arch_is_in_isr()) {
		thread_halt_spin(thread, key);
	} else  {
		z_sched_add_to_waitq_locked(_current, wq);
		z_swap_locked(key);
	}
	/* The target's next_up self-halt path passed NULL to
	 * z_sched_halt_thread_locked() and could not retry on -EAGAIN; an
	 * in-flight handler on a third CPU may not have run yet
	 * (it is blocked on the scheduler spinlock and will only
	 * acquire it after we drop it via the swap/spin above). Wait
	 * now, outside any lock, before the caller may free the thread
	 * storage. The handler, when it runs, sees _THREAD_DEAD /
	 * _THREAD_SUSPENDED and either bails (killed check) or
	 * no-ops in ready_thread() (z_is_thread_ready() rejects
	 * suspended threads). After this loop returns, no further
	 * dereference of thread->base will occur.
	 */
	while (z_try_abort_thread_timeout(thread) == -EAGAIN) {
	}
}

void z_smp_halt_queue_wake(struct k_thread *thread)
{
	z_sched_unpend_all_locked(&thread->halt_queue);
}
