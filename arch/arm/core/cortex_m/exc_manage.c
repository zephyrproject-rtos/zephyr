/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file exception related routines
 */

#include <nanokernel.h>
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

