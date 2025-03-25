/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Indirect Branch Tracking
 *
 * Indirect Branch Tracking (IBT) setup routines.
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/x86/msr.h>

void z_x86_cet_enable(void)
{
	__asm volatile (
			"movl %cr4, %eax\n\t"
			"orl $0x800000, %eax\n\t"
			"movl %eax, %cr4\n\t"
			);
}

#ifdef CONFIG_X86_CET_IBT
void z_x86_ibt_enable(void)
{
	uint64_t msr = z_x86_msr_read(X86_S_CET_MSR);

	msr |= X86_S_CET_MSR_ENDBR | X86_S_CET_MSR_NO_TRACK;
	z_x86_msr_write(X86_S_CET_MSR, msr);
}
#endif
