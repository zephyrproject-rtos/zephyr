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
#include <_soc_inthandlers.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>
#include <offsets.h>
#include <zsr.h>
#include <zephyr/arch/common/exc_handle.h>

#include <xtensa_internal.h>

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

extern char xtensa_arch_except_epc[];
extern char xtensa_arch_kernel_oops_epc[];

#ifdef CONFIG_USERSPACE
Z_EXC_DECLARE(xtensa_user_string_nlen);

static const struct z_exc_handle exceptions[] = {
	Z_EXC_HANDLE(xtensa_user_string_nlen)
};
#endif /* CONFIG_USERSPACE */

void xtensa_dump_stack(const z_arch_esf_t *stack)
{
	_xtensa_irq_stack_frame_raw_t *frame = (void *)stack;
	_xtensa_irq_bsa_t *bsa = frame->ptr_to_bsa;
	uintptr_t num_high_regs;
	int reg_blks_remaining;

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

	ps = bsa->ps;
	pc = (void *)bsa->pc;

	__asm__ volatile("rsr.excvaddr %0" : "=r"(vaddr));

	LOG_ERR(" ** FATAL EXCEPTION%s", (is_dblexc ? " (DOUBLE)" : ""));
	LOG_ERR(" ** CPU %d EXCCAUSE %d (%s)",
		arch_curr_cpu()->id, cause,
		xtensa_exccause(cause));
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
	return _current_cpu->nested <= 1 ?
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

#if XCHAL_NMILEVEL >= 2
DEF_INT_C_HANDLER(2)
#endif

#if XCHAL_NMILEVEL >= 3
DEF_INT_C_HANDLER(3)
#endif

#if XCHAL_NMILEVEL >= 4
DEF_INT_C_HANDLER(4)
#endif

#if XCHAL_NMILEVEL >= 5
DEF_INT_C_HANDLER(5)
#endif

#if XCHAL_NMILEVEL >= 6
DEF_INT_C_HANDLER(6)
#endif

#if XCHAL_NMILEVEL >= 7
DEF_INT_C_HANDLER(7)
#endif

static inline DEF_INT_C_HANDLER(1)

/* C handler for level 1 exceptions/interrupts.  Hooked from the
 * DEF_EXCINT 1 vector declaration in assembly code.  This one looks
 * different because exceptions and interrupts land at the same
 * vector; other interrupt levels have their own vectors.
 */
void *xtensa_excint1_c(int *interrupted_stack)
{
	int cause;
	_xtensa_irq_bsa_t *bsa = (void *)*(int **)interrupted_stack;
	bool is_fatal_error = false;
	bool is_dblexc = false;
	uint32_t ps;
	void *pc, *print_stack = (void *)interrupted_stack;
	uint32_t depc = 0;

	__asm__ volatile("rsr.exccause %0" : "=r"(cause));

#ifdef CONFIG_XTENSA_MMU
	__asm__ volatile("rsr.depc %0" : "=r"(depc));

	is_dblexc = (depc != 0U);
#endif /* CONFIG_XTENSA_MMU */

	switch (cause) {
	case EXCCAUSE_LEVEL1_INTERRUPT:
		if (!is_dblexc) {
			return xtensa_int1_c(interrupted_stack);
		}
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
		ps = bsa->ps;
		pc = (void *)bsa->pc;

#ifdef CONFIG_USERSPACE
		/* If the faulting address is from one of the known
		 * exceptions that should not be fatal, return to
		 * the fixup address.
		 */
		for (int i = 0; i < ARRAY_SIZE(exceptions); i++) {
			if ((pc >= exceptions[i].start) &&
			    (pc < exceptions[i].end)) {
				bsa->pc = (uintptr_t)exceptions[i].fixup;

				goto fixup_out;
			}
		}
#endif /* CONFIG_USERSPACE */

		/* Default for exception */
		int reason = K_ERR_CPU_EXCEPTION;
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

		/* We are going to manipulate _current_cpu->nested manually.
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
				: "=r" (ignore) : "i"(XCHAL_NMILEVEL));

		_current_cpu->nested = 1;
	}

#ifdef CONFIG_XTENSA_MMU
#ifdef CONFIG_USERSPACE
fixup_out:
#endif
	if (is_dblexc) {
		__asm__ volatile("wsr.depc %0" : : "r"(0));
	}
#endif /* CONFIG_XTENSA_MMU */


	return return_to(interrupted_stack);
}

#if defined(CONFIG_GDBSTUB)
void *xtensa_debugint_c(int *interrupted_stack)
{
	extern void z_gdb_isr(z_arch_esf_t *esf);

	z_gdb_isr((void *)interrupted_stack);

	return return_to(interrupted_stack);
}
#endif
