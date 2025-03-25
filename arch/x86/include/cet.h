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

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_ARCH_X86_INCLUDE_CET_H */
