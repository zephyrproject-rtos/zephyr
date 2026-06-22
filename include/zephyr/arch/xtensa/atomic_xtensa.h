/*
 * Copyright (c) 2021, 2026 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ATOMIC_XTENSA_H_
#define ZEPHYR_INCLUDE_ATOMIC_XTENSA_H_

/* Included from <zephyr/sys/atomic.h> */

#include <xtensa/config/core-isa.h>

/* Recent GCC versions actually do have working atomics support on
 * Xtensa with S32C1I and so should work with CONFIG_ATOMIC_OPERATIONS_BUILTIN.
 * Existing versions of Xtensa's XCC do not and GCC also do not support
 * L32EX/S32EX.  So we define an inline implementation here for atomic CAS.
 */

/** Implementation of @ref atomic_get. */
static ALWAYS_INLINE atomic_val_t atomic_get(const atomic_t *target)
{
	atomic_val_t ret;

	/* Actual Xtensa hardware seems to have only in-order
	 * pipelines, but the architecture does define a barrier load,
	 * so use it.  There is a matching s32ri instruction, but
	 * nothing in the Zephyr API requires a barrier store (all the
	 * atomic write ops have exchange semantics.
	 */
	__asm__ volatile("l32ai %0, %1, 0"
			 : "=r"(ret) : "r"(target) : "memory");
	return ret;
}

/**
 * @fn static atomic_val_t xtensa_cas(atomic_t *addr, atomic_val_t oldval, atomic_val_t newval)
 * @brief Xtensa specific atomic compare-and-set (CAS).
 *
 * @param addr Address of atomic variable.
 * @param oldval Original value to compare against.
 * @param newval New value to store.
 *
 * @return The value at the memory location before CAS.
 *
 * @see atomic_cas.
 */

#if XCHAL_HAVE_EXCLUSIVE || defined(__DOXYGEN__)

/*
 * This utilizes L32EX/S32EX instructions to perform compare-and-set atomic
 * operation. This will unconditionally read from the atomic variable at @p addr
 * before the comparison. This value is returned from the function.
 */
static ALWAYS_INLINE
atomic_val_t xtensa_cas(atomic_t *addr, atomic_val_t oldval,
			atomic_val_t newval)
{
	atomic_val_t mem_val;

	/* Read from address and mark it for exclusive access. */
	__asm__ volatile("l32ex %0, %1" : "=r"(mem_val) : "r"(addr));

	if (mem_val == oldval) {
		uint32_t result;

		__asm__ volatile("s32ex %1, %2; getex %0" : "=r"(result) : "r"(newval), "r"(addr));

		/* If GETEX returns store successful, we return the old value.
		 * Otherwise, we must return some other value to signal that
		 * the store failed to function caller.
		 */
		return (result == 1U) ? oldval : ~oldval;
	}

	/* Since *addr != oldval, we skip writing to memory and
	 * need to remove the exclusive lock before returning.
	 */
	__asm__("clrex");

	return mem_val;
}

#elif XCHAL_HAVE_S32C1I

/*
 * This utilizes SCOMPARE1 register and s32c1i instruction to
 * perform compare-and-set atomic operation. This will
 * unconditionally read from the atomic variable at @p addr
 * before the comparison. This value is returned from
 * the function.
 */
static ALWAYS_INLINE
atomic_val_t xtensa_cas(atomic_t *addr, atomic_val_t oldval,
			atomic_val_t newval)
{
	__asm__ volatile("wsr %1, SCOMPARE1; s32c1i %0, %2, 0"
			 : "+r"(newval), "+r"(oldval) : "r"(addr) : "memory");

	return newval; /* got swapped with the old memory by s32c1i */
}

#else
#error "No available hardware support for atomic operations"
#endif

/** Implementation of @ref atomic_cas. */
static ALWAYS_INLINE
bool atomic_cas(atomic_t *target, atomic_val_t oldval, atomic_val_t newval)
{
	return oldval == xtensa_cas(target, oldval, newval);
}

/** Implementation of @ref atomic_ptr_cas. */
static ALWAYS_INLINE
bool atomic_ptr_cas(atomic_ptr_t *target, void *oldval, void *newval)
{
	return (atomic_val_t) oldval
		== xtensa_cas((atomic_t *) target, (atomic_val_t) oldval,
			      (atomic_val_t) newval);
}

/* Generates an atomic exchange sequence that swaps the value at
 * address "target", whose old value is read to be "cur", with the
 * specified expression.  Evaluates to the old value which was
 * atomically replaced.
 */
#define Z__GEN_ATOMXCHG(expr) ({				\
	atomic_val_t res, cur;				\
	do {						\
		cur = *target;				\
		res = xtensa_cas(target, cur, (expr));	\
	} while (res != cur);				\
	res; })

/** Implementation of @ref atomic_set. */
static ALWAYS_INLINE
atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(value);
}

/** Implementation of @ref atomic_add. */
static ALWAYS_INLINE
atomic_val_t atomic_add(atomic_t *target, atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(cur + value);
}

/** Implementation of @ref atomic_sub. */
static ALWAYS_INLINE
atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(cur - value);
}

/** Implementation of @ref atomic_inc. */
static ALWAYS_INLINE
atomic_val_t atomic_inc(atomic_t *target)
{
	return Z__GEN_ATOMXCHG(cur + 1);
}

/** Implementation of @ref atomic_dec. */
static ALWAYS_INLINE
atomic_val_t atomic_dec(atomic_t *target)
{
	return Z__GEN_ATOMXCHG(cur - 1);
}

/** Implementation of @ref atomic_or. */
static ALWAYS_INLINE atomic_val_t atomic_or(atomic_t *target,
					    atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(cur | value);
}

/** Implementation of @ref atomic_xor. */
static ALWAYS_INLINE atomic_val_t atomic_xor(atomic_t *target,
					     atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(cur ^ value);
}

/** Implementation of @ref atomic_and. */
static ALWAYS_INLINE atomic_val_t atomic_and(atomic_t *target,
					     atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(cur & value);
}

/** Implementation of @ref atomic_nand. */
static ALWAYS_INLINE atomic_val_t atomic_nand(atomic_t *target,
					      atomic_val_t value)
{
	return Z__GEN_ATOMXCHG(~(cur & value));
}

/** Implementation of @ref atomic_ptr_get. */
static ALWAYS_INLINE void *atomic_ptr_get(const atomic_ptr_t *target)
{
	return (void *) atomic_get((atomic_t *)target);
}

/** Implementation of @ref atomic_ptr_set. */
static ALWAYS_INLINE void *atomic_ptr_set(atomic_ptr_t *target, void *value)
{
	return (void *) atomic_set((atomic_t *) target, (atomic_val_t) value);
}

/** Implementation of @ref atomic_clear. */
static ALWAYS_INLINE atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

/** Implementation of @ref atomic_ptr_clear. */
static ALWAYS_INLINE void *atomic_ptr_clear(atomic_ptr_t *target)
{
	return (void *) atomic_set((atomic_t *) target, 0);
}

#endif /* ZEPHYR_INCLUDE_ATOMIC_XTENSA_H_ */
