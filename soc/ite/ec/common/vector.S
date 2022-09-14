/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 * Jyunlin Chen <jyunlin.chen@ite.com.tw>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "chip_chipregs.h"
#include <zephyr/toolchain.h>

/* exports */
GTEXT(__start)

/* imports */
GTEXT(__initialize)
GTEXT(_isr_wrapper)

SECTION_FUNC(vectors, __start)
#ifdef CONFIG_RISCV_GP
	.option push
	.option norelax
	/* Configure the GP register */
	la gp, __global_pointer$
	.option pop
#endif

	.option norvc;

	/*
	 * Set mtvec (Machine Trap-Vector Base-Address Register)
	 * to _isr_wrapper.
	 */
	la t0, _isr_wrapper
	csrw mtvec, t0
	csrwi mie, 0
#if (CONFIG_SOC_IT8XXX2_FLASH_SIZE_BYTES == 0x100000)
	/*
	 * bit[3-0]@EIDSR=8: instruction local memory size is 1M byte
	 * This operation must be done before accessing memory.
	 */
	la t0, IT8XXX2_GCTRL_EIDSR
	lb t1, 0(t0)
	andi t1, t1, 0xf0
	ori  t1, t1, 0x8
	sb t1, 0(t0)
#endif
	/* Jump to __initialize */
	tail __initialize

/*
 * eflash signature used to enable specific function after power-on reset.
 * (HW mechanism)
 * The content of 16-bytes must be the following and at offset 0x80 of binary.
 * ----------------------------------------------------------------------------
 * 1st 2nd 3rd 4th 5th 6th 7th    8th    9th 10th 11th 12th 13th 14th 15th 16th
 * ----------------------------------------------------------------------------
 * A5h A5h A5h A5h A5h A5h [host] [flag] 85h  12h  5Ah  5Ah  AAh  AAh  55h  55h
 * ----------------------------------------------------------------------------
 * [host]: A4h = enable eSPI, A5h = enable LPC
 * [flag]:
 * bit7: it must be 1b.
 * bit6: it must be 0b.
 * bit5: it must be 1b.
 * bit4: 1b = 32.768KHz is from the internal clock generator.
 * bit3: it must be 0b.
 * bit2: it must be 1b.
 * bit1: it must be 0b.
 * bit0: it must be 0b.
 */
.org 0x80
.balign 16
.global eflash_sig
eflash_sig:
.byte 0xA5, 0xA5, 0xA5, 0xA5, 0xA5, 0xA5
#ifdef CONFIG_ESPI
.byte 0xA4 /* enable eSPI */
#else
.byte 0xA5 /* enable LPC */
#endif
/* flag of signature */
#ifdef CONFIG_SOC_IT8XXX2_EXT_32K
.byte 0xA4 /* use external 32.768 kHz oscillator */
#else
.byte 0xB4 /* enable internal clock generator */
#endif
.byte 0x85, 0x12, 0x5A, 0x5A, 0xAA, 0xAA, 0x55, 0x55
