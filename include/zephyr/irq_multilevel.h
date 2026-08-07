/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public interface for multi-level interrupts
 */
#ifndef ZEPHYR_INCLUDE_IRQ_MULTILEVEL_H_
#define ZEPHYR_INCLUDE_IRQ_MULTILEVEL_H_

#ifndef _ASMLANGUAGE
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) || defined(__DOXYGEN__)

typedef union _z_irq {
	/* Zephyr multilevel-encoded IRQ */
	uint32_t irq;

	/* Interrupt bits */
	struct {
		/* First level interrupt bits */
		uint32_t l1: CONFIG_1ST_LEVEL_INTERRUPT_BITS;
		/* Second level interrupt bits */
		uint32_t l2: CONFIG_2ND_LEVEL_INTERRUPT_BITS;
#if defined(CONFIG_3RD_LEVEL_INTERRUPTS) || defined(__DOXYGEN__)
		/* Third level interrupt bits */
		uint32_t l3: CONFIG_3RD_LEVEL_INTERRUPT_BITS;
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
	} bits;

#if defined(CONFIG_3RD_LEVEL_INTERRUPTS) || defined(__DOXYGEN__)
	/* Third level IRQ's interrupt controller */
	struct {
		/* IRQ of the third level interrupt aggregator */
		uint32_t irq: CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS;
	} l3_intc;
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	/* Second level IRQ's interrupt controller */
	struct {
		/* IRQ of the second level interrupt aggregator */
		uint32_t irq: CONFIG_1ST_LEVEL_INTERRUPT_BITS;
	} l2_intc;
} _z_irq_t;

BUILD_ASSERT(sizeof(_z_irq_t) == sizeof(uint32_t), "Size of `_z_irq_t` must equal to `uint32_t`");

static inline uint32_t _z_l1_irq(_z_irq_t irq)
{
	return irq.bits.l1;
}

static inline uint32_t _z_l2_irq(_z_irq_t irq)
{
	return irq.bits.l2 - 1;
}

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
static inline uint32_t _z_l3_irq(_z_irq_t irq)
{
	return irq.bits.l3 - 1;
}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

static inline unsigned int _z_irq_get_level(_z_irq_t z_irq)
{
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	if (z_irq.bits.l3 != 0) {
		return 3;
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	if (z_irq.bits.l2 != 0) {
		return 2;
	}

	return 1;
}

/**
 * @brief Return IRQ level
 * This routine returns the interrupt level number of the provided interrupt.
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 1 if IRQ level 1, 2 if IRQ level 2, 3 if IRQ level 3
 */
static inline unsigned int irq_get_level(unsigned int irq)
{
	_z_irq_t z_irq = {
		.irq = irq,
	};

	return _z_irq_get_level(z_irq);
}

/**
 * @brief Return the 2nd level interrupt number
 *
 * This routine returns the second level irq number of the zephyr irq
 * number passed in
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ number
 */
static inline unsigned int irq_from_level_2(unsigned int irq)
{
	_z_irq_t z_irq = {
		.irq = irq,
	};

	return _z_l2_irq(z_irq);
}

/**
 * @brief Preprocessor macro to convert `irq` from level 1 to level 2 format
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ number
 */
#define IRQ_TO_L2(irq) ((irq + 1) << CONFIG_1ST_LEVEL_INTERRUPT_BITS)

/**
 * @brief Converts irq from level 1 to level 2 format
 *
 *
 * This routine converts the input into the level 2 irq number format
 *
 * @note Values >= 0xFF are invalid
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ number
 */
static inline unsigned int irq_to_level_2(unsigned int irq)
{
	_z_irq_t z_irq = {
		.bits = {
			.l1 = 0,
			.l2 = irq + 1,
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
			.l3 = 0,
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
		},
	};

	return z_irq.irq;
}

/**
 * @brief Returns the parent IRQ of the level 2 raw IRQ number
 *
 *
 * The parent of a 2nd level interrupt is in the 1st byte
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ parent
 */
static inline unsigned int irq_parent_level_2(unsigned int irq)
{
	_z_irq_t z_irq = {
		.irq = irq,
	};

	return _z_l1_irq(z_irq);
}

#if defined(CONFIG_3RD_LEVEL_INTERRUPTS) || defined(__DOXYGEN__)
/**
 * @brief Return the 3rd level interrupt number
 *
 *
 * This routine returns the third level irq number of the zephyr irq
 * number passed in
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ number
 */
static inline unsigned int irq_from_level_3(unsigned int irq)
{
	_z_irq_t z_irq = {
		.irq = irq,
	};

	return _z_l3_irq(z_irq);
}

/**
 * @brief Preprocessor macro to convert `irq` from level 1 to level 3 format
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ number
 */
#define IRQ_TO_L3(irq)                                                                             \
	((irq + 1) << (CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS))

/**
 * @brief Converts irq from level 1 to level 3 format
 *
 *
 * This routine converts the input into the level 3 irq number format
 *
 * @note Values >= 0xFF are invalid
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ number
 */
static inline unsigned int irq_to_level_3(unsigned int irq)
{
	_z_irq_t z_irq = {
		.bits = {
			.l1 = 0,
			.l2 = 0,
			.l3 = irq + 1,
		},
	};

	return z_irq.irq;
}

/**
 * @brief Returns the parent IRQ of the level 3 raw IRQ number
 *
 *
 * The parent of a 3rd level interrupt is in the 2nd byte
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ parent
 */
static inline unsigned int irq_parent_level_3(unsigned int irq)
{
	_z_irq_t z_irq = {
		.irq = irq,
	};

	return _z_l2_irq(z_irq);
}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

/**
 * @brief Return the interrupt number for a given level
 *
 * @param irq IRQ number in its zephyr format
 * @param level IRQ level
 *
 * @return IRQ number in the level
 */
static inline unsigned int irq_from_level(unsigned int irq, unsigned int level)
{
	if (level == 1) {
		return irq;
	} else if (level == 2) {
		return irq_from_level_2(irq);
	}
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	else if (level == 3) {
		return irq_from_level_3(irq);
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	/* level is higher than what's supported */
	__ASSERT_NO_MSG(false);
	return irq;
}

/**
 * @brief Converts irq from level 1 to a given level
 *
 * @param irq IRQ number in its zephyr format
 * @param level IRQ level
 *
 * @return Converted IRQ number in the level
 */
static inline unsigned int irq_to_level(unsigned int irq, unsigned int level)
{
	if (level == 1) {
		return irq;
	} else if (level == 2) {
		return irq_to_level_2(irq);
	}
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	else if (level == 3) {
		return irq_to_level_3(irq);
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	/* level is higher than what's supported */
	__ASSERT_NO_MSG(false);
	return irq;
}

/**
 * @brief Returns the parent IRQ of the given level raw IRQ number
 *
 * @param irq IRQ number in its zephyr format
 * @param level IRQ level
 *
 * @return IRQ parent of the given level
 */
static inline unsigned int irq_parent_level(unsigned int irq, unsigned int level)
{
	if (level == 1) {
		/* doesn't really make sense, but return anyway */
		return irq;
	} else if (level == 2) {
		return irq_parent_level_2(irq);
	}
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	else if (level == 3) {
		return irq_parent_level_3(irq);
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	/* level is higher than what's supported */
	__ASSERT_NO_MSG(false);
	return irq;
}

/**
 * @brief Returns the parent interrupt controller IRQ of the given IRQ number
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return IRQ of the interrupt controller
 */
static inline unsigned int irq_get_intc_irq(unsigned int irq)
{
	const unsigned int level = irq_get_level(irq);
	_z_irq_t z_irq = {
		.irq = irq,
	};

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	__ASSERT_NO_MSG(level <= 3);
	if (level == 3) {
		return z_irq.l3_intc.irq;
	}
#else
	__ASSERT_NO_MSG(level <= 2);
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	if (level == 2) {
		return z_irq.l2_intc.irq;
	}

	return irq;
}

/**
 * @brief Increments the multilevel-encoded @a irq by @a val
 *
 * @param irq IRQ number in its zephyr format
 * @param val Amount to increment
 *
 * @return @a irq incremented by @a val
 */
static inline unsigned int irq_increment(unsigned int irq, unsigned int val)
{
	_z_irq_t z_irq = {
		.irq = irq,
	};

	if (false) {
		/* so that it evaluates the next condition */
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	} else if (z_irq.bits.l3 != 0) {
		z_irq.bits.l3 += val;
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
	} else if (z_irq.bits.l2 != 0) {
		z_irq.bits.l2 += val;
	} else {
		z_irq.bits.l1 += val;
	}

	return z_irq.irq;
}

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_IRQ_MULTILEVEL_H_ */
