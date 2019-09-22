/*
 * Copyright (c) 2018 Linaro, Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <toolchain.h>
#include <kernel_structs.h>

#ifdef CONFIG_EXECUTION_BENCHMARKING
extern void read_timer_start_of_swap(void);
#endif
extern const int _k_neg_eagain;

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The z_arch_swap() routine is invoked by various kernel services to effect
 * a cooperative context context switch.  Prior to invoking z_arch_swap(), the caller
 * disables interrupts via irq_lock() and the return 'key' is passed as a
 * parameter to z_arch_swap().  The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * z_arch_swap() itself does not do much.
 *
 * It simply stores the intlock key (the BASEPRI value) parameter into
 * current->basepri, and then triggers a PendSV exception, which does
 * the heavy lifting of context switching.

 * This is the only place we have to save BASEPRI since the other paths to
 * __pendsv all come from handling an interrupt, which means we know the
 * interrupts were not locked: in that case the BASEPRI value is 0.
 *
 * Given that z_arch_swap() is called to effect a cooperative context switch,
 * only the caller-saved integer registers need to be saved in the thread of the
 * outgoing thread. This is all performed by the hardware, which stores it in
 * its exception stack frame, created when handling the __pendsv exception.
 *
 * On ARMv6-M, the intlock key is represented by the PRIMASK register,
 * as BASEPRI is not available.
 *
 * @return -EAGAIN, or a return value set by a call to
 * z_arch_thread_return_value_set()
 *
 */
int z_arch_swap(unsigned int key)
{
#ifdef CONFIG_EXECUTION_BENCHMARKING
	read_timer_start_of_swap();
#endif

	/* store off key and return value */
	_current->arch.basepri = key;
	_current->arch.swap_return_value = _k_neg_eagain;

#if defined(CONFIG_CPU_CORTEX_M)
	/* set pending bit to make sure we will take a PendSV exception */
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

	/* clear mask or enable all irqs to take a pendsv */
	irq_unlock(0);
#elif defined(CONFIG_CPU_CORTEX_R)
	cortex_r_svc();
	irq_unlock(key);
#endif

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return _current->arch.swap_return_value;
}
