/*
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief DT bindings for STM32G5 clock system
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G5_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G5_CLOCK_H_
/** @cond INTERNAL_HIDDEN */

#include "stm32_common_clocks.h"

/* RM0514 clock tree for STM32G5 series */

/* Fixed clocks */
#define STM32_SRC_HSE           (STM32_SRC_LSI + 1)
#define STM32_SRC_HSI           (STM32_SRC_HSE + 1)
#define STM32_SRC_HSI48         (STM32_SRC_HSI + 1)

/* Bus clocks */
#define STM32_SRC_PCLK1         (STM32_SRC_HSI48 + 1)
#define STM32_SRC_PCLK2         (STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK3         (STM32_SRC_PCLK2 + 1)
#define STM32_SRC_HCLK4         (STM32_SRC_PCLK3 + 1)
#define STM32_SRC_PLL1_P        (STM32_SRC_HCLK4 + 1)
#define STM32_SRC_PLL1_Q        (STM32_SRC_PLL1_P + 1)
#define STM32_SRC_PLL2_P        (STM32_SRC_PLL1_Q + 1)
#define STM32_SRC_PLL2_Q        (STM32_SRC_PLL2_P + 1)
#define STM32_SRC_ICLK1         (STM32_SRC_PLL2_Q + 1)
#define STM32_SRC_ICLK2         (STM32_SRC_ICLK1 + 1)

/* Bus registers */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB4    0x094
#define STM32_CLOCK_BUS_APB1    0x09C
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB3    0x0A8

#define STM32_PERIPH_BUS_MIN    STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX    STM32_CLOCK_BUS_APB3

/* RCC register offsets */
#define CCIPR1_REG              0xD8
#define CCIPR2_REG              0xDC
#define CCIPR3_REG              0xE0
#define CCIPR4_REG              0xE4
#define CCIPR5_REG              0xE8
#define RTCCR_REG               0xF0
#define CFGR1_REG               0x1C

/* CCIPR1 */
#define USART1_SEL(val)         STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR1_REG)
#define USART2_SEL(val)         STM32_DT_CLOCK_SELECT((val), 5, 3, CCIPR1_REG)
#define USART3_SEL(val)         STM32_DT_CLOCK_SELECT((val), 8, 6, CCIPR1_REG)
#define UART4_SEL(val)          STM32_DT_CLOCK_SELECT((val), 11, 9, CCIPR1_REG)
#define UART5_SEL(val)          STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR1_REG)
#define UART6_SEL(val)          STM32_DT_CLOCK_SELECT((val), 17, 15, CCIPR1_REG)
#define LPUART1_SEL(val)        STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR1_REG)

/* CCIPR2 */
#define DAC1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR2_REG)
#define DAC2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 6, 4, CCIPR2_REG)
#define DAC5_SEL(val)           STM32_DT_CLOCK_SELECT((val), 10, 8, CCIPR2_REG)
#define DAC6_SEL(val)           STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR2_REG)
#define DAC7_SEL(val)           STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR2_REG)
#define DAC8_SEL(val)           STM32_DT_CLOCK_SELECT((val), 22, 20, CCIPR2_REG)
#define DAC9_SEL(val)           STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR2_REG)
#define DAC10_SEL(val)          STM32_DT_CLOCK_SELECT((val), 30, 28, CCIPR2_REG)

/* CCIPR3 */
#define SPI1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR3_REG)
#define SPI2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 5, 3, CCIPR3_REG)
#define SPI3_SEL(val)           STM32_DT_CLOCK_SELECT((val), 8, 6, CCIPR3_REG)
#define PLAY1_SEL(val)          STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR3_REG)
#define PLAY1_DIV(val)          STM32_DT_CLOCK_SELECT((val), 23, 20, CCIPR3_REG)
#define LPTIM1_SEL(val)         STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR3_REG)
#define SYSTICK_SEL(val)        STM32_DT_CLOCK_SELECT((val), 29, 28, CCIPR3_REG)

/* CCIPR4 */
#define XSPI1_SEL(val)          STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR4_REG)
#define UCPD1_SEL(val)          STM32_DT_CLOCK_SELECT((val), 5, 4, CCIPR4_REG)
#define USB_SEL(val)            STM32_DT_CLOCK_SELECT((val), 9, 8, CCIPR4_REG)
#define I3C1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 14, 12, CCIPR4_REG)
#define I2C1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR4_REG)
#define I2C2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 22, 20, CCIPR4_REG)
#define I2C3_SEL(val)           STM32_DT_CLOCK_SELECT((val), 26, 24, CCIPR4_REG)
#define I2C4_SEL(val)           STM32_DT_CLOCK_SELECT((val), 30, 28, CCIPR4_REG)

/* CCIPR5 */
#define ADC_SEL(val)            STM32_DT_CLOCK_SELECT((val), 2, 0, CCIPR5_REG)
#define ADCDIV_SEL(val)         STM32_DT_CLOCK_SELECT((val), 7, 4, CCIPR5_REG)
#define FDCAN_SEL(val)          STM32_DT_CLOCK_SELECT((val), 13, 12, CCIPR5_REG)
#define POSENC1_SEL(val)        STM32_DT_CLOCK_SELECT((val), 18, 16, CCIPR5_REG)
#define POSENC2_SEL(val)        STM32_DT_CLOCK_SELECT((val), 22, 20, CCIPR5_REG)

/* RTCCR */
#define RTC_SEL(val)            STM32_DT_CLOCK_SELECT((val), 9, 8, RTCCR_REG)

/* CFGR1 */
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 21, 18, CFGR1_REG)
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 24, 22, CFGR1_REG)
#define MCO2_PRE(val)           STM32_DT_CLOCK_SELECT((val), 28, 25, CFGR1_REG)
#define MCO2_SEL(val)           STM32_DT_CLOCK_SELECT((val), 31, 29, CFGR1_REG)

#define MCO_PRE_DIV_1           1
#define MCO_PRE_DIV_2           2
#define MCO_PRE_DIV_3           3
#define MCO_PRE_DIV_4           4
#define MCO_PRE_DIV_5           5
#define MCO_PRE_DIV_6           6
#define MCO_PRE_DIV_7           7
#define MCO_PRE_DIV_8           8
#define MCO_PRE_DIV_9           9
#define MCO_PRE_DIV_10          10
#define MCO_PRE_DIV_11          11
#define MCO_PRE_DIV_12          12
#define MCO_PRE_DIV_13          13
#define MCO_PRE_DIV_14          14
#define MCO_PRE_DIV_15          15

#define MCO1_SEL_HSI            0
#define MCO1_SEL_LSE            1
#define MCO1_SEL_HSE            2
#define MCO1_SEL_PLL1_Q         3
#define MCO1_SEL_HSI48          4

#define MCO2_SEL_SYSCLK         0
#define MCO2_SEL_PLL2_P         1
#define MCO2_SEL_HSE            2
#define MCO2_SEL_PLL1_P         3
#define MCO2_SEL_LSI            4

/** @endcond */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32G5_CLOCK_H_ */
