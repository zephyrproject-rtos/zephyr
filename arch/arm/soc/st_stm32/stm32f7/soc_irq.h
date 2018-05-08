/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Interrupt numbers for STM32F7 family processors.
 *
 * Based on reference manual:
 *   advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 10.1.3: Interrupt and exception vectors
 */


#ifndef _STM32F7_SOC_IRQ_H_
#define _STM32F7_SOC_IRQ_H_

#define STM32F7_IRQ_WWDG                    0      /*!< Window WatchDog Interrupt                                         */
#define STM32F7_IRQ_PVD                     1      /*!< PVD through EXTI Line detection Interrupt                         */
#define STM32F7_IRQ_EXTI16    STM32F7_IRQ_PVD
#define STM32F7_IRQ_TAMP_STAMP              2      /*!< Tamper and TimeStamp interrupts through the EXTI line             */
#define STM32F7_IRQ_EXTI21    STM32F7_IRQ_TAMP_STAMP
#define STM32F7_IRQ_RTC_WKUP                3      /*!< RTC Wakeup interrupt through the EXTI line                        */
#define STM32F7_IRQ_EXTI22    STM32F7_IRQ_RTC_WKUP
#define STM32F7_IRQ_FLASH                   4      /*!< FLASH global Interrupt                                            */
#define STM32F7_IRQ_RCC                     5      /*!< RCC global Interrupt                                              */
#define STM32F7_IRQ_EXTI0                   6      /*!< EXTI Line0 Interrupt                                              */
#define STM32F7_IRQ_EXTI1                   7      /*!< EXTI Line1 Interrupt                                              */
#define STM32F7_IRQ_EXTI2                   8      /*!< EXTI Line2 Interrupt                                              */
#define STM32F7_IRQ_EXTI3                   9      /*!< EXTI Line3 Interrupt                                              */
#define STM32F7_IRQ_EXTI4                   10     /*!< EXTI Line4 Interrupt                                              */
#define STM32F7_IRQ_DMA1_STREAM0            11     /*!< DMA1 Stream 0 global Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM1            12     /*!< DMA1 Stream 1 global Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM2            13     /*!< DMA1 Stream 2 global Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM3            14     /*!< DMA1 Stream 3 global Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM4            15     /*!< DMA1 Stream 4 global Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM5            16     /*!< DMA1 Stream 5 global Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM6            17     /*!< DMA1 Stream 6 global Interrupt                                    */
#define STM32F7_IRQ_ADC                     18     /*!< ADC1 ADC2 and ADC3 global Interrupts                             */
#define STM32F7_IRQ_CAN1_TX                 19     /*!< CAN1 TX Interrupt                                                 */
#define STM32F7_IRQ_CAN1_RX0                20     /*!< CAN1 RX0 Interrupt                                                */
#define STM32F7_IRQ_CAN1_RX1                21     /*!< CAN1 RX1 Interrupt                                                */
#define STM32F7_IRQ_CAN1_SCE                22     /*!< CAN1 SCE Interrupt                                                */
#define STM32F7_IRQ_EXTI9_5                 23     /*!< External Line[9:5] Interrupts                                     */
#define STM32F7_IRQ_TIM1_BRK_TIM9           24     /*!< TIM1 Break interrupt and TIM9 global interrupt                    */
#define STM32F7_IRQ_TIM1_UP_TIM10           25     /*!< TIM1 Update Interrupt and TIM10 global interrupt                  */
#define STM32F7_IRQ_TIM1_TRG_COM_TIM11      26     /*!< TIM1 Trigger and Commutation Interrupt and TIM11 global interrupt */
#define STM32F7_IRQ_TIM1_CC                 27     /*!< TIM1 Capture Compare Interrupt                                    */
#define STM32F7_IRQ_TIM2                    28     /*!< TIM2 global Interrupt                                             */
#define STM32F7_IRQ_TIM3                    29     /*!< TIM3 global Interrupt                                             */
#define STM32F7_IRQ_TIM4                    30     /*!< TIM4 global Interrupt                                             */
#define STM32F7_IRQ_I2C1_EV                 31     /*!< I2C1 Event Interrupt                                              */
#define STM32F7_IRQ_I2C1_ER                 32     /*!< I2C1 Error Interrupt                                              */
#define STM32F7_IRQ_I2C2_EV                 33     /*!< I2C2 Event Interrupt                                              */
#define STM32F7_IRQ_I2C2_ER                 34     /*!< I2C2 Error Interrupt                                              */
#define STM32F7_IRQ_SPI1                    35     /*!< SPI1 global Interrupt                                             */
#define STM32F7_IRQ_SPI2                    36     /*!< SPI2 global Interrupt                                             */
#define STM32F7_IRQ_USART1                  37     /*!< USART1 global Interrupt                                           */
#define STM32F7_IRQ_USART2                  38     /*!< USART2 global Interrupt                                           */
#define STM32F7_IRQ_USART3                  39     /*!< USART3 global Interrupt                                           */
#define STM32F7_IRQ_EXTI15_10               40     /*!< External Line[15:10] Interrupts                                   */
#define STM32F7_IRQ_RTC_ALARM               41     /*!< RTC Alarm (A and B) through EXTI Line Interrupt                   */
#define STM32F7_IRQ_EXTI17    STM32F7_IRQ_RTC_ALARM
#define STM32F7_IRQ_OTG_FS_WKUP             42     /*!< USB OTG FS Wakeup through EXTI line interrupt                     */
#define STM32F7_IRQ_EXTI18    STM32F7_IRQ_OTG_FS_WKUP
#define STM32F7_IRQ_TIM8_BRK_TIM12          43     /*!< TIM8 Break Interrupt and TIM12 global interrupt                   */
#define STM32F7_IRQ_TIM8_UP_TIM13           44     /*!< TIM8 Update Interrupt and TIM13 global interrupt                  */
#define STM32F7_IRQ_TIM8_TRG_COM_TIM14      45     /*!< TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt */
#define STM32F7_IRQ_TIM8_CC                 46     /*!< TIM8 Capture Compare Interrupt                                    */
#define STM32F7_IRQ_DMA1_STREAM7            47     /*!< DMA1 Stream7 Interrupt                                            */
#define STM32F7_IRQ_FMC                     48     /*!< FMC global Interrupt                                              */
#define STM32F7_IRQ_SDMMC1                  49     /*!< SDMMC1 global Interrupt                                           */
#define STM32F7_IRQ_TIM5                    50     /*!< TIM5 global Interrupt                                             */
#define STM32F7_IRQ_SPI3                    51     /*!< SPI3 global Interrupt                                             */
#define STM32F7_IRQ_UART4                   52     /*!< UART4 global Interrupt                                            */
#define STM32F7_IRQ_UART5                   53     /*!< UART5 global Interrupt                                            */
#define STM32F7_IRQ_TIM6_DAC                54     /*!< TIM6 global and DAC1&2 underrun error  interrupts                 */
#define STM32F7_IRQ_TIM7                    55     /*!< TIM7 global interrupt                                             */
#define STM32F7_IRQ_DMA2_STREAM0            56     /*!< DMA2 Stream 0 global Interrupt                                    */
#define STM32F7_IRQ_DMA2_STREAM1            57     /*!< DMA2 Stream 1 global Interrupt                                    */
#define STM32F7_IRQ_DMA2_STREAM2            58     /*!< DMA2 Stream 2 global Interrupt                                    */
#define STM32F7_IRQ_DMA2_STREAM3            59     /*!< DMA2 Stream 3 global Interrupt                                    */
#define STM32F7_IRQ_DMA2_STREAM4            60     /*!< DMA2 Stream 4 global Interrupt                                    */
#define STM32F7_IRQ_ETH                     61     /*!< Ethernet global Interrupt                                         */
#define STM32F7_IRQ_ETH_WKUP                62     /*!< Ethernet Wakeup through EXTI line Interrupt                       */
#define STM32F7_IRQ_EXTI19    STM32F7_IRQ_ETH_WKUP
#define STM32F7_IRQ_CAN2_TX                 63     /*!< CAN2 TX Interrupt                                                 */
#define STM32F7_IRQ_CAN2_RX0                64     /*!< CAN2 RX0 Interrupt                                                */
#define STM32F7_IRQ_CAN2_RX1                65     /*!< CAN2 RX1 Interrupt                                                */
#define STM32F7_IRQ_CAN2_SCE                66     /*!< CAN2 SCE Interrupt                                                */
#define STM32F7_IRQ_OTG_FS                  67     /*!< USB OTG FS global Interrupt                                       */
#define STM32F7_IRQ_DMA2_STREAM5            68     /*!< DMA2 Stream 5 global interrupt                                    */
#define STM32F7_IRQ_DMA2_STREAM6            69     /*!< DMA2 Stream 6 global interrupt                                    */
#define STM32F7_IRQ_DMA2_STREAM7            70     /*!< DMA2 Stream 7 global interrupt                                    */
#define STM32F7_IRQ_USART6                  71     /*!< USART6 global interrupt                                           */
#define STM32F7_IRQ_I2C3_EV                 72     /*!< I2C3 event interrupt                                              */
#define STM32F7_IRQ_I2C3_ER                 73     /*!< I2C3 error interrupt                                              */
#define STM32F7_IRQ_OTG_HS_EP1_OUT          74     /*!< USB OTG HS End Point 1 Out global interrupt                       */
#define STM32F7_IRQ_OTG_HS_EP1_IN           75     /*!< USB OTG HS End Point 1 In global interrupt                        */
#define STM32F7_IRQ_OTG_HS_WKUP             76     /*!< USB OTG HS Wakeup through EXTI interrupt                          */
#define STM32F7_IRQ_EXTI20    STM32F7_IRQ_OTG_HS_WKUP
#define STM32F7_IRQ_OTG_HS                  77     /*!< USB OTG HS global interrupt                                       */
#define STM32F7_IRQ_DCMI                    78     /*!< DCMI global interrupt                                             */

#define STM32F7_IRQ_RNG                     80     /*!< RNG global interrupt                                              */
#define STM32F7_IRQ_FPU                     81     /*!< FPU global interrupt                                              */
#define STM32F7_IRQ_UART7                   82     /*!< UART7 global interrupt                                            */
#define STM32F7_IRQ_UART8                   83     /*!< UART8 global interrupt                                            */
#define STM32F7_IRQ_SPI4                    84     /*!< SPI4 global Interrupt                                             */
#define STM32F7_IRQ_SPI5                    85     /*!< SPI5 global Interrupt                                             */
#define STM32F7_IRQ_SPI6                    86     /*!< SPI6 global Interrupt                                             */
#define STM32F7_IRQ_SAI1                    87     /*!< SAI1 global Interrupt                                             */
#define STM32F7_IRQ_LTDC                    88     /*!< LTDC global Interrupt                                             */
#define STM32F7_IRQ_LTDC_ER                 89     /*!< LTDC Error global Interrupt                                       */
#define STM32F7_IRQ_DMA2D                   90     /*!< DMA2D global Interrupt                                            */
#define STM32F7_IRQ_SAI2                    91     /*!< SAI2 global Interrupt                                             */
#define STM32F7_IRQ_QUADSPI                 92     /*!< Quad SPI global interrupt                                         */
#define STM32F7_IRQ_LPTIM1                  93     /*!< LP TIM1 interrupt                                                 */
#define STM32F7_IRQ_EXTI23    STM32F7_IRQ_LPTIM1
#define STM32F7_IRQ_CEC                     94     /*!< HDMI-CEC global Interrupt                                         */
#define STM32F7_IRQ_I2C4_EV                 95     /*!< I2C4 Event Interrupt                                              */
#define STM32F7_IRQ_I2C4_ER                 96     /*!< I2C4 Error Interrupt                                              */
#define STM32F7_IRQ_SPDIF_RX                97     /*!< SPDIF-RX global Interrupt                                         */
#define STM32F7_IRQ_DSI                     98     /*!< DSI global Interrupt                                              */
#define STM32F7_IRQ_DFSDM1_FLT0	            99     /*!< DFSDM1 Filter 0 global Interrupt                                  */
#define STM32F7_IRQ_DFSDM1_FLT1	            100    /*!< DFSDM1 Filter 1 global Interrupt                                  */
#define STM32F7_IRQ_DFSDM1_FLT2	            101    /*!< DFSDM1 Filter 2 global Interrupt                                  */
#define STM32F7_IRQ_DFSDM1_FLT3	            102    /*!< DFSDM1 Filter 3 global Interrupt                                  */
#define STM32F7_IRQ_SDMMC2                  103    /*!< SDMMC2 global Interrupt                                           */
#define STM32F7_IRQ_CAN3_TX                 104    /*!< CAN3 TX Interrupt                                                 */
#define STM32F7_IRQ_CAN3_RX0                105    /*!< CAN3 RX0 Interrupt                                                */
#define STM32F7_IRQ_CAN3_RX1                106    /*!< CAN3 RX1 Interrupt                                                */
#define STM32F7_IRQ_CAN3_SCE                107    /*!< CAN3 SCE Interrupt                                                */
#define STM32F7_IRQ_JPEG                    108    /*!< JPEG global Interrupt                                             */
#define STM32F7_IRQ_MDIOS                   109    /*!< MDIO Slave global Interrupt                                       */
#define STM32F7_IRQ_EXTI24    STM32F7_IRQ_MDIOS
#endif	/* _STM32F7_SOC_IRQ_H_ */
