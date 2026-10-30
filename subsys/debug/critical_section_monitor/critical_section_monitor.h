/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_DEBUG_CRITICAL_SECTION_MONITOR_H_
#define ZEPHYR_SUBSYS_DEBUG_CRITICAL_SECTION_MONITOR_H_

#include <stdbool.h>
#include <stdint.h>

struct k_spinlock;
struct k_thread;

enum z_critical_section_monitor_event {
	Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED,
	Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT,
	Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD,
	Z_CRITICAL_SECTION_MONITOR_EVENT_COUNT,
};

/*
 * These addresses are for diagnostics only. The lock or thread may no longer
 * exist, so never dereference them. Optimizations can make the caller address
 * hard to map back to source code. Zero cycles means no interval was recorded.
 */
struct z_critical_section_monitor_record {
	uintptr_t caller;
	const struct k_spinlock *spinlock;
	/* Thread at interval start; in an ISR, the interrupted thread. */
	const struct k_thread *thread;
	uint32_t cycles;
	bool in_isr;
};

struct z_critical_section_monitor_stats {
	struct z_critical_section_monitor_record max[Z_CRITICAL_SECTION_MONITOR_EVENT_COUNT];
	uint32_t spinlock_tracking_overflows;
};

/*
 * Copy one CPU's statistics, retrying if they change during the read.
 * Returns -EINVAL for an invalid CPU index or NULL destination.
 * Returns -EAGAIN if no consistent snapshot is obtained within the retry limit.
 * The destination is valid only on success.
 * Do not call from an NMI or zero-latency interrupt.
 */
int z_critical_section_monitor_stats_get(unsigned int cpu,
					 struct z_critical_section_monitor_stats *stats);

/*
 * Reset the current CPU's statistics without stopping active measurements.
 * An interval that began before the reset may become the next maximum when it ends.
 * Do not call from an NMI or zero-latency interrupt.
 */
void z_critical_section_monitor_stats_reset(void);

#endif /* ZEPHYR_SUBSYS_DEBUG_CRITICAL_SECTION_MONITOR_H_ */
