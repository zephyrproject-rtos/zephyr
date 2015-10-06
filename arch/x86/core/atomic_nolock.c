/* atomic_nolock.c - nanokernel atomic operators for IA-32 */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
DESCRIPTION
This module provides the atomic operators for IA-32 architectures on platforms
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

/**
 *
 * @brief Atomic compare-and-set primitive
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
 * @return Returns 1 if <newValue> is written, 0 otherwise.
 */

int atomic_cas(
	atomic_t *target,     /* address to be tested */
	atomic_val_t oldValue, /* value to compare against */
	atomic_val_t newValue  /* value to set to */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* temporary storage */

	key = irq_lock();
	ovalue = *target;
	if (ovalue != oldValue) {
		irq_unlock(key);
		return 0;
	}
	*target = newValue;
	irq_unlock(key);
	return 1;
}

/**
 *
 * @brief Atomic addition primitive
 *
 * This routine provides the atomic addition operator. The <value> is
 * atomically added to the value at <target>, placing the result at <target>,
 * and the old value from <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_add(
	atomic_t *target, /* memory location to add to */
	atomic_val_t value /* value to add */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue + value;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic subtraction primitive
 *
 * This routine provides the atomic subtraction operator. The <value> is
 * atomically subtracted from the value at <target>, placing the result at
 * <target>, and the old value from <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_sub(
	atomic_t *target, /* memory location to subtract from */
	atomic_val_t value /* value to subtract */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue - value;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic increment primitive
 *
 * This routine provides the atomic increment operator. The value at <target>
 * is atomically incremented by 1, and the old value from <target> is returned.
 *
 * @return The value from <target> before the increment
 */

atomic_val_t atomic_inc(
	atomic_t *target /* memory location to increment */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* value from <target> before the increment */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue + 1;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic decrement primitive
 *
 * This routine provides the atomic decrement operator. The value at <target>
 * is atomically decremented by 1, and the old value from <target> is returned.
 *
 * @return The value from <target> prior to the decrement
 */

atomic_val_t atomic_dec(
	atomic_t *target /* memory location to decrement */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* value from <target> prior to the decrement */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue - 1;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic get primitive
 *
 * This routine provides the atomic get primitive to atomically read
 * a value from <target>. It simply does an ordinary load.  Note that <target>
 * is expected to be aligned to a 4-byte boundary.
 *
 * @return The value read from <target>
 */

atomic_val_t atomic_get(const atomic_t *target /* memory location to read from */
			  )
{
	return *target;
}

/**
 *
 * @brief Atomic get-and-set primitive
 *
 * This routine provides the atomic set operator. The <value> is atomically
 * written at <target> and the previous value at <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_set(
	atomic_t *target, /* memory location to write to */
	atomic_val_t value /* value to write */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = value;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic clear primitive
 *
 * This routine provides the atomic clear operator. The value of 0 is atomically
 * written at <target> and the previous value at <target> is returned. (Hence,
 * atomic_clear(pAtomicVar) is equivalent to atomic_set(pAtomicVar, 0).)
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_clear(
	atomic_t *target /* memory location to write to */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = 0;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic bitwise inclusive OR primitive
 *
 * This routine provides the atomic bitwise inclusive OR operator. The <value>
 * is atomically bitwise OR'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_or(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to OR */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue | value;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic bitwise exclusive OR (XOR) primitive
 *
 * This routine provides the atomic bitwise exclusive OR operator. The <value>
 * is atomically bitwise XOR'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_xor(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to XOR */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue ^ value;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic bitwise AND primitive
 *
 * This routine provides the atomic bitwise AND operator. The <value> is
 * atomically bitwise AND'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_and(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to AND */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = ovalue & value;
	irq_unlock(key);
	return ovalue;
}

/**
 *
 * @brief Atomic bitwise NAND primitive
 *
 * This routine provides the atomic bitwise NAND operator. The <value> is
 * atomically bitwise NAND'ed with the value at <target>, placing the result
 * at <target>, and the previous value at <target> is returned.
 *
 * @return The previous value from <target>
 */

atomic_val_t atomic_nand(
	atomic_t *target, /* memory location to be modified */
	atomic_val_t value /* value to NAND */
	)
{
	int key;	    /* interrupt lock level */
	atomic_val_t ovalue; /* previous value from <target> */

	key = irq_lock();
	ovalue = *target;
	*target = ~(ovalue & value);
	irq_unlock(key);
	return ovalue;
}

#endif /* CONFIG_LOCK_INSTRUCTION_UNSUPPORTED */
