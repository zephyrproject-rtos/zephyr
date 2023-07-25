/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 - 2019 , NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#if defined(CONFIG_PLATFORM_SPECIFIC_INIT) && defined(CONFIG_SOC_LPC54114_M4)

    .syntax unified
    .arch armv7-m
    .text
    .thumb

rel_vals:
    .long   0xE000ED00   /* cpu_id */
    .long   0x40000800   /* cpu_ctrl */
    .long   0x40000804   /* coproc_boot */
    .long   0x40000808   /* coproc_stack */
    .short  0x0FFF
    .short  0x0C24

GTEXT(z_arm_platform_init)
SECTION_FUNC(TEXT,z_arm_platform_init)

/* Both the M0+ and M4 core come via this shared startup code,
 * but the M0+ and M4 core have different vector tables.
 * Determine if the core executing this code is the master or
 * the slave and handle each core state individually. */

shared_boot_entry:
    ldr     r6, =rel_vals

    /* Flag for slave core (0) */
    movs    r4, 0
    movs    r5, 1
    push {lr}
    bl SystemInit
    pop {lr}

    /* Determine which core (M0+ or M4) this code is running on */
    /* r2 = (((*cpu_id) >> 4) & 0xFFF); (M4 core == 0xC24) */
get_current_core_id:
    ldr     r0, [r6, #0]
    ldr     r1, [r0]                        /* r1 = CPU ID status */
    lsrs    r1, r1, #4                      /* Right justify 12 CPU ID bits */
    ldrh    r2, [r6, #16]                   /* Mask for CPU ID bits */
    ands    r2, r1, r2                      /* r2 = ARM COrtex CPU ID */
    ldrh    r3, [r6, #18]                   /* Mask for CPU ID bits */
    cmp     r3, r2                          /* Core ID matches M4 identifier */
    bne     get_master_status
    mov     r4, r5                          /* Set flag for master core (1) */

    /* Determine if M4 core is the master or slave */
    /* r3 = ((*cpu_ctrl) & 1); (0 == m0+, 1 == M4) */
get_master_status:
    ldr     r0, [r6, #4]
    ldr     r3, [r0]                        /* r3 = SYSCON co-processor CPU control status */

    ands    r3, r3, r5                      /* r3 = (Bit 0: 1 = M4 is master, 0 = M4 is slave) */

    /* Select boot based on selected master core and core ID */

select_boot:
    eors    r3, r3, r4                      /* r4 = (Bit 0: 0 = master, 1 = slave) */

    bne     slave_boot
    b       normal_boot

    /* Slave boot */
slave_boot:
    ldr     r0, [r6, #8]
    ldr     r2, [r0]                        /* r1 = SYSCON co-processor boot address */

    cmp     r2, #0                          /* Slave boot address = 0 (not set up)? */

    beq     cpu_sleep
    ldr     r0, [r6, #12]
    ldr     r1, [r0]                        /* r5 = SYSCON co-processor stack address */

    mov     sp, r1                          /* Update slave CPU stack pointer */

    /* Be sure to update VTOR for the slave MCU to point to the */
    /* slave vector table in boot memory */
    bx      r2                              /* Jump to slave boot address */

    /* Slave isn't yet setup for system boot from the master */
    /* so sleep until the master sets it up and then reboots it */
cpu_sleep:
    mov     sp, r5                          /* Will force exception if something happens */
cpu_sleep_wfi:
    wfi                                     /* Sleep forever until master reboots */
    b       cpu_sleep_wfi
normal_boot:
    bx      lr

#endif
