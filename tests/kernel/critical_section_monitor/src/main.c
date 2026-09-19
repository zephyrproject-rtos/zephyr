/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel/smp.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include <critical_section_monitor.h>

#define MEASURE_US 1000U

static struct k_spinlock measured_lock;

static void check_record(const struct z_critical_section_monitor_record *record,
			 const struct k_spinlock *spinlock, const struct k_thread *thread,
			 bool in_isr)
{
	zassert_true(record->cycles > 0U, "no duration was recorded");
	zassert_not_equal(record->caller, 0U, "call site was not captured");
	zassert_equal_ptr(record->spinlock, spinlock, "wrong spinlock");
	zassert_equal_ptr(record->thread, thread, "wrong thread");
	zassert_equal(record->in_isr, in_isr, "wrong execution context");
}

ZTEST(kernel_critical_section_monitor, test_nested_irq_lock_is_one_interval)
{
	struct z_critical_section_monitor_stats stats;
	const struct k_thread *thread = k_current_get();
	unsigned int outer_key;
	unsigned int inner_key;
	unsigned int cpu;

	outer_key = k_irq_lock();
	cpu = CPU_ID;
	z_critical_section_monitor_stats_reset();

	k_busy_wait(MEASURE_US / 2U);
	inner_key = irq_lock();
	k_busy_wait(MEASURE_US / 2U);
	irq_unlock(inner_key);
	zassert_ok(z_critical_section_monitor_stats_get(cpu, &stats));
	zassert_equal(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles, 0U);
	k_busy_wait(MEASURE_US / 2U);
	z_critical_section_monitor_irq_end(outer_key);
	zassert_ok(z_critical_section_monitor_stats_get(cpu, &stats));
	k_irq_unlock(outer_key);

	check_record(&stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED], NULL, thread, false);
	zassert_true(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles >=
		     k_us_to_cyc_floor32(MEASURE_US * 3U / 2U));
	zassert_equal(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles, 0U);
	zassert_equal(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT].cycles, 0U);
	zassert_equal(stats.spinlock_tracking_overflows, 0U);
}

ZTEST(kernel_critical_section_monitor, test_public_spin_unlock)
{
	struct z_critical_section_monitor_stats stats;
	unsigned int cpu;
	k_spinlock_key_t key;

	k_sched_lock();
	z_critical_section_monitor_stats_reset();
	key = k_spin_lock(&measured_lock);
	cpu = CPU_ID;
	k_busy_wait(MEASURE_US);
	k_spin_unlock(&measured_lock, key);
	zassert_ok(z_critical_section_monitor_stats_get(cpu, &stats));
	k_sched_unlock();

	check_record(&stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD], &measured_lock,
		     k_current_get(), false);
	check_record(&stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED], &measured_lock,
		     k_current_get(), false);
	zassert_true(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles >=
		     k_us_to_cyc_floor32(MEASURE_US));
	zassert_true(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles >=
		     stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles);
	zassert_equal(stats.spinlock_tracking_overflows, 0U);
#ifndef CONFIG_SMP
	zassert_equal(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT].cycles, 0U);
#endif
}

K_TIMER_DEFINE(idle_wakeup, NULL, NULL);

static struct z_critical_section_monitor_stats *idle_snapshots[CONFIG_MP_MAX_NUM_CPUS];

void __real_arch_cpu_atomic_idle(unsigned int key);
void __wrap_arch_cpu_atomic_idle(unsigned int key);
void __real_arch_cpu_idle(void);
void __wrap_arch_cpu_idle(void);

static void snapshot_before_idle(void)
{
	unsigned int cpu = CPU_ID;

	/* Capture before IRQs are enabled: unrelated records cannot replace it. */
	if (idle_snapshots[cpu] != NULL) {
		(void)z_critical_section_monitor_stats_get(cpu, idle_snapshots[cpu]);
		idle_snapshots[cpu] = NULL;
	}
}

void __wrap_arch_cpu_atomic_idle(unsigned int key)
{
	snapshot_before_idle();
	__real_arch_cpu_atomic_idle(key);
}

void __wrap_arch_cpu_idle(void)
{
	snapshot_before_idle();
	__real_arch_cpu_idle();
}

static void check_idle_handoff(bool atomic_idle)
{
	struct z_critical_section_monitor_stats stats = {0};
	k_spinlock_key_t key;
	unsigned int cpu;
	unsigned int irq_key;
	bool irqs_enabled;

	k_sched_lock();
	k_timer_start(&idle_wakeup, K_MSEC(10), K_NO_WAIT);
	key = k_spin_lock(&measured_lock);
	cpu = CPU_ID;
	z_critical_section_monitor_stats_reset();
	k_busy_wait(MEASURE_US);
	/* Do not carry an SMP global IRQ lock across idle. */
	k_spin_release(&measured_lock);
	idle_snapshots[cpu] = &stats;
	if (atomic_idle) {
		k_cpu_atomic_idle(key.key);
	} else {
		k_cpu_idle();
	}

	irq_key = arch_irq_lock();
	irqs_enabled = arch_irq_unlocked(irq_key);
	arch_irq_unlock(irq_key);
	k_timer_stop(&idle_wakeup);
	k_sched_unlock();

	zassert_true(irqs_enabled, "idle did not restore IRQs");
	check_record(&stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED], &measured_lock,
		     k_current_get(), false);
	zassert_true(stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED].cycles >=
		     k_us_to_cyc_floor32(MEASURE_US));
}

ZTEST(kernel_critical_section_monitor, test_atomic_idle_closes_irq_interval)
{
	check_idle_handoff(true);
}

ZTEST(kernel_critical_section_monitor, test_idle_closes_irq_interval)
{
	check_idle_handoff(false);
}

ZTEST(kernel_critical_section_monitor, test_stats_arguments)
{
	struct z_critical_section_monitor_stats stats;

	zassert_equal(z_critical_section_monitor_stats_get(arch_num_cpus(), &stats), -EINVAL);
	zassert_equal(z_critical_section_monitor_stats_get(0U, NULL), -EINVAL);
}

static struct z_critical_section_monitor_stats isr_stats;
static int isr_stats_result;

static void measure_from_isr(const void *parameter)
{
	struct k_spinlock *lock = (struct k_spinlock *)parameter;
	k_spinlock_key_t key;
	unsigned int cpu;

	key = k_spin_lock(lock);
	cpu = CPU_ID;
	z_critical_section_monitor_stats_reset();
	k_busy_wait(MEASURE_US);
	k_spin_release(lock);
	isr_stats_result = z_critical_section_monitor_stats_get(cpu, &isr_stats);
	arch_irq_unlock(key.key);
}

ZTEST(kernel_critical_section_monitor, test_isr_metadata)
{
	const struct k_thread *thread = k_current_get();

	isr_stats_result = -ENODATA;
	irq_offload(measure_from_isr, &measured_lock);

	zassert_ok(isr_stats_result);
	check_record(&isr_stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD], &measured_lock,
		     thread, true);
}

#ifndef CONFIG_SMP

#define HANDOFF_STACK_SIZE (1536 + CONFIG_TEST_EXTRA_STACK_SIZE)

K_THREAD_STACK_DEFINE(handoff_stack, HANDOFF_STACK_SIZE);

static struct k_thread handoff_thread;
static struct z_critical_section_monitor_stats handoff_stats;
static int handoff_stats_result;

static void snapshot_after_handoff(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	handoff_stats_result = z_critical_section_monitor_stats_get(CPU_ID, &handoff_stats);
}

ZTEST(kernel_critical_section_monitor, test_scheduler_handoff_attribution)
{
	const struct z_critical_section_monitor_record *record;
	k_tid_t origin = k_current_get();
	k_tid_t thread;

	thread = k_thread_create(&handoff_thread, handoff_stack,
				 K_THREAD_STACK_SIZEOF(handoff_stack), snapshot_after_handoff, NULL,
				 NULL, NULL, k_thread_priority_get(origin), 0, K_FOREVER);

	handoff_stats_result = -ENODATA;
	k_thread_start(thread);
	z_critical_section_monitor_stats_reset();

	/*
	 * Yielding transfers _current to handoff_thread before the scheduler
	 * spinlock is physically released. The record must retain origin.
	 */
	k_yield();

	zassert_ok(k_thread_join(thread, K_SECONDS(1)));
	zassert_ok(handoff_stats_result);

	record = &handoff_stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED];
	zassert_not_null(record->spinlock, "scheduler spinlock was not recorded");
	check_record(record, record->spinlock, origin, false);
}

#endif /* !CONFIG_SMP */

#ifdef CONFIG_SMP

#define WORKER_STACK_SIZE (1536 + CONFIG_TEST_EXTRA_STACK_SIZE)
K_THREAD_STACK_DEFINE(waiter_stack, WORKER_STACK_SIZE);

static struct k_thread waiter_thread;
static struct k_spinlock contention_lock;
static atomic_t holder_locked;
static atomic_t waiter_tried;
static struct z_critical_section_monitor_stats waiter_stats;
static int waiter_trylock_result;

static void waiter_fn(void *p1, void *p2, void *p3)
{
	k_spinlock_key_t try_key;
	unsigned int irq_key;
	unsigned int cpu;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (atomic_get(&holder_locked) == 0) {
		k_yield();
	}

	irq_key = arch_irq_lock();
	z_critical_section_monitor_stats_reset();
	waiter_trylock_result = k_spin_trylock(&contention_lock, &try_key);
	atomic_set(&waiter_tried, 1);
	(void)k_spin_lock(&contention_lock);
	cpu = CPU_ID;
	k_spin_release(&contention_lock);
	(void)z_critical_section_monitor_stats_get(cpu, &waiter_stats);
	arch_irq_unlock(irq_key);
}

ZTEST(kernel_critical_section_monitor, test_smp_spin_contention)
{
	struct z_critical_section_monitor_stats holder_stats;
	k_tid_t holder = k_current_get();
	k_spinlock_key_t key;
	unsigned int cpu;
	k_tid_t waiter;

	atomic_clear(&holder_locked);
	atomic_clear(&waiter_tried);
	waiter_trylock_result = 0;

	k_sched_lock();
	cpu = arch_curr_cpu()->id;
	waiter = k_thread_create(&waiter_thread, waiter_stack, K_THREAD_STACK_SIZEOF(waiter_stack),
				 waiter_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(waiter, (cpu + 1U) % arch_num_cpus()));
	k_thread_start(waiter);

	key = k_spin_lock(&contention_lock);
	z_critical_section_monitor_stats_reset();
	atomic_set(&holder_locked, 1);
	while (atomic_get(&waiter_tried) == 0) {
		arch_spin_relax();
	}
	k_busy_wait(MEASURE_US);
	k_spin_release(&contention_lock);
	z_critical_section_monitor_irq_end(key.key);
	(void)z_critical_section_monitor_stats_get(cpu, &holder_stats);
	arch_irq_unlock(key.key);
	k_sched_unlock();
	zassert_ok(k_thread_join(waiter, K_SECONDS(5)));

	zassert_equal(waiter_trylock_result, -EBUSY);
	check_record(&holder_stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD],
		     &contention_lock, holder, false);
	check_record(&waiter_stats.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT],
		     &contention_lock, &waiter_thread, false);
	zassert_equal(holder_stats.spinlock_tracking_overflows, 0U);
	zassert_equal(waiter_stats.spinlock_tracking_overflows, 0U);
}
#endif /* CONFIG_SMP */

static void *monitor_setup(void)
{
#if defined(CONFIG_SMP) && DT_PROP_OR(DT_PATH(cpus, cpu_1), zephyr_deferred_start, 0)
	k_smp_cpu_start(1, NULL, NULL);
#endif

	return NULL;
}

ZTEST_SUITE(kernel_critical_section_monitor, NULL, monitor_setup, NULL, NULL, NULL);
