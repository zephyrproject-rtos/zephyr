/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(power_sample, LOG_LEVEL_DBG);

static inline bool cpu_is_primary(uint32_t core_id)
{
	return core_id == 0;
}

int cpu_enable_core(int id)
{
	/* only called from single core, no RMW lock */
	__ASSERT_NO_MSG(cpu_is_primary(arch_proc_id()));

	if (arch_cpu_active(id)) {
		return 0;
	}

	if (pm_state_next_get(id)->state == PM_STATE_ACTIVE) {
		printk("k_smp_cpu_start(%d)\n", id);
		k_smp_cpu_start(id, NULL, NULL);
		return 0;
	}

	printk("k_smp_cpu_resume(%d)\n", id);
	k_smp_cpu_resume(id, NULL, NULL);

	return 0;
}

void cpu_disable_core(int id)
{
	/* only called from single core, no RMW lock */
	__ASSERT_NO_MSG(cpu_is_primary(arch_proc_id()));

	if (!arch_cpu_active(id)) {
		LOG_WRN("core %d is already disabled", id);
		goto out;
	}
#if defined(CONFIG_PM)
	/* TODO: before requesting core shut down check if it's not actively used */
	if (!pm_state_force(id, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0})) {
		LOG_ERR("failed to set PM_STATE_SOFT_OFF on core %d", id);
		goto out;
	}

	/* Primary core will be turn off by the host after it enter SOFT_OFF state */
	if (cpu_is_primary(id)) {
		goto out;
	}

	/* Broadcasting interrupts to other cores. */
	arch_sched_ipi();

	uint64_t timeout = k_cycle_get_64() + k_ms_to_cyc_ceil64(10000);

	/* Waiting for secondary core to enter idle state */
	printk("waiting for core %d to enter idle, timeout %llu, cycle %llu\n", id, timeout,
	       k_cycle_get_64());
	while (arch_cpu_active(id) && (k_cycle_get_64() < timeout)) {
		k_busy_wait(10000);
	}
	printk("done waiting for core %d, active? %d\n", id, arch_cpu_active(id));

	if (arch_cpu_active(id)) {
		printk("core %d did not enter idle state\n", id);
		goto out;
	}

	if (soc_adsp_halt_cpu(id) != 0) {
		printk("failed to disable core %d", id);
	}
#endif /* CONFIG_PM */
out:
	return;
}

int cpu_is_core_enabled(int id)
{
	return arch_cpu_active(id);
}

int cpu_enabled_cores(void)
{
	unsigned int i;
	int mask = 0;

	for (i = 0; i < CONFIG_MP_MAX_NUM_CPUS; i++) {
		if (arch_cpu_active(i)) {
			mask |= BIT(i);
		}
	}

	return mask;
}

#define NTHREADS   CONFIG_MP_MAX_NUM_CPUS
#define STACK_SIZE 1024
#define PRIORITY   7

static struct k_sem sems[NTHREADS];
static struct k_sem done[NTHREADS];
static struct k_thread threads[NTHREADS];
K_THREAD_STACK_ARRAY_DEFINE(tstack, NTHREADS, STACK_SIZE);

static void add(volatile uint32_t *x, const uint32_t val)
{
	*x += val;
}

/* This NOP macro generates at least 2 instructions. Notably if only nop.n is
 * used the compiler will remove duplicate nop.n instructions so a volatile add
 * is done to ensure this does not occur.
 */
#define NOP(i, val, ...)                                                                           \
	add(&val, i);                                                                              \
	__asm__ volatile("nop.n")

static bool cpu_active[NTHREADS];

static void do_work(void)
{
	volatile uint32_t v = 0;

	/* These LISTIFY macros will ensure some number of actual cpu operations
	 * will occur to simulate real work being done.
	 * Can be copied/pasted a few times as needed to generate more op codes.
	 */
	LISTIFY(4096, NOP, (;), v);
	LISTIFY(4096, NOP, (;), v);
}

void coreN_loop(void *arg0, void *arg1, void *arg2)
{
	uint32_t cnt = 0;
	uint32_t cpu = arch_curr_cpu()->id;

	while (true) {
		printk("core %d waiting on semaphore\n", cpu);
		k_sem_take(&sems[cpu],
			   K_FOREVER); /* put core X to sleep until semaphore is given */
		printk("core %d doing work\n", cpu);
		do_work();
		printk("core %d done\n", cpu);
		k_sem_give(&done[cpu]);
	}
}

const char *names[4] = {
	"core0_loop",
	"core1_loop",
	"core2_loop",
	"core3_loop",
};

void core0_loop(void *arg0, void *arg1, void *arg2)
{
	uint32_t cnt = 0;
	uint32_t cpu = arch_curr_cpu()->id;

	for (int i = 1; i < NTHREADS; i++) {
		k_thread_create(&threads[i], tstack[i], STACK_SIZE, coreN_loop, &sems[i], NULL,
				NULL, K_HIGHEST_THREAD_PRIO, 0, K_FOREVER);
		k_thread_cpu_pin(&threads[i], i);
		k_thread_name_set(&threads[i], names[i]);
		k_thread_start(&threads[i]);
	}

	while (true) {
		k_sem_take(&sems[cpu], K_FOREVER);
		for (int i = 1; i < NTHREADS; i++) {
			printk("enabling core %d\n", i);
			cpu_enable_core(i);
			k_busy_wait(1000);
			arch_sched_ipi();
			k_sem_give(&sems[i]);
		}
		do_work();
		for (int i = 1; i < NTHREADS; i++) {
			k_sem_take(&done[i], K_FOREVER);
			printk("disabling core %d\n", i);
			cpu_disable_core(i);
		}
		k_sem_give(&done[cpu]);
		printk("core %d thread %p loop %u\n", cpu, k_current_get(), cnt);
		cnt++;
	}
}

void timer_cb(struct k_timer *t)
{
	printk("core %d timer\n", arch_curr_cpu()->id);

	k_sem_give(&sems[0]);
}

K_TIMER_DEFINE(timer0, timer_cb, NULL);

#if defined(CONFIG_PM_POLICY_CUSTOM)

#define PLATFORM_PRIMARY_CORE_ID 0

const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	unsigned int num_cpu_states;
	const struct pm_state_info *cpu_states;

	num_cpu_states = pm_state_cpu_get_all(cpu, &cpu_states);

	printk("cpu %d current state %d, ticks %d\n", cpu, cpu_states[cpu].state, ticks);
	if (cpu == 0) {
		return NULL;
	}

	for (int i = num_cpu_states - 1; i >= 0; i--) {
		const struct pm_state_info *state = &cpu_states[i];
		uint32_t min_residency, exit_latency;

		/* policy cannot lead to D3 */
		if (state->state == PM_STATE_SOFT_OFF) {
			continue;
		}

		/* checking conditions for D0i3 */
		if (state->state == PM_STATE_RUNTIME_IDLE) {
			/* skipping when secondary cores are active */
			if (cpu_enabled_cores() & ~BIT(PLATFORM_PRIMARY_CORE_ID)) {
				continue;
			}
		}

		min_residency = k_us_to_ticks_ceil32(state->min_residency_us);
		exit_latency = k_us_to_ticks_ceil32(state->exit_latency_us);

		if (ticks == K_TICKS_FOREVER || (ticks >= (min_residency + exit_latency))) {
			/* TODO: PM_STATE_RUNTIME_IDLE requires substates to be defined
			 * to handle case with enabled PG andf disabled CG.
			 */
			printk("cpu %d transition to state %x (min_residency = %u, exit_latency = "
			       "%u)\n",
			       cpu, state->state, min_residency, exit_latency);
			return state;
		}
	}

	printk("cpu %d next state returned null\n", cpu);
	return NULL;
}
#endif /* CONFIG_PM_POLICY_CUSTOM */

int main(void)
{
	printk("starting\n");
	k_msleep(1000);
	for (int i = 0; i < NTHREADS; i++) {
		k_sem_init(&sems[i], 0, 1);
		k_sem_init(&done[i], 0, 1);
		cpu_active[i] = true;
	}
	k_thread_create(&threads[0], tstack[0], STACK_SIZE, core0_loop, &sems[0], NULL, NULL,
			K_HIGHEST_THREAD_PRIO, 0, K_FOREVER);
	k_thread_name_set(&threads[0], "core0_loop");
	k_thread_cpu_pin(&threads[0], 0);
	k_thread_start(&threads[0]);

	printk("starting timer\n");
	k_timer_start(&timer0, K_MSEC(1000), K_MSEC(1000));
	return 0;
}
