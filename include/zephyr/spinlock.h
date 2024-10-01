/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public interface for spinlocks
 */

#ifndef ZEPHYR_INCLUDE_SPINLOCK_H_
#define ZEPHYR_INCLUDE_SPINLOCK_H_

#include <errno.h>
#include <stdbool.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/time_units.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Spinlock APIs
 * @defgroup spinlock_apis Spinlock APIs
 * @ingroup kernel_apis
 * @{
 */

struct z_spinlock_key {
	int key;
};

/**
 * @brief Kernel Spin Lock
 *
 * This struct defines a spin lock record on which CPUs can wait with
 * k_spin_lock().  Any number of spinlocks may be defined in
 * application code.
 */
struct k_spinlock {
/**
 * @cond INTERNAL_HIDDEN
 */
#ifdef CONFIG_SMP
#ifdef CONFIG_TICKET_SPINLOCKS
	/*
	 * Ticket spinlocks are conceptually two atomic variables,
	 * one indicating the current FIFO head (spinlock owner),
	 * and the other indicating the current FIFO tail.
	 * Spinlock is acquired in the following manner:
	 * - current FIFO tail value is atomically incremented while it's
	 *   original value is saved as a "ticket"
	 * - we spin until the FIFO head becomes equal to the ticket value
	 *
	 * Spinlock is released by atomic increment of the FIFO head
	 */
	atomic_t owner;
	atomic_t tail;
#else
	atomic_t locked;
#endif /* CONFIG_TICKET_SPINLOCKS */
#endif /* CONFIG_SMP */

#ifdef CONFIG_SPIN_VALIDATE
	/* Stores the thread that holds the lock with the locking CPU
	 * ID in the bottom two bits.
	 */
	uintptr_t thread_cpu;
#ifdef CONFIG_SPIN_LOCK_TIME_LIMIT
	/* Stores the time (in cycles) when a lock was taken
	 */
	uint32_t lock_time;
#endif /* CONFIG_SPIN_LOCK_TIME_LIMIT */
#endif /* CONFIG_SPIN_VALIDATE */

#if defined(CONFIG_CPP) && !defined(CONFIG_SMP) && \
	!defined(CONFIG_SPIN_VALIDATE)
	/* If CONFIG_SMP and CONFIG_SPIN_VALIDATE are both not defined
	 * the k_spinlock struct will have no members. The result
	 * is that in C sizeof(k_spinlock) is 0 and in C++ it is 1.
	 *
	 * This size difference causes problems when the k_spinlock
	 * is embedded into another struct like k_msgq, because C and
	 * C++ will have different ideas on the offsets of the members
	 * that come after the k_spinlock member.
	 *
	 * To prevent this we add a 1 byte dummy member to k_spinlock
	 * when the user selects C++ support and k_spinlock would
	 * otherwise be empty.
	 */
	char dummy;
#endif
/**
 * INTERNAL_HIDDEN @endcond
 */
};

/* There's a spinlock validation framework available when asserts are
 * enabled.  It adds a relatively hefty overhead (about 3k or so) to
 * kernel code size, don't use on platforms known to be small.
 */
#ifdef CONFIG_SPIN_VALIDATE
bool z_spin_lock_valid(struct k_spinlock *l);
bool z_spin_unlock_valid(struct k_spinlock *l);
void z_spin_lock_set_owner(struct k_spinlock *l);
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 4, "Too many CPUs for mask");

# ifdef CONFIG_KERNEL_COHERENCE
bool z_spin_lock_mem_coherent(struct k_spinlock *l);
# endif /* CONFIG_KERNEL_COHERENCE */

#endif /* CONFIG_SPIN_VALIDATE */

/**
 * @brief Spinlock key type
 *
 * This type defines a "key" value used by a spinlock implementation
 * to store the system interrupt state at the time of a call to
 * k_spin_lock().  It is expected to be passed to a matching
 * k_spin_unlock().
 *
 * This type is opaque and should not be inspected by application
 * code.
 */
typedef struct z_spinlock_key k_spinlock_key_t;

static ALWAYS_INLINE void z_spinlock_validate_pre(struct k_spinlock *l)
{
	ARG_UNUSED(l);
#ifdef CONFIG_SPIN_VALIDATE
	__ASSERT(z_spin_lock_valid(l), "Invalid spinlock %p", l);
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT_NO_MSG(z_spin_lock_mem_coherent(l));
#endif
#endif
}

static ALWAYS_INLINE void z_spinlock_validate_post(struct k_spinlock *l)
{
	ARG_UNUSED(l);
#ifdef CONFIG_SPIN_VALIDATE
	z_spin_lock_set_owner(l);
#if defined(CONFIG_SPIN_LOCK_TIME_LIMIT) && (CONFIG_SPIN_LOCK_TIME_LIMIT != 0)
	l->lock_time = sys_clock_cycle_get_32();
#endif /* CONFIG_SPIN_LOCK_TIME_LIMIT */
#endif /* CONFIG_SPIN_VALIDATE */
}

/**
 * @brief Lock a spinlock
 *
 * This routine locks the specified spinlock, returning a key handle
 * representing interrupt state needed at unlock time.  Upon
 * returning, the calling thread is guaranteed not to be suspended or
 * interrupted on its current CPU until it calls k_spin_unlock().  The
 * implementation guarantees mutual exclusion: exactly one thread on
 * one CPU will return from k_spin_lock() at a time.  Other CPUs
 * trying to acquire a lock already held by another CPU will enter an
 * implementation-defined busy loop ("spinning") until the lock is
 * released.
 *
 * Separate spin locks may be nested. It is legal to lock an
 * (unlocked) spin lock while holding a different lock.  Spin locks
 * are not recursive, however: an attempt to acquire a spin lock that
 * the CPU already holds will deadlock.
 *
 * In circumstances where only one CPU exists, the behavior of
 * k_spin_lock() remains as specified above, though obviously no
 * spinning will take place.  Implementations may be free to optimize
 * in uniprocessor contexts such that the locking reduces to an
 * interrupt mask operation.
 *
 * @param l A pointer to the spinlock to lock
 * @return A key value that must be passed to k_spin_unlock() when the
 *         lock is released.
 */
static ALWAYS_INLINE k_spinlock_key_t k_spin_lock(struct k_spinlock *l)
{
	ARG_UNUSED(l);
	k_spinlock_key_t k;

	/* Note that we need to use the underlying arch-specific lock
	 * implementation.  The "irq_lock()" API in SMP context is
	 * actually a wrapper for a global spinlock!
	 */
	k.key = arch_irq_lock();

	z_spinlock_validate_pre(l);
#ifdef CONFIG_SMP
#ifdef CONFIG_TICKET_SPINLOCKS
	/*
	 * Enqueue ourselves to the end of a spinlock waiters queue
	 * receiving a ticket
	 */
	atomic_val_t ticket = atomic_inc(&l->tail);
	/* Spin until our ticket is served */
	while (atomic_get(&l->owner) != ticket) {
		arch_spin_relax();
	}
#else
	while (!atomic_cas(&l->locked, 0, 1)) {
		arch_spin_relax();
	}
#endif /* CONFIG_TICKET_SPINLOCKS */
#endif /* CONFIG_SMP */
	z_spinlock_validate_post(l);

	return k;
}

/**
 * @brief Attempt to lock a spinlock
 *
 * This routine makes one attempt to lock @p l. If it is successful, then
 * it will store the key into @p k.
 *
 * @param[in] l A pointer to the spinlock to lock
 * @param[out] k A pointer to the spinlock key
 * @retval 0 on success
 * @retval -EBUSY if another thread holds the lock
 *
 * @see k_spin_lock
 * @see k_spin_unlock
 */
static ALWAYS_INLINE int k_spin_trylock(struct k_spinlock *l, k_spinlock_key_t *k)
{
	int key = arch_irq_lock();

	z_spinlock_validate_pre(l);
#ifdef CONFIG_SMP
#ifdef CONFIG_TICKET_SPINLOCKS
	/*
	 * atomic_get and atomic_cas operations below are not executed
	 * simultaneously.
	 * So in theory k_spin_trylock can lock an already locked spinlock.
	 * To reproduce this the following conditions should be met after we
	 * executed atomic_get and before we executed atomic_cas:
	 *
	 * - spinlock needs to be taken 0xffff_..._ffff + 1 times
	 * (which requires 0xffff_..._ffff number of CPUs, as k_spin_lock call
	 * is blocking) or
	 * - spinlock needs to be taken and released 0xffff_..._ffff times and
	 * then taken again
	 *
	 * In real-life systems this is considered non-reproducible given that
	 * required actions need to be done during this tiny window of several
	 * CPU instructions (which execute with interrupt locked,
	 * so no preemption can happen here)
	 */
	atomic_val_t ticket_val = atomic_get(&l->owner);

	if (!atomic_cas(&l->tail, ticket_val, ticket_val + 1)) {
		goto busy;
	}
#else
	if (!atomic_cas(&l->locked, 0, 1)) {
		goto busy;
	}
#endif /* CONFIG_TICKET_SPINLOCKS */
#endif /* CONFIG_SMP */
	z_spinlock_validate_post(l);

	k->key = key;

	return 0;

#ifdef CONFIG_SMP
busy:
	arch_irq_unlock(key);
	return -EBUSY;
#endif /* CONFIG_SMP */
}

/**
 * @brief Unlock a spin lock
 *
 * This releases a lock acquired by k_spin_lock().  After this
 * function is called, any CPU will be able to acquire the lock.  If
 * other CPUs are currently spinning inside k_spin_lock() waiting for
 * this lock, exactly one of them will return synchronously with the
 * lock held.
 *
 * Spin locks must be properly nested.  A call to k_spin_unlock() must
 * be made on the lock object most recently locked using
 * k_spin_lock(), using the key value that it returned.  Attempts to
 * unlock mis-nested locks, or to unlock locks that are not held, or
 * to passing a key parameter other than the one returned from
 * k_spin_lock(), are illegal.  When CONFIG_SPIN_VALIDATE is set, some
 * of these errors can be detected by the framework.
 *
 * @param l A pointer to the spinlock to release
 * @param key The value returned from k_spin_lock() when this lock was
 *        acquired
 */
static ALWAYS_INLINE void k_spin_unlock(struct k_spinlock *l,
					k_spinlock_key_t key)
{
	ARG_UNUSED(l);
#ifdef CONFIG_SPIN_VALIDATE
	__ASSERT(z_spin_unlock_valid(l), "Not my spinlock %p", l);

#if defined(CONFIG_SPIN_LOCK_TIME_LIMIT) && (CONFIG_SPIN_LOCK_TIME_LIMIT != 0)
	uint32_t delta = sys_clock_cycle_get_32() - l->lock_time;

	__ASSERT(delta < CONFIG_SPIN_LOCK_TIME_LIMIT,
		 "Spin lock %p held %u cycles, longer than limit of %u cycles",
		 l, delta, CONFIG_SPIN_LOCK_TIME_LIMIT);
#endif /* CONFIG_SPIN_LOCK_TIME_LIMIT */
#endif /* CONFIG_SPIN_VALIDATE */

#ifdef CONFIG_SMP
#ifdef CONFIG_TICKET_SPINLOCKS
	/* Give the spinlock to the next CPU in a FIFO */
	(void)atomic_inc(&l->owner);
#else
	/* Strictly we don't need atomic_clear() here (which is an
	 * exchange operation that returns the old value).  We are always
	 * setting a zero and (because we hold the lock) know the existing
	 * state won't change due to a race.  But some architectures need
	 * a memory barrier when used like this, and we don't have a
	 * Zephyr framework for that.
	 */
	(void)atomic_clear(&l->locked);
#endif /* CONFIG_TICKET_SPINLOCKS */
#endif /* CONFIG_SMP */
	arch_irq_unlock(key.key);
}

/**
 * @cond INTERNAL_HIDDEN
 */

#if defined(CONFIG_SMP) && defined(CONFIG_TEST)
/*
 * @brief Checks if spinlock is held by some CPU, including the local CPU.
 *		This API shouldn't be used outside the tests for spinlock
 *
 * @param l A pointer to the spinlock
 * @retval true - if spinlock is held by some CPU; false - otherwise
 */
static ALWAYS_INLINE bool z_spin_is_locked(struct k_spinlock *l)
{
#ifdef CONFIG_TICKET_SPINLOCKS
	atomic_val_t ticket_val = atomic_get(&l->owner);

	return !atomic_cas(&l->tail, ticket_val, ticket_val);
#else
	return l->locked;
#endif /* CONFIG_TICKET_SPINLOCKS */
}
#endif /* defined(CONFIG_SMP) && defined(CONFIG_TEST) */

/* Internal function: releases the lock, but leaves local interrupts disabled */
static ALWAYS_INLINE void k_spin_release(struct k_spinlock *l)
{
	ARG_UNUSED(l);
#ifdef CONFIG_SPIN_VALIDATE
	__ASSERT(z_spin_unlock_valid(l), "Not my spinlock %p", l);
#endif
#ifdef CONFIG_SMP
#ifdef CONFIG_TICKET_SPINLOCKS
	(void)atomic_inc(&l->owner);
#else
	(void)atomic_clear(&l->locked);
#endif /* CONFIG_TICKET_SPINLOCKS */
#endif /* CONFIG_SMP */
}

#if defined(CONFIG_SPIN_VALIDATE) && defined(__GNUC__)
static ALWAYS_INLINE void z_spin_onexit(__maybe_unused k_spinlock_key_t *k)
{
	__ASSERT(k->key, "K_SPINLOCK exited with goto, break or return, "
			 "use K_SPINLOCK_BREAK instead.");
}
#define K_SPINLOCK_ONEXIT __attribute__((__cleanup__(z_spin_onexit)))
#else
#define K_SPINLOCK_ONEXIT
#endif

/**
 * INTERNAL_HIDDEN @endcond
 */

/**
 * @brief Leaves a code block guarded with @ref K_SPINLOCK after releasing the
 * lock.
 *
 * See @ref K_SPINLOCK for details.
 */
#define K_SPINLOCK_BREAK continue

/**
 * @brief Guards a code block with the given spinlock, automatically acquiring
 * the lock before executing the code block. The lock will be released either
 * when reaching the end of the code block or when leaving the block with
 * @ref K_SPINLOCK_BREAK.
 *
 * @details Example usage:
 *
 * @code{.c}
 * K_SPINLOCK(&mylock) {
 *
 *   ...execute statements with the lock held...
 *
 *   if (some_condition) {
 *     ...release the lock and leave the guarded section prematurely:
 *     K_SPINLOCK_BREAK;
 *   }
 *
 *   ...execute statements with the lock held...
 *
 * }
 * @endcode
 *
 * Behind the scenes this pattern expands to a for-loop whose body is executed
 * exactly once:
 *
 * @code{.c}
 * for (k_spinlock_key_t key = k_spin_lock(&mylock); ...; k_spin_unlock(&mylock, key)) {
 *     ...
 * }
 * @endcode
 *
 * @warning The code block must execute to its end or be left by calling
 * @ref K_SPINLOCK_BREAK. Otherwise, e.g. if exiting the block with a break,
 * goto or return statement, the spinlock will not be released on exit.
 *
 * @note In user mode the spinlock must be placed in memory accessible to the
 * application, see @ref K_APP_DMEM and @ref K_APP_BMEM macros for details.
 *
 * @param lck Spinlock used to guard the enclosed code block.
 */
#define K_SPINLOCK(lck)                                                                            \
	for (k_spinlock_key_t __i K_SPINLOCK_ONEXIT = {}, __key = k_spin_lock(lck); !__i.key;      \
	     k_spin_unlock((lck), __key), __i.key = 1)

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SPINLOCK_H_ */
