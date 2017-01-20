/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Cadence Design Systems, Inc.
 * SPDX-License-Identifier: Apache-2.0
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
 * @brief Xtensa kernel structure member offset definition file
 *
 * This module is responsible for the generation of the absolute symbols whose
 * value represents the member offsets for various Xtensa nanokernel
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

#include <gen_offset.h> /* located in kernel/arch/common/include */

/* list of headers that define whose structure offsets will be generated */

#include <kernel_structs.h>

#include <kernel_offsets.h>

/* Xtensa-specific TCS structure member offsets */
GEN_OFFSET_SYM(_callee_saved_t, topOfStack);
GEN_OFFSET_SYM(_callee_saved_t, retval);

GEN_OFFSET_SYM(_thread_arch_t, preempCoprocReg);
#if XCHAL_CP_NUM > 0
GEN_OFFSET_SYM(tPreempCoprocReg, cpStack);
#endif

/* Xtensa-specific _thread_arch_t structure member offsets */
GEN_OFFSET_SYM(_thread_arch_t, flags);
#ifdef CONFIG_SYS_POWER_MANAGEMENT
GEN_OFFSET_SYM(_thread_arch_t, idle);
#endif

#ifdef CONFIG_THREAD_CUSTOM_DATA
GEN_OFFSET_SYM(_thread_arch_t, custom_data);
#endif

/* Xtensa-specific ESF structure member offsets */
GEN_OFFSET_SYM(__esf_t, sp);
GEN_OFFSET_SYM(__esf_t, pc);

/* size of the entire __esf_t structure */
GEN_ABSOLUTE_SYM(____esf_t_SIZEOF, sizeof(__esf_t));

/* size of the entire preempt registers structure */
GEN_ABSOLUTE_SYM(__tPreempt_SIZEOF, sizeof(_caller_saved_t));

/* size of the struct tcs structure without save area for coproc regs */
GEN_ABSOLUTE_SYM(_K_THREAD_NO_FLOAT_SIZEOF,
		 sizeof(struct k_thread) - sizeof(tCoopCoprocReg) -
			 sizeof(tPreempCoprocReg));

GEN_ABS_SYM_END
