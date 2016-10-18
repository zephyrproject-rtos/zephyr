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

enum power_states {
	SYS_POWER_STATE_CPU_LPS,       /* C2LP state */
	SYS_POWER_STATE_CPU_LPS_1,     /* C2 state */
	SYS_POWER_STATE_CPU_LPS_2,     /* C1 state */
	SYS_POWER_STATE_DEEP_SLEEP,    /* DEEP SLEEP state */
	SYS_POWER_STATE_DEEP_SLEEP_1,  /* SLEEP state */
	SYS_POWER_STATE_MAX
};

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
 * @brief Put processor into low power state
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 *
 * Wake up considerations:
 * SYS_POWER_STATE_CPU_LPS_2: Any interrupt works as wake event.
 *
 * SYS_POWER_STATE_CPU_LPS_1: Any interrupt works as wake event except
 * if the core enters LPSS where SYS_POWER_STATE_DEEP_SLEEP wake events
 * applies.
 *
 * SYS_POWER_STATE_CPU_LPS: Any interrupt works as wake event except the
 * PIC timer which is gated. If the core enters LPSS only
 * SYS_POWER_STATE_DEEP_SLEEP wake events applies.
 *
 * SYS_POWER_STATE_DEEP_SLEEP: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 *
 * SYS_POWER_STATE_DEEP_SLEEP_1: Only Always-On peripherals can wake up
 * the SoC. This consists of the Counter, RTC, GPIO 1 and AIO Comparator.
 */
void _sys_soc_set_power_state(enum power_states state);

/**
 * @brief Do any SoC or architecture specific post ops after low power states.
 *
 * This function is a place holder to do any operations that may
 * be needed to be done after deep sleep exits.  Currently it enables
 * interrupts after resuming from deep sleep. In future, the enabling
 * of interrupts may be moved into the kernel.
 */
void _sys_soc_power_state_post_ops(enum power_states state);

#ifdef __cplusplus
}
#endif

#endif /* _SOC_POWER_H_ */
