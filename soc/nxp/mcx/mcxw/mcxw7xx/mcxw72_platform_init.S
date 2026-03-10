/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MCXW72 Platform-Specific Initialization
 *
 * When compared to MCXW71, the Ram Banks with ECC
 * are located in different addresses.
 *
 * MCXW72 SOC reset code that initializes RAM
 * to prevent ECC causing faults, and calls SystemInit
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(soc_reset_hook)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,soc_reset_hook)

.soc_reset_hook:
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
    ldr r4, =0
    ldr r5, =0
.loop01:
    stmia r0!, {r2 - r5}
    cmp r0, r1
    bcc.n .loop01
.ram_init_stcm012: /* Initialize stcm012 */
    ldr r0, =0x30000000
    ldr r1, =0x30010000
.loop012:
    stmia r0!, {r2 - r5}
    cmp r0, r1
    bcc.n .loop012
.ram_init_stcm8:
    ldr r0, =0x30038000
    ldr r1, =0x3003a000
.loop8: /* Initialize stcm5 */
    stmia r0!, {r2 - r5}
    cmp r0, r1
    bcc.n .loop8
.ram_init_done:
    b SystemInit
