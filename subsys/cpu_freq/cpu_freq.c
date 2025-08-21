/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/policy.h>
#include <zephyr/cpu_freq/cpu_freq.h>

LOG_MODULE_REGISTER(cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
#define CPU_FREQ_IPI_REQUIRED

static struct k_ipi_work cpu_freq_work;

#ifndef CONFIG_CPU_FREQ_PER_CPU_SCALING

/*
 * IPI tracking is needed on SMP systems where all CPUs share the same
 * frequency. The last CPU to call cpu_freq_best_pstate() sets the best
 * pstate for all CPUs.
 */

#define CPU_FREQ_IPI_TRACKING

static struct k_spinlock lock;
static const struct pstate *pstate_best;
static unsigned int num_unprocessed_cpus;
#endif /* !CONFIG_CPU_FREQ_PER_CPU_SCALING */
#endif /* CONFIG_SMP && (CONFIG_MP_MAX_NUM_CPUS > 1) */

static void cpu_freq_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(cpu_freq_timer, cpu_freq_timer_handler, NULL);

/*
 * Determine the best pstate. On both UP systems and SMP systems that allow
 * per-CPU frequency control, this is simply the pstate passed in. However, on
 * SMP systems where all CPUs share the same frequency, the best pstate is the
 * one with the highest load threshold from all CPUs.
 */
static const struct pstate *cpu_freq_best_pstate(const struct pstate *state)
{
#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	if ((pstate_best == NULL) ||
	    (state->load_threshold > pstate_best->load_threshold)) {
		pstate_best = state;
	}

	__ASSERT(num_unprocessed_cpus != 0U, "cpu_freq: Out of sync");

	num_unprocessed_cpus--;
	if (num_unprocessed_cpus > 0) {
		k_spin_unlock(&lock, key);
		return NULL;
	}
	k_spin_unlock(&lock, key);

	return pstate_best;
#else
	return state;
#endif
}

static void cpu_freq_next_pstate(void)
{
	int ret;

	/* Get next performance state */
	const struct pstate *pstate_next;

	ret = cpu_freq_policy_select_pstate(&pstate_next);
	if (ret) {
		LOG_ERR("Failed to get pstate: %d", ret);
		return;
	}

	pstate_next = cpu_freq_best_pstate(pstate_next);

	/* Set performance state using pstate driver */
	if (pstate_next != NULL) {
		ret = cpu_freq_pstate_set(pstate_next);
		if (ret) {
			LOG_ERR("Failed to set performance state: %d", ret);
			return;
		}
	}
}

void cpu_freq_ipi_tracking_init(void)
{
#ifdef CPU_FREQ_IPI_TRACKING
	k_spinlock_key_t key = k_spin_lock(&lock);

	pstate_best = NULL;
	num_unprocessed_cpus = arch_num_cpus();

	k_spin_unlock(&lock, key);
#endif
}

#ifdef CPU_FREQ_IPI_REQUIRED
static void cpu_freq_ipi_handler(struct k_ipi_work *work)
{
	ARG_UNUSED(work);

	cpu_freq_next_pstate();
}
#endif

/*
 * Timer that expires periodically to execute the selected policy algorithm
 * and pass the next P-state to the P-state driver.
 */
static void cpu_freq_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

#ifdef CPU_FREQ_IPI_REQUIRED
	uint32_t num_cpus = arch_num_cpus();
	uint32_t target_cpus;
	int ret;

	__ASSERT(num_cpus <= 32U, "Too many CPUs");

	/*
	 * Create a bitmask identifying all the CPUs except the current CPU.
	 * A special case is required for 32 CPUs since shifting a 32-bit
	 * integer left by 32 bits is undefined behavior.
	 */

	target_cpus = (num_cpus == 32U) ? 0xFFFFFFFF : (1U << num_cpus) - 1U;
	target_cpus ^= (1U << _current_cpu->id),

	ret = k_ipi_work_add(&cpu_freq_work,
			     target_cpus ^ (1U << _current_cpu->id),
			     cpu_freq_ipi_handler);

	if (ret != 0) {
		/*
		 * The previous CPU frequency work item has yet to finish
		 * processing. Either one or more of the previously target CPUs
		 * has been too busy to process it, and/or the CPU frequency
		 * policy algorithm is taking too long to complete. Log the
		 * error and try again on the next timer expiration.
		 */

		LOG_ERR("Failed to add IPI work: %d", ret);

		return;
	}
	cpu_freq_ipi_tracking_init();

	k_ipi_work_signal();
#endif

	cpu_freq_next_pstate();
}

static int cpu_freq_init(void)
{
#ifdef CPU_FREQ_IPI_REQUIRED
	k_ipi_work_init(&cpu_freq_work);
#endif
	k_timer_start(&cpu_freq_timer, K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS),
		      K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS));
	LOG_INF("CPU frequency subsystem initialized with interval %d ms",
		CONFIG_CPU_FREQ_INTERVAL_MS);
	return 0;
}

SYS_INIT(cpu_freq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
