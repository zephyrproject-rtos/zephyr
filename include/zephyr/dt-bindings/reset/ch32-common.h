/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32_COMMON_H
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32_COMMON_H

/**
 * Pack RCC register offset and bit in one 32-bit value.
 *
 * 5 LSBs are used to keep bit number in 32-bit RCC register.
 * Next 12 bits are used to keep RCC register offset.
 * Reamaining bits are unused.
 *
 * @param bus CH32 bus name (expands to CH32_RESET_BUT_{bus})
 * @param bit Reset bit
 */
#define CH32_RESET(bus, bit) (((CH32_RESET_BUS_##bus) << 5) | (bit))

/**
 * Unpack RCC register bus offset
 */
#define CH32_RESET_BUS(reset) (((reset) >> 5) & 0xFF)
/**
 * Unpack RCC register bit offset
 */
#define CH32_RESET_BIT(reset) ((reset) & 0x1F)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32_COMMON_H */
