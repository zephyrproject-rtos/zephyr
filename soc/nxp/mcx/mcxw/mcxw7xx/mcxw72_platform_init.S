/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MCXW72 Platform-Specific Initialization
 *
 * The ECC RAM banks are at different addresses than on MCXW71.
 *
 * Initialize RAM to prevent ECC faults, then call SystemInit.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

#if defined(CONFIG_PM_S2RAM)
/* The early ECC hook must not use a stack or clobber callee-saved registers. */
GDATA(nxp_mcxw7x_s2ram_mark)
GTEXT(soc_early_reset_hook)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,soc_early_reset_hook)
#define ECC_ZERO_REGS r2 - r3
#else
GTEXT(soc_reset_hook)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,soc_reset_hook)
#define ECC_ZERO_REGS r2 - r5
#endif
    ldr r0, =0x14000000
    ldr r1, =.ram_init_ctcm01
    bics r1, #0x10000000
    cmp r0, r1
    bcc .ram_init_done
.ram_init_ctcm01: /* Initialize ctcm01 */
    ldr r0, =0x14000000
    ldr r1, =0x14008000
    ldr r2, =0
    ldr r3, =0
#if !defined(CONFIG_PM_S2RAM)
    ldr r4, =0
    ldr r5, =0
#endif
.loop01:
    stmia r0!, {ECC_ZERO_REGS}
    cmp r0, r1
    bcc.n .loop01
.ram_init_stcm012: /* Initialize stcm012 */
    ldr r0, =0x30000000
    ldr r1, =0x30010000
.loop012:
    stmia r0!, {ECC_ZERO_REGS}
    cmp r0, r1
    bcc.n .loop012
.ram_init_stcm8:
    ldr r0, =0x30038000
    ldr r1, =0x3003a000
.loop8: /* Initialize STCM8. */
    stmia r0!, {ECC_ZERO_REGS}
    cmp r0, r1
    bcc.n .loop8
.ram_init_done:
#if defined(CONFIG_PM_S2RAM)
    ldr r0, =nxp_mcxw7x_s2ram_mark
    movs r1, #0
    str r1, [r0]
    bx lr

GTEXT(soc_reset_hook)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,soc_reset_hook)
#endif
    b SystemInit
