/* power.c - power management */

/*
 * Copyright (c) 2011-2015 Wind River Systems, Inc.
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
This module provides _sys_power_save_flag and _sys_power_save_idle(),
invoked within the microkernel idle loop.

\INTERNAL
*/

#if defined(CONFIG_MICROKERNEL)
#include <toolchain.h>
#include <sections.h>

unsigned char _sys_power_save_flag = 1;

#if defined(CONFIG_ADVANCED_POWER_MANAGEMENT)

#include <cputype.h>
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <microkernel/k_types.h>
#ifdef CONFIG_ADVANCED_IDLE
#include <advidle.h>
#endif
#if defined(CONFIG_TICKLESS_IDLE)
#include <drivers/system_timer.h>
#endif

extern void nano_cpu_idle(void);
extern void nano_cpu_set_idle(int32_t ticks);

#if defined(CONFIG_TICKLESS_IDLE)
/*
 * Idle time must be this value or higher for timer to go into tickless idle
 * state.
 */
int32_t _sys_idle_threshold_ticks =
	CONFIG_TICKLESS_IDLE_THRESH;
#endif /* CONFIG_TICKLESS_IDLE */

/*******************************************************************************
*
* _sys_power_save_idle - power management policy when kernel is idle
*
* This routine implements the power management policy based on the time
* until the timer expires, in system ticks.
* Routine is invoked from the idle task with interrupts disabled
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _sys_power_save_idle(int32_t ticks)
{
#if defined(CONFIG_TICKLESS_IDLE)
	if ((ticks == -1) || ticks >= _sys_idle_threshold_ticks) {
		/* Put the system timer into idle state until the next timer
		 * event */
		_timer_idle_enter(ticks);
	}
#endif /* CONFIG_TICKLESS_IDLE */
#ifdef CONFIG_ADVANCED_IDLE
	/*
	 * Call the advanced sleep function, which checks if the system should
	 * enter a deep sleep state. If so, the function will return a non-zero
	 * value when the system resumes here after the deep sleep ends.
	 * If the time to sleep is too short to go to advanced sleep mode, the
	 * function returns zero immediately and we do normal idle processing.
	 */

	if (_AdvIdleFunc(ticks) == 0) {
		nano_cpu_set_idle(ticks);
		nano_cpu_idle();
	}
#else
	nano_cpu_set_idle(ticks);
	nano_cpu_idle();
#endif /* CONFIG_ADVANCED_IDLE */
}

/*******************************************************************************
*
* _SysPowerSaveIdleExit - power management policy when kernel leaves idle state
*
* This routine implements the power management policy when kernel leaves idle
* state. Routine can be modified to wake up other devices.
* The routine is invoked from interrupt context, with interrupts disabled.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void _SysPowerSaveIdleExit(int32_t ticks)
{
#ifdef CONFIG_TICKLESS_IDLE
	if ((ticks == -1) || ticks >= _sys_idle_threshold_ticks) {
		_timer_idle_exit();
	}
#else
	ARG_UNUSED(ticks);
#endif /* CONFIG_TICKLESS_IDLE */
}

#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */
#endif /* CONFIG_MICROKERNEL */
