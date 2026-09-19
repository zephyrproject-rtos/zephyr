/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include <critical_section_monitor.h>

static atomic_t busy_cpu_zero;

int __real_z_critical_section_monitor_stats_get(unsigned int cpu,
						struct z_critical_section_monitor_stats *stats);
int __wrap_z_critical_section_monitor_stats_get(unsigned int cpu,
						struct z_critical_section_monitor_stats *stats);

int __wrap_z_critical_section_monitor_stats_get(unsigned int cpu,
						struct z_critical_section_monitor_stats *stats)
{
	if (cpu == 0U && atomic_get(&busy_cpu_zero) != 0) {
		return -EAGAIN;
	}
	return __real_z_critical_section_monitor_stats_get(cpu, stats);
}

ZTEST(kernel_critical_section_monitor_shell, test_busy_cpu_does_not_stop_output)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int ret;

	shell_backend_dummy_clear_output(sh);
	atomic_set(&busy_cpu_zero, 1);
	ret = shell_execute_cmd(sh, "kernel critical");
	atomic_clear(&busy_cpu_zero);

	zassert_equal(ret, -EAGAIN);
	output = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(output, "CPU 0: statistics busy; try again"));
	for (unsigned int cpu = 1U; cpu < arch_num_cpus(); ++cpu) {
		char heading[32];

		snprintf(heading, sizeof(heading), "CPU %u:", cpu);
		zassert_not_null(strstr(output, heading), "missing %s after busy CPU", heading);
	}
}

ZTEST(kernel_critical_section_monitor_shell, test_output_is_read_only)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	struct z_critical_section_monitor_stats before;
	struct z_critical_section_monitor_stats after;
	struct k_spinlock lock = {0};
	k_spinlock_key_t key;
	unsigned int cpu;
	const char *output;
	size_t size;

	key = k_spin_lock(&lock);
	cpu = CPU_ID;
	k_busy_wait(1000U);
	k_spin_unlock(&lock, key);
	zassert_ok(z_critical_section_monitor_stats_get(cpu, &before));
	zassert_true(before.max[Z_CRITICAL_SECTION_MONITOR_EVENT_SPINLOCK_HOLD].cycles > 0U);

	shell_backend_dummy_clear_output(sh);
	zassert_ok(shell_execute_cmd(sh, "kernel critical"));
	output = shell_backend_dummy_get_output(sh, &size);
	zassert_true(size > 0U);

	for (unsigned int i = 0U; i < arch_num_cpus(); ++i) {
		char heading[32];

		snprintf(heading, sizeof(heading), "CPU %u:", i);
		zassert_not_null(strstr(output, heading), "missing %s", heading);
	}
	zassert_not_null(strstr(output, "irq-locked: cycles="));
	zassert_not_null(strstr(output, "spin-wait: cycles="));
	zassert_not_null(strstr(output, "spin-hold: cycles="));
	zassert_not_null(strstr(output, " ns="));
	zassert_not_null(strstr(output, " caller=0x"));
	zassert_not_null(strstr(output, " lock="));
	zassert_not_null(strstr(output, " thread="));
	zassert_not_null(strstr(output, " isr="));
	zassert_not_null(strstr(output, "tracking overflows:"));

	zassert_equal(shell_execute_cmd(sh, "kernel critical reset"), -EINVAL);
	zassert_ok(z_critical_section_monitor_stats_get(cpu, &after));

	/* Shell activity can raise the maxima, but reading must not clear them. */
	for (size_t i = 0U; i < ARRAY_SIZE(before.max); ++i) {
		zassert_true(after.max[i].cycles >= before.max[i].cycles);
	}
	zassert_true(after.spinlock_tracking_overflows >= before.spinlock_tracking_overflows);
}

static void *shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");
	return NULL;
}

ZTEST_SUITE(kernel_critical_section_monitor_shell, NULL, shell_setup, NULL, NULL, NULL);
