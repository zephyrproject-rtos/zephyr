/**
  ******************************************************************************
  * @file    stm32g0xx_ll_dmamux.h
  * @author  MCD Application Team
  * @brief   Header file of DMAMUX LL module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the 
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32G0xx_LL_DMAMUX_H
#define STM32G0xx_LL_DMAMUX_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
  * @{
  */

#if defined (DMAMUX1)

/** @defgroup DMAMUX_LL DMAMUX
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup DMAMUX_LL_Private_Constants DMAMUX Private Constants
  * @{
  */
/* Define used to get DMAMUX CCR register size */
#define DMAMUX_CCR_SIZE                   0x00000004UL

/* Define used to get DMAMUX RGCR register size */
#define DMAMUX_RGCR_SIZE                  0x00000004UL
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup DMAMUX_LL_Exported_Constants DMAMUX Exported Constants
  * @{
  */
/** @defgroup DMAMUX_LL_EC_CLEAR_FLAG Clear Flags Defines
  * @brief    Flags defines which can be used with LL_DMAMUX_WriteReg function
  * @{
  */
#define LL_DMAMUX_CFR_CSOF0               DMAMUX_CFR_CSOF0       /*!< Synchronization Event Overrun Flag Channel 0  */
#define LL_DMAMUX_CFR_CSOF1               DMAMUX_CFR_CSOF1       /*!< Synchronization Event Overrun Flag Channel 1  */
#define LL_DMAMUX_CFR_CSOF2               DMAMUX_CFR_CSOF2       /*!< Synchronization Event Overrun Flag Channel 2  */
#define LL_DMAMUX_CFR_CSOF3               DMAMUX_CFR_CSOF3       /*!< Synchronization Event Overrun Flag Channel 3  */
#define LL_DMAMUX_CFR_CSOF4               DMAMUX_CFR_CSOF4       /*!< Synchronization Event Overrun Flag Channel 4  */
#if defined(DMAMUX1_Channel5)
#define LL_DMAMUX_CFR_CSOF5               DMAMUX_CFR_CSOF5       /*!< Synchronization Event Overrun Flag Channel 5  */
#endif
#if defined(DMAMUX1_Channel6)
#define LL_DMAMUX_CFR_CSOF6               DMAMUX_CFR_CSOF6       /*!< Synchronization Event Overrun Flag Channel 6  */
#endif
#define LL_DMAMUX_RGCFR_RGCOF0            DMAMUX_RGCFR_COF0      /*!< Request Generator 0 Trigger Event Overrun Flag */
#define LL_DMAMUX_RGCFR_RGCOF1            DMAMUX_RGCFR_COF1      /*!< Request Generator 1 Trigger Event Overrun Flag */
#define LL_DMAMUX_RGCFR_RGCOF2            DMAMUX_RGCFR_COF2      /*!< Request Generator 2 Trigger Event Overrun Flag */
#define LL_DMAMUX_RGCFR_RGCOF3            DMAMUX_RGCFR_COF3      /*!< Request Generator 3 Trigger Event Overrun Flag */
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_DMAMUX_ReadReg function
  * @{
  */
#define LL_DMAMUX_CSR_SOF0                DMAMUX_CSR_SOF0       /*!< Synchronization Event Overrun Flag Channel 0  */
#define LL_DMAMUX_CSR_SOF1                DMAMUX_CSR_SOF1       /*!< Synchronization Event Overrun Flag Channel 1  */
#define LL_DMAMUX_CSR_SOF2                DMAMUX_CSR_SOF2       /*!< Synchronization Event Overrun Flag Channel 2  */
#define LL_DMAMUX_CSR_SOF3                DMAMUX_CSR_SOF3       /*!< Synchronization Event Overrun Flag Channel 3  */
#define LL_DMAMUX_CSR_SOF4                DMAMUX_CSR_SOF4       /*!< Synchronization Event Overrun Flag Channel 4  */
#if defined(DMAMUX1_Channel5)
#define LL_DMAMUX_CSR_SOF5                DMAMUX_CSR_SOF5       /*!< Synchronization Event Overrun Flag Channel 5  */
#endif
#if defined(DMAMUX1_Channel6)
#define LL_DMAMUX_CSR_SOF6                DMAMUX_CSR_SOF6       /*!< Synchronization Event Overrun Flag Channel 6  */
#endif
#define LL_DMAMUX_RGSR_RGOF0              DMAMUX_RGSR_OF0       /*!< Request Generator 0 Trigger Event Overrun Flag */
#define LL_DMAMUX_RGSR_RGOF1              DMAMUX_RGSR_OF1       /*!< Request Generator 1 Trigger Event Overrun Flag */
#define LL_DMAMUX_RGSR_RGOF2              DMAMUX_RGSR_OF2       /*!< Request Generator 2 Trigger Event Overrun Flag */
#define LL_DMAMUX_RGSR_RGOF3              DMAMUX_RGSR_OF3       /*!< Request Generator 3 Trigger Event Overrun Flag */
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_IT IT Defines
  * @brief    IT defines which can be used with LL_DMA_ReadReg and  LL_DMAMUX_WriteReg functions
  * @{
  */
#define LL_DMAMUX_CCR_SOIE                DMAMUX_CxCR_SOIE          /*!< Synchronization Event Overrun Interrupt */
#define LL_DMAMUX_RGCR_RGOIE              DMAMUX_RGxCR_OIE          /*!< Request Generation Trigger Event Overrun Interrupt    */
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_REQUEST Transfer request
  * @{
  */
#define LL_DMAMUX_REQ_MEM2MEM             0x00000000U  /*!< memory to memory transfer  */
#define LL_DMAMUX_REQ_GENERATOR0          0x00000001U  /*!< DMAMUX request generator 0 */
#define LL_DMAMUX_REQ_GENERATOR1          0x00000002U  /*!< DMAMUX request generator 1 */
#define LL_DMAMUX_REQ_GENERATOR2          0x00000003U  /*!< DMAMUX request generator 2 */
#define LL_DMAMUX_REQ_GENERATOR3          0x00000004U  /*!< DMAMUX request generator 3 */
#define LL_DMAMUX_REQ_ADC1                0x00000005U  /*!< DMAMUX ADC1 request        */
#if defined(AES)
#define LL_DMAMUX_REQ_AES_IN              0x00000006U  /*!< DMAMUX AES_IN request      */
#define LL_DMAMUX_REQ_AES_OUT             0x00000007U  /*!< DMAMUX AES_OUT request     */
#endif
#if defined(DAC1)
#define LL_DMAMUX_REQ_DAC1_CH1            0x00000008U  /*!< DMAMUX DAC_CH1 request     */
#define LL_DMAMUX_REQ_DAC1_CH2            0x00000009U  /*!< DMAMUX DAC_CH2 request     */
#endif
#define LL_DMAMUX_REQ_I2C1_RX             0x0000000AU  /*!< DMAMUX I2C1 RX request     */
#define LL_DMAMUX_REQ_I2C1_TX             0x0000000BU  /*!< DMAMUX I2C1 TX request     */
#define LL_DMAMUX_REQ_I2C2_RX             0x0000000CU  /*!< DMAMUX I2C2 RX request     */
#define LL_DMAMUX_REQ_I2C2_TX             0x0000000DU  /*!< DMAMUX I2C2 TX request     */
#if defined(LPUART1)
#define LL_DMAMUX_REQ_LPUART1_RX          0x0000000EU  /*!< DMAMUX LPUART1 RX request  */
#define LL_DMAMUX_REQ_LPUART1_TX          0x0000000FU  /*!< DMAMUX LPUART1 TX request  */
#endif
#define LL_DMAMUX_REQ_SPI1_RX             0x00000010U  /*!< DMAMUX SPI1 RX request     */
#define LL_DMAMUX_REQ_SPI1_TX             0x00000011U  /*!< DMAMUX SPI1 TX request     */
#define LL_DMAMUX_REQ_SPI2_RX             0x00000012U  /*!< DMAMUX SPI2 RX request     */
#define LL_DMAMUX_REQ_SPI2_TX             0x00000013U  /*!< DMAMUX SPI2 TX request     */
#define LL_DMAMUX_REQ_TIM1_CH1            0x00000014U  /*!< DMAMUX TIM1 CH1 request    */
#define LL_DMAMUX_REQ_TIM1_CH2            0x00000015U  /*!< DMAMUX TIM1 CH2 request    */
#define LL_DMAMUX_REQ_TIM1_CH3            0x00000016U  /*!< DMAMUX TIM1 CH3 request    */
#define LL_DMAMUX_REQ_TIM1_CH4            0x00000017U  /*!< DMAMUX TIM1 CH4 request    */
#define LL_DMAMUX_REQ_TIM1_TRIG_COM       0x00000018U  /*!< DMAMUX TIM1 TRIG COM request */
#define LL_DMAMUX_REQ_TIM1_UP             0x00000019U  /*!< DMAMUX TIM1 UP request     */
#if defined(TIM2)
#define LL_DMAMUX_REQ_TIM2_CH1            0x0000001AU  /*!< DMAMUX TIM2 CH1 request    */
#define LL_DMAMUX_REQ_TIM2_CH2            0x0000001BU  /*!< DMAMUX TIM2 CH2 request    */
#define LL_DMAMUX_REQ_TIM2_CH3            0x0000001CU  /*!< DMAMUX TIM2 CH3 request    */
#define LL_DMAMUX_REQ_TIM2_CH4            0x0000001DU  /*!< DMAMUX TIM2 CH4 request    */
#define LL_DMAMUX_REQ_TIM2_TRIG           0x0000001EU  /*!< DMAMUX TIM2 TRIG request   */
#define LL_DMAMUX_REQ_TIM2_UP             0x0000001FU  /*!< DMAMUX TIM2 UP request     */
#endif
#define LL_DMAMUX_REQ_TIM3_CH1            0x00000020U  /*!< DMAMUX TIM3 CH1 request    */
#define LL_DMAMUX_REQ_TIM3_CH2            0x00000021U  /*!< DMAMUX TIM3 CH2 request    */
#define LL_DMAMUX_REQ_TIM3_CH3            0x00000022U  /*!< DMAMUX TIM3 CH3 request    */
#define LL_DMAMUX_REQ_TIM3_CH4            0x00000023U  /*!< DMAMUX TIM3 CH4 request    */
#define LL_DMAMUX_REQ_TIM3_TRIG           0x00000024U  /*!< DMAMUX TIM3 TRIG request   */
#define LL_DMAMUX_REQ_TIM3_UP             0x00000025U  /*!< DMAMUX TIM3 UP request     */
#if defined(TIM6)
#define LL_DMAMUX_REQ_TIM6_UP             0x00000026U  /*!< DMAMUX TIM6 UP request     */
#endif
#if defined(TIM7)
#define LL_DMAMUX_REQ_TIM7_UP             0x00000027U  /*!< DMAMUX TIM7 UP request     */
#endif
#if defined(TIM15)
#define LL_DMAMUX_REQ_TIM15_CH1           0x00000028U  /*!< DMAMUX TIM15 CH1 request   */
#define LL_DMAMUX_REQ_TIM15_CH2           0x00000029U  /*!< DMAMUX TIM15 CH2 request   */
#define LL_DMAMUX_REQ_TIM15_TRIG_COM      0x0000002AU  /*!< DMAMUX TIM15 TRIG COM request */
#define LL_DMAMUX_REQ_TIM15_UP            0x0000002BU  /*!< DMAMUX TIM15 UP request    */
#endif
#define LL_DMAMUX_REQ_TIM16_CH1           0x0000002CU  /*!< DMAMUX TIM16 CH1 request   */
#define LL_DMAMUX_REQ_TIM16_TRIG_COM      0x0000002DU  /*!< DMAMUX TIM16 TRIG COM request */
#define LL_DMAMUX_REQ_TIM16_UP            0x0000002EU  /*!< DMAMUX TIM16 UP request    */
#define LL_DMAMUX_REQ_TIM17_CH1           0x0000002FU  /*!< DMAMUX TIM17 CH1 request    */
#define LL_DMAMUX_REQ_TIM17_TRIG_COM      0x00000030U  /*!< DMAMUX TIM17 TRIG COM request */
#define LL_DMAMUX_REQ_TIM17_UP            0x00000031U  /*!< DMAMUX TIM17 UP request    */
#define LL_DMAMUX_REQ_USART1_RX           0x00000032U  /*!< DMAMUX USART1 RX request  */
#define LL_DMAMUX_REQ_USART1_TX           0x00000033U  /*!< DMAMUX USART1 TX request  */
#define LL_DMAMUX_REQ_USART2_RX           0x00000034U  /*!< DMAMUX USART2 RX request  */
#define LL_DMAMUX_REQ_USART2_TX           0x00000035U  /*!< DMAMUX USART2 TX request  */
#if defined(USART3)
#define LL_DMAMUX_REQ_USART3_RX           0x00000036U  /*!< DMAMUX USART3 RX request  */
#define LL_DMAMUX_REQ_USART3_TX           0x00000037U  /*!< DMAMUX USART3 TX request  */
#endif
#if defined(USART4)
#define LL_DMAMUX_REQ_USART4_RX           0x00000038U  /*!< DMAMUX USART4 RX request  */
#define LL_DMAMUX_REQ_USART4_TX           0x00000039U  /*!< DMAMUX USART4 TX request  */
#endif
#if defined(UCPD1)
#define LL_DMAMUX_REQ_UCPD1_RX            0x0000003AU  /*!< DMAMUX UCPD1 RX request  */
#define LL_DMAMUX_REQ_UCPD1_TX            0x0000003BU  /*!< DMAMUX UCPD1 TX request  */
#endif
#if defined(UCPD2)
#define LL_DMAMUX_REQ_UCPD2_RX            0x0000003CU  /*!< DMAMUX UCPD2 RX request  */
#define LL_DMAMUX_REQ_UCPD2_TX            0x0000003DU  /*!< DMAMUX UCPD2 TX request  */
#endif
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_CHANNEL DMAMUX Channel
  * @{
  */
#define LL_DMAMUX_CHANNEL_0               0x00000000U               /*!< DMAMUX Channel 0 connected to DMA1 Channel 1  */
#define LL_DMAMUX_CHANNEL_1               0x00000001U               /*!< DMAMUX Channel 1 connected to DMA1 Channel 2  */
#define LL_DMAMUX_CHANNEL_2               0x00000002U               /*!< DMAMUX Channel 2 connected to DMA1 Channel 3  */
#define LL_DMAMUX_CHANNEL_3               0x00000003U               /*!< DMAMUX Channel 3 connected to DMA1 Channel 4  */
#define LL_DMAMUX_CHANNEL_4               0x00000004U               /*!< DMAMUX Channel 4 connected to DMA1 Channel 5  */
#if defined(DMAMUX1_Channel5)
#define LL_DMAMUX_CHANNEL_5               0x00000005U               /*!< DMAMUX Channel 5 connected to DMA1 Channel 6  */
#endif
#if defined(DMAMUX1_Channel6)
#define LL_DMAMUX_CHANNEL_6               0x00000006U               /*!< DMAMUX Channel 6 connected to DMA1 Channel 7  */
#endif
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_SYNC_NO Synchronization Signal Polarity
  * @{
  */
#define LL_DMAMUX_SYNC_NO_EVENT            0x00000000U                               /*!< All requests are blocked   */
#define LL_DMAMUX_SYNC_POL_RISING          DMAMUX_CxCR_SPOL_0                        /*!< Synchronization on event on rising edge */
#define LL_DMAMUX_SYNC_POL_FALLING         DMAMUX_CxCR_SPOL_1                        /*!< Synchronization on event on falling edge */
#define LL_DMAMUX_SYNC_POL_RISING_FALLING  (DMAMUX_CxCR_SPOL_0 | DMAMUX_CxCR_SPOL_1) /*!< Synchronization on event on rising and falling edge */
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_SYNC_EVT Synchronization Signal Event
  * @{
  */
#define LL_DMAMUX_SYNC_EXTI_LINE0         0x00000000U                                                                                     /*!< Synchronization signal from EXTI Line0  */
#define LL_DMAMUX_SYNC_EXTI_LINE1         DMAMUX_CxCR_SYNC_ID_0                                                                           /*!< Synchronization signal from EXTI Line1  */
#define LL_DMAMUX_SYNC_EXTI_LINE2         DMAMUX_CxCR_SYNC_ID_1                                                                           /*!< Synchronization signal from EXTI Line2  */
#define LL_DMAMUX_SYNC_EXTI_LINE3         (DMAMUX_CxCR_SYNC_ID_1 | DMAMUX_CxCR_SYNC_ID_0)                                                 /*!< Synchronization signal from EXTI Line3  */
#define LL_DMAMUX_SYNC_EXTI_LINE4         DMAMUX_CxCR_SYNC_ID_2                                                                           /*!< Synchronization signal from EXTI Line4  */
#define LL_DMAMUX_SYNC_EXTI_LINE5         (DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_0)                                                 /*!< Synchronization signal from EXTI Line5  */
#define LL_DMAMUX_SYNC_EXTI_LINE6         (DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_1)                                                 /*!< Synchronization signal from EXTI Line6  */
#define LL_DMAMUX_SYNC_EXTI_LINE7         (DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_1 | DMAMUX_CxCR_SYNC_ID_0)                         /*!< Synchronization signal from EXTI Line7  */
#define LL_DMAMUX_SYNC_EXTI_LINE8         DMAMUX_CxCR_SYNC_ID_3                                                                           /*!< Synchronization signal from EXTI Line8  */
#define LL_DMAMUX_SYNC_EXTI_LINE9         (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_0)                                                 /*!< Synchronization signal from EXTI Line9  */
#define LL_DMAMUX_SYNC_EXTI_LINE10        (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_1)                                                 /*!< Synchronization signal from EXTI Line10  */
#define LL_DMAMUX_SYNC_EXTI_LINE11        (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_1 | DMAMUX_CxCR_SYNC_ID_0)                         /*!< Synchronization signal from EXTI Line11  */
#define LL_DMAMUX_SYNC_EXTI_LINE12        (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_2)                                                 /*!< Synchronization signal from EXTI Line12  */
#define LL_DMAMUX_SYNC_EXTI_LINE13        (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_0)                         /*!< Synchronization signal from EXTI Line1 3 */
#define LL_DMAMUX_SYNC_EXTI_LINE14        (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_1)                         /*!< Synchronization signal from EXTI Line1 4 */
#define LL_DMAMUX_SYNC_EXTI_LINE15        (DMAMUX_CxCR_SYNC_ID_3 | DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_1 | DMAMUX_CxCR_SYNC_ID_0) /*!< Synchronization signal from EXTI Line1 5 */
#define LL_DMAMUX_SYNC_DMAMUX_CH0         DMAMUX_CxCR_SYNC_ID_4                                                                           /*!< Synchronization signal from DMAMUX channel0 Event */
#define LL_DMAMUX_SYNC_DMAMUX_CH1         (DMAMUX_CxCR_SYNC_ID_4 | DMAMUX_CxCR_SYNC_ID_0)                                                 /*!< Synchronization signal from DMAMUX channel1 Event */
#define LL_DMAMUX_SYNC_DMAMUX_CH2         (DMAMUX_CxCR_SYNC_ID_4 | DMAMUX_CxCR_SYNC_ID_1)                                                 /*!< Synchronization signal from DMAMUX channel2 Event */
#define LL_DMAMUX_SYNC_DMAMUX_CH3         (DMAMUX_CxCR_SYNC_ID_4 | DMAMUX_CxCR_SYNC_ID_1 | DMAMUX_CxCR_SYNC_ID_0)                         /*!< Synchronization signal from DMAMUX channel3 Event */
#if defined(LPTIM1)
#define LL_DMAMUX_SYNC_LPTIM1_OUT         (DMAMUX_CxCR_SYNC_ID_4 | DMAMUX_CxCR_SYNC_ID_2)                                                 /*!< Synchronization signal from LPTIM1 Ouput */
#endif
#if defined(LPTIM2)
#define LL_DMAMUX_SYNC_LPTIM2_OUT         (DMAMUX_CxCR_SYNC_ID_4 | DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_0)                         /*!< Synchronization signal from LPTIM2 Ouput */
#endif
#define LL_DMAMUX_SYNC_TIM14_OC           (DMAMUX_CxCR_SYNC_ID_4 | DMAMUX_CxCR_SYNC_ID_2 | DMAMUX_CxCR_SYNC_ID_1)                         /*!< Synchronization signal from TIM14 OC */
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_REQUEST_GENERATOR Request Generator Channel
  * @{
  */
#define LL_DMAMUX_REQ_GEN_0               0x00000000U
#define LL_DMAMUX_REQ_GEN_1               0x00000001U
#define LL_DMAMUX_REQ_GEN_2               0x00000002U
#define LL_DMAMUX_REQ_GEN_3               0x00000003U
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_REQUEST_GEN_POLARITY External Request Signal Generation Polarity
  * @{
  */
#define LL_DMAMUX_REQ_GEN_NO_EVENT             0x00000000U                                  /*!< No external DMA request  generation */
#define LL_DMAMUX_REQ_GEN_POL_RISING           DMAMUX_RGxCR_GPOL_0                          /*!< External DMA request generation on event on rising edge */
#define LL_DMAMUX_REQ_GEN_POL_FALLING          DMAMUX_RGxCR_GPOL_1                          /*!< External DMA request generation on event on falling edge */
#define LL_DMAMUX_REQ_GEN_POL_RISING_FALLING   (DMAMUX_RGxCR_GPOL_0 | DMAMUX_RGxCR_GPOL_1)  /*!< External DMA request generation on rising and falling edge */
/**
  * @}
  */

/** @defgroup DMAMUX_LL_EC_REQUEST_GEN External Request Signal Generation
  * @{
  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE0      0x00000000U                                                                                     /*!< Request signal generation from EXTI Line0  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE1      DMAMUX_RGxCR_SIG_ID_0                                                                           /*!< Request signal generation from EXTI Line1  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE2      DMAMUX_RGxCR_SIG_ID_1                                                                           /*!< Request signal generation from EXTI Line2  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE3      (DMAMUX_RGxCR_SIG_ID_1 |DMAMUX_RGxCR_SIG_ID_0)                                                  /*!< Request signal generation from EXTI Line3  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE4      DMAMUX_RGxCR_SIG_ID_2                                                                           /*!< Request signal generation from EXTI Line4  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE5      (DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_0)                                                 /*!< Request signal generation from EXTI Line5  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE6      (DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_1)                                                 /*!< Request signal generation from EXTI Line6  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE7      (DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_1 | DMAMUX_RGxCR_SIG_ID_0)                         /*!< Request signal generation from EXTI Line7  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE8      DMAMUX_RGxCR_SIG_ID_3                                                                           /*!< Request signal generation from EXTI Line8  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE9      (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_0)                                                 /*!< Request signal generation from EXTI Line9  */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE10     (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_1)                                                 /*!< Request signal generation from EXTI Line10 */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE11     (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_1 | DMAMUX_RGxCR_SIG_ID_0)                         /*!< Request signal generation from EXTI Line11 */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE12     (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_2)                                                 /*!< Request signal generation from EXTI Line12 */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE13     (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_0)                         /*!< Request signal generation from EXTI Line13 */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE14     (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_1)                         /*!< Request signal generation from EXTI Line14 */
#define LL_DMAMUX_REQ_GEN_EXTI_LINE15     (DMAMUX_RGxCR_SIG_ID_3 | DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_1 | DMAMUX_RGxCR_SIG_ID_0) /*!< Request signal generation from EXTI Line15 */
#define LL_DMAMUX_REQ_GEN_DMAMUX_CH0      DMAMUX_RGxCR_SIG_ID_4                                                                           /*!< Request signal generation from DMAMUX channel0 Event */
#define LL_DMAMUX_REQ_GEN_DMAMUX_CH1      (DMAMUX_RGxCR_SIG_ID_4 | DMAMUX_RGxCR_SIG_ID_0)                                                 /*!< Request signal generation from DMAMUX channel1 Event */
#define LL_DMAMUX_REQ_GEN_DMAMUX_CH2      (DMAMUX_RGxCR_SIG_ID_4 | DMAMUX_RGxCR_SIG_ID_1)                                                 /*!< Request signal generation from DMAMUX channel2 Event */
#define LL_DMAMUX_REQ_GEN_DMAMUX_CH3      (DMAMUX_RGxCR_SIG_ID_4 | DMAMUX_RGxCR_SIG_ID_1 | DMAMUX_RGxCR_SIG_ID_0)                         /*!< Request signal generation from DMAMUX channel3 Event */
#if defined(LPTIM1)
#define LL_DMAMUX_REQ_GEN_LPTIM1_OUT      (DMAMUX_RGxCR_SIG_ID_4 | DMAMUX_RGxCR_SIG_ID_2)                                                 /*!< Request signal generation from LPTIM1 Ouput */
#endif
#if defined(LPTIM2)
#define LL_DMAMUX_REQ_GEN_LPTIM2_OUT      (DMAMUX_RGxCR_SIG_ID_4 | DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_0)                         /*!< Request signal generation from LPTIM2 Ouput */
#endif
#define LL_DMAMUX_REQ_GEN_TIM14_OC        (DMAMUX_RGxCR_SIG_ID_4 | DMAMUX_RGxCR_SIG_ID_2 | DMAMUX_RGxCR_SIG_ID_1)                         /*!< Request signal generation from TIM14 OC */
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup DMAMUX_LL_Exported_Macros DMAMUX Exported Macros
  * @{
  */

/** @defgroup DMAMUX_LL_EM_WRITE_READ Common Write and read registers macros
  * @{
  */
/**
  * @brief  Write a value in DMAMUX register
  * @param  __INSTANCE__ DMAMUX Instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_DMAMUX_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG(__INSTANCE__->__REG__, (__VALUE__))

/**
  * @brief  Read a value in DMAMUX register
  * @param  __INSTANCE__ DMAMUX Instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_DMAMUX_ReadReg(__INSTANCE__, __REG__) READ_REG(__INSTANCE__->__REG__)
/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup DMAMUX_LL_Exported_Functions DMAMUX Exported Functions
 * @{
 */

/** @defgroup DMAMUX_LL_EF_Configuration Configuration
  * @{
  */
/**
  * @brief  Set DMAMUX request ID for DMAMUX Channel x.
  * @note   DMAMUX channel 0 to 6 are mapped to DMA1 channel 1 to 7.
  * @rmtoll CxCR         DMAREQ_ID     LL_DMAMUX_SetRequestID
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @param  Request This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_MEM2MEM
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR0
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR1
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR2
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR3
  *         @arg @ref LL_DMAMUX_REQ_ADC1
  *         @arg @ref LL_DMAMUX_REQ_AES_IN
  *         @arg @ref LL_DMAMUX_REQ_AES_OUT
  *         @arg @ref LL_DMAMUX_REQ_DAC1_CH1
  *         @arg @ref LL_DMAMUX_REQ_DAC1_CH2
  *         @arg @ref LL_DMAMUX_REQ_I2C1_RX
  *         @arg @ref LL_DMAMUX_REQ_I2C1_TX
  *         @arg @ref LL_DMAMUX_REQ_I2C2_RX
  *         @arg @ref LL_DMAMUX_REQ_I2C2_TX
  *         @arg @ref LL_DMAMUX_REQ_LPUART1_RX
  *         @arg @ref LL_DMAMUX_REQ_LPUART1_TX
  *         @arg @ref LL_DMAMUX_REQ_SPI1_RX
  *         @arg @ref LL_DMAMUX_REQ_SPI1_TX
  *         @arg @ref LL_DMAMUX_REQ_SPI2_RX
  *         @arg @ref LL_DMAMUX_REQ_SPI2_TX
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH3
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH4
  *         @arg @ref LL_DMAMUX_REQ_TIM1_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM1_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH3
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH4
  *         @arg @ref LL_DMAMUX_REQ_TIM2_TRIG
  *         @arg @ref LL_DMAMUX_REQ_TIM2_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH3
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH4
  *         @arg @ref LL_DMAMUX_REQ_TIM3_TRIG
  *         @arg @ref LL_DMAMUX_REQ_TIM3_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM6_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM7_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM15_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM15_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM15_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM15_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM16_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM16_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM16_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM17_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM17_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM17_UP
  *         @arg @ref LL_DMAMUX_REQ_USART1_RX
  *         @arg @ref LL_DMAMUX_REQ_USART1_TX
  *         @arg @ref LL_DMAMUX_REQ_USART2_RX
  *         @arg @ref LL_DMAMUX_REQ_USART2_TX
  *         @arg @ref LL_DMAMUX_REQ_USART3_RX
  *         @arg @ref LL_DMAMUX_REQ_USART3_TX
  *         @arg @ref LL_DMAMUX_REQ_USART4_RX
  *         @arg @ref LL_DMAMUX_REQ_USART4_TX
  *         @arg @ref LL_DMAMUX_REQ_UCPD1_RX
  *         @arg @ref LL_DMAMUX_REQ_UCPD1_TX
  *         @arg @ref LL_DMAMUX_REQ_UCPD2_RX
  *         @arg @ref LL_DMAMUX_REQ_UCPD2_TX
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetRequestID(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel, uint32_t Request)
{
  MODIFY_REG(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_DMAREQ_ID, Request);
}

/**
  * @brief  Get DMAMUX request ID for DMAMUX Channel x.
  * @note   DMAMUX channel 0 to 6 are mapped to DMA1 channel 1 to 7.
  * @rmtoll CxCR         DMAREQ_ID     LL_DMAMUX_GetRequestID
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_MEM2MEM
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR0
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR1
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR2
  *         @arg @ref LL_DMAMUX_REQ_GENERATOR3
  *         @arg @ref LL_DMAMUX_REQ_ADC1
  *         @arg @ref LL_DMAMUX_REQ_AES_IN
  *         @arg @ref LL_DMAMUX_REQ_AES_OUT
  *         @arg @ref LL_DMAMUX_REQ_DAC1_CH1
  *         @arg @ref LL_DMAMUX_REQ_DAC1_CH2
  *         @arg @ref LL_DMAMUX_REQ_I2C1_RX
  *         @arg @ref LL_DMAMUX_REQ_I2C1_TX
  *         @arg @ref LL_DMAMUX_REQ_I2C2_RX
  *         @arg @ref LL_DMAMUX_REQ_I2C2_TX
  *         @arg @ref LL_DMAMUX_REQ_LPUART1_RX
  *         @arg @ref LL_DMAMUX_REQ_LPUART1_TX
  *         @arg @ref LL_DMAMUX_REQ_SPI1_RX
  *         @arg @ref LL_DMAMUX_REQ_SPI1_TX
  *         @arg @ref LL_DMAMUX_REQ_SPI2_RX
  *         @arg @ref LL_DMAMUX_REQ_SPI2_TX
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH3
  *         @arg @ref LL_DMAMUX_REQ_TIM1_CH4
  *         @arg @ref LL_DMAMUX_REQ_TIM1_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM1_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH3
  *         @arg @ref LL_DMAMUX_REQ_TIM2_CH4
  *         @arg @ref LL_DMAMUX_REQ_TIM2_TRIG
  *         @arg @ref LL_DMAMUX_REQ_TIM2_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH3
  *         @arg @ref LL_DMAMUX_REQ_TIM3_CH4
  *         @arg @ref LL_DMAMUX_REQ_TIM3_TRIG
  *         @arg @ref LL_DMAMUX_REQ_TIM3_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM6_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM7_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM15_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM15_CH2
  *         @arg @ref LL_DMAMUX_REQ_TIM15_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM15_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM16_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM16_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM16_UP
  *         @arg @ref LL_DMAMUX_REQ_TIM17_CH1
  *         @arg @ref LL_DMAMUX_REQ_TIM17_TRIG_COM
  *         @arg @ref LL_DMAMUX_REQ_TIM17_UP
  *         @arg @ref LL_DMAMUX_REQ_USART1_RX
  *         @arg @ref LL_DMAMUX_REQ_USART1_TX
  *         @arg @ref LL_DMAMUX_REQ_USART2_RX
  *         @arg @ref LL_DMAMUX_REQ_USART2_TX
  *         @arg @ref LL_DMAMUX_REQ_USART3_RX
  *         @arg @ref LL_DMAMUX_REQ_USART3_TX
  *         @arg @ref LL_DMAMUX_REQ_USART4_RX
  *         @arg @ref LL_DMAMUX_REQ_USART4_TX
  *         @arg @ref LL_DMAMUX_REQ_UCPD1_RX
  *         @arg @ref LL_DMAMUX_REQ_UCPD1_TX
  *         @arg @ref LL_DMAMUX_REQ_UCPD2_RX
  *         @arg @ref LL_DMAMUX_REQ_UCPD2_TX
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetRequestID(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return (uint32_t)(READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_DMAREQ_ID));
}

/**
  * @brief  Set the number of DMA request that will be autorized after a synchronization event and/or the number of DMA request needed to generate an event.
  * @rmtoll CxCR         NBREQ         LL_DMAMUX_SetSyncRequestNb
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @param  RequestNb This parameter must be a value between Min_Data = 1 and Max_Data = 32.
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetSyncRequestNb(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel, uint32_t RequestNb)
{
  MODIFY_REG(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_NBREQ, ((RequestNb - 1U) << DMAMUX_CxCR_NBREQ_Pos));
}

/**
  * @brief  Get the number of DMA request that will be autorized after a synchronization event and/or the number of DMA request needed to generate an event.
  * @rmtoll CxCR         NBREQ         LL_DMAMUX_GetSyncRequestNb
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval Between Min_Data = 1 and Max_Data = 32
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetSyncRequestNb(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return (uint32_t)(((READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_NBREQ)) >> DMAMUX_CxCR_NBREQ_Pos) + 1U);
}

/**
  * @brief  Set the polarity of the signal on which the DMA request is synchronized.
  * @rmtoll CxCR         SPOL          LL_DMAMUX_SetSyncPolarity
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @param  Polarity This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_SYNC_NO_EVENT
  *         @arg @ref LL_DMAMUX_SYNC_POL_RISING
  *         @arg @ref LL_DMAMUX_SYNC_POL_FALLING
  *         @arg @ref LL_DMAMUX_SYNC_POL_RISING_FALLING
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetSyncPolarity(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel, uint32_t Polarity)
{
  MODIFY_REG(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SPOL, Polarity);
}

/**
  * @brief  Get the polarity of the signal on which the DMA request is synchronized.
  * @rmtoll CxCR         SPOL          LL_DMAMUX_GetSyncPolarity
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DMAMUX_SYNC_NO_EVENT
  *         @arg @ref LL_DMAMUX_SYNC_POL_RISING
  *         @arg @ref LL_DMAMUX_SYNC_POL_FALLING
  *         @arg @ref LL_DMAMUX_SYNC_POL_RISING_FALLING
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetSyncPolarity(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return (uint32_t)(READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SPOL));
}

/**
  * @brief  Enable the Event Generation on DMAMUX channel x.
  * @rmtoll CxCR         EGE           LL_DMAMUX_EnableEventGeneration
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_EnableEventGeneration(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  SET_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_EGE);
}

/**
  * @brief  Disable the Event Generation on DMAMUX channel x.
  * @rmtoll CxCR         EGE           LL_DMAMUX_DisableEventGeneration
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_DisableEventGeneration(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  CLEAR_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_EGE);
}

/**
  * @brief  Check if the Event Generation on DMAMUX channel x is enabled or disabled.
  * @rmtoll CxCR         EGE           LL_DMAMUX_IsEnabledEventGeneration
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsEnabledEventGeneration(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return ((READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_EGE) == (DMAMUX_CxCR_EGE))? 1UL : 0UL);
}

/**
  * @brief  Enable the synchronization mode.
  * @rmtoll CxCR         SE            LL_DMAMUX_EnableSync
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_EnableSync(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  SET_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SE);
}

/**
  * @brief  Disable the synchronization mode.
  * @rmtoll CxCR         SE            LL_DMAMUX_DisableSync
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_DisableSync(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  CLEAR_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SE);
}

/**
  * @brief  Check if the synchronization mode is enabled or disabled.
  * @rmtoll CxCR         SE            LL_DMAMUX_IsEnabledSync
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsEnabledSync(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return ((READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SE) == (DMAMUX_CxCR_SE))? 1UL : 0UL);
}

/**
  * @brief  Set DMAMUX synchronization ID  on DMAMUX Channel x.
  * @rmtoll CxCR         SYNC_ID       LL_DMAMUX_SetSyncID
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @param  SyncID This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE0
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE1
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE2
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE3
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE4
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE5
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE6
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE7
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE8
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE9
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE10
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE11
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE12
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE13
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE14
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE15
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH0
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH1
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH2
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH3
  *         @arg @ref LL_DMAMUX_SYNC_LPTIM1_OUT
  *         @arg @ref LL_DMAMUX_SYNC_LPTIM2_OUT
  *         @arg @ref LL_DMAMUX_SYNC_TIM14_OC
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetSyncID(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel, uint32_t SyncID)
{
  MODIFY_REG(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SYNC_ID, SyncID);
}

/**
  * @brief  Get DMAMUX synchronization ID  on DMAMUX Channel x.
  * @rmtoll CxCR         SYNC_ID       LL_DMAMUX_GetSyncID
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE0
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE1
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE2
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE3
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE4
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE5
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE6
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE7
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE8
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE9
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE10
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE11
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE12
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE13
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE14
  *         @arg @ref LL_DMAMUX_SYNC_EXTI_LINE15
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH0
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH1
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH2
  *         @arg @ref LL_DMAMUX_SYNC_DMAMUX_CH3
  *         @arg @ref LL_DMAMUX_SYNC_LPTIM1_OUT
  *         @arg @ref LL_DMAMUX_SYNC_LPTIM2_OUT
  *         @arg @ref LL_DMAMUX_SYNC_TIM14_OC
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetSyncID(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return (uint32_t)(READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SYNC_ID));
}

/**
  * @brief  Enable the Request Generator.
  * @rmtoll RGxCR        GE            LL_DMAMUX_EnableRequestGen
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_EnableRequestGen(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  SET_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GE);
}

/**
  * @brief  Disable the Request Generator.
  * @rmtoll RGxCR        GE            LL_DMAMUX_DisableRequestGen
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_DisableRequestGen(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  CLEAR_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GE);
}

/**
  * @brief  Check if the Request Generator is enabled or disabled.
  * @rmtoll RGxCR        GE            LL_DMAMUX_IsEnabledRequestGen
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsEnabledRequestGen(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  return ((READ_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GE) == (DMAMUX_RGxCR_GE))? 1UL : 0UL);
}

/**
  * @brief  Set the polarity of the signal on which the DMA request is generated.
  * @rmtoll RGxCR        GPOL          LL_DMAMUX_SetRequestGenPolarity
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @param  Polarity This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_NO_EVENT
  *         @arg @ref LL_DMAMUX_REQ_GEN_POL_RISING
  *         @arg @ref LL_DMAMUX_REQ_GEN_POL_FALLING
  *         @arg @ref LL_DMAMUX_REQ_GEN_POL_RISING_FALLING
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetRequestGenPolarity(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel, uint32_t Polarity)
{
  MODIFY_REG(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GPOL, Polarity);
}

/**
  * @brief  Get the polarity of the signal on which the DMA request is generated.
  * @rmtoll RGxCR        GPOL          LL_DMAMUX_GetRequestGenPolarity
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_NO_EVENT
  *         @arg @ref LL_DMAMUX_REQ_GEN_POL_RISING
  *         @arg @ref LL_DMAMUX_REQ_GEN_POL_FALLING
  *         @arg @ref LL_DMAMUX_REQ_GEN_POL_RISING_FALLING
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetRequestGenPolarity(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  return (uint32_t)(READ_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GPOL));
}

/**
  * @brief  Set the number of DMA request that will be autorized after a generation event.
  * @note   This field can only be written when Generator is disabled.
  * @rmtoll RGxCR        GNBREQ        LL_DMAMUX_SetGenRequestNb
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @param  RequestNb This parameter must be a value between Min_Data = 1 and Max_Data = 32.
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetGenRequestNb(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel, uint32_t RequestNb)
{
  MODIFY_REG(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GNBREQ, (RequestNb - 1U) << DMAMUX_RGxCR_GNBREQ_Pos);
}

/**
  * @brief  Get the number of DMA request that will be autorized after a generation event.
  * @rmtoll RGxCR        GNBREQ        LL_DMAMUX_GetGenRequestNb
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval Between Min_Data = 1 and Max_Data = 32
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetGenRequestNb(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  return (uint32_t)((READ_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_GNBREQ) >> DMAMUX_RGxCR_GNBREQ_Pos) + 1U);
}

/**
  * @brief  Set DMAMUX external Request Signal ID on DMAMUX Request Generation Trigger Event Channel x.
  * @rmtoll RGxCR        SIG_ID        LL_DMAMUX_SetRequestSignalID
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @param  RequestSignalID This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE0
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE1
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE2
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE3
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE4
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE5
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE6
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE7
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE8
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE9
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE10
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE11
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE12
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE13
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE14
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE15
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH0
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH1
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH2
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH3
  *         @arg @ref LL_DMAMUX_REQ_GEN_LPTIM1_OUT
  *         @arg @ref LL_DMAMUX_REQ_GEN_LPTIM2_OUT
  *         @arg @ref LL_DMAMUX_REQ_GEN_TIM14_OC
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_SetRequestSignalID(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel, uint32_t RequestSignalID)
{
  MODIFY_REG(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_SIG_ID, RequestSignalID);
}

/**
  * @brief  Get DMAMUX external Request Signal ID set on DMAMUX Channel x.
  * @rmtoll RGxCR        SIG_ID        LL_DMAMUX_GetRequestSignalID
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE0
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE1
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE2
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE3
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE4
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE5
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE6
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE7
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE8
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE9
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE10
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE11
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE12
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE13
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE14
  *         @arg @ref LL_DMAMUX_REQ_GEN_EXTI_LINE15
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH0
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH1
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH2
  *         @arg @ref LL_DMAMUX_REQ_GEN_DMAMUX_CH3
  *         @arg @ref LL_DMAMUX_REQ_GEN_LPTIM1_OUT
  *         @arg @ref LL_DMAMUX_REQ_GEN_LPTIM2_OUT
  *         @arg @ref LL_DMAMUX_REQ_GEN_TIM14_OC
  */
__STATIC_INLINE uint32_t LL_DMAMUX_GetRequestSignalID(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  return (uint32_t)(READ_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_SIG_ID));
}

/**
  * @}
  */

/** @defgroup DMAMUX_LL_EF_FLAG_Management FLAG_Management
  * @{
  */

/**
  * @brief  Get Synchronization Event Overrun Flag Channel 0.
  * @rmtoll CSR          SOF0          LL_DMAMUX_IsActiveFlag_SO0
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO0(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF0) == (DMAMUX_CSR_SOF0)) ? 1UL : 0UL);
}

/**
  * @brief  Get Synchronization Event Overrun Flag Channel 1.
  * @rmtoll CSR          SOF1          LL_DMAMUX_IsActiveFlag_SO1
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO1(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF1) == (DMAMUX_CSR_SOF1)) ? 1UL : 0UL);;
}

/**
  * @brief  Get Synchronization Event Overrun Flag Channel 2.
  * @rmtoll CSR          SOF2          LL_DMAMUX_IsActiveFlag_SO2
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO2(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF2) == (DMAMUX_CSR_SOF2)) ? 1UL : 0UL);;
}

/**
  * @brief  Get Synchronization Event Overrun Flag Channel 3.
  * @rmtoll CSR          SOF3          LL_DMAMUX_IsActiveFlag_SO3
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO3(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF3) == (DMAMUX_CSR_SOF3)) ? 1UL : 0UL);;
}

/**
  * @brief  Get Synchronization Event Overrun Flag Channel 4.
  * @rmtoll CSR          SOF4          LL_DMAMUX_IsActiveFlag_SO4
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO4(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF4) == (DMAMUX_CSR_SOF4)) ? 1UL : 0UL);;
}

#if defined(DMAMUX1_Channel5)
/**
  * @brief  Get Synchronization Event Overrun Flag Channel 5.
  * @rmtoll CSR          SOF5          LL_DMAMUX_IsActiveFlag_SO5
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO5(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF5) == (DMAMUX_CSR_SOF5)) ? 1UL : 0UL);;
}
#endif
#if defined(DMAMUX1_Channel6)
/**
  * @brief  Get Synchronization Event Overrun Flag Channel 6.
  * @rmtoll CSR          SOF6          LL_DMAMUX_IsActiveFlag_SO6
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_SO6(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_ChannelStatus->CSR, DMAMUX_CSR_SOF6) == (DMAMUX_CSR_SOF6)) ? 1UL : 0UL);
}
#endif

/**
  * @brief  Get Request Generator 0 Trigger Event Overrun Flag.
  * @rmtoll RGSR         OF0           LL_DMAMUX_IsActiveFlag_RGO0
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_RGO0(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_RequestGenStatus->RGSR, DMAMUX_RGSR_OF0) == (DMAMUX_RGSR_OF0)) ? 1UL : 0UL);;
}

/**
  * @brief  Get Request Generator 1 Trigger Event Overrun Flag.
  * @rmtoll RGSR         OF1           LL_DMAMUX_IsActiveFlag_RGO1
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_RGO1(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_RequestGenStatus->RGSR, DMAMUX_RGSR_OF1) == (DMAMUX_RGSR_OF1)) ? 1UL : 0UL);;
}

/**
  * @brief  Get Request Generator 2 Trigger Event Overrun Flag.
  * @rmtoll RGSR         OF2           LL_DMAMUX_IsActiveFlag_RGO2
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_RGO2(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_RequestGenStatus->RGSR, DMAMUX_RGSR_OF2) == (DMAMUX_RGSR_OF2)) ? 1UL : 0UL);;
}

/**
  * @brief  Get Request Generator 3 Trigger Event Overrun Flag.
  * @rmtoll RGSR         OF3           LL_DMAMUX_IsActiveFlag_RGO3
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsActiveFlag_RGO3(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  return ((READ_BIT(DMAMUX1_RequestGenStatus->RGSR, DMAMUX_RGSR_OF3) == (DMAMUX_RGSR_OF3)) ? 1UL : 0UL);;
}

/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 0.
  * @rmtoll CFR          CSOF0         LL_DMAMUX_ClearFlag_SO0
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO0(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF0);
}

/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 1.
  * @rmtoll CFR          CSOF1         LL_DMAMUX_ClearFlag_SO1
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO1(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF1);
}

/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 2.
  * @rmtoll CFR          CSOF2         LL_DMAMUX_ClearFlag_SO2
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO2(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF2);
}

/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 3.
  * @rmtoll CFR          CSOF3         LL_DMAMUX_ClearFlag_SO3
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO3(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF3);
}

/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 4.
  * @rmtoll CFR          CSOF4         LL_DMAMUX_ClearFlag_SO4
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO4(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF4);
}

#if defined(DMAMUX1_Channel5)
/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 5.
  * @rmtoll CFR          CSOF5         LL_DMAMUX_ClearFlag_SO5
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO5(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF5);
}
#endif
#if defined(DMAMUX1_Channel6)
/**
  * @brief  Clear Synchronization Event Overrun Flag Channel 6.
  * @rmtoll CFR          CSOF6         LL_DMAMUX_ClearFlag_SO6
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_SO6(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_ChannelStatus->CFR, DMAMUX_CFR_CSOF6);
}
#endif

/**
  * @brief  Clear Request Generator 0 Trigger Event Overrun Flag.
  * @rmtoll RGCFR        COF0          LL_DMAMUX_ClearFlag_RGO0
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_RGO0(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_RequestGenStatus->RGCFR, DMAMUX_RGCFR_COF0);
}

/**
  * @brief  Clear Request Generator 1 Trigger Event Overrun Flag.
  * @rmtoll RGCFR        COF1          LL_DMAMUX_ClearFlag_RGO1
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_RGO1(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_RequestGenStatus->RGCFR, DMAMUX_RGCFR_COF1);
}

/**
  * @brief  Clear Request Generator 2 Trigger Event Overrun Flag.
  * @rmtoll RGCFR        COF2          LL_DMAMUX_ClearFlag_RGO2
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_RGO2(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_RequestGenStatus->RGCFR, DMAMUX_RGCFR_COF2);
}

/**
  * @brief  Clear Request Generator 3 Trigger Event Overrun Flag.
  * @rmtoll RGCFR        COF3          LL_DMAMUX_ClearFlag_RGO3
  * @param  DMAMUXx DMAMUXx DMAMUXx Instance
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_ClearFlag_RGO3(DMAMUX_Channel_TypeDef *DMAMUXx)
{
  SET_BIT(DMAMUX1_RequestGenStatus->RGCFR, DMAMUX_RGCFR_COF3);
}

/**
  * @}
  */

/** @defgroup DMAMUX_LL_EF_IT_Management IT_Management
  * @{
  */

/**
  * @brief  Enable the Synchronization Event Overrun Interrupt on DMAMUX channel x.
  * @rmtoll CxCR         SOIE          LL_DMAMUX_EnableIT_SO
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_EnableIT_SO(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  SET_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SOIE);
}

/**
  * @brief  Disable the Synchronization Event Overrun Interrupt on DMAMUX channel x.
  * @rmtoll CxCR         SOIE          LL_DMAMUX_DisableIT_SO
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_DisableIT_SO(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  CLEAR_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SOIE);
}

/**
  * @brief  Check if the Synchronization Event Overrun Interrupt on DMAMUX channel x is enabled or disabled.
  * @rmtoll CxCR         SOIE          LL_DMAMUX_IsEnabledIT_SO
  * @param  DMAMUXx DMAMUXx Instance
  * @param  Channel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_CHANNEL_0
  *         @arg @ref LL_DMAMUX_CHANNEL_1
  *         @arg @ref LL_DMAMUX_CHANNEL_2
  *         @arg @ref LL_DMAMUX_CHANNEL_3
  *         @arg @ref LL_DMAMUX_CHANNEL_4
  *         @arg @ref LL_DMAMUX_CHANNEL_5
  *         @arg @ref LL_DMAMUX_CHANNEL_6
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsEnabledIT_SO(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t Channel)
{
  return (((READ_BIT(((DMAMUX_Channel_TypeDef *)((uint32_t)((uint32_t)DMAMUXx + (DMAMUX_CCR_SIZE * (Channel)))))->CCR, DMAMUX_CxCR_SOIE)) == (DMAMUX_CxCR_SOIE))? 1UL : 0UL);
}

/**
  * @brief  Enable the Request Generation Trigger Event Overrun Interrupt on DMAMUX channel x.
  * @rmtoll RGxCR        OIE           LL_DMAMUX_EnableIT_RGO
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_EnableIT_RGO(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  SET_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_OIE);
}

/**
  * @brief  Disable the Request Generation Trigger Event Overrun Interrupt on DMAMUX channel x.
  * @rmtoll RGxCR        OIE           LL_DMAMUX_DisableIT_RGO
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval None
  */
__STATIC_INLINE void LL_DMAMUX_DisableIT_RGO(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  CLEAR_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_OIE);
}

/**
  * @brief  Check if the Request Generation Trigger Event Overrun Interrupt on DMAMUX channel x is enabled or disabled.
  * @rmtoll RGxCR        OIE           LL_DMAMUX_IsEnabledIT_RGO
  * @param  DMAMUXx DMAMUXx Instance
  * @param  RequestGenChannel This parameter can be one of the following values:
  *         @arg @ref LL_DMAMUX_REQ_GEN_0
  *         @arg @ref LL_DMAMUX_REQ_GEN_1
  *         @arg @ref LL_DMAMUX_REQ_GEN_2
  *         @arg @ref LL_DMAMUX_REQ_GEN_3
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_DMAMUX_IsEnabledIT_RGO(DMAMUX_Channel_TypeDef *DMAMUXx, uint32_t RequestGenChannel)
{
  return ((READ_BIT(((DMAMUX_RequestGen_TypeDef *)((uint32_t)((uint32_t)DMAMUX1_RequestGenerator0 + (DMAMUX_RGCR_SIZE * (RequestGenChannel)))))->RGCR, DMAMUX_RGxCR_OIE) == (DMAMUX_RGxCR_OIE))? 1UL : 0UL);
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* DMAMUX1 */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_LL_DMAMUX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
