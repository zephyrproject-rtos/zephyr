/* cpu_idle.S - ARM CORTEX-M3 power management */

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
*/

#define _ASMLANGUAGE

#include <offsets.h>
#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#ifdef CONFIG_TICKLESS_IDLE
#include <nano_private.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(_CpuIdleInit)
#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT
GTEXT(_NanoIdleValGet)
GTEXT(_NanoIdleValClear)
#endif
GTEXT(nano_cpu_idle)
GTEXT(nano_cpu_atomic_idle)

#define _SCR_INIT_BITS _SCB_SCR_SEVONPEND

/*******************************************************************************
*
* _CpuIdleInit - initialization of CPU idle
*
* Only called by nanoArchInit(). Sets SEVONPEND bit once for the system's
* duration.
*
* RETURNS: N/A
*
* C function prototype:
*
* void _CpuIdleInit (void);
*/

SECTION_FUNC(TEXT, _CpuIdleInit)
    ldr r1, =_SCB_SCR
    movs.n r2, #_SCR_INIT_BITS
    str r2, [r1]
    bx lr

#ifdef CONFIG_ADVANCED_POWER_MANAGEMENT

/*******************************************************************************
*
* _NanoIdleValGet - get the kernel idle setting
*
* Returns the nanokernel idle setting, in ticks. Only called by __systick().
*
* RETURNS: the requested number of ticks for the kernel to be idle
*
* C function prototype:
*
* int32_t _NanoIdleValGet (void);
*/

SECTION_FUNC(TEXT, _NanoIdleValGet)
    ldr r0, =_nanokernel
    ldr r0, [r0, #__tNANO_idle_OFFSET]
    bx lr

/*******************************************************************************
*
* _NanoIdleValClear - clear the kernel idle setting
*
* Sets the nanokernel idle setting to 0. Only called by __systick().
*
* RETURNS: N/A
*
* C function prototype:
*
* void _NanoIdleValClear (void);
*/

SECTION_FUNC(TEXT, _NanoIdleValClear)
    ldr r0, =_nanokernel
    eors.n r1, r1
    str r1, [r0, #__tNANO_idle_OFFSET]
    bx lr

#endif /* CONFIG_ADVANCED_POWER_MANAGEMENT */

/*******************************************************************************
*
* nano_cpu_idle - power save idle routine for ARM Cortex-M
*
* This function will be called by the nanokernel idle loop or possibly within
* an implementation of _sys_power_save_idle in the microkernel when the
* '_sys_power_save_flag' variable is non-zero.  The ARM 'wfi' instruction
* will be issued, causing a low-power consumption sleep mode.
*
* RETURNS: N/A
*
* C function prototype:
*
* void nano_cpu_idle (void);
*/

SECTION_FUNC(TEXT, nano_cpu_idle)
    /* clear BASEPRI so wfi is awakened by incoming interrupts */
    eors.n r0, r0
    msr BASEPRI, r0

    wfi

    bx lr

/*******************************************************************************
*
* nano_cpu_atomic_idle - atomically re-enable interrupts and enter low power mode
*
* This function is utilized by the nanokernel object "wait" APIs for task
* contexts, e.g. nano_task_lifo_get_wait(), nano_task_sem_take_wait(), nano_task_stack_pop_wait(),
* and nano_task_fifo_get_wait().
*
* INTERNAL
* The requirements for nano_cpu_atomic_idle() are as follows:
* 1) The enablement of interrupts and entering a low-power mode needs to be
*    atomic, i.e. there should be no period of time where interrupts are
*    enabled before the processor enters a low-power mode.  See the comments
*    in nano_task_lifo_get_wait(), for example, of the race condition that occurs
*    if this requirement is not met.
*
* 2) After waking up from the low-power mode, the interrupt lockout state
*    must be restored as indicated in the 'imask' input parameter.
*
* RETURNS: N/A
*
* C function prototype:
*
* void nano_cpu_atomic_idle (unsigned int imask);
*/

SECTION_FUNC(TEXT, nano_cpu_atomic_idle)

    /* 
     * r0: interrupt mask from caller
     * r1: zero, for setting BASEPRI (needs a register)
     */

    eors.n r1, r1

    /* 
     * Lock PRIMASK while sleeping: wfe will still get interrupted by incoming
     * interrupts but the CPU will not service them right away.
     */
    cpsid i

    /*
     * No need to set SEVONPEND, it's set once in _CpuIdleInit() and never
     * touched again.
     */

    /* unlock BASEPRI so wfe gets interrupted by incoming interrupts */
    msr BASEPRI, r1

    wfe

    msr BASEPRI, r0
    cpsie i
    bx lr
