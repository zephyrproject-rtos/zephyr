/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CH32 reset controller devicetree common helper macros
 * @ingroup reset_controller_ch32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32_COMMON_H_

/**
 * @defgroup reset_controller_ch32 CH32 reset controller helpers
 * @ingroup reset_controller_interface
 *
 * @brief Macros for encoding CH32 peripheral reset cells.
 *
 * Devicetree macro for encoding peripheral reset on CH32 devices, for use with the
 * <tt>wch,ch32-rcc-rctl</tt> compatible reset controller.
 *
 * Each SoC family header (for example @c ch32v20x_30x-reset.h) defines the RCC bus reset register
 * offsets for that family as @c CH32_RESET_BUS_<bus> constants. A peripheral reset cell is the
 * encoded with @c CH32_RESET() by combining a bus constant with the bit position of the
 * peripheral's reset line within that bus reset register.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/reset/ch32v20x_30x-reset.h>
 *
 * &usart1 {
 * 	resets = <&rctl CH32_RESET(APB2, 14)>;
 * }
 * @endcode
 * @{
 */

/**
 * @brief Encode a peripheral reset cell value for the <tt>wch,ch32-rcc-rctl</tt> binding.
 *
 * Packs an RCC bus register offest and a bit position into one 32-bit reset cell value.
 *
 * Bits [4:0] hold the reset bit position within the 32-bit RCC bus register;
 * bits [16:5] hold the RCC register byte offset relative to the RCC base address.
 *
 * @param bus RCC bus name.
 * @param bit Bit position of the peripheral's reset line within the bus reset register.
 */
#define CH32_RESET(bus, bit) (((CH32_RESET_BUS_##bus) << 5U) | (bit))

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_CH32_COMMON_H_ */
