/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32V00X_CLOCKS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32V00X_CLOCKS_H_

#define CH32V00X_AHB_PCENR_OFFSET  0
#define CH32V00X_APB2_PCENR_OFFSET 1
#define CH32V00X_APB1_PCENR_OFFSET 2

#define CH32V00X_CLOCK_CONFIG(bus, bit) (((CH32V00X_##bus##_PCENR_OFFSET) << 5) | (bit))

#define CH32V00X_CLOCK_DMA1  CH32V00X_CLOCK_CONFIG(AHB, 0)
#define CH32V00X_CLOCK_SRAM  CH32V00X_CLOCK_CONFIG(AHB, 2)
#define CH32V00X_CLOCK_FLITF CH32V00X_CLOCK_CONFIG(AHB, 4)
#define CH32V00X_CLOCK_CRC   CH32V00X_CLOCK_CONFIG(AHB, 6)
#define CH32V00X_CLOCK_USB   CH32V00X_CLOCK_CONFIG(AHB, 12)

#define CH32V00X_CLOCK_AFIO   CH32V00X_CLOCK_CONFIG(APB2, 0)
#define CH32V00X_CLOCK_IOPA   CH32V00X_CLOCK_CONFIG(APB2, 2)
#define CH32V00X_CLOCK_IOPB   CH32V00X_CLOCK_CONFIG(APB2, 3)
#define CH32V00X_CLOCK_IOPC   CH32V00X_CLOCK_CONFIG(APB2, 4)
#define CH32V00X_CLOCK_IOPD   CH32V00X_CLOCK_CONFIG(APB2, 5)
#define CH32V00X_CLOCK_ADC1   CH32V00X_CLOCK_CONFIG(APB2, 9)
#define CH32V00X_CLOCK_ADC2   CH32V00X_CLOCK_CONFIG(APB2, 10)
#define CH32V00X_CLOCK_TIM1   CH32V00X_CLOCK_CONFIG(APB2, 11)
#define CH32V00X_CLOCK_SPI1   CH32V00X_CLOCK_CONFIG(APB2, 12)
#define CH32V00X_CLOCK_USART1 CH32V00X_CLOCK_CONFIG(APB2, 14)

#define CH32V00X_CLOCK_TIM2   CH32V00X_CLOCK_CONFIG(APB1, 0)
#define CH32V00X_CLOCK_TIM3   CH32V00X_CLOCK_CONFIG(APB1, 1)
#define CH32V00X_CLOCK_WWDG   CH32V00X_CLOCK_CONFIG(APB1, 11)
#define CH32V00X_CLOCK_USART2 CH32V00X_CLOCK_CONFIG(APB1, 17)
#define CH32V00X_CLOCK_I2C1   CH32V00X_CLOCK_CONFIG(APB1, 21)
#define CH32V00X_CLOCK_BKP    CH32V00X_CLOCK_CONFIG(APB1, 27)
#define CH32V00X_CLOCK_PWR    CH32V00X_CLOCK_CONFIG(APB1, 28)
#define CH32V00X_CLOCK_USB    CH32V00X_CLOCK_CONFIG(APB1, 23)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32V00X_CLOCKS_H_ */
