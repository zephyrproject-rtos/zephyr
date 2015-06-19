/* offsets.c - ARM nano kernel structure member offset definition file */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
value represents the member offsets for various ARM nanokernel
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
#include <offsets/common.h>

/* ARM-specific tNANO structure member offsets */

GEN_OFFSET_SYM(tNANO, flags);
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GEN_OFFSET_SYM(tNANO, idle);
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/* ARM-specific tCCS structure member offsets */

GEN_OFFSET_SYM(tCCS, basepri);
#ifdef CONFIG_CONTEXT_CUSTOM_DATA
GEN_OFFSET_SYM(tCCS, custom_data);
#endif

/* ARM-specific ESF structure member offsets */

GEN_OFFSET_SYM(tESF, a1);
GEN_OFFSET_SYM(tESF, a2);
GEN_OFFSET_SYM(tESF, a3);
GEN_OFFSET_SYM(tESF, a4);
GEN_OFFSET_SYM(tESF, ip);
GEN_OFFSET_SYM(tESF, lr);
GEN_OFFSET_SYM(tESF, pc);
GEN_OFFSET_SYM(tESF, xpsr);

/* size of the entire tESF structure */

GEN_ABSOLUTE_SYM(__tESF_SIZEOF, sizeof(tESF));

/* ARM-specific preempt registers structure member offsets */

GEN_OFFSET_SYM(tPreempt, v1);
GEN_OFFSET_SYM(tPreempt, v2);
GEN_OFFSET_SYM(tPreempt, v3);
GEN_OFFSET_SYM(tPreempt, v4);
GEN_OFFSET_SYM(tPreempt, v5);
GEN_OFFSET_SYM(tPreempt, v6);
GEN_OFFSET_SYM(tPreempt, v7);
GEN_OFFSET_SYM(tPreempt, v8);
GEN_OFFSET_SYM(tPreempt, psp);

/* size of the entire preempt registers structure */

GEN_ABSOLUTE_SYM(__tPreempt_SIZEOF, sizeof(tPreempt));

/* size of the tCCS structure sans save area for floating point regs */

GEN_ABSOLUTE_SYM(__tCCS_NOFLOAT_SIZEOF, sizeof(tCCS));

GEN_ABS_SYM_END
