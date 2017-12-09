/*
 * Copyright (c) 2017, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include <xtensa-asm2.h>
#include <kernel.h>
#include <kernel_structs.h>

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

	memset(bsa, 0, bsasz);

	bsa[BSA_PC_OFF/4] = entry;
	bsa[BSA_PS_OFF/4] = (void *)(PS_WOE | PS_UM | PS_CALLINC(1));

	/* Arguments.  Remember these start at A6, which will be
	 * rotated into A2 by the ENTRY instruction that begins the
	 * entry function.  And A4-A7 and A8-A11 are optional quads
	 * that live below the BSA!
	 */
	bsa[-1] = arg2; /* a7 */
	bsa[-2] = arg1; /* a6 */
	bsa[-3] = 0;    /* a5 */
	bsa[-4] = 0;    /* a4 */

	bsa[-5] = 0;    /* a11 */
	bsa[-6] = 0;    /* a10 */
	bsa[-7] = 0;    /* a9 */
	bsa[-8] = arg3; /* a8 */

	/* Finally push the BSA pointer and return the stack pointer
	 * as the handle
	 */
	bsa[-9] = bsa;
	return &bsa[-9];
}

void _new_thread(struct k_thread *thread, k_thread_stack_t *stack, size_t sz,
		 k_thread_entry_t entry, void *p1, void *p2, void *p3,
		 int prio, unsigned int opts)
{
	char *base = K_THREAD_STACK_BUFFER(stack);
	char *top = base + sz;

	__ASSERT((((size_t)top) & 3) == 0, "Misaligned stack");

	_new_thread_init(thread, base, sz, prio, opts);

	thread->switch_handle = xtensa_init_stack(top, entry, p1, p2, p3);
}
