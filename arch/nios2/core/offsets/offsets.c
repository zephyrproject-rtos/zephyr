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
#include <kernel_structs.h>
#include <kernel_offsets.h>

/* struct coop member offsets */
GEN_OFFSET_SYM(_callee_saved_t, r16);
GEN_OFFSET_SYM(_callee_saved_t, r17);
GEN_OFFSET_SYM(_callee_saved_t, r18);
GEN_OFFSET_SYM(_callee_saved_t, r19);
GEN_OFFSET_SYM(_callee_saved_t, r20);
GEN_OFFSET_SYM(_callee_saved_t, r21);
GEN_OFFSET_SYM(_callee_saved_t, r22);
GEN_OFFSET_SYM(_callee_saved_t, r23);
GEN_OFFSET_SYM(_callee_saved_t, r28);
GEN_OFFSET_SYM(_callee_saved_t, ra);
GEN_OFFSET_SYM(_callee_saved_t, sp);
GEN_OFFSET_SYM(_callee_saved_t, key);
GEN_OFFSET_SYM(_callee_saved_t, retval);

GEN_OFFSET_SYM(NANO_ESF, ra);
GEN_OFFSET_SYM(NANO_ESF, r1);
GEN_OFFSET_SYM(NANO_ESF, r2);
GEN_OFFSET_SYM(NANO_ESF, r3);
GEN_OFFSET_SYM(NANO_ESF, r4);
GEN_OFFSET_SYM(NANO_ESF, r5);
GEN_OFFSET_SYM(NANO_ESF, r6);
GEN_OFFSET_SYM(NANO_ESF, r7);
GEN_OFFSET_SYM(NANO_ESF, r8);
GEN_OFFSET_SYM(NANO_ESF, r9);
GEN_OFFSET_SYM(NANO_ESF, r10);
GEN_OFFSET_SYM(NANO_ESF, r11);
GEN_OFFSET_SYM(NANO_ESF, r12);
GEN_OFFSET_SYM(NANO_ESF, r13);
GEN_OFFSET_SYM(NANO_ESF, r14);
GEN_OFFSET_SYM(NANO_ESF, r15);
GEN_OFFSET_SYM(NANO_ESF, estatus);
GEN_OFFSET_SYM(NANO_ESF, instr);
GEN_ABSOLUTE_SYM(__NANO_ESF_SIZEOF, sizeof(NANO_ESF));

/* size of the struct tcs structure sans save area for floating point regs */
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF, sizeof(struct k_thread));

GEN_ABS_SYM_END
