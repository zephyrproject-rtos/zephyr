/*
 * Copyright (c) 2026 GigaDevice Semiconductor Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GD32C2X1 reset controller definitions for device tree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32C2X1_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32C2X1_H_

#include "gd32-common.h"

/**
 * @name Register offsets
 * @{
 */

#define GD32_AHB1RST_OFFSET 0x10U /**< AHB1 peripheral reset register offset */
#define GD32_AHB2RST_OFFSET 0x14U /**< AHB2 peripheral reset register offset */
#define GD32_APBRST_OFFSET  0x24U /**< APB peripheral reset register offset */

/** @} */

/**
 * @name Reset definitions for peripherals
 * @{
 */

/* AHB1 peripherals */
#define GD32_RESET_CRC GD32_RESET_CONFIG(AHB1RST, 12U) /**< CRC reset */
#define GD32_RESET_DMA GD32_RESET_CONFIG(AHB1RST, 21U) /**< DMA reset */

/* AHB2 peripherals */
#define GD32_RESET_GPIOA GD32_RESET_CONFIG(AHB2RST, 17U) /**< GPIOA reset */
#define GD32_RESET_GPIOB GD32_RESET_CONFIG(AHB2RST, 18U) /**< GPIOB reset */
#define GD32_RESET_GPIOC GD32_RESET_CONFIG(AHB2RST, 19U) /**< GPIOC reset */
#define GD32_RESET_GPIOD GD32_RESET_CONFIG(AHB2RST, 20U) /**< GPIOD reset */
#define GD32_RESET_GPIOF GD32_RESET_CONFIG(AHB2RST, 22U) /**< GPIOF reset */

/* APB peripherals */
#define GD32_RESET_SYSCFG  GD32_RESET_CONFIG(APBRST, 0U)  /**< SYSCFG reset */
#define GD32_RESET_CMP     GD32_RESET_CONFIG(APBRST, 1U)  /**< Comparator reset */
#define GD32_RESET_WWDGT   GD32_RESET_CONFIG(APBRST, 8U)  /**< Window watchdog reset */
#define GD32_RESET_ADC     GD32_RESET_CONFIG(APBRST, 9U)  /**< ADC reset */
#define GD32_RESET_TIMER0  GD32_RESET_CONFIG(APBRST, 10U) /**< TIMER0 reset */
#define GD32_RESET_TIMER2  GD32_RESET_CONFIG(APBRST, 11U) /**< TIMER2 reset */
#define GD32_RESET_SPI0    GD32_RESET_CONFIG(APBRST, 12U) /**< SPI0 reset */
#define GD32_RESET_SPI1    GD32_RESET_CONFIG(APBRST, 13U) /**< SPI1 reset */
#define GD32_RESET_USART0  GD32_RESET_CONFIG(APBRST, 14U) /**< USART0 reset */
#define GD32_RESET_USART1  GD32_RESET_CONFIG(APBRST, 15U) /**< USART1 reset */
#define GD32_RESET_TIMER13 GD32_RESET_CONFIG(APBRST, 16U) /**< TIMER13 reset */
#define GD32_RESET_TIMER15 GD32_RESET_CONFIG(APBRST, 17U) /**< TIMER15 reset */
#define GD32_RESET_TIMER16 GD32_RESET_CONFIG(APBRST, 18U) /**< TIMER16 reset */
#define GD32_RESET_USART2  GD32_RESET_CONFIG(APBRST, 19U) /**< USART2 reset */
#define GD32_RESET_I2C0    GD32_RESET_CONFIG(APBRST, 21U) /**< I2C0 reset */
#define GD32_RESET_I2C1    GD32_RESET_CONFIG(APBRST, 22U) /**< I2C1 reset */
#define GD32_RESET_PMU     GD32_RESET_CONFIG(APBRST, 28U) /**< PMU reset */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32C2X1_H_ */
