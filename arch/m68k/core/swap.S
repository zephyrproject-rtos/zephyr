/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>
#include <offsets_short.h>

#define M68K_THREAD_SP_OFFSET \
	(___thread_t_callee_saved_OFFSET + ___callee_saved_t_sp_OFFSET)

GTEXT(z_m68k_switch)

#ifdef CONFIG_THREAD_LOCAL_STORAGE
GTEXT(__m68k_read_tp)
GDATA(z_m68k_tls_ptr)
#endif

/*
 * MOVEM saves and restores GCC's callee-saved registers on each thread stack.
 * GCC passes new and old at 4(%sp) and 8(%sp).
 */
SECTION_FUNC(TEXT, z_m68k_switch)
	movea.l 4(%sp), %a0
	movea.l 8(%sp), %a1

	movem.l %d2-%d7/%a2-%a6,-(%sp)
	move.l %sp, M68K_THREAD_SP_OFFSET(%a1)
	move.l %a1, ___thread_t_switch_handle_OFFSET(%a1)

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* Publish TLS through GCC's ABI helper; m68k has no TP register. */
	move.l ___thread_t_tls_OFFSET(%a0), z_m68k_tls_ptr
#endif

	movea.l M68K_THREAD_SP_OFFSET(%a0), %sp
	movem.l (%sp)+, %d2-%d7/%a2-%a6
	rts

#ifdef CONFIG_THREAD_LOCAL_STORAGE
/* GCC's m68k TLS ABI returns TP in %a0. */
SECTION_FUNC(TEXT, __m68k_read_tp)
	move.l z_m68k_tls_ptr, %a0
	rts
#endif
