/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_SF32LB_CLOCKS_COMMON_H_
#define _INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_SF32LB_CLOCKS_COMMON_H_

/** @cond INTERNAL_HIDDEN */

#define SF32LB_CLOCK_OFFSET_POS  0U
#define SF32LB_CLOCK_OFFSET_MSK  0xFFU
#define SF32LB_CLOCK_BIT_POS     8U
#define SF32LB_CLOCK_BIT_MSK     0x1F00U

/** @endcond */

/**
 * @brief Encode RCC register offset and configuration bit.
 *
 * Bitmap:
 * - 0..7:  offset
 * - 8..12: bit number
 * - 13-15: reserved
 *
 * @param offset RCC register offset to ENRx register
 * @param bit Configuration bit
 */
#define SF32LB_CLOCK_CONFIG(offset, bit) \
	((((offset) & SF32LB_CLOCK_OFFSET_MSK) << SF32LB_CLOCK_OFFSET_POS) | \
	 (((bit) & SF32LB_CLOCK_BIT_MSK) << SF32LB_CLOCK_BIT_POS))

#endif /* _INCLUDE_ZEPHYR_DT_BINDINGS_CLOCK_SF32LB_CLOCKS_COMMON_H_ */
