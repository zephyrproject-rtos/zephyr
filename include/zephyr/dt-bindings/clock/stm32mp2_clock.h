/*
 * Copyright (C) 2025 Savoir-faire Linux, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_

#include "stm32_common_clocks.h"

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
/** @brief STM32_CLK: stm32 clk */
#define STM32_CLK		1U
/** @brief STM32_LP_CLK: stm32 lp clk */
#define STM32_LP_CLK		2U

/* GPIO Peripheral */
/** @brief STM32 clock periph gpioa */
#define STM32_CLOCK_PERIPH_GPIOA	0x52C
/** @brief STM32 clock periph gpiob */
#define STM32_CLOCK_PERIPH_GPIOB	0x530
/** @brief STM32 clock periph gpioc */
#define STM32_CLOCK_PERIPH_GPIOC	0x534
/** @brief STM32 clock periph gpiod */
#define STM32_CLOCK_PERIPH_GPIOD	0x538
/** @brief STM32 clock periph gpioe */
#define STM32_CLOCK_PERIPH_GPIOE	0x53C
/** @brief STM32 clock periph gpiof */
#define STM32_CLOCK_PERIPH_GPIOF	0x540
/** @brief STM32 clock periph gpiog */
#define STM32_CLOCK_PERIPH_GPIOG	0x544
/** @brief STM32 clock periph gpioh */
#define STM32_CLOCK_PERIPH_GPIOH	0x548
/** @brief STM32 clock periph gpioi */
#define STM32_CLOCK_PERIPH_GPIOI	0x54C
/** @brief STM32 clock periph gpioj */
#define STM32_CLOCK_PERIPH_GPIOJ	0x550
/** @brief STM32 clock periph gpiok */
#define STM32_CLOCK_PERIPH_GPIOK	0x554
/** @brief STM32 clock periph gpioz */
#define STM32_CLOCK_PERIPH_GPIOZ	0x558

/* SPI Peripheral */
/** @brief STM32 clock periph spi1 */
#define STM32_CLOCK_PERIPH_SPI1		0x758
/** @brief STM32 clock periph spi2 */
#define STM32_CLOCK_PERIPH_SPI2		0x75C
/** @brief STM32 clock periph spi3 */
#define STM32_CLOCK_PERIPH_SPI3		0x760
/** @brief STM32 clock periph spi4 */
#define STM32_CLOCK_PERIPH_SPI4		0x764
/** @brief STM32 clock periph spi5 */
#define STM32_CLOCK_PERIPH_SPI5		0x768
/** @brief STM32 clock periph spi6 */
#define STM32_CLOCK_PERIPH_SPI6		0x76C
/** @brief STM32 clock periph spi7 */
#define STM32_CLOCK_PERIPH_SPI7		0x770

/* USART/UART Peripheral */
/** @brief STM32 clock periph usart1 */
#define STM32_CLOCK_PERIPH_USART1	0x77C
/** @brief STM32 clock periph usart2 */
#define STM32_CLOCK_PERIPH_USART2	0x780
/** @brief STM32 clock periph usart3 */
#define STM32_CLOCK_PERIPH_USART3	0x784
/** @brief STM32 clock periph uart4 */
#define STM32_CLOCK_PERIPH_UART4	0x788
/** @brief STM32 clock periph uart5 */
#define STM32_CLOCK_PERIPH_UART5	0x78C
/** @brief STM32 clock periph usart6 */
#define STM32_CLOCK_PERIPH_USART6	0x790
/** @brief STM32 clock periph uart7 */
#define STM32_CLOCK_PERIPH_UART7	0x794
/** @brief STM32 clock periph uart8 */
#define STM32_CLOCK_PERIPH_UART8	0x798
/** @brief STM32 clock periph uart9 */
#define STM32_CLOCK_PERIPH_UART9	0x79C

/* I2C Peripheral */
/** @brief STM32 clock periph i2c1 */
#define STM32_CLOCK_PERIPH_I2C1		0x7A0
/** @brief STM32 clock periph i2c2 */
#define STM32_CLOCK_PERIPH_I2C2		0x7A8
/** @brief STM32 clock periph i2c3 */
#define STM32_CLOCK_PERIPH_I2C3		0x7AC
/** @brief STM32 clock periph i2c4 */
#define STM32_CLOCK_PERIPH_I2C4		0x7B0
/** @brief STM32 clock periph i2c5 */
#define STM32_CLOCK_PERIPH_I2C5		0x7B4
/** @brief STM32 clock periph i2c6 */
#define STM32_CLOCK_PERIPH_I2C6		0x7B8
/** @brief STM32 clock periph i2c7 */
#define STM32_CLOCK_PERIPH_I2C7		0x7BC
/** @brief STM32 clock periph i2c8 */
#define STM32_CLOCK_PERIPH_I2C8		0x7C0

/* FDCAN Peripheral */
/** @brief STM32 clock periph fdcan */
#define STM32_CLOCK_PERIPH_FDCAN	0x7E0

/* Watchdog Peripheral */
/** @brief STM32 clock periph iwdg4 */
#define STM32_CLOCK_PERIPH_IWDG4	0x894
/** @brief STM32 clock periph wwdg1 */
#define STM32_CLOCK_PERIPH_WWDG1	0x89C

/* CRC peripheral */
/** @brief STM32 clock periph crc */
#define STM32_CLOCK_PERIPH_CRC		0x8B4

/* I3C Peripheral */
/** @brief STM32 clock periph i3c1 */
#define STM32_CLOCK_PERIPH_I3C1		0x8C8
/** @brief STM32 clock periph i3c2 */
#define STM32_CLOCK_PERIPH_I3C2		0x8CC
/** @brief STM32 clock periph i3c3 */
#define STM32_CLOCK_PERIPH_I3C3		0x8D0
/** @brief STM32 clock periph i3c4 */
#define STM32_CLOCK_PERIPH_I3C4		0x8D4

/** @brief STM32 clock periph min */
#define STM32_CLOCK_PERIPH_MIN	STM32_CLOCK_PERIPH_GPIOA
/** @brief STM32 clock periph max */
#define STM32_CLOCK_PERIPH_MAX	STM32_CLOCK_PERIPH_I3C4

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32MP2_CLOCK_H_ */
