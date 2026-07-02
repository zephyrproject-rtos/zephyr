/*
 * Copyright (c) 2022 YuLong Yao <feilongphone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Peripheral reset identifiers for GigaDevice GD32A50x
 * @ingroup reset_controller_gd32a50x
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32A50X_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32A50X_H_

#include "gd32-common.h"

/**
 * @defgroup reset_controller_gd32a50x GigaDevice GD32A50x reset controller helpers
 * @brief GigaDevice GD32A50x reset controller helpers
 * @ingroup reset_controller_gigadevice
 *
 * Reset identifiers follow the pattern @c GD32_RESET_\<PERIPHERAL\>, where
 * @c \<PERIPHERAL\> is the GD32A50x peripheral name from the reference manual (for example, @c
 * GD32_RESET_USART1 resets USART1 and @c GD32_RESET_GPIOA resets GPIO port A). Pass these
 * identifiers directly to a @c resets property.
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @name Register offsets
 * @{
 */

#define GD32_AHBRST_OFFSET        0x28U
#define GD32_APB1RST_OFFSET       0x10U
#define GD32_APB2RST_OFFSET       0x0CU

/** @} */

/**
 * @name Peripheral reset identifiers
 * @{
 */


/* AHB peripherals */
#define GD32_RESET_DMA0     GD32_RESET_CONFIG(AHBRST, 0U)
#define GD32_RESET_DMA1     GD32_RESET_CONFIG(AHBRST, 1U)
#define GD32_RESET_SRAMSP   GD32_RESET_CONFIG(AHBRST, 2U)
#define GD32_RESET_DMAMUX   GD32_RESET_CONFIG(AHBRST, 3U)
#define GD32_RESET_FMCSP    GD32_RESET_CONFIG(AHBRST, 4U)
#define GD32_RESET_CRC      GD32_RESET_CONFIG(AHBRST, 6U)
#define GD32_RESET_MFCOM    GD32_RESET_CONFIG(AHBRST, 14U)
#define GD32_RESET_GPIOA    GD32_RESET_CONFIG(AHBRST, 17U)
#define GD32_RESET_GPIOB    GD32_RESET_CONFIG(AHBRST, 18U)
#define GD32_RESET_GPIOC    GD32_RESET_CONFIG(AHBRST, 19U)
#define GD32_RESET_GPIOD    GD32_RESET_CONFIG(AHBRST, 20U)
#define GD32_RESET_GPIOE    GD32_RESET_CONFIG(AHBRST, 21U)
#define GD32_RESET_GPIOF    GD32_RESET_CONFIG(AHBRST, 22U)

/* APB1 peripherals */
#define GD32_RESET_TIMER1     GD32_RESET_CONFIG(APB1RST, 0U)
#define GD32_RESET_TIMER5     GD32_RESET_CONFIG(APB1RST, 4U)
#define GD32_RESET_TIMER6     GD32_RESET_CONFIG(APB1RST, 5U)
#define GD32_RESET_WWDGT      GD32_RESET_CONFIG(APB1RST, 11U)
#define GD32_RESET_SPI1       GD32_RESET_CONFIG(APB1RST, 14U)
#define GD32_RESET_USART1     GD32_RESET_CONFIG(APB1RST, 17U)
#define GD32_RESET_USART2     GD32_RESET_CONFIG(APB1RST, 18U)
#define GD32_RESET_I2C0       GD32_RESET_CONFIG(APB1RST, 21U)
#define GD32_RESET_I2C1       GD32_RESET_CONFIG(APB1RST, 22U)
#define GD32_RESET_BKP        GD32_RESET_CONFIG(APB1RST, 26U)
#define GD32_RESET_PMU        GD32_RESET_CONFIG(APB1RST, 28U)
#define GD32_RESET_DAC        GD32_RESET_CONFIG(APB1RST, 29U)

/* APB2 peripherals */
#define GD32_RESET_SYSCFG     GD32_RESET_CONFIG(APB2RST, 0U)
#define GD32_RESET_CMP        GD32_RESET_CONFIG(APB2RST, 1U)
#define GD32_RESET_ADC0       GD32_RESET_CONFIG(APB2RST, 9U)
#define GD32_RESET_ADC1       GD32_RESET_CONFIG(APB2RST, 10U)
#define GD32_RESET_TIMER0     GD32_RESET_CONFIG(APB2RST, 11U)
#define GD32_RESET_SPI0       GD32_RESET_CONFIG(APB2RST, 12U)
#define GD32_RESET_TIMER7     GD32_RESET_CONFIG(APB2RST, 13U)
#define GD32_RESET_USART0     GD32_RESET_CONFIG(APB2RST, 14U)
#define GD32_RESET_TIMER19    GD32_RESET_CONFIG(APB2RST, 20U)
#define GD32_RESET_TIMER20    GD32_RESET_CONFIG(APB2RST, 21U)
#define GD32_RESET_TRIGSEL    GD32_RESET_CONFIG(APB2RST, 29U)
#define GD32_RESET_CAN0       GD32_RESET_CONFIG(APB2RST, 30U)
#define GD32_RESET_CAN1       GD32_RESET_CONFIG(APB2RST, 31U)

/** @} */

/** @endcond */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_GD32A50X_H_ */
