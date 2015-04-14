/* stack.h - stack helpers for Cortex-M CPUs */

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
Stack helper functions.
*/

#ifndef _ARM_CORTEXM_STACK__H_
#define _ARM_CORTEXM_STACK__H_

#include <nanok.h>

#ifdef CONFIG_STACK_GROWS_DOWN
#define STACK_DIR STACK_GROWS_DOWN
#else
#define STACK_DIR STACK_GROWS_UP
#endif

#ifdef CONFIG_STACK_ALIGN_DOUBLE_WORD
#define STACK_ALIGN_SIZE 8
#else
#define STACK_ALIGN_SIZE 4
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

#if (STACK_DIR == STACK_GROWS_DOWN)
#define __GET_MSP() \
	STACK_ROUND_DOWN(&_InterruptStack[CONFIG_ISR_STACK_SIZE - 1])
#else
#define __GET_MSP() STACK_ROUND_UP(&_InterruptStack[0])
#endif

extern char _InterruptStack[CONFIG_ISR_STACK_SIZE];

/*******************************************************************************
*
* _MspSet - set the value of the Main Stack Pointer register
*
* Store the value of <msp> in MSP register.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

#if defined(__GNUC__)
static ALWAYS_INLINE void _MspSet(uint32_t msp /* value to store in MSP */
				  )
{
	__asm__ volatile("msr MSP, %0\n\t" :  : "r"(msp));
}
#elif defined(__DCC__)
__asm volatile void _MspSet(uint32_t msp)
{
	% reg msp !"r0" msr MSP, msp
}
#endif

/*******************************************************************************
*
* _InterruptStackSetup - setup interrupt stack
*
* On Cortex-M, the interrupt stack is registered in the MSP (main stack
* pointer) register, and switched to automatically when taking an exception.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static ALWAYS_INLINE void _InterruptStackSetup(void)
{
	uint32_t msp = __GET_MSP();

	_MspSet(msp);
}

#endif /* _ASMLANGUAGE */

#endif /* _ARM_CORTEXM_STACK__H_ */
