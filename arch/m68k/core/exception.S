/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets_short.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

GTEXT(z_m68k_exc_entry_sync)
GTEXT(z_m68k_exc_entry_async)
GTEXT(z_m68k_exc_handle_sync)
GTEXT(z_m68k_exc_handle_async)
GTEXT(z_m68k_isr_enter)
GTEXT(z_m68k_isr_exit)
GTEXT(z_m68k_switch)

#define M68K_EXC_VECTOR_SIZE \
	(__struct_arch_esf_sr_OFFSET - __struct_arch_esf_vector_OFFSET)

/*
 * The vector stub adds a 32-bit vector number above the CPU frame:
 * SP+0 vector, SP+4 SR, SP+6 PC, SP+10 format/vector (68010 and later).
 * Synchronous exceptions stay in thread context and cannot reschedule.
 */
SECTION_FUNC(TEXT, z_m68k_exc_entry_sync)
	movem.l	%d0/%d1/%a0/%a1,-(%sp)

	movea.l	%sp,%a1

	move.l	%a1,-(%sp)
	jbsr	z_m68k_exc_handle_sync
	adda.l	#4,%sp

	movem.l	(%sp)+,%d0/%d1/%a0/%a1

	adda.l	#M68K_EXC_VECTOR_SIZE,%sp

	rte

/* Interrupt return may switch threads. */
SECTION_FUNC(TEXT, z_m68k_exc_entry_async)
	movem.l	%d0/%d1/%a0/%a1,-(%sp)

	jbsr	z_m68k_isr_enter

	movea.l	%sp,%a1

	move.l	%a1,-(%sp)
	jbsr	z_m68k_exc_handle_async
	adda.l	#4,%sp

	suba.l	#4,%sp
	movea.l	%sp,%a0
	move.l	%a0,-(%sp)
	jbsr	z_m68k_isr_exit
	adda.l	#4,%sp

	/*
	 * A switch returns here when the interrupted thread runs again, with its
	 * exception frame intact.
	 */
	tst.l	%d0
	beq	1f
	move.l	%d0,-(%sp)
	jbsr	z_m68k_switch
	adda.l	#8,%sp
	bra	2f
1:
	adda.l	#4,%sp
2:

	movem.l	(%sp)+,%d0/%d1/%a0/%a1

	adda.l	#M68K_EXC_VECTOR_SIZE,%sp

	rte
