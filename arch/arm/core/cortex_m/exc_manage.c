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
	printk("r0/a1:  0x%08x  ", esf->basic.a1);
	printk("r1/a2:  0x%08x  ", esf->basic.a2);
	printk("r2/a3:  0x%08x\n", esf->basic.a3);
	printk("r3/a4:  0x%08x  ", esf->basic.a4);
	printk("r12/ip: 0x%08x  ", esf->basic.ip);
	printk("r14/lr: 0x%08x\n", esf->basic.lr);
	printk("r15/pc: 0x%08x  ", esf->basic.pc);
	printk("xpsr:   0x%08x\n", esf->basic.xpsr);
#if defined(CONFIG_FLOAT) && defined(CONFIG_FP_SHARING)
	for (int i = 0; i < 16; i += 4) {
		printk("s[%d]:  0x%08x  s[%d]:  0x%08x"
			"  s[%d]:  0x%08x  s[%d]:  0x%08x\n",
		       i, (u32_t)esf->s[i],
		       i + 1, (u32_t)esf->s[i + 1],
		       i + 2, (u32_t)esf->s[i + 2],
		       i + 3, (u32_t)esf->s[i + 3]);
	}
	printk("fpscr:  0x%08x\n", esf->fpscr);
#endif
}

