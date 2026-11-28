/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Parking of the other CPUs when one CPU halts the system on a fatal error.
 * The fatal error handling in kernel/fatal.c reaches this code through the
 * hooks in ksmp.h.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/platform/hooks.h>
#include <ksmp.h>

#define FATAL_NO_CPU           (-1)
/* Time given to the other CPUs to park, in microseconds */
#define FATAL_PAUSE_TIMEOUT_US 100000

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS < ATOMIC_BITS);

/* CPU halting the system, FATAL_NO_CPU when none */
static atomic_t fatal_cpu = ATOMIC_INIT(FATAL_NO_CPU);
/* CPUs parked on behalf of fatal_cpu */
static atomic_t paused_cpus;

static bool paused_by_other_cpu(int cpu)
{
	atomic_val_t owner = atomic_get(&fatal_cpu);

	return (owner != FATAL_NO_CPU) && (owner != cpu);
}

static FUNC_NORETURN void park(int cpu)
{
	(void)arch_irq_lock();
	atomic_or(&paused_cpus, BIT(cpu));
	soc_fatal_cpu_park_hook();
	for (;;) {
		arch_spin_relax();
	}
}

void z_fatal_pause_cpus(void)
{
	atomic_val_t others;
	int cpu;

	/* k_fatal_halt() may be called by a thread that could still change CPU */
	(void)arch_irq_lock();
	cpu = arch_curr_cpu()->id;
	others = BIT_MASK(arch_num_cpus()) & ~BIT(cpu);

	if (atomic_get(&fatal_cpu) == cpu) {
		return;
	}

	if (!atomic_cas(&fatal_cpu, FATAL_NO_CPU, cpu)) {
		park(cpu);
	}

#ifdef CONFIG_SCHED_IPI_SUPPORTED
	arch_sched_broadcast_ipi();
#endif
	for (int i = 0; i < FATAL_PAUSE_TIMEOUT_US; i += 10) {
		if ((atomic_get(&paused_cpus) & others) == others) {
			break;
		}
		k_busy_wait(10);
	}
}

void z_fatal_ipi(void)
{
	int cpu = arch_curr_cpu()->id;

	if (paused_by_other_cpu(cpu)) {
		park(cpu);
	}
}
