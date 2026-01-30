/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

GTEXT(tricore_cpu_disable_virtualization)

SECTION_FUNC(TEXT, tricore_cpu_disable_virtualization)
	/* Disable virtualization by clearing VM bit in VCON0.
	 * Next VMN is set to 1 by default in VCON2
	 */
	mov     %d15, 0
	mtcr    0xB000, %d15
	/* Set PSW to avoid call stack trap */
	mov     %d15, 0xB00
	mtcr    0xFE04, %d15
	/* Ensure all changes to control registers take effect */
	isync
	/* Use rfh to set current VMN to 1 (default VM) */
	rfh

#if CONFIG_SOC_SERIES_TC4X

#define CPU_PROTSFR (0xF880E080 + 0x40000*CONFIG_TRICORE_CORE_ID)

GTEXT(tricore_cpu_endinit_disable)
GTEXT(tricore_cpu_endinit_enable)
GTEXT(tricore_safety_endinit_enable)

SECTION_FUNC(TEXT, tricore_cpu_endinit_disable)
	movh.a  %a15, hi:CPU_PROTSFR
	lea     %a15, [%a15], lo:CPU_PROTSFR

	/* Load and check PROT register */
	ld.w	%d15, [%a15]
	extr.u	%d15, %d15, 0, 3
	jlt.u	%d15, 3, is_config

	/* Go to config state */
	insert	%d15, %d15, 9, 0, 16
	st.w	[%a15], %d15
	dsync

is_config:
	ji %a11

SECTION_FUNC(TEXT, tricore_cpu_endinit_enable)
	movh.a	%a15, hi:CPU_PROTSFR
	lea 	%a15, [%a15], lo:CPU_PROTSFR

	/* Load and check PROT register */
	ld.w	%d15, [%a15]
	extr.u	%d15, %d15, 0, 3
	jge.u	%d15, 2, no_lock

	/* Go to run state */
	insert	%d15, %d15, 12, 0, 16
	st.w	[%a15], %d15
	dsync

no_lock:
	ji %a11

SECTION_FUNC(TEXT, tricore_safety_endinit_enable)
	ji %a11

#else
#error "Invalid SoC series"
#endif
