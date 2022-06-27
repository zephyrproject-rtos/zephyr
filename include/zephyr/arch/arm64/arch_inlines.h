/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ARCH_INLINES_H
#define ZEPHYR_INCLUDE_ARCH_ARM64_ARCH_INLINES_H

#ifndef _ASMLANGUAGE

#include <zephyr/kernel_structs.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/arch/arm64/tpidrro_el0.h>

static ALWAYS_INLINE _cpu_t *arch_curr_cpu(void)
{
	return (_cpu_t *)(read_tpidrro_el0() & TPIDRROEL0_CURR_CPU);
}

static ALWAYS_INLINE int arch_exception_depth(void)
{
	return (read_tpidrro_el0() & TPIDRROEL0_EXC_DEPTH) / TPIDRROEL0_EXC_UNIT;
}

#endif /* !_ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ARCH_INLINES_H */
