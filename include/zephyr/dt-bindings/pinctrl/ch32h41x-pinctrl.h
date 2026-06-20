/*
 * Copyright (c) 2026 Liu Changjie
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32H41X_PINCTRL_H__
#define __CH32H41X_PINCTRL_H__

/*
 * The CH32H41x uses an STM32-style per-pin alternate-function mux: each GPIO
 * pin has a 4-bit AF selector in the AFIO GPIOx_AFLR/AFHR registers, rather
 * than the CH32V20x/30x AFIO remap (PCFR) model.
 *
 * Encoding (packed into the pinmux cell):
 *   bits  0..2  : port (0=PA, 1=PB, ...)
 *   bits  3..6  : pin  (0..15)
 *   bits  7..10 : alternate function (0..15)
 */
#define CH32H41X_PINMUX_PORT_PA 0
#define CH32H41X_PINMUX_PORT_PB 1
#define CH32H41X_PINMUX_PORT_PC 2
#define CH32H41X_PINMUX_PORT_PD 3
#define CH32H41X_PINMUX_PORT_PE 4
#define CH32H41X_PINMUX_PORT_PF 5

#define CH32H41X_PINMUX_PORT_MASK 0x007
#define CH32H41X_PINMUX_PIN_MASK  0x078
#define CH32H41X_PINMUX_AF_MASK   0x780

#define CH32H41X_PINMUX(port, pin, af)                                                              \
	((CH32H41X_PINMUX_PORT_##port) | ((pin) << 3) | ((af) << 7))

/* USART1: AF7 */
#define USART1_TX_PA9_AF7  CH32H41X_PINMUX(PA, 9, 7)
#define USART1_RX_PA10_AF7 CH32H41X_PINMUX(PA, 10, 7)

/* USART8: AF11 */
#define USART8_TX_PB4_AF11 CH32H41X_PINMUX(PB, 4, 11)

#endif /* __CH32H41X_PINCTRL_H__ */
