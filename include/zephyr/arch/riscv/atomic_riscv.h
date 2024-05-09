/**
 * Copyright (c) 2024 NextSilicon
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ATOMIC_RISCV_H_
#define ZEPHYR_INCLUDE_ATOMIC_RISCV_H_

/* The standard atomic-instruction extension, "A", specifies the
 * number of instructions that atomically read-modify-write memory,
 * which RISC-V harts should support in order to synchronise harts
 * running in the same memory space.
 */

static ALWAYS_INLINE atomic_val_t atomic_add(const atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amoadd.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_swap(const atomic_t *target, atomic_val_t newval)
{
	atomic_val_t ret;

	__asm__ volatile("amoswap.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(newval), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_inc(const atomic_t *target)
{
	return atomic_add(target, 1);
}

static ALWAYS_INLINE atomic_val_t atomic_dec(const atomic_t *target)
{
	return atomic_add(target, -1);
}

static ALWAYS_INLINE atomic_val_t atomic_get(const atomic_t *target)
{
	return atomic_add(target, 0);
}

static ALWAYS_INLINE atomic_val_t atomic_sub(const atomic_t *target, atomic_val_t value)
{
	return atomic_add(target, -value);
}

static ALWAYS_INLINE atomic_val_t atomic_and(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amoand.d.aq %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_or(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amoor.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amoxor.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_set(atomic_t *target, atomic_val_t value)
{
	return atomic_swap(target, value);
}

static ALWAYS_INLINE atomic_val_t atomic_cas(atomic_t *target, atomic_val_t oldval,
					     atomic_val_t newval)
{
	atomic_val_t ret;

	__asm__ volatile("lr.d.aq %0, %3\n"
			"bne %0, %1, 1f\n"
			"sc.d.aqrl %0, %2, %3\n"
			"1:\n"
			: "=&r"(ret)
			: "r"(oldval), "r"(newval), "A"(*target)
			: "memory");

	return ret == 0;
}

static ALWAYS_INLINE atomic_val_t atomic_max(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amomax.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_min(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amomin.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE bool atomic_ptr_cas(atomic_ptr_t *target, void *oldval, void *newval)
{
	atomic_val_t ret;

	ret = atomic_cas((atomic_t *)target, (atomic_val_t)oldval, (atomic_val_t)newval);

	return ret == (atomic_val_t)oldval;
}

static ALWAYS_INLINE atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret = *target;

	atomic_set(target, ~(ret & value));

	return ret;
}

static ALWAYS_INLINE void *atomic_ptr_get(const atomic_ptr_t *target)
{
	return (void *)atomic_get((atomic_t *)target);
}

static ALWAYS_INLINE void *atomic_ptr_set(atomic_ptr_t *target, void *value)
{
	return (void *)atomic_set((atomic_t *)target, (atomic_val_t)value);
}

static ALWAYS_INLINE atomic_val_t atomic_clear(atomic_t *target)
{
	return atomic_set(target, 0);
}

static ALWAYS_INLINE void *atomic_ptr_clear(atomic_ptr_t *target)
{
	return (void *)atomic_set((atomic_t *)target, 0);
}

#endif /* ZEPHYR_INCLUDE_ATOMIC_RISCV_H_ */
