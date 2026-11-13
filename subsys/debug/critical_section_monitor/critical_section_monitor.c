/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/internal/critical_section_hooks.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include <kernel_arch_func.h>
#include "critical_section_monitor.h"

#define Z_CRITICAL_SECTION_MONITOR_FN     __no_instrumentation__ __noasan
#define Z_CRITICAL_SECTION_MONITOR_HOOK   __noinline Z_CRITICAL_SECTION_MONITOR_FN
#define Z_CRITICAL_SECTION_MONITOR_INLINE static ALWAYS_INLINE Z_CRITICAL_SECTION_MONITOR_FN

#define SNAPSHOT_MAX_ATTEMPTS 32U

struct z_critical_section_monitor_interval {
	uintptr_t caller;
	const struct k_spinlock *spinlock;
	const struct k_thread *thread;
	uint32_t start_cycles;
};

/*
 * Keep independently updated per-CPU state on separate cache lines. This is
 * especially important when another CPU snapshots the sequence counter.
 */
#if defined(CONFIG_SMP) && defined(CONFIG_DCACHE)
#define Z_CRITICAL_SECTION_MONITOR_CPU_ALIGN __aligned(CONFIG_DCACHE_LINE_SIZE)
#else
#define Z_CRITICAL_SECTION_MONITOR_CPU_ALIGN
#endif

struct z_critical_section_monitor_cpu {
	struct z_critical_section_monitor_interval irq;
#ifdef CONFIG_SMP
	struct z_critical_section_monitor_interval pending;
#endif
	struct z_critical_section_monitor_interval
		held[CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS];
	unsigned int held_count;
	bool irq_active;
#ifdef CONFIG_SMP
	bool pending_active;
	atomic_t sequence;
#endif
	struct z_critical_section_monitor_stats stats;
} Z_CRITICAL_SECTION_MONITOR_CPU_ALIGN;

static struct z_critical_section_monitor_cpu critical_section_monitor_cpus[CONFIG_MP_MAX_NUM_CPUS];

static bool monitor_ready;

static Z_CRITICAL_SECTION_MONITOR_FN int critical_section_monitor_init(void)
{
	unsigned int key = arch_irq_lock();

	/* Set before any secondary CPU starts. */
	monitor_ready = true;

	arch_irq_unlock(key);
	return 0;
}

SYS_INIT(critical_section_monitor_init, POST_KERNEL, 0);

Z_CRITICAL_SECTION_MONITOR_INLINE struct z_critical_section_monitor_cpu *current_cpu_state(void)
{
#ifdef CONFIG_SMP
	return &critical_section_monitor_cpus[arch_curr_cpu()->id];
#else
	return &critical_section_monitor_cpus[0];
#endif
}

/* Save _current when timing starts; it may change before timing ends. */
Z_CRITICAL_SECTION_MONITOR_INLINE struct z_critical_section_monitor_interval
interval_start(const struct k_spinlock *spinlock, uintptr_t caller, uint32_t now)
{
	return (struct z_critical_section_monitor_interval){
		.caller = caller,
		.spinlock = spinlock,
		.thread = _current,
		.start_cycles = now,
	};
}

/*
 * Other CPUs may read stats while this CPU updates or resets them.
 * Access each field atomically. The sequence counter and fences let readers
 * detect updates and retry instead of returning a mix of old and new values.
 */
#ifdef CONFIG_SMP
#define STATS_LOAD(ptr)         __atomic_load_n(ptr, __ATOMIC_RELAXED)
#define STATS_STORE(ptr, value) __atomic_store_n(ptr, value, __ATOMIC_RELAXED)

BUILD_ASSERT(__atomic_always_lock_free(sizeof(uintptr_t), 0));
BUILD_ASSERT(__atomic_always_lock_free(sizeof(uint32_t), 0));
BUILD_ASSERT(__atomic_always_lock_free(sizeof(bool), 0));
#else
#define STATS_LOAD(ptr)         (*(ptr))
#define STATS_STORE(ptr, value) (*(ptr) = (value))
#endif

static Z_CRITICAL_SECTION_MONITOR_FN void
record_store(struct z_critical_section_monitor_record *dest,
	     const struct z_critical_section_monitor_record *src)
{
	STATS_STORE(&dest->caller, src->caller);
	STATS_STORE(&dest->spinlock, src->spinlock);
	STATS_STORE(&dest->thread, src->thread);
	STATS_STORE(&dest->cycles, src->cycles);
	STATS_STORE(&dest->in_isr, src->in_isr);
}

static Z_CRITICAL_SECTION_MONITOR_FN void
stats_write_begin(struct z_critical_section_monitor_cpu *state)
{
#ifdef CONFIG_SMP
	(void)atomic_inc(&state->sequence);
	/* Make the odd sequence visible before any new stats values. */
	__atomic_thread_fence(__ATOMIC_RELEASE);
#else
	ARG_UNUSED(state);
#endif
}

static Z_CRITICAL_SECTION_MONITOR_FN void
stats_write_end(struct z_critical_section_monitor_cpu *state)
{
#ifdef CONFIG_SMP
	(void)atomic_inc(&state->sequence);
#else
	ARG_UNUSED(state);
#endif
}

static Z_CRITICAL_SECTION_MONITOR_HOOK void
record_max_slow(struct z_critical_section_monitor_cpu *state,
		enum z_critical_section_monitor_event event, uint32_t cycles,
		const struct z_critical_section_monitor_interval *interval)
{
	struct z_critical_section_monitor_record *record = &state->stats.max[event];
	const struct z_critical_section_monitor_record next = {
		.caller = interval->caller,
		.spinlock = interval->spinlock,
		.thread = interval->thread,
		.cycles = cycles,
		/*
		 * IRQs have not been restored and no thread switch has occurred.
		 * The ISR flag still describes the interval that just ended.
		 */
		.in_isr = arch_is_in_isr(),
	};

	stats_write_begin(state);
	record_store(record, &next);
	stats_write_end(state);
}

Z_CRITICAL_SECTION_MONITOR_INLINE void
record_max(struct z_critical_section_monitor_cpu *state,
	   enum z_critical_section_monitor_event event, uint32_t cycles,
	   const struct z_critical_section_monitor_interval *interval)
{
	struct z_critical_section_monitor_record *record = &state->stats.max[event];

	if (likely(cycles <= STATS_LOAD(&record->cycles))) {
		return;
	}

	record_max_slow(state, event, cycles, interval);
}

static Z_CRITICAL_SECTION_MONITOR_FN void
record_spinlock_overflow(struct z_critical_section_monitor_cpu *state)
{
	uint32_t *overflows = &state->stats.spinlock_tracking_overflows;

	stats_write_begin(state);
	/* Only this CPU writes its statistics, with local interrupts locked. */
	STATS_STORE(overflows, STATS_LOAD(overflows) + 1U);
	stats_write_end(state);
}

Z_CRITICAL_SECTION_MONITOR_INLINE void
spin_hold_start(struct z_critical_section_monitor_cpu *state,
		const struct z_critical_section_monitor_interval *interval)
{
	unsigned int count = state->held_count;

	if (count == ARRAY_SIZE(state->held)) {
		record_spinlock_overflow(state);
		return;
	}

	state->held[count] = *interval;
	state->held_count = count + 1U;
}

Z_CRITICAL_SECTION_MONITOR_INLINE void
irq_start(struct z_critical_section_monitor_cpu *state,
	  const struct z_critical_section_monitor_interval *interval)
{
	/* IRQs were enabled before this lock, so replace any previous interval. */
	state->irq = *interval;
	state->irq_active = true;
}

Z_CRITICAL_SECTION_MONITOR_INLINE void
irq_end(struct z_critical_section_monitor_cpu *state, uint32_t now)
{
	if (!state->irq_active) {
		return;
	}

	state->irq_active = false;

	record_max(state, Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED,
		   now - state->irq.start_cycles, &state->irq);
}

Z_CRITICAL_SECTION_MONITOR_HOOK void z_crit_hook_irq_begin(unsigned int key, uintptr_t caller)
{
	if (unlikely(!monitor_ready) || !arch_irq_unlocked(key)) {
		return;
	}

	struct z_critical_section_monitor_interval interval =
		interval_start(NULL, caller, k_cycle_get_32());
	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	irq_start(state, &interval);
}

Z_CRITICAL_SECTION_MONITOR_HOOK void z_crit_hook_irq_end(unsigned int key)
{
	if (unlikely(!monitor_ready) || !arch_irq_unlocked(key)) {
		return;
	}

	uint32_t now = k_cycle_get_32();
	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	irq_end(state, now);
}

Z_CRITICAL_SECTION_MONITOR_HOOK void z_crit_hook_irq_idle(unsigned int key)
{
	if (unlikely(!monitor_ready)) {
		return;
	}

	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	if (arch_irq_unlocked(key)) {
		/* IRQs were enabled on entry, so discard any stale IRQ interval. */
		state->irq_active = false;
	} else if (state->irq_active) {
		irq_end(state, k_cycle_get_32());
	}
}

#ifdef CONFIG_SMP
Z_CRITICAL_SECTION_MONITOR_HOOK void
z_crit_hook_spin_attempt(const struct k_spinlock *lock, unsigned int key)
{
	if (unlikely(!monitor_ready)) {
		return;
	}

	struct z_critical_section_monitor_interval interval =
		interval_start(lock, Z_CRITICAL_SECTION_CALLER(), k_cycle_get_32());
	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	if (arch_irq_unlocked(key)) {
		irq_start(state, &interval);
	}

	if (state->pending_active) {
		return;
	}

	state->pending = interval;
	state->pending_active = true;
}
#endif /* CONFIG_SMP */

Z_CRITICAL_SECTION_MONITOR_HOOK void
z_crit_hook_spin_acquired(const struct k_spinlock *lock, unsigned int key)
{
	if (unlikely(!monitor_ready)) {
		return;
	}

	uint32_t now = k_cycle_get_32();
	struct z_critical_section_monitor_cpu *state = current_cpu_state();

#ifdef CONFIG_SMP
	struct z_critical_section_monitor_interval *pending = &state->pending;

	ARG_UNUSED(key);

	if (!state->pending_active || pending->spinlock != lock) {
		return;
	}

	state->pending_active = false;
	record_max(state, Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT,
		   now - pending->start_cycles, pending);

	pending->start_cycles = now;
	spin_hold_start(state, pending);
#else
	struct z_critical_section_monitor_interval interval =
		interval_start(lock, Z_CRITICAL_SECTION_CALLER(), now);

	if (arch_irq_unlocked(key)) {
		irq_start(state, &interval);
	}
	spin_hold_start(state, &interval);
#endif /* CONFIG_SMP */
}

#ifdef CONFIG_SMP
Z_CRITICAL_SECTION_MONITOR_HOOK void
z_crit_hook_spin_abort(const struct k_spinlock *lock, unsigned int key)
{
	if (unlikely(!monitor_ready)) {
		return;
	}

	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	if (state->pending_active && state->pending.spinlock == lock) {
		state->pending_active = false;
	}

	if (arch_irq_unlocked(key)) {
		irq_end(state, k_cycle_get_32());
	}
}

#endif /* CONFIG_SMP */

Z_CRITICAL_SECTION_MONITOR_INLINE void
spin_released_at(struct z_critical_section_monitor_cpu *state, const struct k_spinlock *lock,
		 uint32_t now)
{
	unsigned int count = state->held_count;

	/* Usually the last entry matches, but z_pend_curr() releases out of order. */
	for (unsigned int i = count; i > 0U; --i) {
		struct z_critical_section_monitor_interval *held = &state->held[i - 1U];

		if (held->spinlock != lock) {
			continue;
		}

		record_max(state, Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD,
			   now - held->start_cycles, held);
		/* Keep acquisition order so normal nested releases stay on the fast path. */
		for (unsigned int j = i; j < count; ++j) {
			state->held[j - 1U] = state->held[j];
		}
		state->held_count = count - 1U;
		return;
	}
	/* A lock acquired while all tracking slots were in use has no record. */
}

Z_CRITICAL_SECTION_MONITOR_HOOK void
z_crit_hook_spin_released(const struct k_spinlock *lock)
{
	if (unlikely(!monitor_ready)) {
		return;
	}

	uint32_t now = k_cycle_get_32();
	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	spin_released_at(state, lock, now);
}

Z_CRITICAL_SECTION_MONITOR_HOOK void
z_crit_hook_spin_unlocked(const struct k_spinlock *lock, unsigned int key)
{
	if (unlikely(!monitor_ready)) {
		return;
	}

	uint32_t now = k_cycle_get_32();
	struct z_critical_section_monitor_cpu *state = current_cpu_state();

	spin_released_at(state, lock, now);

	if (arch_irq_unlocked(key)) {
		irq_end(state, now);
	}
}

Z_CRITICAL_SECTION_MONITOR_FN int
z_critical_section_monitor_stats_get(unsigned int cpu,
				     struct z_critical_section_monitor_stats *stats)
{
	struct z_critical_section_monitor_cpu *state;
#ifdef CONFIG_SMP
	atomic_val_t sequence;
#else
	unsigned int key;
#endif

	if (stats == NULL || cpu >= arch_num_cpus()) {
		return -EINVAL;
	}

	state = &critical_section_monitor_cpus[cpu];

#ifdef CONFIG_SMP
	/* Another CPU may stop in the middle of an update; limit retries. */
	for (unsigned int attempt = 0U; attempt < SNAPSHOT_MAX_ATTEMPTS; ++attempt) {
		sequence = atomic_get(&state->sequence);
		if ((sequence & 1) != 0) {
			continue;
		}

		for (size_t i = 0; i < ARRAY_SIZE(stats->max); ++i) {
			const struct z_critical_section_monitor_record *src = &state->stats.max[i];
			struct z_critical_section_monitor_record *dest = &stats->max[i];

			dest->caller = STATS_LOAD(&src->caller);
			dest->spinlock = STATS_LOAD(&src->spinlock);
			dest->thread = STATS_LOAD(&src->thread);
			dest->cycles = STATS_LOAD(&src->cycles);
			dest->in_isr = STATS_LOAD(&src->in_isr);
		}
		stats->spinlock_tracking_overflows =
			STATS_LOAD(&state->stats.spinlock_tracking_overflows);

		/* Read all stats fields before checking the sequence again. */
		__atomic_thread_fence(__ATOMIC_ACQUIRE);
		if (sequence == atomic_get(&state->sequence)) {
			return 0;
		}
	}
	return -EAGAIN;
#else
	key = arch_irq_lock();
	memcpy(stats, &state->stats, sizeof(*stats));
	arch_irq_unlock(key);
	return 0;
#endif
}

Z_CRITICAL_SECTION_MONITOR_FN void z_critical_section_monitor_stats_reset(void)
{
	const struct z_critical_section_monitor_record empty = {0};
	struct z_critical_section_monitor_cpu *state;
	unsigned int key = arch_irq_lock();

	state = current_cpu_state();
	stats_write_begin(state);
	for (size_t i = 0; i < ARRAY_SIZE(state->stats.max); ++i) {
		record_store(&state->stats.max[i], &empty);
	}
	STATS_STORE(&state->stats.spinlock_tracking_overflows, 0U);
	stats_write_end(state);

	arch_irq_unlock(key);
}
