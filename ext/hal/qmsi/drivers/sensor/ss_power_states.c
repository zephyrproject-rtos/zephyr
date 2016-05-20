/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ss_power_states.h"

/* Sensor Subsystem sleep operand definition.
 * Only a subset applies as internal sensor RTC
 * is not available.
 *
 *  OP | Core | Timers | RTC
 * 000 |    0 |      1 |   1 <-- used for SS1
 * 001 |    0 |      0 |   1
 * 010 |    0 |      1 |   0
 * 011 |    0 |      0 |   0 <-- used for SS2
 * 100 |    0 |      0 |   0
 * 101 |    0 |      0 |   0
 * 110 |    0 |      0 |   0
 * 111 |    0 |      0 |   0
 *
 * sleep opcode argument:
 *  - [7:5] : Sleep Operand
 *  - [4]   : Interrupt enable
 *  - [3:0] : Interrupt threshold value
 */
#define QM_SS_SLEEP_MODE_CORE_OFF (0x0)
#define QM_SS_SLEEP_MODE_CORE_OFF_TIMER_OFF (0x20)
#define QM_SS_SLEEP_MODE_CORE_TIMERS_RTC_OFF (0x60)

/* Enter SS1 :
 * SLEEP + sleep operand
 * __builtin_arc_sleep is not used here as it does not propagate sleep operand.
 */
void ss_power_cpu_ss1(const ss_power_cpu_ss1_mode_t mode)
{
	/* Enter SS1 */
	switch (mode) {
	case SS_POWER_CPU_SS1_TIMER_OFF:
		__asm__ __volatile__(
		    "sleep %0"
		    :
		    : "i"(QM_SS_SLEEP_MODE_CORE_OFF_TIMER_OFF));
		break;
	case SS_POWER_CPU_SS1_TIMER_ON:
	default:
		__asm__ __volatile__("sleep %0"
				     :
				     : "i"(QM_SS_SLEEP_MODE_CORE_OFF));
		break;
	}
}

/* Enter SS2 :
 * SLEEP + sleep operand
 * __builtin_arc_sleep is not used here as it does not propagate sleep operand.
 */
void ss_power_cpu_ss2(void)
{
	/* Enter SS2 */
	__asm__ __volatile__("sleep %0"
			     :
			     : "i"(QM_SS_SLEEP_MODE_CORE_TIMERS_RTC_OFF));
}
