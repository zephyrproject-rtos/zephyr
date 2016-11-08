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
 * @brief ARM CORTEX-M3 power management
 *
 */

#define _ASMLANGUAGE

#include <offsets_short.h>
#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#ifdef CONFIG_TICKLESS_IDLE
#include <kernel_structs.h>
#endif

_ASM_FILE_PROLOGUE

GTEXT(_CpuIdleInit)
#ifdef CONFIG_SYS_POWER_MANAGEMENT
GTEXT(_NanoIdleValGet)
GTEXT(_NanoIdleValClear)
#endif
GTEXT(nano_cpu_idle)
GTEXT(nano_cpu_atomic_idle)

#define _SCR_INIT_BITS _SCB_SCR_SEVONPEND

/**
 *
 * @brief Initialization of CPU idle
 *
 * Only called by nanoArchInit(). Sets SEVONPEND bit once for the system's
 * duration.
 *
 * @return N/A
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

#ifdef CONFIG_SYS_POWER_MANAGEMENT

/**
 *
 * @brief Get the kernel idle setting
 *
 * Returns the nanokernel idle setting, in ticks. Only called by __systick().
 *
 * @return the requested number of ticks for the kernel to be idle
 *
 * C function prototype:
 *
 * int32_t _NanoIdleValGet (void);
 */

SECTION_FUNC(TEXT, _NanoIdleValGet)
    ldr r0, =_kernel
    ldr r0, [r0, #_kernel_offset_to_idle]
    bx lr

/**
 *
 * @brief Clear the kernel idle setting
 *
 * Sets the nanokernel idle setting to 0. Only called by __systick().
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void _NanoIdleValClear (void);
 */

SECTION_FUNC(TEXT, _NanoIdleValClear)
    ldr r0, =_kernel
    eors.n r1, r1
    str r1, [r0, #_kernel_offset_to_idle]
    bx lr

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

/**
 *
 * @brief Power save idle routine for ARM Cortex-M
 *
 * This function will be called by the nanokernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the microkernel when the
 * '_sys_power_save_flag' variable is non-zero.  The ARM 'wfi' instruction
 * will be issued, causing a low-power consumption sleep mode.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void nano_cpu_idle (void);
 */

SECTION_FUNC(TEXT, nano_cpu_idle)
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	push {lr}
	bl    _sys_k_event_logger_enter_sleep
	pop {r0}
	mov lr, r0
#endif

#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    cpsie i
#else /* CONFIG_CPU_CORTEX_M3_M4 */
    /* clear BASEPRI so wfi is awakened by incoming interrupts */
    eors.n r0, r0
    msr BASEPRI, r0
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS */

    wfi

    bx lr

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * This function is utilized by the nanokernel object "wait" APIs for tasks,
 * e.g. nano_task_lifo_get(), nano_task_sem_take(),
 * nano_task_stack_pop(), and nano_task_fifo_get().
 *
 * INTERNAL
 * The requirements for nano_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in nano_task_lifo_get(), for example, of the race condition that occurs
 *    if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 *
 * C function prototype:
 *
 * void nano_cpu_atomic_idle (unsigned int imask);
 */

SECTION_FUNC(TEXT, nano_cpu_atomic_idle)
#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
	push {lr}
	bl    _sys_k_event_logger_enter_sleep
	pop {r1}
	mov lr, r1
#endif

    /*
     * Lock PRIMASK while sleeping: wfe will still get interrupted by incoming
     * interrupts but the CPU will not service them right away.
     */
    cpsid i

    /*
     * No need to set SEVONPEND, it's set once in _CpuIdleInit() and never
     * touched again.
     */

     /* r0: interrupt mask from caller */

#if defined(CONFIG_CPU_CORTEX_M0_M0PLUS)
    /* No BASEPRI, call wfe directly (SEVONPEND set in _CpuIdleInit()) */
    wfe

    cmp r0, #0
    bne _irq_disabled
    cpsie i
_irq_disabled:

#else /* CONFIG_CPU_CORTEX_M3_M4 */
    /* r1: zero, for setting BASEPRI (needs a register) */
    eors.n r1, r1

    /* unlock BASEPRI so wfe gets interrupted by incoming interrupts */
    msr BASEPRI, r1

    wfe

    msr BASEPRI, r0
    cpsie i
#endif /* CONFIG_CPU_CORTEX_M0_M0PLUS  */
    bx lr
