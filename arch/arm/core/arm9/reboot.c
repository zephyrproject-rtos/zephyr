/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <arm9/cp15.h>

#include <zephyr/kernel.h>

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
