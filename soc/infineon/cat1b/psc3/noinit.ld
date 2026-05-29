/*
 * Copyright (c) 2025 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The boot ROM in this device uses the first 8k of SRAM while running.  Applications like
 * tests\drivers\watchdog\wdt_basic_api expect that variables placed in noint memory will retain
 * there value through a reset.  This means that the noinit memory region must be located after
 * the first 8k of SRAM.  The line below is added to the noinit linker section to ensure that.
 * It must be used in conjunction with the NOINIT_SNIPPET_FIRST configuration option that has
 * been added to the Kconfig file.
 */
. = MAX(ABSOLUTE(.), 0x34002000);
