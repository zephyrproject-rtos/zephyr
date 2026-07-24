/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GD32C2X1 clock definitions for device tree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32C2X1_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32C2X1_CLOCKS_H_

#include "gd32-clocks-common.h"

/**
 * @name Register offsets
 * @{
 */

#define GD32_AHB1EN_OFFSET 0x30U /**< AHB1 peripheral clock enable register offset */
#define GD32_AHB2EN_OFFSET 0x34U /**< AHB2 peripheral clock enable register offset */
#define GD32_APBEN_OFFSET  0x44U /**< APB peripheral clock enable register offset */

/** @} */

/**
 * @name Clock enable/disable definitions for peripherals
 * @{
 */

/* AHB1 peripherals */
#define GD32_CLOCK_FMC   GD32_CLOCK_CONFIG(AHB1EN, 4U)  /**< FMC clock */
#define GD32_CLOCK_CRC   GD32_CLOCK_CONFIG(AHB1EN, 12U) /**< CRC clock */
#define GD32_CLOCK_DMA   GD32_CLOCK_CONFIG(AHB1EN, 21U) /**< DMA clock */
#define GD32_CLOCK_GPIOF GD32_CLOCK_CONFIG(AHB1EN, 23U) /**< GPIOF clock */

/* AHB2 peripherals */
#define GD32_CLOCK_GPIOA GD32_CLOCK_CONFIG(AHB2EN, 17U) /**< GPIOA clock */
#define GD32_CLOCK_GPIOB GD32_CLOCK_CONFIG(AHB2EN, 18U) /**< GPIOB clock */
#define GD32_CLOCK_GPIOC GD32_CLOCK_CONFIG(AHB2EN, 19U) /**< GPIOC clock */
#define GD32_CLOCK_GPIOD GD32_CLOCK_CONFIG(AHB2EN, 20U) /**< GPIOD clock */

/* APB peripherals */
#define GD32_CLOCK_SYSCFG  GD32_CLOCK_CONFIG(APBEN, 0U)  /**< SYSCFG clock */
#define GD32_CLOCK_CMP     GD32_CLOCK_CONFIG(APBEN, 1U)  /**< Comparator clock */
#define GD32_CLOCK_WWDGT   GD32_CLOCK_CONFIG(APBEN, 8U)  /**< Window watchdog clock */
#define GD32_CLOCK_ADC     GD32_CLOCK_CONFIG(APBEN, 9U)  /**< ADC clock */
#define GD32_CLOCK_TIMER0  GD32_CLOCK_CONFIG(APBEN, 10U) /**< TIMER0 clock */
#define GD32_CLOCK_TIMER2  GD32_CLOCK_CONFIG(APBEN, 11U) /**< TIMER2 clock */
#define GD32_CLOCK_SPI0    GD32_CLOCK_CONFIG(APBEN, 12U) /**< SPI0 clock */
#define GD32_CLOCK_SPI1    GD32_CLOCK_CONFIG(APBEN, 13U) /**< SPI1 clock */
#define GD32_CLOCK_USART0  GD32_CLOCK_CONFIG(APBEN, 14U) /**< USART0 clock */
#define GD32_CLOCK_USART1  GD32_CLOCK_CONFIG(APBEN, 15U) /**< USART1 clock */
#define GD32_CLOCK_TIMER13 GD32_CLOCK_CONFIG(APBEN, 16U) /**< TIMER13 clock */
#define GD32_CLOCK_TIMER15 GD32_CLOCK_CONFIG(APBEN, 17U) /**< TIMER15 clock */
#define GD32_CLOCK_TIMER16 GD32_CLOCK_CONFIG(APBEN, 18U) /**< TIMER16 clock */
#define GD32_CLOCK_USART2  GD32_CLOCK_CONFIG(APBEN, 19U) /**< USART2 clock */
#define GD32_CLOCK_I2C0    GD32_CLOCK_CONFIG(APBEN, 21U) /**< I2C0 clock */
#define GD32_CLOCK_I2C1    GD32_CLOCK_CONFIG(APBEN, 22U) /**< I2C1 clock */
#define GD32_CLOCK_DBG     GD32_CLOCK_CONFIG(APBEN, 27U) /**< Debug module clock */
#define GD32_CLOCK_PMU     GD32_CLOCK_CONFIG(APBEN, 28U) /**< PMU clock */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32C2X1_CLOCKS_H_ */
