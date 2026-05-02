/*
 * Copyright (c) 2021 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * Macros for MIPS CP0 registers manipulations
 * inspired by linux/arch/mips/include/asm/mipsregs.h
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ZEPHYR_ARCH_MIPS_INCLUDE_MIPS_MIPSREGS_H_
#define _ZEPHYR_ARCH_MIPS_INCLUDE_MIPS_MIPSREGS_H_

#define CP0_BADVADDR	$8
#define CP0_COUNT	$9
#define CP0_COMPARE	$11
#define CP0_STATUS	$12
#define CP0_CAUSE	$13
#define CP0_EPC		$14

/* CP0_STATUS bits */
#define ST0_IE		0x00000001
#define ST0_EXL		0x00000002
#define ST0_ERL		0x00000004
#define ST0_IP0		0x00000100
#define ST0_BEV		0x00400000

/* CP0_CAUSE bits */
#define CAUSE_EXP_MASK	0x0000007c
#define CAUSE_EXP_SHIFT	2
#define CAUSE_IP_MASK	0x0000ff00
#define CAUSE_IP_SHIFT	8

#define _mips_read_32bit_c0_register(reg) \
({ \
	uint32_t val; \
	__asm__ __volatile__("mfc0\t%0, " STRINGIFY(reg) "\n" \
	: "=r" (val)); \
	val; \
})

#define _mips_write_32bit_c0_register(reg, val) \
({ \
	__asm__ __volatile__("mtc0 %z0, " STRINGIFY(reg) "\n" \
	: \
	: "Jr" ((uint32_t)(val))); \
})

#define read_c0_status()	_mips_read_32bit_c0_register(CP0_STATUS)
#define write_c0_status(val)	_mips_write_32bit_c0_register(CP0_STATUS, val)

#define read_c0_cause()		_mips_read_32bit_c0_register(CP0_CAUSE)

#endif /* _ZEPHYR_ARCH_MIPS_INCLUDE_MIPS_MIPSREGS_H_ */
