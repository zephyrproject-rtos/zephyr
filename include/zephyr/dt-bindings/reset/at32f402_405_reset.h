/*
 * Copyright (c) 2022 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file at32f402_405_reset.h
 * @brief List reset subsystem IDs for artery at32f402/405.
 *
 * Reset subsystem IDs. To be used in devicetree nodes, and as argument for reset API.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_AT32F402_405_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_AT32F402_405_RESET_H_

/**
 * Encode CRM register offset and configuration bit.
 *
 * - 0..5: bit number
 * - 6..14: offset
 * - 15: reserved
 *
 * @param reg CRM register name (expands to AT32_{reg}_OFFSET)
 * @param bit Configuration bit
 */
#define AT32_RESET_CONFIG(reg, bit) (((AT32_##reg##_OFFSET) << 6U) | (bit))

/**
 * @name Register offsets
 * @{
 */

 /** AHB1 reset address offset */
#define AT32_AHB1RST_OFFSET    0x10U

 /** AHB2 reset address offset */
#define AT32_AHB2RST_OFFSET    0x14U

 /** AHB3 reset address offset */
#define AT32_AHB3RST_OFFSET    0x18U

 /** APB1 reset address offset */
#define AT32_APB1RST_OFFSET    0x20U

 /** APB2 reset address offset */
#define AT32_APB2RST_OFFSET    0x24U

/** @} */

/**
 * @name Reset definitions for peripherals
 * @{
 */

/** AHB1 GPIOA reset */
#define AT32_RESET_GPIOA AT32_RESET_CONFIG(AHB1RST, 0)

/** AHB1 GPIOB reset */
#define AT32_RESET_GPIOB AT32_RESET_CONFIG(AHB1RST, 1)

/** AHB1 GPIOC reset */
#define AT32_RESET_GPIOC AT32_RESET_CONFIG(AHB1RST, 2)

/** AHB1 GPIOD reset */
#define AT32_RESET_GPIOD AT32_RESET_CONFIG(AHB1RST, 3)

/** AHB1 GPIOF reset */
#define AT32_RESET_GPIOF AT32_RESET_CONFIG(AHB1RST, 5)

/** AHB1 CRC reset */
#define AT32_RESET_CRC   AT32_RESET_CONFIG(AHB1RST, 12)

/** AHB1 DMA1 reset */
#define AT32_RESET_DMA1  AT32_RESET_CONFIG(AHB1RST, 22)

/** AHB1 DMA2 reset */
#define AT32_RESET_DMA2  AT32_RESET_CONFIG(AHB1RST, 24)

/** AHB1 USBHS reset */
#define AT32_RESET_USBHS AT32_RESET_CONFIG(AHB1RST, 29)

/** AHB2 USBFS reset */
#define AT32_RESET_USBFS AT32_RESET_CONFIG(AHB2RST, 7)

/** AHB3 QSPI1 reset */
#define AT32_RESET_QSPI1 AT32_RESET_CONFIG(AHB3RST, 1)

/** APB1 TIMER2 reset */
#define AT32_RESET_TIMER2  AT32_RESET_CONFIG(APB1RST, 0)

/** APB1 TIMER3 reset */
#define AT32_RESET_TIMER3  AT32_RESET_CONFIG(APB1RST, 1)

/** APB1 TIMER4 reset */
#define AT32_RESET_TIMER4  AT32_RESET_CONFIG(APB1RST, 2)

/** APB1 TIMER5 reset */
#define AT32_RESET_TIMER5  AT32_RESET_CONFIG(APB1RST, 3)

/** APB1 TIMER6 reset */
#define AT32_RESET_TIMER6  AT32_RESET_CONFIG(APB1RST, 4)

/** APB1 TIMER7 reset */
#define AT32_RESET_TIMER7  AT32_RESET_CONFIG(APB1RST, 5)

/** APB1 TIMER13 reset */
#define AT32_RESET_TIMER13 AT32_RESET_CONFIG(APB1RST, 7)

/** APB1 TIMER14 reset */
#define AT32_RESET_TIMER14 AT32_RESET_CONFIG(APB1RST, 8)

/** APB1 WWDT reset */
#define AT32_RESET_WWDT    AT32_RESET_CONFIG(APB1RST, 11)

/** APB1 SPI2 reset */
#define AT32_RESET_SPI2    AT32_RESET_CONFIG(APB1RST, 14)

/** APB1 SPI3 reset */
#define AT32_RESET_SPI3    AT32_RESET_CONFIG(APB1RST, 15)

/** APB1 USART2 reset */
#define AT32_RESET_USART2  AT32_RESET_CONFIG(APB1RST, 17)

/** APB1 USART3 reset */
#define AT32_RESET_USART3  AT32_RESET_CONFIG(APB1RST, 18)

/** APB1 USART4 reset */
#define AT32_RESET_USART4  AT32_RESET_CONFIG(APB1RST, 19)

/** APB1 USART5 reset */
#define AT32_RESET_USART5  AT32_RESET_CONFIG(APB1RST, 20)

/** APB1 I2C1 reset */
#define AT32_RESET_I2C1    AT32_RESET_CONFIG(APB1RST, 21)

/** APB1 I2C2 reset */
#define AT32_RESET_I2C2    AT32_RESET_CONFIG(APB1RST, 22)

/** APB1 I2C3 reset */
#define AT32_RESET_I2C3    AT32_RESET_CONFIG(APB1RST, 23)

/** APB1 CAN1 reset */
#define AT32_RESET_CAN1    AT32_RESET_CONFIG(APB1RST, 25)

/** APB1 PWC reset */
#define AT32_RESET_PWC     AT32_RESET_CONFIG(APB1RST, 28)

/** APB1 UART7 reset */
#define AT32_RESET_UART7   AT32_RESET_CONFIG(APB1RST, 30)

/** APB1 UART8 reset */
#define AT32_RESET_UART8   AT32_RESET_CONFIG(APB1RST, 31)

/** APB2 TIMER1 reset */
#define AT32_RESET_TIMER1  AT32_RESET_CONFIG(APB2RST, 0)

/** APB2 USART1 reset */
#define AT32_RESET_USART1  AT32_RESET_CONFIG(APB2RST, 4)

/** APB2 USART6 reset */
#define AT32_RESET_USART6  AT32_RESET_CONFIG(APB2RST, 5)

/** APB2 ADC1 reset */
#define AT32_RESET_ADC1    AT32_RESET_CONFIG(APB2RST, 8)

/** APB2 SPI1 reset */
#define AT32_RESET_SPI1    AT32_RESET_CONFIG(APB2RST, 12)

/** APB2 SYSCFG reset */
#define AT32_RESET_SYSCFG  AT32_RESET_CONFIG(APB2RST, 14)

/** APB2 TIMER9 reset */
#define AT32_RESET_TIMER9  AT32_RESET_CONFIG(APB2RST, 16)

/** APB2 TIMER10 reset */
#define AT32_RESET_TIMER10 AT32_RESET_CONFIG(APB2RST, 17)

/** APB2 TIMER11 reset */
#define AT32_RESET_TIMER11 AT32_RESET_CONFIG(APB2RST, 18)

/** APB2 I2SF5 reset */
#define AT32_RESET_I2SF5   AT32_RESET_CONFIG(APB2RST, 20)

/** APB2 ACC reset */
#define AT32_RESET_ACC     AT32_RESET_CONFIG(APB2RST, 29)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_AT32F402_405_RESET_H_ */
