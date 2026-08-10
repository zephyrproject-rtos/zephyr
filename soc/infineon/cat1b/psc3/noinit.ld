/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
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

/*
 * PDL references these symbols from its DeepSleep-RAM warm-boot path, which
 * is never taken when using plain CY_SYSPM_MODE_DEEPSLEEP. Zephyr's linker
 * does not define them, so provide dummy values to satisfy the linker.
 */
PROVIDE(__Vectors = __rom_region_start);
PROVIDE(__s_vector_table_rw = _image_ram_start);
PROVIDE(__ns_vector_table_rw = _image_ram_start);
PROVIDE(__StackLimit = _image_ram_start);
