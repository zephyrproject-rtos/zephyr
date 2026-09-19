/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include <kspinlock.h>
#include <critical_section_monitor.h>

BUILD_ASSERT(sizeof(struct k_spinlock) > 0U, "Tracked locks need distinct addresses");

static struct {
	bool enabled;
	uint32_t cycles;
} test_clocks[CONFIG_MP_MAX_NUM_CPUS];

uint32_t __real_sys_clock_cycle_get_32(void);
uint32_t __wrap_sys_clock_cycle_get_32(void);

uint32_t __wrap_sys_clock_cycle_get_32(void)
{
	unsigned int key = arch_irq_lock();
	unsigned int cpu = CPU_ID;
	uint32_t cycles = test_clocks[cpu].enabled ? test_clocks[cpu].cycles
						   : __real_sys_clock_cycle_get_32();

	arch_irq_unlock(key);
	return cycles;
}

/* The caller keeps raw local IRQs locked until the real clock is restored. */
static void set_test_clock(uint32_t cycles)
{
	unsigned int cpu = CPU_ID;

	test_clocks[cpu].cycles = cycles;
	test_clocks[cpu].enabled = true;
}

static void restore_clock(void)
{
	test_clocks[CPU_ID].enabled = false;
}

ZTEST(kernel_critical_section_monitor, test_exact_irq_timing)
{
	struct z_critical_section_monitor_stats nested;
	struct z_critical_section_monitor_stats wrapped;
	struct z_critical_section_monitor_stats tied;
	struct z_critical_section_monitor_stats restarted;
	unsigned int key = arch_irq_lock();
	unsigned int inner_key = arch_irq_lock();
	unsigned int cpu = CPU_ID;

	z_critical_section_monitor_stats_reset();
	set_test_clock(UINT32_MAX - 10U);
	z_critical_section_monitor_irq_start(key, NULL, 1U);
	set_test_clock(0U);
	z_critical_section_monitor_irq_start(inner_key, NULL, 2U);
	z_critical_section_monitor_irq_end(inner_key);
	(void)z_critical_section_monitor_stats_get(cpu, &nested);
	/* Reset must preserve the active interval, even across a counter wrap. */
	z_critical_section_monitor_stats_reset();
	set_test_clock(15U);
	z_critical_section_monitor_irq_end(key);
	(void)z_critical_section_monitor_stats_get(cpu, &wrapped);

	set_test_clock(100U);
	z_critical_section_monitor_irq_start(key, NULL, 2U);
	set_test_clock(126U);
	z_critical_section_monitor_irq_end(key);
	(void)z_critical_section_monitor_stats_get(cpu, &tied);

	/* A hardware-enabled entry replaces any stale, unpaired interval. */
	set_test_clock(200U);
	z_critical_section_monitor_irq_start(key, NULL, 2U);
	set_test_clock(300U);
	z_critical_section_monitor_irq_start(key, NULL, 3U);
	set_test_clock(340U);
	z_critical_section_monitor_irq_end(key);
	(void)z_critical_section_monitor_stats_get(cpu, &restarted);
	restore_clock();
	arch_irq_unlock(key);

	zassert_equal(nested.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles, 0U);
	zassert_equal(wrapped.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles, 26U);
	zassert_equal(wrapped.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].caller, 1U);
	zassert_equal(tied.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles, 26U);
	zassert_equal(tied.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].caller, 1U);
	zassert_equal(restarted.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles, 40U);
	zassert_equal(restarted.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].caller, 3U);
}

ZTEST(kernel_critical_section_monitor, test_exact_spin_tracking_overflow_recovery)
{
	struct k_spinlock locks[CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS + 1] = {0};
	k_spinlock_key_t keys[ARRAY_SIZE(locks)];
	struct z_critical_section_monitor_stats overflow;
	struct z_critical_section_monitor_stats recovered;
	unsigned int key = arch_irq_lock();
	unsigned int cpu = CPU_ID;

	z_critical_section_monitor_stats_reset();
	set_test_clock(0U);
	z_critical_section_monitor_irq_start(key, &locks[0], 1U);
	for (size_t i = 0; i < ARRAY_SIZE(locks); ++i) {
		set_test_clock(i * 10U);
		keys[i] = k_spin_lock(&locks[i]);
	}

	set_test_clock(1000U);
	for (size_t i = ARRAY_SIZE(locks); i > 0; --i) {
		k_spin_unlock(&locks[i - 1U], keys[i - 1U]);
	}
	z_critical_section_monitor_irq_end(key);
	(void)z_critical_section_monitor_stats_get(cpu, &overflow);

	z_critical_section_monitor_stats_reset();
	set_test_clock(2000U);
	keys[0] = k_spin_lock(&locks[0]);
	set_test_clock(2050U);
	k_spin_unlock(&locks[0], keys[0]);
	(void)z_critical_section_monitor_stats_get(cpu, &recovered);
	restore_clock();
	arch_irq_unlock(key);

	zassert_equal(overflow.spinlock_tracking_overflows, 1U);
	zassert_equal(overflow.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles, 1000U);
	zassert_equal(overflow.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 1000U);
	zassert_equal_ptr(overflow.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].spinlock,
			  &locks[0]);
	zassert_equal(recovered.spinlock_tracking_overflows, 0U);
	zassert_equal(recovered.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 50U);
	zassert_equal_ptr(recovered.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].spinlock,
			  &locks[0]);
}

ZTEST(kernel_critical_section_monitor, test_exact_crossed_spin_release)
{
	struct k_spinlock locks[3] = {0};
	struct z_critical_section_monitor_stats middle;
	struct z_critical_section_monitor_stats last;
	unsigned int key = arch_irq_lock();
	unsigned int cpu = CPU_ID;

	z_critical_section_monitor_stats_reset();
	set_test_clock(100U);
	(void)k_spin_lock(&locks[0]);
	set_test_clock(200U);
	(void)k_spin_lock(&locks[1]);
	set_test_clock(300U);
	(void)k_spin_lock(&locks[2]);
	set_test_clock(400U);
	k_spin_release(&locks[1]);
	(void)z_critical_section_monitor_stats_get(cpu, &middle);
	set_test_clock(450U);
	k_spin_release(&locks[0]);
	set_test_clock(600U);
	k_spin_release(&locks[2]);
	(void)z_critical_section_monitor_stats_get(cpu, &last);
	restore_clock();
	arch_irq_unlock(key);

	zassert_equal(middle.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles,
		      CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS > 1 ? 200U : 0U);
	zassert_equal(last.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 350U);
	zassert_equal_ptr(last.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].spinlock,
			  &locks[0]);
	zassert_equal(last.spinlock_tracking_overflows,
		      3U - MIN(3U, CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS));
}

ZTEST(kernel_critical_section_monitor, test_exact_slot_reuse_after_overflow)
{
	struct k_spinlock locks[CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS + 2] = {0};
	struct z_critical_section_monitor_stats released;
	struct z_critical_section_monitor_stats untracked;
	struct z_critical_section_monitor_stats reused;
	unsigned int capacity = CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS;
	unsigned int key = arch_irq_lock();
	unsigned int cpu = CPU_ID;

	z_critical_section_monitor_stats_reset();
	for (unsigned int i = 0U; i <= capacity; ++i) {
		set_test_clock(100U + i * 10U);
		(void)k_spin_lock(&locks[i]);
	}
	/* Free a tracked slot while the overflowed lock remains held. */
	set_test_clock(500U);
	k_spin_release(&locks[0]);
	(void)z_critical_section_monitor_stats_get(cpu, &released);
	z_critical_section_monitor_stats_reset();
	set_test_clock(1000U);
	(void)k_spin_lock(&locks[capacity + 1U]);
	set_test_clock(2000U);
	k_spin_release(&locks[capacity]);
	(void)z_critical_section_monitor_stats_get(cpu, &untracked);
	set_test_clock(2100U);
	k_spin_release(&locks[capacity + 1U]);
	(void)z_critical_section_monitor_stats_get(cpu, &reused);
	for (unsigned int i = capacity; i > 1U; --i) {
		k_spin_release(&locks[i - 1U]);
	}
	restore_clock();
	arch_irq_unlock(key);

	zassert_equal(released.spinlock_tracking_overflows, 1U);
	zassert_equal(released.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 400U);
	zassert_equal_ptr(released.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].spinlock,
			  &locks[0]);
	zassert_equal(untracked.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 0U);
	zassert_equal(reused.spinlock_tracking_overflows, 0U);
	zassert_equal(reused.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 1100U);
	zassert_equal_ptr(reused.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].spinlock,
			  &locks[capacity + 1U]);
}

#define PEND_STACK_SIZE (1536 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_DEFINE(pend_stack, PEND_STACK_SIZE);
K_SEM_DEFINE(pend_sem, 0, 1);
static struct k_thread pend_thread;
static struct pend_capture {
	struct k_thread *origin;
	struct k_spinlock *object_lock;
	const struct k_spinlock *next_release;
	struct z_critical_section_monitor_stats object;
	struct z_critical_section_monitor_stats scheduler;
} pend_captures[CONFIG_MP_MAX_NUM_CPUS];

int __real_z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key, _wait_q_t *wait_q,
		       k_timeout_t timeout);
int __wrap_z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key, _wait_q_t *wait_q,
		       k_timeout_t timeout);
void __real_z_critical_section_monitor_spin_released(const struct k_spinlock *lock);
void __wrap_z_critical_section_monitor_spin_released(const struct k_spinlock *lock);

int __wrap_z_pend_curr(struct k_spinlock *lock, k_spinlock_key_t key, _wait_q_t *wait_q,
		       k_timeout_t timeout)
{
	struct pend_capture *capture = &pend_captures[CPU_ID];

	if (capture->origin == _current) {
		capture->object_lock = lock;
		capture->next_release = lock;
		capture->origin = NULL;
	}
	return __real_z_pend_curr(lock, key, wait_q, timeout);
}

void __wrap_z_critical_section_monitor_spin_released(const struct k_spinlock *lock)
{
	struct pend_capture *capture = &pend_captures[CPU_ID];
	struct z_critical_section_monitor_stats *stats = NULL;

	if (lock == capture->next_release) {
		bool object = lock == capture->object_lock;

		stats = object ? &capture->object : &capture->scheduler;
		capture->next_release = object ? &_sched_spinlock : NULL;
		/* Isolate each release from unrelated maxima without changing held locks. */
		z_critical_section_monitor_stats_reset();
		k_busy_wait(1U);
	}
	__real_z_critical_section_monitor_spin_released(lock);
	if (stats != NULL) {
		(void)z_critical_section_monitor_stats_get(CPU_ID, stats);
	}
}

static void wake_pended_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sem_give(&pend_sem);
}

ZTEST(kernel_critical_section_monitor, test_blocking_pend_tracks_both_locks)
{
	const struct z_critical_section_monitor_record *record;
	struct k_thread *origin = k_current_get();
	struct pend_capture *capture;
	unsigned int key;
	unsigned int cpu;
	k_tid_t waker;
	int result;

	k_sched_lock();
	key = arch_irq_lock();
	cpu = CPU_ID;
	capture = &pend_captures[cpu];
	*capture = (struct pend_capture){.origin = origin};
	arch_irq_unlock(key);
	waker = k_thread_create(&pend_thread, pend_stack, K_THREAD_STACK_SIZEOF(pend_stack),
				wake_pended_thread, NULL, NULL, NULL,
				k_thread_priority_get(origin) + 1, 0, K_FOREVER);
#ifdef CONFIG_SMP
	zassert_ok(k_thread_cpu_pin(waker, cpu));
#endif
	k_thread_start(waker);
	result = k_sem_take(&pend_sem, K_FOREVER);
	k_sched_unlock();
	zassert_ok(k_thread_join(waker, K_SECONDS(1)));

	zassert_ok(result);
	zassert_not_null(capture->object_lock, "did not block in z_pend_curr");
	zassert_is_null(capture->next_release, "did not observe both pend releases");
	record = &capture->object.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD];
	zassert_true(record->cycles > 0U);
	zassert_equal_ptr(record->spinlock, capture->object_lock);
	zassert_equal_ptr(record->thread, origin);
	if (CONFIG_CRITICAL_SECTION_MONITOR_MAX_SPINLOCKS > 1) {
		record = &capture->scheduler.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD];
		zassert_true(record->cycles > 0U);
		zassert_equal_ptr(record->spinlock, &_sched_spinlock);
		zassert_equal_ptr(record->thread, origin);
	}
}

#ifdef CONFIG_SMP

#define SNAPSHOT_ITERATIONS 10000U
#define SNAPSHOT_STACK_SIZE (1536 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_DEFINE(writer_stack, SNAPSHOT_STACK_SIZE);
static struct k_thread writer_thread;
static struct k_spinlock record_locks[2];
static atomic_t writer_ready;
static atomic_t reader_ready;
static atomic_t writer_done;
static atomic_t reader_done;

static void snapshot_writer(void *p1, void *p2, void *p3)
{
	unsigned int key = arch_irq_lock();

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	z_critical_section_monitor_stats_reset();
	atomic_set(&writer_ready, 1);
	while (atomic_get(&reader_ready) == 0) {
		arch_spin_relax();
	}

	for (uint32_t i = 1U; i <= SNAPSHOT_ITERATIONS; ++i) {
		if ((i % 64U) == 0U) {
			z_critical_section_monitor_stats_reset();
		}
		set_test_clock(0U);
		z_critical_section_monitor_irq_start(key, &record_locks[i % 2U], i);
		set_test_clock(i);
		z_critical_section_monitor_irq_end(key);
	}

	atomic_set(&writer_done, 1);
	/* Keep unrelated scheduling/ISR records out until the reader finishes. */
	while (atomic_get(&reader_done) == 0) {
		arch_spin_relax();
	}
	restore_clock();
	arch_irq_unlock(key);
}

static bool valid_snapshot(const struct z_critical_section_monitor_stats *stats)
{
	const struct z_critical_section_monitor_record *record =
		&stats->max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED];

	if (stats->spinlock_tracking_overflows != 0U ||
	    stats->max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT].cycles != 0U ||
	    stats->max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles != 0U) {
		return false;
	}

	if (record->cycles == 0U) {
		return record->caller == 0U && record->spinlock == NULL && record->thread == NULL &&
		       !record->in_isr;
	}

	return record->cycles <= SNAPSHOT_ITERATIONS && record->caller == record->cycles &&
	       record->spinlock == &record_locks[record->cycles % 2U] &&
	       record->thread == &writer_thread && !record->in_isr;
}

ZTEST(kernel_critical_section_monitor, test_smp_remote_snapshot)
{
	struct z_critical_section_monitor_stats stats;
	unsigned int attempts = 0U;
	bool snapshot_failed = false;
	unsigned int cpu;
	k_tid_t writer;

	atomic_clear(&writer_ready);
	atomic_clear(&reader_ready);
	atomic_clear(&writer_done);
	atomic_clear(&reader_done);
	/* Keep this reader on one CPU and run the writer on another. */
	k_sched_lock();
	cpu = (arch_curr_cpu()->id + 1U) % arch_num_cpus();
	writer =
		k_thread_create(&writer_thread, writer_stack, K_THREAD_STACK_SIZEOF(writer_stack),
				snapshot_writer, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(writer, cpu));
	k_thread_start(writer);

	while (atomic_get(&writer_ready) == 0) {
		k_busy_wait(1U);
	}

	atomic_set(&reader_ready, 1);
	do {
		int ret = z_critical_section_monitor_stats_get(cpu, &stats);

		if ((ret == 0 && !valid_snapshot(&stats)) || (ret != 0 && ret != -EAGAIN)) {
			snapshot_failed = true;
		}
		++attempts;
	} while (atomic_get(&writer_done) == 0);

	if (z_critical_section_monitor_stats_get(cpu, &stats) != 0 || !valid_snapshot(&stats) ||
	    stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles != SNAPSHOT_ITERATIONS) {
		snapshot_failed = true;
	}
	atomic_set(&reader_done, 1);
	k_sched_unlock();
	zassert_ok(k_thread_join(writer, K_SECONDS(5)));
	zassert_false(snapshot_failed, "mixed publication/reset snapshot");
	zassert_true(attempts > 1U, "no concurrent snapshot attempts exercised");
}

#endif /* CONFIG_SMP */
