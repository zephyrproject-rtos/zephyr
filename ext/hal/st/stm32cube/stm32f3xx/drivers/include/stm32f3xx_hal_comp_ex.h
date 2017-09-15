/**
  ******************************************************************************
  * @file    stm32f3xx_hal_comp_ex.h
  * @author  MCD Application Team
  * @brief   Header file of COMP HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F3xx_HAL_COMP_EX_H
#define __STM32F3xx_HAL_COMP_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal_def.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

/** @defgroup COMPEx COMPEx
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup COMPEx_Exported_Constants COMP Extended Exported Constants
  * @{
  */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)
/** @defgroup COMPEx_InvertingInput  COMP Extended InvertingInput (STM32F302xE/STM32F303xE/STM32F398xx/STM32F302xC/STM32F303xC/STM32F358xx Product devices)
  * @{
  */
#define COMP_INVERTINGINPUT_1_4VREFINT       (0x00000000U)                        /*!< 1U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_1_2VREFINT       COMP_CSR_COMPxINSEL_0                         /*!< 1U/2 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_3_4VREFINT       COMP_CSR_COMPxINSEL_1                         /*!< 3U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_VREFINT          (COMP_CSR_COMPxINSEL_1|COMP_CSR_COMPxINSEL_0) /*!< VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH1         COMP_CSR_COMPxINSEL_2                         /*!< DAC1_CH1_OUT (PA4) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH2         (COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_0) /*!< DAC1_CH2_OUT (PA5) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_IO1              (COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_1) /*!< IO1 (PA0 for COMP1, PA2 for COMP2, PD15 for COMP3,
                                                                                                PE8 for COMP4, PD13 for COMP5, PD10 for COMP6,
                                                                                                PC0 for COMP7) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_IO2               COMP_CSR_COMPxINSEL                          /*!< IO2 (PB12 for COMP3, PB2 for COMP4, PB10 for COMP5,
                                                                                               PB15 for COMP6) connected to comparator inverting input */
/* Aliases for compatibility */
#define COMP_INVERTINGINPUT_DAC1              COMP_INVERTINGINPUT_DAC1_CH1
#define COMP_INVERTINGINPUT_DAC2              COMP_INVERTINGINPUT_DAC1_CH2
/**
  * @}
  */
#elif defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/** @defgroup COMPEx_InvertingInput COMP Extended InvertingInput (STM32F301x8/STM32F302x8/STM32F318xx Product devices)
  * @{
  */
#define COMP_INVERTINGINPUT_1_4VREFINT     (0x00000000U)                        /*!< 1U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_1_2VREFINT     COMP_CSR_COMPxINSEL_0                         /*!< 1U/2 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_3_4VREFINT     COMP_CSR_COMPxINSEL_1                         /*!< 3U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_VREFINT        (COMP_CSR_COMPxINSEL_1|COMP_CSR_COMPxINSEL_0) /*!< VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH1       COMP_CSR_COMPxINSEL_2                         /*!< DAC1_CH1_OUT (PA4) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_IO1            (COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_1) /*!< IO1 (PA2 for COMP2, PB2 for COMP4, PB15 for COMP6)
                                                                                              connected to comparator inverting input */
/* Aliases for compatibility */
#define COMP_INVERTINGINPUT_DAC1           COMP_INVERTINGINPUT_DAC1_CH1
/**
  * @}
  */
#elif defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/** @defgroup COMPEx_InvertingInput COMP Extended InvertingInput (STM32F303x8/STM32F334x8/STM32F328xx Product devices)
  * @{
  */
/* Note: On these STM32 devices, there is only 1 comparator inverting input   */
/*       connected to a GPIO.                                                 */
/*       It must be chosen among the 2 literals COMP_INVERTINGINPUT_IOx       */
/*       depending on comparator instance COMPx.                              */
#define COMP_INVERTINGINPUT_1_4VREFINT     (0x00000000U)                        /*!< 1U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_1_2VREFINT     COMP_CSR_COMPxINSEL_0                         /*!< 1U/2 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_3_4VREFINT     COMP_CSR_COMPxINSEL_1                         /*!< 3U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_VREFINT        (COMP_CSR_COMPxINSEL_1|COMP_CSR_COMPxINSEL_0) /*!< VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH1       COMP_CSR_COMPxINSEL_2                         /*!< DAC1_CH1_OUT (PA4) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH2       (COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_0) /*!< DAC1_CH2_OUT (PA5) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_IO1            (COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_1) /*!< IO1 (PA2 for COMP2),
                                                                                              connected to comparator inverting input */
#define COMP_INVERTINGINPUT_IO2            (COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_1|COMP_CSR_COMPxINSEL_0) /*!< IO2 (PB2 for COMP4, PB15 for COMP6)
                                                                                              connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC2_CH1       COMP_CSR_COMPxINSEL_3                         /*!< DAC2_CH1_OUT connected to comparator inverting input */

/* Aliases for compatibility */
#define COMP_INVERTINGINPUT_DAC1           COMP_INVERTINGINPUT_DAC1_CH1
#define COMP_INVERTINGINPUT_DAC2           COMP_INVERTINGINPUT_DAC1_CH2
/**
  * @}
  */
#elif defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup COMPEx_InvertingInput COMP Extended InvertingInput (STM32F373xC/STM32F378xx Product devices)
  * @{
  */
#define COMP_INVERTINGINPUT_1_4VREFINT  (0x00000000U)                        /*!< 1U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_1_2VREFINT  ((uint32_t)COMP_CSR_COMPxINSEL_0)             /*!< 1U/2 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_3_4VREFINT  ((uint32_t)COMP_CSR_COMPxINSEL_1)             /*!< 3U/4 VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_VREFINT     ((uint32_t)(COMP_CSR_COMPxINSEL_1|COMP_CSR_COMPxINSEL_0)) /*!< VREFINT connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH1    ((uint32_t)COMP_CSR_COMPxINSEL_2)                         /*!< DAC1_CH1_OUT (PA4) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC1_CH2    ((uint32_t)(COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_0)) /*!< DAC1_CH2_OUT (PA5) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_IO1         ((uint32_t)(COMP_CSR_COMPxINSEL_2|COMP_CSR_COMPxINSEL_1)) /*!< IO1 (PA0 for COMP1, PA2 for COMP2) connected to comparator inverting input */
#define COMP_INVERTINGINPUT_DAC2_CH1    ((uint32_t)COMP_CSR_COMPxINSEL)                          /*!< DAC2_CH1_OUT connected to comparator inverting input */

/* Aliases for compatibility */
#define COMP_INVERTINGINPUT_DAC1        COMP_INVERTINGINPUT_DAC1_CH1
#define COMP_INVERTINGINPUT_DAC2        COMP_INVERTINGINPUT_DAC1_CH2
/**
  * @}
  */
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)
/** @defgroup COMPEx_NonInvertingInput  COMP Extended NonInvertingInput (STM32F302xC/STM32F303xC/STM32F358xx Product devices)
  * @{
  */
#define COMP_NONINVERTINGINPUT_IO1               (0x00000000U) /*!< IO1 (PA1 for COMP1, PA7 for COMP2, PB14 for COMP3,
                                                                             PB0 for COMP4, PD12 for COMP5, PD11 for COMP6,
                                                                             PA0 for COMP7) connected to comparator non inverting input */
#define COMP_NONINVERTINGINPUT_IO2               COMP_CSR_COMPxNONINSEL /*!< IO2 (PA3 for COMP2, PD14 for COMP3, PE7 for COMP4, PB13 for COMP5,
                                                                             PB11 for COMP6, PC1 for COMP7) connected to comparator non inverting input */
#define COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED  COMP1_CSR_COMP1SW1     /*!< DAC ouput connected to comparator COMP1 non inverting input */
/**
  * @}
  */
#elif defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/** @defgroup COMPEx_NonInvertingInput COMP Extended NonInvertingInput (STM32F301x8/STM32F302x8/STM32F318xx Product devices)
  * @{
  */
#define COMP_NONINVERTINGINPUT_IO1               (0x00000000U) /*!< IO1 (PA7 for COMP2, PB0 for COMP4, PB11 for COMP6)
                                                                             connected to comparator non inverting input */
#define COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED  COMP2_CSR_COMP2INPDAC  /*!< DAC ouput connected to comparator COMP2 non inverting input */
/**
  * @}
  */
#elif defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup COMPEx_NonInvertingInput COMP Extended NonInvertingInput (STM32F373xC/STM32F378xx Product devices)
  * @{
  */
#define COMP_NONINVERTINGINPUT_IO1               (0x00000000U) /*!< IO1 (PA1 for COMP1, PA3 for COMP2)
                                                                             connected to comparator non inverting input */
#define COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED  COMP_CSR_COMP1SW1  /*!< DAC ouput connected to comparator COMP1 non inverting input */
/**
  * @}
  */
#elif defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
/** @defgroup COMPEx_NonInvertingInput  COMP Extended NonInvertingInput (STM32F302xE/STM32F303xE/STM32F398xx Product devices)
  * @{
  */
#define COMP_NONINVERTINGINPUT_IO1             (0x00000000U)   /*!< IO1 (PA1 for COMP1, PA7 for COMP2, PB14 for COMP3,
                                                                            PB0 for COMP4, PD12 for COMP5, PD11 for COMP6,
                                                                            PA0 for COMP7) connected to comparator non inverting input */
#define COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED COMP1_CSR_COMP1SW1      /*!< DAC ouput connected to comparator COMP1 non inverting input */
/**
  * @}
  */
#else
/** @defgroup COMPEx_NonInvertingInput COMP Extended NonInvertingInput (Other Product devices)
  * @{
  */
#define COMP_NONINVERTINGINPUT_IO1             (0x00000000U) /*!< IO1 (PA7 for COMP2, PB0 for COMP4, PB11 for COMP6)
                                                                           connected to comparator non inverting input */
/**
  * @}
  */
#endif /* STM32F302xC || STM32F303xC || STM32F358xx */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
/** @defgroup COMPEx_Output COMP Extended Output (STM32F301x8/STM32F302x8/STM32F318xx Product devices)
  *        Elements value convention on 16 LSB: 00XXXX0000YYYYYYb
  *           - YYYYYY : Applicable comparator instance number (bitmap format: 000010 for COMP2, 100000 for COMP6)
  *           - XXXX   : COMPxOUTSEL value
  * @{
  */
/* Output Redirection values common to all comparators COMP2, COMP4 and COMP6 */
#define COMP_OUTPUT_NONE                  (0x0000002AU)   /*!< COMP2, COMP4 or COMP6 output isn't connected to other peripherals */
#define COMP_OUTPUT_TIM1BKIN              (0x0000042AU)   /*!< COMP2, COMP4 or COMP6 output connected to TIM1 Break Input (BKIN) */
#define COMP_OUTPUT_TIM1BKIN2_BRK2        (0x0000082AU)   /*!< COMP2, COMP4 or COMP6  output connected to TIM1 Break Input 2 (BRK2) */
#define COMP_OUTPUT_TIM1BKIN2             (0x0000142AU)   /*!< COMP2, COMP4 or COMP6 output connected to TIM1 Break Input 2 (BKIN2) */
/* Output Redirection specific to COMP2 */
#define COMP_OUTPUT_TIM1OCREFCLR          (0x00001802U)   /*!< COMP2 output connected to TIM1 OCREF Clear */
#define COMP_OUTPUT_TIM1IC1               (0x00001C02U)   /*!< COMP2 output connected to TIM1 Input Capture 1U */
#define COMP_OUTPUT_TIM2IC4               (0x00002002U)   /*!< COMP2 output connected to TIM2 Input Capture 4U */
#define COMP_OUTPUT_TIM2OCREFCLR          (0x00002402U)   /*!< COMP2 output connected to TIM2 OCREF Clear */
/* Output Redirection specific to COMP4 */
#define COMP_OUTPUT_TIM15IC2              (0x00002008U)   /*!< COMP4 output connected to TIM15 Input Capture 2U */
#define COMP_OUTPUT_TIM15OCREFCLR         (0x00002808U)   /*!< COMP4 output connected to TIM15 OCREF Clear */
/* Output Redirection specific to COMP6 */
#define COMP_OUTPUT_TIM2IC2               (0x00001820U)   /*!< COMP6 output connected to TIM2 Input Capture 2U */
#define COMP_OUTPUT_COMP6_TIM2OCREFCLR    (0x00002020U)   /*!< COMP6 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM16OCREFCLR         (0x00002420U)   /*!< COMP6 output connected to TIM16 OCREF Clear */
#define COMP_OUTPUT_TIM16IC1              (0x00002820U)   /*!< COMP6 output connected to TIM16 Input Capture 1U */
/**
  * @}
  */
#elif  defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/** @defgroup COMPEx_Output COMP Extended Output (STM32F303x8/STM32F334x8/STM32F328xx Product devices)
  *        Elements value convention on 16 LSB: 00XXXX0000YYYYYYb
  *           - YYYYYY : Applicable comparator instance number (bitmap format: 000010 for COMP2, 100000 for COMP6)
  *           - XXXX   : COMPxOUTSEL value
  * @{
  */
/* Output Redirection values common to all comparators COMP2, COMP4 and COMP6 */
#define COMP_OUTPUT_NONE                  (0x0000002AU)   /*!< COMP2, COMP4 or COMP6 output isn't connected to other peripherals */
#define COMP_OUTPUT_TIM1BKIN              (0x0000042AU)   /*!< COMP2, COMP4 or COMP6 output connected to TIM1 Break Input (BKIN) */
#define COMP_OUTPUT_TIM1BKIN2             (0x0000082AU)   /*!< COMP2, COMP4 or COMP6 output connected to TIM1 Break Input 2 (BKIN2) */
/* Output Redirection common to COMP2 and COMP4 */
#define COMP_OUTPUT_TIM3OCREFCLR          (0x00002C0AU)   /*!< COMP2 or COMP4 output connected to TIM3 OCREF Clear */
/* Output Redirection specific to COMP2 */
#define COMP_OUTPUT_TIM1OCREFCLR          (0x00001802U)   /*!< COMP2 output connected to TIM1 OCREF Clear */
#define COMP_OUTPUT_TIM1IC1               (0x00001C02U)   /*!< COMP2 output connected to TIM1 Input Capture 1U */
#define COMP_OUTPUT_TIM2IC4               (0x00002002U)   /*!< COMP2 output connected to TIM2 Input Capture 4U */
#define COMP_OUTPUT_TIM2OCREFCLR          (0x00002402U)   /*!< COMP2 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM3IC1               (0x00002802U)   /*!< COMP2 output connected to TIM3 Input Capture 1U */
/* Output Redirection specific to COMP4 */
#define COMP_OUTPUT_TIM3IC3               (0x00001808U)   /*!< COMP4 output connected to TIM3 Input Capture 3U */
#define COMP_OUTPUT_TIM15IC2              (0x00002008U)   /*!< COMP4 output connected to TIM15 Input Capture 2U */
#define COMP_OUTPUT_TIM15OCREFCLR         (0x00002808U)   /*!< COMP4 output connected to TIM15 OCREF Clear */
/* Output Redirection specific to COMP6 */
#define COMP_OUTPUT_TIM2IC2               (0x00001820U)   /*!< COMP6 output connected to TIM2 Input Capture 2U */
#define COMP_OUTPUT_COMP6_TIM2OCREFCLR    (0x00002020U)   /*!< COMP6 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM16OCREFCLR         (0x00002420U)   /*!< COMP6 output connected to TIM16 OCREF Clear */
#define COMP_OUTPUT_TIM16IC1              (0x00002820U)   /*!< COMP6 output connected to TIM16 Input Capture 1U */
/**
  * @}
  */
#elif  defined(STM32F302xC) || defined(STM32F302xE)
/** @defgroup COMPEx_Output COMP Extended Output (STM32F302xC/STM32F302xE Product devices)
  *        Elements value convention on 16 LSB: 00XXXX0000YYYYYYb
  *           - YYYYYY : Applicable comparator instance number (bitmap format: 000001 for COMP1, 100000 for COMP6)
  *           - XXXX   : COMPxOUTSEL value
  * @{
  */
/* Output Redirection values common to all comparators COMP1, COMP2, COMP4, COMP6 */
#define COMP_OUTPUT_NONE                  (0x0000002BU)   /*!< COMP1, COMP2, COMP4 or COMP6 output isn't connected to other peripherals */
#define COMP_OUTPUT_TIM1BKIN              (0x0000042BU)   /*!< COMP1, COMP2, COMP4 or COMP6 output connected to TIM1 Break Input (BKIN) */
#define COMP_OUTPUT_TIM1BKIN2_BRK2        (0x0000082BU)   /*!< COMP1, COMP2, COMP4 or COMP6 output connected to TIM1 Break Input 2 (BRK2) */
#define COMP_OUTPUT_TIM1BKIN2             (0x0000142BU)   /*!< COMP1, COMP2, COMP4 or COMP6 output connected to TIM1 Break Input 2 (BKIN2) */
/* Output Redirection common to COMP1 and COMP2 */
#define COMP_OUTPUT_TIM1OCREFCLR          (0x00001803U)   /*!< COMP1 or COMP2 output connected to TIM1 OCREF Clear */
#define COMP_OUTPUT_TIM1IC1               (0x00001C03U)   /*!< COMP1 or COMP2 output connected to TIM1 Input Capture 1U */
#define COMP_OUTPUT_TIM2IC4               (0x00002003U)   /*!< COMP1 or COMP2 output connected to TIM2 Input Capture 4U */
#define COMP_OUTPUT_TIM2OCREFCLR          (0x00002403U)   /*!< COMP1 or COMP2 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM3IC1               (0x00002803U)   /*!< COMP1 or COMP2 output connected to TIM3 Input Capture 1U */
/* Output Redirection common to COMP1,COMP2 and COMP4 */
#define COMP_OUTPUT_TIM3OCREFCLR          (0x00002C0BU)   /*!< COMP1, COMP2 or COMP4 output connected to TIM3 OCREF Clear */
/* Output Redirection specific to COMP4 */
#define COMP_OUTPUT_TIM3IC3               (0x00001808U)   /*!< COMP4 output connected to TIM3 Input Capture 3U */
#define COMP_OUTPUT_TIM15IC2              (0x00002008U)   /*!< COMP4 output connected to TIM15 Input Capture 2U */
#define COMP_OUTPUT_TIM4IC2               (0x00002408U)   /*!< COMP4 output connected to TIM4 Input Capture 2U */
#define COMP_OUTPUT_TIM15OCREFCLR         (0x00002808U)   /*!< COMP4 output connected to TIM15 OCREF Clear */
/* Output Redirection specific to COMP6 */
#define COMP_OUTPUT_TIM2IC2               (0x00001820U)   /*!< COMP6 output connected to TIM2 Input Capture 2U */
#define COMP_OUTPUT_COMP6_TIM2OCREFCLR    (0x00002020U)   /*!< COMP6 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM16OCREFCLR         (0x00002420U)   /*!< COMP6 output connected to TIM16 OCREF Clear */
#define COMP_OUTPUT_TIM16IC1              (0x00002820U)   /*!< COMP6 output connected to TIM16 Input Capture 1U */
#define COMP_OUTPUT_TIM4IC4               (0x00002C20U)   /*!< COMP6 output connected to TIM4 Input Capture 4U */
/**
  * @}
  */
#elif  defined(STM32F303xC) || defined(STM32F358xx)
/** @defgroup COMPEx_Output COMP Extended Output (STM32F303xC/STM32F358xx Product devices)
  *        Elements value convention on 16 LSB: 00XXXX000YYYYYYYb
  *           - YYYYYYY : Applicable comparator instance number (bitmap format: 0000001 for COMP1, 1000000 for COMP7)
  *           - XXXX    : COMPxOUTSEL value
  * @{
  */
/* Output Redirection values common to all comparators COMP1...COMP7 */
#define COMP_OUTPUT_NONE                  (0x0000007FU)   /*!< COMP1, COMP2... or COMP7 output isn't connected to other peripherals */
#define COMP_OUTPUT_TIM1BKIN              (0x0000047FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input (BKIN) */
#define COMP_OUTPUT_TIM1BKIN2             (0x0000087FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input 2 (BKIN2) */
#define COMP_OUTPUT_TIM8BKIN              (0x00000C7FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM8 Break Input (BKIN) */
#define COMP_OUTPUT_TIM8BKIN2             (0x0000107FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM8 Break Input 2 (BKIN2) */
#define COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2   (0x0000147FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input 2 and TIM8 Break Input 2U */
/* Output Redirection common to COMP1, COMP2, COMP3 and COMP7 */
#define COMP_OUTPUT_TIM1OCREFCLR          (0x00001847U)   /*!< COMP1, COMP2, COMP3 or COMP7 output connected to TIM1 OCREF Clear */
/* Output Redirection common to COMP1, COMP2 and COMP3 */
#define COMP_OUTPUT_TIM2OCREFCLR          (0x00002407U)   /*!< COMP1, COMP2 or COMP3 output connected to TIM2 OCREF Clear */
/* Output Redirection common to COMP1, COMP2, COMP4 and COMP5 */
#define COMP_OUTPUT_TIM3OCREFCLR          (0x00002C1BU)   /*!< COMP1, COMP2, COMP4 or COMP5 output connected to TIM3 OCREF Clear */
/* Output Redirection common to COMP4, COMP5, COMP6 and COMP7 */
#define COMP_OUTPUT_TIM8OCREFCLR          (0x00001C78U)   /*!< COMP4, COMP5, COMP6 or COMP7 output connected to TIM8 OCREF Clear */
/* Output Redirection common to COMP1 and COMP2 */
#define COMP_OUTPUT_TIM1IC1               (0x00001C03U)   /*!< COMP1 or COMP2 output connected to TIM1 Input Capture 1U */
#define COMP_OUTPUT_TIM2IC4               (0x00002003U)   /*!< COMP1 or COMP2 output connected to TIM2 Input Capture 4U */
#define COMP_OUTPUT_TIM3IC1               (0x00002803U)   /*!< COMP1 or COMP2 output connected to TIM3 Input Capture 1U */
/* Output Redirection specific to COMP3 */
#define COMP_OUTPUT_TIM4IC1               (0x00001C04U)   /*!< COMP3 output connected to TIM4 Input Capture 1U */
#define COMP_OUTPUT_TIM3IC2               (0x00002004U)   /*!< COMP3 output connected to TIM3 Input Capture 2U */
#define COMP_OUTPUT_TIM15IC1              (0x00002804U)   /*!< COMP3 output connected to TIM15 Input Capture 1U */
#define COMP_OUTPUT_TIM15BKIN             (0x00002C04U)   /*!< COMP3 output connected to TIM15 Break Input (BKIN) */
/* Output Redirection specific to COMP4 */
#define COMP_OUTPUT_TIM3IC3               (0x00001808U)   /*!< COMP4 output connected to TIM3 Input Capture 3U */
#define COMP_OUTPUT_TIM15IC2              (0x00002008U)   /*!< COMP4 output connected to TIM15 Input Capture 2U */
#define COMP_OUTPUT_TIM4IC2               (0x00002408U)   /*!< COMP4 output connected to TIM4 Input Capture 2U */
#define COMP_OUTPUT_TIM15OCREFCLR         (0x00002808U)   /*!< COMP4 output connected to TIM15 OCREF Clear */
/* Output Redirection specific to COMP5 */
#define COMP_OUTPUT_TIM2IC1               (0x00001810U)   /*!< COMP5 output connected to TIM2 Input Capture 1U */
#define COMP_OUTPUT_TIM17IC1              (0x00002010U)   /*!< COMP5 output connected to TIM17 Input Capture 1U */
#define COMP_OUTPUT_TIM4IC3               (0x00002410U)   /*!< COMP5 output connected to TIM4 Input Capture 3U */
#define COMP_OUTPUT_TIM16BKIN             (0x00002810U)   /*!< COMP5 output connected to TIM16 Break Input (BKIN) */
/* Output Redirection specific to COMP6 */
#define COMP_OUTPUT_TIM2IC2               (0x00001820U)   /*!< COMP6 output connected to TIM2 Input Capture 2U */
#define COMP_OUTPUT_COMP6_TIM2OCREFCLR    (0x00002020U)   /*!< COMP6 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM16OCREFCLR         (0x00002420U)   /*!< COMP6 output connected to TIM16 OCREF Clear */
#define COMP_OUTPUT_TIM16IC1              (0x00002820U)   /*!< COMP6 output connected to TIM16 Input Capture 1U */
#define COMP_OUTPUT_TIM4IC4               (0x00002C20U)   /*!< COMP6 output connected to TIM4 Input Capture 4U */
/* Output Redirection specific to COMP7 */
#define COMP_OUTPUT_TIM2IC3               (0x00002040U)   /*!< COMP7 output connected to TIM2 Input Capture 3U */
#define COMP_OUTPUT_TIM1IC2               (0x00002440U)   /*!< COMP7 output connected to TIM1 Input Capture 2U */
#define COMP_OUTPUT_TIM17OCREFCLR         (0x00002840U)   /*!< COMP7 output connected to TIM17 OCREF Clear */
#define COMP_OUTPUT_TIM17BKIN             (0x00002C40U)   /*!< COMP7 output connected to TIM17 Break Input (BKIN) */
/**
  * @}
  */
#elif defined(STM32F303xE) || defined(STM32F398xx)
/** @defgroup COMPEx_Output COMP Extended Output (STM32F303xE/STM32F398xx Product devices)
  *        Elements value convention on 16 LSB: 00XXXX000YYYYYYYb
  *           - YYYYYYY : Applicable comparator instance number (bitmap format: 0000001 for COMP1, 1000000 for COMP7)
  *           - XXXX    : COMPxOUTSEL value
  * @{
  */
/* Output Redirection values common to all comparators COMP1...COMP7 */
#define COMP_OUTPUT_NONE                  (0x0000007FU)   /*!< COMP1, COMP2... or COMP7 output isn't connected to other peripherals */
#define COMP_OUTPUT_TIM1BKIN              (0x0000047FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input (BKIN) */
#define COMP_OUTPUT_TIM1BKIN2             (0x0000087FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input 2 (BKIN2) */
#define COMP_OUTPUT_TIM8BKIN              (0x00000C7FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM8 Break Input (BKIN) */
#define COMP_OUTPUT_TIM8BKIN2             (0x0000107FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM8 Break Input 2 (BKIN2) */
#define COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2   (0x0000147FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input 2 and TIM8 Break Input 2U */
#define COMP_OUTPUT_TIM20BKIN             (0x0000307FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM20 Break Input (BKIN) */
#define COMP_OUTPUT_TIM20BKIN2            (0x0000347FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM20 Break Input 2 (BKIN2) */
#define COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2    (0x0000387FU)   /*!< COMP1, COMP2... or COMP7 output connected to TIM1 Break Input 2U, TIM8 Break Input 2 and TIM20 Break Input 2  */
/* Output Redirection common to COMP1, COMP2, COMP3 and COMP7 */
#define COMP_OUTPUT_TIM1OCREFCLR          (0x00001847U)   /*!< COMP1, COMP2, COMP3 or COMP7 output connected to TIM1 OCREF Clear */
/* Output Redirection common to COMP1, COMP2 and COMP3 */
#define COMP_OUTPUT_TIM2OCREFCLR          (0x00002407U)   /*!< COMP1, COMP2 or COMP3 output connected to TIM2 OCREF Clear */
/* Output Redirection common to COMP1, COMP2, COMP4 and COMP5 */
#define COMP_OUTPUT_TIM3OCREFCLR          (0x00002C1BU)   /*!< COMP1, COMP2, COMP4 or COMP5 output connected to TIM3 OCREF Clear */
/* Output Redirection common to COMP4, COMP5, COMP6 and COMP7 */
#define COMP_OUTPUT_TIM8OCREFCLR          (0x00001C78U)   /*!< COMP4, COMP5, COMP6 or COMP7 output connected to TIM8 OCREF Clear */
/* Output Redirection common to COMP1 and COMP2 */
#define COMP_OUTPUT_TIM1IC1               (0x00001C03U)   /*!< COMP1 or COMP2 output connected to TIM1 Input Capture 1U */
#define COMP_OUTPUT_TIM2IC4               (0x00002003U)   /*!< COMP1 or COMP2 output connected to TIM2 Input Capture 4U */
#define COMP_OUTPUT_TIM3IC1               (0x00002803U)   /*!< COMP1 or COMP2 output connected to TIM3 Input Capture 1U */
/* Output Redirection specific to COMP2 */
#define COMP_OUTPUT_TIM20OCREFCLR         (0x00003C04U)   /*!< COMP2 output connected to TIM20 OCREF Clear */
/* Output Redirection specific to COMP3 */
#define COMP_OUTPUT_TIM4IC1               (0x00001C04U)   /*!< COMP3 output connected to TIM4 Input Capture 1U */
#define COMP_OUTPUT_TIM3IC2               (0x00002004U)   /*!< COMP3 output connected to TIM3 Input Capture 2U */
#define COMP_OUTPUT_TIM15IC1              (0x00002804U)   /*!< COMP3 output connected to TIM15 Input Capture 1U */
#define COMP_OUTPUT_TIM15BKIN             (0x00002C04U)   /*!< COMP3 output connected to TIM15 Break Input (BKIN) */
/* Output Redirection specific to COMP4 */
#define COMP_OUTPUT_TIM3IC3               (0x00001808U)   /*!< COMP4 output connected to TIM3 Input Capture 3U */
#define COMP_OUTPUT_TIM15IC2              (0x00002008U)   /*!< COMP4 output connected to TIM15 Input Capture 2U */
#define COMP_OUTPUT_TIM4IC2               (0x00002408U)   /*!< COMP4 output connected to TIM4 Input Capture 2U */
#define COMP_OUTPUT_TIM15OCREFCLR         (0x00002808U)   /*!< COMP4 output connected to TIM15 OCREF Clear */
/* Output Redirection specific to COMP5 */
#define COMP_OUTPUT_TIM2IC1               (0x00001810U)   /*!< COMP5 output connected to TIM2 Input Capture 1U */
#define COMP_OUTPUT_TIM17IC1              (0x00002010U)   /*!< COMP5 output connected to TIM17 Input Capture 1U */
#define COMP_OUTPUT_TIM4IC3               (0x00002410U)   /*!< COMP5 output connected to TIM4 Input Capture 3U */
#define COMP_OUTPUT_TIM16BKIN             (0x00002810U)   /*!< COMP5 output connected to TIM16 Break Input (BKIN) */
/* Output Redirection specific to COMP6 */
#define COMP_OUTPUT_TIM2IC2               (0x00001820U)   /*!< COMP6 output connected to TIM2 Input Capture 2U */
#define COMP_OUTPUT_COMP6_TIM2OCREFCLR    (0x00002020U)   /*!< COMP6 output connected to TIM2 OCREF Clear */
#define COMP_OUTPUT_TIM16OCREFCLR         (0x00002420U)   /*!< COMP6 output connected to TIM16 OCREF Clear */
#define COMP_OUTPUT_TIM16IC1              (0x00002820U)   /*!< COMP6 output connected to TIM16 Input Capture 1U */
#define COMP_OUTPUT_TIM4IC4               (0x00002C20U)   /*!< COMP6 output connected to TIM4 Input Capture 4U */
/* Output Redirection specific to COMP7 */
#define COMP_OUTPUT_TIM2IC3               (0x00002040U)   /*!< COMP7 output connected to TIM2 Input Capture 3U */
#define COMP_OUTPUT_TIM1IC2               (0x00002440U)   /*!< COMP7 output connected to TIM1 Input Capture 2U */
#define COMP_OUTPUT_TIM17OCREFCLR         (0x00002840U)   /*!< COMP7 output connected to TIM17 OCREF Clear */
#define COMP_OUTPUT_TIM17BKIN             (0x00002C40U)   /*!< COMP7 output connected to TIM17 Break Input (BKIN) */
/**
  * @}
  */
#elif  defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup COMPEx_Output COMP Extended Output (STM32F373xC/STM32F378xx Product devices)
  *        Elements value convention: 00000XXX000000YYb
  *           - YY   : Applicable comparator instance number (bitmap format: 01 for COMP1, 10 for COMP2)
  *           - XXX  : COMPxOUTSEL value
  * @{
  */
/* Output Redirection values common to all comparators COMP1 and COMP2 */
#define COMP_OUTPUT_NONE                  (0x0003U)   /*!< COMP1 or COMP2 output isn't connected to other peripherals */
#define COMP_OUTPUT_TIM2IC4               (0x0403U)   /*!< COMP1 or COMP2 output connected to TIM2 Input Capture 4U */
#define COMP_OUTPUT_TIM2OCREFCLR          (0x0503U)   /*!< COMP1 or COMP2 output connected to TIM2 OCREF Clear */
/* Output Redirection specific to COMP1 */
#define COMP_OUTPUT_TIM15BKIN             (0x0101U)   /*!< COMP1 output connected to TIM15 Break Input */
#define COMP_OUTPUT_COMP1_TIM3IC1         (0x0201U)   /*!< COMP1 output connected to TIM3 Input Capture 1U */
#define COMP_OUTPUT_COMP1_TIM3OCREFCLR    (0x0301U)   /*!< COMP1 output connected to TIM3 OCREF Clear */
#define COMP_OUTPUT_TIM5IC4               (0x0601U)   /*!< COMP1 output connected to TIM5 Input Capture 4U */
#define COMP_OUTPUT_TIM5OCREFCLR          (0x0701U)   /*!< COMP1 output connected to TIM5 OCREF Clear */
/* Output Redirection specific to COMP2 */
#define COMP_OUTPUT_TIM16BKIN             (0x0102U)   /*!< COMP2 output connected to TIM16 Break Input */
#define COMP_OUTPUT_TIM4IC1               (0x0202U)   /*!< COMP2 output connected to TIM4 Input Capture 1U */
#define COMP_OUTPUT_TIM4OCREFCLR          (0x0302U)   /*!< COMP2 output connected to TIM4 OCREF Clear */
#define COMP_OUTPUT_COMP2_TIM3IC1         (0x0602U)   /*!< COMP2 output connected to TIM3 Input Capture 1U */
#define COMP_OUTPUT_COMP2_TIM3OCREFCLR    (0x0702U)   /*!< COMP2 output connected to TIM3 OCREF Clear */
/**
  * @}
  */
#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx */

#if  defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)
/** @defgroup COMPEx_WindowMode COMP Extended WindowMode (STM32F302xC/STM32F303xC/STM32F358xx Product devices)
  * @{
  */
#define COMP_WINDOWMODE_DISABLE           (0x00000000U)  /*!< Window mode disabled */
#define COMP_WINDOWMODE_ENABLE            COMP_CSR_COMPxWNDWEN    /*!< Window mode enabled: non inverting input of comparator X (x=2U,4,6U)
                                                                       is connected to the non inverting input of comparator X-1U */
/**
  * @}
  */
#elif defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup COMPEx_WindowMode COMP Extended WindowMode (STM32F373xC/STM32F378xx Product devices)
  * @{
  */
#define COMP_WINDOWMODE_DISABLE           (0x00000000U)  /*!< Window mode disabled */
#define COMP_WINDOWMODE_ENABLE            ((uint32_t)COMP_CSR_COMPxWNDWEN) /*!< Window mode enabled: non inverting input of comparator 2
                                                                                is connected to the non inverting input of comparator 1 (PA1) */
/**
  * @}
  */
#else
/** @defgroup COMPEx_WindowMode COMP Extended WindowMode (Other Product devices)
  * @{
  */
#define COMP_WINDOWMODE_DISABLE           (0x00000000U)  /*!< Window mode disabled (not available) */
/**
  * @}
  */
#endif /* STM32F302xC || STM32F303xC || STM32F358xx */

/** @defgroup COMPEx_Mode COMP Extended Mode
  * @{
  */
#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F373xC) || defined(STM32F378xx)

/* Please refer to the electrical characteristics in the device datasheet for
   the power consumption values */
#define COMP_MODE_HIGHSPEED               (0x00000000U) /*!< High Speed */
#define COMP_MODE_MEDIUMSPEED             COMP_CSR_COMPxMODE_0   /*!< Medium Speed */
#define COMP_MODE_LOWPOWER                COMP_CSR_COMPxMODE_1   /*!< Low power mode */
#define COMP_MODE_ULTRALOWPOWER           COMP_CSR_COMPxMODE     /*!< Ultra-low power mode */

#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx */
/**
  * @}
  */

/** @defgroup COMPEx_Hysteresis COMP Extended Hysteresis
  * @{
  */
#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) || \
    defined(STM32F373xC) || defined(STM32F378xx)

#define COMP_HYSTERESIS_NONE              (0x00000000U)  /*!< No hysteresis */
#define COMP_HYSTERESIS_LOW               COMP_CSR_COMPxHYST_0    /*!< Hysteresis level low */
#define COMP_HYSTERESIS_MEDIUM            COMP_CSR_COMPxHYST_1    /*!< Hysteresis level medium */
#define COMP_HYSTERESIS_HIGH              COMP_CSR_COMPxHYST      /*!< Hysteresis level high */

#else

#define COMP_HYSTERESIS_NONE              (0x00000000U)  /*!< No hysteresis */

#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F373xC || STM32F378xx */
/**
  * @}
  */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/** @defgroup COMPEx_BlankingSrce  COMP Extended Blanking Source (STM32F301x8/STM32F302x8/STM32F303x8/STM32F334x8/STM32F318xx/STM32F328xx Product devices)
  * @{
  */
/* No blanking source can be selected for all comparators */
#define COMP_BLANKINGSRCE_NONE                 (0x00000000U)    /*!< No blanking source */
/* Blanking source for COMP2 */
#define COMP_BLANKINGSRCE_TIM1OC5              COMP_CSR_COMPxBLANKING_0  /*!< TIM1 OC5 selected as blanking source for COMP2 */
#define COMP_BLANKINGSRCE_TIM2OC3              COMP_CSR_COMPxBLANKING_1  /*!< TIM2 OC3 selected as blanking source for COMP2 */
#define COMP_BLANKINGSRCE_TIM3OC3              (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM3 OC3 selected as blanking source for COMP2 */
/* Blanking source for COMP4 */
#define COMP_BLANKINGSRCE_TIM3OC4              COMP_CSR_COMPxBLANKING_0    /*!< TIM3 OC4 selected as blanking source for COMP4 */
#define COMP_BLANKINGSRCE_TIM15OC1             (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM15 OC1 selected as blanking source for COMP4 */
/* Blanking source for COMP6 */
#define COMP_BLANKINGSRCE_TIM2OC4              (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM2 OC4 selected as blanking source for COMP6 */
#define COMP_BLANKINGSRCE_TIM15OC2             COMP_CSR_COMPxBLANKING_2    /*!< TIM15 OC2 selected as blanking source for COMP6 */
/**
  * @}
  */

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx */

#if defined(STM32F302xE) ||\
    defined(STM32F302xC)
/** @defgroup COMPEx_BlankingSrce COMP Extended Blanking Source (STM32F302xE/STM32F302xC Product devices)
  * @{
  */
/* No blanking source can be selected for all comparators */
#define COMP_BLANKINGSRCE_NONE                 (0x00000000U)    /*!< No blanking source */
/* Blanking source common for COMP1 and COMP2 */
#define COMP_BLANKINGSRCE_TIM1OC5              COMP_CSR_COMPxBLANKING_0  /*!< TIM1 OC5 selected as blanking source for COMP1 and COMP2 */
/* Blanking source common for COMP1 and COMP2 */
#define COMP_BLANKINGSRCE_TIM2OC3              COMP_CSR_COMPxBLANKING_1  /*!< TIM2 OC3 selected as blanking source for COMP1 and COMP2 */
/* Blanking source common for COMP1 and COMP2 */
#define COMP_BLANKINGSRCE_TIM3OC3              (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM3 OC3 selected as blanking source for COMP1 and COMP2 */
/* Blanking source for COMP4 */
#define COMP_BLANKINGSRCE_TIM3OC4              COMP_CSR_COMPxBLANKING_0    /*!< TIM3 OC4 selected as blanking source for COMP4 */
#define COMP_BLANKINGSRCE_TIM15OC1             (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM15 OC1 selected as blanking source for COMP4 */
/* Blanking source for COMP6 */
#define COMP_BLANKINGSRCE_TIM2OC4              (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM2 OC4 selected as blanking source for COMP6 */
#define COMP_BLANKINGSRCE_TIM15OC2             COMP_CSR_COMPxBLANKING_2    /*!< TIM15 OC2 selected as blanking source for COMP6 */
/**
  * @}
  */

#endif /* STM32F302xE || */
       /* STM32F302xC    */

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)
/** @defgroup COMPEx_BlankingSrce COMP Extended Blanking Source (STM32F303xE/STM32F398xx/STM32F303xC/STM32F358xx Product devices)
  * @{
  */
/* No blanking source can be selected for all comparators */
#define COMP_BLANKINGSRCE_NONE                 (0x00000000U)    /*!< No blanking source */
/* Blanking source common for COMP1, COMP2, COMP3 and COMP7 */
#define COMP_BLANKINGSRCE_TIM1OC5              COMP_CSR_COMPxBLANKING_0  /*!< TIM1 OC5 selected as blanking source for COMP1, COMP2, COMP3 and COMP7 */
/* Blanking source common for COMP1 and COMP2 */
#define COMP_BLANKINGSRCE_TIM2OC3              COMP_CSR_COMPxBLANKING_1  /*!< TIM2 OC5 selected as blanking source for COMP1 and COMP2 */
/* Blanking source common for COMP1, COMP2 and COMP5 */
#define COMP_BLANKINGSRCE_TIM3OC3              (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM2 OC3 selected as blanking source for COMP1, COMP2 and COMP5 */
/* Blanking source common for COMP3 and COMP6 */
#define COMP_BLANKINGSRCE_TIM2OC4              (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM2 OC4 selected as blanking source for COMP3 and COMP6 */
/* Blanking source common for COMP4, COMP5, COMP6 and COMP7 */
#define COMP_BLANKINGSRCE_TIM8OC5              COMP_CSR_COMPxBLANKING_1  /*!< TIM8 OC5 selected as blanking source for COMP4, COMP5, COMP6 and COMP7 */
/* Blanking source for COMP4 */
#define COMP_BLANKINGSRCE_TIM3OC4              COMP_CSR_COMPxBLANKING_0  /*!< TIM3 OC4 selected as blanking source for COMP4 */
#define COMP_BLANKINGSRCE_TIM15OC1             (COMP_CSR_COMPxBLANKING_0|COMP_CSR_COMPxBLANKING_1)    /*!< TIM15 OC1 selected as blanking source for COMP4 */
/* Blanking source common for COMP6 and COMP7 */
#define COMP_BLANKINGSRCE_TIM15OC2             COMP_CSR_COMPxBLANKING_2  /*!< TIM15 OC2 selected as blanking source for COMP6 and COMP7 */
/**
  * @}
  */

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/** @defgroup COMPEx_BlankingSrce COMP Extended Blanking Source (STM32F373xC/STM32F378xx Product devices)
  * @{
  */
/* No blanking source can be selected for all comparators */
#define COMP_BLANKINGSRCE_NONE                 (0x00000000U)     /*!< No blanking source */
/**
  * @}
  */

#endif /* STM32F373xC || STM32F378xx */

/** @defgroup COMP_Flag COMP Flag
  * @{
  */
#if defined(STM32F373xC) || defined(STM32F378xx)
#define COMP_FLAG_LOCK                         COMP_CSR_COMP1LOCK      /*!< Lock flag */
#else
#define COMP_FLAG_LOCK                         COMP_CSR_LOCK           /*!< Lock flag */
#endif /* STM32F373xC || STM32F378xx */
/**
  * @}
  */


/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup COMPEx_Exported_Macros COMP Extended Exported Macros
  * @{
  */
#if defined(STM32F373xC) || defined(STM32F378xx)

/**
  * @brief  Enable the specified comparator.
  * @param  __HANDLE__  COMP handle.
  * @retval None
  */
#define __HAL_COMP_ENABLE(__HANDLE__)                                          \
        do {                                                                   \
          uint32_t regshift = COMP_CSR_COMP1_SHIFT;                            \
                                                                               \
          if((__HANDLE__)->Instance == COMP2)                                  \
          {                                                                    \
            regshift = COMP_CSR_COMP2_SHIFT;                                   \
          }                                                                    \
          SET_BIT(COMP->CSR, (uint32_t)COMP_CSR_COMPxEN << regshift);          \
        } while(0U)

/**
  * @brief  Disable the specified comparator.
  * @param  __HANDLE__  COMP handle.
  * @retval None
  */
#define __HAL_COMP_DISABLE(__HANDLE__)                                         \
        do {                                                                   \
          uint32_t regshift = COMP_CSR_COMP1_SHIFT;                            \
                                                                               \
          if((__HANDLE__)->Instance == COMP2)                                  \
          {                                                                    \
            regshift = COMP_CSR_COMP2_SHIFT;                                   \
          }                                                                    \
          CLEAR_BIT(COMP->CSR, (uint32_t)COMP_CSR_COMPxEN << regshift);        \
        } while(0U)

/**
  * @brief  Lock a comparator instance
  * @param  __HANDLE__  COMP handle
  * @retval None.
  */
#define __HAL_COMP_LOCK(__HANDLE__)                                            \
        do {                                                                   \
          uint32_t regshift = COMP_CSR_COMP1_SHIFT;                            \
                                                                               \
          if((__HANDLE__)->Instance == COMP2)                                  \
          {                                                                    \
            regshift = COMP_CSR_COMP2_SHIFT;                                   \
          }                                                                    \
          SET_BIT(COMP->CSR, (uint32_t)COMP_CSR_COMPxLOCK << regshift);        \
        } while(0U)

/** @brief  Check whether the specified COMP flag is set or not.
  * @param  __HANDLE__  COMP Handle.
  * @param  __FLAG__  flag to check.
  *        This parameter can be one of the following values:
  *            @arg @ref COMP_FLAG_LOCK   lock flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_COMP_GET_FLAG(__HANDLE__, __FLAG__)                              \
        (((__HANDLE__)->Instance == COMP1) ? (((__HANDLE__)->Instance->CSR & (__FLAG__)) == (__FLAG__)) \
         (((__HANDLE__)->Instance->CSR & (uint32_t)((__FLAG__) << COMP_CSR_COMP2_SHIFT) == (__FLAG__))))

#else

/**
  * @brief  Enable the specified comparator.
  * @param  __HANDLE__  COMP handle.
  * @retval None
  */
#define __HAL_COMP_ENABLE(__HANDLE__)    SET_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_COMPxEN)

/**
  * @brief  Disable the specified comparator.
  * @param  __HANDLE__  COMP handle.
  * @retval None
  */
#define __HAL_COMP_DISABLE(__HANDLE__)   CLEAR_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_COMPxEN)

/**
  * @brief  Lock a comparator instance
  * @param  __HANDLE__  COMP handle
  * @retval None.
  */
#define __HAL_COMP_LOCK(__HANDLE__)      SET_BIT((__HANDLE__)->Instance->CSR, COMP_CSR_COMPxLOCK)

/** @brief  Check whether the specified COMP flag is set or not.
  * @param  __HANDLE__  COMP Handle.
  * @param  __FLAG__  flag to check.
  *        This parameter can be one of the following values:
  *            @arg @ref COMP_FLAG_LOCK   lock flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_COMP_GET_FLAG(__HANDLE__, __FLAG__)     (((__HANDLE__)->Instance->CSR & (__FLAG__)) == (__FLAG__))

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) ||  \
    defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) ||  \
    defined(STM32F373xC) || defined(STM32F378xx)

/**
  * @brief  Enable the COMP1 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP1 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP1 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP1_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP1_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP1 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_RISING_FALLING_EDGE()  do { \
                                                               __HAL_COMP_COMP1_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP1_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP1 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Generate a software interrupt on the COMP1 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Enable the COMP1 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_ENABLE_EVENT()           SET_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Disable the COMP1 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_DISABLE_EVENT()          CLEAR_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Check whether the COMP1 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP1_EXTI_GET_FLAG()              READ_BIT(EXTI->PR, COMP_EXTI_LINE_COMP1)

/**
  * @brief  Clear the COMP1 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP1_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR, COMP_EXTI_LINE_COMP1)

#endif /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F373xC || STM32F378xx */

/**
  * @brief  Enable the COMP2 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Enable the COMP2 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Enable the COMP2 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP2_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP2_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP2 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP2_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP2_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP2 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Generate a software interrupt on the COMP2 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Enable the COMP2 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_ENABLE_EVENT()          SET_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Disable the COMP2 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_DISABLE_EVENT()         CLEAR_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Check whether the COMP2 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP2_EXTI_GET_FLAG()              READ_BIT(EXTI->PR, COMP_EXTI_LINE_COMP2)

/**
  * @brief  Clear the COMP2 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP2_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR, COMP_EXTI_LINE_COMP2)

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)

/**
  * @brief  Enable the COMP3 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Disable the COMP3 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Enable the COMP3 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Disable the COMP3 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Enable the COMP3 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP3_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP3_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP3 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_DISABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP3_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP3_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP3 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Disable the COMP3 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Generate a software interrupt on the COMP3 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Enable the COMP3 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_ENABLE_EVENT()          SET_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Disable the COMP3 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_DISABLE_EVENT()         CLEAR_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Check whether the COMP3 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP3_EXTI_GET_FLAG()              READ_BIT(EXTI->PR, COMP_EXTI_LINE_COMP3)

/**
  * @brief  Clear the COMP3 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP3_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR, COMP_EXTI_LINE_COMP3)

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) ||  \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) ||  \
    defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) ||  \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)

/**
  * @brief  Enable the COMP4 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Disable the COMP4 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Enable the COMP4 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Disable the COMP4 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Enable the COMP4 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP4_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP4_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP4 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_DISABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP4_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP4_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP4 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Disable the COMP4 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Generate a software interrupt on the COMP4 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Enable the COMP4 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_ENABLE_EVENT()          SET_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Disable the COMP4 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_DISABLE_EVENT()         CLEAR_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Check whether the COMP4 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP4_EXTI_GET_FLAG()              READ_BIT(EXTI->PR, COMP_EXTI_LINE_COMP4)

/**
  * @brief  Clear the COMP4 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP4_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR, COMP_EXTI_LINE_COMP4)

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)

/**
  * @brief  Enable the COMP5 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Disable the COMP5 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Enable the COMP5 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Disable the COMP5 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Enable the COMP5 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP5_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP5_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP5 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_DISABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP5_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP5_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP5 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Disable the COMP5 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Generate a software interrupt on the COMP5 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Enable the COMP5 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_ENABLE_EVENT()          SET_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Disable the COMP5 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_DISABLE_EVENT()         CLEAR_BIT(EXTI->EMR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Check whether the COMP5 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP5_EXTI_GET_FLAG()              READ_BIT(EXTI->PR, COMP_EXTI_LINE_COMP5)

/**
  * @brief  Clear the COMP5 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP5_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR, COMP_EXTI_LINE_COMP5)

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) ||  \
    defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx) ||  \
    defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx) ||  \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)

/**
  * @brief  Enable the COMP6 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Disable the COMP6 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Enable the COMP6 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Disable the COMP6 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Enable the COMP6 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP6_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP6_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP6 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_DISABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP6_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP6_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP6 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Disable the COMP6 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Generate a software interrupt on the COMP6 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Enable the COMP6 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_ENABLE_EVENT()          SET_BIT(EXTI->EMR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Disable the COMP6 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_DISABLE_EVENT()         CLEAR_BIT(EXTI->EMR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Check whether the COMP6 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP6_EXTI_GET_FLAG()              READ_BIT(EXTI->PR2, COMP_EXTI_LINE_COMP6)

/**
  * @brief  Clear the COMP6 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP6_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR2, COMP_EXTI_LINE_COMP6)

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)
/**
  * @brief  Enable the COMP7 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_ENABLE_RISING_EDGE()    SET_BIT(EXTI->RTSR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Disable the COMP7 EXTI line rising edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_DISABLE_RISING_EDGE()   CLEAR_BIT(EXTI->RTSR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Enable the COMP7 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_ENABLE_FALLING_EDGE()   SET_BIT(EXTI->FTSR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Disable the COMP7 EXTI line falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_DISABLE_FALLING_EDGE()  CLEAR_BIT(EXTI->FTSR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Enable the COMP7 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_ENABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP7_EXTI_ENABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP7_EXTI_ENABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Disable the COMP7 EXTI line rising & falling edge trigger.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_DISABLE_RISING_FALLING_EDGE()   do { \
                                                               __HAL_COMP_COMP7_EXTI_DISABLE_RISING_EDGE(); \
                                                               __HAL_COMP_COMP7_EXTI_DISABLE_FALLING_EDGE(); \
                                                             } while(0U)

/**
  * @brief  Enable the COMP7 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_ENABLE_IT()             SET_BIT(EXTI->IMR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Disable the COMP7 EXTI line in interrupt mode.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_DISABLE_IT()            CLEAR_BIT(EXTI->IMR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Generate a software interrupt on the COMP7 EXTI line.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_GENERATE_SWIT()         SET_BIT(EXTI->SWIER2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Enable the COMP7 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_ENABLE_EVENT()          SET_BIT(EXTI->EMR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Disable the COMP7 EXTI line in event mode.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_DISABLE_EVENT()         CLEAR_BIT(EXTI->EMR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Check whether the COMP7 EXTI line flag is set or not.
  * @retval RESET or SET
  */
#define __HAL_COMP_COMP7_EXTI_GET_FLAG()              READ_BIT(EXTI->PR2, COMP_EXTI_LINE_COMP7)

/**
  * @brief  Clear the COMP7 EXTI flag.
  * @retval None
  */
#define __HAL_COMP_COMP7_EXTI_CLEAR_FLAG()            WRITE_REG(EXTI->PR2, COMP_EXTI_LINE_COMP7)

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

/**
  * @}
  */

/* Private types -------------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup COMPEx_Private_Constants COMP Extended Private Constants
  * @{
  */
/** @defgroup COMPEx_ExtiLineEvent COMP Extended EXTI lines
  * @{
  */
#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)

#define COMP_EXTI_LINE_COMP2                   EXTI_IMR_MR22    /*!< External interrupt line 22 connected to COMP2 */
#define COMP_EXTI_LINE_COMP4                   EXTI_IMR_MR30    /*!< External interrupt line 30 connected to COMP4 */
#define COMP_EXTI_LINE_COMP6                   EXTI_IMR2_MR32   /*!< External interrupt line 32 connected to COMP6 */

#define COMP_EXTI_LINE_REG2_MASK               EXTI_IMR2_MR32   /*!< Mask for External interrupt line control in register xxx2 */

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx */

#if defined(STM32F302xE) || \
    defined(STM32F302xC)

#define COMP_EXTI_LINE_COMP1                   EXTI_IMR_MR21    /*!< External interrupt line 21 connected to COMP1 */
#define COMP_EXTI_LINE_COMP2                   EXTI_IMR_MR22    /*!< External interrupt line 22 connected to COMP2 */
#define COMP_EXTI_LINE_COMP4                   EXTI_IMR_MR30    /*!< External interrupt line 30 connected to COMP4 */
#define COMP_EXTI_LINE_COMP6                   EXTI_IMR2_MR32   /*!< External interrupt line 32 connected to COMP6 */

#define COMP_EXTI_LINE_REG2_MASK               EXTI_IMR2_MR32   /*!< Mask for External interrupt line control in register xxx2 */

#endif /* STM32F302xE || */
       /* STM32F302xC    */

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)

#define COMP_EXTI_LINE_COMP1                   EXTI_IMR_MR21    /*!< External interrupt line 21 connected to COMP1 */
#define COMP_EXTI_LINE_COMP2                   EXTI_IMR_MR22    /*!< External interrupt line 22 connected to COMP2 */
#define COMP_EXTI_LINE_COMP3                   EXTI_IMR_MR29    /*!< External interrupt line 29 connected to COMP3 */
#define COMP_EXTI_LINE_COMP4                   EXTI_IMR_MR30    /*!< External interrupt line 30 connected to COMP4 */
#define COMP_EXTI_LINE_COMP5                   EXTI_IMR_MR31    /*!< External interrupt line 31 connected to COMP5 */
#define COMP_EXTI_LINE_COMP6                   EXTI_IMR2_MR32   /*!< External interrupt line 32 connected to COMP6 */
#define COMP_EXTI_LINE_COMP7                   EXTI_IMR2_MR33   /*!< External interrupt line 33 connected to COMP7 */

#define COMP_EXTI_LINE_REG2_MASK               (EXTI_IMR2_MR33 | EXTI_IMR2_MR32) /*!< Mask for External interrupt line control in register xxx2 */

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)

#define COMP_EXTI_LINE_COMP1                   EXTI_IMR_MR21    /*!< External interrupt line 21 connected to COMP1 */
#define COMP_EXTI_LINE_COMP2                   EXTI_IMR_MR22    /*!< External interrupt line 22 connected to COMP2 */

#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

/** @defgroup COMPEx_Misc COMP Extended miscellaneous defines
  * @{
  */

/* CSR masks redefinition for internal use */
#define COMP_CSR_COMPxINSEL_MASK              COMP_CSR_COMPxINSEL   /*!< COMP_CSR_COMPxINSEL Mask */
#define COMP_CSR_COMPxOUTSEL_MASK             COMP_CSR_COMPxOUTSEL  /*!< COMP_CSR_COMPxOUTSEL Mask */
#define COMP_CSR_COMPxPOL_MASK                COMP_CSR_COMPxPOL     /*!< COMP_CSR_COMPxPOL Mask   */
#if  defined(STM32F373xC) || defined(STM32F378xx)
/* CSR register reset value */
#define COMP_CSR_RESET_VALUE                  (0x00000000U)
#define COMP_CSR_RESET_PARAMETERS_MASK        (0x00003FFFU)
#define COMP_CSR_UPDATE_PARAMETERS_MASK       (0x00003FFEU)
/* CSR COMP1/COMP2 shift */
#define COMP_CSR_COMP1_SHIFT                  0U
#define COMP_CSR_COMP2_SHIFT                  16U
#else
/* CSR register reset value */
#define COMP_CSR_RESET_VALUE                  (0x00000000U)
#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
#define COMP_CSR_COMPxNONINSEL_MASK            (COMP2_CSR_COMP2INPDAC) /*!< COMP_CSR_COMPxNONINSEL mask */
#define COMP_CSR_COMPxWNDWEN_MASK              (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxMODE_MASK                (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxHYST_MASK                (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxBLANKING_MASK            COMP_CSR_COMPxBLANKING /*!< COMP_CSR_COMPxBLANKING mask */
#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx */

#if defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
#define COMP_CSR_COMPxNONINSEL_MASK            (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxWNDWEN_MASK              (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxMODE_MASK                (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxHYST_MASK                (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxBLANKING_MASK            COMP_CSR_COMPxBLANKING /*!< COMP_CSR_COMPxBLANKING mask */
#endif /* STM32F303x8 || STM32F334x8 || STM32F328xx */

#if  defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)
#define COMP_CSR_COMPxNONINSEL_MASK            (COMP_CSR_COMPxNONINSEL | COMP1_CSR_COMP1SW1) /*!< COMP_CSR_COMPxNONINSEL mask */
#define COMP_CSR_COMPxWNDWEN_MASK              COMP_CSR_COMPxWNDWEN   /*!< COMP_CSR_COMPxWNDWEN mask */
#define COMP_CSR_COMPxMODE_MASK                COMP_CSR_COMPxMODE     /*!< COMP_CSR_COMPxMODE Mask */
#define COMP_CSR_COMPxHYST_MASK                COMP_CSR_COMPxHYST     /*!< COMP_CSR_COMPxHYST Mask */
#define COMP_CSR_COMPxBLANKING_MASK            COMP_CSR_COMPxBLANKING /*!< COMP_CSR_COMPxBLANKING mask */
#endif /* STM32F302xC || STM32F303xC || STM32F358xx */

#if  defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)
#define COMP_CSR_COMPxNONINSEL_MASK            (COMP1_CSR_COMP1SW1) /*!< COMP_CSR_COMPxNONINSEL mask */
#define COMP_CSR_COMPxWNDWEN_MASK              (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxMODE_MASK                (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxHYST_MASK                (0x00000000U) /*!< Mask empty: feature not available */
#define COMP_CSR_COMPxBLANKING_MASK            COMP_CSR_COMPxBLANKING /*!< COMP_CSR_COMPxBLANKING mask */
#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined(STM32F373xC) || defined(STM32F378xx)
#define COMP_CSR_COMPxNONINSEL_MASK            (COMP_CSR_COMP1SW1)  /*!< COMP_CSR_COMPxNONINSEL mask */
#define COMP_CSR_COMPxWNDWEN_MASK              COMP_CSR_COMPxWNDWEN /*!< COMP_CSR_COMPxWNDWEN mask */
#define COMP_CSR_COMPxMODE_MASK                COMP_CSR_COMPxMODE   /*!< COMP_CSR_COMPxMODE Mask */
#define COMP_CSR_COMPxHYST_MASK                COMP_CSR_COMPxHYST   /*!< COMP_CSR_COMPxHYST Mask */
#define COMP_CSR_COMPxBLANKING_MASK            (0x00000000U) /*!< Mask empty: feature not available */
#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup COMPEx_Private_Macros COMP Extended Private Macros
  * @{
  */
/** @defgroup COMP_GET_EXTI_LINE COMP Extended Private macro to get the EXTI line associated with a comparator handle
  * @{
  */
#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) || \
    defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
/**
  * @brief  Get the specified EXTI line for a comparator instance
  * @param  __INSTANCE__ specifies the COMP instance.
  * @retval value of @ref COMPEx_ExtiLineEvent
  */
#define COMP_GET_EXTI_LINE(__INSTANCE__) (((__INSTANCE__) == COMP2) ? COMP_EXTI_LINE_COMP2 : \
                                          ((__INSTANCE__) == COMP4) ? COMP_EXTI_LINE_COMP4 : \
                                          COMP_EXTI_LINE_COMP6)
#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx */

#if defined(STM32F302xE) || \
    defined(STM32F302xC)
/**
  * @brief  Get the specified EXTI line for a comparator instance
  * @param  __INSTANCE__ specifies the COMP instance.
  * @retval value of @ref COMPEx_ExtiLineEvent
  */
#define COMP_GET_EXTI_LINE(__INSTANCE__) (((__INSTANCE__) == COMP1) ? COMP_EXTI_LINE_COMP1 : \
                                          ((__INSTANCE__) == COMP2) ? COMP_EXTI_LINE_COMP2 : \
                                          ((__INSTANCE__) == COMP4) ? COMP_EXTI_LINE_COMP4 : \
                                          COMP_EXTI_LINE_COMP6)
#endif /* STM32F302xE || */
       /* STM32F302xC    */

#if defined(STM32F303xE) || defined(STM32F398xx) || \
    defined(STM32F303xC) || defined(STM32F358xx)
/**
  * @brief  Get the specified EXTI line for a comparator instance
  * @param  __INSTANCE__ specifies the COMP instance.
  * @retval value of @ref COMPEx_ExtiLineEvent
  */
#define COMP_GET_EXTI_LINE(__INSTANCE__) (((__INSTANCE__) == COMP1) ? COMP_EXTI_LINE_COMP1 : \
                                          ((__INSTANCE__) == COMP2) ? COMP_EXTI_LINE_COMP2 : \
                                          ((__INSTANCE__) == COMP3) ? COMP_EXTI_LINE_COMP3 : \
                                          ((__INSTANCE__) == COMP4) ? COMP_EXTI_LINE_COMP4 : \
                                          ((__INSTANCE__) == COMP5) ? COMP_EXTI_LINE_COMP5 : \
                                          ((__INSTANCE__) == COMP6) ? COMP_EXTI_LINE_COMP6 : \
                                          COMP_EXTI_LINE_COMP7)
#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Get the specified EXTI line for a comparator instance
  * @param  __INSTANCE__ specifies the COMP instance.
  * @retval value of @ref COMPEx_ExtiLineEvent
  */
#define COMP_GET_EXTI_LINE(__INSTANCE__) (((__INSTANCE__) == COMP1) ? COMP_EXTI_LINE_COMP1 : \
                                          COMP_EXTI_LINE_COMP2)
#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

/** @defgroup COMPEx_Private_Macros_Misc COMP Extended miscellaneous private macros
  * @{
  */

#if defined(STM32F373xC) || defined(STM32F378xx)
/**
  * @brief  Init a comparator instance
  * @param  __HANDLE__  COMP handle
  * @note   The common output selection is checked versus the COMP instance to set the right output configuration
  * @retval None.
  */

#define COMP_INIT(__HANDLE__)                                                          \
        do {                                                                           \
          uint32_t regshift = COMP_CSR_COMP1_SHIFT;                                    \
          uint32_t compoutput = (__HANDLE__)->Init.Output & COMP_CSR_COMPxOUTSEL_MASK; \
                                                                                       \
          if((__HANDLE__)->Instance == COMP2)                                          \
          {                                                                            \
            regshift = COMP_CSR_COMP2_SHIFT;                                           \
          }                                                                            \
                                                                                       \
          MODIFY_REG(COMP->CSR,                                                        \
                     (COMP_CSR_COMPxINSEL  | COMP_CSR_COMPxNONINSEL_MASK |             \
                     COMP_CSR_COMPxOUTSEL  | COMP_CSR_COMPxPOL           |             \
                     COMP_CSR_COMPxHYST    | COMP_CSR_COMPxMODE) << regshift,          \
                     ((__HANDLE__)->Init.InvertingInput    |                           \
                     (__HANDLE__)->Init.NonInvertingInput  |                           \
                     compoutput                            |                           \
                     (__HANDLE__)->Init.OutputPol          |                           \
                     (__HANDLE__)->Init.Hysteresis         |                           \
                     (__HANDLE__)->Init.Mode) << regshift);                            \
                                                                                       \
          if((__HANDLE__)->Init.WindowMode != COMP_WINDOWMODE_DISABLE)                 \
          {                                                                            \
            COMP->CSR |= COMP_CSR_WNDWEN;                                              \
          }                                                                            \
        } while(0U)

/**
  * @brief  DeInit a comparator instance
  * @param  __HANDLE__  COMP handle
  * @retval None.
  */
#define COMP_DEINIT(__HANDLE__)                                                \
        do {                                                                   \
          uint32_t regshift = COMP_CSR_COMP1_SHIFT;                            \
                                                                               \
          if((__HANDLE__)->Instance == COMP2)                                  \
          {                                                                    \
            regshift = COMP_CSR_COMP2_SHIFT;                                   \
          }                                                                    \
          MODIFY_REG(COMP->CSR,                                                \
                     COMP_CSR_RESET_PARAMETERS_MASK << regshift,               \
                     COMP_CSR_RESET_VALUE << regshift);                        \
        } while(0U)


/**
  * @brief  Enable the Exti Line rising edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_RISING_ENABLE(__EXTILINE__)      SET_BIT(EXTI->RTSR, (__EXTILINE__))

/**
  * @brief  Disable the Exti Line rising edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_RISING_DISABLE(__EXTILINE__)     CLEAR_BIT(EXTI->RTSR, (__EXTILINE__))

/**
  * @brief  Enable the Exti Line falling edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_FALLING_ENABLE(__EXTILINE__)     SET_BIT(EXTI->FTSR, (__EXTILINE__))

/**
  * @brief  Disable the Exti Line falling edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_FALLING_DISABLE(__EXTILINE__)    CLEAR_BIT(EXTI->FTSR, (__EXTILINE__))

/**
  * @brief  Enable the COMP Exti Line interrupt generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_ENABLE_IT(__EXTILINE__)          SET_BIT(EXTI->IMR, (__EXTILINE__))

/**
  * @brief  Disable the COMP Exti Line interrupt generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_DISABLE_IT(__EXTILINE__)         CLEAR_BIT(EXTI->IMR, (__EXTILINE__))

/**
  * @brief  Enable the COMP Exti Line event generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_ENABLE_EVENT(__EXTILINE__)       SET_BIT(EXTI->EMR, (__EXTILINE__))

/**
  * @brief  Disable the COMP Exti Line event generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_DISABLE_EVENT(__EXTILINE__)      CLEAR_BIT(EXTI->EMR, (__EXTILINE__))

/**
  * @brief  Check whether the specified EXTI line flag is set or not.
  * @param  __FLAG__ specifies the COMP Exti sources to be checked.
  *          This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval The state of __FLAG__ (SET or RESET).
  */
#define COMP_EXTI_GET_FLAG(__FLAG__)               READ_BIT(EXTI->PR, (__FLAG__))

/**
  * @brief Clear the COMP Exti flags.
  * @param  __FLAG__ specifies the COMP Exti sources to be cleared.
  *          This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_CLEAR_FLAG(__FLAG__)             WRITE_REG(EXTI->PR, (__FLAG__))

#else /* STM32F30x, STM32F32xx, STM32F35x, STM32F39x, STM32F33x */


/**
  * @brief  Init a comparator instance
  * @param  __HANDLE__  COMP handle
  * @retval None.
  */
#define COMP_INIT(__HANDLE__)                                                                                   \
        do {                                                                                                    \
          __IO uint32_t     csrreg = 0U;                                                                         \
                                                                                                                \
          csrreg = READ_REG((__HANDLE__)->Instance->CSR);                                                       \
          MODIFY_REG(csrreg, COMP_CSR_COMPxINSEL_MASK, (__HANDLE__)->Init.InvertingInput);                      \
          MODIFY_REG(csrreg, COMP_CSR_COMPxNONINSEL_MASK, (__HANDLE__)->Init.NonInvertingInput);                \
          MODIFY_REG(csrreg, COMP_CSR_COMPxBLANKING_MASK, (__HANDLE__)->Init.BlankingSrce);                     \
          MODIFY_REG(csrreg, COMP_CSR_COMPxOUTSEL_MASK, (__HANDLE__)->Init.Output & COMP_CSR_COMPxOUTSEL_MASK); \
          MODIFY_REG(csrreg, COMP_CSR_COMPxPOL_MASK, (__HANDLE__)->Init.OutputPol);                             \
          MODIFY_REG(csrreg, COMP_CSR_COMPxHYST_MASK, (__HANDLE__)->Init.Hysteresis);                           \
          MODIFY_REG(csrreg, COMP_CSR_COMPxMODE_MASK, (__HANDLE__)->Init.Mode);                                 \
          MODIFY_REG(csrreg, COMP_CSR_COMPxWNDWEN_MASK, (__HANDLE__)->Init.WindowMode);                         \
          WRITE_REG((__HANDLE__)->Instance->CSR, csrreg);                                                       \
        } while(0U)

/**
  * @brief  DeInit a comparator instance
  * @param  __HANDLE__  COMP handle
  * @retval None.
  */
#define COMP_DEINIT(__HANDLE__)    WRITE_REG((__HANDLE__)->Instance->CSR, COMP_CSR_RESET_VALUE)

/**
  * @brief  Enable the Exti Line rising edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_RISING_ENABLE(__EXTILINE__)      ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? SET_BIT(EXTI->RTSR2, (__EXTILINE__)) : SET_BIT(EXTI->RTSR, (__EXTILINE__)))

/**
  * @brief  Disable the Exti Line rising edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_RISING_DISABLE(__EXTILINE__)     ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? CLEAR_BIT(EXTI->RTSR2, (__EXTILINE__)) : CLEAR_BIT(EXTI->RTSR, (__EXTILINE__)))

/**
  * @brief  Enable the Exti Line falling edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_FALLING_ENABLE(__EXTILINE__)     ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? SET_BIT(EXTI->FTSR2, (__EXTILINE__)) : SET_BIT(EXTI->FTSR, (__EXTILINE__)))

/**
  * @brief  Disable the Exti Line falling edge trigger.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_FALLING_DISABLE(__EXTILINE__)    ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? CLEAR_BIT(EXTI->FTSR2, (__EXTILINE__)) : CLEAR_BIT(EXTI->FTSR, (__EXTILINE__)))

/**
  * @brief  Enable the COMP Exti Line interrupt generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_ENABLE_IT(__EXTILINE__)          ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? SET_BIT(EXTI->IMR2, (__EXTILINE__)) : SET_BIT(EXTI->IMR, (__EXTILINE__)))

/**
  * @brief  Disable the COMP Exti Line interrupt generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_DISABLE_IT(__EXTILINE__)         ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? CLEAR_BIT(EXTI->IMR2, (__EXTILINE__)) : CLEAR_BIT(EXTI->IMR, (__EXTILINE__)))

/**
  * @brief  Enable the COMP Exti Line event generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be enabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_ENABLE_EVENT(__EXTILINE__)       ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? SET_BIT(EXTI->EMR2, (__EXTILINE__)) : SET_BIT(EXTI->EMR, (__EXTILINE__)))

/**
  * @brief  Disable the COMP Exti Line event generation.
  * @param  __EXTILINE__ specifies the COMP Exti sources to be disabled.
  *         This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_DISABLE_EVENT(__EXTILINE__)      ((((__EXTILINE__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? CLEAR_BIT(EXTI->EMR2, (__EXTILINE__)) : CLEAR_BIT(EXTI->EMR, (__EXTILINE__)))

/**
  * @brief  Check whether the specified EXTI line flag is set or not.
  * @param  __FLAG__ specifies the COMP Exti sources to be checked.
  *          This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval The state of __FLAG__ (SET or RESET).
  */
#define COMP_EXTI_GET_FLAG(__FLAG__)               ((((__FLAG__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? READ_BIT(EXTI->PR2, (__FLAG__)) : READ_BIT(EXTI->PR, (__FLAG__)))

/**
  * @brief Clear the COMP Exti flags.
  * @param  __FLAG__ specifies the COMP Exti sources to be cleared.
  *          This parameter can be a value of @ref COMPEx_ExtiLineEvent
  * @retval None.
  */
#define COMP_EXTI_CLEAR_FLAG(__FLAG__)             ((((__FLAG__) & COMP_EXTI_LINE_REG2_MASK) != RESET) ? WRITE_REG(EXTI->PR2, (__FLAG__)) : WRITE_REG(EXTI->PR, (__FLAG__)))

#endif /* STM32F373xC || STM32F378xx */


/**
  * @brief  Manage inverting input comparator inverting input connected to a GPIO
  *         for STM32F302x, STM32F32xx, STM32F33x.
  *         - On devices STM32F302x, STM32F32xx, STM32F33x, there is
  *           only 1 comparator inverting input connected to a GPIO.
  *           Legacy definition of literal COMP_INVERTINGINPUT_IO1
  *           was initially the only selection, but depending on
  *           comparator instance it corresponds to COMP_INVERTINGINPUT_IO2
  *           (for instances COMP4, COMP6).
  *           Since, COMP_INVERTINGINPUT_IO2 has been created and this macro
  *           selects the correct literal COMP_INVERTINGINPUT_IOx in function
  *           of comparator instance.
  *         - On other STM32F3 devices, this macro performs no action.
  * @param  __COMP_INSTANCE__  COMP instance
  * @param  __INVERTINGINPUT__  COMP inverting input
  * @retval None.
  */
#if defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
#define COMP_INVERTINGINPUT_SELECTION(__COMP_INSTANCE__, __INVERTINGINPUT__)   \
  (((__INVERTINGINPUT__) != COMP_INVERTINGINPUT_IO1)                           \
    ? (                                                                        \
       (__INVERTINGINPUT__)                                                    \
      )                                                                        \
      :                                                                        \
      (((__COMP_INSTANCE__) == COMP2)                                          \
        ? (                                                                    \
           (COMP_INVERTINGINPUT_IO1)                                           \
          )                                                                    \
          :                                                                    \
          (                                                                    \
           (COMP_INVERTINGINPUT_IO2)                                           \
          )                                                                    \
      )                                                                        \
  )
#else
#define COMP_INVERTINGINPUT_SELECTION(__COMP_INSTANCE__, __INVERTINGINPUT__)   \
  (__INVERTINGINPUT__)
#endif

/**
  * @}
  */

/** @defgroup COMPEx_IS_COMP_Definitions COMP Extended Private macros to check input parameters
  * @{
  */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

#define IS_COMP_INVERTINGINPUT(INPUT) (((INPUT) == COMP_INVERTINGINPUT_1_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_1_2VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_3_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_VREFINT)          || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH1)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO1))

#define IS_COMP_NONINVERTINGINPUT(INPUT) (((INPUT) == COMP_NONINVERTINGINPUT_IO1) || \
                                          ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED))

/* STM32F301x6/x8, STM32F302x6/x8, STM32F318xx devices comparator instances non inverting source values */
#define IS_COMP_NONINVERTINGINPUT_INSTANCE(INSTANCE, INPUT)    \
   ((((INSTANCE) == COMP2)  &&                                 \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)                || \
     ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED)))    \
    ||                                                         \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)))

#define IS_COMP_WINDOWMODE(WINDOWMODE) ((WINDOWMODE) == (WINDOWMODE)) /*!< Not available: check always true */

#define IS_COMP_MODE(MODE)  ((MODE) == (MODE))  /*!< Not available: check always true */

#define IS_COMP_HYSTERESIS(HYSTERESIS)    ((HYSTERESIS) == (HYSTERESIS)) /*!< Not available: check always true */

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)      || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC2)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)      \
   ((((INSTANCE) == COMP2)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)))          \
    ||                                                 \
    (((INSTANCE) == COMP4)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)))         \
    ||                                                 \
    (((INSTANCE) == COMP6)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16IC1)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR))))

#define IS_COMP_BLANKINGSRCE(SOURCE) (((SOURCE) == COMP_BLANKINGSRCE_NONE)     || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM1OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC1) || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC2))

/* STM32F301x6/x8, STM32F302x6/x8, STM32F303x6/x8, STM32F334x4/6U/8U, STM32F318xx/STM32F328xx devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   ((((INSTANCE) == COMP2)  &&                                \
    (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)     ||        \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5)  ||        \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC3)  ||        \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))          \
    ||                                                        \
    (((INSTANCE) == COMP4) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC1)))        \
    ||                                                        \
    (((INSTANCE) == COMP6) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2))))

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx */

#if defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)

#define IS_COMP_INVERTINGINPUT(INPUT) (((INPUT) == COMP_INVERTINGINPUT_1_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_1_2VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_3_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_VREFINT)          || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH1)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH2)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO1)              || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO2)              || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC2_CH1))

/*!< Non inverting input not available */
#define IS_COMP_NONINVERTINGINPUT(INPUT) ((INPUT) == (INPUT))  /*!< Multiple selection not available: check always true */

#define IS_COMP_NONINVERTINGINPUT_INSTANCE(INSTANCE, INPUT) ((INPUT) == (INPUT))   /*!< Multiple selection not available: check always true */

#define IS_COMP_WINDOWMODE(WINDOWMODE) ((WINDOWMODE) == (WINDOWMODE)) /*!< Not available: check always true */

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC2)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)      \
   ((((INSTANCE) == COMP2)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))          \
    ||                                                 \
    (((INSTANCE) == COMP4)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC3)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)))         \
    ||                                                 \
    (((INSTANCE) == COMP6)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16IC1)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR))))

#define IS_COMP_MODE(MODE)  ((MODE) == (MODE))  /*!< Not available: check always true */

#define IS_COMP_HYSTERESIS(HYSTERESIS)    ((HYSTERESIS) == (HYSTERESIS)) /*!< Not available: check always true */

#define IS_COMP_BLANKINGSRCE(SOURCE) (((SOURCE) == COMP_BLANKINGSRCE_NONE)     || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM1OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC1) || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC2))

/* STM32F301x6/x8, STM32F302x6/x8, STM32F303x6/x8, STM32F334x4/6U/8U, STM32F318xx/STM32F328xx devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   ((((INSTANCE) == COMP2)  &&                                \
    (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)     ||        \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5)  ||        \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC3)  ||        \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))          \
    ||                                                        \
    (((INSTANCE) == COMP4) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC1)))        \
    ||                                                        \
    (((INSTANCE) == COMP6) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2))))

#endif /* STM32F303x8 || STM32F334x8 || STM32F328xx */

#if defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)

#define IS_COMP_INVERTINGINPUT(INPUT) (((INPUT) == COMP_INVERTINGINPUT_1_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_1_2VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_3_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_VREFINT)          || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH1)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH2)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO1)              || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO2))

#define IS_COMP_NONINVERTINGINPUT(INPUT) (((INPUT) == COMP_NONINVERTINGINPUT_IO1) || \
                                          ((INPUT) == COMP_NONINVERTINGINPUT_IO2) || \
                                          ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED))

/* STM32F302xB/xC, STM32F303xB/xC, STM32F358xx devices comparator instances non inverting source values */
#define IS_COMP_NONINVERTINGINPUT_INSTANCE(INSTANCE, INPUT)    \
   ((((INSTANCE) == COMP1)  &&                                 \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)                || \
     ((INPUT) == COMP_NONINVERTINGINPUT_IO2)                || \
     ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED)))    \
    ||                                                         \
    ((((INPUT) == COMP_NONINVERTINGINPUT_IO1)               || \
      ((INPUT) == COMP_NONINVERTINGINPUT_IO2))))

#define IS_COMP_WINDOWMODE(WINDOWMODE) (((WINDOWMODE) == COMP_WINDOWMODE_DISABLE) || \
                                        ((WINDOWMODE) == COMP_WINDOWMODE_ENABLE))

#define IS_COMP_MODE(MODE)  (((MODE) == COMP_MODE_HIGHSPEED)     || \
                             ((MODE) == COMP_MODE_MEDIUMSPEED)   || \
                             ((MODE) == COMP_MODE_LOWPOWER)      || \
                             ((MODE) == COMP_MODE_ULTRALOWPOWER))

#define IS_COMP_HYSTERESIS(HYSTERESIS)    (((HYSTERESIS) == COMP_HYSTERESIS_NONE)   || \
                                           ((HYSTERESIS) == COMP_HYSTERESIS_LOW)    || \
                                           ((HYSTERESIS) == COMP_HYSTERESIS_MEDIUM) || \
                                           ((HYSTERESIS) == COMP_HYSTERESIS_HIGH))

#if  defined(STM32F302xC)

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)      || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC2)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC4))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)      \
   ((((INSTANCE) == COMP1)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))          \
    ||                                                 \
    (((INSTANCE) == COMP2)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))          \
    ||                                                 \
    (((INSTANCE) == COMP4)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC3)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)))         \
    ||                                                 \
    (((INSTANCE) == COMP6)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16IC1)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR))))

#define IS_COMP_BLANKINGSRCE(SOURCE) (((SOURCE) == COMP_BLANKINGSRCE_NONE)     || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM1OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC1) || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC2))

/* STM32F302xB/STM32F302xC/STM32F302xE devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   (((((INSTANCE) == COMP1) || ((INSTANCE) == COMP2))  &&     \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC3) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))         \
    ||                                                        \
    (((INSTANCE) == COMP4) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC1)))        \
    ||                                                        \
    (((INSTANCE) == COMP6) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2))))

#endif /* STM32F302xC */

#if  defined(STM32F303xC) || defined(STM32F358xx)

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC2)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM17BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM17IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM17OCREFCLR))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)       \
   ((((INSTANCE) == COMP1)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))           \
    ||                                                  \
    (((INSTANCE) == COMP2)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))           \
    ||                                                  \
    (((INSTANCE) == COMP3)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC2)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC1)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15BKIN)))              \
    ||                                                  \
    (((INSTANCE) == COMP4)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC3)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC2)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)))          \
    ||                                                  \
    (((INSTANCE) == COMP5)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC1)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC3)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM17IC1)))               \
    ||                                                  \
    (((INSTANCE) == COMP6)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC4)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR)))          \
    ||                                                  \
    (((INSTANCE) == COMP7)  &&                          \
    (((OUTPUT) == COMP_OUTPUT_NONE)                ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC2)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC3)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM17BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM17OCREFCLR))))

#define IS_COMP_BLANKINGSRCE(SOURCE) (((SOURCE) == COMP_BLANKINGSRCE_NONE)     || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM1OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM8OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC1) || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC2))

/* STM32F303xE/STM32F398xx/STM32F303xB/STM32F303xC/STM32F358xx devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   (((((INSTANCE) == COMP1) || ((INSTANCE) == COMP2))  &&     \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC3) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))         \
    ||                                                        \
    (((INSTANCE) == COMP3) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4)))         \
    ||                                                        \
    (((INSTANCE) == COMP4) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC1)))        \
    ||                                                        \
    (((INSTANCE) == COMP5) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))         \
    ||                                                        \
    (((INSTANCE) == COMP6) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2)))        \
    ||                                                        \
    (((INSTANCE) == COMP7) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2))))

#endif /* STM32F303xC || STM32F358xx */

#endif /* STM32F302xC || STM32F303xC || STM32F358xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)

#define IS_COMP_INVERTINGINPUT(INPUT) (((INPUT) == COMP_INVERTINGINPUT_1_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_1_2VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_3_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_VREFINT)          || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH1)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH2)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO1)              || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO2))

#define IS_COMP_NONINVERTINGINPUT(INPUT) (((INPUT) == COMP_NONINVERTINGINPUT_IO1) || \
                                          ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED))

/* STM32F302xE/STM32F303xE/STM32F398xx devices comparator instances non inverting source values */
#define IS_COMP_NONINVERTINGINPUT_INSTANCE(INSTANCE, INPUT)    \
   ((((INSTANCE) == COMP1)  &&                                 \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)                || \
     ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED)))    \
    ||                                                         \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)))

#define IS_COMP_WINDOWMODE(WINDOWMODE) ((WINDOWMODE) == (WINDOWMODE))    /*!< Not available: check always true */

#define IS_COMP_MODE(MODE)  ((MODE) == (MODE))  /*!< Not available: check always true */

#define IS_COMP_HYSTERESIS(HYSTERESIS)    ((HYSTERESIS) == (HYSTERESIS)) /*!< Not available: check always true */

#if defined(STM32F302xE)

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)      || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC2)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC4))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)      \
   ((((INSTANCE) == COMP1)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))          \
    ||                                                 \
    (((INSTANCE) == COMP2)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))          \
    ||                                                 \
    (((INSTANCE) == COMP4)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC3)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC2)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)))         \
    ||                                                 \
    (((INSTANCE) == COMP6)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_BRK2)     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)          ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16IC1)           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR))))

#define IS_COMP_BLANKINGSRCE(SOURCE) (((SOURCE) == COMP_BLANKINGSRCE_NONE)     || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM1OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC1) || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC2))

/* STM32F302xB/STM32F302xC/STM32F302xE devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   (((((INSTANCE) == COMP1) || ((INSTANCE) == COMP2))  &&     \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC3) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))         \
    ||                                                        \
    (((INSTANCE) == COMP4) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC1)))        \
    ||                                                        \
    (((INSTANCE) == COMP6) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2))))

#endif /* STM32F302xE */

#if defined(STM32F303xE) || defined(STM32F398xx)

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM20OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2) || \
                                ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)          || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM3IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM17IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16IC1)            || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR)       || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC3)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM1IC2)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM17BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM17OCREFCLR))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)                  \
   ((((INSTANCE) == COMP1)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)                   ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)                   ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)))                      \
    ||                                                             \
    (((INSTANCE) == COMP2)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC1)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1OCREFCLR)                   ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)                   ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC1)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3OCREFCLR)                   ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20OCREFCLR)))                     \
    ||                                                             \
    (((INSTANCE) == COMP3)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC2)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC1)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC1)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15BKIN)))                         \
    ||                                                             \
    (((INSTANCE) == COMP4)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM3IC3)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC2)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8OCREFCLR)                   ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15IC2)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15OCREFCLR)))                     \
    ||                                                             \
    (((INSTANCE) == COMP5)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC1)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC3)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM17IC1)))                          \
    ||                                                             \
    (((INSTANCE) == COMP6)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC2)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP6_TIM2OCREFCLR)             ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC4)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16IC1)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16OCREFCLR)))                     \
    ||                                                             \
    (((INSTANCE) == COMP7)  &&                                     \
    (((OUTPUT) == COMP_OUTPUT_NONE)                           ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN)                       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM8BKIN2)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM20BKIN2)                     ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1BKIN2_TIM8BKIN2_TIM20BKIN2) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM1IC2)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC3)                        ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM17BKIN)                      ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM17OCREFCLR))))

#define IS_COMP_BLANKINGSRCE(SOURCE) (((SOURCE) == COMP_BLANKINGSRCE_NONE)     || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM1OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC3)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM2OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM8OC5)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM3OC4)  || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC1) || \
                                      ((SOURCE) == COMP_BLANKINGSRCE_TIM15OC2))

/* STM32F303xE/STM32F398xx/STM32F303xB/STM32F303xC/STM32F358xx devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   (((((INSTANCE) == COMP1) || ((INSTANCE) == COMP2))  &&     \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC3) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))         \
    ||                                                        \
    (((INSTANCE) == COMP3) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4)))         \
    ||                                                        \
    (((INSTANCE) == COMP4) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC1)))        \
    ||                                                        \
    (((INSTANCE) == COMP5) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM3OC3)))         \
    ||                                                        \
    (((INSTANCE) == COMP6) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM2OC4) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2)))        \
    ||                                                        \
    (((INSTANCE) == COMP7) &&                                 \
     (((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE)    ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM1OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM8OC5) ||        \
      ((BLANKINGSRCE) == COMP_BLANKINGSRCE_TIM15OC2))))

#endif /* STM32F303xE || STM32F398xx */

#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined(STM32F373xC) || defined(STM32F378xx)

#define IS_COMP_INVERTINGINPUT(INPUT) (((INPUT) == COMP_INVERTINGINPUT_1_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_1_2VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_3_4VREFINT)       || \
                                       ((INPUT) == COMP_INVERTINGINPUT_VREFINT)          || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH1)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC1_CH2)         || \
                                       ((INPUT) == COMP_INVERTINGINPUT_IO1)              || \
                                       ((INPUT) == COMP_INVERTINGINPUT_DAC2_CH1))

#define IS_COMP_NONINVERTINGINPUT(INPUT) (((INPUT) == COMP_NONINVERTINGINPUT_IO1) || \
                                          ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED))

/* STM32F373xB/xC, STM32F378xx devices comparator instances non inverting source values */
#define IS_COMP_NONINVERTINGINPUT_INSTANCE(INSTANCE, INPUT)    \
   ((((INSTANCE) == COMP1)  &&                                 \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)                || \
     ((INPUT) == COMP_NONINVERTINGINPUT_DAC1SWITCHCLOSED)))    \
    ||                                                         \
    (((INPUT) == COMP_NONINVERTINGINPUT_IO1)))

#define IS_COMP_WINDOWMODE(WINDOWMODE) (((WINDOWMODE) == COMP_WINDOWMODE_DISABLE) || \
                                        ((WINDOWMODE) == COMP_WINDOWMODE_ENABLE))

#define IS_COMP_MODE(MODE)  (((MODE) == COMP_MODE_HIGHSPEED)     || \
                             ((MODE) == COMP_MODE_MEDIUMSPEED)   || \
                             ((MODE) == COMP_MODE_LOWPOWER)      || \
                             ((MODE) == COMP_MODE_ULTRALOWPOWER))

#define IS_COMP_HYSTERESIS(HYSTERESIS)    (((HYSTERESIS) == COMP_HYSTERESIS_NONE)   || \
                                           ((HYSTERESIS) == COMP_HYSTERESIS_LOW)    || \
                                           ((HYSTERESIS) == COMP_HYSTERESIS_MEDIUM) || \
                                           ((HYSTERESIS) == COMP_HYSTERESIS_HIGH))

#define IS_COMP_OUTPUT(OUTPUT) (((OUTPUT) == COMP_OUTPUT_NONE)                || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_COMP1_TIM3IC1)       || \
                                ((OUTPUT) == COMP_OUTPUT_COMP1_TIM3OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_COMP2_TIM3IC1)       || \
                                ((OUTPUT) == COMP_OUTPUT_COMP2_TIM3OCREFCLR)  || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4IC1)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM4OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM5IC4)             || \
                                ((OUTPUT) == COMP_OUTPUT_TIM5OCREFCLR)        || \
                                ((OUTPUT) == COMP_OUTPUT_TIM15BKIN)           || \
                                ((OUTPUT) == COMP_OUTPUT_TIM16BKIN))

#define IS_COMP_OUTPUT_INSTANCE(INSTANCE, OUTPUT)      \
   ((((INSTANCE) == COMP1)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP1_TIM3IC1)      ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP1_TIM3OCREFCLR) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM5IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM5OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM15BKIN)))             \
    ||                                                 \
    (((INSTANCE) == COMP2)  &&                         \
    (((OUTPUT) == COMP_OUTPUT_NONE)               ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2IC4)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM2OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP2_TIM3IC1)      ||   \
     ((OUTPUT) == COMP_OUTPUT_COMP2_TIM3OCREFCLR) ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4IC1)            ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM4OCREFCLR)       ||   \
     ((OUTPUT) == COMP_OUTPUT_TIM16BKIN))))

#define IS_COMP_BLANKINGSRCE(SOURCE) ((SOURCE) == (SOURCE)) /*!< Not available: check always true */

/* STM32F373xB/STM32F373xC/STM32F378xx devices comparator instances blanking source values */
#define IS_COMP_BLANKINGSRCE_INSTANCE(INSTANCE, BLANKINGSRCE) \
   ((((INSTANCE) == COMP1) || ((INSTANCE) == COMP2))  &&     \
     ((BLANKINGSRCE) == COMP_BLANKINGSRCE_NONE))

#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F3xx_HAL_COMP_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
