/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32F4XX_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32F4XX_CLOCKS_H_

#include "gd32-clocks-common.h"

/**
 * @name Register offsets
 * @{
 */

#define GD32_AHB1EN_OFFSET       0x30U
#define GD32_AHB2EN_OFFSET       0x34U
#define GD32_AHB3EN_OFFSET       0x38U
#define GD32_APB1EN_OFFSET       0x40U
#define GD32_APB2EN_OFFSET       0x44U
#define GD32_ADDAPB1EN_OFFSET    0xE4U

/** @} */

/**
 * @name Clock enable/disable definitions for peripherals
 * @{
 */

/* AHB1 peripherals */
#define GD32_CLOCK_GPIOA      GD32_CLOCK_CONFIG(AHB1EN, 0U)
#define GD32_CLOCK_GPIOB      GD32_CLOCK_CONFIG(AHB1EN, 1U)
#define GD32_CLOCK_GPIOC      GD32_CLOCK_CONFIG(AHB1EN, 2U)
#define GD32_CLOCK_GPIOD      GD32_CLOCK_CONFIG(AHB1EN, 3U)
#define GD32_CLOCK_GPIOE      GD32_CLOCK_CONFIG(AHB1EN, 4U)
#define GD32_CLOCK_GPIOF      GD32_CLOCK_CONFIG(AHB1EN, 5U)
#define GD32_CLOCK_GPIOG      GD32_CLOCK_CONFIG(AHB1EN, 6U)
#define GD32_CLOCK_GPIOH      GD32_CLOCK_CONFIG(AHB1EN, 7U)
#define GD32_CLOCK_GPIOI      GD32_CLOCK_CONFIG(AHB1EN, 8U)
#define GD32_CLOCK_CRC        GD32_CLOCK_CONFIG(AHB1EN, 12U)
#define GD32_CLOCK_BKPSRAM    GD32_CLOCK_CONFIG(AHB1EN, 18U)
#define GD32_CLOCK_TCMSRAM    GD32_CLOCK_CONFIG(AHB1EN, 20U)
#define GD32_CLOCK_DMA0       GD32_CLOCK_CONFIG(AHB1EN, 21U)
#define GD32_CLOCK_DMA1       GD32_CLOCK_CONFIG(AHB1EN, 22U)
#define GD32_CLOCK_IPA        GD32_CLOCK_CONFIG(AHB1EN, 23U)
#define GD32_CLOCK_ENET       GD32_CLOCK_CONFIG(AHB1EN, 25U)
#define GD32_CLOCK_ENETTX     GD32_CLOCK_CONFIG(AHB1EN, 26U)
#define GD32_CLOCK_ENETRX     GD32_CLOCK_CONFIG(AHB1EN, 27U)
#define GD32_CLOCK_ENETPTP    GD32_CLOCK_CONFIG(AHB1EN, 28U)
#define GD32_CLOCK_USBHS      GD32_CLOCK_CONFIG(AHB1EN, 29U)
#define GD32_CLOCK_USBHSULPI  GD32_CLOCK_CONFIG(AHB1EN, 30U)

/* AHB2 peripherals */
#define GD32_CLOCK_DCI        GD32_CLOCK_CONFIG(AHB2EN, 0U)
#define GD32_CLOCK_TRNG       GD32_CLOCK_CONFIG(AHB2EN, 6U)
#define GD32_CLOCK_USBFS      GD32_CLOCK_CONFIG(AHB2EN, 7U)

/* AHB3 peripherals */
#define GD32_CLOCK_EXMC       GD32_CLOCK_CONFIG(AHB3EN, 0U)

/* APB1 peripherals */
#define GD32_CLOCK_TIMER1     GD32_CLOCK_CONFIG(APB1EN, 0U)
#define GD32_CLOCK_TIMER2     GD32_CLOCK_CONFIG(APB1EN, 1U)
#define GD32_CLOCK_TIMER3     GD32_CLOCK_CONFIG(APB1EN, 2U)
#define GD32_CLOCK_TIMER4     GD32_CLOCK_CONFIG(APB1EN, 3U)
#define GD32_CLOCK_TIMER5     GD32_CLOCK_CONFIG(APB1EN, 4U)
#define GD32_CLOCK_TIMER6     GD32_CLOCK_CONFIG(APB1EN, 5U)
#define GD32_CLOCK_TIMER11    GD32_CLOCK_CONFIG(APB1EN, 6U)
#define GD32_CLOCK_TIMER12    GD32_CLOCK_CONFIG(APB1EN, 7U)
#define GD32_CLOCK_TIMER13    GD32_CLOCK_CONFIG(APB1EN, 8U)
#define GD32_CLOCK_WWDGT      GD32_CLOCK_CONFIG(APB1EN, 11U)
#define GD32_CLOCK_SPI1       GD32_CLOCK_CONFIG(APB1EN, 14U)
#define GD32_CLOCK_SPI2       GD32_CLOCK_CONFIG(APB1EN, 15U)
#define GD32_CLOCK_USART1     GD32_CLOCK_CONFIG(APB1EN, 17U)
#define GD32_CLOCK_USART2     GD32_CLOCK_CONFIG(APB1EN, 18U)
#define GD32_CLOCK_UART3      GD32_CLOCK_CONFIG(APB1EN, 19U)
#define GD32_CLOCK_UART4      GD32_CLOCK_CONFIG(APB1EN, 20U)
#define GD32_CLOCK_I2C0       GD32_CLOCK_CONFIG(APB1EN, 21U)
#define GD32_CLOCK_I2C1       GD32_CLOCK_CONFIG(APB1EN, 22U)
#define GD32_CLOCK_I2C2       GD32_CLOCK_CONFIG(APB1EN, 23U)
#define GD32_CLOCK_CAN0       GD32_CLOCK_CONFIG(APB1EN, 25U)
#define GD32_CLOCK_CAN1       GD32_CLOCK_CONFIG(APB1EN, 26U)
#define GD32_CLOCK_PMU        GD32_CLOCK_CONFIG(APB1EN, 28U)
#define GD32_CLOCK_DAC        GD32_CLOCK_CONFIG(APB1EN, 29U)
#define GD32_CLOCK_UART6      GD32_CLOCK_CONFIG(APB1EN, 30U)
#define GD32_CLOCK_UART7      GD32_CLOCK_CONFIG(APB1EN, 31U)
#define GD32_CLOCK_RTC        GD32_CLOCK_CONFIG(BDCTL, 15U)

/* APB2 peripherals */
#define GD32_CLOCK_TIMER0     GD32_CLOCK_CONFIG(APB2EN, 0U)
#define GD32_CLOCK_TIMER7     GD32_CLOCK_CONFIG(APB2EN, 1U)
#define GD32_CLOCK_USART0     GD32_CLOCK_CONFIG(APB2EN, 4U)
#define GD32_CLOCK_USART5     GD32_CLOCK_CONFIG(APB2EN, 5U)
#define GD32_CLOCK_ADC0       GD32_CLOCK_CONFIG(APB2EN, 8U)
#define GD32_CLOCK_ADC1       GD32_CLOCK_CONFIG(APB2EN, 9U)
#define GD32_CLOCK_ADC2       GD32_CLOCK_CONFIG(APB2EN, 10U)
#define GD32_CLOCK_SDIO       GD32_CLOCK_CONFIG(APB2EN, 11U)
#define GD32_CLOCK_SPI0       GD32_CLOCK_CONFIG(APB2EN, 12U)
#define GD32_CLOCK_SPI3       GD32_CLOCK_CONFIG(APB2EN, 13U)
#define GD32_CLOCK_SYSCFG     GD32_CLOCK_CONFIG(APB2EN, 14U)
#define GD32_CLOCK_TIMER8     GD32_CLOCK_CONFIG(APB2EN, 16U)
#define GD32_CLOCK_TIMER9     GD32_CLOCK_CONFIG(APB2EN, 17U)
#define GD32_CLOCK_TIMER10    GD32_CLOCK_CONFIG(APB2EN, 18U)
#define GD32_CLOCK_SPI4       GD32_CLOCK_CONFIG(APB2EN, 20U)
#define GD32_CLOCK_SPI5       GD32_CLOCK_CONFIG(APB2EN, 21U)
#define GD32_CLOCK_TLI        GD32_CLOCK_CONFIG(APB2EN, 26U)

/* APB1 additional peripherals */
#define GD32_CLOCK_CTC        GD32_CLOCK_CONFIG(ADDAPB1EN, 27U)
#define GD32_CLOCK_IREF       GD32_CLOCK_CONFIG(ADDAPB1EN, 31U)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_GD32F4XX_CLOCKS_H_ */
