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
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) || defined(__DOXYGEN__)

#if defined(CONFIG_2ND_LEVEL_INTERRUPTS)
BUILD_ASSERT(CONFIG_2ND_LEVEL_INTERRUPT_BITS > 0,
	     "Second-level IRQ field must contain at least one bit");
#endif

#if defined(CONFIG_3RD_LEVEL_INTERRUPTS)
BUILD_ASSERT(CONFIG_3RD_LEVEL_INTERRUPT_BITS > 0,
	     "Third-level IRQ field must contain at least one bit");
BUILD_ASSERT(CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS +
			     CONFIG_3RD_LEVEL_INTERRUPT_BITS <=
		     32,
	     "Multilevel IRQ encoding exceeds 32 bits");
#elif defined(CONFIG_2ND_LEVEL_INTERRUPTS)
BUILD_ASSERT(CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS <= 32,
	     "Multilevel IRQ encoding exceeds 32 bits");
#endif

/* Masks for fields in the numeric multilevel IRQ encoding. */
#define Z_IRQ_L1_MASK ((uint32_t)GENMASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS - 1, 0))
#define Z_IRQ_L2_MASK                                                                              \
	((uint32_t)GENMASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS - 1,  \
			   CONFIG_1ST_LEVEL_INTERRUPT_BITS))
#if defined(CONFIG_3RD_LEVEL_INTERRUPTS) || defined(__DOXYGEN__)
#define Z_IRQ_L3_MASK                                                                              \
	((uint32_t)GENMASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS +     \
				   CONFIG_3RD_LEVEL_INTERRUPT_BITS - 1,                            \
			   CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS))
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

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
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	if ((irq & Z_IRQ_L3_MASK) != 0U) {
		return 3;
	}
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	if ((irq & Z_IRQ_L2_MASK) != 0U) {
		return 2;
	}

	return 1;
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
	return FIELD_GET(Z_IRQ_L2_MASK, irq) - 1U;
}

/**
 * @brief Preprocessor macro to convert `irq` from level 1 to level 2 format
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 2nd level IRQ number
 */
#define IRQ_TO_L2(irq) FIELD_PREP(Z_IRQ_L2_MASK, (irq) + 1U)

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
	return IRQ_TO_L2(irq);
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
	return FIELD_GET(Z_IRQ_L1_MASK, irq);
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
	return FIELD_GET(Z_IRQ_L3_MASK, irq) - 1U;
}

/**
 * @brief Preprocessor macro to convert `irq` from level 1 to level 3 format
 *
 * @param irq IRQ number in its zephyr format
 *
 * @return 3rd level IRQ number
 */
#define IRQ_TO_L3(irq) FIELD_PREP(Z_IRQ_L3_MASK, (irq) + 1U)

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
	return IRQ_TO_L3(irq);
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
	return irq_from_level_2(irq);
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

#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	__ASSERT_NO_MSG(level <= 3);
	if (level == 3) {
		return irq & (Z_IRQ_L1_MASK | Z_IRQ_L2_MASK);
	}
#else
	__ASSERT_NO_MSG(level <= 2);
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */

	if (level == 2) {
		return irq_parent_level_2(irq);
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
	unsigned int mask;
	const unsigned int level = irq_get_level(irq);

	if (false) {
		/* so that it evaluates the next condition */
#ifdef CONFIG_3RD_LEVEL_INTERRUPTS
	} else if (level == 3) {
		mask = Z_IRQ_L3_MASK;
#endif /* CONFIG_3RD_LEVEL_INTERRUPTS */
	} else if (level == 2) {
		mask = Z_IRQ_L2_MASK;
	} else {
		mask = Z_IRQ_L1_MASK;
	}

	return (irq & ~mask) | FIELD_PREP(mask, FIELD_GET(mask, irq) + val);
}

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_IRQ_MULTILEVEL_H_ */
