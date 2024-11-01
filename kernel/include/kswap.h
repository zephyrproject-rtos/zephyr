/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KSWAP_H_
#define ZEPHYR_KERNEL_INCLUDE_KSWAP_H_

#include <ksched.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/barrier.h>
#include <kernel_arch_func.h>

#ifdef CONFIG_STACK_SENTINEL
extern void z_check_stack_sentinel(void);
#else
#define z_check_stack_sentinel() /**/
#endif /* CONFIG_STACK_SENTINEL */

extern struct k_spinlock _sched_spinlock;

/* In SMP, the irq_lock() is a spinlock which is implicitly released
 * and reacquired on context switch to preserve the existing
 * semantics.  This means that whenever we are about to return to a
 * thread (via either z_swap() or interrupt/exception return!) we need
 * to restore the lock state to whatever the thread's counter
 * expects.
 */
void z_smp_release_global_lock(struct k_thread *thread);

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* Spin, with the scheduler lock held (!), on a thread that is known
 * (!!) to have released the lock and be on a path where it will
 * deterministically (!!!) reach arch_switch() in very small constant
 * time.
 *
 * This exists to treat an unavoidable SMP race when threads swap --
 * their thread record is in the queue (and visible to other CPUs)
 * before arch_switch() finishes saving state.  We must spin for the
 * switch handle before entering a new thread.  See docs on
 * arch_switch().
 *
 * Stated differently: there's a chicken and egg bug with the question
 * of "is a thread running or not?".  The thread needs to mark itself
 * "not running" from its own context, but at that moment it obviously
 * is still running until it reaches arch_switch()!  Locking can't
 * treat this because the scheduler lock can't be released by the
 * switched-to thread, which is going to (obviously) be running its
 * own code and doesn't know it was switched out.
 */
static inline void z_sched_switch_spin(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	volatile void **shp = (void *)&thread->switch_handle;

	while (*shp == NULL) {
		arch_spin_relax();
	}
	/* Read barrier: don't allow any subsequent loads in the
	 * calling code to reorder before we saw switch_handle go
	 * non-null.
	 */
	barrier_dmem_fence_full();
#endif /* CONFIG_SMP */
}

/* New style context switching.  arch_switch() is a lower level
 * primitive that doesn't know about the scheduler or return value.
 * Needed for SMP, where the scheduler requires spinlocking that we
 * don't want to have to do in per-architecture assembly.
 *
 * Note that is_spinlock is a compile-time construct which will be
 * optimized out when this function is expanded.
 */
static ALWAYS_INLINE unsigned int do_swap(unsigned int key,
					  struct k_spinlock *lock,
					  bool is_spinlock)
{
	ARG_UNUSED(lock);
	struct k_thread *new_thread, *old_thread;

#ifdef CONFIG_SPIN_VALIDATE
	/* Make sure the key acts to unmask interrupts, if it doesn't,
	 * then we are context switching out of a nested lock
	 * (i.e. breaking the lock of someone up the stack) which is
	 * forbidden!  The sole exception are dummy threads used
	 * during initialization (where we start with interrupts
	 * masked and switch away to begin scheduling) and the case of
	 * a dead current thread that was just aborted (where the
	 * damage was already done by the abort anyway).
	 *
	 * (Note that this is disabled on ARM64, where system calls
	 * can sometimes run with interrupts masked in ways that don't
	 * represent lock state.  See #35307)
	 */
# ifndef CONFIG_ARM64
	__ASSERT(arch_irq_unlocked(key) ||
		 _current->base.thread_state & (_THREAD_DUMMY | _THREAD_DEAD),
		 "Context switching while holding lock!");
# endif /* CONFIG_ARM64 */
#endif /* CONFIG_SPIN_VALIDATE */

	old_thread = _current;

	z_check_stack_sentinel();

	old_thread->swap_retval = -EAGAIN;

	/* We always take the scheduler spinlock if we don't already
	 * have it.  We "release" other spinlocks here.  But we never
	 * drop the interrupt lock.
	 */
	if (is_spinlock && lock != NULL && lock != &_sched_spinlock) {
		k_spin_release(lock);
	}
	if (!is_spinlock || lock != &_sched_spinlock) {
		(void) k_spin_lock(&_sched_spinlock);
	}

	new_thread = z_swap_next_thread();

	if (new_thread != old_thread) {
		z_sched_usage_switch(new_thread);

#ifdef CONFIG_SMP
		_current_cpu->swap_ok = 0;
		new_thread->base.cpu = arch_curr_cpu()->id;

		if (!is_spinlock) {
			z_smp_release_global_lock(new_thread);
		}
#endif /* CONFIG_SMP */
		z_thread_mark_switched_out();
		z_sched_switch_spin(new_thread);
		arch_current_thread_set(new_thread);

#ifdef CONFIG_TIMESLICING
		z_reset_time_slice(new_thread);
#endif /* CONFIG_TIMESLICING */

#ifdef CONFIG_SPIN_VALIDATE
		z_spin_lock_set_owner(&_sched_spinlock);
#endif /* CONFIG_SPIN_VALIDATE */

		arch_cohere_stacks(old_thread, NULL, new_thread);

#ifdef CONFIG_SMP
		/* Now add _current back to the run queue, once we are
		 * guaranteed to reach the context switch in finite
		 * time.  See z_sched_switch_spin().
		 */
		z_requeue_current(old_thread);
#endif /* CONFIG_SMP */
		void *newsh = new_thread->switch_handle;

		if (IS_ENABLED(CONFIG_SMP)) {
			/* Active threads must have a null here.  And
			 * it must be seen before the scheduler lock
			 * is released!
			 */
			new_thread->switch_handle = NULL;
			barrier_dmem_fence_full(); /* write barrier */
		}
		k_spin_release(&_sched_spinlock);
		arch_switch(newsh, &old_thread->switch_handle);
	} else {
		k_spin_release(&_sched_spinlock);
	}

	if (is_spinlock) {
		arch_irq_unlock(key);
	} else {
		irq_unlock(key);
	}

	return _current->swap_retval;
}

static inline int z_swap_irqlock(unsigned int key)
{
	return do_swap(key, NULL, false);
}

static inline int z_swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	return do_swap(key.key, lock, true);
}

static inline void z_swap_unlocked(void)
{
	(void) do_swap(arch_irq_lock(), NULL, true);
}

#else /* !CONFIG_USE_SWITCH */

extern int arch_swap(unsigned int key);

static inline void z_sched_switch_spin(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}

static inline int z_swap_irqlock(unsigned int key)
{
	int ret;
	z_check_stack_sentinel();
	ret = arch_swap(key);
	return ret;
}

/* If !USE_SWITCH, then spinlocks are guaranteed degenerate as we
 * can't be in SMP.  The k_spin_release() call is just for validation
 * handling.
 */
static ALWAYS_INLINE int z_swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	k_spin_release(lock);
	return z_swap_irqlock(key.key);
}

static inline void z_swap_unlocked(void)
{
	(void) z_swap_irqlock(arch_irq_lock());
}

#endif /* !CONFIG_USE_SWITCH */

/**
 * Set up a "dummy" thread, used at early initialization to launch the
 * first thread on a CPU.
 *
 * Needs to set enough fields such that the context switching code can
 * use it to properly store state, which will just be discarded.
 *
 * The memory of the dummy thread can be completely uninitialized.
 */
static inline void z_dummy_thread_init(struct k_thread *dummy_thread)
{
	dummy_thread->base.thread_state = _THREAD_DUMMY;
#ifdef CONFIG_SCHED_CPU_MASK
	dummy_thread->base.cpu_mask = -1;
#endif /* CONFIG_SCHED_CPU_MASK */
	dummy_thread->base.user_options = K_ESSENTIAL;
#ifdef CONFIG_THREAD_STACK_INFO
	dummy_thread->stack_info.start = 0U;
	dummy_thread->stack_info.size = 0U;
#endif /* CONFIG_THREAD_STACK_INFO */
#ifdef CONFIG_USERSPACE
	dummy_thread->mem_domain_info.mem_domain = &k_mem_domain_default;
#endif /* CONFIG_USERSPACE */
#if (K_HEAP_MEM_POOL_SIZE > 0)
	k_thread_system_pool_assign(dummy_thread);
#else
	dummy_thread->resource_pool = NULL;
#endif /* K_HEAP_MEM_POOL_SIZE */

#ifdef CONFIG_TIMESLICE_PER_THREAD
	dummy_thread->base.slice_ticks = 0;
#endif /* CONFIG_TIMESLICE_PER_THREAD */

	arch_current_thread_set(dummy_thread);
}
#endif /* ZEPHYR_KERNEL_INCLUDE_KSWAP_H_ */
