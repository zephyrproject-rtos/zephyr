/*
 * Copyright (c) 2016 Intel Corporation.
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

#ifndef _SOC_POWER_H_
#define _SOC_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Save CPU context
 *
 * This function would save the CPU context in the stack. It
 * would also save the idtr and gdtr registers. When context is
 * restored by _sys_soc_restore_cpu_context(), control will be
 * transferred into this function where the context was originally
 * saved.  The return values would indicate whether it is returning
 * after saving context or after a context restore transferred
 * control to it.
 *
 * @retval 0 Indicates it is returning after saving cpu context
 * @retval 1 Indicates cpu context restore transferred control to it.
 */
int _sys_soc_save_cpu_context(void);

/**
 * @brief Restore CPU context
 *
 * This function would restore the CPU context that was saved in
 * the stack by _sys_soc_save_cpu_context(). It would also restore
 * the idtr and gdtr registers.
 *
 * After context is restored, control will be transferred into
 * _sys_soc_save_cpu_context() function where the context was originally
 * saved.
 */
FUNC_NORETURN void _sys_soc_restore_cpu_context(void);

/**
 * @brief Put SoC into deep sleep power state
 *
 * This function implements the SoC specific details necessary
 * to put the SoC into deep sleep state.
 *
 * This function will not return;
 */
FUNC_NORETURN void _sys_soc_put_deep_sleep(void);

/**
 * @brief Do any SoC or architecture specific post ops after deep sleep.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after deep sleep exits.  Currently it enables
 * interrupts after resuming from deep sleep. In future, the enabling
 * of interrupts may be moved into the kernel.
 */
void _sys_soc_deep_sleep_post_ops(void);

/**
 * @brief Put processor into low power state
 *
 * This function implements the SoC specific details necessary
 * to put the processor into deep sleep state.
 */
void _sys_soc_put_low_power_state(void);

/**
 * @brief Save the current power policy
 *
 * This function implements the SoC specific details necessary
 * to save the current power policy. This should save the information
 * in a location that would be persistent across deep sleep if deep
 * sleep is one of the power policies supported.
 */
void _sys_soc_set_power_policy(uint32_t pm_policy);

/**
 * @brief Retrieve the saved current power policy
 *
 * This function implements the SoC specific details necessary
 * to retrieve the power policy information saved by
 * _sys_soc_set_power_policy().
 */
int _sys_soc_get_power_policy(void);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
