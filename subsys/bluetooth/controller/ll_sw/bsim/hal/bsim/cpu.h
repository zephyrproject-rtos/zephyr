/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

static inline void cpu_sleep(void)
{
	k_cpu_atomic_idle(irq_lock());
}

static inline void cpu_dmb(void)
{
	compiler_barrier();
}
