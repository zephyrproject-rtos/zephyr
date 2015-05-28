/* exc.h - exception/interrupt context helpers for Cortex-M CPUs */

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
Exception/interrupt context helpers.
*/

#ifndef _ARM_CORTEXM_ISR__H_
#define _ARM_CORTEXM_ISR__H_

#include <arch/cpu.h>
#include <asm_inline.h>

#ifdef _ASMLANGUAGE

/* nothing */

#else

/*******************************************************************************
*
* _IsInIsr - find out if running in an ISR context
*
* The current executing vector is found in the IPSR register. We consider the
* IRQs (exception 16 and up), and the PendSV and SYSTICK exceptions, to be
* interrupts. Taking a fault within an exception is also considered in
* interrupt context.
*
* RETURNS: 1 if in ISR, 0 if not.
*
* \NOMANUAL
*/
static ALWAYS_INLINE int _IsInIsr(void)
{
	uint32_t vector = _IpsrGet();

	/* IRQs + PendSV + SYSTICK are interrupts */
	return (vector > 13) || (vector && _ScbIsNestedExc());
}

/*******************************************************************************
* _ExcSetup - setup system exceptions
*
* Set exception priorities to conform with the BASEPRI locking mechanism.
* Set PendSV priority to lowest possible.
*
* Enable fault exceptions.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

static ALWAYS_INLINE void _ExcSetup(void)
{
	_ScbExcPrioSet(_EXC_PENDSV, _EXC_PRIO(0xff));
	_ScbExcPrioSet(_EXC_SVC, _EXC_PRIO(0x01));
	_ScbExcPrioSet(_EXC_MPU_FAULT, _EXC_PRIO(0x01));
	_ScbExcPrioSet(_EXC_BUS_FAULT, _EXC_PRIO(0x01));
	_ScbExcPrioSet(_EXC_USAGE_FAULT, _EXC_PRIO(0x01));

	_ScbUsageFaultEnable();
	_ScbBusFaultEnable();
	_ScbMemFaultEnable();
}

#endif /* _ASMLANGUAGE */

#endif /* _ARM_CORTEXM_ISR__H_ */
