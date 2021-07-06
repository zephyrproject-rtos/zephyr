/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CPU_H_
#define _CPU_H_

#if !defined(__ASSEMBLER__)
#include <sys/types.h>
#endif

/*
 * MIPS32 Cause Register (CP0 Register 13, Select 0)
 */
#define CR_IP_MASK	0x0000ff00
#define  CR_IP_SHIFT		 8

/* interrupt pending bits */
#define CR_HINT5	0x00008000	/* h/w interrupt 5 */
#define CR_HINT4	0x00004000	/* h/w interrupt 4 */
#define CR_HINT3	0x00002000	/* h/w interrupt 3 */
#define CR_HINT2	0x00001000	/* h/w interrupt 2 */
#define CR_HINT1	0x00000800	/* h/w interrupt 1 */
#define CR_HINT0	0x00000400	/* h/w interrupt 0 */
#define CR_SINT1	0x00000200	/* s/w interrupt 1 */
#define CR_SINT0	0x00000100	/* s/w interrupt 0 */

/*
 * MIPS32 Status Register  (CP0 Register 12, Select 0)
 */
#define SR_BEV		0x00400000	/* boot exception vectors */

/* interrupt mask bits */
#define SR_HINT5	0x00008000	/* enable h/w interrupt 6 */
#define SR_HINT4	0x00004000	/* enable h/w interrupt 5 */
#define SR_HINT3	0x00002000	/* enable h/w interrupt 4 */
#define SR_HINT2	0x00001000	/* enable h/w interrupt 3 */
#define SR_HINT1	0x00000800	/* enable h/w interrupt 2 */
#define SR_HINT0	0x00000400	/* enable h/w interrupt 1 */
#define SR_SINT1	0x00000200	/* enable s/w interrupt 1 */
#define SR_SINT0	0x00000100	/* enable s/w interrupt 0 */

#define SR_IE		0x00000001	/* interrupt enable */

#ifdef __ASSEMBLER__

/*
 * MIPS32 Coprocessor 0 register numbers
 */
#define C0_BADVADDR	$8
#define C0_SR		$12
#define C0_CAUSE	$13
#define C0_CR		$13
#define C0_EPC		$14

#define CP0_COUNT	$9
#define CP0_COMPARE	$11
#define CP0_STATUS	$12
#define CP0_CAUSE	$13
#define CP0_CONFIG	$16

#else /* !__ASSEMBLER__ */

/*
 * Standard types
 */
typedef unsigned int		reg32_t;	/* a 32-bit register */
#if _MIPS_SIM == _ABIO32
typedef unsigned int		reg_t;
#else
typedef unsigned long long	reg_t;
#endif

/*
 * MIPS32 Coprocessor 0 register encodings for C use.
 * These encodings are implementation specific.
 */
#define C0_COUNT	9
#define C0_COMPARE	11
#define C0_SR		12
#define C0_CR		13

#define CP0_COUNT	9
#define CP0_COMPARE	11
#define CP0_STATUS	12
#define CP0_CAUSE	13
#define CP0_CONFIG	16

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__ASSEMBLER__)

/*
 * Define macros for accessing the MIPS32 coprocessor 0 registers. Most apart
 * from "set" return the original register value. These macros take an encoded
 * (register, select) combination, so they can use the textual names above.
 */

#define _mips_mfc0(selreg) \
__extension__ ({ \
	register unsigned long __r; \
	__asm__ __volatile ("mfc0 %0,$%1,%2" \
		      : "=d" (__r) \
		      : "JK" (selreg & 0x1F), "JK" (selreg >> 8)); \
	__r; \
})

#define _mips_mtc0(selreg, val) \
do { \
	__asm__ __volatile (".set push \n"\
			".set noreorder\n"\
			"mtc0 %z0,$%1,%2\n"\
			"ehb\n" \
			".set pop" \
			: \
			: "dJ" ((reg32_t)(val)), "JK" (selreg & 0x1F),\
			  "JK" (selreg >> 8) \
			: "memory"); \
} while (0)


/* bit clear non-zero bits from CLR in cp0 register REG */
#define _mips_bcc0(reg, clr) \
__extension__ ({ \
	register reg32_t __o; \
	__o = _mips_mfc0(reg); \
	_mips_mtc0(reg, __o & ~(clr)); \
	__o; \
})

/* bit set non-zero bits from SET in cp0 register REG */
#define _mips_bsc0(reg, set) \
__extension__ ({ \
	register reg32_t __o; \
	__o = _mips_mfc0(reg); \
	_mips_mtc0(reg, __o | (set)); \
	__o; \
})

/*
 * Standard MIPS CP0 register access functions
 */

/* CP0 Status register (NOTE: not atomic operations) */
#define mips_getsr()		_mips_mfc0(C0_SR)
#define mips_setsr(v)		_mips_mtc0(C0_SR, v)
#define mips_bicsr(clr)		_mips_bcc0(C0_SR, clr)
#define mips_bissr(set)		_mips_bsc0(C0_SR, set)

/* CP0 Cause register (NOTE: not atomic operations) */
#define mips_getcr()		_mips_mfc0(C0_CR)
#define mips_setcr(v)		_mips_mtc0(C0_CR, v)
#define mips_biccr(clr)		_mips_bcc0(C0_CR, clr)
#define mips_biscr(set)		_mips_bsc0(C0_CR, set)

/* CP0 Count register */
#define mips_getcount()		_mips_mfc0(C0_COUNT)
#define mips_setcount(v)	_mips_mtc0(C0_COUNT, v)

/* CP0 Compare register*/
#define mips_getcompare()	_mips_mfc0(C0_COMPARE)
#define mips_setcompare(v)	_mips_mtc0(C0_COMPARE, v)

#endif /* !ASSEMBLER */

#ifdef __cplusplus
}
#endif
#endif /*_CPU_H_*/
