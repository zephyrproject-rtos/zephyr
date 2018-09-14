/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SPINLOCK_H_
#define ZEPHYR_INCLUDE_SPINLOCK_H_

#include <atomic.h>

struct k_spinlock_key {
	int key;
};

typedef struct k_spinlock_key k_spinlock_key_t;

struct k_spinlock {
#ifdef CONFIG_SMP
	atomic_t locked;
#ifdef CONFIG_DEBUG
	int saved_key;
#endif
#endif
};

static inline k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
	k_spinlock_key_t k;

	/* Note that we need to use the underlying arch-specific lock
	 * implementation.  The "irq_lock()" API in SMP context is
	 * actually a wrapper for a global spinlock!
	 */
	k.key = _arch_irq_lock();

#ifdef CONFIG_SMP
# ifdef CONFIG_DEBUG
	l->saved_key = k.key;
# endif
	while (!atomic_cas(&l->locked, 0, 1)) {
	}
#endif

	return k;
}

static inline void k_spin_unlock(struct k_spinlock *l, k_spinlock_key_t key)
{
#ifdef CONFIG_SMP
# ifdef CONFIG_DEBUG
	/* This doesn't attempt to catch all mismatches, just spots
	 * where the arch state register shows a difference.  Should
	 * add a nesting count or something...
	 */
	__ASSERT(l->saved_key == key.key, "Mismatched spin lock/unlock");
# endif
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

#endif /* ZEPHYR_INCLUDE_SPINLOCK_H_ */
