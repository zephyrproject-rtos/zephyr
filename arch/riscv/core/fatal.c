/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <inttypes.h>
#include <zephyr/arch/common/exc_handle.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(z_riscv_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(z_riscv_user_string_nlen),
};
#endif /* CONFIG_USERSPACE */

#if __riscv_xlen == 32
 #define PR_REG "%08" PRIxPTR
 #define NO_REG "        "
#elif __riscv_xlen == 64
 #define PR_REG "%016" PRIxPTR
 #define NO_REG "                "
#endif

/* Stack trace function */
void z_riscv_unwind_stack(const struct arch_esf *esf, const _callee_saved_t *csf);

uintptr_t z_riscv_get_sp_before_exc(const struct arch_esf *esf)
{
	/*
	 * Kernel stack pointer prior this exception i.e. before
	 * storing the exception stack frame.
	 */
	uintptr_t sp = (uintptr_t)esf + sizeof(struct arch_esf);

#ifdef CONFIG_USERSPACE
	if ((esf->mstatus & MSTATUS_MPP) == PRV_U) {
		/*
		 * Exception happened in user space:
		 * consider the saved user stack instead.
		 */
		sp = esf->sp;
	}
#endif

	return sp;
}

const char *z_riscv_mcause_str(unsigned long cause)
{
	static const char *const mcause_str[17] = {
		[0] = "Instruction address misaligned",
		[1] = "Instruction Access fault",
		[2] = "Illegal instruction",
		[3] = "Breakpoint",
		[4] = "Load address misaligned",
		[5] = "Load access fault",
		[6] = "Store/AMO address misaligned",
		[7] = "Store/AMO access fault",
		[8] = "Environment call from U-mode",
		[9] = "Environment call from S-mode",
		[10] = "unknown",
		[11] = "Environment call from M-mode",
		[12] = "Instruction page fault",
		[13] = "Load page fault",
		[14] = "unknown",
		[15] = "Store/AMO page fault",
		[16] = "unknown",
	};

	return mcause_str[MIN(cause, ARRAY_SIZE(mcause_str) - 1)];
}

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const struct arch_esf *esf)
{
	z_riscv_fatal_error_csf(reason, esf, NULL);
}

FUNC_NORETURN void z_riscv_fatal_error_csf(unsigned int reason, const struct arch_esf *esf,
					   const _callee_saved_t *csf)
{
	unsigned long mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

	mcause &= CONFIG_RISCV_MCAUSE_EXCEPTION_MASK;
	LOG_ERR("");
	LOG_ERR(" mcause: %ld, %s", mcause, z_riscv_mcause_str(mcause));

#ifndef CONFIG_SOC_OPENISA_RV32M1
	unsigned long mtval;

	__asm__ volatile("csrr %0, mtval" : "=r" (mtval));
	LOG_ERR("  mtval: %lx", mtval);
#endif /* CONFIG_SOC_OPENISA_RV32M1 */

#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		LOG_ERR("     a0: " PR_REG "    t0: " PR_REG, esf->a0, esf->t0);
		LOG_ERR("     a1: " PR_REG "    t1: " PR_REG, esf->a1, esf->t1);
		LOG_ERR("     a2: " PR_REG "    t2: " PR_REG, esf->a2, esf->t2);
#if defined(CONFIG_RISCV_ISA_RV32E)
		LOG_ERR("     a3: " PR_REG, esf->a3);
		LOG_ERR("     a4: " PR_REG, esf->a4);
		LOG_ERR("     a5: " PR_REG, esf->a5);
#else
		LOG_ERR("     a3: " PR_REG "    t3: " PR_REG, esf->a3, esf->t3);
		LOG_ERR("     a4: " PR_REG "    t4: " PR_REG, esf->a4, esf->t4);
		LOG_ERR("     a5: " PR_REG "    t5: " PR_REG, esf->a5, esf->t5);
		LOG_ERR("     a6: " PR_REG "    t6: " PR_REG, esf->a6, esf->t6);
		LOG_ERR("     a7: " PR_REG, esf->a7);
#endif /* CONFIG_RISCV_ISA_RV32E */
		LOG_ERR("     sp: " PR_REG, z_riscv_get_sp_before_exc(esf));
		LOG_ERR("     ra: " PR_REG, esf->ra);
		LOG_ERR("   mepc: " PR_REG, esf->mepc);
		LOG_ERR("mstatus: " PR_REG, esf->mstatus);
		LOG_ERR("");
	}

	if (csf != NULL) {
#if defined(CONFIG_RISCV_ISA_RV32E)
		LOG_ERR("     s0: " PR_REG, csf->s0);
		LOG_ERR("     s1: " PR_REG, csf->s1);
#else
		LOG_ERR("     s0: " PR_REG "    s6: " PR_REG, csf->s0, csf->s6);
		LOG_ERR("     s1: " PR_REG "    s7: " PR_REG, csf->s1, csf->s7);
		LOG_ERR("     s2: " PR_REG "    s8: " PR_REG, csf->s2, csf->s8);
		LOG_ERR("     s3: " PR_REG "    s9: " PR_REG, csf->s3, csf->s9);
		LOG_ERR("     s4: " PR_REG "   s10: " PR_REG, csf->s4, csf->s10);
		LOG_ERR("     s5: " PR_REG "   s11: " PR_REG, csf->s5, csf->s11);
#endif /* CONFIG_RISCV_ISA_RV32E */
		LOG_ERR("");
	}
#endif /* CONFIG_EXCEPTION_DEBUG */

#ifdef CONFIG_EXCEPTION_STACK_TRACE
	z_riscv_unwind_stack(esf, csf);
#endif /* CONFIG_EXCEPTION_STACK_TRACE */

	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static bool bad_stack_pointer(struct arch_esf *esf)
{
#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * Check if the kernel stack pointer prior this exception (before
	 * storing the exception stack frame) was in the stack guard area.
	 */
	uintptr_t sp = (uintptr_t)esf + sizeof(struct arch_esf);

#ifdef CONFIG_USERSPACE
	if (_current->arch.priv_stack_start != 0 &&
	    sp >= _current->arch.priv_stack_start &&
	    sp <  _current->arch.priv_stack_start + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}

	if (z_stack_is_user_capable(_current->stack_obj) &&
	    sp >= _current->stack_info.start - K_THREAD_STACK_RESERVED &&
	    sp <  _current->stack_info.start - K_THREAD_STACK_RESERVED
		  + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}
#endif /* CONFIG_USERSPACE */

#if CONFIG_MULTITHREADING
	if (sp >= _current->stack_info.start - K_KERNEL_STACK_RESERVED &&
	    sp <  _current->stack_info.start - K_KERNEL_STACK_RESERVED
		  + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}
#else
	uintptr_t isr_stack = (uintptr_t)z_interrupt_stacks;
	uintptr_t main_stack = (uintptr_t)z_main_stack;

	if ((sp >= isr_stack && sp < isr_stack + Z_RISCV_STACK_GUARD_SIZE) ||
	    (sp >= main_stack && sp < main_stack + Z_RISCV_STACK_GUARD_SIZE)) {
		return true;
	}
#endif /* CONFIG_MULTITHREADING */
#endif /* CONFIG_PMP_STACK_GUARD */

#ifdef CONFIG_USERSPACE
	if ((esf->mstatus & MSTATUS_MPP) == 0 &&
	    (esf->sp < _current->stack_info.start ||
	     esf->sp > _current->stack_info.start +
		       _current->stack_info.size -
		       _current->stack_info.delta)) {
		/* user stack pointer moved outside of its allowed stack */
		return true;
	}
#endif

	return false;
}

void z_riscv_fault(struct arch_esf *esf)
{
#ifdef CONFIG_USERSPACE
	/*
	 * Perform an assessment whether an PMP fault shall be
	 * treated as recoverable.
	 */
	for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
		unsigned long start = (unsigned long)exceptions[i].start;
		unsigned long end = (unsigned long)exceptions[i].end;

		if (esf->mepc >= start && esf->mepc < end) {
			esf->mepc = (unsigned long)exceptions[i].fixup;
			return;
		}
	}
#endif /* CONFIG_USERSPACE */

	unsigned int reason = K_ERR_CPU_EXCEPTION;

	if (bad_stack_pointer(esf)) {
#ifdef CONFIG_PMP_STACK_GUARD
		/*
		 * Remove the thread's PMP setting to prevent triggering a stack
		 * overflow error again due to the previous configuration.
		 */
		z_riscv_pmp_stackguard_disable();
#endif /* CONFIG_PMP_STACK_GUARD */
		reason = K_ERR_STACK_CHK_FAIL;
	}

	z_riscv_fatal_error(reason, esf);
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_syscall_oops(void *ssf_ptr)
{
	user_fault(K_ERR_KERNEL_OOPS);
	CODE_UNREACHABLE;
}

void z_impl_user_fault(unsigned int reason)
{
	struct arch_esf *oops_esf = _current->syscall_frame;

	if (((_current->base.user_options & K_USER) != 0) &&
		reason != K_ERR_STACK_CHK_FAIL) {
		reason = K_ERR_KERNEL_OOPS;
	}
	z_riscv_fatal_error(reason, oops_esf);
}

static void z_vrfy_user_fault(unsigned int reason)
{
	z_impl_user_fault(reason);
}

#include <zephyr/syscalls/user_fault_mrsh.c>

#endif /* CONFIG_USERSPACE */
