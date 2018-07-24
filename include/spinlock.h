/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SPINLOCK_H_
#define ZEPHYR_INCLUDE_SPINLOCK_H_

#include <atomic.h>

#if defined(CONFIG_ASSERT) && (CONFIG_MP_NUM_CPUS < 4)
#include <kernel_structs.h>
#define SPIN_VALIDATE
#endif

struct k_spinlock_key {
	int key;
};

typedef struct k_spinlock_key k_spinlock_key_t;

struct k_spinlock {
#ifdef CONFIG_SMP
	atomic_t locked;
#endif

#ifdef SPIN_VALIDATE
	/* Stores the thread that holds the lock with the locking CPU
	 * ID in the bottom two bits.
	 */
	size_t thread_cpu;
#endif
};

static ALWAYS_INLINE k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
	k_spinlock_key_t k;

	/* Note that we need to use the underlying arch-specific lock
	 * implementation.  The "irq_lock()" API in SMP context is
	 * actually a wrapper for a global spinlock!
	 */
	k.key = _arch_irq_lock();

#ifdef SPIN_VALIDATE
	if (l->thread_cpu) {
		__ASSERT((l->thread_cpu & 3) != _current_cpu->id,
			 "Recursive spinlock");
	}
	l->thread_cpu = _current_cpu->id | (u32_t)_current;
#endif

#ifdef CONFIG_SMP
	while (!atomic_cas(&l->locked, 0, 1)) {
	}
#endif

	return k;
}

static ALWAYS_INLINE void k_spin_unlock(struct k_spinlock *l,
					k_spinlock_key_t key)
{
#ifdef SPIN_VALIDATE
	__ASSERT(l->thread_cpu == (_current_cpu->id | (u32_t)_current),
		 "Not my spinlock!");
	l->thread_cpu = 0;
#endif


#ifdef CONFIG_SMP
	/* Strictly we don't need atomic_clear() here (which is an
	 * exchange operation that returns the old value).  We are always
	 * setting a zero and (because we hold the lock) know the existing
	 * state won't change due to a race.  But some architectures need
	 * a memory barrier when used like this, and we don't have a
	 * Zephyr framework for that.
	 */
	atomic_clear(&l->locked);
#endif
	_arch_irq_unlock(key.key);
}

/* Internal function: releases the lock, but leaves local interrupts
 * disabled
 */
static ALWAYS_INLINE void k_spin_release(struct k_spinlock *l)
{
#ifdef SPIN_VALIDATE
	__ASSERT(z_spin_unlock_valid(l), "Not my spinlock!");
#endif
#ifdef CONFIG_SMP
	atomic_clear(&l->locked);
#endif
}


#endif /* ZEPHYR_INCLUDE_SPINLOCK_H_ */
