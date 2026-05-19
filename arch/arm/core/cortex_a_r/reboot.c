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
#include <zephyr/arch/common/init.h>

#if defined(CONFIG_AARCH32_ARMV8_R) || defined(CONFIG_AARCH32_ARMV8_A)

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

static inline void relocate_vector_table(void)
{
	write_sctlr(read_sctlr() & ~HIVECS);
	write_vbar(VECTOR_ADDRESS & VBAR_MASK);
	barrier_isync_fence_full();
}
#elif defined(CONFIG_ARMV5)
#include <arm9/cp15.h>
/*
 * ARMv5 has no VBAR register; the exception vector base is fixed at address
 * 0x00000000 (normal vectors) or 0xFFFF0000 (high vectors, SCTLR bit 13).
 *
 * Here the default (exception vectors mapped at 0x0) is used, so all that is
 * needed is to ensure the HIVECS bit is clear so that the processor fetches
 * the vectors from 0x00000000.
 *
 * '__weak' is used for allowing the routine be overridden if the high vectors
 * is used, or if vectors need to be copied first.
 */
__weak void relocate_vector_table(void)
{
	/* Clear HIVECS: select low exception vectors at 0x00000000 */
	__set_SCTLR(__get_SCTLR() & ~HIVECS);

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
	!defined(CONFIG_XIP) && (DT_CHOSEN_SRAM_ADDR != 0)
	write_sctlr(read_sctlr() & ~HIVECS);
	size_t vector_size = (size_t)_vector_end - (size_t)_vector_start;
	(void)arch_early_memcpy(VECTOR_ADDRESS, _vector_start, vector_size);
#endif
}

TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_NONNULL)

#endif /* CONFIG_AARCH32_ARMV8_R || CONFIG_AARCH32_ARMV8_A */

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
