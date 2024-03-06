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

#ifdef CONFIG_RISCV_EXCEPTION_STACK_TRACE
#define MAX_STACK_FRAMES 8

struct stackframe {
	uintptr_t fp;
	uintptr_t ra;
};

static bool in_stack_bound(uintptr_t addr)
{
#ifdef CONFIG_THREAD_STACK_INFO
	uintptr_t start, end;

	if (_current == NULL || arch_is_in_isr()) {
		/* We were servicing an interrupt */
		int cpu_id;

#ifdef CONFIG_SMP
		cpu_id = arch_curr_cpu()->id;
#else
		cpu_id = 0;
#endif

		start = (uintptr_t)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[cpu_id]);
		end = start + CONFIG_ISR_STACK_SIZE;
#ifdef CONFIG_USERSPACE
		/* TODO: handle user threads */
#endif
	} else {
		start = _current->stack_info.start;
		end = Z_STACK_PTR_ALIGN(_current->stack_info.start + _current->stack_info.size);
	}

	return (addr >= start) && (addr < end);
#else
	ARG_UNUSED(addr);
	return true;
#endif /* CONFIG_THREAD_STACK_INFO */
}

static inline bool in_text_region(uintptr_t addr)
{
	extern uintptr_t __text_region_start, __text_region_end;

	return (addr >= (uintptr_t)&__text_region_start) && (addr < (uintptr_t)&__text_region_end);
}

static void unwind_stack(const z_arch_esf_t *esf)
{
	uintptr_t fp = esf->s0;
	uintptr_t ra;
	struct stackframe *frame;

	LOG_ERR("call trace:");

	for (int i = 0; (i < MAX_STACK_FRAMES) && (fp != 0U) && in_stack_bound((uintptr_t)fp);) {
		frame = (struct stackframe *)fp - 1;
		ra = frame->ra;
		if (in_text_region(ra)) {
			LOG_ERR("     %2d: fp: " PR_REG "   ra: " PR_REG, i, (uintptr_t)fp, ra);
			/*
			 * Increment the iterator only if `ra` is within the text region to get the
			 * most out of it
			 */
			i++;
		}
		fp = frame->fp;
	}
}
#endif /* CONFIG_RISCV_EXCEPTION_STACK_TRACE */

FUNC_NORETURN void z_riscv_fatal_error(unsigned int reason,
				       const z_arch_esf_t *esf)
{
	z_riscv_fatal_error_csf(reason, esf, NULL);
}

FUNC_NORETURN void z_riscv_fatal_error_csf(unsigned int reason, const z_arch_esf_t *esf,
					   const _callee_saved_t *csf)
{
#ifdef CONFIG_EXCEPTION_DEBUG
	if (esf != NULL) {
		/*
		 * Kernel stack pointer prior this exception i.e. before
		 * storing the exception stack frame.
		 */
		uintptr_t sp = (uintptr_t)esf + sizeof(z_arch_esf_t);

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
#ifdef CONFIG_USERSPACE
		if ((esf->mstatus & MSTATUS_MPP) == 0) {
			/*
			 * Exception happened in user space:
			 * consider the saved user stack instead.
			 */
			sp = esf->sp;
		}
#endif
		LOG_ERR("     sp: " PR_REG, sp);
		LOG_ERR("     ra: " PR_REG, esf->ra);
		LOG_ERR("   mepc: " PR_REG, esf->mepc);
		LOG_ERR("mstatus: " PR_REG, esf->mstatus);
		LOG_ERR("");
#ifdef CONFIG_RISCV_EXCEPTION_STACK_TRACE
		unwind_stack(esf);
#endif /* CONFIG_RISCV_EXCEPTION_STACK_TRACE */
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
	z_fatal_error(reason, esf);
	CODE_UNREACHABLE;
}

static char *cause_str(unsigned long cause)
{
	switch (cause) {
	case 0:
		return "Instruction address misaligned";
	case 1:
		return "Instruction Access fault";
	case 2:
		return "Illegal instruction";
	case 3:
		return "Breakpoint";
	case 4:
		return "Load address misaligned";
	case 5:
		return "Load access fault";
	case 6:
		return "Store/AMO address misaligned";
	case 7:
		return "Store/AMO access fault";
	case 8:
		return "Environment call from U-mode";
	case 9:
		return "Environment call from S-mode";
	case 11:
		return "Environment call from M-mode";
	case 12:
		return "Instruction page fault";
	case 13:
		return "Load page fault";
	case 15:
		return "Store/AMO page fault";
	default:
		return "unknown";
	}
}

static bool bad_stack_pointer(z_arch_esf_t *esf)
{
#ifdef CONFIG_PMP_STACK_GUARD
	/*
	 * Check if the kernel stack pointer prior this exception (before
	 * storing the exception stack frame) was in the stack guard area.
	 */
	uintptr_t sp = (uintptr_t)esf + sizeof(z_arch_esf_t);

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

	if (sp >= _current->stack_info.start - K_KERNEL_STACK_RESERVED &&
	    sp <  _current->stack_info.start - K_KERNEL_STACK_RESERVED
		  + Z_RISCV_STACK_GUARD_SIZE) {
		return true;
	}
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

void _Fault(z_arch_esf_t *esf)
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

	unsigned long mcause;

	__asm__ volatile("csrr %0, mcause" : "=r" (mcause));

#ifndef CONFIG_SOC_OPENISA_RV32M1
	unsigned long mtval;
	__asm__ volatile("csrr %0, mtval" : "=r" (mtval));
#endif

	mcause &= CONFIG_RISCV_MCAUSE_EXCEPTION_MASK;
	LOG_ERR("");
	LOG_ERR(" mcause: %ld, %s", mcause, cause_str(mcause));
#ifndef CONFIG_SOC_OPENISA_RV32M1
	LOG_ERR("  mtval: %lx", mtval);
#endif

	unsigned int reason = K_ERR_CPU_EXCEPTION;

	if (bad_stack_pointer(esf)) {
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
	z_arch_esf_t *oops_esf = _current->syscall_frame;

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

#include <syscalls/user_fault_mrsh.c>

#endif /* CONFIG_USERSPACE */
