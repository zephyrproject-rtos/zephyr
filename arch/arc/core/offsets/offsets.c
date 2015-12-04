/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief ARCv2 nano kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various ARCv2 nanokernel
 * structures.
 *
 * All of the absolute symbols defined by this module will be present in the
 * final microkernel or nanokernel ELF image (due to the linker's reference to
 * the _OffsetAbsSyms symbol).
 *
 * INTERNAL
 * It is NOT necessary to define the offset for every member of a structure.
 * Typically, only those members that are accessed by assembly language routines
 * are defined; however, it doesn't hurt to define all fields for the sake of
 * completeness.
 */

#include <gen_offset.h>
#include <nano_private.h>
#include <nano_offsets.h>

/* ARCv2-specific tNANO structure member offsets */
GEN_OFFSET_SYM(tNANO, rirq_sp);
GEN_OFFSET_SYM(tNANO, firq_regs);
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GEN_OFFSET_SYM(tNANO, idle);
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/* ARCv2-specific struct tcs structure member offsets */
GEN_OFFSET_SYM(tTCS, intlock_key);
GEN_OFFSET_SYM(tTCS, relinquish_cause);
GEN_OFFSET_SYM(tTCS, return_value);
#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(tTCS, custom_data);
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

/* size of the struct tcs structure sans save area for floating point regs */
GEN_ABSOLUTE_SYM(__tTCS_NOFLOAT_SIZEOF, sizeof(tTCS));

GEN_ABS_SYM_END
