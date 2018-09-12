/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <misc/printk.h>
#include <string.h>
#include <xtensa-asm2.h>
#include <kernel.h>
#include <ksched.h>
#include <kernel_structs.h>
#include <kernel_internal.h>
#include <kswap.h>
#include <_soc_inthandlers.h>

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

	bsa[BSA_PC_OFF/4] = _thread_entry;
	bsa[BSA_PS_OFF/4] = (void *)(PS_WOE | PS_UM | PS_CALLINC(1));

	/* Arguments to _thread_entry().  Remember these start at A6,
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

/* This is a kernel hook, just a wrapper around other APIs.  Build
 * only if we're using asm2 as the core OS interface and not just as
 * utilities/testables.
 */
#ifdef CONFIG_XTENSA_ASM2
void _new_thread(struct k_thread *thread, k_thread_stack_t *stack, size_t sz,
		 k_thread_entry_t entry, void *p1, void *p2, void *p3,
		 int prio, unsigned int opts)
{
	char *base = K_THREAD_STACK_BUFFER(stack);
	char *top = base + sz;

	/* Align downward.  The API as specified requires a runtime check. */
	top = (char *)(((unsigned int)top) & ~3);

	_new_thread_init(thread, base, sz, prio, opts);

	thread->switch_handle = xtensa_init_stack((void *)top, entry,
						  p1, p2, p3);
}
#endif

#ifdef CONFIG_XTENSA_ASM2
void _irq_spurious(void *arg)
{
	int irqs, ie;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs));
	__asm__ volatile("rsr.intenable %0" : "=r"(ie));
	printk(" ** Spurious INTERRUPT(s) %p, INTENABLE = %p\n",
	       (void *)irqs, (void *)ie);
	_NanoFatalErrorHandler(_NANO_ERR_RESERVED_IRQ, &_default_esf);
}
#endif

static void dump_stack(int *stack)
{
	int *bsa = *(int **)stack;

	printk(" **  A0 %p  SP %p  A2 %p  A3 %p\n",
	       (void *)bsa[BSA_A0_OFF/4], ((char *)bsa) + BASE_SAVE_AREA_SIZE,
	       (void *)bsa[BSA_A2_OFF/4], (void *)bsa[BSA_A3_OFF/4]);

	if (bsa - stack > 4) {
		printk(" **  A4 %p  A5 %p  A6 %p  A7 %p\n",
		       (void *)bsa[-4], (void *)bsa[-3],
		       (void *)bsa[-2], (void *)bsa[-1]);
	}

	if (bsa - stack > 8) {
		printk(" **  A8 %p  A9 %p A10 %p A11 %p\n",
		       (void *)bsa[-8], (void *)bsa[-7],
		       (void *)bsa[-6], (void *)bsa[-5]);
	}

	if (bsa - stack > 12) {
		printk(" ** A12 %p A13 %p A14 %p A15 %p\n",
		       (void *)bsa[-12], (void *)bsa[-11],
		       (void *)bsa[-10], (void *)bsa[-9]);
	}

#if XCHAL_HAVE_LOOPS
	printk(" ** LBEG %p LEND %p LCOUNT %p\n",
	       (void *)bsa[BSA_LBEG_OFF/4],
	       (void *)bsa[BSA_LEND_OFF/4],
	       (void *)bsa[BSA_LCOUNT_OFF/4]);

#endif

	printk(" ** SAR %p\n", (void *)bsa[BSA_SAR_OFF/4]);
}

/* The wrapper code lives here instead of in the python script that
 * generates _xtensa_handle_one_int*().  Seems cleaner, still kind of
 * ugly.
 */
#define DEF_INT_C_HANDLER(l)				\
void *xtensa_int##l##_c(void *interrupted_stack)	\
{							   \
	int irqs, m;					   \
	__asm__ volatile("rsr.interrupt %0" : "=r"(irqs)); \
							   \
	while ((m = _xtensa_handle_one_int##l(irqs))) {		\
		irqs ^= m;					\
		__asm__ volatile("wsr.intclear %0" : : "r"(m)); \
	}							\
	return _get_next_switch_handle(interrupted_stack);		\
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
		printk(" ** SYSCALL PS %p PC %p\n",
		       (void *)bsa[BSA_PS_OFF/4], (void *)bsa[BSA_PC_OFF/4]);
		dump_stack(interrupted_stack);

		/* Xtensa exceptions don't automatically advance PC,
		 * have to skip the SYSCALL instruction manually or
		 * else it will just loop forever
		 */
		bsa[BSA_PC_OFF/4] += 3;

	} else {
		__asm__ volatile("rsr.excvaddr %0" : "=r"(vaddr));

		/* Wouldn't hurt to translate EXCCAUSE to a string for
		 * the user...
		 */
		printk(" ** FATAL EXCEPTION\n");
		printk(" ** CPU %d EXCCAUSE %d PS %p PC %p VADDR %p\n",
		       _arch_curr_cpu()->id, cause, (void *)bsa[BSA_PS_OFF/4],
		       (void *)bsa[BSA_PC_OFF/4], (void *)vaddr);

		dump_stack(interrupted_stack);

		/* FIXME: legacy xtensa port reported "HW" exception
		 * for all unhandled exceptions, which seems incorrect
		 * as these are software errors.  Should clean this
		 * up.
		 */
		_NanoFatalErrorHandler(_NANO_ERR_HW_EXCEPTION, &_default_esf);
	}

	return _get_next_switch_handle(interrupted_stack);
}

