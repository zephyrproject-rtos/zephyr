/*
 * Copyright (c) 2018 Endre Karlson <endre.karlson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Interrupt numbers for STM32L0 family processors.
 *
 * Based on reference manual:
 *   STM32L0X advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 11.1.3: Interrupt and exception vectors
 */


#ifndef _STM32L0_SOC_IRQ_H_
#define _STM32L0_SOC_IRQ_H_

/* FIXME: Remove when use of enum line number in IRQ_CONNECT is
 * made possible by GH-2657.
 * soc_irq.h, once it is possible, should be removed.
 */

#define STM32L0_IRQ_WWDG			0
#define STM32L0_IRQ_PVD         		1
#define STM32L0_IRQ_RTC				2
#define STM32L0_IRQ_FLASH			3
#define STM32L0_IRQ_RCC				4
#define STM32L0_IRQ_EXTI0_1			5
#define STM32L0_IRQ_EXTI2_3			6
#define STM32L0_IRQ_EXTI4_15			7
#define STM32L0_IRQ_TSC             		8
#define STM32L0_IRQ_DMA_CH1			9
#define STM32L0_IRQ_DMA_CH2_3			10
#define STM32L0_IRQ_DMA_CH4_7			11
#define STM32L0_IRQ_ADC				12
#define STM32L0_IRQ_LPTIM1			13
#define STM32L0_IRQ_USART_4_5   		14
#define STM32L0_IRQ_TIM2			15
#define STM32L0_IRQ_TIM3			16
#define STM32L0_IRQ_TIM6_DAC			17
#define STM32L0_IRQ_TIM7			18
/* reserved */
#define STM32L0_IRQ_TIM21			20
#define STM32L0_IRQ_I2C3			21
#define STM32L0_IRQ_TIM22			22
#define STM32L0_IRQ_I2C1			23
#define STM32L0_IRQ_I2C2			24
#define STM32L0_IRQ_SPI1			25
#define STM32L0_IRQ_SPI2			26
#define STM32L0_IRQ_USART1			27
#define STM32L0_IRQ_USART2			28
#define STM32L0_IRQ_LPUART1_AES_RNG     	29
#define STM32L0_IRQ_LCD         		30
#define STM32L0_IRQ_USB				31

#endif	/* _STM32L0_SOC_IRQ_H_ */
