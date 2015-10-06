/* offsets.c - ARM nano kernel structure member offset definition file */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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
#include <nano_offsets.h>

/* ARM-specific tNANO structure member offsets */

GEN_OFFSET_SYM(tNANO, flags);
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GEN_OFFSET_SYM(tNANO, idle);
#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/* ARM-specific struct tcs structure member offsets */

GEN_OFFSET_SYM(tTCS, basepri);
#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(tTCS, custom_data);
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

/* size of the struct tcs structure sans save area for floating point regs */

GEN_ABSOLUTE_SYM(__tTCS_NOFLOAT_SIZEOF, sizeof(tTCS));

GEN_ABS_SYM_END
