/* fault_s.S - fault handlers for ARCv2 */

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

/*
 * DESCRIPTION
 * Fault handlers for ARCv2 processors.
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#include "swap_macros.h"

GTEXT(_Fault)

GTEXT(__reset)
GTEXT(__memory_error)
GTEXT(__instruction_error)
GTEXT(__ev_machine_check)
GTEXT(__ev_tlb_miss_i)
GTEXT(__ev_tlb_miss_d)
GTEXT(__ev_prot_v)
GTEXT(__ev_privilege_v)
GTEXT(__ev_swi)
GTEXT(__ev_trap)
GTEXT(__ev_extension)
GTEXT(__ev_div_zero)
GTEXT(__ev_dc_error)
GTEXT(__ev_maligned)
GDATA(_firq_stack)

SECTION_VAR(BSS, saved_stack_pointer)
	.word 0

/*
 * @brief Fault handler installed in the fault and reserved vectors
 */

SECTION_SUBSEC_FUNC(TEXT,__fault,__memory_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__instruction_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_machine_check)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_tlb_miss_i)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_tlb_miss_d)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_prot_v)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_privilege_v)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_swi)
#ifndef CONFIG_IRQ_OFFLOAD
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
#endif
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_extension)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_div_zero)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_dc_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_maligned)

	/*
	 * Before invoking exception handler, the kernel switches to an exception
	 * stack, which is really the FIRQ stack, to save the faulting thread's
	 * registers. It can use the FIRQ stack because it knows it is unused
	 * since it is save to assume that if an exception has happened in FIRQ
	 * handler, the problem is fatal and all the kernel can do is just print
	 * a diagnostic message and halt.
	 */

	st sp, [saved_stack_pointer]
	mov_s sp, _firq_stack
	add sp, sp, CONFIG_FIRQ_STACK_SIZE

	/* save caller saved registers */
	_create_irq_stack_frame

	jl _Fault

	/* if _Fault returns, restore the registers */
	_pop_irq_stack_frame

	/* now restore the stack */
	ld sp,[saved_stack_pointer]
	rtie

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_irq_do_offload);

SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
	/*
	 * Before invoking exception handler, the kernel switches to an exception
	 * stack, which is really the FIRQ stack, to save the faulting thread's
	 * registers. It can use the FIRQ stack because it knows it is unused
	 * since it is save to assume that if an exception has happened in FIRQ
	 * handler, the problem is fatal and all the kernel can do is just print
	 * a diagnostic message and halt.
	 */

	st sp, [saved_stack_pointer]
	mov_s sp, _firq_stack
	add sp, sp, CONFIG_FIRQ_STACK_SIZE

	/* save caller saved registers */
	_create_irq_stack_frame

	jl _irq_do_offload

	/* if _Fault returns, restore the registers */
	_pop_irq_stack_frame

	/* now restore the stack */
	ld sp,[saved_stack_pointer]
	rtie
#endif /* CONFIG_IRQ_OFFLOAD */
