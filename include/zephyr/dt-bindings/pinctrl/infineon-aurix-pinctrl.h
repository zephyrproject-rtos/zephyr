/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon AURIX pin controller devicetree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_INFINEON_AURIX_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_INFINEON_AURIX_PINCTRL_H_

/**
 * @brief AURIX pinmux cell
 *
 * @param port Port number, for example 14 for P14.
 * @param pin Pin number within the port, 0 to 15.
 * @param func Alternate output function (0 selects GPIO) for a pin with
 *        output-enable; for an input, the input select of the peripheral
 *        (0 = A ... 7 = H).
 */
#define AURIX_PINMUX(port, pin, func) (((port) << 4) | (pin) | ((func) << 12))

/** Pin number of a pinmux cell */
#define AURIX_PINMUX_PIN(pinmux)  ((pinmux) & 0xF)
/** Port number of a pinmux cell */
#define AURIX_PINMUX_PORT(pinmux) (((pinmux) >> 4) & 0xFF)
/** Function or input select of a pinmux cell */
#define AURIX_PINMUX_FUNC(pinmux) (((pinmux) >> 12) & 0xF)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_INFINEON_AURIX_PINCTRL_H_ */
