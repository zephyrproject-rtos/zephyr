/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file at32f402_405_clocks.h
 * @brief List clock subsystem IDs for at32f402/405.
 *
 * Clock subsystem IDs. To be used in devicetree nodes, and as argument for clock API.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AT32F402_405_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AT32F402_405_CLOCKS_H_

/**
 * Encode crm register offset and configuration bit.
 *
 * - 0..5: bit number
 * - 6..14: offset
 * - 15: reserved
 *
 * @param reg crm register name (expands to AT32{reg}_OFFSET)
 * @param bit Configuration bit
 */
#define AT32_CLOCK_CONFIG(reg, bit) (((AT32_##reg##_OFFSET) << 6U) | (bit))

/**
 * @name Register offsets
 * @{
 */

 /** AHB1 clock address offset */
#define AT32_AHB1EN_OFFSET 0x30U

 /** AHB2 clock address offset */
#define AT32_AHB2EN_OFFSET 0x34U

 /** AHB3 clock address offset */
#define AT32_AHB3EN_OFFSET 0x38U

 /** APB1 clock address offset */
#define AT32_APB1EN_OFFSET 0x40U

 /** APB2 clock address offset */
#define AT32_APB2EN_OFFSET 0x44U

/** @} */

/**
 * @name Clock enable definitions for peripherals
 * @{
 */

/** AHB1 GPIOA clock */
#define AT32_CLOCK_GPIOA AT32_CLOCK_CONFIG(AHB1EN, 0)

/** AHB1 GPIOB clock */
#define AT32_CLOCK_GPIOB AT32_CLOCK_CONFIG(AHB1EN, 1)

/** AHB1 GPIOC clock */
#define AT32_CLOCK_GPIOC AT32_CLOCK_CONFIG(AHB1EN, 2)

/** AHB1 GPIOD clock */
#define AT32_CLOCK_GPIOD AT32_CLOCK_CONFIG(AHB1EN, 3)

/** AHB1 GPIOF clock */
#define AT32_CLOCK_GPIOF AT32_CLOCK_CONFIG(AHB1EN, 5)

/** AHB1 CRC clock */
#define AT32_CLOCK_CRC   AT32_CLOCK_CONFIG(AHB1EN, 12)

/** AHB1 DMA1 clock */
#define AT32_CLOCK_DMA1  AT32_CLOCK_CONFIG(AHB1EN, 22)

/** AHB1 DMA2 clock */
#define AT32_CLOCK_DMA2  AT32_CLOCK_CONFIG(AHB1EN, 24)

/** AHB1 USBHS clock */
#define AT32_CLOCK_USBHS AT32_CLOCK_CONFIG(AHB1EN, 29)

/** AHB2 USBFS clock */
#define AT32_CLOCK_USBFS AT32_CLOCK_CONFIG(AHB2EN, 7)

/** AHB3 QSPI1 clock */
#define AT32_CLOCK_QSPI1 AT32_CLOCK_CONFIG(AHB3EN, 1)

/** APB1 TIMER2 clock */
#define AT32_CLOCK_TIMER2  AT32_CLOCK_CONFIG(APB1EN, 0)

/** APB1 TIMER3 clock */
#define AT32_CLOCK_TIMER3  AT32_CLOCK_CONFIG(APB1EN, 1)

/** APB1 TIMER4 clock */
#define AT32_CLOCK_TIMER4  AT32_CLOCK_CONFIG(APB1EN, 2)

/** APB1 TIMER5 clock */
#define AT32_CLOCK_TIMER5  AT32_CLOCK_CONFIG(APB1EN, 3)

/** APB1 TIMER6 clock */
#define AT32_CLOCK_TIMER6  AT32_CLOCK_CONFIG(APB1EN, 4)

/** APB1 TIMER7 clock */
#define AT32_CLOCK_TIMER7  AT32_CLOCK_CONFIG(APB1EN, 5)

/** APB1 TIMER13 clock */
#define AT32_CLOCK_TIMER13 AT32_CLOCK_CONFIG(APB1EN, 7)

/** APB1 TIMER14 clock */
#define AT32_CLOCK_TIMER14 AT32_CLOCK_CONFIG(APB1EN, 8)

/** APB1 WWDT clock */
#define AT32_CLOCK_WWDT    AT32_CLOCK_CONFIG(APB1EN, 11)

/** APB1 SPI2 clock */
#define AT32_CLOCK_SPI2    AT32_CLOCK_CONFIG(APB1EN, 14)

/** APB1 SPI3 clock */
#define AT32_CLOCK_SPI3    AT32_CLOCK_CONFIG(APB1EN, 15)

/** APB1 USART2 clock */
#define AT32_CLOCK_USART2  AT32_CLOCK_CONFIG(APB1EN, 17)

/** APB1 USART3 clock */
#define AT32_CLOCK_USART3  AT32_CLOCK_CONFIG(APB1EN, 18)

/** APB1 USART4 clock */
#define AT32_CLOCK_USART4  AT32_CLOCK_CONFIG(APB1EN, 19)

/** APB1 USART5 clock */
#define AT32_CLOCK_USART5  AT32_CLOCK_CONFIG(APB1EN, 20)

/** APB1 I2C1 clock */
#define AT32_CLOCK_I2C1    AT32_CLOCK_CONFIG(APB1EN, 21)

/** APB1 I2C2 clock */
#define AT32_CLOCK_I2C2    AT32_CLOCK_CONFIG(APB1EN, 22)

/** APB1 I2C3 clock */
#define AT32_CLOCK_I2C3    AT32_CLOCK_CONFIG(APB1EN, 23)

/** APB1 CAN1 clock */
#define AT32_CLOCK_CAN1    AT32_CLOCK_CONFIG(APB1EN, 25)

/** APB1 PWC clock */
#define AT32_CLOCK_PWC     AT32_CLOCK_CONFIG(APB1EN, 28)

/** APB1 UART7 clock */
#define AT32_CLOCK_UART7   AT32_CLOCK_CONFIG(APB1EN, 30)

/** APB1 UART8 clock */
#define AT32_CLOCK_UART8   AT32_CLOCK_CONFIG(APB1EN, 31)

/** APB2 TIMER1 clock */
#define AT32_CLOCK_TIMER1  AT32_CLOCK_CONFIG(APB2EN, 0)

/** APB2 USART1 clock */
#define AT32_CLOCK_USART1  AT32_CLOCK_CONFIG(APB2EN, 4)

/** APB2 USART6 clock */
#define AT32_CLOCK_USART6  AT32_CLOCK_CONFIG(APB2EN, 5)

/** APB2 ADC1 clock */
#define AT32_CLOCK_ADC1    AT32_CLOCK_CONFIG(APB2EN, 8)

/** APB2 SPI1 clock */
#define AT32_CLOCK_SPI1    AT32_CLOCK_CONFIG(APB2EN, 12)

/** APB2 SYSCFG clock */
#define AT32_CLOCK_SYSCFG  AT32_CLOCK_CONFIG(APB2EN, 14)

/** APB2 TIMER9 clock */
#define AT32_CLOCK_TIMER9  AT32_CLOCK_CONFIG(APB2EN, 16)

/** APB2 TIMER10 clock */
#define AT32_CLOCK_TIMER10 AT32_CLOCK_CONFIG(APB2EN, 17)

/** APB2 TIMER11 clock */
#define AT32_CLOCK_TIMER11 AT32_CLOCK_CONFIG(APB2EN, 18)

/** APB2 I2SF5 clock */
#define AT32_CLOCK_I2SF5   AT32_CLOCK_CONFIG(APB2EN, 20)

/** APB2 ACC clock */
#define AT32_CLOCK_ACC     AT32_CLOCK_CONFIG(APB2EN, 29)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_AT32F402_405_CLOCKS_H_ */
