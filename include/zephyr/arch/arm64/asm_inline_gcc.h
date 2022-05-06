/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Either public functions or macros or invoked by public functions */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_ASM_INLINE_GCC_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_ASM_INLINE_GCC_H_

/*
 * The file must not be included directly
 * Include arch/cpu.h instead
 */

#ifndef _ASMLANGUAGE

#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned int key;

	/*
	 * Return the whole DAIF register as key but use DAIFSET to disable
	 * IRQs.
	 */
	key = read_daif();
	disable_irq();

	return key;
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	write_daif(key);
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	/* We only check the (I)RQ bit on the DAIF register */
	return (key & DAIF_IRQ_BIT) == 0;
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_ASM_INLINE_GCC_H_ */
