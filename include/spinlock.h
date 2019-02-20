/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SPINLOCK_H_
#define ZEPHYR_INCLUDE_SPINLOCK_H_

#include <atomic.h>

/* These stubs aren't provided by the mocking framework, and I can't
 * find a proper place to put them as mocking seems not to have a
 * proper "arch" layer.
 */
#ifdef ZTEST_UNITTEST
static inline int z_arch_irq_lock(void)
{
	return 0;
}

static inline void z_arch_irq_unlock(int key)
{
	ARG_UNUSED(key);
}
#endif

/* There's a spinlock validation framework available when asserts are
 * enabled.  It adds a relatively hefty overhead (about 3k or so) to
 * kernel code size, don't use on platforms known to be small. (Note
 * we're using the kconfig value here.  This isn't defined for every
 * board, but the default of zero works well as an "infinity"
 * fallback.  There is a DT_FLASH_SIZE parameter too, but that seems
 * even more poorly supported.
 */
#if (CONFIG_FLASH_SIZE == 0) || (CONFIG_FLASH_SIZE > 32)
#if defined(CONFIG_ASSERT) && (CONFIG_MP_NUM_CPUS < 4)
#include <misc/__assert.h>
struct k_spinlock;
int z_spin_lock_valid(struct k_spinlock *l);
int z_spin_unlock_valid(struct k_spinlock *l);
void z_spin_lock_set_owner(struct k_spinlock *l);
#define SPIN_VALIDATE
#endif
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
	ARG_UNUSED(l);
	k_spinlock_key_t k;

	/* Note that we need to use the underlying arch-specific lock
	 * implementation.  The "irq_lock()" API in SMP context is
	 * actually a wrapper for a global spinlock!
	 */
	k.key = z_arch_irq_lock();

#ifdef SPIN_VALIDATE
	__ASSERT(z_spin_lock_valid(l), "Recursive spinlock");
#endif

#ifdef CONFIG_SMP
	while (!atomic_cas(&l->locked, 0, 1)) {
	}
#endif

#ifdef SPIN_VALIDATE
	z_spin_lock_set_owner(l);
#endif
	return k;
}

static ALWAYS_INLINE void k_spin_unlock(struct k_spinlock *l,
					k_spinlock_key_t key)
{
	ARG_UNUSED(l);
#ifdef SPIN_VALIDATE
	__ASSERT(z_spin_unlock_valid(l), "Not my spinlock!");
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
	z_arch_irq_unlock(key.key);
}

/* Internal function: releases the lock, but leaves local interrupts
 * disabled
 */
static ALWAYS_INLINE void k_spin_release(struct k_spinlock *l)
{
	ARG_UNUSED(l);
#ifdef SPIN_VALIDATE
	__ASSERT(z_spin_unlock_valid(l), "Not my spinlock!");
#endif
#ifdef CONFIG_SMP
	atomic_clear(&l->locked);
#endif
}


#endif /* ZEPHYR_INCLUDE_SPINLOCK_H_ */
