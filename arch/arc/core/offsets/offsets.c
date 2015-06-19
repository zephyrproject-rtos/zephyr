/* offsets.c - ARCv2 nano kernel structure member offset definition file */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module is responsible for the generation of the absolute symbols whose
value represents the member offsets for various ARCv2 nanokernel
structures.

All of the absolute symbols defined by this module will be present in the
final microkernel or nanokernel ELF image (due to the linker's reference to
the _OffsetAbsSyms symbol).

INTERNAL
It is NOT necessary to define the offset for every member of a structure.
Typically, only those members that are accessed by assembly language routines
are defined; however, it doesn't hurt to define all fields for the sake of
completeness.

*/

#include <gen_offset.h>
#include <nano_private.h>
#include <nano_offsets.h>

/* ARCv2-specific tCCS structure member offsets */
GEN_OFFSET_SYM(tNANO, rirq_sp);
GEN_OFFSET_SYM(tNANO, firq_regs);
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GEN_OFFSET_SYM(tNANO, idle);
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/* ARCv2-specific tCCS structure member offsets */
GEN_OFFSET_SYM(tCCS, intlock_key);
GEN_OFFSET_SYM(tCCS, relinquish_cause);
GEN_OFFSET_SYM(tCCS, return_value);
#ifdef CONFIG_CONTEXT_CUSTOM_DATA
GEN_OFFSET_SYM(tCCS, custom_data);
#endif


/* ARCv2-specific IRQ stack frame structure member offsets */
GEN_OFFSET_SYM(tISF, r0);
GEN_OFFSET_SYM(tISF, r1);
GEN_OFFSET_SYM(tISF, r2);
GEN_OFFSET_SYM(tISF, r3);
GEN_OFFSET_SYM(tISF, r4);
GEN_OFFSET_SYM(tISF, r5);
GEN_OFFSET_SYM(tISF, r6);
GEN_OFFSET_SYM(tISF, r7);
GEN_OFFSET_SYM(tISF, r8);
GEN_OFFSET_SYM(tISF, r9);
GEN_OFFSET_SYM(tISF, r10);
GEN_OFFSET_SYM(tISF, r11);
GEN_OFFSET_SYM(tISF, r12);
GEN_OFFSET_SYM(tISF, r13);
GEN_OFFSET_SYM(tISF, blink);
GEN_OFFSET_SYM(tISF, lp_end);
GEN_OFFSET_SYM(tISF, lp_start);
GEN_OFFSET_SYM(tISF, lp_count);
GEN_OFFSET_SYM(tISF, pc);
GEN_OFFSET_SYM(tISF, status32);
GEN_ABSOLUTE_SYM(__tISF_SIZEOF, sizeof(tISF));

/* ARCv2-specific preempt registers structure member offsets */
GEN_OFFSET_SYM(tPreempt, sp);
GEN_ABSOLUTE_SYM(__tPreempt_SIZEOF, sizeof(tPreempt));

/* ARCv2-specific callee-saved stack */
GEN_OFFSET_SYM(tCalleeSaved, r13);
GEN_OFFSET_SYM(tCalleeSaved, r14);
GEN_OFFSET_SYM(tCalleeSaved, r15);
GEN_OFFSET_SYM(tCalleeSaved, r16);
GEN_OFFSET_SYM(tCalleeSaved, r17);
GEN_OFFSET_SYM(tCalleeSaved, r18);
GEN_OFFSET_SYM(tCalleeSaved, r19);
GEN_OFFSET_SYM(tCalleeSaved, r20);
GEN_OFFSET_SYM(tCalleeSaved, r21);
GEN_OFFSET_SYM(tCalleeSaved, r22);
GEN_OFFSET_SYM(tCalleeSaved, r23);
GEN_OFFSET_SYM(tCalleeSaved, r24);
GEN_OFFSET_SYM(tCalleeSaved, r25);
GEN_OFFSET_SYM(tCalleeSaved, r26);
GEN_OFFSET_SYM(tCalleeSaved, fp);
GEN_OFFSET_SYM(tCalleeSaved, r30);
GEN_ABSOLUTE_SYM(__tCalleeSaved_SIZEOF, sizeof(tCalleeSaved));

/* ARCv2-specific registers-saved-in-FIRQ structure member offsets */
GEN_OFFSET_SYM(tFirqRegs, lp_count);
GEN_OFFSET_SYM(tFirqRegs, lp_start);
GEN_OFFSET_SYM(tFirqRegs, lp_end);
GEN_ABSOLUTE_SYM(__tFirqRegs_SIZEOF, sizeof(tFirqRegs));

/* size of the tCCS structure sans save area for floating point regs */
GEN_ABSOLUTE_SYM(__tCCS_NOFLOAT_SIZEOF, sizeof(tCCS));

GEN_ABS_SYM_END
