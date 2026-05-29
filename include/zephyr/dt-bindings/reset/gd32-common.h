/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32_RESET_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32_RESET_COMMON_H_

/**
 * Encode RCU register offset and configuration bit.
 *
 * - 0..5: bit number
 * - 6..14: offset
 * - 15: reserved
 *
 * @param reg RCU register name (expands to GD32_{reg}_OFFSET)
 * @param bit Configuration bit
 */
#define GD32_RESET_CONFIG(reg, bit) \
	(((GD32_ ## reg ## _OFFSET) << 6U) | (bit))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32_RESET_COMMON_H_ */
