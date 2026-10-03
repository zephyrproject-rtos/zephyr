/*
 * Copyright (c) 2022 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/irq_offload.h>
#include <zephyr/arch/riscv/syscall.h>
#include <zephyr/arch/riscv/csr.h>
#include <ksched.h>

#ifdef CONFIG_RISCV_S_MODE
#if defined(CONFIG_FPU_SHARING) && defined(CONFIG_RISCV_ISA_EXT_F)
extern void z_riscv_fpu_enter_exc(void);
extern void z_riscv_fpu_irq_offload_exit(void);
#endif
#endif

void arch_irq_offload(irq_offload_routine_t routine, const void *parameter)
{
#ifdef CONFIG_RISCV_S_MODE
	/*
	 * In S-mode, ecall (cause=9) is routed to the M-mode SBI handler
	 * which does not know about RV_ECALL_IRQ_OFFLOAD.  Simulate ISR
	 * execution directly: bump the nested counter so k_is_in_isr()
	 * returns true, run the routine, then check for a pending reschedule.
	 *
	 * Mirror what _isr_wrapper does for FPU: disable FPU access on entry
	 * and restore it on exit so FPU state is consistent with a real ISR.
	 * With Zfinx there is no FPU access control, only the fcsr of the
	 * interrupted context has to be preserved.
	 */
	unsigned int key = arch_irq_lock();

#ifdef CONFIG_FPU_SHARING
	_current->arch.exception_depth++;
#ifdef CONFIG_RISCV_ISA_EXT_F
	z_riscv_fpu_enter_exc();
#endif
#ifdef CONFIG_RISCV_ISA_EXT_ZFINX
	unsigned long saved_fcsr = csr_read(fcsr);
#endif
#endif
	arch_curr_cpu()->nested++;
	routine(parameter);
	arch_curr_cpu()->nested--;
#ifdef CONFIG_FPU_SHARING
#ifdef CONFIG_RISCV_ISA_EXT_F
	z_riscv_fpu_irq_offload_exit();
#endif
#ifdef CONFIG_RISCV_ISA_EXT_ZFINX
	csr_write(fcsr, saved_fcsr);
#endif
	_current->arch.exception_depth--;
#endif

	arch_irq_unlock(key);
	z_reschedule_unlocked();
#else
	arch_syscall_invoke2((uintptr_t)routine, (uintptr_t)parameter, RV_ECALL_IRQ_OFFLOAD);
#endif
}

void arch_irq_offload_init(void)
{
}
