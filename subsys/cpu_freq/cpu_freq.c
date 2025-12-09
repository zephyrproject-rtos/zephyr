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

/* Build-time validation: require performance_states node with at least one child */
#define PSTATE_ROOT DT_PATH(performance_states)
#define CPU_FREQ_COUNT_OKAY_CHILD(_node_id) + 1
enum { CPU_FREQ_PSTATE_COUNT = 0 DT_FOREACH_CHILD_STATUS_OKAY(PSTATE_ROOT,
		CPU_FREQ_COUNT_OKAY_CHILD) };

BUILD_ASSERT(DT_NODE_EXISTS(PSTATE_ROOT),
	"cpu_freq: performance_states node missing in devicetree");
BUILD_ASSERT(CPU_FREQ_PSTATE_COUNT > 0,
	"cpu_freq: No P-states defined in devicetree");

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)

static struct k_ipi_work cpu_freq_work;

#endif /* CONFIG_SMP && (CONFIG_MP_MAX_NUM_CPUS > 1) */

static void cpu_freq_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(cpu_freq_timer, cpu_freq_timer_handler, NULL);

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

	cpu_freq_policy_pstate_set(pstate_next);
}

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
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

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
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
	target_cpus ^= (1U << _current_cpu->id);

	ret = k_ipi_work_add(&cpu_freq_work, target_cpus, cpu_freq_ipi_handler);

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
	cpu_freq_policy_reset();

	k_ipi_work_signal();
#else
	cpu_freq_policy_reset();
#endif

	cpu_freq_next_pstate();
}

static int cpu_freq_init(void)
{
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	k_ipi_work_init(&cpu_freq_work);
#endif
	k_timer_start(&cpu_freq_timer, K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS),
		      K_MSEC(CONFIG_CPU_FREQ_INTERVAL_MS));
	LOG_INF("CPU frequency subsystem initialized with interval %d ms",
		CONFIG_CPU_FREQ_INTERVAL_MS);
	return 0;
}

SYS_INIT(cpu_freq_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
