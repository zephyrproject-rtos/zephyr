/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_

#include "stm32_common_clocks.h"

/** Peripheral clock sources */

/* RM0493, Figure 34, clock tree */
/* RM0515, Figure 36, clock tree */

/** System clock */
/* defined in stm32_common_clocks.h */
/** Fixed clocks  */
/* Low speed clocks defined in stm32_common_clocks.h */
#define STM32_SRC_HSE		(STM32_SRC_LSI + 1)
#define STM32_SRC_HSI16		(STM32_SRC_HSE + 1)
/** Bus clock */
#define STM32_SRC_HCLK1		(STM32_SRC_HSI16 + 1)
#define STM32_SRC_HCLK5		(STM32_SRC_HCLK1 + 1)
#define STM32_SRC_PCLK1		(STM32_SRC_HCLK5 + 1)
#define STM32_SRC_PCLK2		(STM32_SRC_PCLK1 + 1)
#define STM32_SRC_PCLK7		(STM32_SRC_PCLK2 + 1)
/** PLL outputs */
#define STM32_SRC_PLL1_P	(STM32_SRC_PCLK7 + 1)
#define STM32_SRC_PLL1_Q	(STM32_SRC_PLL1_P + 1)
#define STM32_SRC_PLL1_R	(STM32_SRC_PLL1_Q + 1)

#define STM32_SRC_CLOCK_MIN	STM32_SRC_PLL1_P
#define STM32_SRC_CLOCK_MAX	STM32_SRC_SYSCLK

/** Bus clocks (Register address offsets) */
#define STM32_CLOCK_BUS_AHB1    0x088
#define STM32_CLOCK_BUS_AHB2    0x08C
#define STM32_CLOCK_BUS_AHB4    0x094
#define STM32_CLOCK_BUS_AHB5    0x098
#define STM32_CLOCK_BUS_APB1    0x09C
#define STM32_CLOCK_BUS_APB1_2  0x0A0
#define STM32_CLOCK_BUS_APB2    0x0A4
#define STM32_CLOCK_BUS_APB7    0x0A8

#define STM32_PERIPH_BUS_MIN	STM32_CLOCK_BUS_AHB1
#define STM32_PERIPH_BUS_MAX	STM32_CLOCK_BUS_APB7

/** @brief RCC_CCIPRx register offset (RM0493.pdf) */
#define CCIPR1_REG		0xE0
#define CCIPR2_REG		0xE4
#define CCIPR3_REG		0xE8
/** @brief RCC_BCDR1 register offset (RM0493.pdf) */
#define BCDR1_REG		0xF0

/** @brief Device clk sources selection helpers */
/** CCIPR1 devices */
#define USART1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR1_REG)
#define USART2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 2, CCIPR1_REG)
#define USART3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 4, CCIPR1_REG)
#define I2C1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 10, CCIPR1_REG)
#define I2C2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR1_REG)
#define I2C4_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 14, CCIPR1_REG)
#define SPI2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 16, CCIPR1_REG)
#define LPTIM2_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 18, CCIPR1_REG)
#define SPI1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 20, CCIPR1_REG)
#define SYSTICK_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 22, CCIPR1_REG)
#define TIMIC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 1, 31, CCIPR1_REG)
/** CCIPR2 devices */
#define RNG_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 12, CCIPR2_REG)
/** CCIPR3 devices */
#define LPUART1_SEL(val)	STM32_DT_CLOCK_SELECT((val), 3, 0, CCIPR3_REG)
#define SPI3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 3, CCIPR3_REG)
#define I2C3_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 6, CCIPR3_REG)
#define LPTIM1_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 10, CCIPR3_REG)
#define ADC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 7, 12, CCIPR3_REG)
/** BCDR1 devices */
#define RTC_SEL(val)		STM32_DT_CLOCK_SELECT((val), 3, 8, BCDR1_REG)
/** @brief RCC_CFGRx register offset */
#define CFGR1_REG               0x1C
/** CFGR1 devices */
#define MCO1_SEL(val)           STM32_DT_CLOCK_SELECT((val), 0xF, 24, CFGR1_REG)
#define MCO1_PRE(val)           STM32_DT_CLOCK_SELECT((val), 0x7, 28, CFGR1_REG)

/* MCO prescaler : division factor */
#define MCO_PRE_DIV_1 0
#define MCO_PRE_DIV_2 1
#define MCO_PRE_DIV_4 2
#define MCO_PRE_DIV_8 3
#define MCO_PRE_DIV_16 4

/* MCO clock output */
#define MCO_SEL_SYSCLKPRE 1
#define MCO_SEL_HSI16 3
#define MCO_SEL_HSE32 4
#define MCO_SEL_PLL1RCLK 5
#define MCO_SEL_LSI 6
#define MCO_SEL_LSE 7
#define MCO_SEL_PLL1PCLK 8
#define MCO_SEL_PLL1QCLK 9
#define MCO_SEL_HCLK5 10


#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_STM32WBA_CLOCK_H_ */
