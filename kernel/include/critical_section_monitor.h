/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_KERNEL_INCLUDE_CRITICAL_SECTION_MONITOR_H_
#define ZEPHYR_KERNEL_INCLUDE_CRITICAL_SECTION_MONITOR_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel/internal/critical_section_monitor.h>

struct k_thread;

enum z_critical_section_monitor_event {
	Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED,
	Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT,
	Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD,
	Z_CRITICAL_SECTION_MONITOR_EVENT_COUNT,
};

/*
 * Diagnostic identifiers only: spinlock and thread may be stale. Never
 * dereference them. The caller may not resolve after compiler/linker
 * optimization. Zero cycles means that no event has been recorded.
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
 * Copy a consistent per-CPU snapshot with a bounded number of attempts.
 * Returns -EINVAL for an invalid CPU index or NULL destination.
 * Returns -EAGAIN if concurrent publication prevents a consistent snapshot.
 * The destination is valid only on success.
 * Do not call from an NMI or zero-latency interrupt.
 */
int z_critical_section_monitor_stats_get(unsigned int cpu,
					 struct z_critical_section_monitor_stats *stats);

/*
 * Reset only the current CPU's published statistics, preserving active
 * intervals and held-lock tracking. An interval that began before the reset
 * may become the next maximum when it ends.
 * Do not call from an NMI or zero-latency interrupt.
 */
void z_critical_section_monitor_stats_reset(void);

#endif /* ZEPHYR_KERNEL_INCLUDE_CRITICAL_SECTION_MONITOR_H_ */
