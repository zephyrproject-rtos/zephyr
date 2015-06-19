/* atomic.S - nanokernel atomic operators for IA-32 */

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
This module provides the atomic operators for IA-32 architectures on BSPs
that support the LOCK prefix instruction.

The atomic operations are guaranteed to be atomic with respect to interrupt
service routines, and to operations performed by peer processors.

INTERNAL
These operators are currently unavailable to user space applications,
as there is no requirement for this capability.
*/

/* includes */

#if !defined(CONFIG_LOCK_INSTRUCTION_UNSUPPORTED)

#define _ASMLANGUAGE

#include <arch/x86/asm.h>

	/* exports (public APIs) */

	GTEXT(atomic_cas)
	GTEXT(atomic_add)
	GTEXT(atomic_sub)
	GTEXT(atomic_inc)
	GTEXT(atomic_dec)
	GTEXT(atomic_get)
	GTEXT(atomic_set)
	GTEXT(atomic_clear)
	GTEXT(atomic_or)
	GTEXT(atomic_xor)
	GTEXT(atomic_and)
	GTEXT(atomic_nand)

/*******************************************************************************
*
* atomic_cas - atomic compare-and-set primitive
*
* This routine provides the compare-and-set operator. If the original value at
* <target> equals <oldValue>, then <newValue> is stored at <target> and the
* function returns 1.
*
* If the original value at <target> does not equal <oldValue>, then the store
* is not done and the function returns 0.
*
* The reading of the original value at <target>, the comparison,
* and the write of the new value (if it occurs) all happen atomically with
* respect to both interrupts and accesses of other processors to <target>.
*
* RETURNS: Returns 1 if <newValue> is written, 0 otherwise.
*
* int atomic_cas
*     (
*     atomic_t * target,         /@ address to be tested @/
*     atomic_val_t oldValue,      /@ value to compare against @/
*     atomic_val_t newValue       /@ value to compare against @/
*     )
*
* INTERNAL
* The 'cmpxchg' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_cas)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  SP_ARG2(%esp),%eax  /* get oldValue to cmp */
	movl  SP_ARG3(%esp),%ecx  /* get newValue to set */
	lock			  /* lock the bus during the next inst */
	cmpxchg %ecx,(%edx)	  /* if (%eax == (%edx) */
				  /*   {ZF = 1; (%edx) = %ecx;} */
				  /* else              */
				  /*   {ZF = 0; %eax = (%edx);} */
	jne   atomic_cas1
	movl  $1,%eax		  /* set return status to 1 */
	ret

BRANCH_LABEL(atomic_cas1)
	xorl  %eax,%eax		  /* set return status to 0 */
	ret


/*******************************************************************************
*
* atomic_add - atomic add primitive
*
* This routine provides the atomic addition operator. The <value> is
* atomically added to the value at <target>, placing the result at <target>,
* and the old value from <target> is returned.
* 
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_add 
*	(
*	atomic_t * target,      /@ memory location to add to @/
*	atomic_val_t value       /@ value to add @/
*	)
*
* INTERNAL
* The 'xadd' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_add)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  SP_ARG2(%esp),%eax  /* get value to add */

	lock			  /* lock the bus during the next inst */
	xadd  %eax,(%edx)	  /* TMP = %eax + (%edx) */
				  /* %eax = (%edx) */
				  /* (%edx) = TMP */
	ret


/*******************************************************************************
*
* atomic_sub - atomic subtraction primitive
*
* This routine provides the atomic subtraction operator. The <value> is
* atomically subtracted from the value at <target>, placing the result at
* <target>, and the old value from <target> is returned.
*
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_sub
*     (
*     atomic_t * target,        /@ memory location to subtract from @/
*     atomic_val_t value         /@ value to subtract @/
*     )
*
* INTERNAL
* The 'xadd' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_sub)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  SP_ARG2(%esp),%eax  /* get value to add */
	negl  %eax		  /* Negate %eax... */

	lock			  /* lock the bus during the next inst */
	xadd  %eax,(%edx)	  /* TMP = %eax + (%edx) */
				  /* %eax = (%edx) */
				  /* (%edx) = TMP */
	ret


/*******************************************************************************
*
* atomic_inc - atomic increment primitive
*
* This routine provides the atomic increment operator. The value at <target>
* is atomically incremented by 1, and the old value from <target> is returned.
*
* RETURNS: The value from <target> before the increment
* 
* atomic_val_t atomic_inc
*     (
*     atomic_t *target          /@ memory location to increment @/
*     )
*
* INTERNAL
* The 'xadd' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_inc)

	movl  SP_ARG1(%esp),%edx  /* get address */

	xorl  %eax, %eax
	incl  %eax                /* increment by 1 */

	lock			  /* lock the bus during the next inst */
	xadd  %eax,(%edx)	  /* TMP = %eax + (%edx) */
				  /* %eax = (%edx) */
				  /* (%edx) = TMP */
	ret


/*******************************************************************************
*
* atomic_dec - atomic decrement primitive
*
* This routine provides the atomic decrement operator. The value at <target>
* is atomically decremented by 1, and the old value from <target> is returned.
*
* RETURNS: The value from <target> prior to the decrement
* 
* atomic_val_t atomic_dec
*     (
*     atomic_t *target          /@ memory location to decrement @/
*     )
*
* INTERNAL
* The 'xadd' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_dec)

	movl  SP_ARG1(%esp),%edx  /* get address */
	orl   $-1,%eax		  /* dec by 1 */

	lock			  /* lock the bus during the next inst */
	xadd  %eax,(%edx)	  /* TMP = %eax + (%edx) */
				  /* %eax = (%edx) */
				  /* (%edx) = TMP */
	ret


/*******************************************************************************
*
* atomic_get - atomic get primitive
*
* This routine provides the atomic get primitive to atomically read
* a value from <target>. It simply does an ordinary load.  Note that <target>
* is expected to be aligned to a 4-byte boundary.
*
* RETURNS: The value read from <target>
* 
* atomic_t atomic_get
*     (
*     atomic_t *target          /@ memory location to read from @/
*     )
*
*/

SECTION_FUNC(TEXT, atomic_get)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  (%edx), %eax	  /* get value */
	ret


/*******************************************************************************
*
* atomic_set - atomic get-and-set primitive
*
* This routine provides the atomic set operator. The <value> is atomically
* written at <target> and the previous value at <target> is returned.
* 
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_set
*     (
*     atomic_t *target,         /@ memory location to write to @/
*     atomic_val_t  value        /@ value to set @/
*     )
*
* INTERNAL
* The XCHG instruction is executed on the specified address to
* swap in value. The value swapped out is returned by this function.
*/

SECTION_FUNC(TEXT, atomic_set)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  SP_ARG2(%esp),%eax  /* get value to set */

	/*
	 * The 'lock' prefix is not required with the 'xchg' instruction.
	 * According to the IA-32 instruction reference manual: 
	 *
	 *   "If a memory operand is referenced, the processor's locking
	 *    protocol is automatically implemented for the duration of
	 *    the exchange operation, regardless of the presence
	 *    or absence of the LOCK prefix or of the value of the IOPL."
	 */

	xchg  %eax,(%edx)	  /* set value with bus-lock */

	ret


/*******************************************************************************
*
* atomic_clear - atomic clear primitive
*
* This routine provides the atomic clear operator. The value of 0 is atomically
* written at <target> and the previous value at <target> is returned. (Hence,
* atomic_clear(pAtomicVar) is equivalent to atomic_set(pAtomicVar, 0).)
*
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_clear
*     (
*     atomic_t *target          /@ memory location to write to @/
*     )
*
*/

SECTION_FUNC(TEXT, atomic_clear)

	movl  SP_ARG1(%esp),%edx  /* get address */
	xorl  %eax,%eax		  /* clear value to set */

	/*
	 * The 'lock' prefix is not required with the 'xchg' instruction.
	 * According to the IA-32 instruction reference manual: 
	 *
	 *   "If a memory operand is referenced, the processor's locking
	 *    protocol is automatically implemented for the duration of
	 *    the exchange operation, regardless of the presence
	 *    or absence of the LOCK prefix or of the value of the IOPL."
	 */

	xchg  %eax,(%edx)	  /* swap 'clear' value with bus-lock */

	ret


/*******************************************************************************
*
* atomic_or - atomic bitwise inclusive OR primitive
*
* This routine provides the atomic bitwise inclusive OR operator. The <value>
* is atomically bitwise OR'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_or
*     (
*     atomic_t *target,         /@ memory location to be modified @/
*     atomic_val_t  value        /@ value to OR @/
*     )
*
* INTERNAL
* The 'cmpxchg' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_or)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  (%edx),%eax	  /* get old value */

BRANCH_LABEL(atomic_or_retry)

	movl  SP_ARG2(%esp),%ecx  /* get value to OR */
	orl   %eax, %ecx
	lock			  /* lock the bus during the next inst */
	cmpxchg  %ecx,(%edx)	  /* if (%eax == (%edx)) */
				  /*   {ZF = 1 ;  (%edx) = %ecx;} */
				  /* else                */
				  /*   {ZF = 0 ;  %eax = (%edx);} */
	jnz atomic_or_retry
	ret


/*******************************************************************************
*
* atomic_xor - atomic bitwise exclusive OR (XOR) primitive
*
* This routine provides the atomic bitwise exclusive OR operator. The <value>
* is atomically bitwise XOR'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_xor
*     (
*     atomic_t *target,         /@ memory location to be modified @/
*     atomic_t  value           /@ value to XOR @/
*     )
*
* INTERNAL
* The 'cmpxchg' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_xor)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  (%edx),%eax	  /* get old value */

BRANCH_LABEL(atomic_xor_retry)

	movl  SP_ARG2(%esp),%ecx  /* get value to set */
	xorl  %eax, %ecx
	lock			  /* lock the bus during the next inst */
	cmpxchg  %ecx,(%edx)	  /* if (%eax == (%edx)) */
				  /*   {ZF = 1 ;  (%edx) = %ecx;} */
				  /* else                */
				  /*   {ZF = 0 ;  %eax = (%edx);} */
	jnz atomic_xor_retry
	ret


/*******************************************************************************
*
* atomic_and  - atomic bitwise AND primitive
*
* This routine provides the atomic bitwise AND operator. The <value> is
* atomically bitwise AND'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
* 
* atomic_val_t atomic_and
*     (
*     atomic_t *target,         /@ memory location to be modified @/
*     atomic_val_t value         /@ value to AND @/
*     )
*
* INTERNAL
* The 'cmpxchg' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_and)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  (%edx),%eax	  /* get old value */

BRANCH_LABEL(atomic_and_retry)

	movl  SP_ARG2(%esp),%ecx  /* get value to set */
	andl  %eax, %ecx
	lock			  /* lock the bus during the next inst */
	cmpxchg  %ecx,(%edx)	  /* if (%eax == (%edx)) */
				  /*   {ZF = 1 ;  (%edx) = %ecx;} */
				  /* else                */
				  /*   {ZF = 0 ;  %eax = (%edx);} */
	jnz atomic_and_retry
	ret


/*******************************************************************************
*
* atomic_nand  - atomic bitwise NAND primitive
*
* This routine provides the atomic bitwise NAND operator. The <value> is
* atomically bitwise NAND'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
*
* atomic_val_t atomic_nand
*     (
*     atomic_t * target,        /@ memory location to be modified @/
*     atomic_val_t  value        /@ value to NAND @/
*     )
*
* INTERNAL
* The 'cmpxchg' instruction is NOT supported on processor prior to the 80486
*/

SECTION_FUNC(TEXT, atomic_nand)

	movl  SP_ARG1(%esp),%edx  /* get address */
	movl  (%edx),%eax	  /* get old value */

BRANCH_LABEL(atomic_nand_retry)

	movl  SP_ARG2(%esp),%ecx  /* get value to nand with old value */

	andl  %eax,%ecx
	not   %ecx

	lock			  /* lock the bus during the next inst */
	cmpxchg  %ecx,(%edx)	  /* if (%eax == (%edx)) */
				  /*   {ZF = 1 ;  (%edx) = %ecx;} */
				  /* else                */
				  /*   {ZF = 0 ;  %eax = (%edx);} */
	jnz atomic_nand_retry
	ret

#endif /* !CONFIG_LOCK_INSTRUCTION_UNSUPPORTED */

