/* ffs.S - ARCv2 find first set assembly routines */

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
This library implements nanoFfsMsb() and nanoFfsLsb() which returns the
most and least significant bit set respectively.
*/

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>

/* Exports */

GTEXT(nanoFfsMsb)
GTEXT(nanoFfsLsb)

/*******************************************************************************
*
* nanoFfsMsb - find first set bit (searching from the most significant bit)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit.  A return value of zero indicates that
* the value passed is zero.
*
* RETURNS: most significant bit set
*/

SECTION_FUNC(TEXT, nanoFfsMsb)

	/*
	 * The FLS instruction returns the bit number (0-31), and 0 if the operand
	 * is 0. So, the bit number must be incremented by 1 only in the case of
	 * an operand value that is non-zero.
	 */
	fls.f r0, r0
	j_s.d [blink]
	add.nz r0, r0, 1

/*******************************************************************************
*
* nanoFfsLsb - find first set bit (searching from the least significant bit)
*
* This routine finds the first bit set in the argument passed it and
* returns the index of that bit.  Bits are numbered starting
* at 1 from the least significant bit.  A return value of zero indicates that
* the value passed is zero.
*
* RETURNS: least significant bit set
*/

SECTION_FUNC(TEXT, nanoFfsLsb)

	/*
	 * The FFS instruction returns the bit number (0-31), and 31 if the operand
	 * is 0. So, the bit number must be incremented by 1, or, in the case of an
	 * operand value of 0, set to 0.
	 */
	ffs.f r0, r0
	add.nz r0, r0, 1
	j_s.d [blink]
	mov.z r0, 0
