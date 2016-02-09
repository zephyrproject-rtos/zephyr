/* advidle.h - header file for custom advanced idle manager */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
 * @brief Power management hooks
 *
 * This header file specifies the Power Management hook interface.
 * All of the APIs declared here must be supplied by the Power Manager
 * application, namely the _sys_soc_suspend() and _sys_soc_resume()
 * functions.
 */

#ifndef __INCadvidle
#define __INCadvidle

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ADVANCED_IDLE

/**
 * @brief Exit deep sleep, low power or tickless idle states
 *
 * The main purpose of this routine is to notify exit from
 * deep sleep, low power or tickless idle. States altered
 * at _sys_soc_suspend() should be restored in this function.
 * This can be called under following conditions each of which
 * require different handling.
 *
 * Deep sleep recovery:
 * App should save information in SoC at _sys_soc_suspend() that
 * will persist across deep sleep.  This function should check
 * that information to identify deep sleep recovery.  In this case
 * this function will restore states and resume execution at the
 * point were system entered deep sleep. In this mode, this
 * function is called with the interrupt stack. It is important
 * that this function, before interrupts are enabled, restores
 * the stack that was in use when system went to deep sleep. This
 * is to avoid interfering interrupt handlers use of this stack.
 *
 * Cold boot:
 * Cold boot and deep sleep recovery happen at the same location.
 * The function identifies it is a cold boot if it does not find
 * state information indicating deep sleep, low power state or
 * tickless idle. In this case the function returns immediately.
 *
 * Low power recovery:
 * Low power is entered by turning off peripherals, gating clocks
 * and entering a low power CPU state like C2.  This state is exited by
 * an interrupt.  In this case this function would be called from
 * the interrupt's context.  Any peripherals turned off at
 * suspend should be turned back on in this function.
 *
 * Tickless idle exit:
 * This function will also be called at exit of kernel's tickless
 * idle. Restore any states altered in _sys_soc_suspend().
 *
 * @return will not return to caller if deep sleep recovery
 */
extern void _sys_soc_resume(void);

/**
 * @brief Enter deep sleep, low power or tickless idle states
 *
 * This routine is called by the kernel when it is about to idle.
 * This routine is passed the number of clock ticks that the kernel
 * calculated as available time to idle. This function should compare
 * this time with the wake latencies of the various power saving schemes
 * and use the best one that fits. The power saving schemes use the
 * following modes.
 *
 * Deep Sleep:
 * This turns off the core voltage rail and core clock.  This would save
 * most power but would also have a high wake latency. CPU loses state
 * so this function should save CPU states and the location in this
 * function where system should resume execution at resume. Function
 * should re-enable interrupts and return a non-zero value.
 *
 * Low Power:
 * Peripherals can be turned off and clocks can be gated depending on
 * time available. Then swithes to CPU low power state.  In this state
 * the CPU is still active but in a low power state and does not lose
 * any state. This state is exited by an interrupt from where the
 * _sys_soc_resume() will be called.  To allow the interrupt,
 * this function should ensure that interrupts are atomically
 * enabled before going to the low power CPU state.  This function
 * should return a non-zero value to indicate it was handled and kernel
 * should not do its own CPU idle. Interrupts should be enabled on exit.
 *
 * Tickless Idle:
 * This routine can take advantage of the kernel's tickless idle logic
 * by turning off peripherals and clocks depending on available time.
 * It can return zero to indicate the kernel should do its own CPU idle.
 * After the tickless idle wait is completed or if any interrupt occurs,
 * the _sys_soc_resume() function will be called to allow restoring
 * altered states. Function should return zero. Interrupts should not
 * be turned on.
 *
 * If this function decides to not do any operation then it should
 * return zero to let kernel do its idle wait.
 *
 * This function is entered with interrupts disabled.  It should
 * re-enable interrupts if it returns non-zero value i.e. if it
 * does its own CPU low power wait or deep sleep.
 *
 * @param ticks the upcoming kernel idle time
 *
 * @return  non-zero value if deep sleep or CPU low power entered
 */
extern int _sys_soc_suspend(int32_t ticks);

#endif /* CONFIG_ADVANCED_IDLE */

#ifdef __cplusplus
}
#endif

#endif /* __INCadvidle */
