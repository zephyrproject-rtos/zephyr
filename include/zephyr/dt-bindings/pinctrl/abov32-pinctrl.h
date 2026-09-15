/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * ABOV Semiconductor SoC specific Devicetree pin control definitions
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ABOV32_PINCTRL_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ABOV32_PINCTRL_H_

/**
 * @name ABOV pinmux bitfield positions and masks
 * @{
 */

/** Pin number position (bits 0..3: 0..15) */
#define ABOV_PIN_POS     0U
/** Pin number mask */
#define ABOV_PIN_MSK     0xFU

/** Port ID position (bits 4..7: 0=PA, 1=PB, 2=PC, etc.) */
#define ABOV_PORT_POS    4U
/** Port ID mask */
#define ABOV_PORT_MSK    0xFU

/** Alternate function position (bits 8..11: 0=GPIO/ALT0, 1=ALT1, etc.) */
#define ABOV_ALT_POS     8U
/** Alternate function mask */
#define ABOV_ALT_MSK     0xFU

/** @} */

/**
 * @brief Utility macro to encode port, pin, and alternate function for Devicetree.
 *
 * @param port Port number (0 = Port A, 1 = Port B, 2 = Port C, etc.).
 * @param pin Pin number (0..15).
 * @param alt Alternate function number (0..10).
 */
#define ABOV_PINMUX(port, pin, alt) \
	(((port) << ABOV_PORT_POS) | ((pin) << ABOV_PIN_POS) | ((alt) << ABOV_ALT_POS))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_ABOV32_PINCTRL_H_ */
