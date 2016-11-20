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

	mov sp, INIT_STACK
	add sp, sp, INIT_STACK_SIZE

	j @_PrepC
