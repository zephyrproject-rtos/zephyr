/*
 * Copyright (c) 2018 Linaro, Limited
 * Copyright (c) 2023 Arm Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>

#include <errno.h>

/* The 'key' actually represents the BASEPRI register
 * prior to disabling interrupts via the BASEPRI mechanism.
 *
 * arch_swap() itself does not do much.
 *
 * It simply stores the intlock key (the BASEPRI value) parameter into
 * current->basepri, and then triggers a PendSV exception, which does
 * the heavy lifting of context switching.

 * This is the only place we have to save BASEPRI since the other paths to
 * z_arm_pendsv all come from handling an interrupt, which means we know the
 * interrupts were not locked: in that case the BASEPRI value is 0.
 *
 * Given that arch_swap() is called to effect a cooperative context switch,
 * only the caller-saved integer registers need to be saved in the thread of the
 * outgoing thread. This is all performed by the hardware, which stores it in
 * its exception stack frame, created when handling the z_arm_pendsv exception.
 *
 * On ARMv6-M, the intlock key is represented by the PRIMASK register,
 * as BASEPRI is not available.
 */
int arch_swap(unsigned int key)
{
	/* store off key and return value */
	_current->arch.basepri = key;
	_current->arch.swap_return_value = -EAGAIN;

	/* set pending bit to make sure we will take a PendSV exception */
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;

	/* clear mask or enable all irqs to take a pendsv */
	irq_unlock(0);

	/* Context switch is performed here. Returning implies the
	 * thread has been context-switched-in again.
	 */
	return _current->arch.swap_return_value;
}

uintptr_t z_arm_pendsv_c(uintptr_t exc_ret)
{
	/* Store LSB of LR (EXC_RETURN) to the thread's 'mode' word. */
	IF_ENABLED(CONFIG_ARM_STORE_EXC_RETURN,
		   (_kernel.cpus[0].current->arch.mode_exc_return = (uint8_t)exc_ret;));

	/* Protect the kernel state while we play with the thread lists */
	uint32_t basepri = arch_irq_lock();

	/* fetch the thread to run from the ready queue cache */
	struct k_thread *current = _kernel.cpus[0].current = _kernel.ready_q.cache;

	/*
	 * Clear PendSV so that if another interrupt comes in and
	 * decides, with the new kernel state based on the new thread
	 * being context-switched in, that it needs to reschedule, it
	 * will take, but that previously pended PendSVs do not take,
	 * since they were based on the previous kernel state and this
	 * has been handled.
	 */
	SCB->ICSR = SCB_ICSR_PENDSVCLR_Msk;

	/* For Cortex-M, store TLS pointer in a global variable,
	 * as it lacks the process ID or thread ID register
	 * to be used by toolchain to access thread data.
	 */
	IF_ENABLED(CONFIG_THREAD_LOCAL_STORAGE,
		   (extern uintptr_t z_arm_tls_ptr; z_arm_tls_ptr = current->tls));

	IF_ENABLED(CONFIG_ARM_STORE_EXC_RETURN,
		   (exc_ret = (exc_ret & 0xFFFFFF00) | current->arch.mode_exc_return));

	/* Restore previous interrupt disable state (irq_lock key)
	 * (We clear the arch.basepri field after restoring state)
	 */
	basepri = current->arch.basepri;
	current->arch.basepri = 0;

	arch_irq_unlock(basepri);

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	/* Re-program dynamic memory map */
	z_arm_configure_dynamic_mpu_regions(current);
#endif

	/* restore mode */
	IF_ENABLED(CONFIG_USERSPACE, ({
			   CONTROL_Type ctrl = {.w = __get_CONTROL()};
			   /* exit privileged state when returing to thread mode. */
			   ctrl.b.nPRIV = 0;
			   __set_CONTROL(ctrl.w | current->arch.mode);
		   }));

	return exc_ret;
}
