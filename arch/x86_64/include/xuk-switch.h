/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _XUK_SWITCH_H
#define _XUK_SWITCH_H

/* This lives separate from the rest of the xuk API, as it has
 * to be inlined into Zephyr code.
 */

static inline void xuk_switch(void *switch_to, void **switched_from)
{
	/* Constructs an IRETQ interrupt frame, the final CALL pushes
	 * the RIP to which to return
	 */
	__asm__ volatile("mov %%rsp, %%rcx;"
			 "pushq $0x10;"      /* SS */
			 "pushq %%rcx;"      /* RSP */
			 "pushfq;"           /* RFLAGS */
			 "pushq $0x08;"      /* CS */
			 "callq _switch_top"
			 : : "a"(switch_to), "d"(switched_from)
			 : "ecx", "memory");
}

#endif /* _XUK_SWITCH_H */
