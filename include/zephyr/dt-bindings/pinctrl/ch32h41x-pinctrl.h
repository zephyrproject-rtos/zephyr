/*
 * Copyright (c) 2026 Liu Changjie
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32H41X_PINCTRL_H__
#define __CH32H41X_PINCTRL_H__

/**
 * @file
 * @brief Devicetree pin control helpers for WCH CH32H41x
 * @ingroup pinctrl_ch32h41x
 */

/**
 * @defgroup pinctrl_ch32h41x WCH CH32H41x pin control helpers
 * @brief Macros for pin control configuration of WCH CH32H41x
 * @ingroup devicetree-pinctrl
 *
 * The CH32H41x uses an STM32-style per-pin alternate-function mux: each GPIO
 * pin has a 4-bit AF selector in the AFIO GPIOx_AFLR/AFHR registers, rather
 * than the CH32V20x/30x AFIO remap (PCFR) model.
 *
 * Encoding (packed into the pinmux cell):
 *   - bits  0..2  : port (0=PA, 1=PB, ...)
 *   - bits  3..6  : pin  (0..15)
 *   - bits  7..10 : alternate function (0..15)
 *
 * @{
 */

/** Port PA selector */
#define CH32H41X_PINMUX_PORT_PA 0
/** Port PB selector */
#define CH32H41X_PINMUX_PORT_PB 1
/** Port PC selector */
#define CH32H41X_PINMUX_PORT_PC 2
/** Port PD selector */
#define CH32H41X_PINMUX_PORT_PD 3
/** Port PE selector */
#define CH32H41X_PINMUX_PORT_PE 4
/** Port PF selector */
#define CH32H41X_PINMUX_PORT_PF 5

/** Mask for the port field of the pinmux cell */
#define CH32H41X_PINMUX_PORT_MASK 0x007
/** Mask for the pin field of the pinmux cell */
#define CH32H41X_PINMUX_PIN_MASK  0x078
/** Mask for the alternate-function field of the pinmux cell */
#define CH32H41X_PINMUX_AF_MASK   0x780

/**
 * @brief Encode a port, pin and alternate function into a pinmux cell
 *
 * @param port Port name (PA, PB, ...)
 * @param pin  Pin number (0..15)
 * @param af   Alternate function (0..15)
 */
#define CH32H41X_PINMUX(port, pin, af) ((CH32H41X_PINMUX_PORT_##port) | ((pin) << 3) | ((af) << 7))

/* USART1: AF7 */
/** USART1 TX on PA9 (AF7) */
#define USART1_TX_PA9_AF7  CH32H41X_PINMUX(PA, 9, 7)
/** USART1 RX on PA10 (AF7) */
#define USART1_RX_PA10_AF7 CH32H41X_PINMUX(PA, 10, 7)

/* USART8: AF11 */
/** USART8 TX on PB4 (AF11) */
#define USART8_TX_PB4_AF11 CH32H41X_PINMUX(PB, 4, 11)

/** @} */

#endif /* __CH32H41X_PINCTRL_H__ */
