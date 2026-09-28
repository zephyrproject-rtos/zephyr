/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief CH32 Clock control devicetree common helper macros
 * @ingroup clock_control_ch32
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32_COMMON_H_

#include <zephyr/dt-bindings/dt-util.h>

/**
 * @defgroup clock_control_ch32 CH32 clock control helpers
 * @ingroup clock_control_interface
 *
 * @brief Macros for encoding CH32 peripheral clocks.
 *
 * Devicetree Macro for encoding peripheral clocks on CH32 devices, for use with the
 * <tt>wch,rcc</tt> compatible clock controller.
 *
 * Each SoC family header (for example @c ch32v20x_30x-clocks.h) defines the clock enable register
 * offsets for that family as @c CH32_CLOCK_PCENR_<bus> constants. A peripheral clock cell is then
 * encoded with @c CH32_CLOCK() by combining a bus constant with the bit position of the
 * peripheral's clock enable bit within that bus clock register.
 *
 * @note There are also special values for e.g. MCO as an clock ID.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/clock/ch32v20x_30x-clocks.h>
 *
 * &usart1 {
 *         clocks = <&rcc CH32_CLOCK(APB2, 14)>;
 * };
 * @endcode
 * @{
 */

/**
 * @brief Encode a peripheral clock cell value for the <tt>wch,rcc</tt> binding.
 *
 * Packs an RCC bus register offset and a bit position into one 32-bit reset clock cell value.
 *
 * Bits [4:0] hold the clock bit position within the 32-bit RCC bus register;
 * bits [16:5] hold the RCC register byte offset relative to the RCC base address;
 * bits [31:23] hold the special function IDs (e.g. MCO).
 *
 * @param bus RCC bus name.
 * @param bit Bit position of the peripheral's clock enable bit within the bus clock register.
 */
#define CH32_CLOCK(bus, bit) (((CH32_CLOCK_PCENR_##bus) << 5U) | (bit))

/**
 * @brief special MCO clock ID
 *
 * Clock cell id for the MCO clock function. This is used for the <tt>wch,rcc</tt> binding to select
 * the MCO function.
 *
 * See @ref CH32_CLOCK for bit field documentation.
 */
#define CH32_CLOCK_MCO BIT(24)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32_COMMON_H_ */
