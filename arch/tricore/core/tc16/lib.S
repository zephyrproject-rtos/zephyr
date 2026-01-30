/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#if CONFIG_SOC_SERIES_TC3X

#define WDTCPU0CON0 (0xF0036000 + 0x024C + 0x12*CONFIG_TRICORE_CORE_ID)
#define WDTSCON0    (0xF0036000 + 0x02A8)

GTEXT(tricore_cpu_endinit_disable)
GTEXT(tricore_cpu_endinit_enable)
GTEXT(tricore_safety_endinit_enable)

SECTION_FUNC(TEXT, tricore_cpu_endinit_disable)
	movh.a	%a15, hi:WDTCPU0CON0
	lea	%a15, [%a15], lo:WDTCPU0CON0

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

SECTION_FUNC(TEXT, tricore_cpu_endinit_enable)
	movh.a	%a15, hi:WDTCPU0CON0
	lea	%a15, [%a15], lo:WDTCPU0CON0

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

SECTION_FUNC(TEXT, tricore_safety_endinit_enable)
	movh.a	%a15, hi:WDTSCON0
	lea	%a15, [%a15], lo:WDTSCON0

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
#else
#error "Invalid SoC series"
#endif
