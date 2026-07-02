/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GD32 reset controller devicetree helper macros
 * @ingroup reset_controller_gigadevice
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32_RESET_COMMON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32_RESET_COMMON_H_

/**
 * @defgroup reset_controller_gigadevice GigaDevice reset controller helpers
 * @ingroup reset_controller_interface
 *
 * @brief Macros for encoding GD32 peripheral reset cells.
 *
 * Devicetree macros for encoding peripheral reset cells on GigaDevice GD32 devices, for use with
 * the <tt>gd,gd32-rctl</tt> compatible reset controller.
 *
 * Each SoC family header (for example @c gd32f4xx.h) defines RCU reset register offsets for that
 * family and peripheral reset identifiers as @c GD32_RESET_\<peripheral\> constants. A peripheral
 * reset cell is encoded by combining an RCU register offset with the bit position of the
 * peripheral's reset line within that register.
 *
 * @code{.dts}
 * #include <zephyr/dt-bindings/reset/gd32f4xx.h>
 *
 * &usart0 {
 *         resets = <&rctl GD32_RESET_USART0>;
 * };
 * @endcode
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Encode a peripheral reset cell value for the <tt>gd,gd32-rctl</tt> binding.
 *
 * Packs an RCU register offset and a bit position into one 32-bit reset cell value.
 *
 * Bits [5:0]: bit position
 * Bits [14:6]: RCU register offset
 * Bit 15: reserved
 *
 * @param reg RCU register name (expands to GD32_{reg}_OFFSET)
 * @param bit Bit position of the peripheral's reset line within that register
 */
#define GD32_RESET_CONFIG(reg, bit) \
	(((GD32_ ## reg ## _OFFSET) << 6U) | (bit))

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32_RESET_COMMON_H_ */
