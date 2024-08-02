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
	const uint32_t mask2 = BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS) <<
		CONFIG_1ST_LEVEL_INTERRUPT_BITS;
	const uint32_t mask3 = BIT_MASK(CONFIG_3RD_LEVEL_INTERRUPT_BITS) <<
		(CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS);

	if (IS_ENABLED(CONFIG_3RD_LEVEL_INTERRUPTS) && (irq & mask3) != 0) {
		return 3;
	}

	if (IS_ENABLED(CONFIG_2ND_LEVEL_INTERRUPTS) && (irq & mask2) != 0) {
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
	if (IS_ENABLED(CONFIG_3RD_LEVEL_INTERRUPTS)) {
		return ((irq >> CONFIG_1ST_LEVEL_INTERRUPT_BITS) &
			BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS)) - 1;
	} else {
		return (irq >> CONFIG_1ST_LEVEL_INTERRUPT_BITS) - 1;
	}
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
	return irq & BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS);
}

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
	return (irq >> (CONFIG_1ST_LEVEL_INTERRUPT_BITS + CONFIG_2ND_LEVEL_INTERRUPT_BITS)) - 1;
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
	return (irq >> CONFIG_1ST_LEVEL_INTERRUPT_BITS) &
		BIT_MASK(CONFIG_2ND_LEVEL_INTERRUPT_BITS);
}

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
	} else if (level == 3) {
		return irq_from_level_3(irq);
	}

	/* level is higher than 3 */
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
	} else if (level == 3) {
		return irq_to_level_3(irq);
	}

	/* level is higher than 3 */
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
	} else if (level == 3) {
		return irq_parent_level_3(irq);
	}

	/* level is higher than 3 */
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

	__ASSERT_NO_MSG(level > 1 && level <= 3);

	return irq & BIT_MASK(CONFIG_1ST_LEVEL_INTERRUPT_BITS +
			      (level == 3 ? CONFIG_2ND_LEVEL_INTERRUPT_BITS : 0));
}

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS */
#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */
#endif /* ZEPHYR_INCLUDE_IRQ_MULTILEVEL_H_ */
