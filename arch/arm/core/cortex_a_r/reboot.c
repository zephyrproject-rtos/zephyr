/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright 2024 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-A and Cortex-R System Control Block interface
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/linker/linker-defs.h>

#if defined(CONFIG_AARCH32_ARMV8_R)

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

static inline void relocate_vector_table(void)
{
	write_sctlr(read_sctlr() & ~HIVECS);
	write_vbar(VECTOR_ADDRESS & VBAR_MASK);
	barrier_isync_fence_full();
}

#else

/*
 * GCC can detect if memcpy is passed a NULL argument, however one of
 * the cases of relocate_vector_table() it is valid to pass NULL, so we
 * suppress the warning for this case.  We need to do this before
 * string.h is included to get the declaration of memcpy.
 */
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_NONNULL)

#include <string.h>

#define VECTOR_ADDRESS 0

void __weak relocate_vector_table(void)
{
#if defined(CONFIG_XIP) && (CONFIG_FLASH_BASE_ADDRESS != 0) ||                                     \
	!defined(CONFIG_XIP) && (CONFIG_SRAM_BASE_ADDRESS != 0)
	write_sctlr(read_sctlr() & ~HIVECS);
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;
	(void)memcpy(VECTOR_ADDRESS, _vector_start, vector_size);
#endif
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_NONNULL)

#endif /* !CONFIG_AARCH32_ARMV8_R */

void z_arm_relocate_vector_table(void)
{
	relocate_vector_table();
}

/**
 *
 * @brief Reset the system
 *
 * This routine resets the processor.
 *
 */

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);
}
