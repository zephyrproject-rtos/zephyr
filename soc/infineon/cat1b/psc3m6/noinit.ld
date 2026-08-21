/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* The boot ROM in this device uses the first 8k of SRAM while running.
 * Applications like tests\drivers\watchdog\wdt_basic_api expect that variables
 * placed in noinit memory will retain their value through a reset. This means
 * that the noinit memory region must be located after the first 8k of SRAM.
 * The line below is added to the noinit linker section to ensure that. It must
 * be used in conjunction with the NOINIT_SNIPPET_FIRST configuration option
 * that has been added to the Kconfig file.
 */
. = MAX(ABSOLUTE(.), DT_CHOSEN_SRAM_ADDR + 0x2000);
