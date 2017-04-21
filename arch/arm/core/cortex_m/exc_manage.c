/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file exception related routines
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <inttypes.h>

#include <misc/printk.h>
void sys_exc_esf_dump(NANO_ESF *esf)
{
	printk("r0/a1:  %x  ", esf->a1);
	printk("r1/a2:  %x  ", esf->a2);
	printk("r2/a3:  %x\n", esf->a3);
	printk("r3/a4:  %x  ", esf->a4);
	printk("r12/ip: %x  ", esf->ip);
	printk("r14/lr: %x\n", esf->lr);
	printk("r15/pc: %x  ", esf->pc);
	printk("xpsr:   %x\n", esf->xpsr);
#ifdef CONFIG_FLOAT
	for (int i = 0; i < 16; i += 4) {
		printk("s[%d]:  %x  s[%d]:  %x  s[%d]:  %x  s[%d]:  %x\n",
		       i, (u32_t)esf->s[i],
		       i + 1, (u32_t)esf->s[i + 1],
		       i + 2, (u32_t)esf->s[i + 2],
		       i + 3, (u32_t)esf->s[i + 3]);
	}
	printk("fpscr:  %x\n", esf->fpscr);
#endif
}

