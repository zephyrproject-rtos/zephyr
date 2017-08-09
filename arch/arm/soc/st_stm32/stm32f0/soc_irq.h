/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Interrupt numbers for STM32F0 family processors.
 *
 * Based on reference manual:
 *   STM32F030x4/x6/x8/xC,
 *   STM32F070x6/xB advanced ARM ® -based MCUs
 *
 * Chapter 11.1.3: Interrupt and exception vectors
 */


#ifndef _STM32F0_SOC_IRQ_H_
#define _STM32F0_SOC_IRQ_H_

/* FIXME: Remove when use of enum line number in IRQ_CONNECT is
 * made possible by ZEP-1165.
 * soc_irq.h, once it is possible, should be removed.
 */

#define STM32F0_IRQ_WWDG			0
/* reserved */
#define STM32F0_IRQ_RTC			2
#define STM32F0_IRQ_FLASH			3
#define STM32F0_IRQ_RCC			4
#define STM32F0_IRQ_EXTI0_1			5
#define STM32F0_IRQ_EXTI2_3			6
#define STM32F0_IRQ_EXTI4_15			7
/* reserved */
#define STM32F0_IRQ_DMA_CH1			9
#define STM32F0_IRQ_DMA_CH2_3			10
#define STM32F0_IRQ_DMA_CH4_5			11
#define STM32F0_IRQ_ADC			12
#define STM32F0_IRQ_TIM1_BRK_UP_TRG_COM	13
#define STM32F0_IRQ_TIM1_CC			14
/* reserved */
#define STM32F0_IRQ_TIM3			16
#define STM32F0_IRQ_TIM6			17
/* reserved */
#define STM32F0_IRQ_TIM14			19
#define STM32F0_IRQ_TIM15			20
#define STM32F0_IRQ_TIM16			21
#define STM32F0_IRQ_TIM17			22
#define STM32F0_IRQ_I2C1			23
#define STM32F0_IRQ_I2C2			24
#define STM32F0_IRQ_SPI1			25
#define STM32F0_IRQ_SPI2			26
#define STM32F0_IRQ_USART1			27
#define STM32F0_IRQ_USART2			28
#define STM32F0_IRQ_USART3_4_5_6		29
/* reserved */
#define STM32F0_IRQ_USB			31

#endif	/* _STM32F0_SOC_IRQ_H_ */
