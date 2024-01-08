/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_NSI_INTERNAL_H
#define NSI_COMMON_SRC_NSI_INTERNAL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit. Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static inline unsigned int nsi_find_lsb_set(uint32_t op)
{
	return __builtin_ffs(op);
}

/**
 * @brief find least significant bit set in a 64-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit. Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static inline unsigned int nsi_find_lsb_set64(uint64_t op)
{
	return __builtin_ffsll(op);
}

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_NSI_INTERNAL_H */
