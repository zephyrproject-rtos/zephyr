/**
  ******************************************************************************
  * @file    stm32g0xx_ll_adc.c
  * @author  MCD Application Team
  * @brief   ADC LL module driver
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_ll_adc.h"
#include "stm32g0xx_ll_bus.h"

#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32G0xx_LL_Driver
  * @{
  */

#if defined (ADC1)

/** @addtogroup ADC_LL ADC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @addtogroup ADC_LL_Private_Constants
  * @{
  */

/* Definitions of ADC hardware constraints delays */
/* Note: Only ADC IP HW delays are defined in ADC LL driver driver,           */
/*       not timeout values:                                                  */
/*       Timeout values for ADC operations are dependent to device clock      */
/*       configuration (system clock versus ADC clock),                       */
/*       and therefore must be defined in user application.                   */
/*       Refer to @ref ADC_LL_EC_HW_DELAYS for description of ADC timeout     */
/*       values definition.                                                   */
/* Note: ADC timeout values are defined here in CPU cycles to be independent  */
/*       of device clock setting.                                             */
/*       In user application, ADC timeout values should be defined with       */
/*       temporal values, in function of device clock settings.               */
/*       Highest ratio CPU clock frequency vs ADC clock frequency:            */
/*        - ADC clock from synchronous clock with AHB prescaler 512,          */
/*          APB prescaler 16, ADC prescaler 4.                                */
/*        - ADC clock from asynchronous clock (HSI) with prescaler 1,         */
/*          with highest ratio CPU clock frequency vs HSI clock frequency:    */
/*          CPU clock frequency max 56MHz, HSI frequency 16MHz: ratio 4.      */
/* Unit: CPU cycles.                                                          */
#define ADC_CLOCK_RATIO_VS_CPU_HIGHEST          (512UL * 16UL * 4UL)
#define ADC_TIMEOUT_DISABLE_CPU_CYCLES          (ADC_CLOCK_RATIO_VS_CPU_HIGHEST * 1UL)
#define ADC_TIMEOUT_STOP_CONVERSION_CPU_CYCLES  (ADC_CLOCK_RATIO_VS_CPU_HIGHEST * 1UL)
/* Note: CCRDY handshake requires 1APB + 2 ADC + 3 APB cycles                 */
/*       after the channel configuration has been changed.                    */
/*       Driver timeout is approximated to 6 CPU cycles.                      */
#define ADC_TIMEOUT_CCRDY_CPU_CYCLES            (ADC_CLOCK_RATIO_VS_CPU_HIGHEST * 6UL)

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/

/** @addtogroup ADC_LL_Private_Macros
  * @{
  */

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* common to several ADC instances.                                           */
#define IS_LL_ADC_COMMON_CLOCK(__CLOCK__)                                      \
  (   ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV1)                                 \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV2)                                 \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV4)                                 \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV6)                                 \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV8)                                 \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV10)                                \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV12)                                \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV16)                                \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV32)                                \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV64)                                \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV128)                               \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV256)                               \
  )

#define IS_LL_ADC_CLOCK_FREQ_MODE(__CLOCK_FREQ_MODE__)                         \
  (   ((__CLOCK_FREQ_MODE__) == LL_ADC_CLOCK_FREQ_MODE_HIGH)                   \
   || ((__CLOCK_FREQ_MODE__) == LL_ADC_CLOCK_FREQ_MODE_LOW)                    \
  )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC instance.                                                              */
#define IS_LL_ADC_CLOCK(__CLOCK__)                                             \
  (   ((__CLOCK__) == LL_ADC_CLOCK_SYNC_PCLK_DIV4)                             \
   || ((__CLOCK__) == LL_ADC_CLOCK_SYNC_PCLK_DIV2)                             \
   || ((__CLOCK__) == LL_ADC_CLOCK_SYNC_PCLK_DIV1)                             \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC)                                      \
  )

#define IS_LL_ADC_RESOLUTION(__RESOLUTION__)                                   \
  (   ((__RESOLUTION__) == LL_ADC_RESOLUTION_12B)                              \
   || ((__RESOLUTION__) == LL_ADC_RESOLUTION_10B)                              \
   || ((__RESOLUTION__) == LL_ADC_RESOLUTION_8B)                               \
   || ((__RESOLUTION__) == LL_ADC_RESOLUTION_6B)                               \
  )

#define IS_LL_ADC_DATA_ALIGN(__DATA_ALIGN__)                                   \
  (   ((__DATA_ALIGN__) == LL_ADC_DATA_ALIGN_RIGHT)                            \
   || ((__DATA_ALIGN__) == LL_ADC_DATA_ALIGN_LEFT)                             \
  )

#define IS_LL_ADC_LOW_POWER(__LOW_POWER__)                                     \
  (   ((__LOW_POWER__) == LL_ADC_LP_MODE_NONE)                                 \
   || ((__LOW_POWER__) == LL_ADC_LP_AUTOWAIT)                                  \
   || ((__LOW_POWER__) == LL_ADC_LP_AUTOPOWEROFF)                              \
   || ((__LOW_POWER__) == LL_ADC_LP_AUTOWAIT_AUTOPOWEROFF)                     \
  )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC group regular                                                          */
#if defined(TIM15) && defined(TIM6) && defined(TIM2)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH4 )                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
  )
#elif defined(TIM15) && defined(TIM6)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH4 )                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
  )
#elif defined(TIM2)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH4 )                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
  )
#else
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH4 )                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
  )
#endif

#define IS_LL_ADC_REG_CONTINUOUS_MODE(__REG_CONTINUOUS_MODE__)                 \
  (   ((__REG_CONTINUOUS_MODE__) == LL_ADC_REG_CONV_SINGLE)                    \
   || ((__REG_CONTINUOUS_MODE__) == LL_ADC_REG_CONV_CONTINUOUS)                \
  )

#define IS_LL_ADC_REG_DMA_TRANSFER(__REG_DMA_TRANSFER__)                       \
  (   ((__REG_DMA_TRANSFER__) == LL_ADC_REG_DMA_TRANSFER_NONE)                 \
   || ((__REG_DMA_TRANSFER__) == LL_ADC_REG_DMA_TRANSFER_LIMITED)              \
   || ((__REG_DMA_TRANSFER__) == LL_ADC_REG_DMA_TRANSFER_UNLIMITED)            \
  )

#define IS_LL_ADC_REG_OVR_DATA_BEHAVIOR(__REG_OVR_DATA_BEHAVIOR__)             \
  (   ((__REG_OVR_DATA_BEHAVIOR__) == LL_ADC_REG_OVR_DATA_PRESERVED)           \
   || ((__REG_OVR_DATA_BEHAVIOR__) == LL_ADC_REG_OVR_DATA_OVERWRITTEN)         \
  )

#define IS_LL_ADC_REG_SEQ_MODE(__REG_SEQ_MODE__)                               \
  (   ((__REG_SEQ_MODE__) == LL_ADC_REG_SEQ_FIXED)                             \
   || ((__REG_SEQ_MODE__) == LL_ADC_REG_SEQ_CONFIGURABLE)                      \
  )

#define IS_LL_ADC_REG_SEQ_SCAN_LENGTH(__REG_SEQ_SCAN_LENGTH__)                 \
  (   ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_DISABLE)               \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS)         \
  )

#define IS_LL_ADC_REG_SEQ_SCAN_DISCONT_MODE(__REG_SEQ_DISCONT_MODE__)          \
  (   ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_DISABLE)           \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_1RANK)             \
  )

/**
  * @}
  */


/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup ADC_LL_Exported_Functions
  * @{
  */

/** @addtogroup ADC_LL_EF_Init
  * @{
  */

/**
  * @brief  De-initialize registers of all ADC instances belonging to
  *         the same ADC common instance to their default reset values.
  * @note   This function is performing a hard reset, using high level
  *         clock source RCC ADC reset.
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC common registers are de-initialized
  *          - ERROR: not applicable
  */
ErrorStatus LL_ADC_CommonDeInit(ADC_Common_TypeDef *ADCxy_COMMON)
{
  /* Check the parameters */
  assert_param(IS_ADC_COMMON_INSTANCE(ADCxy_COMMON));
  
  /* Force reset of ADC clock (core clock) */
  LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_ADC);
  
  /* Release reset of ADC clock (core clock) */
  LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_ADC);
  
  return SUCCESS;
}

/**
  * @brief  Initialize some features of ADC common parameters
  *         (all ADC instances belonging to the same ADC common instance)
  *         and multimode (for devices with several ADC instances available).
  * @note   The setting of ADC common parameters is conditioned to
  *         ADC instances state:
  *         All ADC instances belonging to the same ADC common instance
  *         must be disabled.
  * @param  ADCxy_COMMON ADC common instance
  *         (can be set directly from CMSIS definition or by using helper macro @ref __LL_ADC_COMMON_INSTANCE() )
  * @param  ADC_CommonInitStruct Pointer to a @ref LL_ADC_CommonInitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC common registers are initialized
  *          - ERROR: ADC common registers are not initialized
  */
ErrorStatus LL_ADC_CommonInit(ADC_Common_TypeDef *ADCxy_COMMON, LL_ADC_CommonInitTypeDef *ADC_CommonInitStruct)
{
  ErrorStatus status = SUCCESS;
  
  /* Check the parameters */
  assert_param(IS_ADC_COMMON_INSTANCE(ADCxy_COMMON));
  assert_param(IS_LL_ADC_COMMON_CLOCK(ADC_CommonInitStruct->CommonClock));
  
  /* Note: Hardware constraint (refer to description of functions             */
  /*       "LL_ADC_SetCommonXXX()":                                           */
  /*       On this STM32 serie, setting of these features is conditioned to   */
  /*       ADC state:                                                         */
  /*       All ADC instances of the ADC common group must be disabled.        */
  if(__LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE(ADCxy_COMMON) == 0UL)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - common to several ADC                                               */
    /*    (all ADC instances belonging to the same ADC common instance)       */
    /*    - Set ADC clock (conversion clock)                                  */
    LL_ADC_SetCommonClock(ADCxy_COMMON, ADC_CommonInitStruct->CommonClock);
  }
  else
  {
    /* Initialization error: One or several ADC instances belonging to        */
    /* the same ADC common instance are not disabled.                         */
    status = ERROR;
  }
  
  return status;
}

/**
  * @brief  Set each @ref LL_ADC_CommonInitTypeDef field to default value.
  * @param  ADC_CommonInitStruct Pointer to a @ref LL_ADC_CommonInitTypeDef structure
  *                              whose fields will be set to default values.
  * @retval None
  */
void LL_ADC_CommonStructInit(LL_ADC_CommonInitTypeDef *ADC_CommonInitStruct)
{
  /* Set ADC_CommonInitStruct fields to default values */
  /* Set fields of ADC common */
  /* (all ADC instances belonging to the same ADC common instance) */
  ADC_CommonInitStruct->CommonClock = LL_ADC_CLOCK_ASYNC_DIV2;
  
}

/**
  * @brief  De-initialize registers of the selected ADC instance
  *         to their default reset values.
  * @note   To reset all ADC instances quickly (perform a hard reset),
  *         use function @ref LL_ADC_CommonDeInit().
  * @note   If this functions returns error status, it means that ADC instance
  *         is in an unknown state.
  *         In this case, perform a hard reset using high level
  *         clock source RCC ADC reset.
  *         Refer to function @ref LL_ADC_CommonDeInit().
  * @param  ADCx ADC instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are de-initialized
  *          - ERROR: ADC registers are not de-initialized
  */
ErrorStatus LL_ADC_DeInit(ADC_TypeDef *ADCx)
{
  ErrorStatus status = SUCCESS;
  
  __IO uint32_t timeout_cpu_cycles = 0UL;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));
  
  /* Disable ADC instance if not already disabled.                            */
  if(LL_ADC_IsEnabled(ADCx) == 1UL)
  {
    /* Set ADC group regular trigger source to SW start to ensure to not      */
    /* have an external trigger event occurring during the conversion stop    */
    /* ADC disable process.                                                   */
    LL_ADC_REG_SetTriggerSource(ADCx, LL_ADC_REG_TRIG_SOFTWARE);
    
    /* Stop potential ADC conversion on going on ADC group regular.           */
    if(LL_ADC_REG_IsConversionOngoing(ADCx) != 0UL)
    {
      if(LL_ADC_REG_IsStopConversionOngoing(ADCx) == 0UL)
      {
        LL_ADC_REG_StopConversion(ADCx);
      }
    }
    
    /* Wait for ADC conversions are effectively stopped                       */
    timeout_cpu_cycles = ADC_TIMEOUT_STOP_CONVERSION_CPU_CYCLES;
    while (LL_ADC_REG_IsStopConversionOngoing(ADCx) == 1UL)
    {
      timeout_cpu_cycles--;
      if(timeout_cpu_cycles == 0UL)
      {
        /* Time-out error */
        status = ERROR;
      }
    }
    
    /* Disable the ADC instance */
    LL_ADC_Disable(ADCx);
    
    /* Wait for ADC instance is effectively disabled */
    timeout_cpu_cycles = ADC_TIMEOUT_DISABLE_CPU_CYCLES;
    while (LL_ADC_IsDisableOngoing(ADCx) == 1UL)
    {
      timeout_cpu_cycles--;
      if(timeout_cpu_cycles == 0UL)
      {
        /* Time-out error */
        status = ERROR;
      }
    }
  }
  
  /* Check whether ADC state is compliant with expected state */
  if(READ_BIT(ADCx->CR,
              (  ADC_CR_ADSTP | ADC_CR_ADSTART
               | ADC_CR_ADDIS | ADC_CR_ADEN   )
             )
     == 0UL)
  {
    /* ========== Reset ADC registers ========== */
    /* Reset register IER */
    CLEAR_BIT(ADCx->IER,
              (  LL_ADC_IT_ADRDY
               | LL_ADC_IT_EOC
               | LL_ADC_IT_EOS
               | LL_ADC_IT_OVR
               | LL_ADC_IT_EOSMP
               | LL_ADC_IT_AWD1
               | LL_ADC_IT_AWD2
               | LL_ADC_IT_AWD3
               | LL_ADC_IT_EOCAL
               | LL_ADC_IT_CCRDY
              )
             );
    
    /* Reset register ISR */
    SET_BIT(ADCx->ISR,
            (  LL_ADC_FLAG_ADRDY
             | LL_ADC_FLAG_EOC
             | LL_ADC_FLAG_EOS
             | LL_ADC_FLAG_OVR
             | LL_ADC_FLAG_EOSMP
             | LL_ADC_FLAG_AWD1
             | LL_ADC_FLAG_AWD2
             | LL_ADC_FLAG_AWD3
             | LL_ADC_FLAG_EOCAL
             | LL_ADC_FLAG_CCRDY
            )
           );
    
    /* Reset register CR */
    /* Bits ADC_CR_ADCAL, ADC_CR_ADSTP, ADC_CR_ADSTART are in access mode     */
    /* "read-set": no direct reset applicable.                                */
    CLEAR_BIT(ADCx->CR, ADC_CR_ADVREGEN);
    
    /* Reset register CFGR1 */
    CLEAR_BIT(ADCx->CFGR1,
              (  ADC_CFGR1_AWD1CH  | ADC_CFGR1_AWD1EN | ADC_CFGR1_AWD1SGL | ADC_CFGR1_DISCEN
               | ADC_CFGR1_AUTOFF  | ADC_CFGR1_WAIT   | ADC_CFGR1_CONT    | ADC_CFGR1_OVRMOD
               | ADC_CFGR1_EXTEN   | ADC_CFGR1_EXTSEL | ADC_CFGR1_ALIGN   | ADC_CFGR1_RES
               | ADC_CFGR1_SCANDIR | ADC_CFGR1_DMACFG | ADC_CFGR1_DMAEN                     )
             );
    
    /* Reset register CFGR2 */
    /* Note: Update of ADC clock mode is conditioned to ADC state disabled:   */
    /*       already done above.                                              */
    CLEAR_BIT(ADCx->CFGR2,
              (  ADC_CFGR2_CKMODE
               | ADC_CFGR2_TOVS   | ADC_CFGR2_OVSS  | ADC_CFGR2_OVSR
               | ADC_CFGR2_OVSE   | ADC_CFGR2_CKMODE                )
             );
    
    /* Reset register SMPR */
    CLEAR_BIT(ADCx->SMPR, ADC_SMPR_SMP1 | ADC_SMPR_SMP2 | ADC_SMPR_SMPSEL);

    /* Reset register TR1 */
    MODIFY_REG(ADCx->TR1, ADC_TR1_HT1 | ADC_TR1_LT1, ADC_TR1_HT1);
    
    /* Reset register TR2 */
    MODIFY_REG(ADCx->TR2, ADC_TR2_HT2 | ADC_TR2_LT2, ADC_TR2_HT2);
    
    /* Reset register TR3 */
    MODIFY_REG(ADCx->TR3, ADC_TR3_HT3 | ADC_TR3_LT3, ADC_TR3_HT3);
    
    /* Reset register CHSELR */
    CLEAR_BIT(ADCx->CHSELR,
              (  ADC_CHSELR_CHSEL18 | ADC_CHSELR_CHSEL17 | ADC_CHSELR_CHSEL16
               | ADC_CHSELR_CHSEL15 | ADC_CHSELR_CHSEL14 | ADC_CHSELR_CHSEL13 | ADC_CHSELR_CHSEL12
               | ADC_CHSELR_CHSEL11 | ADC_CHSELR_CHSEL10 | ADC_CHSELR_CHSEL9  | ADC_CHSELR_CHSEL8
               | ADC_CHSELR_CHSEL7  | ADC_CHSELR_CHSEL6  | ADC_CHSELR_CHSEL5  | ADC_CHSELR_CHSEL4
               | ADC_CHSELR_CHSEL3  | ADC_CHSELR_CHSEL2  | ADC_CHSELR_CHSEL1  | ADC_CHSELR_CHSEL0 )
             );
    
    /* Wait for ADC channel configuration ready */
    timeout_cpu_cycles = ADC_TIMEOUT_CCRDY_CPU_CYCLES;
    while (LL_ADC_IsActiveFlag_CCRDY(ADCx) == 0UL)
    {
      timeout_cpu_cycles--;
      if(timeout_cpu_cycles == 0UL)
      {
        /* Time-out error */
        status = ERROR;
      }
    }
    
    /* Clear flag ADC channel configuration ready */
    LL_ADC_ClearFlag_CCRDY(ADCx);
    
    /* Reset register DR */
    /* bits in access mode read only, no direct reset applicable */
    
    /* Reset register CALFACT */
    CLEAR_BIT(ADCx->CALFACT, ADC_CALFACT_CALFACT);
    
  }
  else
  {
    /* ADC instance is in an unknown state */
    /* Need to performing a hard reset of ADC instance, using high level      */
    /* clock source RCC ADC reset.                                            */
    /* Caution: On this STM32 serie, if several ADC instances are available   */
    /*          on the selected device, RCC ADC reset will reset              */
    /*          all ADC instances belonging to the common ADC instance.       */
    status = ERROR;
  }
  
  return status;
}

/**
  * @brief  Initialize some features of ADC instance.
  * @note   These parameters have an impact on ADC scope: ADC instance.
  *         Refer to corresponding unitary functions into
  *         @ref ADC_LL_EF_Configuration_ADC_Instance .
  * @note   The setting of these parameters by function @ref LL_ADC_Init()
  *         is conditioned to ADC state:
  *         ADC instance must be disabled.
  *         This condition is applied to all ADC features, for efficiency
  *         and compatibility over all STM32 families. However, the different
  *         features can be set under different ADC state conditions
  *         (setting possible with ADC enabled without conversion on going,
  *         ADC enabled with conversion on going, ...)
  *         Each feature can be updated afterwards with a unitary function
  *         and potentially with ADC in a different state than disabled,
  *         refer to description of each function for setting
  *         conditioned to ADC state.
  * @note   After using this function, some other features must be configured
  *         using LL unitary functions.
  *         The minimum configuration remaining to be done is:
  *          - Set ADC group regular sequencer:
  *            Depending on the sequencer mode (refer to
  *            function @ref LL_ADC_REG_SetSequencerConfigurable() ):
  *            - map channel on the selected sequencer rank.
  *              Refer to function @ref LL_ADC_REG_SetSequencerRanks();
  *            - map channel on rank corresponding to channel number.
  *              Refer to function @ref LL_ADC_REG_SetSequencerChannels();
  *          - Set ADC channel sampling time
  *            Refer to function LL_ADC_SetSamplingTimeCommonChannels();
  *            Refer to function LL_ADC_SetChannelSamplingTime();
  * @param  ADCx ADC instance
  * @param  ADC_InitStruct Pointer to a @ref LL_ADC_REG_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are initialized
  *          - ERROR: ADC registers are not initialized
  */
ErrorStatus LL_ADC_Init(ADC_TypeDef *ADCx, LL_ADC_InitTypeDef *ADC_InitStruct)
{
  ErrorStatus status = SUCCESS;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));
  
  assert_param(IS_LL_ADC_CLOCK(ADC_InitStruct->Clock));
  assert_param(IS_LL_ADC_RESOLUTION(ADC_InitStruct->Resolution));
  assert_param(IS_LL_ADC_DATA_ALIGN(ADC_InitStruct->DataAlignment));
  assert_param(IS_LL_ADC_LOW_POWER(ADC_InitStruct->LowPowerMode));
  
  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0UL)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC instance                                                        */
    /*    - Set ADC data resolution                                           */
    /*    - Set ADC conversion data alignment                                 */
    /*    - Set ADC low power mode                                            */
    MODIFY_REG(ADCx->CFGR1,
                 ADC_CFGR1_RES
               | ADC_CFGR1_ALIGN
               | ADC_CFGR1_WAIT
               | ADC_CFGR1_AUTOFF
              ,
                 ADC_InitStruct->Resolution
               | ADC_InitStruct->DataAlignment
               | ADC_InitStruct->LowPowerMode
              );
    
  }
  else
  {
    /* Initialization error: ADC instance is not disabled. */
    status = ERROR;
  }
  return status;
}

/**
  * @brief  Set each @ref LL_ADC_InitTypeDef field to default value.
  * @param  ADC_InitStruct Pointer to a @ref LL_ADC_InitTypeDef structure
  *                        whose fields will be set to default values.
  * @retval None
  */
void LL_ADC_StructInit(LL_ADC_InitTypeDef *ADC_InitStruct)
{
  /* Set ADC_InitStruct fields to default values */
  /* Set fields of ADC instance */
  ADC_InitStruct->Clock         = LL_ADC_CLOCK_SYNC_PCLK_DIV2;
  ADC_InitStruct->Resolution    = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct->DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  ADC_InitStruct->LowPowerMode  = LL_ADC_LP_MODE_NONE;
  
}

/**
  * @brief  Initialize some features of ADC group regular.
  * @note   These parameters have an impact on ADC scope: ADC group regular.
  *         Refer to corresponding unitary functions into
  *         @ref ADC_LL_EF_Configuration_ADC_Group_Regular
  *         (functions with prefix "REG").
  * @note   The setting of these parameters by function @ref LL_ADC_Init()
  *         is conditioned to ADC state:
  *         ADC instance must be disabled.
  *         This condition is applied to all ADC features, for efficiency
  *         and compatibility over all STM32 families. However, the different
  *         features can be set under different ADC state conditions
  *         (setting possible with ADC enabled without conversion on going,
  *         ADC enabled with conversion on going, ...)
  *         Each feature can be updated afterwards with a unitary function
  *         and potentially with ADC in a different state than disabled,
  *         refer to description of each function for setting
  *         conditioned to ADC state.
  * @note   After using this function, other features must be configured
  *         using LL unitary functions.
  *         The minimum configuration remaining to be done is:
  *          - Set ADC group regular sequencer:
  *            Depending on the sequencer mode (refer to
  *            function @ref LL_ADC_REG_SetSequencerConfigurable() ):
  *            - map channel on the selected sequencer rank.
  *              Refer to function @ref LL_ADC_REG_SetSequencerRanks();
  *            - map channel on rank corresponding to channel number.
  *              Refer to function @ref LL_ADC_REG_SetSequencerChannels();
  *          - Set ADC channel sampling time
  *            Refer to function LL_ADC_SetSamplingTimeCommonChannels();
  *            Refer to function LL_ADC_SetChannelSamplingTime();
  * @param  ADCx ADC instance
  * @param  ADC_REG_InitStruct Pointer to a @ref LL_ADC_REG_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are initialized
  *          - ERROR: ADC registers are not initialized
  */
ErrorStatus LL_ADC_REG_Init(ADC_TypeDef *ADCx, LL_ADC_REG_InitTypeDef *ADC_REG_InitStruct)
{
  ErrorStatus status = SUCCESS;
  
  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));
  assert_param(IS_LL_ADC_REG_TRIG_SOURCE(ADC_REG_InitStruct->TriggerSource));
  assert_param(IS_LL_ADC_REG_SEQ_SCAN_LENGTH(ADC_REG_InitStruct->SequencerLength));
  if(ADC_REG_InitStruct->SequencerLength != LL_ADC_REG_SEQ_SCAN_DISABLE)
  {
    assert_param(IS_LL_ADC_REG_SEQ_SCAN_DISCONT_MODE(ADC_REG_InitStruct->SequencerDiscont));
  }
  assert_param(IS_LL_ADC_REG_CONTINUOUS_MODE(ADC_REG_InitStruct->ContinuousMode));
  assert_param(IS_LL_ADC_REG_DMA_TRANSFER(ADC_REG_InitStruct->DMATransfer));
  assert_param(IS_LL_ADC_REG_OVR_DATA_BEHAVIOR(ADC_REG_InitStruct->Overrun));
  
  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0UL)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC group regular                                                   */
    /*    - Set ADC group regular trigger source                              */
    /*    - Set ADC group regular sequencer length                            */
    /*    - Set ADC group regular sequencer discontinuous mode                */
    /*    - Set ADC group regular continuous mode                             */
    /*    - Set ADC group regular conversion data transfer: no transfer or    */
    /*      transfer by DMA, and DMA requests mode                            */
    /*    - Set ADC group regular overrun behavior                            */
    /* Note: On this STM32 serie, ADC trigger edge is set to value 0x0 by     */
    /*       setting of trigger source to SW start.                           */
    if(   (LL_ADC_REG_GetSequencerConfigurable(ADCx) == LL_ADC_REG_SEQ_FIXED)
       || (ADC_REG_InitStruct->SequencerLength != LL_ADC_REG_SEQ_SCAN_DISABLE)
      )
    {
      MODIFY_REG(ADCx->CFGR1,
                   ADC_CFGR1_EXTSEL
                 | ADC_CFGR1_EXTEN
                 | ADC_CFGR1_DISCEN
                 | ADC_CFGR1_CONT
                 | ADC_CFGR1_DMAEN
                 | ADC_CFGR1_DMACFG
                 | ADC_CFGR1_OVRMOD
                ,
                   ADC_REG_InitStruct->TriggerSource
                 | ADC_REG_InitStruct->SequencerDiscont
                 | ADC_REG_InitStruct->ContinuousMode
                 | ADC_REG_InitStruct->DMATransfer
                 | ADC_REG_InitStruct->Overrun
                );
    }
    else
    {
      MODIFY_REG(ADCx->CFGR1,
                   ADC_CFGR1_EXTSEL
                 | ADC_CFGR1_EXTEN
                 | ADC_CFGR1_DISCEN
                 | ADC_CFGR1_CONT
                 | ADC_CFGR1_DMAEN
                 | ADC_CFGR1_DMACFG
                 | ADC_CFGR1_OVRMOD
                ,
                   ADC_REG_InitStruct->TriggerSource
                 | LL_ADC_REG_SEQ_DISCONT_DISABLE
                 | ADC_REG_InitStruct->ContinuousMode
                 | ADC_REG_InitStruct->DMATransfer
                 | ADC_REG_InitStruct->Overrun
                );
    }

    /* Set ADC group regular sequencer length and scan direction */
    LL_ADC_REG_SetSequencerLength(ADCx, ADC_REG_InitStruct->SequencerLength);
  }
  else
  {
    /* Initialization error: ADC instance is not disabled. */
    status = ERROR;
  }
  return status;
}

/**
  * @brief  Set each @ref LL_ADC_REG_InitTypeDef field to default value.
  * @param  ADC_REG_InitStruct Pointer to a @ref LL_ADC_REG_InitTypeDef structure
  *                            whose fields will be set to default values.
  * @retval None
  */
void LL_ADC_REG_StructInit(LL_ADC_REG_InitTypeDef *ADC_REG_InitStruct)
{
  /* Set ADC_REG_InitStruct fields to default values */
  /* Set fields of ADC group regular */
  /* Note: On this STM32 serie, ADC trigger edge is set to value 0x0 by       */
  /*       setting of trigger source to SW start.                             */
  ADC_REG_InitStruct->TriggerSource    = LL_ADC_REG_TRIG_SOFTWARE;
  ADC_REG_InitStruct->SequencerLength  = LL_ADC_REG_SEQ_SCAN_DISABLE;
  ADC_REG_InitStruct->SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  ADC_REG_InitStruct->ContinuousMode   = LL_ADC_REG_CONV_SINGLE;
  ADC_REG_InitStruct->DMATransfer      = LL_ADC_REG_DMA_TRANSFER_NONE;
  ADC_REG_InitStruct->Overrun          = LL_ADC_REG_OVR_DATA_OVERWRITTEN;
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

#endif /* ADC1 */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
