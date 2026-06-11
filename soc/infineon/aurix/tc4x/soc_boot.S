/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

#define CPU_PROTSFR (DT_REG_ADDR(DT_NODELABEL(cpu)) + 0xE080)

GTEXT(z_tricore_cpu_endinit_disable)
GTEXT(z_tricore_cpu_endinit_enable)

SECTION_FUNC(TEXT, z_tricore_cpu_endinit_disable)
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

SECTION_FUNC(TEXT, z_tricore_cpu_endinit_enable)
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
