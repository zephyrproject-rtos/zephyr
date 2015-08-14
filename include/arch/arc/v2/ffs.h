/* v2/ffs.h - ARCv2 public nanokernel find-first-set interface */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
ARC-specific nanokernel ffs interface. Included by ARC/arch.h.
 */

#ifndef _ARCH_ARC_V2_FFS_H_
#define _ARCH_ARC_V2_FFS_H_

#ifndef _ASMLANGUAGE

/**
 *
 * @brief Find First Set bit (searching from most significant bit)
 *
 * This routine finds the last bit set in the argument passed it and returns
 * the index of that bit.  Bits are numbered starting at 1 from the least
 * significant bit.  A return value of zero indicates that the value passed
 * is zero.
 *
 * @return most significant bit set
 */

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int find_last_set(unsigned int op)
{
	unsigned int bit;

	__asm__ volatile(

		/* see explanation in ffs.S */
		"fls.f %0, %1;\n\t"
		"add.nz %0, %0, 1;\n\t"
		: "=r"(bit)
		: "r"(op));

	return bit;
}
#endif

/**
 *
 * @brief Find first set bit (searching from the least significant bit)
 *
 * This routine finds the first bit set in the argument passed it and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit.  A return value of zero indicates that
 * the value passed is zero.
 *
 * @return least significant bit set
 */

#if defined(__GNUC__)
static ALWAYS_INLINE unsigned int find_first_set(unsigned int op)
{
	unsigned int bit;

	__asm__ volatile(

		/* see explanation in ffs.S */
		"ffs.f %0, %1;\n\t"
		"add.nz %0, %0, 1;\n\t"
		"mov.z %0, 0;\n\t"
		: "=&r"(bit)
		: "r"(op));

	return bit;
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* _ARCH_ARC_V2_FFS_H_ */
