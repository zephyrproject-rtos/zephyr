/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_

#include <zephyr/dt-bindings/clock/stm32_common_clocks.h>

/** @cond INTERNAL_HIDDEN */

/* Undefine the common clocks macro */
#undef STM32_CLOCK

/**
 * Pack RCC clock register offset and bit in two 32-bit values
 * as expected for the Device Tree `clocks` property on STM32.
 *
 * @param per STM32 Peripheral name (expands to STM32_CLOCK_PERIPH_{PER})
 * @param bit Clock bit
 */
#define STM32_CLOCK(per, bit) (STM32_CLOCK_PERIPH_##per) (1 << bit)

/* Clock reg */
#define STM32_CLK		1U
#define STM32_LP_CLK		2U

/*
 * Domain clock source IDs.
 *
 * These replace a STM32_CLOCK() specifier to query the rate of a clock source
 * instead of a peripheral gate. The clock tree is owned by the Cortex-A35, so
 * these entries are read-only: they can neither be gated nor re-routed from
 * the Cortex-M33.
 *
 * Values must stay below STM32_CLOCK_PERIPH_MIN and must not collide with the
 * IDs defined in stm32_common_clocks.h (STM32_SRC_SYSCLK/LSE/LSI).
 */

/* Bus clocks. STM32_SRC_SYSCLK is ck_icn_hs_mcu, the Cortex-M33 clock */
#define STM32_SRC_ICN_LS_MCU	0x010
#define STM32_SRC_PCLK1		0x011
#define STM32_SRC_PCLK2		0x012
#define STM32_SRC_PCLK3		0x013
#define STM32_SRC_PCLK4		0x014
#define STM32_SRC_PCLK5		0x015
#define STM32_SRC_PCLKDBG	0x016

/* Oscillators. LSE and LSI come from stm32_common_clocks.h */
#define STM32_SRC_HSI		0x017
#define STM32_SRC_HSE		0x018
#define STM32_SRC_MSI		0x019

/* PLL FOUTPOSTDIV outputs routed to the clock crossbar */
#define STM32_SRC_PLL4		0x01A
#define STM32_SRC_PLL5		0x01B
#define STM32_SRC_PLL6		0x01C
#define STM32_SRC_PLL7		0x01D
#define STM32_SRC_PLL8		0x01E

/*
 * Output of clock crossbar (flexgen) channel @p ch, in the 0..63 range. Every
 * peripheral kernel clock is fed by one of these channels, so declaring it is
 * enough for the clock controller to resolve the peripheral rate.
 */
#define STM32_SRC_FLEXGEN_MIN	0x100
#define STM32_SRC_FLEXGEN_MAX	0x13F
#define STM32_SRC_FLEXGEN(ch)	(STM32_SRC_FLEXGEN_MIN + (ch))

/* GPIO Peripheral */
#define STM32_CLOCK_PERIPH_GPIOA	0x52C
#define STM32_CLOCK_PERIPH_GPIOB	0x530
#define STM32_CLOCK_PERIPH_GPIOC	0x534
#define STM32_CLOCK_PERIPH_GPIOD	0x538
#define STM32_CLOCK_PERIPH_GPIOE	0x53C
#define STM32_CLOCK_PERIPH_GPIOF	0x540
#define STM32_CLOCK_PERIPH_GPIOG	0x544
#define STM32_CLOCK_PERIPH_GPIOH	0x548
#define STM32_CLOCK_PERIPH_GPIOI	0x54C
#define STM32_CLOCK_PERIPH_GPIOJ	0x550
#define STM32_CLOCK_PERIPH_GPIOK	0x554
#define STM32_CLOCK_PERIPH_GPIOZ	0x558

/* Timer Peripheral */
#define STM32_CLOCK_PERIPH_TIM12	0x728

/* SPI Peripheral */
#define STM32_CLOCK_PERIPH_SPI1		0x758
#define STM32_CLOCK_PERIPH_SPI2		0x75C
#define STM32_CLOCK_PERIPH_SPI3		0x760
#define STM32_CLOCK_PERIPH_SPI4		0x764
#define STM32_CLOCK_PERIPH_SPI5		0x768
#define STM32_CLOCK_PERIPH_SPI6		0x76C
#define STM32_CLOCK_PERIPH_SPI7		0x770
#define STM32_CLOCK_PERIPH_SPI8		0x774

/* USART/UART Peripheral */
#define STM32_CLOCK_PERIPH_USART1	0x77C
#define STM32_CLOCK_PERIPH_USART2	0x780
#define STM32_CLOCK_PERIPH_USART3	0x784
#define STM32_CLOCK_PERIPH_UART4	0x788
#define STM32_CLOCK_PERIPH_UART5	0x78C
#define STM32_CLOCK_PERIPH_USART6	0x790
#define STM32_CLOCK_PERIPH_UART7	0x794
#define STM32_CLOCK_PERIPH_UART8	0x798
#define STM32_CLOCK_PERIPH_UART9	0x79C

/* I2C Peripheral */
#define STM32_CLOCK_PERIPH_I2C1		0x7A0
#define STM32_CLOCK_PERIPH_I2C2		0x7A8
#define STM32_CLOCK_PERIPH_I2C3		0x7AC
#define STM32_CLOCK_PERIPH_I2C4		0x7B0
#define STM32_CLOCK_PERIPH_I2C5		0x7B4
#define STM32_CLOCK_PERIPH_I2C6		0x7B8
#define STM32_CLOCK_PERIPH_I2C7		0x7BC
#define STM32_CLOCK_PERIPH_I2C8		0x7C0

/* FDCAN Peripheral */
#define STM32_CLOCK_PERIPH_FDCAN	0x7E0

/* Watchdog Peripheral */
#define STM32_CLOCK_PERIPH_IWDG4	0x894
#define STM32_CLOCK_PERIPH_WWDG1	0x89C

/* Camera peripherals */
#define STM32_CLOCK_PERIPH_CSI2		0x858
#define STM32_CLOCK_PERIPH_DCMIPP	0x85C

/* CRC peripheral */
#define STM32_CLOCK_PERIPH_CRC		0x8B4

/* I3C Peripheral */
#define STM32_CLOCK_PERIPH_I3C1		0x8C8
#define STM32_CLOCK_PERIPH_I3C2		0x8CC
#define STM32_CLOCK_PERIPH_I3C3		0x8D0
#define STM32_CLOCK_PERIPH_I3C4		0x8D4

#define STM32_CLOCK_PERIPH_MIN	STM32_CLOCK_PERIPH_GPIOA
#define STM32_CLOCK_PERIPH_MAX	STM32_CLOCK_PERIPH_I3C4

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_ */
