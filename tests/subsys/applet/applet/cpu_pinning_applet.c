/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/ztest_assert.h>

/**
 * @param arg  Opaque argument forwarded from z_applet_opts.arg; the main application
 *             passes the CPU number the applet should run on.
 */
void cpu_pinning_main(void *arg)
{
	/* arch_curr_cpu() is only provided by the arch layer on SMP targets. */
#ifdef CONFIG_SMP
	unsigned int cpu = arch_curr_cpu()->id;
#else
	unsigned int cpu = 0;
#endif

	zassert_equal(cpu, (unsigned int)(uintptr_t)arg,
		      "applet main running on wrong CPU %u, expected %u", cpu,
		      (unsigned int)(uintptr_t)arg);
}

LL_EXTENSION_SYMBOL(cpu_pinning_main);
