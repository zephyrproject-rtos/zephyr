/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
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
	mfcr	%d14, TRICORE_PCXI
	st.w	[%a5]+_thread_offset_to_saved_pcxi, %d14
	/* Load new context */
	ld.w 	%d14, [%a4]+_thread_offset_to_saved_pcxi

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	call	z_thread_mark_switched_in
#endif

	/* Activate new thread context */
	mtcr	TRICORE_PCXI, %d14
	isync

	fret
