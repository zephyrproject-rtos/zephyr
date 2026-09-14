/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel_shell.h"

#include <errno.h>
#include <inttypes.h>

#include <zephyr/kernel.h>

#include <critical_section_monitor.h>

static int cmd_kernel_critical(const struct shell *sh, size_t argc, char **argv)
{
	static const char *const names[Z_CRITICAL_SECTION_MONITOR_EVENT_COUNT] = {
		[Z_CRITICAL_SECTION_MONITOR_EVENT_IRQ_LOCKED] = "irq-locked",
		[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_WAIT] = "spin-wait",
		[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD] = "spin-hold",
	};
	int result = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	for (unsigned int cpu = 0U; cpu < arch_num_cpus(); ++cpu) {
		struct z_critical_section_monitor_stats stats;
		int err = z_critical_section_monitor_stats_get(cpu, &stats);

		if (err != 0) {
			if (err == -EAGAIN) {
				shell_warn(sh, "CPU %u: statistics busy; try again", cpu);
			} else {
				shell_error(sh, "Cannot read CPU %u statistics: %d", cpu, err);
			}
			result = err;
			continue;
		}

		shell_print(sh, "CPU %u:", cpu);
		for (size_t i = 0U; i < ARRAY_SIZE(names); ++i) {
			const struct z_critical_section_monitor_record *record = &stats.max[i];

			shell_print(sh,
				    "  %s: cycles=%" PRIu32 " ns=%" PRIu64 " caller=0x%" PRIxPTR
				    " lock=%p thread=%p isr=%u",
				    names[i], record->cycles, k_cyc_to_ns_floor64(record->cycles),
				    record->caller, (const void *)record->spinlock,
				    (const void *)record->thread, (unsigned int)record->in_isr);
		}
		shell_print(sh, "  tracking overflows: %" PRIu32,
			    stats.spinlock_tracking_overflows);
	}

	return result;
}

KERNEL_CMD_ARG_ADD(critical, NULL, "Show per-CPU maximum critical section times.",
		   cmd_kernel_critical, 1, 0);
