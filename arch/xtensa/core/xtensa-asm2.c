/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <xtensa-asm2.h>
#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <_soc_inthandlers.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(os);

void *xtensa_init_stack(int *stack_top,
			void (*entry)(void *, void *, void *),
			void *arg1, void *arg2, void *arg3)
{
	/* We cheat and shave 16 bytes off, the top four words are the
	 * A0-A3 spill area for the caller of the entry function,
	 * which doesn't exist.  It will never be touched, so we
	 * arrange to enter the function with a CALLINC of 1 and a
	 * stack pointer 16 bytes above the top, so its ENTRY at the
	 * start will decrement the stack pointer by 16.
	 */
	const int bsasz = BASE_SAVE_AREA_SIZE - 16;
	void **bsa = (void **) (((char *) stack_top) - bsasz);

	(void)memset(bsa, 0, bsasz);

	bsa[BSA_PC_OFF/4] = z_thread_entry;
	bsa[BSA_PS_OFF/4] = (void *)(PS_WOE | PS_UM | PS_CALLINC(1));

	/* Arguments to z_thread_entry().  Remember these start at A6,
	 * which will be rotated into A2 by the ENTRY instruction that
	 * begins the C function.  And A4-A7 and A8-A11 are optional
	 * quads that live below the BSA!
	 */
	bsa[-1] = arg1;  /* a7 */
	bsa[-2] = entry; /* a6 */
	bsa[-3] = 0;     /* a5 */
	bsa[-4] = 0;     /* a4 */

	bsa[-5] = 0;     /* a11 */
	bsa[-6] = 0;     /* a10 */
	bsa[-7] = arg3;  /* a9 */
	bsa[-8] = arg2;  /* a8 */

	/* Finally push the BSA pointer and return the stack pointer
	 * as the handle
	 */
	bsa[-9] = bsa;
	return &bsa[-9];
}

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	thread->switch_handle = xtensa_init_stack((int *)stack_ptr, entry,
						  p1, p2, p3);
}

void z_irq_spurious(void *arg)
{
	int irqs, ie;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	LOG_ERR(" ** Spurious INTERRUPT(s) %p, INTENABLE = %p",
		(void *)irqs, (void *)ie);
	z_xtensa_fatal_error(K_ERR_SPURIOUS_IRQ, NULL);
}

void z_xtensa_dump_stack(const z_arch_esf_t *stack)
{
	int *bsa = *(int **)stack;

	LOG_ERR(" **  A0 %p  SP %p  A2 %p  A3 %p",
		(void *)bsa[BSA_A0_OFF/4],
		((char *)bsa) + BASE_SAVE_AREA_SIZE,
		(void *)bsa[BSA_A2_OFF/4], (void *)bsa[BSA_A3_OFF/4]);

	if (bsa - stack > 4) {
		LOG_ERR(" **  A4 %p  A5 %p  A6 %p  A7 %p",
			(void *)bsa[-4], (void *)bsa[-3],
			(void *)bsa[-2], (void *)bsa[-1]);
	}

	if (bsa - stack > 8) {
		LOG_ERR(" **  A8 %p  A9 %p A10 %p A11 %p",
			(void *)bsa[-8], (void *)bsa[-7],
			(void *)bsa[-6], (void *)bsa[-5]);
	}

	if (bsa - stack > 12) {
		LOG_ERR(" ** A12 %p A13 %p A14 %p A15 %p",
			(void *)bsa[-12], (void *)bsa[-11],
			(void *)bsa[-10], (void *)bsa[-9]);
	}

#if XCHAL_HAVE_LOOPS
	LOG_ERR(" ** LBEG %p LEND %p LCOUNT %p",
		(void *)bsa[BSA_LBEG_OFF/4],
		(void *)bsa[BSA_LEND_OFF/4],
		(void *)bsa[BSA_LCOUNT_OFF/4]);
#endif

	LOG_ERR(" ** SAR %p", (void *)bsa[BSA_SAR_OFF/4]);
}

static inline unsigned int get_bits(int offset, int num_bits, unsigned int val)
{
	int mask;

	mask = BIT(num_bits) - 1;
	val = val >> offset;
	return val & mask;
}

/* The wrapper code lives here instead of in the python script that
 * generates _xtensa_handle_one_int*().  Seems cleaner, still kind of
 * ugly.
 */
#define DEF_INT_C_HANDLER(l)				\
void *xtensa_int##l##_c(void *interrupted_stack)	\
{							   \
	uint32_t irqs, intenable, m;			   \
	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs)); \
	__asm__ volatile("rsr.intenable %0" : "=r"(intenable)); \
	irqs &= intenable;					\
	while ((m = _xtensa_handle_one_int##l(irqs))) {		\
		irqs ^= m;					\
		__asm__ volatile("wsr.intclear %0" : : "r"(m)); \
	}							\
	return z_get_next_switch_handle(interrupted_stack);		\
}

DEF_INT_C_HANDLER(2)
DEF_INT_C_HANDLER(3)
DEF_INT_C_HANDLER(4)
DEF_INT_C_HANDLER(5)
DEF_INT_C_HANDLER(6)
DEF_INT_C_HANDLER(7)

static inline DEF_INT_C_HANDLER(1)

/* C handler for level 1 exceptions/interrupts.  Hooked from the
 * DEF_EXCINT 1 vector declaration in assembly code.  This one looks
 * different because exceptions and interrupts land at the same
 * vector; other interrupt levels have their own vectors.
 */
void *xtensa_excint1_c(int *interrupted_stack)
{
	int cause, vaddr, *bsa = *(int **)interrupted_stack;

	__asm__ volatile("rsr.exccause %0" : "=r"(cause));

	if (cause == EXCCAUSE_LEVEL1_INTERRUPT) {

		return xtensa_int1_c(interrupted_stack);

	} else if (cause == EXCCAUSE_SYSCALL) {

		/* Just report it to the console for now */
		LOG_ERR(" ** SYSCALL PS %p PC %p",
			(void *)bsa[BSA_PS_OFF/4], (void *)bsa[BSA_PC_OFF/4]);
		z_xtensa_dump_stack(interrupted_stack);

		/* Xtensa exceptions don't automatically advance PC,
		 * have to skip the SYSCALL instruction manually or
		 * else it will just loop forever
		 */
		bsa[BSA_PC_OFF/4] += 3;

	} else {
		uint32_t ps = bsa[BSA_PS_OFF/4];

		__asm__ volatile("rsr.excvaddr %0" : "=r"(vaddr));

		LOG_ERR(" ** FATAL EXCEPTION");
		LOG_ERR(" ** CPU %d EXCCAUSE %d (%s)",
			arch_curr_cpu()->id, cause,
			z_xtensa_exccause(cause));
		LOG_ERR(" **  PC %p VADDR %p",
			(void *)bsa[BSA_PC_OFF/4], (void *)vaddr);
		LOG_ERR(" **  PS %p", (void *)bsa[BSA_PS_OFF/4]);
		LOG_ERR(" **    (INTLEVEL:%d EXCM: %d UM:%d RING:%d WOE:%d OWB:%d CALLINC:%d)",
			get_bits(0, 4, ps), get_bits(4, 1, ps),
			get_bits(5, 1, ps), get_bits(6, 2, ps),
			get_bits(18, 1, ps),
			get_bits(8, 4, ps), get_bits(16, 2, ps));

		/* FIXME: legacy xtensa port reported "HW" exception
		 * for all unhandled exceptions, which seems incorrect
		 * as these are software errors.  Should clean this
		 * up.
		 */
		z_xtensa_fatal_error(K_ERR_CPU_EXCEPTION,
				     (void *)interrupted_stack);
	}

	return z_get_next_switch_handle(interrupted_stack);
}

int z_xtensa_irq_is_enabled(unsigned int irq)
{
	uint32_t ie;

	__asm__ volatile("rsr.intenable %0" : "=r"(ie));

	return (ie & (1 << irq)) != 0;
}
