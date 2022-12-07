/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2020 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/arch/riscv/csr.h>
#include <stdio.h>
#include <pmp.h>

#ifdef CONFIG_USERSPACE
/*
 * Per-thread (TLS) variable indicating whether execution is in user mode.
 */
__thread uint8_t is_user_mode;
#endif

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	extern void z_riscv_thread_start(void);
	struct __esf *stack_init;

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	const struct soc_esf soc_esf_init = {SOC_ESF_INIT};
#endif

	/* Initial stack frame for thread */
	stack_init = (struct __esf *)Z_STACK_PTR_ALIGN(
				Z_STACK_PTR_TO_FRAME(struct __esf, stack_ptr)
				);

	/* Setup the initial stack frame */
	stack_init->a0 = (unsigned long)entry;
	stack_init->a1 = (unsigned long)p1;
	stack_init->a2 = (unsigned long)p2;
	stack_init->a3 = (unsigned long)p3;

	/*
	 * Following the RISC-V architecture,
	 * the MSTATUS register (used to globally enable/disable interrupt),
	 * as well as the MEPC register (used to by the core to save the
	 * value of the program counter at which an interrupt/exception occurs)
	 * need to be saved on the stack, upon an interrupt/exception
	 * and restored prior to returning from the interrupt/exception.
	 * This shall allow to handle nested interrupts.
	 *
	 * Given that thread startup happens through the exception exit
	 * path, initially set:
	 * 1) MSTATUS to MSTATUS_DEF_RESTORE in the thread stack to enable
	 *    interrupts when the newly created thread will be scheduled;
	 * 2) MEPC to the address of the z_thread_entry in the thread
	 *    stack.
	 * Hence, when going out of an interrupt/exception/context-switch,
	 * after scheduling the newly created thread:
	 * 1) interrupts will be enabled, as the MSTATUS register will be
	 *    restored following the MSTATUS value set within the thread stack;
	 * 2) the core will jump to z_thread_entry, as the program
	 *    counter will be restored following the MEPC value set within the
	 *    thread stack.
	 */
	stack_init->mstatus = MSTATUS_DEF_RESTORE;

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Shared FP mode: enable FPU of threads with K_FP_REGS. */
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		stack_init->mstatus |= MSTATUS_FS_INIT;
	}
	thread->callee_saved.fcsr = 0;
#elif defined(CONFIG_FPU)
	/* Unshared FP mode: enable FPU of each thread. */
	stack_init->mstatus |= MSTATUS_FS_INIT;
#endif

#if defined(CONFIG_USERSPACE)
	/* Clear user thread context */
	z_riscv_pmp_usermode_init(thread);
	thread->arch.priv_stack_start = 0;

	/* the unwound stack pointer upon exiting exception */
	stack_init->sp = (unsigned long)(stack_init + 1);
#endif /* CONFIG_USERSPACE */

	/* Assign thread entry point and mstatus.MPRV mode. */
	if (IS_ENABLED(CONFIG_USERSPACE)
	    && (thread->base.user_options & K_USER)) {
		/* User thread */
		stack_init->mepc = (unsigned long)k_thread_user_mode_enter;

	} else {
		/* Supervisor thread */
		stack_init->mepc = (unsigned long)z_thread_entry;

#if defined(CONFIG_PMP_STACK_GUARD)
		/* Enable PMP in mstatus.MPRV mode for RISC-V machine mode
		 * if thread is supervisor thread.
		 */
		stack_init->mstatus |= MSTATUS_MPRV;
#endif /* CONFIG_PMP_STACK_GUARD */
	}

#if defined(CONFIG_PMP_STACK_GUARD)
	/* Setup PMP regions of PMP stack guard of thread. */
	z_riscv_pmp_stackguard_prepare(thread);
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_RISCV_SOC_CONTEXT_SAVE
	stack_init->soc_context = soc_esf_init;
#endif

	thread->callee_saved.sp = (unsigned long)stack_init;

	/* where to go when returning from z_riscv_switch() */
	thread->callee_saved.ra = (unsigned long)z_riscv_thread_start;

	/* our switch handle is the thread pointer itself */
	thread->switch_handle = thread;
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	unsigned int key;

	if (thread != _current) {
		return -EINVAL;
	}

	if (arch_is_in_isr()) {
		return -EINVAL;
	}

	/* Ensure a preemptive context switch does not occur */
	key = irq_lock();

	/* Disable all floating point capabilities for the thread */
	thread->base.user_options &= ~K_FP_REGS;

	/* Clear the FS bits to disable the FPU. */
	__asm__ volatile (
		"mv t0, %0\n"
		"csrrc x0, mstatus, t0\n"
		:
		: "r" (MSTATUS_FS_MASK)
		);

	irq_unlock(key);

	return 0;
}


int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	unsigned int key;

	if (thread != _current) {
		return -EINVAL;
	}

	if (arch_is_in_isr()) {
		return -EINVAL;
	}

	/* Ensure a preemptive context switch does not occur */
	key = irq_lock();

	/* Enable all floating point capabilities for the thread. */
	thread->base.user_options |= K_FP_REGS;

	/* Set the FS bits to Initial and clear the fcsr to enable the FPU. */
	__asm__ volatile (
		"mv t0, %0\n"
		"csrrs x0, mstatus, t0\n"
		"fscsr x0, x0\n"
		:
		: "r" (MSTATUS_FS_INIT)
		);

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE

/*
 * User space entry function
 *
 * This function is the entry point to user mode from privileged execution.
 * The conversion is one way, and threads which transition to user mode do
 * not transition back later, unless they are doing system calls.
 */
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	unsigned long top_of_user_stack, top_of_priv_stack;
	unsigned long status;

	/* Set up privileged stack */
#ifdef CONFIG_GEN_PRIV_STACKS
	_current->arch.priv_stack_start =
			(unsigned long)z_priv_stack_find(_current->stack_obj);
	/* remove the stack guard from the main stack */
	_current->stack_info.start -= K_THREAD_STACK_RESERVED;
	_current->stack_info.size += K_THREAD_STACK_RESERVED;
#else
	_current->arch.priv_stack_start = (unsigned long)_current->stack_obj;
#endif /* CONFIG_GEN_PRIV_STACKS */
	top_of_priv_stack = Z_STACK_PTR_ALIGN(_current->arch.priv_stack_start +
					      K_KERNEL_STACK_RESERVED +
					      CONFIG_PRIVILEGED_STACK_SIZE);

	top_of_user_stack = Z_STACK_PTR_ALIGN(
				_current->stack_info.start +
				_current->stack_info.size -
				_current->stack_info.delta);

	status = csr_read(mstatus);

	/* Set next CPU status to user mode */
	status = INSERT_FIELD(status, MSTATUS_MPP, PRV_U);
	/* Enable IRQs for user mode */
	status = INSERT_FIELD(status, MSTATUS_MPIE, 1);
	/* Disable IRQs for m-mode until the mode switch */
	status = INSERT_FIELD(status, MSTATUS_MIE, 0);

	csr_write(mstatus, status);
	csr_write(mepc, z_thread_entry);

#ifdef CONFIG_PMP_STACK_GUARD
	/* reconfigure as the kernel mode stack will be different */
	z_riscv_pmp_stackguard_prepare(_current);
#endif

	/* Set up Physical Memory Protection */
	z_riscv_pmp_usermode_prepare(_current);
	z_riscv_pmp_usermode_enable(_current);

	/* exception stack has to be in mscratch */
	csr_write(mscratch, top_of_priv_stack);

	is_user_mode = true;

	register void *a0 __asm__("a0") = user_entry;
	register void *a1 __asm__("a1") = p1;
	register void *a2 __asm__("a2") = p2;
	register void *a3 __asm__("a3") = p3;

	__asm__ volatile (
	"mv sp, %4; mret"
	:
	: "r" (a0), "r" (a1), "r" (a2), "r" (a3), "r" (top_of_user_stack)
	: "memory");

	CODE_UNREACHABLE;
}

#endif /* CONFIG_USERSPACE */
