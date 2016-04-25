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
 * @file dynamic exception management
 */

/* can only be used with non-XIP kernels, since they don't have their vector
 * table in the FLASH
 */
#if !defined(CONFIG_XIP)

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <sections.h>
#include "vector_table.h"

static inline int exc_can_be_connected(int num)
{
	static const uint16_t connectable_exceptions = (
		(1 << _EXC_MPU_FAULT) |
		(1 << _EXC_BUS_FAULT) |
		(1 << _EXC_USAGE_FAULT) |
		(1 << _EXC_DEBUG) |
		0
	);

	return !!(connectable_exceptions & (1 << num));
}

/*
 * Can be initialized with garbage, it doesn't matter until the wrapper is
 * inserted in the vector table, at which point the relevant entry will contain
 * the pointer to the handler.
 */
sys_exc_handler_t *_sw_exc_table[_NUM_EXC] __noinit;

extern void _exc_wrapper(void);
void sys_exc_connect(unsigned int num, sys_exc_handler_t *handler, void *unused)
{
	__ASSERT(exc_can_be_connected(num), "not a connectable exception");

	_sw_exc_table[num] = handler;

	/*
	 * The compiler sets thumb bit (bit0) of the value of the _vector_table
	 * symbol, probably because it is in the .text section: to get the correct
	 * offset in the table, mask bit0.
	 */
	((void **)(((uint32_t)_vector_table) & 0xfffffffe))[num] = _exc_wrapper;
}

FUNC_ALIAS(sys_exc_connect, nanoCpuExcConnect, void);

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
			i, esf->s[i], i + 1, esf->s[i + 1],
			i + 2, esf->s[i + 2], i + 3, esf->s[i + 3]);
	}
	printk("fpscr:  %x\n", esf->fpscr);
#endif
}

#endif /* CONFIG_XIP */
