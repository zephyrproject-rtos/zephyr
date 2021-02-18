/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_KERNEL_INCLUDE_KSWAP_H_
#define ZEPHYR_KERNEL_INCLUDE_KSWAP_H_

#include <ksched.h>
#include <spinlock.h>
#include <kernel_arch_func.h>

#ifdef CONFIG_STACK_SENTINEL
extern void z_check_stack_sentinel(void);
#else
#define z_check_stack_sentinel() /**/
#endif

extern struct k_spinlock sched_spinlock;

/* In SMP, the irq_lock() is a spinlock which is implicitly released
 * and reacquired on context switch to preserve the existing
 * semantics.  This means that whenever we are about to return to a
 * thread (via either z_swap() or interrupt/exception return!) we need
 * to restore the lock state to whatever the thread's counter
 * expects.
 */
void z_smp_reacquire_global_lock(struct k_thread *thread);
void z_smp_release_global_lock(struct k_thread *thread);

/* context switching and scheduling-related routines */
#ifdef CONFIG_USE_SWITCH

/* There is an unavoidable SMP race when threads swap -- their thread
 * record is in the queue (and visible to other CPUs) before
 * arch_switch() finishes saving state.  We must spin for the switch
 * handle before entering a new thread.  See docs on arch_switch().
 *
 * Note: future SMP architectures may need a fence/barrier or cache
 * invalidation here.  Current ones don't, and sadly Zephyr doesn't
 * have a framework for that yet.
 */
static inline void wait_for_switch(struct k_thread *thread)
{
#ifdef CONFIG_SMP
	volatile void **shp = (void *)&thread->switch_handle;

	while (*shp == NULL) {
		k_busy_wait(1);
	}
#endif
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
					  int is_spinlock)
{
	ARG_UNUSED(lock);
	struct k_thread *new_thread, *old_thread;

	old_thread = _current;

	z_check_stack_sentinel();

	/* We always take the scheduler spinlock if we don't already
	 * have it.  We "release" other spinlocks here.  But we never
	 * drop the interrupt lock.
	 */
	if (is_spinlock && lock != NULL && lock != &sched_spinlock) {
		k_spin_release(lock);
	}
	if (!is_spinlock || lock != &sched_spinlock) {
		(void) k_spin_lock(&sched_spinlock);
	}

	new_thread = z_swap_next_thread();

	if (new_thread != old_thread) {
#ifdef CONFIG_TIMESLICING
		z_reset_time_slice();
#endif

		old_thread->swap_retval = -EAGAIN;

#ifdef CONFIG_SMP
		_current_cpu->swap_ok = 0;
		new_thread->base.cpu = arch_curr_cpu()->id;

		if (!is_spinlock) {
			z_smp_release_global_lock(new_thread);
		}
#endif
		z_thread_mark_switched_out();
		wait_for_switch(new_thread);
		_current_cpu->current = new_thread;

#ifdef CONFIG_SPIN_VALIDATE
		z_spin_lock_set_owner(&sched_spinlock);
#endif

#ifdef CONFIG_SMP
		/* Add _current back to the run queue HERE. After
		 * wait_for_switch() we are guaranteed to reach the
		 * context switch in finite time, avoiding a potential
		 * deadlock.
		 */
		z_requeue_current(old_thread);
#endif
		void *newsh = new_thread->switch_handle;

		if (IS_ENABLED(CONFIG_SMP)) {
			/* Active threads MUST have a null here */
			new_thread->switch_handle = NULL;
		}
		k_spin_release(&sched_spinlock);
		arch_cohere_stacks(old_thread, NULL, new_thread);
		arch_switch(newsh, &old_thread->switch_handle);
	} else {
		k_spin_release(&sched_spinlock);
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
	return do_swap(key, NULL, 0);
}

static inline int z_swap(struct k_spinlock *lock, k_spinlock_key_t key)
{
	return do_swap(key.key, lock, 1);
}

static inline void z_swap_unlocked(void)
{
	(void) do_swap(arch_irq_lock(), NULL, 1);
}

#else /* !CONFIG_USE_SWITCH */

extern int arch_swap(unsigned int key);

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
#endif
	dummy_thread->base.user_options = K_ESSENTIAL;
#ifdef CONFIG_THREAD_STACK_INFO
	dummy_thread->stack_info.start = 0U;
	dummy_thread->stack_info.size = 0U;
#endif
#ifdef CONFIG_USERSPACE
	dummy_thread->mem_domain_info.mem_domain = &k_mem_domain_default;
#endif

	_current_cpu->current = dummy_thread;
}
#endif /* ZEPHYR_KERNEL_INCLUDE_KSWAP_H_ */
