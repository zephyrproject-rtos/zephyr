/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>

GTEXT(z_tricore_switch)
GTEXT(z_thread_mark_switched_in)

/* void z_tricore_switch(k_thread_t *switch_to, k_thread_t *switch_from) */
SECTION_FUNC(TEXT, z_tricore_switch)
	/* Save old context */
	mfcr	%d14, 0xFE00
	mfcr	%d15, 0xFE34
	st.d	[%a5]+_thread_offset_to_saved_pcxi, %e14

	/* Load new context */
	ld.d 	%e14, [%a4]+_thread_offset_to_saved_pcxi

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	call	z_thread_mark_switched_in
#endif
	/* Activate new thread context */
	mtcr	0xFE34, %d15
	mtcr	0xFE00, %d14
	isync

	/* CPU PCXI now on new chain - safe to reclaim old dead thread */
	ld.bu	%d15, [%a5]+_thread_offset_to_thread_state
	jz.t	%d15, 3, not_dead
	mov.aa	%a4, %a5
	call	z_tricore_reclaim_csa

not_dead:

	fret
