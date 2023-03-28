/*
 * Copyright (c) 2023 Silicom Connectivity Solutions, Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_XEC_RESET_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_XEC_RESET_COMMON_H_

/**
 * Pack RC register offset and bit in one 32-bit value.
 *
 * 5 LSBs are used to keep bit number in 32-bit RC register.
 * Next 12 bits are used to keep RC register offset.
 * Remaining bits are unused.
 *
 * @param reg XEC reg name (expands to XEC_PCR_PERIPH_RST{reg})
 * @param bit Reset bit
 */
#define XEC_RESET(reg, bit) (((XEC_PCR_PERIPH_RST##reg) << 5U) | (bit))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_XEC_RESET_COMMON_H_ */
