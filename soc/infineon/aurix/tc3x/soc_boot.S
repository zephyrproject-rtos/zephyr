/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

/* WDTCPUyCON0 in the SCU, CPU y at 12 bytes stride (TC3xx UM v2.0.0, 9.4) */
#define WDTCPU0CON0 0xF003624C

GTEXT(z_tricore_cpu_endinit_disable)
GTEXT(z_tricore_cpu_endinit_enable)

/*
 * Set the CPU ENDINIT bit of the running core to \value: password access
 * (bits 1:0 = 01b, PW bits 7:2 re-inverted), modify access (LCK = 1,
 * ENDINIT = \value), then wait until the bit reads back. Called with JL
 * before the context save areas exist: uses D14, D15 and A15 only and
 * returns through A11. CPU5 has CORE_ID 6.
 */
.macro endinit_set value
	mfcr	%d15, TRICORE_CORE_ID
	and	%d15, %d15, 7
	eq	%d14, %d15, 6
	sub	%d15, %d15, %d14
	mul	%d15, %d15, 12
	movh	%d14, hi:WDTCPU0CON0
	addi	%d14, %d14, lo:WDTCPU0CON0
	add	%d15, %d15, %d14
	mov.a	%a15, %d15
	ld.w	%d14, [%a15]
	insert	%d14, %d14, 0, 0, 2
	xor	%d14, %d14, 0xFC
	or	%d14, %d14, 1
	st.w	[%a15], %d14
	insert	%d14, %d14, (2 | \value), 0, 2
	st.w	[%a15], %d14
	dsync
1:
	ld.w	%d14, [%a15]
	and	%d14, %d14, 1
	jne	%d14, \value, 1b
	ji	%a11
.endm

SECTION_FUNC(TEXT, z_tricore_cpu_endinit_disable)
	endinit_set 0

SECTION_FUNC(TEXT, z_tricore_cpu_endinit_enable)
	endinit_set 1
