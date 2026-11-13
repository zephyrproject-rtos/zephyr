/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Reset specifiers for Puya PY32F0 devices.
 * @ingroup reset_controller_puya_py32f0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_PUYA_PY32F0_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_PUYA_PY32F0_RESET_H_

/**
 * @defgroup reset_controller_puya_py32f0 Puya PY32F0 reset specifiers
 * @ingroup reset_controller_interface
 *
 * Use the bus names IOP, AHB1, APB1 and APB2 with @ref PY32_RESET to encode
 * peripheral reset specifiers. Bus constants are byte offsets from the RCC base
 * address to the corresponding reset register.
 * @{
 */

/**
 * @brief Encode a PY32 peripheral reset specifier.
 * @param bus RCC bus name (IOP, AHB1, APB1 or APB2).
 * @param bit Peripheral bit number, from 0 through 31.
 * @return Reset identifier containing the register offset and bit number.
 */
#define PY32_RESET(bus, bit) ((PY32_RESET_BUS_##bus << 5U) | (bit))

/** RCC I/O port reset register offset, in bytes from the RCC base. */
#define PY32_RESET_BUS_IOP  0x24U
/** RCC AHB reset register offset, in bytes from the RCC base. */
#define PY32_RESET_BUS_AHB1 0x28U
/** RCC APB1 reset register offset, in bytes from the RCC base. */
#define PY32_RESET_BUS_APB1 0x2cU
/** RCC APB2 reset register offset, in bytes from the RCC base. */
#define PY32_RESET_BUS_APB2 0x30U

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_PUYA_PY32F0_RESET_H_ */
