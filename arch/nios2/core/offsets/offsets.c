/*
 * Copyright (c) 2016 Intel Corporation
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
 * @brief Nios II nano kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various Nios II nanokernel
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

/* Nios II specific tNANO structure member offsets */
GEN_OFFSET_SYM(tNANO, irq_sp);
GEN_OFFSET_SYM(tNANO, nested);

/* struct coop member offsets */
GEN_OFFSET_SYM(t_coop, r16);
GEN_OFFSET_SYM(t_coop, r17);
GEN_OFFSET_SYM(t_coop, r18);
GEN_OFFSET_SYM(t_coop, r19);
GEN_OFFSET_SYM(t_coop, r20);
GEN_OFFSET_SYM(t_coop, r21);
GEN_OFFSET_SYM(t_coop, r22);
GEN_OFFSET_SYM(t_coop, r23);
GEN_OFFSET_SYM(t_coop, r28);
GEN_OFFSET_SYM(t_coop, ra);
GEN_OFFSET_SYM(t_coop, sp);
GEN_OFFSET_SYM(t_coop, key);
GEN_OFFSET_SYM(t_coop, retval);

/* size of the struct tcs structure sans save area for floating point regs */
GEN_ABSOLUTE_SYM(__tTCS_NOFLOAT_SIZEOF, sizeof(tTCS));

GEN_ABS_SYM_END
