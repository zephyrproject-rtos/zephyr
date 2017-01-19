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
	printk("r0/a1:  %" PRIx32 "  ", esf->a1);
	printk("r1/a2:  %" PRIx32 "  ", esf->a2);
	printk("r2/a3:  %" PRIx32 "\n", esf->a3);
	printk("r3/a4:  %" PRIx32 "  ", esf->a4);
	printk("r12/ip: %" PRIx32 "  ", esf->ip);
	printk("r14/lr: %" PRIx32 "\n", esf->lr);
	printk("r15/pc: %" PRIx32 "  ", esf->pc);
	printk("xpsr:   %" PRIx32 "\n", esf->xpsr);
#ifdef CONFIG_FLOAT
	for (int i = 0; i < 16; i += 4) {
		printk("s[%d]:  %" PRIx32 "  s[%d]:  %" PRIx32 "  s[%d]:  %"
		       PRIx32 "  s[%d]:  %" PRIx32 "\n",
		       i, (uint32_t)esf->s[i],
		       i + 1, (uint32_t)esf->s[i + 1],
		       i + 2, (uint32_t)esf->s[i + 2],
		       i + 3, (uint32_t)esf->s[i + 3]);
	}
	printk("fpscr:  %" PRIx32 "\n", esf->fpscr);
#endif
}

