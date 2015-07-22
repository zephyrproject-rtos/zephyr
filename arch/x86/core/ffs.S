/* ffs.S - find first set bit APIs for IA-32 architecture */

/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
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
This module implements the ffsMsb() and ffsLsb() functions for the IA-32 
architecture.

INTERNAL
Inline versions of these APIs, find_last_set_inline() and find_first_set_inline(),
are defined in arch.h.
 */

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <offsets.h>	/* nanokernel structure offset definitions */


	/* exports (public APIs) */

	GTEXT(find_last_set)
	GTEXT(find_first_set)

/**
 *
 * @brief Find first set bit searching from the LSB 
 *
 * This routine finds the first bit set in the passed argument and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit to 32 for the most significant bit.
 * A return value of zero indicates that the value passed is zero.
 *
 * @return bit position from 1 to 32, or 0 if the argument is zero.
 *
 * INTERNAL
 * For Intel64 (x86_64) architectures, the 'cmovz' can be removed
 * and leverage the fact that the 'bsrl' doesn't modify the destination operand
 * when the source operand is zero.  The "bitpos" variable can be preloaded
 * into the destination register, and given the unconditional ++bitpos that
 * is performed after the 'cmovz', the correct results are yielded.
 */

SECTION_FUNC(TEXT, find_first_set)

#if defined(CONFIG_CMOV)

	movl	$0xffffffff, %ecx	/* preload for 0 return value */
	bsfl	0x4(%esp), %eax		/* bit scan "forward" */
	cmovz 	%ecx, %eax		/* if operand == 0 then %ecx -> eax */ 
	addl	$1, %eax
	ret

#else

	bsfl	0x4(%esp), %eax		/* bit scan "forward" */
	jnz	ffsLsb_argNotZero
	xorl	%eax, %eax		/* return 0 when arg=0 */
	ret
	
BRANCH_LABEL(ffsLsb_argNotZero)	/* this label serves find_first_set() & find_last_set() */
	addl	$1, %eax
	ret

#endif /* CONFIG_CMOV */

 	
/**
 *
 * @brief Find first set bit searching from the MSB 
 *
 * This routine finds the first bit set in the passed argument and
 * returns the index of that bit.  Bits are numbered starting
 * at 1 from the least significant bit to 32 for the most significant bit.
 * A return value of zero indicates that the value passed is zero.
 *
 * @return bit position from 1 to 32, or 0 if the argument is zero.
 *
 * INTERNAL
 * For Intel64 (x86_64) architectures, the 'cmovz' can be removed
 * and leverage the fact that the 'bsfl' doesn't modify the destination operand
 * when the source operand is zero.  The "bitpos" variable can be preloaded
 * into the destination register, and given the unconditional ++bitpos that
 * is performed after the 'cmovz', the correct results are yielded.
 */

SECTION_FUNC(TEXT, find_last_set)

#if defined(CONFIG_CMOV)

	movl	$0xffffffff, %ecx	/* preload for 0 return value */
	bsrl	0x4(%esp), %eax		/* bit scan "reverse" */
	cmovz 	%ecx, %eax		/* if operand == 0 then %ecx -> eax */ 
	addl	$1, %eax
	ret

#else

	bsrl	0x4(%esp), %eax		/* bit scan "reverse" */
	jnz	ffsLsb_argNotZero
	xorl	%eax, %eax		/* return 0 when arg=0 */
	ret
	
#endif /* CONFIG_CMOV */

