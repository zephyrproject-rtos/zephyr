/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Interrupt numbers for STM32F4 family processors.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM-based 32-bit MCUs
 *
 * Chapter 10.1.3: Interrupt and exception vectors
 */


#ifndef _STM32F4_SOC_IRQ_H_
#define _STM32F4_SOC_IRQ_H_

#define STM32F4_IRQ_WWDG		0
#define STM32F4_IRQ_PVD			1
#define STM32F4_IRQ_EXTI16		STM32F4_IRQ_PVD
#define STM32F4_IRQ_TAMP_STAMP		2
#define STM32F4_IRQ_EXTI21		STM32F4_IRQ_TAMP_STAMP
#define STM32F4_IRQ_RTC_WKUP		3
#define STM32F4_IRQ_EXTI22		STM32F4_IRQ_RTC_WKUP
#define STM32F4_IRQ_FLASH		4
#define STM32F4_IRQ_RCC			5
#define STM32F4_IRQ_EXTI0		6
#define STM32F4_IRQ_EXTI1		7
#define STM32F4_IRQ_EXTI2		8
#define STM32F4_IRQ_EXTI3		9
#define STM32F4_IRQ_EXTI4		10
#define STM32F4_IRQ_DMA1_STREAM0	11
#define STM32F4_IRQ_DMA1_STREAM1	12
#define STM32F4_IRQ_DMA1_STREAM2	13
#define STM32F4_IRQ_DMA1_STREAM3	14
#define STM32F4_IRQ_DMA1_STREAM4	15
#define STM32F4_IRQ_DMA1_STREAM5	16
#define STM32F4_IRQ_DMA1_STREAM6	17
#define STM32F4_IRQ_ADC			18
#define STM32F4_IRQ_CAN1_TX		19
#define STM32F4_IRQ_CAN1_RX0		20
#define STM32F4_IRQ_CAN1_RX1		21
#define STM32F4_IRQ_CAN1_SCE		22
#define STM32F4_IRQ_EXTI9_5		23
#define STM32F4_IRQ_TIM1_BRK_TIM9	24
#define STM32F4_IRQ_TIM1_UP_TIM10	25
#define STM32F4_IRQ_TIM1_TRG_COM_TIM11	26
#define STM32F4_IRQ_TIM1_CC		27
#define STM32F4_IRQ_TIM2		28
#define STM32F4_IRQ_TIM3		29
#define STM32F4_IRQ_TIM4		30
#define STM32F4_IRQ_I2C1_EV		31
#define STM32F4_IRQ_I2C1_ER		32
#define STM32F4_IRQ_I2C2_EV		33
#define STM32F4_IRQ_I2C2_ER		34
#define STM32F4_IRQ_SPI1		35
#define STM32F4_IRQ_SPI2		36
#define STM32F4_IRQ_USART1		37
#define STM32F4_IRQ_USART2		38
#define STM32F4_IRQ_USART3		39
#define STM32F4_IRQ_EXTI15_10		40
#define STM32F4_IRQ_RTC_ALARM		41
#define STM32F4_IRQ_EXTI17		STM32F4_IRQ_RTC_ALARM
#define STM32F4_IRQ_OTG_FS_WKUP		42
#define STM32F4_IRQ_EXTI18		STM32F4_IRQ_OTG_FS_WKUP
#define STM32F4_IRQ_TIM8_BRK_TIM12	43
#define STM32F4_IRQ_TIM8_UP_TIM12	44
#define STM32F4_IRQ_TIM8_TRG_COM_TIM14	45
#define STM32F4_IRQ_TIM8_CC		46
#define STM32F4_IRQ_DMA1_STREAM7	47
#define STM32F4_IRQ_FSMC		48
#define STM32F4_IRQ_SDIO		49
#define STM32F4_IRQ_TIM5		50
#define STM32F4_IRQ_SPI3		51
#define STM32F4_IRQ_UART4		52
#define STM32F4_IRQ_UART5		53
#define STM32F4_IRQ_TIM6_DAC		54
#define STM32F4_IRQ_TIM7		55
#define STM32F4_IRQ_DMA2_STREAM0	56
#define STM32F4_IRQ_DMA2_STREAM1	57
#define STM32F4_IRQ_DMA2_STREAM2	58
#define STM32F4_IRQ_DMA2_STREAM3	59
#define STM32F4_IRQ_DMA2_STREAM4	60
#define STM32F4_IRQ_ETH			61
#define STM32F4_IRQ_ETH_WKUP		62
#define STM32F4_IRQ_CAN2_TX		63
#define STM32F4_IRQ_CAN2_RX0		64
#define STM32F4_IRQ_CAN2_RX1		65
#define STM32F4_IRQ_CAN2_SCE		66
#define STM32F4_IRQ_OTG_FS		67
#define STM32F4_IRQ_DMA2_STREAM5	68
#define STM32F4_IRQ_DMA2_STREAM6	69
#define STM32F4_IRQ_DMA2_STREAM7	70
#define STM32F4_IRQ_USART6		71
#define STM32F4_IRQ_I2C3_EV		72
#define STM32F4_IRQ_I2C3_ER		73
#define STM32F4_IRQ_OTG_HS_EP1_OUT	74
#define STM32F4_IRQ_OTG_HS_EP1_IN	75
#define STM32F4_IRQ_OTG_HS_WKUP		76
#define STM32F4_IRQ_OTG_HS		77
#define STM32F4_IRQ_DCMI		78
#define STM32F4_IRQ_CRYP		79
#define STM32F4_IRQ_HASH_RNG		80
#define STM32F4_IRQ_FPU			81
#define STM32F4_IRQ_UART7		82
#define STM32F4_IRQ_UART8		83
#define STM32F4_IRQ_SPI4		84
#define STM32F4_IRQ_SPI5		85
#define STM32F4_IRQ_SPI6		86
#define STM32F4_IRQ_SAI1		87
#define STM32F4_IRQ_LTDC		88
#define STM32F4_IRQ_LTDC_ER		89
#define STM32F4_IRQ_DMA2D		90
#define STM32F4_IRQ_QUADSPI		91
#define STM32F4_IRQ_DSI			92
#define STM32F4_IRQ_CEC			93
#define STM32F4_IRQ_SPDIF_RX		94
#define STM32F4_IRQ_FMPI2C1_EV		95
#define STM32F4_IRQ_FMPI2C1_ER		96
#define STM32F4_IRQ_LPTIM1		97

#endif	/* _STM32F4_SOC_IRQ_H_ */
