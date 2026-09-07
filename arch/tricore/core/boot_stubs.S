/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

/*
 * Weak defaults for the SoC boot hooks called from reset.S. SoCs that gate
 * ENDINIT or disable a watchdog override these with strong versions; targets
 * that do not model them (e.g. QEMU) link these no-ops.
 */
WTEXT(z_tricore_cpu_endinit_disable)
SECTION_FUNC(TEXT, z_tricore_cpu_endinit_disable)
	ji %a11

WTEXT(z_tricore_cpu_endinit_enable)
SECTION_FUNC(TEXT, z_tricore_cpu_endinit_enable)
	ji %a11

WTEXT(z_tricore_wdt_boot)
SECTION_FUNC(TEXT, z_tricore_wdt_boot)
	ji %a11
