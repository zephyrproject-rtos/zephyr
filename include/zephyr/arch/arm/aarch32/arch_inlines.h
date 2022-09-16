/*
 * Copyright 2022 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_ARCH_INLINES_H

#include <zephyr/kernel_structs.h>

#ifndef CONFIG_SMP
static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	/* Dummy implementation always return the first cpu */
	return &_kernel.cpus[0];
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_ARCH_INLINES_H */
