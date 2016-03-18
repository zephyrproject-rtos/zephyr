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

#ifndef __INCpower
#define __INCpower

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SYS_POWER_MANAGEMENT

/* Constants identifying power policies */
#define SYS_PM_NOT_HANDLED		0 /* No PM operations */
#define SYS_PM_DEVICE_SUSPEND_ONLY	1 /* Only Devices are suspended */
#define SYS_PM_LOW_POWER_STATE		2 /* Low Power State */
#define SYS_PM_DEEP_SLEEP		4 /* Deep Sleep */

/**
 * @brief Hook function to notify exit of a power policy
 *
 * The purpose of this function is to notify exit from
 * deep sleep, low power state or device suspend only policy.
 * States altered at _sys_soc_suspend() should be restored in this
 * function. Exit from each policy requires different handling as
 * follows.
 *
 * Deep sleep policy exit:
 * App should save information in SoC at _sys_soc_suspend() that
 * will persist across deep sleep. This function should check
 * that information to identify deep sleep recovery. In this case
 * this function will restore states and resume execution at the
 * point were system entered deep sleep. In this mode, this
 * function is called with the interrupt stack. It is important
 * that this function, before interrupts are enabled, restores
 * the stack that was in use when system went to deep sleep. This
 * is to avoid interfering interrupt handlers use of this stack.
 *
 * Cold boot and deep sleep recovery happen at the same location.
 * Since kernel does not store deep sleep state, kernel will call
 * this function in both cases. It is the responsibility of the
 * power manager application to identify whether it is cold boot
 * or deep sleep exit using state information that it stores.
 * If the function detects cold boot, then it returns immediately.
 *
 * Low power state policy exit:
 * Low power state policy does a CPU idle wait using a low power
 * CPU idle state supported by the processor. This state is exited
 * by an interrupt.  In this case this function would be called
 * from the interrupt's ISR context.  Any state altered at
 * _sys_soc_suspend should be restored and the function should
 * return quickly.
 *
 * Device suspend only policy exit:
 * This function will be called at the exit of kernel's CPU idle
 * wait if device suspend only policy was used. Resume operations
 * should be done for devices that were suspended in _sys_soc_suspend().
 * This function is called in ISR context and it should return quickly.
 *
 * @return will not return to caller in deep sleep recovery
 */
extern void _sys_soc_resume(void);

/**
 * @brief Hook function to allow power policy entry
 *
 * This function is called by the kernel when it is about to idle.
 * It is passed the number of clock ticks that the kernel calculated
 * as available time to idle. This function should compare this time
 * with the wake latency of various power saving schemes that the
 * power manager application implements and use the one that fits best.
 * The power saving schemes can be mapped to following policies.
 *
 * Deep sleep policy:
 * This turns off the core voltage rail and system clock, while RAM is
 * retained.  This would save most power but would also have a high wake
 * latency. CPU loses state so this function should save CPU states in RAM
 * and the location in this function where system should resume execution at
 * resume. It should re-enable interrupts and return SYS_PM_DEEP_SLEEP.
 *
 * Low power state policy:
 * Peripherals can be turned off and clocks can be gated depending on
 * time available. Then switches to CPU low power state.  In this state
 * the CPU is still active but in a low power state and does not lose
 * any state. This state is exited by an interrupt from where the
 * _sys_soc_resume() will be called. To allow interrupts to occur,
 * this function should ensure that interrupts are atomically enabled
 * before going to the low power CPU idle state. The atomicity of enabling
 * interrupts before entering cpu idle wait is essential to avoid a task
 * switch away from the kernel idle task before the cpu idle wait is reached.
 * This function should return SYS_PM_LOW_POWER_STATE.
 *
 * Device suspend only policy:
 * This function can take advantage of the kernel's idling logic
 * by turning off peripherals and clocks depending on available time.
 * It can return SYS_PM_DEVICE_SUSPEND_ONLY to indicate the kernel should
 * do its own CPU idle wait. After the Kernel's idle wait is completed or if
 * any interrupt occurs, the _sys_soc_resume() function will be called to
 * allow restoring of altered states. Interrupts should not be turned on in
 * this case.
 *
 * If this function decides to not do any operation then it should
 * return SYS_PM_NOT_HANDLED to let kernel do its normal idle processing.
 *
 * This function is entered with interrupts disabled.  It should
 * re-enable interrupts if it does CPU low power wait or deep sleep.
 *
 * @param ticks the upcoming kernel idle time
 *
 * @retval SYS_PM_NOT_HANDLED If No PM operations done.
 * @retval SYS_PM_DEVICE_SUSPEND_ONLY If only devices were suspended.
 * @retval SYS_PM_LOW_POWER_STATE If LPS policy entered.
 * @retval SYS_PM_DEEP_SLEEP If Deep Sleep policy entered.
 */
extern int _sys_soc_suspend(int32_t ticks);

#endif /* CONFIG_SYS_POWER_MANAGEMENT */

#ifdef __cplusplus
}
#endif

#endif /* __INCpower */
