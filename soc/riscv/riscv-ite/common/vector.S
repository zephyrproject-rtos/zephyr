/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * Jyunlin Chen <jyunlin.chen@ite.com.tw>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(__irq_wrapper)

SECTION_FUNC(vectors, __start)
	.option norvc;

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to __irq_wrapper.
	 */
	la t0, __irq_wrapper
	csrw mtvec, t0
	csrwi mie, 0

	/* Jump to __initialize */
	tail __initialize

/**
 *-----------------------------------------------------------------------------
 * 16-bytes signature
 *-----------------------------------------------------------------------------
 */
    .org 0x84
                ## 0    1    2    3    4    5    6                     7
    .byte       0xA5,0xA5,0xA5,0xA5,0xA5,0xA5,0xA5,0xB5
                ## 8    9   10   11   12                    13   14   15
    .byte       0x85,0x12,0x5A,0x5A,0xAA,0xAA,0x55,0x55
/**
 *-----------------------------------------------------------------------------
 * EC firmware version
 *-----------------------------------------------------------------------------
 */

    .byte  'I', 'T', '8', 'X', 'X', 'X', '2', '-', 'A', 'X', '-',\
	'V', '0', '.', '0', '0'
    .org 0xa4
