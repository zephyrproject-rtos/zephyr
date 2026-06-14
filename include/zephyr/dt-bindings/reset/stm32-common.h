/*
 * Copyright (c) 2022 Google Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STM32 reset controller devicetree common helper macros
 * @ingroup reset_controller_stm32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32_RESET_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32_RESET_COMMON_H_

/**
 * @defgroup reset_controller_stm32 STM32 reset controller helpers
 * @ingroup reset_controller_interface
 *
 * @brief Macros for encoding STM32 peripheral reset cells.
 *
 * Devicetree macro for encoding peripheral reset cells on STM32 devices, for use with the
 * <tt>st,stm32-rcc-rctl</tt> compatible reset controller.
 *
 * Each SoC family header (for example @c stm32h5_reset.h) defines the RCC bus reset register
 * offsets for that family as @c STM32_RESET_BUS_<bus> constants. A peripheral reset cell is then
 * encoded with @c STM32_RESET() by combining a bus constant with the bit position of the
 * peripheral's reset line within that bus reset register.
 *
 * @note STM32MP1 and STM32MP13 redefine @c STM32_RESET() with separate SET/CLEAR offsets. STM32MP2
 * uses per-peripheral registers.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/reset/stm32h5_reset.h>
 *
 * &usart1 {
 *         resets = <&rctl STM32_RESET(APB2, 14)>;
 * };
 * @endcode
 * @{
 */

/**
 * @brief Encode a peripheral reset cell value for the <tt>st,stm32-rcc-rctl</tt> binding.
 *
 * Packs an RCC bus register offset and a bit position into one 32-bit reset cell value.
 *
 * Bits [4:0] hold the reset bit position within the 32-bit RCC bus register;
 * bits [16:5] hold the RCC register byte offset relative to the RCC base address.
 *
 * @param bus RCC bus name.
 * @param bit Bit position of the peripheral's reset line within the bus reset register
 */
#define STM32_RESET(bus, bit) (((STM32_RESET_BUS_##bus) << 5U) | (bit))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_STM32_RESET_COMMON_H_ */
