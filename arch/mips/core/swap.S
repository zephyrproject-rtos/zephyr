/*
 * Copyright (c) 2017 Imagination Technologies Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <irq.h>
#include <kernel_structs.h>
#include <offsets_short.h>

#include <mips/regdef.h>
#include <mips/asm.h>
#include <mips/cpu.h>
#include <mips/hal.h>

/* exports */
GTEXT(__swap)

/*
 * unsigned int __swap(unsigned int key)
 *
 * Always called with interrupts locked
 * key is stored in a0 register
 */
SECTION_FUNC(exception.other, __swap)

	/* Set cause to trigger a software interrupt */
    mfc0    k0, C0_CR
    addi    k0, k0, 256
    mtc0    k0, C0_CR

    /* Enable interrupts */
    mfc0 k0, C0_SR
    or  k0, k0, a0
    mtc0 k0, C0_SR
    ehb

    la k0, _kernel

	/* Get pointer to _kernel.current */
	lw k1, _kernel_offset_to_current(k0)

	/* Load return value of __swap function in temp register v0 */
	lw v0, _thread_offset_to_swap_return_value(k1)

	jr ra
