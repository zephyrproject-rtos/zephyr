/* CortexM/ffs.h - Cortex-M public nanokernel find-first-set interface */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
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

/*
DESCRIPTION
ARM-specific nanokernel ffs interface. Included by ARM/arch.h.
*/

#ifndef _ARCH_ARM_CORTEXM_FFS_H_
#define _ARCH_ARM_CORTEXM_FFS_H_

#ifdef _ASMLANGUAGE
GTEXT(find_first_set);
GTEXT(find_last_set);
#else
extern unsigned find_first_set(unsigned int);
extern unsigned find_last_set(unsigned int);

/*******************************************************************************
*
* find_last_set_inline - Find First Set bit (searching from most significant bit)
*
* This routine finds the first bit set in the argument passed it and returns
* the index of that bit.  Bits are numbered starting at 1 from the least
* significant bit.  A return value of zero indicates that the value passed
* is zero.
*
* RETURNS: most significant bit set
*/

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int find_last_set_inline(unsigned int op)
{
	unsigned int bit;

	__asm__ volatile(
		"cmp %1, #0;\n\t"
		"itt ne;\n\t"
		"   clzne %1, %1;\n\t"
		"   rsbne %0, %1, #32;\n\t"
		: "=r"(bit)
		: "r"(op));

	return bit;
}
#elif defined(__DCC__)
__asm volatile unsigned int find_last_set_inline(unsigned int op)
{
	% reg op !"r0" cmp op, #0 itt ne clzne r0, op rsbne r0, r0, #32
}
#endif

/*******************************************************************************
*
* find_first_set - find first set bit (searching from the least significant bit)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit.  A return value of zero indicates that
* the value passed is zero.
*
* RETURNS: least significant bit set
*/

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int find_first_set_inline(unsigned int op)
{
	unsigned int bit;

	__asm__ volatile(
		"rsb %0, %1, #0;\n\t"
		"ands %0, %0, %1;\n\t" /* r0 = x & (-x): only LSB set */
		"itt ne;\n\t"
		"   clzne %0, %0;\n\t" /* count leading zeroes */
		"   rsbne %0, %0, #32;\n\t"
		: "=&r"(bit)
		: "r"(op));

	return bit;
}
#elif defined(__DCC__)
__asm volatile unsigned int find_first_set_inline(unsigned int op)
{
	% reg op !"r0", "r1" rsb r1, op, #0 ands r0, r1, op itt ne clzne r0,
		r0 rsbne r0, r0, #32
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _ARCH_ARM_CORTEXM_FFS_H_ */
