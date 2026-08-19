/*
 * Copyright (c) 2026 Lucy Wong
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __CH32V10X_CLOCKS_H__
#define __CH32V10X_CLOCKS_H__

#include "ch32-common.h"

/**
 * @file
 * @brief CH32V10x peripheral clock identifiers for devicetree bindings
 * @ingroup clock_control_ch32
 */

/** @brief AHB bus register offset */
#define CH32_CLOCK_PCENR_AHB  0x14
/** @brief APB2 bus register offset */
#define CH32_CLOCK_PCENR_APB2 0x18
/** @brief APB1 bus register offset */
#define CH32_CLOCK_PCENR_APB1 0x1C

/** @brief DMA1 clock (AHB) */
#define CH32V10X_CLOCK_DMA1  CH32_CLOCK(AHB, 0)
/** @brief SRAM clock (AHB) */
#define CH32V10X_CLOCK_SRAM  CH32_CLOCK(AHB, 2)
/** @brief Flash interface clock (AHB) */
#define CH32V10X_CLOCK_FLITF CH32_CLOCK(AHB, 4)
/** @brief CRC clock (AHB) */
#define CH32V10X_CLOCK_CRC   CH32_CLOCK(AHB, 6)
/** @brief FSMC clock (AHB) */
#define CH32V10X_CLOCK_FSMC  CH32_CLOCK(AHB, 8)
/** @brief USB HD clock (AHB) */
#define CH32V10X_CLOCK_USBHD CH32_CLOCK(AHB, 12)

/** @brief AFIO clock (APB2) */
#define CH32V10X_CLOCK_AFIO   CH32_CLOCK(APB2, 0)
/** @brief GPIO port A clock (APB2) */
#define CH32V10X_CLOCK_IOPA   CH32_CLOCK(APB2, 2)
/** @brief GPIO port B clock (APB2) */
#define CH32V10X_CLOCK_IOPB   CH32_CLOCK(APB2, 3)
/** @brief GPIO port C clock (APB2) */
#define CH32V10X_CLOCK_IOPC   CH32_CLOCK(APB2, 4)
/** @brief GPIO port D clock (APB2) */
#define CH32V10X_CLOCK_IOPD   CH32_CLOCK(APB2, 5)
/** @brief ADC1 clock (APB2) */
#define CH32V10X_CLOCK_ADC1   CH32_CLOCK(APB2, 9)
/** @brief TIM1 clock (APB2) */
#define CH32V10X_CLOCK_TIM1   CH32_CLOCK(APB2, 11)
/** @brief SPI1 clock (APB2) */
#define CH32V10X_CLOCK_SPI1   CH32_CLOCK(APB2, 12)
/** @brief USART1 clock (APB2) */
#define CH32V10X_CLOCK_USART1 CH32_CLOCK(APB2, 14)

/** @brief TIM2 clock (APB1) */
#define CH32V10X_CLOCK_TIM2   CH32_CLOCK(APB1, 0)
/** @brief TIM3 clock (APB1) */
#define CH32V10X_CLOCK_TIM3   CH32_CLOCK(APB1, 1)
/** @brief TIM4 clock (APB1) */
#define CH32V10X_CLOCK_TIM4   CH32_CLOCK(APB1, 2)
/** @brief WWDG clock (APB1) */
#define CH32V10X_CLOCK_WWDG   CH32_CLOCK(APB1, 11)
/** @brief SPI2 clock (APB1) */
#define CH32V10X_CLOCK_SPI2   CH32_CLOCK(APB1, 14)
/** @brief USART2 clock (APB1) */
#define CH32V10X_CLOCK_USART2 CH32_CLOCK(APB1, 17)
/** @brief USART3 clock (APB1) */
#define CH32V10X_CLOCK_USART3 CH32_CLOCK(APB1, 18)
/** @brief I2C1 clock (APB1) */
#define CH32V10X_CLOCK_I2C1   CH32_CLOCK(APB1, 21)
/** @brief I2C2 clock (APB1) */
#define CH32V10X_CLOCK_I2C2   CH32_CLOCK(APB1, 22)
/** @brief USB device clock (APB1) */
#define CH32V10X_CLOCK_USBD   CH32_CLOCK(APB1, 23)
/** @brief CAN1 clock (APB1) */
#define CH32V10X_CLOCK_CAN1   CH32_CLOCK(APB1, 25)
/** @brief Backup domain clock (APB1) */
#define CH32V10X_CLOCK_BKP    CH32_CLOCK(APB1, 27)
/** @brief Power interface clock (APB1) */
#define CH32V10X_CLOCK_PWR    CH32_CLOCK(APB1, 28)

#endif
