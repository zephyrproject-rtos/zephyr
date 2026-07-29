/*
 * Copyright (c) 2026 Lucy Wong
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32V10X_CLOCKS_H__
#define __CH32V10X_CLOCKS_H__

/**
 * @file
 * @brief CH32V10x peripheral clock identifiers for devicetree bindings
 */

/** @brief AHB bus register offset */
#define CH32V10X_AHB_PCENR_OFFSET  0
/** @brief APB2 bus register offset */
#define CH32V10X_APB2_PCENR_OFFSET 1
/** @brief APB1 bus register offset */
#define CH32V10X_APB1_PCENR_OFFSET 2

/**
 * @brief Encode a clock identifier from bus and bit position
 */
#define CH32V10X_CLOCK_CONFIG(bus, bit) (((CH32V10X_##bus##_PCENR_OFFSET) << 5) | (bit))

/** @brief DMA1 clock (AHB) */
#define CH32V10X_CLOCK_DMA1  CH32V10X_CLOCK_CONFIG(AHB, 0)
/** @brief SRAM clock (AHB) */
#define CH32V10X_CLOCK_SRAM  CH32V10X_CLOCK_CONFIG(AHB, 2)
/** @brief Flash interface clock (AHB) */
#define CH32V10X_CLOCK_FLITF CH32V10X_CLOCK_CONFIG(AHB, 4)
/** @brief CRC clock (AHB) */
#define CH32V10X_CLOCK_CRC   CH32V10X_CLOCK_CONFIG(AHB, 6)
/** @brief FSMC clock (AHB) */
#define CH32V10X_CLOCK_FSMC  CH32V10X_CLOCK_CONFIG(AHB, 8)
/** @brief USB HD clock (AHB) */
#define CH32V10X_CLOCK_USBHD CH32V10X_CLOCK_CONFIG(AHB, 12)

/** @brief AFIO clock (APB2) */
#define CH32V10X_CLOCK_AFIO   CH32V10X_CLOCK_CONFIG(APB2, 0)
/** @brief GPIO port A clock (APB2) */
#define CH32V10X_CLOCK_IOPA   CH32V10X_CLOCK_CONFIG(APB2, 2)
/** @brief GPIO port B clock (APB2) */
#define CH32V10X_CLOCK_IOPB   CH32V10X_CLOCK_CONFIG(APB2, 3)
/** @brief GPIO port C clock (APB2) */
#define CH32V10X_CLOCK_IOPC   CH32V10X_CLOCK_CONFIG(APB2, 4)
/** @brief GPIO port D clock (APB2) */
#define CH32V10X_CLOCK_IOPD   CH32V10X_CLOCK_CONFIG(APB2, 5)
/** @brief ADC1 clock (APB2) */
#define CH32V10X_CLOCK_ADC1   CH32V10X_CLOCK_CONFIG(APB2, 9)
/** @brief TIM1 clock (APB2) */
#define CH32V10X_CLOCK_TIM1   CH32V10X_CLOCK_CONFIG(APB2, 11)
/** @brief SPI1 clock (APB2) */
#define CH32V10X_CLOCK_SPI1   CH32V10X_CLOCK_CONFIG(APB2, 12)
/** @brief USART1 clock (APB2) */
#define CH32V10X_CLOCK_USART1 CH32V10X_CLOCK_CONFIG(APB2, 14)

/** @brief TIM2 clock (APB1) */
#define CH32V10X_CLOCK_TIM2   CH32V10X_CLOCK_CONFIG(APB1, 0)
/** @brief TIM3 clock (APB1) */
#define CH32V10X_CLOCK_TIM3   CH32V10X_CLOCK_CONFIG(APB1, 1)
/** @brief TIM4 clock (APB1) */
#define CH32V10X_CLOCK_TIM4   CH32V10X_CLOCK_CONFIG(APB1, 2)
/** @brief WWDG clock (APB1) */
#define CH32V10X_CLOCK_WWDG   CH32V10X_CLOCK_CONFIG(APB1, 11)
/** @brief SPI2 clock (APB1) */
#define CH32V10X_CLOCK_SPI2   CH32V10X_CLOCK_CONFIG(APB1, 14)
/** @brief USART2 clock (APB1) */
#define CH32V10X_CLOCK_USART2 CH32V10X_CLOCK_CONFIG(APB1, 17)
/** @brief USART3 clock (APB1) */
#define CH32V10X_CLOCK_USART3 CH32V10X_CLOCK_CONFIG(APB1, 18)
/** @brief I2C1 clock (APB1) */
#define CH32V10X_CLOCK_I2C1   CH32V10X_CLOCK_CONFIG(APB1, 21)
/** @brief I2C2 clock (APB1) */
#define CH32V10X_CLOCK_I2C2   CH32V10X_CLOCK_CONFIG(APB1, 22)
/** @brief USB device clock (APB1) */
#define CH32V10X_CLOCK_USBD   CH32V10X_CLOCK_CONFIG(APB1, 23)
/** @brief CAN1 clock (APB1) */
#define CH32V10X_CLOCK_CAN1   CH32V10X_CLOCK_CONFIG(APB1, 25)
/** @brief Backup domain clock (APB1) */
#define CH32V10X_CLOCK_BKP    CH32V10X_CLOCK_CONFIG(APB1, 27)
/** @brief Power interface clock (APB1) */
#define CH32V10X_CLOCK_PWR    CH32V10X_CLOCK_CONFIG(APB1, 28)

#endif
