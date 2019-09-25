/*
 * Copyright (c) 2018 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>

.macro clear_scratch_regs
	mov r1, 0
	mov r2, 0
	mov r3, 0
	mov r4, 0
	mov r5, 0
	mov r6, 0
	mov r7, 0
	mov r8, 0
	mov r9, 0
	mov r10, 0
	mov r11, 0
	mov r12, 0
.endm

.macro clear_callee_regs
	mov r25, 0
	mov r24, 0
	mov r23, 0
	mov r22, 0
	mov r21, 0
	mov r20, 0
	mov r19, 0
	mov r18, 0
	mov r17, 0
	mov r16, 0

	mov r15, 0
	mov r14, 0
	mov r13, 0
.endm

GTEXT(arc_go_to_normal)
GTEXT(_arc_do_secure_call)
GDATA(arc_s_call_table)


SECTION_FUNC(TEXT, _arc_do_secure_call)
	/* r0-r5: arg1-arg6, r6 is call id */
	/* the call id should be checked */
	/* disable normal interrupt happened when processor in secure mode ? */
	/* seti (0x30 | (ARC_N_IRQ_START_LEVEL-1)) */
	breq r6, ARC_S_CALL_CLRI, _s_clri
	breq r6, ARC_S_CALL_SETI, _s_seti
	push_s blink
	mov	blink, arc_s_call_table
	ld.as r6, [blink, r6]

	jl	[r6]

	/*
	 * no need to clear callee regs, as they will be saved and restored
	 * automatically
	 */
	clear_scratch_regs

	mov r29, 0
	mov r30, 0


_arc_do_secure_call_exit:
	pop_s blink

	j [blink]
	/* enable normal interrupt */
	/*
	 * j.d [blink]
	 * seti (0x30 | (CONFIG_NUM_IRQ_PRIO_LEVELS - 1))
	 */

_s_clri:
	lr r0, [_ARC_V2_STATUS32]
	and r0, r0, 0x1e
	asr r0, r0
	or r0, r0, 0x30
	mov r6, (0x30 | (ARC_N_IRQ_START_LEVEL-1))

	j.d [blink]
	seti r6

_s_seti:
	btst r0, 4
	jnz __seti_0
	mov r0, (CONFIG_NUM_IRQ_PRIO_LEVELS - 1)
	lr r6, [_ARC_V2_STATUS32]
	and r6, r6, 0x1e
	asr r6, r6
	cmp r0, r6
	mov.hs r0, r6
__seti_0:
	and r0, r0, 0xf
	brhs r0, ARC_N_IRQ_START_LEVEL, __seti_1
	mov r0, ARC_N_IRQ_START_LEVEL
__seti_1:
	or r0, r0, 0x30

	j.d [blink]
	seti r0


SECTION_FUNC(TEXT, arc_go_to_normal)
	clear_callee_regs
	clear_scratch_regs

	mov fp, 0
	mov r29, 0
	mov r30, 0
	mov blink, 0

	jl [r0]
	/* should not come here */
	kflag 1

