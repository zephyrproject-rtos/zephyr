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

/**
 * @file
 * @brief Stack helpers for Cortex-M CPUs
 *
 * Stack helper functions.
 */

#ifndef _ARM_CORTEXM_STACK__H_
#define _ARM_CORTEXM_STACK__H_

#include <kernel_structs.h>
#include <asm_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_STACK_ALIGN_DOUBLE_WORD
#define STACK_ALIGN_SIZE 8
#else
#define STACK_ALIGN_SIZE 4
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

extern char _interrupt_stack[CONFIG_ISR_STACK_SIZE];

/**
 *
 * @brief Setup interrupt stack
 *
 * On Cortex-M, the interrupt stack is registered in the MSP (main stack
 * pointer) register, and switched to automatically when taking an exception.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _InterruptStackSetup(void)
{
	uint32_t msp = (uint32_t)(_interrupt_stack + CONFIG_ISR_STACK_SIZE);

	_MspSet(msp);
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _ARM_CORTEXM_STACK__H_ */
