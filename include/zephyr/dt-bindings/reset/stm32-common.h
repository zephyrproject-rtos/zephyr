/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32_RESET_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32_RESET_COMMON_H_

/**
 * Pack RCC register offset and bit in one 32-bit value.
 *
 * 5 LSBs are used to keep bit number in 32-bit RCC register.
 * Next 12 bits are used to keep RCC register offset.
 * Remaining bits are unused.
 *
 * @param bus STM32 bus name (expands to STM32_RESET_BUS_{bus})
 * @param bit Reset bit
 */
#define STM32_RESET(bus, bit) (((STM32_RESET_BUS_##bus) << 5U) | (bit))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32_RESET_COMMON_H_ */
