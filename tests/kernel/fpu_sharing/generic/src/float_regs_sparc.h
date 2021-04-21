/*
 * Copyright (c) 2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLOAT_REGS_SPARC_H
#define _FLOAT_REGS_SPARC_H

#include <toolchain.h>
#include "float_context.h"

static inline void _load_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"ldd	[%0 + 0x00], %%f0\n"
		"ldd	[%0 + 0x08], %%f2\n"
		"ldd	[%0 + 0x10], %%f4\n"
		"ldd	[%0 + 0x18], %%f6\n"
		"ldd	[%0 + 0x20], %%f8\n"
		"ldd	[%0 + 0x28], %%f10\n"
		"ldd	[%0 + 0x30], %%f12\n"
		"ldd	[%0 + 0x38], %%f14\n"
		"ldd	[%0 + 0x40], %%f16\n"
		"ldd	[%0 + 0x48], %%f18\n"
		"ldd	[%0 + 0x50], %%f20\n"
		"ldd	[%0 + 0x58], %%f22\n"
		"ldd	[%0 + 0x60], %%f24\n"
		"ldd	[%0 + 0x68], %%f26\n"
		"ldd	[%0 + 0x70], %%f28\n"
		"ldd	[%0 + 0x78], %%f30\n"
		:
		: "r" (&regs->fp_volatile)
		);
}

static inline void _store_all_float_registers(struct fp_register_set *regs)
{
	__asm__ volatile (
		"std	%%f0,  [%0 + 0x00]\n"
		"std	%%f2,  [%0 + 0x08]\n"
		"std	%%f4,  [%0 + 0x10]\n"
		"std	%%f6,  [%0 + 0x18]\n"
		"std	%%f8,  [%0 + 0x20]\n"
		"std	%%f10, [%0 + 0x28]\n"
		"std	%%f12, [%0 + 0x30]\n"
		"std	%%f14, [%0 + 0x38]\n"
		"std	%%f16, [%0 + 0x40]\n"
		"std	%%f18, [%0 + 0x48]\n"
		"std	%%f20, [%0 + 0x50]\n"
		"std	%%f22, [%0 + 0x58]\n"
		"std	%%f24, [%0 + 0x60]\n"
		"std	%%f26, [%0 + 0x68]\n"
		"std	%%f28, [%0 + 0x70]\n"
		"std	%%f30, [%0 + 0x78]\n"
		:
		: "r" (&regs->fp_volatile)
		: "memory"
		);
}

static inline void _load_then_store_all_float_registers(struct fp_register_set
							*regs)
{
	_load_all_float_registers(regs);
	_store_all_float_registers(regs);
}
#endif /* _FLOAT_REGS_SPARC_H */
