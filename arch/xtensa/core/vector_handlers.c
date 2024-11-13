/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <xtensa_asm2_context.h>
#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/kernel_structs.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <zephyr/offsets.h>
#include <zephyr/zsr.h>
#include <zephyr/arch/common/exc_handle.h>

#ifdef CONFIG_XTENSA_GEN_HANDLERS
#include <xtensa_handlers.h>
#else
#include <_soc_inthandlers.h>
#endif

#include <kernel_internal.h>
#include <xtensa_internal.h>
#include <xtensa_stack.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

extern char xtensa_arch_except_epc[];
extern char xtensa_arch_kernel_oops_epc[];

bool xtensa_is_outside_stack_bounds(uintptr_t addr, size_t sz, uint32_t ps)
{
	uintptr_t start, end;
	struct k_thread *thread = _current;
	bool was_in_isr, invalid;

	/* Without userspace, there is no privileged stack so the thread stack
	 * is the whole stack (minus reserved area). So there is no need to
	 * check for PS == UINT32_MAX for special treatment.
	 */
	ARG_UNUSED(ps);

	/* Since both level 1 interrupts and exceptions go through
	 * the same interrupt vector, both of them increase the nested
	 * counter in the CPU struct. The architecture vector handler
	 * moves execution to the interrupt stack when nested goes from
	 * zero to one. Afterwards, any nested interrupts/exceptions will
	 * continue running in interrupt stack. Therefore, only when
	 * nested > 1, then it was running in the interrupt stack, and
	 * we should check bounds against the interrupt stack.
	 */
	was_in_isr = arch_curr_cpu()->nested > 1;

	if ((thread == NULL) || was_in_isr) {
		/* We were servicing an interrupt or in early boot environment
		 * and are supposed to be on the interrupt stack.
		 */
		int cpu_id;

#ifdef CONFIG_SMP
		cpu_id = arch_curr_cpu()->id;
#else
		cpu_id = 0;
#endif

		start = (uintptr_t)K_KERNEL_STACK_BUFFER(z_interrupt_stacks[cpu_id]);
		end = start + CONFIG_ISR_STACK_SIZE;
#ifdef CONFIG_USERSPACE
	} else if (ps == UINT32_MAX) {
		/* Since the stashed PS is inside struct pointed by frame->ptr_to_bsa,
		 * we need to verify that both frame and frame->ptr_to_bsa are valid
		 * pointer within the thread stack. Also without PS, we have no idea
		 * whether we were in kernel mode (using privileged stack) or user
		 * mode (normal thread stack). So we need to check the whole stack
		 * area.
		 *
		 * And... we cannot account for reserved area since we have no idea
		 * which to use: ARCH_KERNEL_STACK_RESERVED or ARCH_THREAD_STACK_RESERVED
		 * as we don't know whether we were in kernel or user mode.
		 */
		start = (uintptr_t)thread->stack_obj;
		end = Z_STACK_PTR_ALIGN(thread->stack_info.start + thread->stack_info.size);
	} else if (((ps & PS_RING_MASK) == 0U) &&
		   ((thread->base.user_options & K_USER) == K_USER)) {
		/* Check if this is a user thread, and that it was running in
		 * kernel mode. If so, we must have been doing a syscall, so
		 * check with privileged stack bounds.
		 */
		start = thread->stack_info.start - CONFIG_PRIVILEGED_STACK_SIZE;
		end = thread->stack_info.start;
#endif
	} else {
		start = thread->stack_info.start;
		end = Z_STACK_PTR_ALIGN(thread->stack_info.start + thread->stack_info.size);
	}

	invalid = (addr <= start) || ((addr + sz) >= end);

	return invalid;
}

bool xtensa_is_frame_pointer_valid(_xtensa_irq_stack_frame_raw_t *frame)
{
	_xtensa_irq_bsa_t *bsa;

	/* Check if the pointer to the frame is within stack bounds. If not, there is no
	 * need to test if the BSA (base save area) pointer is also valid as it is
	 * possibly invalid.
	 */
	if (xtensa_is_outside_stack_bounds((uintptr_t)frame, sizeof(*frame), UINT32_MAX)) {
		return false;
	}

	/* Need to test if the BSA area is also within stack bounds. The information
	 * contained within the BSA is only valid if within stack bounds.
	 */
	bsa = frame->ptr_to_bsa;
	if (xtensa_is_outside_stack_bounds((uintptr_t)bsa, sizeof(*bsa), UINT32_MAX)) {
		return false;
	}

#ifdef CONFIG_USERSPACE
	/* With usespace, we have privileged stack and normal thread stack within
	 * one stack object. So we need to further test whether the frame pointer
	 * resides in the correct stack based on kernel/user mode.
	 */
	if (xtensa_is_outside_stack_bounds((uintptr_t)frame, sizeof(*frame), bsa->ps)) {
		return false;
	}
#endif

	return true;
}

void xtensa_dump_stack(const void *stack)
{
	_xtensa_irq_stack_frame_raw_t *frame = (void *)stack;
	_xtensa_irq_bsa_t *bsa;
	uintptr_t num_high_regs;
	int reg_blks_remaining;

	/* Don't dump stack if the stack pointer is invalid as any frame elements
	 * obtained via de-referencing the frame pointer are probably also invalid.
	 * Or worse, cause another access violation.
	 */
	if (!xtensa_is_frame_pointer_valid(frame)) {
		return;
	}

	bsa = frame->ptr_to_bsa;

	/* Calculate number of high registers. */
	num_high_regs = (uint8_t *)bsa - (uint8_t *)frame + sizeof(void *);
	num_high_regs /= sizeof(uintptr_t);

	/* And high registers are always comes in 4 in a block. */
	reg_blks_remaining = (int)num_high_regs / 4;

	LOG_ERR(" **  A0 %p  SP %p  A2 %p  A3 %p",
		(void *)bsa->a0,
		(void *)((char *)bsa + sizeof(*bsa)),
		(void *)bsa->a2, (void *)bsa->a3);

	if (reg_blks_remaining > 0) {
		reg_blks_remaining--;

		LOG_ERR(" **  A4 %p  A5 %p  A6 %p  A7 %p",
			(void *)frame->blks[reg_blks_remaining].r0,
			(void *)frame->blks[reg_blks_remaining].r1,
			(void *)frame->blks[reg_blks_remaining].r2,
			(void *)frame->blks[reg_blks_remaining].r3);
	}

	if (reg_blks_remaining > 0) {
		reg_blks_remaining--;

		LOG_ERR(" **  A8 %p  A9 %p A10 %p A11 %p",
			(void *)frame->blks[reg_blks_remaining].r0,
			(void *)frame->blks[reg_blks_remaining].r1,
			(void *)frame->blks[reg_blks_remaining].r2,
			(void *)frame->blks[reg_blks_remaining].r3);
	}

	if (reg_blks_remaining > 0) {
		reg_blks_remaining--;

		LOG_ERR(" ** A12 %p A13 %p A14 %p A15 %p",
			(void *)frame->blks[reg_blks_remaining].r0,
			(void *)frame->blks[reg_blks_remaining].r1,
			(void *)frame->blks[reg_blks_remaining].r2,
			(void *)frame->blks[reg_blks_remaining].r3);
	}

#if XCHAL_HAVE_LOOPS
	LOG_ERR(" ** LBEG %p LEND %p LCOUNT %p",
		(void *)bsa->lbeg,
		(void *)bsa->lend,
		(void *)bsa->lcount);
#endif

	LOG_ERR(" ** SAR %p", (void *)bsa->sar);

#if XCHAL_HAVE_THREADPTR
	LOG_ERR(" **  THREADPTR %p", (void *)bsa->threadptr);
#endif
}

static inline unsigned int get_bits(int offset, int num_bits, unsigned int val)
{
	int mask;

	mask = BIT(num_bits) - 1;
	val = val >> offset;
	return val & mask;
}

static void print_fatal_exception(void *print_stack, int cause,
				  bool is_dblexc, uint32_t depc)
{
	void *pc;
	uint32_t ps, vaddr;
	_xtensa_irq_bsa_t *bsa = (void *)*(int **)print_stack;

	__asm__ volatile("rsr.excvaddr %0" : "=r"(vaddr));

	if (is_dblexc) {
		LOG_ERR(" ** FATAL EXCEPTION (DOUBLE)");
	} else {
		LOG_ERR(" ** FATAL EXCEPTION");
	}

	LOG_ERR(" ** CPU %d EXCCAUSE %d (%s)",
		arch_curr_cpu()->id, cause,
		xtensa_exccause(cause));

	/* Don't print information if the BSA area is invalid as any elements
	 * obtained via de-referencing the pointer are probably also invalid.
	 * Or worse, cause another access violation.
	 */
	if (xtensa_is_outside_stack_bounds((uintptr_t)bsa, sizeof(*bsa), UINT32_MAX)) {
		LOG_ERR(" ** VADDR %p Invalid SP %p", (void *)vaddr, print_stack);
		return;
	}

	ps = bsa->ps;
	pc = (void *)bsa->pc;

	LOG_ERR(" **  PC %p VADDR %p", pc, (void *)vaddr);

	if (is_dblexc) {
		LOG_ERR(" **  DEPC %p", (void *)depc);
	}

	LOG_ERR(" **  PS %p", (void *)bsa->ps);
	LOG_ERR(" **    (INTLEVEL:%d EXCM: %d UM:%d RING:%d WOE:%d OWB:%d CALLINC:%d)",
		get_bits(0, 4, ps), get_bits(4, 1, ps),
		get_bits(5, 1, ps), get_bits(6, 2, ps),
		get_bits(18, 1, ps),
		get_bits(8, 4, ps), get_bits(16, 2, ps));
}

static ALWAYS_INLINE void usage_stop(void)
{
#ifdef CONFIG_SCHED_THREAD_USAGE
	z_sched_usage_stop();
#endif
}

static inline void *return_to(void *interrupted)
{
#ifdef CONFIG_MULTITHREADING
	return arch_curr_cpu()->nested <= 1 ?
		z_get_next_switch_handle(interrupted) : interrupted;
#else
	return interrupted;
#endif /* CONFIG_MULTITHREADING */
}

/* The wrapper code lives here instead of in the python script that
 * generates _xtensa_handle_one_int*().  Seems cleaner, still kind of
 * ugly.
 *
 * This may be unused depending on number of interrupt levels
 * supported by the SoC.
 */
#define DEF_INT_C_HANDLER(l)				\
__unused void *xtensa_int##l##_c(void *interrupted_stack)	\
{							   \
	uint32_t irqs, intenable, m;			   \
	usage_stop();					   \
	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs)); \
	__asm__ volatile("rsr.intenable %0" : "=r"(intenable)); \
	irqs &= intenable;					\
	while ((m = _xtensa_handle_one_int##l(irqs))) {		\
		irqs ^= m;					\
		__asm__ volatile("wsr.intclear %0" : : "r"(m)); \
	}							\
	return return_to(interrupted_stack);		\
}

#if XCHAL_HAVE_NMI
#define MAX_INTR_LEVEL XCHAL_NMILEVEL
#elif XCHAL_HAVE_INTERRUPTS
#define MAX_INTR_LEVEL XCHAL_NUM_INTLEVELS
#else
#error Xtensa core with no interrupt support is used
#define MAX_INTR_LEVEL 0
#endif

#if MAX_INTR_LEVEL >= 2
DEF_INT_C_HANDLER(2)
#endif

#if MAX_INTR_LEVEL >= 3
DEF_INT_C_HANDLER(3)
#endif

#if MAX_INTR_LEVEL >= 4
DEF_INT_C_HANDLER(4)
#endif

#if MAX_INTR_LEVEL >= 5
DEF_INT_C_HANDLER(5)
#endif

#if MAX_INTR_LEVEL >= 6
DEF_INT_C_HANDLER(6)
#endif

#if MAX_INTR_LEVEL >= 7
DEF_INT_C_HANDLER(7)
#endif

static inline DEF_INT_C_HANDLER(1)

/* C handler for level 1 exceptions/interrupts.  Hooked from the
 * DEF_EXCINT 1 vector declaration in assembly code.  This one looks
 * different because exceptions and interrupts land at the same
 * vector; other interrupt levels have their own vectors.
 */
void *xtensa_excint1_c(void *esf)
{
	int cause, reason;
	int *interrupted_stack = &((struct arch_esf *)esf)->dummy;
	_xtensa_irq_bsa_t *bsa = (void *)*(int **)interrupted_stack;
	bool is_fatal_error = false;
	bool is_dblexc = false;
	uint32_t ps;
	void *pc, *print_stack = (void *)interrupted_stack;
	uint32_t depc = 0;

#ifdef CONFIG_XTENSA_MMU
	depc = XTENSA_RSR(ZSR_DEPC_SAVE_STR);
	cause = XTENSA_RSR(ZSR_EXCCAUSE_SAVE_STR);

	is_dblexc = (depc != 0U);
#else /* CONFIG_XTENSA_MMU */
	__asm__ volatile("rsr.exccause %0" : "=r"(cause));
#endif /* CONFIG_XTENSA_MMU */

	switch (cause) {
	case EXCCAUSE_LEVEL1_INTERRUPT:
#ifdef CONFIG_XTENSA_MMU
		if (!is_dblexc) {
			return xtensa_int1_c(interrupted_stack);
		}
#else
		return xtensa_int1_c(interrupted_stack);
#endif /* CONFIG_XTENSA_MMU */
		break;
#ifndef CONFIG_USERSPACE
	/* Syscalls are handled earlier in assembly if MMU is enabled.
	 * So we don't need this here.
	 */
	case EXCCAUSE_SYSCALL:
		/* Just report it to the console for now */
		LOG_ERR(" ** SYSCALL PS %p PC %p",
			(void *)bsa->ps, (void *)bsa->pc);
		xtensa_dump_stack(interrupted_stack);

		/* Xtensa exceptions don't automatically advance PC,
		 * have to skip the SYSCALL instruction manually or
		 * else it will just loop forever
		 */
		bsa->pc += 3;
		break;
#endif /* !CONFIG_USERSPACE */
	default:
		reason = K_ERR_CPU_EXCEPTION;

		/* If the BSA area is invalid, we cannot trust anything coming out of it. */
		if (xtensa_is_outside_stack_bounds((uintptr_t)bsa, sizeof(*bsa), UINT32_MAX)) {
			goto skip_checks;
		}

		ps = bsa->ps;
		pc = (void *)bsa->pc;

		/* Default for exception */
		is_fatal_error = true;

		/* We need to distinguish between an ill in xtensa_arch_except,
		 * e.g for k_panic, and any other ill. For exceptions caused by
		 * xtensa_arch_except calls, we also need to pass the reason_p
		 * to xtensa_fatal_error. Since the ARCH_EXCEPT frame is in the
		 * BSA, the first arg reason_p is stored at the A2 offset.
		 * We assign EXCCAUSE the unused, reserved code 63; this may be
		 * problematic if the app or new boards also decide to repurpose
		 * this code.
		 *
		 * Another intentionally ill is from xtensa_arch_kernel_oops.
		 * Kernel OOPS has to be explicity raised so we can simply
		 * set the reason and continue.
		 */
		if (cause == EXCCAUSE_ILLEGAL) {
			if (pc == (void *)&xtensa_arch_except_epc) {
				cause = 63;
				__asm__ volatile("wsr.exccause %0" : : "r"(cause));
				reason = bsa->a2;
			} else if (pc == (void *)&xtensa_arch_kernel_oops_epc) {
				cause = 64; /* kernel oops */
				reason = K_ERR_KERNEL_OOPS;

				/* A3 contains the second argument to
				 * xtensa_arch_kernel_oops(reason, ssf)
				 * where ssf is the stack frame causing
				 * the kernel oops.
				 */
				print_stack = (void *)bsa->a3;
			}
		}

skip_checks:
		if (reason != K_ERR_KERNEL_OOPS) {
			print_fatal_exception(print_stack, cause, is_dblexc, depc);
		}

		/* FIXME: legacy xtensa port reported "HW" exception
		 * for all unhandled exceptions, which seems incorrect
		 * as these are software errors.  Should clean this
		 * up.
		 */
		xtensa_fatal_error(reason, (void *)print_stack);
		break;
	}

#ifdef CONFIG_XTENSA_MMU
	switch (cause) {
	case EXCCAUSE_LEVEL1_INTERRUPT:
#ifndef CONFIG_USERSPACE
	case EXCCAUSE_SYSCALL:
#endif /* !CONFIG_USERSPACE */
		is_fatal_error = false;
		break;
	default:
		is_fatal_error = true;
		break;
	}
#endif /* CONFIG_XTENSA_MMU */

	if (is_dblexc || is_fatal_error) {
		uint32_t ignore;

		/* We are going to manipulate arch_curr_cpu()->nested manually.
		 * Since the error is fatal, for recoverable errors, code
		 * execution must not return back to the current thread as
		 * it is being terminated (via above xtensa_fatal_error()).
		 * So we need to prevent more interrupts coming in which
		 * will affect the nested value as we are going outside of
		 * normal interrupt handling procedure.
		 *
		 * Setting nested to 1 has two effects:
		 * 1. Force return_to() to choose a new thread.
		 *    Since the current thread is being terminated, it will
		 *    not be chosen again.
		 * 2. When context switches to the newly chosen thread,
		 *    nested must be zero for normal code execution,
		 *    as that is not in interrupt context at all.
		 *    After returning from this function, the rest of
		 *    interrupt handling code will decrement nested,
		 *    resulting it being zero before switching to another
		 *    thread.
		 */
		__asm__ volatile("rsil %0, %1"
				: "=r" (ignore) : "i"(XCHAL_EXCM_LEVEL));

		arch_curr_cpu()->nested = 1;
	}

#if defined(CONFIG_XTENSA_MMU)
	if (is_dblexc) {
		XTENSA_WSR(ZSR_DEPC_SAVE_STR, 0);
	}
#endif /* CONFIG_XTENSA_MMU */

	return return_to(interrupted_stack);
}

#if defined(CONFIG_GDBSTUB)
void *xtensa_debugint_c(int *interrupted_stack)
{
	extern void z_gdb_isr(struct arch_esf *esf);

	z_gdb_isr((void *)interrupted_stack);

	return return_to(interrupted_stack);
}
#endif
