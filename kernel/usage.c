/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/timing/timing.h>
#include <ksched.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/check.h>

/* Need one of these for this to work */
#if !defined(CONFIG_USE_SWITCH) && !defined(CONFIG_INSTRUMENT_THREAD_SWITCHING)
#error "No data backend configured for CONFIG_SCHED_THREAD_USAGE"
#endif /* !CONFIG_USE_SWITCH && !CONFIG_INSTRUMENT_THREAD_SWITCHING */

static struct k_spinlock usage_lock;

static uint32_t usage_now(void)
{
	uint32_t now;

#ifdef CONFIG_THREAD_RUNTIME_STATS_USE_TIMING_FUNCTIONS
	now = (uint32_t)timing_counter_get();
#else
	now = k_cycle_get_32();
#endif /* CONFIG_THREAD_RUNTIME_STATS_USE_TIMING_FUNCTIONS */

	/* Edge case: we use a zero as a null ("stop() already called") */
	return (now == 0) ? 1 : now;
}

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
static void sched_cpu_update_usage(struct _cpu *cpu, uint32_t cycles)
{
	if (!cpu->usage->track_usage) {
		return;
	}

	if (cpu->current != cpu->idle_thread) {
		cpu->usage->total += cycles;

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		cpu->usage->current += cycles;

		if (cpu->usage->longest < cpu->usage->current) {
			cpu->usage->longest = cpu->usage->current;
		}
	} else {
		cpu->usage->current = 0;
		cpu->usage->num_windows++;
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
	}
}
#else
#define sched_cpu_update_usage(cpu, cycles)   do { } while (0)
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */

static void sched_thread_update_usage(struct k_thread *thread, uint32_t cycles)
{
	thread->base.usage.total += cycles;

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	thread->base.usage.current += cycles;

	if (thread->base.usage.longest < thread->base.usage.current) {
		thread->base.usage.longest = thread->base.usage.current;
	}
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
}

void z_sched_usage_start(struct k_thread *thread)
{
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	k_spinlock_key_t  key;

	key = k_spin_lock(&usage_lock);

	_current_cpu->usage0 = usage_now();   /* Always update */

	if (thread->base.usage.track_usage) {
		thread->base.usage.num_windows++;
		thread->base.usage.current = 0;
	}

	k_spin_unlock(&usage_lock, key);
#else
	/* One write through a volatile pointer doesn't require
	 * synchronization as long as _usage() treats it as volatile
	 * (we can't race with _stop() by design).
	 */

	_current_cpu->usage0 = usage_now();
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
}

void z_sched_usage_stop(void)
{
	k_spinlock_key_t k   = k_spin_lock(&usage_lock);

	struct _cpu     *cpu = _current_cpu;

	uint32_t u0 = cpu->usage0;

	if (u0 != 0) {
		uint32_t cycles = usage_now() - u0;

		if (cpu->current->base.usage.track_usage) {
			sched_thread_update_usage(cpu->current, cycles);
		}

		sched_cpu_update_usage(cpu, cycles);
	}

	cpu->usage0 = 0;
	k_spin_unlock(&usage_lock, k);
}

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
void z_sched_cpu_usage(uint8_t cpu_id, struct k_thread_runtime_stats *stats)
{
	k_spinlock_key_t  key;
	struct _cpu *cpu;

	key = k_spin_lock(&usage_lock);
	cpu = _current_cpu;


	if (&_kernel.cpus[cpu_id] == cpu) {
		uint32_t  now = usage_now();
		uint32_t cycles = now - cpu->usage0;

		/*
		 * Getting stats for the current CPU. Update both its
		 * current thread stats and the CPU stats as the CPU's
		 * [usage0] field will also get updated. This keeps all
		 * that information up-to-date.
		 */

		if (cpu->current->base.usage.track_usage) {
			sched_thread_update_usage(cpu->current, cycles);
		}

		sched_cpu_update_usage(cpu, cycles);

		cpu->usage0 = now;
	}

	stats->total_cycles     = cpu->usage->total;
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	stats->current_cycles   = cpu->usage->current;
	stats->peak_cycles      = cpu->usage->longest;

	if (cpu->usage->num_windows == 0) {
		stats->average_cycles = 0;
	} else {
		stats->average_cycles = stats->total_cycles /
					cpu->usage->num_windows;
	}
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */

	stats->idle_cycles =
		_kernel.cpus[cpu_id].idle_thread->base.usage.total;

	stats->execution_cycles = stats->total_cycles + stats->idle_cycles;

	k_spin_unlock(&usage_lock, key);
}
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */

void z_sched_thread_usage(struct k_thread *thread,
			  struct k_thread_runtime_stats *stats)
{
	struct _cpu *cpu;
	k_spinlock_key_t  key;

	key = k_spin_lock(&usage_lock);
	cpu = _current_cpu;


	if (thread == cpu->current) {
		uint32_t now = usage_now();
		uint32_t cycles = now - cpu->usage0;

		/*
		 * Getting stats for the current thread. Update both the
		 * current thread stats and its CPU stats as the CPU's
		 * [usage0] field will also get updated. This keeps all
		 * that information up-to-date.
		 */

		if (thread->base.usage.track_usage) {
			sched_thread_update_usage(thread, cycles);
		}

		sched_cpu_update_usage(cpu, cycles);

		cpu->usage0 = now;
	}

	stats->execution_cycles = thread->base.usage.total;
	stats->total_cycles     = thread->base.usage.total;

	/* Copy-out the thread's usage stats */

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	stats->current_cycles = thread->base.usage.current;
	stats->peak_cycles    = thread->base.usage.longest;

	if (thread->base.usage.num_windows == 0) {
		stats->average_cycles = 0;
	} else {
		stats->average_cycles = stats->total_cycles /
					thread->base.usage.num_windows;
	}
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
	stats->idle_cycles = 0;
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */

	k_spin_unlock(&usage_lock, key);
}

#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
int k_thread_runtime_stats_enable(k_tid_t  thread)
{
	k_spinlock_key_t  key;

	CHECKIF(thread == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&usage_lock);

	if (!thread->base.usage.track_usage) {
		thread->base.usage.track_usage = true;
		thread->base.usage.num_windows++;
		thread->base.usage.current = 0;
	}

	k_spin_unlock(&usage_lock, key);

	return 0;
}

int k_thread_runtime_stats_disable(k_tid_t  thread)
{
	k_spinlock_key_t key;

	CHECKIF(thread == NULL) {
		return -EINVAL;
	}

	key = k_spin_lock(&usage_lock);
	struct _cpu *cpu = _current_cpu;

	if (thread->base.usage.track_usage) {
		thread->base.usage.track_usage = false;

		if (thread == cpu->current) {
			uint32_t cycles = usage_now() - cpu->usage0;

			sched_thread_update_usage(thread, cycles);
			sched_cpu_update_usage(cpu, cycles);
		}
	}

	k_spin_unlock(&usage_lock, key);

	return 0;
}
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */

#ifdef CONFIG_SCHED_THREAD_USAGE_ALL
void k_sys_runtime_stats_enable(void)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&usage_lock);

	if (_current_cpu->usage->track_usage) {

		/*
		 * Usage tracking is already enabled on the current CPU
		 * and thus on all other CPUs (if applicable). There is
		 * nothing left to do.
		 */

		k_spin_unlock(&usage_lock, key);
		return;
	}

	/* Enable gathering of runtime stats on each CPU */

	unsigned int num_cpus = arch_num_cpus();

	for (uint8_t i = 0; i < num_cpus; i++) {
		_kernel.cpus[i].usage->track_usage = true;
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
		_kernel.cpus[i].usage->num_windows++;
		_kernel.cpus[i].usage->current = 0;
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
	}

	k_spin_unlock(&usage_lock, key);
}

void k_sys_runtime_stats_disable(void)
{
	struct _cpu *cpu;
	k_spinlock_key_t key;

	key = k_spin_lock(&usage_lock);

	if (!_current_cpu->usage->track_usage) {

		/*
		 * Usage tracking is already disabled on the current CPU
		 * and thus on all other CPUs (if applicable). There is
		 * nothing left to do.
		 */

		k_spin_unlock(&usage_lock, key);
		return;
	}

	uint32_t now = usage_now();

	unsigned int num_cpus = arch_num_cpus();

	for (uint8_t i = 0; i < num_cpus; i++) {
		cpu = &_kernel.cpus[i];
		if (cpu->usage0 != 0) {
			sched_cpu_update_usage(cpu, now - cpu->usage0);
		}
		cpu->usage->track_usage = false;
	}

	k_spin_unlock(&usage_lock, key);
}
#endif /* CONFIG_SCHED_THREAD_USAGE_ALL */

#ifdef CONFIG_OBJ_CORE_STATS_THREAD
int z_thread_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&usage_lock);
	memcpy(stats, obj_core->stats, sizeof(struct k_cycle_stats));
	k_spin_unlock(&usage_lock, key);

	return 0;
}

int z_thread_stats_query(struct k_obj_core *obj_core, void *stats)
{
	struct k_thread *thread;

	thread = CONTAINER_OF(obj_core, struct k_thread, obj_core);

	z_sched_thread_usage(thread, stats);

	return 0;
}

int z_thread_stats_reset(struct k_obj_core *obj_core)
{
	k_spinlock_key_t  key;
	struct k_cycle_stats  *stats;
	struct k_thread *thread;

	thread = CONTAINER_OF(obj_core, struct k_thread, obj_core);
	key = k_spin_lock(&usage_lock);
	stats = obj_core->stats;

	stats->total = 0ULL;
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	stats->current = 0ULL;
	stats->longest = 0ULL;
	stats->num_windows = (thread->base.usage.track_usage) ?  1U : 0U;
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */

	if (thread != _current_cpu->current) {

		/*
		 * If the thread is not running, there is nothing else to do.
		 * If the thread is running on another core, then it is not
		 * safe to do anything else but unlock and return (and pretend
		 * that its stats were reset at the start of its execution
		 * window.
		 */

		k_spin_unlock(&usage_lock, key);

		return 0;
	}

	/* Update the current CPU stats. */

	uint32_t now = usage_now();
	uint32_t cycles = now - _current_cpu->usage0;

	sched_cpu_update_usage(_current_cpu, cycles);

	_current_cpu->usage0 = now;

	k_spin_unlock(&usage_lock, key);

	return 0;
}

int z_thread_stats_disable(struct k_obj_core *obj_core)
{
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	struct k_thread *thread;

	thread = CONTAINER_OF(obj_core, struct k_thread, obj_core);

	return k_thread_runtime_stats_disable(thread);
#else
	return -ENOTSUP;
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
}

int z_thread_stats_enable(struct k_obj_core *obj_core)
{
#ifdef CONFIG_SCHED_THREAD_USAGE_ANALYSIS
	struct k_thread *thread;

	thread = CONTAINER_OF(obj_core, struct k_thread, obj_core);

	return k_thread_runtime_stats_enable(thread);
#else
	return -ENOTSUP;
#endif /* CONFIG_SCHED_THREAD_USAGE_ANALYSIS */
}
#endif /* CONFIG_OBJ_CORE_STATS_THREAD */

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
int z_cpu_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&usage_lock);
	memcpy(stats, obj_core->stats, sizeof(struct k_cycle_stats));
	k_spin_unlock(&usage_lock, key);

	return 0;
}

int z_cpu_stats_query(struct k_obj_core *obj_core, void *stats)
{
	struct _cpu  *cpu;

	cpu = CONTAINER_OF(obj_core, struct _cpu, obj_core);

	z_sched_cpu_usage(cpu->id, stats);

	return 0;
}
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */

#ifdef CONFIG_OBJ_CORE_STATS_SYSTEM
int z_kernel_stats_raw(struct k_obj_core *obj_core, void *stats)
{
	k_spinlock_key_t  key;

	key = k_spin_lock(&usage_lock);
	memcpy(stats, obj_core->stats,
	       CONFIG_MP_MAX_NUM_CPUS * sizeof(struct k_cycle_stats));
	k_spin_unlock(&usage_lock, key);

	return 0;
}

int z_kernel_stats_query(struct k_obj_core *obj_core, void *stats)
{
	ARG_UNUSED(obj_core);

	return k_thread_runtime_stats_all_get(stats);
}
#endif /* CONFIG_OBJ_CORE_STATS_SYSTEM */
