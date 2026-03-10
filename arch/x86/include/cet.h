/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_CET_H
#define ZEPHYR_ARCH_X86_INCLUDE_CET_H

#ifndef _ASMLANGUAGE

/* Enables Control-flow Enforcement Technology (CET). Currently only
 * supported in 32-bit mode.
 */
void z_x86_cet_enable(void);

#ifdef CONFIG_X86_CET_IBT
/* Enables Indirect Branch Tracking (IBT).
 */
void z_x86_ibt_enable(void);
#endif

#if defined(CONFIG_X86_64) && defined(CONFIG_HW_SHADOW_STACK)
static inline void z_x86_setup_interrupt_ssp_table(uintptr_t issp_table)
{
	z_x86_msr_write(X86_INTERRUPT_SSP_TABLE_MSR, issp_table);
}
#endif

#ifdef CONFIG_X86_CET_VERIFY_KERNEL_SHADOW_STACK
void z_x86_cet_shadow_stack_panic(void);
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_CET_H */
