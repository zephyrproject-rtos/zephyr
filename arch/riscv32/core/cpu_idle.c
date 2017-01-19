/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>

/*
 * In RISC-V there is no conventional way to handle CPU power save.
 * Each RISC-V SOC handles it in its own way.
 * Hence, by default, nano_cpu_idle and nano_cpu_atomic_idle functions just
 * unlock interrupts and return to the caller, without issuing any CPU power
 * saving instruction.
 *
 * Nonetheless, define the default nano_cpu_idle and nano_cpu_atomic_idle
 * functions as weak functions, so that they can be replaced at the SOC-level.
 */

/**
 *
 * @brief Power save idle routine
 *
 * This function will be called by the kernel idle loop or possibly within
 * an implementation of _sys_power_save_idle in the kernel when the
 * '_sys_power_save_flag' variable is non-zero.
 *
 * @return N/A
 */
void __weak k_cpu_idle(void)
{
	irq_unlock(SOC_MSTATUS_IEN);
}

/**
 *
 * @brief Atomically re-enable interrupts and enter low power mode
 *
 * This function is utilized by the nanokernel object "wait" APIs for tasks,
 * e.g. nano_task_lifo_get(), nano_task_sem_take(),
 * nano_task_stack_pop(), and nano_task_fifo_get().
 *
 * INTERNAL
 * The requirements for k_cpu_atomic_idle() are as follows:
 * 1) The enablement of interrupts and entering a low-power mode needs to be
 *    atomic, i.e. there should be no period of time where interrupts are
 *    enabled before the processor enters a low-power mode.  See the comments
 *    in k_lifo_get(), for example, of the race condition that
 *    occurs if this requirement is not met.
 *
 * 2) After waking up from the low-power mode, the interrupt lockout state
 *    must be restored as indicated in the 'imask' input parameter.
 *
 * @return N/A
 */
void __weak k_cpu_atomic_idle(unsigned int key)
{
	irq_unlock(key);
}
