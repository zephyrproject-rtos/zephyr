/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets_short.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>
#include <vector.h>

GTEXT(z_m68k_exc_entry_sync)
GTEXT(z_m68k_exc_entry_async)

/*
 * Each stub pushes its vector because 68000 frames lack a vector word, then
 * jumps to the synchronous or asynchronous common entry.
 */
#define M68K_EXC(vecnum, name, form) M68K_EXC_##form(vecnum, name)

#define M68K_EXC_SYNC(vecnum, name) \
	SECTION_FUNC(TEXT, __m68k_vec_##name) \
		move.l	#(vecnum), -(%sp) ;\
		jbra	z_m68k_exc_entry_sync

/*
 * Hardware IRQ nesting is deferred. Mask before touching the software frame;
 * RTE restores the interrupted IPL.
 */
#define M68K_EXC_ASYNC(vecnum, name) \
	SECTION_FUNC(TEXT, __m68k_vec_##name) \
		ori.w	#0x0700, %sr ;\
		move.l	#(vecnum), -(%sp) ;\
		jbra	z_m68k_exc_entry_async

#include <vector_defs.h>
#undef M68K_EXC
#undef M68K_EXC_SYNC
#undef M68K_EXC_ASYNC

/* Every post-reset slot has a stub so its vector remains diagnosable on 68000. */
.section .m68k_vector_table, "a"
.balign 4
vectbl_base:
	.long	z_interrupt_stacks + __z_interrupt_stack_SIZEOF
	.long	__start

#define M68K_EXC(vecnum, name, form) \
	.if	(. - vectbl_base) != ((vecnum) * M68K_VECTOR_SIZE) ;\
	.error	"missing or out-of-order entry in vector_defs.h" ;\
	.endif                                                    ;\
	.long	__m68k_vec_##name
#include <vector_defs.h>
#undef M68K_EXC

	.if	(. - vectbl_base) != (M68K_NUM_VECTORS * M68K_VECTOR_SIZE)
	.error	"vector_defs.h does not fill the CPU vector table"
	.endif
