/* offsets.c - nanokernel structure member offset definition file */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
value represents the member offsets for various IA-32 nanokernel structures.

All of the absolute symbols defined by this module will be present in the
final microkernel or nanokernel ELF image (due to the linker's reference to
the _OffsetAbsSyms symbol).

INTERNAL
It is NOT necessary to define the offset for every member of a structure.
Typically, only those members that are accessed by assembly language routines
are defined; however, it doesn't hurt to define all fields for the sake of
completeness.

*/

#include <genOffset.h> /* located in kernel/arch/common/include */

/* list of headers that define whose structure offsets will be generated */

#include <nanok.h>
#include <swapstk.h>


#include <offsets/common.h>

/* Intel-specific tNANO structure member offsets */

GEN_OFFSET_SYM(tNANO, nested);
GEN_OFFSET_SYM(tNANO, common_isp);
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GEN_OFFSET_SYM(tNANO, idle);
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/* Intel-specific tCCS structure member offsets */

#ifdef CONFIG_GDB_INFO
GEN_OFFSET_SYM(tCCS, esfPtr);
#endif /* CONFIG_GDB_INFO */
#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
GEN_OFFSET_SYM(tCCS, excNestCount);
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */
#ifdef CONFIG_CONTEXT_CUSTOM_DATA
GEN_OFFSET_SYM(tCCS, custom_data);     /* available for custom use */
#endif
GEN_OFFSET_SYM(tCCS, coopFloatReg);   /* start of coop FP register set */
GEN_OFFSET_SYM(tCCS, preempFloatReg); /* start of prempt FP register set */

/* size of the tCCS structure sans save area for floating point regs */

GEN_ABSOLUTE_SYM(__tCCS_NOFLOAT_SIZEOF,
		 sizeof(tCCS) - sizeof(tCoopFloatReg) -
			 sizeof(tPreempFloatReg));

/* tCoopReg structure member offsets: tCCS->coopReg is of type tCoopReg */

GEN_OFFSET_SYM(tCoopReg, esp);

/* tSwapStk structure member offsets */

GEN_OFFSET_SYM(tSwapStk, eax);
GEN_OFFSET_SYM(tSwapStk, ebp);
GEN_OFFSET_SYM(tSwapStk, ebx);
GEN_OFFSET_SYM(tSwapStk, esi);
GEN_OFFSET_SYM(tSwapStk, edi);
GEN_OFFSET_SYM(tSwapStk, retAddr);
GEN_OFFSET_SYM(tSwapStk, param);

/* size of the entire tSwapStk structure */

GEN_ABSOLUTE_SYM(__tSwapStk_SIZEOF, sizeof(tSwapStk));

/* NANO_ESF structure member offsets */

#ifdef CONFIG_GDB_INFO
GEN_OFFSET_SYM(NANO_ESF, ebp);
GEN_OFFSET_SYM(NANO_ESF, ebx);
GEN_OFFSET_SYM(NANO_ESF, esi);
GEN_OFFSET_SYM(NANO_ESF, edi);
#endif /* CONFIG_GDB_INFO */
GEN_OFFSET_SYM(NANO_ESF, cr2);
GEN_OFFSET_SYM(NANO_ESF, edx);
GEN_OFFSET_SYM(NANO_ESF, ecx);
GEN_OFFSET_SYM(NANO_ESF, eax);
GEN_OFFSET_SYM(NANO_ESF, errorCode);
GEN_OFFSET_SYM(NANO_ESF, eip);
GEN_OFFSET_SYM(NANO_ESF, cs);
GEN_OFFSET_SYM(NANO_ESF, eflags);
GEN_OFFSET_SYM(NANO_ESF, esp);
GEN_OFFSET_SYM(NANO_ESF, ss);

/* tTaskStateSegment structure member offsets */


/* size of the ISR_LIST structure. Used by linker scripts */

GEN_ABSOLUTE_SYM(__ISR_LIST_SIZEOF, sizeof(ISR_LIST));


GEN_ABS_SYM_END
