/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tracing_cpu_stats.h>
#include <sys/printk.h>
#include <kernel_internal.h>
#include <ksched.h>

enum cpu_state {
	CPU_STATE_IDLE,
	CPU_STATE_NON_IDLE,
	CPU_STATE_SCHEDULER
};

static enum cpu_state last_cpu_state = CPU_STATE_SCHEDULER;
static enum cpu_state cpu_state_before_interrupts;

static uint32_t last_time;
static struct cpu_stats stats_hw_tick;
static int nested_interrupts;
static struct k_thread *current_thread;

void update_counter(volatile uint64_t *cnt)
{
	uint32_t time = k_cycle_get_32();

	if (time >= last_time) {
		(*cnt) += (time - last_time);
	} else {
		(*cnt) += (UINT32_MAX - last_time + 1 + time);
	}
	last_time = time;
}

static void cpu_stats_update_counters(void)
{
	switch (last_cpu_state) {
	case CPU_STATE_IDLE:
		update_counter(&stats_hw_tick.idle);
		break;

	case CPU_STATE_NON_IDLE:
		update_counter(&stats_hw_tick.non_idle);
		break;

	case CPU_STATE_SCHEDULER:
		update_counter(&stats_hw_tick.sched);
		break;

	default:
		/* Invalid CPU state */
		__ASSERT_NO_MSG(false);
		break;
	}
}

void cpu_stats_get_ns(struct cpu_stats *cpu_stats_ns)
{
	int key = irq_lock();

	cpu_stats_update_counters();
	cpu_stats_ns->idle = k_cyc_to_ns_floor64(stats_hw_tick.idle);
	cpu_stats_ns->non_idle = k_cyc_to_ns_floor64(stats_hw_tick.non_idle);
	cpu_stats_ns->sched = k_cyc_to_ns_floor64(stats_hw_tick.sched);
	irq_unlock(key);
}

uint32_t cpu_stats_non_idle_and_sched_get_percent(void)
{
	int key = irq_lock();

	cpu_stats_update_counters();
	irq_unlock(key);
	return ((stats_hw_tick.non_idle + stats_hw_tick.sched) * 100) /
		(stats_hw_tick.idle + stats_hw_tick.non_idle +
		 stats_hw_tick.sched);
}

void cpu_stats_reset_counters(void)
{
	int key = irq_lock();

	stats_hw_tick.idle = 0;
	stats_hw_tick.non_idle = 0;
	stats_hw_tick.sched = 0;
	last_time = k_cycle_get_32();
	irq_unlock(key);
}

void sys_trace_thread_switched_in(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts == 0);

	cpu_stats_update_counters();
	current_thread = k_current_get();
	if (z_is_idle_thread_object(current_thread)) {
		last_cpu_state = CPU_STATE_IDLE;
	} else {
		last_cpu_state = CPU_STATE_NON_IDLE;
	}
	irq_unlock(key);
}

void sys_trace_thread_switched_out(void)
{
	int key = irq_lock();

	__ASSERT_NO_MSG(nested_interrupts == 0);
	__ASSERT_NO_MSG(!current_thread || (current_thread == k_current_get()));

	cpu_stats_update_counters();
	last_cpu_state = CPU_STATE_SCHEDULER;
	irq_unlock(key);
}

void sys_trace_isr_enter(void)
{
	int key = irq_lock();

	if (nested_interrupts == 0) {
		cpu_stats_update_counters();
		cpu_state_before_interrupts = last_cpu_state;
		last_cpu_state = CPU_STATE_NON_IDLE;
	}
	nested_interrupts++;
	irq_unlock(key);
}

void sys_trace_isr_exit(void)
{
	int key = irq_lock();

	nested_interrupts--;
	if (nested_interrupts == 0) {
		cpu_stats_update_counters();
		last_cpu_state = cpu_state_before_interrupts;
	}
	irq_unlock(key);
}

void sys_trace_idle(void)
{
}

#ifdef CONFIG_TRACING_CPU_STATS_LOG
static struct k_delayed_work cpu_stats_log;

static void cpu_stats_display(void)
{
	printk("CPU usage: %u\n", cpu_stats_non_idle_and_sched_get_percent());
}

static void cpu_stats_log_fn(struct k_work *item)
{
	cpu_stats_display();
	cpu_stats_reset_counters();
	k_delayed_work_submit(&cpu_stats_log,
			      K_MSEC(CONFIG_TRACING_CPU_STATS_INTERVAL));
}

static int cpu_stats_log_init(const struct device *dev)
{
	k_delayed_work_init(&cpu_stats_log, cpu_stats_log_fn);
	k_delayed_work_submit(&cpu_stats_log,
			      K_MSEC(CONFIG_TRACING_CPU_STATS_INTERVAL));

	return 0;
}

SYS_INIT(cpu_stats_log_init, APPLICATION, 0);
#endif /* CONFIG_TRACING_CPU_STATS_LOG */
