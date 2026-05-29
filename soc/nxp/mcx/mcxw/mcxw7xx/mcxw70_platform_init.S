/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MCXW70 Platform-Specific Initialization
 *
 * MCXW70 SOC reset code that initializes RAM
 * to prevent ECC causing faults, and calls SystemInit
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(soc_reset_hook)
SECTION_SUBSEC_FUNC(TEXT,_reset_section,soc_reset_hook)

.soc_reset_hook:
    movs.n r0, #0
    msr msplim, r0
    msr psplim, r0
    ldr r0, =0x20000000
    ldr r1, =.ram_init_sram0
    bics r1, #0x10000000
    cmp r0, r1
    bcc .ram_init_done
.ram_init_sram0: /* Initialize sram0 */
    ldr r0, =0x30000000
    ldr r1, =0x30010000
    ldr r2, =1
    ldr r3, =2
    ldr r4, =3
    ldr r5, =4
.loop01:
    stmia r0!, {r2 - r5}
    cmp r0, r1
    bcc.n .loop01
.ram_init_done:
    b SystemInit
