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
 * @brief Exception/interrupt context helpers for Cortex-M CPUs
 *
 * Exception/interrupt context helpers.
 */

#ifndef _ARM_CORTEXM_ISR__H_
#define _ARM_CORTEXM_ISR__H_

#include <arch/cpu.h>
#include <asm_inline.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ASMLANGUAGE

/* nothing */

#else

/**
 *
 * @brief Find out if running in an ISR context
	 *
 * The current executing vector is found in the IPSR register. We consider the
 * IRQs (exception 16 and up), and the SVC, PendSV, and SYSTICK exceptions,
 * to be interrupts. Taking a fault within an exception is also considered in
 * interrupt context.
 *
 * @return 1 if in ISR, 0 if not.
 */
static ALWAYS_INLINE int _IsInIsr(void)
{
	uint32_t vector = _IpsrGet();

	/*
	 * IRQs + PendSV (14) + SVC (11) + SYSTICK (15) are interrupts.
	 * Vectors 12 and 13 are reserved, we'll never be in there
	 */
	return (vector > 10) || (vector && _ScbIsNestedExc());
}

/**
 * @brief Setup system exceptions
 *
 * Set exception priorities to conform with the BASEPRI locking mechanism.
 * Set PendSV priority to lowest possible.
 *
 * Enable fault exceptions.
 *
 * @return N/A
 */
static ALWAYS_INLINE void _ExcSetup(void)
{
	_ScbExcPrioSet(_EXC_PENDSV, _EXC_PRIO(0xff));
#if !defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
	_ScbExcPrioSet(_EXC_SVC, _EXC_PRIO(0x01));
	_ScbExcPrioSet(_EXC_MPU_FAULT, _EXC_PRIO(0x01));
	_ScbExcPrioSet(_EXC_BUS_FAULT, _EXC_PRIO(0x01));
	_ScbExcPrioSet(_EXC_USAGE_FAULT, _EXC_PRIO(0x01));

	_ScbUsageFaultEnable();
	_ScbBusFaultEnable();
	_ScbMemFaultEnable();
#endif /* !CONFIG_CPU_CORTEX_M0_M0PLUS */
}

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif


#endif /* _ARM_CORTEXM_ISR__H_ */
