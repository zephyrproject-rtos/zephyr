/* offsets.c - nanokernel structure member offset definition file */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various IA-32 nanokernel structures.
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

#include <gen_offset.h> /* located in kernel/arch/common/include */

/* list of headers that define whose structure offsets will be generated */

#include <nano_private.h>
#include <swapstk.h>


#include <nano_offsets.h>

/* Intel-specific tNANO structure member offsets */

GEN_OFFSET_SYM(tNANO, nested);
GEN_OFFSET_SYM(tNANO, common_isp);
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GEN_OFFSET_SYM(tNANO, idle);
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/* Intel-specific struct tcs structure member offsets */

#ifdef CONFIG_GDB_INFO
GEN_OFFSET_SYM(tTCS, esfPtr);
#endif /* CONFIG_GDB_INFO */
#if (defined(CONFIG_FP_SHARING) || defined(CONFIG_GDB_INFO))
GEN_OFFSET_SYM(tTCS, excNestCount);
#endif /* CONFIG_FP_SHARING || CONFIG_GDB_INFO */
#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(tTCS, custom_data);     /* available for custom use */
#endif
GEN_OFFSET_SYM(tTCS, coopFloatReg);   /* start of coop FP register set */
GEN_OFFSET_SYM(tTCS, preempFloatReg); /* start of prempt FP register set */

/* size of the struct tcs structure sans save area for floating point regs */

GEN_ABSOLUTE_SYM(__tTCS_NOFLOAT_SIZEOF,
		 sizeof(tTCS) - sizeof(tCoopFloatReg) -
			 sizeof(tPreempFloatReg));

/* tCoopReg structure member offsets: tTCS->coopReg is of type tCoopReg */

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

GEN_OFFSET_SYM(NANO_ESF, esp);
GEN_OFFSET_SYM(NANO_ESF, ebp);
GEN_OFFSET_SYM(NANO_ESF, ebx);
GEN_OFFSET_SYM(NANO_ESF, esi);
GEN_OFFSET_SYM(NANO_ESF, edi);
GEN_OFFSET_SYM(NANO_ESF, edx);
GEN_OFFSET_SYM(NANO_ESF, ecx);
GEN_OFFSET_SYM(NANO_ESF, eax);
GEN_OFFSET_SYM(NANO_ESF, errorCode);
GEN_OFFSET_SYM(NANO_ESF, eip);
GEN_OFFSET_SYM(NANO_ESF, cs);
GEN_OFFSET_SYM(NANO_ESF, eflags);

/* tTaskStateSegment structure member offsets */


/* size of the ISR_LIST structure. Used by linker scripts */

GEN_ABSOLUTE_SYM(__ISR_LIST_SIZEOF, sizeof(ISR_LIST));


GEN_ABS_SYM_END
