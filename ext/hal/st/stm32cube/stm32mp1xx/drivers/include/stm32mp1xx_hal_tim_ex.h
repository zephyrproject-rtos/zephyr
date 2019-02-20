/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_tim_ex.h
  * @author  MCD Application Team
  * @brief   Header file of TIM HAL Extended module.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
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
#ifndef __STM32MP1xx_HAL_TIM_EX_H
#define __STM32MP1xx_HAL_TIM_EX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup TIMEx
  * @{
  */ 

/* Exported types ------------------------------------------------------------*/ 
/** @defgroup TIMEx_Exported_Types TIM Extended Exported Types
  * @{
  */

/** 
  * @brief  TIM Hall sensor Configuration Structure definition  
  */

typedef struct
{
                                  
  uint32_t IC1Polarity;         /*!< Specifies the active edge of the input signal.
                                        This parameter can be a value of @ref TIM_Input_Capture_Polarity */
                                                                   
  uint32_t IC1Prescaler;        /*!< Specifies the Input Capture Prescaler.
                                     This parameter can be a value of @ref TIM_Input_Capture_Prescaler */
                                  
  uint32_t IC1Filter;           /*!< Specifies the input capture filter.
                                     This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */  
  uint32_t Commutation_Delay;   /*!< Specifies the pulse value to be loaded into the Capture Compare Register. 
                                    This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF */                              
} TIM_HallSensor_InitTypeDef;

/** 
  * @brief  TIM Break/Break2 input configuration   
  */
typedef struct {
  uint32_t Source;         /*!< Specifies the source of the timer break input.
                                This parameter can be a value of @ref TIMEx_Break_Input_Source */
  uint32_t Enable;         /*!< Specifies whether or not the break input source is enabled.
                                This parameter can be a value of @ref TIMEx_Break_Input_Source_Enable */
  uint32_t Polarity;       /*!< Specifies the break input source polarity.
                                This parameter can be a value of @ref TIMEx_Break_Input_Source_Polarity
                                Not relevant when analog watchdog output of the DFSDM1 used as break input source */
} TIMEx_BreakInputConfigTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Constants TIM Extended Exported Constants
  * @{
  */

/** @defgroup TIMEx_Channel TIM Extended Channel
  * @{
  */
#define TIM_CHANNEL_1                      ((uint32_t)0x0000U)   /*!< TIM Channel 1*/
#define TIM_CHANNEL_2                      ((uint32_t)0x0004U)   /*!< TIM Channel 2*/
#define TIM_CHANNEL_3                      ((uint32_t)0x0008U)   /*!< TIM Channel 3*/
#define TIM_CHANNEL_4                      ((uint32_t)0x000CU)   /*!< TIM Channel 4*/
#define TIM_CHANNEL_5                      ((uint32_t)0x0010U)   /*!< TIM Channel 5*/
#define TIM_CHANNEL_6                      ((uint32_t)0x0014U)   /*!< TIM Channel 6*/
#define TIM_CHANNEL_ALL                    ((uint32_t)0x003CU)   /*!< TIM all Channels */
                                 
/**
  * @}
  */ 

/** @defgroup TIMEx_Output_Compare_and_PWM_modes TIM Output Compare and PWM Modes
  * @{
  */
#define TIM_OCMODE_TIMING                   ((uint32_t)0x0000U)                                                  /*!< TIM Output timing mode */
#define TIM_OCMODE_ACTIVE                   ((uint32_t)TIM_CCMR1_OC1M_0)                                         /*!< TIM Output Active mode */
#define TIM_OCMODE_INACTIVE                 ((uint32_t)TIM_CCMR1_OC1M_1)                                         /*!< TIM Output Inactive mode */
#define TIM_OCMODE_TOGGLE                   ((uint32_t)TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0)                      /*!< TIM Output Toggle mode */
#define TIM_OCMODE_PWM1                     ((uint32_t)TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1)                      /*!< TIM PWM mode 1 */
#define TIM_OCMODE_PWM2                     ((uint32_t)TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0)   /*!< TIM PWM mode 2 */
#define TIM_OCMODE_FORCED_ACTIVE            ((uint32_t)TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0)                      /*!< TIM Forced Active mode */   
#define TIM_OCMODE_FORCED_INACTIVE          ((uint32_t)TIM_CCMR1_OC1M_2)                                         /*!< TIM Forced Inactive mode */  

#define TIM_OCMODE_RETRIGERRABLE_OPM1      ((uint32_t)TIM_CCMR1_OC1M_3)                                           /*!< TIM Rettrigerrable OPM mode 1 */  
#define TIM_OCMODE_RETRIGERRABLE_OPM2      ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0)                        /*!< TIM Rettrigerrable OPM mode 2 */ 
#define TIM_OCMODE_COMBINED_PWM1           ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2)                        /*!< TIM Combined PWM mode 1 */ 
#define TIM_OCMODE_COMBINED_PWM2           ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_2)     /*!< TIM Combined PWM mode 2 */ 
#define TIM_OCMODE_ASSYMETRIC_PWM1         ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2)     /*!< TIM Asymetruc PWM mode 1 */    
#define TIM_OCMODE_ASSYMETRIC_PWM2         ((uint32_t)TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M)                          /*!< TIM Asymetruc PWM mode 2 */ 
/**
  * @}
  */

/** @defgroup TIMEx_ClearInput_Source TIM  Extended Clear Input Source
  * @{
  */
#define TIM_CLEARINPUTSOURCE_ETR            ((uint32_t)0x0001U)            /*!< TIM Clear input source connected to ETR */
#define TIM_CLEARINPUTSOURCE_OCREFCLR       ((uint32_t)0x0002U)            /*!< TIM Clear input source connected to OCREFClear */
#define TIM_CLEARINPUTSOURCE_NONE           ((uint32_t)0x0000U)            /*!< TIM Clear input source None */

/**
  * @}
  */

/** @defgroup TIMEx_Break2_Input_enable_disable  TIMEX Break input 2 Enable
  * @{
  */                         
#define TIM_BREAK2_DISABLE         ((uint32_t)0x00000000U)            /*!< TIM Break2 disabled */
#define TIM_BREAK2_ENABLE          ((uint32_t)TIM_BDTR_BK2E)          /*!< TIM Break2 enabled */ 

/**
  * @}
  */
/** @defgroup TIMEx_Break2_Polarity TIM  Extended Break Input 2 Polarity
  * @{
  */
#define TIM_BREAK2POLARITY_LOW        ((uint32_t)0x00000000U)          /*!< TIM Break2 polarity low */
#define TIM_BREAK2POLARITY_HIGH       ((uint32_t)TIM_BDTR_BK2P)        /*!< TIM Break2 polarity high */

/**
  * @}
  */
    
/** @defgroup TIMEx_Trigger_Selection TIM Trigger Selection
  * @{
  */
#define TIM_TS_ITR4                        ((uint32_t)0x0100000)       /*!< TIM Internal trigger 4 */
#define TIM_TS_ITR5                        ((uint32_t)0x0100010)       /*!< TIM Internal trigger 5 */ 
#define TIM_TS_ITR6                        ((uint32_t)0x0100020)       /*!< TIM Internal trigger 6 */
#define TIM_TS_ITR7                        ((uint32_t)0x0100030)       /*!< TIM Internal trigger 7 */
#define TIM_TS_ITR8                        ((uint32_t)0x0100040)       /*!< TIM Internal trigger 8 */


/**
  * @}
  */ 

/** @defgroup TIM_Event_Source TIM  Extended Event Source
  * @{
  */

#define TIM_EVENTSOURCE_UPDATE              TIM_EGR_UG     /*!< Reinitialize the counter and generates an update of the registers */
#define TIM_EVENTSOURCE_CC1                 TIM_EGR_CC1G   /*!< A capture/compare event is generated on channel 1 */
#define TIM_EVENTSOURCE_CC2                 TIM_EGR_CC2G   /*!< A capture/compare event is generated on channel 2 */
#define TIM_EVENTSOURCE_CC3                 TIM_EGR_CC3G   /*!< A capture/compare event is generated on channel 3 */
#define TIM_EVENTSOURCE_CC4                 TIM_EGR_CC4G   /*!< A capture/compare event is generated on channel 4 */
#define TIM_EVENTSOURCE_COM                 TIM_EGR_COMG   /*!< A commutation event is generated */
#define TIM_EVENTSOURCE_TRIGGER             TIM_EGR_TG     /*!< A trigger event is generated */
#define TIM_EVENTSOURCE_BREAK               TIM_EGR_BG     /*!< A break event is generated */
#define TIM_EVENTSOURCE_BREAK2              TIM_EGR_B2G    /*!< A break 2 event is generated */
                                         
/**
  * @}
  */ 

/** @defgroup TIM_DMA_Base_address TIM DMA Base Address
  * @{
  */
#define TIM_DMABASE_CR1                    (0x00000000U)              /*!< TIM DMA Base Address is CR1 */
#define TIM_DMABASE_CR2                    (0x00000001U)              /*!< TIM DMA Base Address is CR2 */
#define TIM_DMABASE_SMCR                   (0x00000002U)              /*!< TIM DMA Base Address is SMCR */
#define TIM_DMABASE_DIER                   (0x00000003U)              /*!< TIM DMA Base Address is DIER */
#define TIM_DMABASE_SR                     (0x00000004U)              /*!< TIM DMA Base Address is SR */
#define TIM_DMABASE_EGR                    (0x00000005U)              /*!< TIM DMA Base Address is EGR */
#define TIM_DMABASE_CCMR1                  (0x00000006U)              /*!< TIM DMA Base Address is CCMR1 */
#define TIM_DMABASE_CCMR2                  (0x00000007U)              /*!< TIM DMA Base Address is CCMR2*/
#define TIM_DMABASE_CCER                   (0x00000008U)              /*!< TIM DMA Base Address is CCER */
#define TIM_DMABASE_CNT                    (0x00000009U)              /*!< TIM DMA Base Address is CNT */ 
#define TIM_DMABASE_PSC                    (0x0000000AU)              /*!< TIM DMA Base Address is PSC */
#define TIM_DMABASE_ARR                    (0x0000000BU)              /*!< TIM DMA Base Address is ARR */
#define TIM_DMABASE_RCR                    (0x0000000CU)              /*!< TIM DMA Base Address is RCR */
#define TIM_DMABASE_CCR1                   (0x0000000DU)              /*!< TIM DMA Base Address is CCR1 */
#define TIM_DMABASE_CCR2                   (0x0000000EU)              /*!< TIM DMA Base Address is CCR2 */
#define TIM_DMABASE_CCR3                   (0x0000000FU)              /*!< TIM DMA Base Address is CCR3 */
#define TIM_DMABASE_CCR4                   (0x00000010U)              /*!< TIM DMA Base Address is CCR3 */
#define TIM_DMABASE_BDTR                   (0x00000011U)              /*!< TIM DMA Base Address is BDTR */
#define TIM_DMABASE_DCR                    (0x00000012U)              /*!< TIM DMA Base Address is DCR */
#define TIM_DMABASE_DMAR                   (0x00000013U)              /*!< TIM DMA Base Address is DMAR */ 
#define TIM_DMABASE_AF1                    (0x00000014U)              /*!< TIM DMA Base Address is AF1 */
#define TIM_DMABASE_CCMR3                  (0x00000015U)              /*!< TIM DMA Base Address is CCMR3 */
#define TIM_DMABASE_CCR5                   (0x00000016U)              /*!< TIM DMA Base Address is CCR5 */
#define TIM_DMABASE_CCR6                   (0x00000017U)              /*!< TIM DMA Base Address is CCR6 */
#define TIM_DMABASE_AF2                    (0x00000018U)              /*!< TIM DMA Base Address is AF2 */
#define TIM_DMABASE_AF3                    (0x00000019U)              /*!< TIM DMA Base Address is AF3 */
#define TIM_DMABASE_TISEL                  (0x0000001AU)              /*!< TIM DMA Base Address is TISEL */
/**
  * @}
  */ 

/** @defgroup TIMEx_Remap TIM  Extended Remapping
  * @{
  */
#define TIM_TIM1_ETR_GPIO                          (0x00000000) /* !< TIM1_ETR is connected to GPIO */
#define TIM_TIM1_ETR_ADC1_AWD1                     (0x0000C000) /* !< TIM1_ETR is connected to ADC1 AWD1 */
#define TIM_TIM1_ETR_ADC1_AWD2                     (0x00010000) /* !< TIM1_ETR is connected to ADC1 AWD2 */
#define TIM_TIM1_ETR_ADC1_AWD3                     (0x00014000) /* !< TIM1_ETR is connected to ADC1 AWD3 */
#define TIM_TIM1_ETR_ADC2_AWD1                     (0x00018000) /* !< TIM1_ETR is connected to ADC2 AWD1 */
#define TIM_TIM1_ETR_ADC2_AWD2                     (0x0001C000) /* !< TIM1_ETR is connected to ADC2 AWD2 */
#define TIM_TIM1_ETR_ADC2_AWD3                     (0x00020000) /* !< TIM1_ETR is connected to ADC2 AWD3 */

#define TIM_TIM8_ETR_GPIO                          (0x00000000) /* !< TIM8_ETR is connected to GPIO */
#define TIM_TIM8_ETR_ADC1_AWD1                     (0x0000C000) /* !< TIM8_ETR is connected to ADC1 AWD1 */
#define TIM_TIM8_ETR_ADC1_AWD2                     (0x00010000) /* !< TIM8_ETR is connected to ADC1 AWD2 */
#define TIM_TIM8_ETR_ADC1_AWD3                     (0x00014000) /* !< TIM8_ETR is connected to ADC1 AWD3 */
#define TIM_TIM8_ETR_ADC2_AWD1                     (0x00018000) /* !< TIM8_ETR is connected to ADC2 AWD1 */
#define TIM_TIM8_ETR_ADC2_AWD2                     (0x0001C000) /* !< TIM8_ETR is connected to ADC2 AWD2 */
#define TIM_TIM8_ETR_ADC2_AWD3                     (0x00020000) /* !< TIM8_ETR is connected to ADC2 AWD3 */

#define TIM_TIM2_ETR_GPIO                          (0x00000000) /* !< TIM2_ETR is connected to GPIO */
#define TIM_TIM2_ETR_RCC_LSE                       (0x0000C000) /* !< TIM2_ETR is connected to RCC LSE */
#define TIM_TIM2_ETR_SAI1_FSA                      (0x00010000) /* !< TIM2_ETR is connected to SAI1 FS_A */
#define TIM_TIM2_ETR_SAI1_FSB                      (0x00014000) /* !< TIM2_ETR is connected to SAI1 FS_B */
#define TIM_TIM2_ETR_ETH_PPS                       (0x00018000) /* !< TIM2_ETR is connected to ETH PPS */

#define TIM_TIM3_ETR_GPIO                          (0x00000000) /* !< TIM3_ETR is connected to GPIO */
#define TIM_TIM3_ETR_ETH_PPS                       (0x00018000) /* !< TIM3_ETR is connected to ETH PPS */

#define TIM_TIM4_ETR_GPIO                          (0x00000000) /* !< TIM4_ETR is connected to GPIO */

#define TIM_TIM5_ETR_GPIO                          (0x00000000) /* !< TIM5_ETR is connected to GPIO */
#define TIM_TIM5_ETR_SAI2_FSA                      (0x00004000) /* !< TIM5_ETR is connected to SAI2 FS_A */
#define TIM_TIM5_ETR_SAI2_FSB                      (0x00008000) /* !< TIM5_ETR is connected to SAI2 FS_B */
#define TIM_TIM5_ETR_OTG_SOF                       (0x0000C000) /* !< TIM5_ETR is connected to OTG SOF */

#define TIM_TIM1_TI1_GPIO                          (0x00000000) /* !< TIM1_TI1 is connected to GPIO */

#define TIM_TIM2_TI4_GPIO                          (0x00000000) /* !< TIM2_TI4 is connected to GPIO */

#define TIM_TIM5_TI1_GPIO                          (0x00000000) /* !< TIM5_TI1 is connected to GPIO */
#define TIM_TIM5_TI1_FDCAN1_TMP                    (0x00000001) /* !< TIM5_TI1 is connected to FDCAN1 TMP */
#define TIM_TIM5_TI1_FDCAN1_RTP                    (0x00000002) /* !< TIM5_TI1 is connected to FDCAN1 RTP */

#define TIM_TIM12_TI1_GPIO                         (0x00000000) /* !< TIM12_TI1 is connected to GPIO */
#define TIM_TIM12_TI1_HSI_CAL_CK                   (0x00000001) /* !< TIM12_TI1 is connected to HSI CAL CK */
#define TIM_TIM12_TI1_CSI_CAL_CK                   (0x00000002) /* !< TIM12_TI1 is connected to CSI CAL CK */

#define TIM_TIM15_TI1_GPIO                         (0x00000000) /* !< TIM15_TI1 is connected to GPIO */
#define TIM_TIM15_TI1_TIM2_CH1                     (0x00000001) /* !< TIM15_TI1 is connected to TIM2 CH1 */
#define TIM_TIM15_TI1_TIM3_CH1                     (0x00000002) /* !< TIM15_TI1 is connected to TIM3 CH1 */
#define TIM_TIM15_TI1_TIM4_CH1                     (0x00000003) /* !< TIM15_TI1 is connected to TIM4 CH1 */
#define TIM_TIM15_TI1_RCC_LSE                      (0x00000004) /* !< TIM15_TI1 is connected to RCC LSE */
#define TIM_TIM15_TI1_RCC_CSI                      (0x00000005) /* !< TIM15_TI1 is connected to RCC CSI */
#define TIM_TIM15_TI1_RCC_MCO2                     (0x00000006) /* !< TIM15_TI1 is connected to RCC MCO2 */
#define TIM_TIM15_TI1_HSI_CAL_CK                   (0x00000007) /* !< TIM15_TI1 is connected to HSI CAL CK */
#define TIM_TIM15_TI1_CSI_CAL_CK                   (0x00000008) /* !< TIM15_TI1 is connected to CSI CAL CK */

#define TIM_TIM15_TI2_GPIO                         (0x00000000) /* !< TIM15_TI2 is connected to GPIO */
#define TIM_TIM15_TI2_TIM2_CH2                     (0x00000100) /* !< TIM15_TI2 is connected to TIM2 CH2 */
#define TIM_TIM15_TI2_TIM3_CH2                     (0x00000200) /* !< TIM15_TI2 is connected to TIM3 CH2 */
#define TIM_TIM15_TI2_TIM4_CH2                     (0x00000300) /* !< TIM15_TI2 is connected to TIM4 CH2 */

#define TIM_TIM16_TI1_GPIO                         (0x00000000) /* !< TIM16 TI1 is connected to GPIO */
#define TIM_TIM16_TI1_RCC_LSI                      (0x00000001) /* !< TIM16 TI1 is connected to RCC LSI */
#define TIM_TIM16_TI1_RCC_LSE                      (0x00000002) /* !< TIM16 TI1 is connected to RCC LSE */
#define TIM_TIM16_TI1_WKUP_IT                      (0x00000003) /* !< TIM16 TI1 is connected to WKUP_IT */

#define TIM_TIM17_TI1_GPIO                         (0x00000000) /* !< TIM17 TI1 is connected to GPIO */
#define TIM_TIM17_TI1_SPDIFRX_FS                   (0x00000001) /* !< TIM17 TI1 is connected to SPDIFRX FS */
#define TIM_TIM17_TI1_RCC_HSE_RTC                  (0x00000002) /* !< TIM17 TI1 is connected to RCC HSE RTC */
#define TIM_TIM17_TI1_RCC_MCO1                     (0x00000003) /* !< TIM17 TI1 is connected to RCC MCO1 */

/**
  * @}
  */ 

/** @defgroup TIMEx_Group_Channel5 Group Channel 5 and Channel 1, 2 or 3
  * @{
  */
#define TIM_GROUPCH5_NONE       (uint32_t)0x00000000  /* !< No effect of OC5REF on OC1REFC, OC2REFC and OC3REFC */
#define TIM_GROUPCH5_OC1REFC    (TIM_CCR5_GC5C1)      /* !< OC1REFC is the logical AND of OC1REFC and OC5REF */
#define TIM_GROUPCH5_OC2REFC    (TIM_CCR5_GC5C2)      /* !< OC2REFC is the logical AND of OC2REFC and OC5REF */
#define TIM_GROUPCH5_OC3REFC    (TIM_CCR5_GC5C3)       /* !< OC3REFC is the logical AND of OC3REFC and OC5REF */


/** @defgroup TIMEx_Break_Input TIM  Extended Break input
  * @{
  */
#define TIM_BREAKINPUT_BRK     ((uint32_t)(0x00000001)) /* !< Timer break input  */
#define TIM_BREAKINPUT_BRK2    ((uint32_t)(0x00000002)) /* !< Timer break2 input */
/**
  * @}
  */ 

/** @defgroup TIMEx_Break_Input_Source TIM  Extended Break input source
  * @{
  */
#define TIM_BREAKINPUTSOURCE_BKIN     ((uint32_t)(0x00000001)) /* !< An external source (GPIO) is connected to the BKIN pin  */
#define TIM_BREAKINPUTSOURCE_DFSDM1   ((uint32_t)(0x00000008)) /* !< The analog watchdog output of the DFSDM1 peripheral is connected to the break input */
/**
  * @}
  */ 

/** @defgroup TIMEx_Break_Input_Source_Enable TIM Extended Break input source enabling
  * @{
  */
#define TIM_BREAKINPUTSOURCE_DISABLE     ((uint32_t)(0x00000000)) /* !< Break input source is disabled */
#define TIM_BREAKINPUTSOURCE_ENABLE      ((uint32_t)(0x00000001)) /* !< Break input source is enabled */
/**
  * @}
  */ 

/** @defgroup TIMEx_Break_Input_Source_Polarity TIM  Extended Break input polarity
  * @{
  */
#define TIM_BREAKINPUTSOURCE_POLARITY_LOW     ((uint32_t)(0x00000001)) /* !< Break input source is active low */
#define TIM_BREAKINPUTSOURCE_POLARITY_HIGH    ((uint32_t)(0x00000000)) /* !< Break input source is active_high */
/**
  * @}
  */ 

/**
  * @}
  */
/**
  * @}
  */ 

/* Exported macro ------------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Macros TIM Extended Exported Macros
  * @{
  */  
   
/* Private macro -------------------------------------------------------------*/
/** @defgroup TIMEx_Private_Macros TIM Extended Private Macros
  * @{
 */	
#define IS_TIM_REMAP(__REMAP__)    (((__REMAP__) <= (uint32_t)0x0001C01F))

#define IS_TIM_BREAKINPUT(__BREAKINPUT__)  (((__BREAKINPUT__) == TIM_BREAKINPUT_BRK)  || \
                                            ((__BREAKINPUT__) == TIM_BREAKINPUT_BRK2))

#define IS_TIM_BREAKINPUTSOURCE(__SOURCE__)  (((__SOURCE__) == TIM_BREAKINPUTSOURCE_BKIN)  || \
                                              ((__SOURCE__) == TIM_BREAKINPUTSOURCE_DFSDM1))

#define IS_TIM_BREAKINPUTSOURCE_STATE(__STATE__)  (((__STATE__) == TIM_BREAKINPUTSOURCE_DISABLE)  || \
                                                   ((__STATE__) == TIM_BREAKINPUTSOURCE_ENABLE))

#define IS_TIM_BREAKINPUTSOURCE_POLARITY(__POLARITY__)  (((__POLARITY__) == TIM_BREAKINPUTSOURCE_POLARITY_LOW)  || \
                                                         ((__POLARITY__) == TIM_BREAKINPUTSOURCE_POLARITY_HIGH))

#define IS_TIM_TISEL(IS_TIM_TISEL) ((((IS_TIM_TISEL) & 0xF0F0F0F0U) == 0x00000000U))


#define IS_TIM_ETRREMAP(ETRREMAP)    (((ETRREMAP) == TIM_TIM1_ETR_GPIO)      ||\
                                      ((ETRREMAP) == TIM_TIM1_ETR_ADC1_AWD1) ||\
                                      ((ETRREMAP) == TIM_TIM1_ETR_ADC1_AWD2) ||\
                                      ((ETRREMAP) == TIM_TIM1_ETR_ADC1_AWD3) ||\
                                      ((ETRREMAP) == TIM_TIM8_ETR_GPIO)      ||\
                                      ((ETRREMAP) == TIM_TIM8_ETR_ADC2_AWD1) ||\
                                      ((ETRREMAP) == TIM_TIM8_ETR_ADC2_AWD2) ||\
                                      ((ETRREMAP) == TIM_TIM8_ETR_ADC2_AWD3) ||\
                                      ((ETRREMAP) == TIM_TIM2_ETR_GPIO)      ||\
                                      ((ETRREMAP) == TIM_TIM2_ETR_RCC_LSE)   ||\
                                      ((ETRREMAP) == TIM_TIM2_ETR_SAI1_FSA)  ||\
                                      ((ETRREMAP) == TIM_TIM2_ETR_SAI1_FSB)  ||\
                                      ((ETRREMAP) == TIM_TIM3_ETR_GPIO)      ||\
                                      ((ETRREMAP) == TIM_TIM5_ETR_GPIO)      ||\
                                      ((ETRREMAP) == TIM_TIM5_ETR_SAI2_FSA)  |\
                                      ((ETRREMAP) == TIM_TIM5_ETR_SAI2_FSB))
/**
  * @}
  */ 
/* End of private macro ------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup TIMEx_Exported_Functions TIM Extended Exported Functions
  * @{
  */

/** @addtogroup TIMEx_Exported_Functions_Group1 Extended Timer Hall Sensor functions 
 *  @brief    Timer Hall Sensor functions
 * @{
 */
/*  Timer Hall Sensor functions  **********************************************/
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Init(TIM_HandleTypeDef *htim, TIM_HallSensor_InitTypeDef* sConfig);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_DeInit(TIM_HandleTypeDef *htim);

void HAL_TIMEx_HallSensor_MspInit(TIM_HandleTypeDef *htim);
void HAL_TIMEx_HallSensor_MspDeInit(TIM_HandleTypeDef *htim);

 /* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_IT(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_DMA(TIM_HandleTypeDef *htim, uint32_t *pData, uint16_t Length);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_DMA(TIM_HandleTypeDef *htim);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group2 Extended Timer Complementary Output Compare functions
 *  @brief   Timer Complementary Output Compare functions
 * @{
 */
/*  Timer Complementary Output Compare functions  *****************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData, uint16_t Length);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group3 Extended Timer Complementary PWM functions
 *  @brief    Timer Complementary PWM functions
 * @{
 */
/*  Timer Complementary PWM functions  ****************************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData, uint16_t Length);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group4 Extended Timer Complementary One Pulse functions
 *  @brief    Timer Complementary One Pulse functions
 * @{
 */
/*  Timer Complementary One Pulse functions  **********************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop(TIM_HandleTypeDef *htim, uint32_t OutputChannel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group5 Extended Peripheral Control functions
 *  @brief    Peripheral Control functions
 * @{
 */
/* Extended Control functions  ************************************************/
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutationEvent(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutationEvent_IT(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutationEvent_DMA(TIM_HandleTypeDef *htim, uint32_t  InputTrigger, uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim, TIM_MasterConfigTypeDef * sMasterConfig);
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakDeadTime(TIM_HandleTypeDef *htim, TIM_BreakDeadTimeConfigTypeDef *sBreakDeadTimeConfig);
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakInput(TIM_HandleTypeDef *htim, uint32_t BreakInput, TIMEx_BreakInputConfigTypeDef *sBreakInputConfig);
HAL_StatusTypeDef HAL_TIMEx_RemapConfig(TIM_HandleTypeDef *htim, uint32_t Remap);
HAL_StatusTypeDef  HAL_TIMEx_TISelection(TIM_HandleTypeDef *htim, uint32_t TISelection , uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_GroupChannel5(TIM_HandleTypeDef *htim, uint32_t Channels);

/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group6 Extended Callbacks functions 
  * @brief    Extended Callbacks functions
  * @{
  */
/* Extended Callback *********************************************************/
void HAL_TIMEx_CommutationCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_BreakCallback(TIM_HandleTypeDef *htim);

/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group7 Extended Peripheral State functions 
  * @brief    Extended Peripheral State functions
  * @{
  */
/* Extended Peripheral State functions  **************************************/
HAL_TIM_StateTypeDef HAL_TIMEx_HallSensor_GetState(TIM_HandleTypeDef *htim);
/**
  * @}
  */
/* End of exported functions -------------------------------------------------*/
/**
  * @}
  */ 
/* Private functions----------------------------------------------------------*/
/** @defgroup TIMEx_Private_Functions TIMEx Private Functions
* @{
*/
void HAL_TIMEx_DMACommutationCplt(DMA_HandleTypeDef *hdma);
/**
* @}
*/ 
/* End of private functions --------------------------------------------------*/
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

#endif /* __STM32MP1xx_HAL_TIM_EX_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
