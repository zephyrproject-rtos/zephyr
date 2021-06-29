/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

#ifndef _ARC

#include <stdint.h>
#define _Interrupt __attribute__((interrupt("ilink")))
#define _lr __builtin_arc_lr
#define _sr __builtin_arc_sr
#define _Uncached volatile

unsigned int __JLI_TABLE__[1];

typedef struct {
	void (*target)();
} vect_entry_type;

#define INT_VECTOR_BASE 0x25
#define VECT_START _lr(INT_VECTOR_BASE)
#define IRQ_INTERRUPT 0x40b
#define IRQ_PRIORITY 0x206
#define IDENTITY 0x0004
#define STATUS32 0x000a
#define ISA_CONFIG 0x00c1

void _setvecti(int vector, _Interrupt void (*target)())
{
	volatile vect_entry_type *vtab = (_Uncached vect_entry_type *)VECT_START;
	vtab[vector].target = (void (*)())target;
	return;
}

void _sleep(int n)
{
	return;
}

void _init_ad(void)
{
	uint32_t stat32 = _lr(STATUS32) | 0x80000;

	__builtin_arc_flag(stat32);
}

int start_init(void)
{
	uint32_t status = 0; /* OK */
	uint32_t identity_rg = _lr(IDENTITY);

	if ((identity_rg & 0xff) > 0x40) {
		/* ARCv2EM */
		uint32_t isa = _lr(ISA_CONFIG);

		if (isa & 0x400000) { /* check processor support unaligned accesses */
			_init_ad(); /* allows unaligned accesses */
		} else {
			status = 1; /* Error */
		}
	} else {
		status = 1; /* Error */
	}

	return status;
}

#endif
