/**
 * Copyright (c) 2026 GlobalFoundries Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARC HS DMB instruction operand masks (_ARCVER >= 0x51)
 *
 * The dmb operand is a 3-bit mask of the data accesses to order.
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARC_DMB_H_
#define ZEPHYR_INCLUDE_ARCH_ARC_DMB_H_

/** Order prior loads before subsequent loads and stores. */
#define ARC_DMB_LOAD       1

/** Order prior stores before subsequent stores. */
#define ARC_DMB_STORE      2

/** Order prior loads and stores before subsequent loads and stores. */
#define ARC_DMB_LOAD_STORE 3

#endif /* ZEPHYR_INCLUDE_ARCH_ARC_DMB_H_ */
