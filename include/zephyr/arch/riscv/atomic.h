/**
 * Copyright (c) 2024 NextSilicon
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_RISCV_ATOMIC_H_
#define ZEPHYR_INCLUDE_ARCH_RISCV_ATOMIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* The standard RISC-V atomic-instruction extension, "A", specifies
 * the number of instructions that atomically read-modify-write memory,
 * which RISC-V harts should support in order to synchronise harts
 * running in the same memory space. This is the subset of RISC-V
 * atomic-instructions not present in atomic_builtin.h file.
 */

#ifdef CONFIG_64BIT
static ALWAYS_INLINE atomic_val_t atomic_swap(const atomic_t *target, atomic_val_t newval)
{
	atomic_val_t ret;

	__asm__ volatile("amoswap.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(newval), "A"(*target)
			 : "memory");

	return ret;
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

static ALWAYS_INLINE atomic_val_t atomic_maxu(unsigned long *target, unsigned long value)
{
	unsigned long ret;

	__asm__ volatile("amomaxu.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_minu(unsigned long *target, unsigned long value)
{
	unsigned long ret;

	__asm__ volatile("amominu.d.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

#else

static ALWAYS_INLINE atomic_val_t atomic_swap(const atomic_t *target, atomic_val_t newval)
{
	atomic_val_t ret;

	__asm__ volatile("amoswap.w.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(newval), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_max(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amomax.w.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE atomic_val_t atomic_min(atomic_t *target, atomic_val_t value)
{
	atomic_val_t ret;

	__asm__ volatile("amomin.w.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE unsigned long atomic_maxu(unsigned long *target, unsigned long value)
{
	unsigned long ret;

	__asm__ volatile("amomaxu.w.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE unsigned long atomic_minu(unsigned long *target, unsigned long value)
{
	unsigned long ret;

	__asm__ volatile("amominu.w.aq  %0, %1, %2"
			 : "=r"(ret)
			 : "r"(value), "A"(*target)
			 : "memory");

	return ret;
}

#endif /* CONFIG_64BIT */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_RISCV_ATOMIC_H_ */
