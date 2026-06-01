/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel_internal.h>
#include <zephyr/spinlock.h>
#include <zephyr/llext/symbol.h>

#define SPIN_CPU_ID_MASK (sizeof(void *) - 1)

/* Outermost spinlock held on each CPU; set-if-NULL on lock,
 * clear-if-matches on unlock.
 */
static struct k_spinlock *z_held_spinlock[CONFIG_MP_MAX_NUM_CPUS];

/* Total spinlock hold count per CPU, incremented on every lock and
 * decremented on every unlock regardless of nesting depth.
 */
static int z_held_spinlock_count[CONFIG_MP_MAX_NUM_CPUS];

/* Sentinel: set in swap_data by the ztest harness before force-aborting a
 * thread that holds a lock after an expected panic/assert.
 */
const uint8_t z_spinlock_abort_sentinel;

bool z_spin_lock_valid(struct k_spinlock *l)
{
	uintptr_t thread_cpu = l->thread_cpu;

	if (thread_cpu != 0U) {
		if ((thread_cpu & SPIN_CPU_ID_MASK) == _current_cpu->id) {
			return false;
		}
	}
	return true;
}
EXPORT_SYMBOL(z_spin_lock_valid);

bool z_spin_unlock_valid(struct k_spinlock *l)
{
	uint8_t cpu_id = _current_cpu->id;
	uintptr_t tcpu = l->thread_cpu;

	l->thread_cpu = 0;

	if (z_held_spinlock[cpu_id] == l) {
		z_held_spinlock[cpu_id] = NULL;
	}

	if (arch_is_in_isr() && _current->base.thread_state & _THREAD_DUMMY) {
		/* Edge case where an ISR aborted _current */
		z_held_spinlock_count[cpu_id]--;
		__ASSERT(z_held_spinlock_count[cpu_id] >= 0,
			 "spinlock unlock with no matching lock");
		return true;
	}
	if (tcpu != (cpu_id | (uintptr_t)_current)) {
		return false;
	}
	z_held_spinlock_count[cpu_id]--;
	__ASSERT(z_held_spinlock_count[cpu_id] >= 0,
		 "spinlock unlock with no matching lock");
	return true;
}
EXPORT_SYMBOL(z_spin_unlock_valid);

void z_spin_lock_set_owner(struct k_spinlock *l)
{
	uint8_t cpu_id = _current_cpu->id;

	l->thread_cpu = cpu_id | (uintptr_t)_current;

	if (z_held_spinlock[cpu_id] == NULL) {
		z_held_spinlock[cpu_id] = l;
	}
	z_held_spinlock_count[cpu_id]++;
}
EXPORT_SYMBOL(z_spin_lock_set_owner);

/* Called from do_swap() after z_current_thread_set() to transfer ownership
 * of an inherited lock.  Does NOT update the tracking arrays.
 */
void z_spin_lock_transfer_owner(struct k_spinlock *l)
{
	l->thread_cpu = _current_cpu->id | (uintptr_t)_current;
}
EXPORT_SYMBOL(z_spin_lock_transfer_owner);

/* Reset per-CPU spinlock tracking after abnormal thread termination.
 * All locks the dying thread held are abandoned (it will never resume to
 * release them), so only the locks still being released by the cleanup
 * path need to be counted.  lock_held=true if exactly one such lock
 * remains and will be released after this call.
 */
void z_spin_validate_reset(bool lock_held)
{
	uint8_t cpu_id = _current_cpu->id;

	z_held_spinlock[cpu_id] = NULL;
	z_held_spinlock_count[cpu_id] = lock_held ? 1 : 0;
}

/* Internal: caller must ensure IRQs are disabled. */
static inline struct k_spinlock *z_spin_get_held_lock_locked(uint8_t cpu_id)
{
	return z_held_spinlock[cpu_id];
}

#ifdef CONFIG_TEST
struct k_spinlock *z_spin_get_held_lock(void)
{
	unsigned int key = arch_irq_lock();
	struct k_spinlock *l = z_spin_get_held_lock_locked(_current_cpu->id);

	arch_irq_unlock(key);
	return l;
}
#endif /* CONFIG_TEST */

/*
 * Verify that a context switch is safe to perform.  Called from
 * do_swap() and z_swap_irqlock() before the scheduler takes over.
 *
 * swap_lock is the spinlock being released as part of this swap (e.g.
 * _sched_spinlock), or NULL for the irq-lock-only path.  It may legitimately
 * be held at the point of the call; any other held spinlock is a bug.
 *
 * key must show that interrupts were unlocked before the swap was entered.
 * If not, we are context switching out of a nested irq_lock() critical
 * section — i.e. breaking the lock of someone higher up the call stack,
 * which is forbidden.
 */
void z_assert_can_swap(unsigned int key, struct k_spinlock *swap_lock)
{
	uint8_t cpu_id = _current_cpu->id;
	struct k_spinlock *held = z_spin_get_held_lock_locked(cpu_id);
	int count = z_held_spinlock_count[cpu_id];
	int extra = (swap_lock != NULL) ? count - 1 : count;

	ARG_UNUSED(held);
	ARG_UNUSED(extra);

	/* ztest expected-fault: thread aborted with a lock held.*/
	if ((_current->base.thread_state & _THREAD_DEAD) &&
	    _current->base.swap_data == (void *)&z_spinlock_abort_sentinel) {
		z_spin_validate_reset(swap_lock != NULL);
	} else {
		__ASSERT(extra >= 0,
			 "swap_lock %p is not tracked in the spinlock hold count!", swap_lock);

		__ASSERT(held == NULL || held == swap_lock,
			 "Context switching while holding spinlock %p!", held);

		/* Catches lock(A); lock(B); unlock(A); z_swap(): slot cleared when A
		 * is released but count stays at 1 because B is still held.
		 */
		__ASSERT(extra == 0,
			 "Context switching while holding %d extra spinlock(s)!", extra);
	}

#ifndef CONFIG_ARM64
	/* Dummy threads start with IRQs masked and dead threads are being
	 * torn down; in both cases the IRQ state self-heals on the next
	 * switch-in, so they are exempt.  Disabled on ARM64 where FP/SIMD
	 * usage in exception context may leave IRQs masked without holding
	 * a lock (see #94285).
	 */
	__ASSERT(arch_irq_unlocked(key) ||
		 _current->base.thread_state & (_THREAD_DUMMY | _THREAD_DEAD),
		 "Context switching with irq_lock held!");
#endif
}

#ifdef CONFIG_KERNEL_COHERENCE
bool z_spin_lock_mem_coherent(struct k_spinlock *l)
{
	return sys_cache_is_mem_coherent((void *)l);
}
EXPORT_SYMBOL(z_spin_lock_mem_coherent);
#endif /* CONFIG_KERNEL_COHERENCE */
