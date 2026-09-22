/*
 * Copyright (c) 2022 BrainCo.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Peripheral reset identifiers for GigaDevice GD32L23x
 * @ingroup reset_controller_gd32l23x
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32L23X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32L23X_H_

#include "gd32-common.h"

/**
 * @defgroup reset_controller_gd32l23x GigaDevice GD32L23x reset controller helpers
 * @brief GigaDevice GD32L23x reset controller helpers
 * @ingroup reset_controller_gigadevice
 *
 * Reset identifiers follow the pattern @c GD32_RESET_\<PERIPHERAL\>, where
 * @c \<PERIPHERAL\> is the GD32L23x peripheral name from the reference manual (for example, @c
 * GD32_RESET_USART0 resets USART0 and @c GD32_RESET_GPIOA resets GPIO port A). Pass these
 * identifiers directly to a @c resets property.
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @name Register offsets
 * @{
 */

#define GD32_AHB1RST_OFFSET       0x28U
#define GD32_APB1RST_OFFSET       0x10U
#define GD32_APB2RST_OFFSET       0x0CU

/** @} */

/**
 * @name Peripheral reset identifiers
 * @{
 */

/* AHB1 peripherals */
#define GD32_RESET_CRC        GD32_RESET_CONFIG(AHB1RST, 6U)
#define GD32_RESET_GPIOA      GD32_RESET_CONFIG(AHB1RST, 17U)
#define GD32_RESET_GPIOB      GD32_RESET_CONFIG(AHB1RST, 18U)
#define GD32_RESET_GPIOC      GD32_RESET_CONFIG(AHB1RST, 19U)
#define GD32_RESET_GPIOD      GD32_RESET_CONFIG(AHB1RST, 20U)
#define GD32_RESET_GPIOF      GD32_RESET_CONFIG(AHB1RST, 22U)

/* AHB2 peripherals */
#define GD32_RESET_CAU        GD32_RESET_CONFIG(AHB2RST, 1U)
#define GD32_RESET_TRNG       GD32_RESET_CONFIG(AHB2RST, 3U)

/* APB1 peripherals */
#define GD32_RESET_TIMER1     GD32_RESET_CONFIG(APB1RST, 0U)
#define GD32_RESET_TIMER2     GD32_RESET_CONFIG(APB1RST, 1U)
#define GD32_RESET_TIMER5     GD32_RESET_CONFIG(APB1RST, 4U)
#define GD32_RESET_TIMER6     GD32_RESET_CONFIG(APB1RST, 5U)
#define GD32_RESET_TIMER11    GD32_RESET_CONFIG(APB1RST, 8U)
#define GD32_RESET_LPTIMER    GD32_RESET_CONFIG(APB1RST, 9U)
#define GD32_RESET_SLCD       GD32_RESET_CONFIG(APB1RST, 10U)
#define GD32_RESET_WWDGT      GD32_RESET_CONFIG(APB1RST, 11U)
#define GD32_RESET_SPI1       GD32_RESET_CONFIG(APB1RST, 14U)
#define GD32_RESET_USART1     GD32_RESET_CONFIG(APB1RST, 17U)
#define GD32_RESET_LPUART     GD32_RESET_CONFIG(APB1RST, 18U)
#define GD32_RESET_UART3      GD32_RESET_CONFIG(APB1RST, 19U)
#define GD32_RESET_UART4      GD32_RESET_CONFIG(APB1RST, 20U)
#define GD32_RESET_I2C0       GD32_RESET_CONFIG(APB1RST, 21U)
#define GD32_RESET_I2C1       GD32_RESET_CONFIG(APB1RST, 22U)
#define GD32_RESET_USBD       GD32_RESET_CONFIG(APB1RST, 23U)
#define GD32_RESET_I2C2       GD32_RESET_CONFIG(APB1RST, 24U)
#define GD32_RESET_PMU        GD32_RESET_CONFIG(APB1RST, 28U)
#define GD32_RESET_DAC        GD32_RESET_CONFIG(APB1RST, 29U)
#define GD32_RESET_CTC        GD32_RESET_CONFIG(APB1RST, 30U)

/* APB2 peripherals */
#define GD32_RESET_SYSCFG     GD32_RESET_CONFIG(APB2RST, 0U)
#define GD32_RESET_CMP        GD32_RESET_CONFIG(APB2RST, 1U)
#define GD32_RESET_ADC        GD32_RESET_CONFIG(APB2RST, 9U)
#define GD32_RESET_TIMER8     GD32_RESET_CONFIG(APB2RST, 11U)
#define GD32_RESET_SPI0       GD32_RESET_CONFIG(APB2RST, 12U)
#define GD32_RESET_USART0     GD32_RESET_CONFIG(APB2RST, 14U)

/** @} */

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32L23X_H_ */
