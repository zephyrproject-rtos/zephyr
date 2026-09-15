/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Per-interrupt statistics
 *
 * Collects, per software ISR table index and per CPU: how often the
 * interrupt fired, the total CPU cycles spent in its handler, and the
 * longest single invocation. Zephyr's equivalent of /proc/interrupts.
 *
 * The architecture interrupt dispatch path reports handler entry and
 * exit through z_irq_stats_enter()/z_irq_stats_exit(); interrupts
 * dispatched without going through the software ISR table (e.g. direct
 * ISRs, Cortex-M system exceptions such as SysTick) are not counted.
 * Durations are gross: time spent in nested interrupts preempting a
 * handler is attributed to both.
 *
 * Counters are kept per CPU, so each CPU only ever writes its own
 * slots; the aggregate figures are summed when read.
 */

#ifndef ZEPHYR_INCLUDE_PROFILING_IRQ_STATS_H_
#define ZEPHYR_INCLUDE_PROFILING_IRQ_STATS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup irq_stats Per-interrupt statistics
 * @ingroup os_services
 * @{
 */

/** Statistics for one interrupt (one software ISR table index) */
struct irq_stats_entry {
	/** Number of invocations since boot or the last reset */
	uint32_t count;
	/** Longest single invocation, in system hardware cycles */
	uint32_t max_cycles;
	/** Cycles spent in the handler in total */
	uint64_t total_cycles;
};

/**
 * @brief Get the statistics for one interrupt, summed over all CPUs.
 *
 * @param irq Software ISR table index [0, CONFIG_NUM_IRQS).
 * @param entry Filled with a snapshot of the statistics.
 *
 * @retval 0 Success
 * @retval -EINVAL @a irq out of range or @a entry is NULL
 */
int irq_stats_get(unsigned int irq, struct irq_stats_entry *entry);

/**
 * @brief Get the statistics for one interrupt on one CPU.
 *
 * @param irq Software ISR table index [0, CONFIG_NUM_IRQS).
 * @param cpu CPU index [0, CONFIG_MP_MAX_NUM_CPUS).
 * @param entry Filled with a snapshot of the statistics.
 *
 * @retval 0 Success
 * @retval -EINVAL @a irq or @a cpu out of range, or @a entry is NULL
 */
int irq_stats_get_cpu(unsigned int irq, unsigned int cpu, struct irq_stats_entry *entry);

/**
 * @brief Reset the statistics of one interrupt to zero.
 *
 * @param irq Software ISR table index [0, CONFIG_NUM_IRQS).
 *
 * @retval 0 Success
 * @retval -EINVAL @a irq out of range
 */
int irq_stats_reset_irq(unsigned int irq);

/**
 * @brief Reset the statistics of all interrupts to zero.
 */
void irq_stats_reset(void);

/** @} */

/** @cond INTERNAL_HIDDEN */

#ifdef CONFIG_IRQ_STATS
/* Called by the architecture interrupt dispatch path */
void z_irq_stats_enter(unsigned int irq);
void z_irq_stats_exit(void);
#else
static inline void z_irq_stats_enter(unsigned int irq)
{
	(void)irq;
}
static inline void z_irq_stats_exit(void)
{
}
#endif

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_PROFILING_IRQ_STATS_H_ */
