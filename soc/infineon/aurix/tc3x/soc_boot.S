/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#define WDTCPUCON0 DT_REG_ADDR(DT_NODELABEL(wdt))

GTEXT(z_tricore_cpu_endinit_disable)
GTEXT(z_tricore_cpu_endinit_enable)

SECTION_FUNC(TEXT, z_tricore_cpu_endinit_disable)
	movh.a	%a15, hi:WDTCPUCON0
	lea	%a15, [%a15], lo:WDTCPUCON0

	/* Load current password and invert */
	ld.w	%d15, [%a15]
	xor	%d15, %d15, 0xFC

	/* Execute Password access */
	insert	%d15, %d15, 1, 0, 2
	st.w	[%a15], %d15

	/* Execute modify access to clear ENDINIT bit */
	insert	%d15, %d15, 2, 0, 2
	st.w	[%a15], %d15

	/* Ensure write completion */
	dsync

	ji	%a11

SECTION_FUNC(TEXT, z_tricore_cpu_endinit_enable)
	movh.a	%a15, hi:WDTCPUCON0
	lea	%a15, [%a15], lo:WDTCPUCON0

	/* Load current password and invert */
	ld.w	%d15, [%a15]
	xor	%d15, %d15, 0xFC

	/* Execute Password access */
	insert	%d15, %d15, 1, 0, 2
	st.w	[%a15], %d15

	/* Execute modify access to set ENDINIT bit */
	insert	%d15, %d15, 3, 0, 2
	st.w	[%a15], %d15

	/* Ensure write completion */
	dsync

	ji	%a11
