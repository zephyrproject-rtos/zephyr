/*
 * Copyright (c) 2015, Wind River Systems, Inc.
 * Copyright (c) 2017, Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_FFS_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_FFS_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_msb_set(uint32_t op)
{
	if (op == 0) {
		return 0;
	}

	return 32 - __builtin_clz(op);
}


/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_lsb_set(uint32_t op)
{
#ifdef CONFIG_TOOLCHAIN_HAS_BUILTIN_FFS
	return __builtin_ffs(op);

#else
	/*
	 * Toolchain does not have __builtin_ffs().
	 * Need to do this manually.
	 */
	int bit;

	if (op == 0) {
		return 0;
	}

	for (bit = 0; bit < 32; bit++) {
		if ((op & (1 << bit)) != 0) {
			return (bit + 1);
		}
	}

	/*
	 * This should never happen but we need to keep
	 * compiler happy.
	 */
	return 0;
#endif /* CONFIG_TOOLCHAIN_HAS_BUILTIN_FFS */
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_FFS_H_ */
