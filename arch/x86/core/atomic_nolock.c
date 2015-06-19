/* atomic_nolock.c - nanokernel atomic operators for IA-32 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
that do not support the LOCK prefix instruction.

The atomic operations are guaranteed to be atomic with respect to interrupt
service routines. However, they are NOT guaranteed to be atomic with respect
to operations performed by peer processors, unlike the versions of these
operators that do utilize the LOCK prefix instruction.

INTERNAL
These operators are currently unavailable to user space applications
as there is no requirement for this capability.
*/

#if defined(CONFIG_LOCK_INSTRUCTION_UNSUPPORTED)

#include <nanokernel.h>
#include <arch/cpu.h>

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
*/

int atomic_cas(
	atomic_t *target,     /* address to be tested */
	atomic_val_t oldValue, /* value to compare against */
	atomic_val_t newValue  /* value to set to */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* temporary storage */

	key = irq_lock_inline();
	ovalue = *target;
	if (ovalue != oldValue) {
		irq_unlock_inline(key);
		return 0;
	}
	*target = newValue;
	irq_unlock_inline(key);
	return 1;
}

/*******************************************************************************
*
* atomic_add - atomic addition primitive
*
* This routine provides the atomic addition operator. The <value> is
* atomically added to the value at <target>, placing the result at <target>,
* and the old value from <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_add(
	atomic_t *target, /* memory location to add to */
	atomic_val_t value /* value to add */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue + value;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_sub - atomic subtraction primitive
*
* This routine provides the atomic subtraction operator. The <value> is
* atomically subtracted from the value at <target>, placing the result at
* <target>, and the old value from <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_sub(
	atomic_t *target, /* memory location to subtract from */
	atomic_val_t value /* value to subtract */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue - value;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_inc - atomic increment primitive
*
* This routine provides the atomic increment operator. The value at <target>
* is atomically incremented by 1, and the old value from <target> is returned.
*
* RETURNS: The value from <target> before the increment
*/

atomic_val_t atomic_inc(
	atomic_t *target /* memory location to increment */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* value from <target> before the increment */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue + 1;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_dec - atomic decrement primitive
*
* This routine provides the atomic decrement operator. The value at <target>
* is atomically decremented by 1, and the old value from <target> is returned.
*
* RETURNS: The value from <target> prior to the decrement
*/

atomic_val_t atomic_dec(
	atomic_t *target /* memory location to decrement */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* value from <target> prior to the decrement */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue - 1;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_get - atomic get primitive
*
* This routine provides the atomic get primitive to atomically read
* a value from <target>. It simply does an ordinary load.  Note that <target>
* is expected to be aligned to a 4-byte boundary.
*
* RETURNS: The value read from <target>
*/

atomic_val_t atomic_get(const atomic_t *target /* memory location to read from */
			  )
{
	return *target;
}

/*******************************************************************************
*
* atomic_set - atomic get-and-set primitive
*
* This routine provides the atomic set operator. The <value> is atomically
* written at <target> and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_set(
	atomic_t *target, /* memory location to write to */
	atomic_val_t value /* value to write */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = value;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_clear - atomic clear primitive
*
* This routine provides the atomic clear operator. The value of 0 is atomically
* written at <target> and the previous value at <target> is returned. (Hence,
* atomic_clear(pAtomicVar) is equivalent to atomic_set(pAtomicVar, 0).)
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_clear(
	atomic_t *target /* memory location to write to */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = 0;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_or - atomic bitwise inclusive OR primitive
*
* This routine provides the atomic bitwise inclusive OR operator. The <value>
* is atomically bitwise OR'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_or(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to OR */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue | value;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_xor - atomic bitwise exclusive OR (XOR) primitive
*
* This routine provides the atomic bitwise exclusive OR operator. The <value>
* is atomically bitwise XOR'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_xor(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to XOR */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue ^ value;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_and  - atomic bitwise AND primitive
*
* This routine provides the atomic bitwise AND operator. The <value> is
* atomically bitwise AND'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_and(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to AND */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ovalue & value;
	irq_unlock_inline(key);
	return ovalue;
}

/*******************************************************************************
*
* atomic_nand  - atomic bitwise NAND primitive
*
* This routine provides the atomic bitwise NAND operator. The <value> is
* atomically bitwise NAND'ed with the value at <target>, placing the result
* at <target>, and the previous value at <target> is returned.
*
* RETURNS: The previous value from <target>
*/

atomic_val_t atomic_nand(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to NAND */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock_inline();
	ovalue = *target;
	*target = ~(ovalue & value);
	irq_unlock_inline(key);
	return ovalue;
}

#endif /* CONFIG_LOCK_INSTRUCTION_UNSUPPORTED */
