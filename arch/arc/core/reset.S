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
 * @brief Reset handler
 *
 * Reset handler that prepares the system for running C code.
 */

#define _ASMLANGUAGE

// #include <board.h>
#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>

GDATA(_interrupt_stack)
GDATA(_firq_stack)
GDATA(_main_stack)

/* use one of the available interrupt stacks during init */

/* FIRQ only ? */
#if CONFIG_NUM_IRQ_PRIO_LEVELS == 1

	/* FIRQ, but uses _interrupt_stack ? */
	#if CONFIG_RGF_NUM_BANKS == 1
		#define INIT_STACK _interrupt_stack
		#define INIT_STACK_SIZE CONFIG_ISR_STACK_SIZE
	#else
		#define INIT_STACK _firq_stack
		#define INIT_STACK_SIZE CONFIG_FIRQ_STACK_SIZE
	#endif
#else
	#define INIT_STACK _interrupt_stack
	#define INIT_STACK_SIZE CONFIG_ISR_STACK_SIZE
#endif

GTEXT(__reset)
GTEXT(__start)

/**
 *
 * @brief Reset vector
 *
 * Ran when the system comes out of reset. The processor is at supervisor level.
 *
 * Locking interrupts prevents anything from interrupting the CPU.
 *
 * When these steps are completed, jump to _PrepC(), which will finish setting
 * up the system for running C code.
 *
 * @return N/A
 */

SECTION_FUNC(TEXT,__reset)
SECTION_FUNC(TEXT,__start)

	/* lock interrupts: will get unlocked when switch to main task */
	clri

#ifdef CONFIG_INIT_STACKS
	/*
	 * use the main stack to call memset on the interrupt stack and the
	 * FIRQ stack when CONFIG_INIT_STACKS is enabled before switching to
	 * one of them for the rest of the early boot
	 */
	mov sp, _main_stack
	add sp, sp, CONFIG_MAIN_STACK_SIZE

	mov_s r0, _interrupt_stack
	mov_s r1, 0xaa
	mov_s r2, CONFIG_ISR_STACK_SIZE
	jl memset

#if CONFIG_RGF_NUM_BANKS != 1
	mov_s r0, _firq_stack
	mov_s r1, 0xaa
	mov_s r2, CONFIG_FIRQ_STACK_SIZE
	jl memset
#endif

#endif /* CONFIG_INIT_STACKS */

	mov sp, INIT_STACK
	add sp, sp, INIT_STACK_SIZE

	j @_PrepC
