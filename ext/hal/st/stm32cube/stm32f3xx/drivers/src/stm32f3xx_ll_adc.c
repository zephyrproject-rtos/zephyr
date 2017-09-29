/**
  ******************************************************************************
  * @file    stm32f3xx_ll_adc.c
  * @author  MCD Application Team
  * @brief   ADC LL module driver
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_ll_adc.h"
#include "stm32f3xx_ll_bus.h"

#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

/* Note: Devices of STM32F3 serie embed 1 out of 2 different ADC IP.   b      */
/*       - STM32F30x, STM32F31x, STM32F32x, STM32F33x, STM32F35x, STM32F39x:  */
/*         ADC IP 5Msamples/sec, from 1 to 4 ADC instances and other specific */
/*         features (refer to reference manual).                              */
/*       - STM32F37x:                                                         */
/*         ADC IP 1Msamples/sec, 1 ADC instance                               */
/*       This file contains the drivers of these ADC IP, located in 2 area    */
/*       delimited by compilation switches.                                   */

#if defined(ADC5_V1_1)

#if defined (ADC1) || defined (ADC2) || defined (ADC3) || defined (ADC4)

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
/*        - ADC clock from asynchronous clock (PLL) with prescaler 1,         */
/*          with highest ratio CPU clock frequency vs HSI clock frequency:    */
/*          CPU clock frequency max 72MHz, PLL frequency 72MHz: ratio 1.      */
/* Unit: CPU cycles.                                                          */
#define ADC_CLOCK_RATIO_VS_CPU_HIGHEST          ((uint32_t) 512U * 16U * 4U)
#define ADC_TIMEOUT_DISABLE_CPU_CYCLES          (ADC_CLOCK_RATIO_VS_CPU_HIGHEST * 1U)
#define ADC_TIMEOUT_STOP_CONVERSION_CPU_CYCLES  (ADC_CLOCK_RATIO_VS_CPU_HIGHEST * 1U)

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
  (   ((__CLOCK__) == LL_ADC_CLOCK_SYNC_PCLK_DIV1)                             \
   || ((__CLOCK__) == LL_ADC_CLOCK_SYNC_PCLK_DIV2)                             \
   || ((__CLOCK__) == LL_ADC_CLOCK_SYNC_PCLK_DIV4)                             \
   || ((__CLOCK__) == LL_ADC_CLOCK_ASYNC_DIV1)                                 \
  )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC instance.                                                              */
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
  )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC group regular                                                          */
#if defined(STM32F303xE) || defined(STM32F398xx)
#define IS_LL_ADC_REG_TRIG_SOURCE(__ADC_INSTANCE__, __REG_TRIG_SOURCE__)       \
  ((((__ADC_INSTANCE__) == ADC1) || ((__ADC_INSTANCE__) == ADC2))              \
    ? (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH1_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH2_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)              \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH2_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH4_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11_ADC12)     \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH4_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_TRG0_ADC12)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_TRG02_ADC12)     \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_CH1_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_CH2_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_CH3_ADC12)       \
      )                                                                        \
      :                                                                        \
      (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH3_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)              \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO__ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE2_ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO__ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO__ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM7_TRGO_ADC34)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_TRG0_ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_TRG02_ADC34)     \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM20_CH1_ADC34)       \
      )                                                                        \
  )
#elif defined(STM32F303xC) || defined(STM32F358xx)
#define IS_LL_ADC_REG_TRIG_SOURCE(__ADC_INSTANCE__, __REG_TRIG_SOURCE__)       \
  ((((__ADC_INSTANCE__) == ADC1) || ((__ADC_INSTANCE__) == ADC2))              \
    ? (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH1_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH2_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)              \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH2_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH4_ADC12)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11_ADC12)     \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO_ADC12)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH4_ADC12)        \
      )                                                                        \
      :                                                                        \
      (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH3_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)              \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO__ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE2_ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH1_ADC34)        \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO__ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO__ADC34)      \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_TRGO)             \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM7_TRGO_ADC34)       \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)            \
       || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH1_ADC34)        \
      )                                                                        \
  )
#elif defined(STM32F303x8) || defined(STM32F328xx)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH1)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH4)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM8_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH4)                  \
  )
#elif defined(STM32F334x8)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH1)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_HRTIM_TRG1)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_HRTIM_TRG3)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH4)                  \
  )
#elif defined(STM32F302xC) || defined(STM32F302xE)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH1)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH4)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_CH4)                  \
  )
#elif defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH1)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_CH3)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11)               \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM1_TRGO2)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM6_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM15_TRGO)                \
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

#define IS_LL_ADC_REG_SEQ_SCAN_LENGTH(__REG_SEQ_SCAN_LENGTH__)                 \
  (   ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_DISABLE)               \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_9RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_10RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_11RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_12RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_13RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_14RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_15RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_16RANKS)        \
  )

#define IS_LL_ADC_REG_SEQ_SCAN_DISCONT_MODE(__REG_SEQ_DISCONT_MODE__)          \
  (   ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_DISABLE)           \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_1RANK)             \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_2RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_3RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_4RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_5RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_6RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_7RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_8RANKS)            \
  )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC group injected                                                         */
#if defined(STM32F303xE) || defined(STM32F398xx)
#define IS_LL_ADC_INJ_TRIG_SOURCE(__ADC_INSTANCE__, __INJ_TRIG_SOURCE__)       \
  ((((__ADC_INSTANCE__) == ADC1) || ((__ADC_INSTANCE__) == ADC2))              \
    ? (   ((__INJ_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)              \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO_ADC12)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_CH1_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH4_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO_ADC12)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15_ADC12)     \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH4_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH3_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH1_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM6_TRGO_ADC12)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM20_TRGO_ADC12)      \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM20_TRGO2_ADC12)     \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM20_CH4_ADC12)       \
      )                                                                        \
      :                                                                        \
      (   ((__INJ_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)              \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_CH3_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH2_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH4__ADC34)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_CH4_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO__ADC34)      \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH3_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO__ADC34)      \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM7_TRGO_ADC34)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM20_TRG_ADC34)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM20_TRG2_ADC34)      \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM20_CH2)             \
      )                                                                        \
  )
#elif defined(STM32F303xC) || defined(STM32F358xx)
#define IS_LL_ADC_INJ_TRIG_SOURCE(__ADC_INSTANCE__, __INJ_TRIG_SOURCE__)       \
  ((((__ADC_INSTANCE__) == ADC1) || ((__ADC_INSTANCE__) == ADC2))              \
    ? (   ((__INJ_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)              \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO_ADC12)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_CH1_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH4_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO_ADC12)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15_ADC12)     \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH4_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH3_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH1_ADC12)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM6_TRGO_ADC12)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)            \
      )                                                                        \
      :                                                                        \
      (   ((__INJ_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                  \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)              \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_CH3_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH2_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH4__ADC34)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_CH4_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO__ADC34)      \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO2)            \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH3_ADC34)        \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)             \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO__ADC34)      \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM7_TRGO_ADC34)       \
       || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)            \
      )                                                                        \
  )

#elif defined(STM32F303x8) || defined(STM32F328xx)
#define IS_LL_ADC_INJ_TRIG_SOURCE(__INJ_TRIG_SOURCE__)                         \
  (   ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_SOFTWARE)                      \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15)               \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM8_TRGO2)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH3)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM6_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)                \
  )
#elif defined(STM32F334x8)
#define IS_LL_ADC_INJ_TRIG_SOURCE(__INJ_TRIG_SOURCE__)                         \
  (   ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_SOFTWARE)                      \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15)               \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_HRTIM_TRG2)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_HRTIM_TRG4)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH3)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM6_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)                \
  )
#elif defined(STM32F302xC) || defined(STM32F302xE)
#define IS_LL_ADC_INJ_TRIG_SOURCE(__INJ_TRIG_SOURCE__)                         \
  (   ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_SOFTWARE)                      \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15)               \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH3)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM6_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)                \
  )
#elif defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
#define IS_LL_ADC_INJ_TRIG_SOURCE(__INJ_TRIG_SOURCE__)                         \
  (   ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_SOFTWARE)                      \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15)               \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM1_TRGO2)                \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM6_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM15_TRGO)                \
  )
#endif

#define IS_LL_ADC_INJ_TRIG_EXT_EDGE(__INJ_TRIG_EXT_EDGE__)                     \
  (   ((__INJ_TRIG_EXT_EDGE__) == LL_ADC_INJ_TRIG_EXT_RISING)                  \
   || ((__INJ_TRIG_EXT_EDGE__) == LL_ADC_INJ_TRIG_EXT_FALLING)                 \
   || ((__INJ_TRIG_EXT_EDGE__) == LL_ADC_INJ_TRIG_EXT_RISINGFALLING)           \
  )

#define IS_LL_ADC_INJ_TRIG_AUTO(__INJ_TRIG_AUTO__)                             \
  (   ((__INJ_TRIG_AUTO__) == LL_ADC_INJ_TRIG_INDEPENDENT)                     \
   || ((__INJ_TRIG_AUTO__) == LL_ADC_INJ_TRIG_FROM_GRP_REGULAR)                \
  )

#define IS_LL_ADC_INJ_SEQ_SCAN_LENGTH(__INJ_SEQ_SCAN_LENGTH__)                 \
  (   ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_DISABLE)               \
   || ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS)         \
   || ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_ENABLE_3RANKS)         \
   || ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS)         \
  )

#define IS_LL_ADC_INJ_SEQ_SCAN_DISCONT_MODE(__INJ_SEQ_DISCONT_MODE__)          \
  (   ((__INJ_SEQ_DISCONT_MODE__) == LL_ADC_INJ_SEQ_DISCONT_DISABLE)           \
   || ((__INJ_SEQ_DISCONT_MODE__) == LL_ADC_INJ_SEQ_DISCONT_1RANK)             \
  )

#if defined(ADC_MULTIMODE_SUPPORT)
/* Check of parameters for configuration of ADC hierarchical scope:           */
/* multimode.                                                                 */
#define IS_LL_ADC_MULTI_MODE(__MULTI_MODE__)                                   \
  (   ((__MULTI_MODE__) == LL_ADC_MULTI_INDEPENDENT)                           \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_REG_SIMULT)                       \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_REG_INTERL)                       \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_INJ_SIMULT)                       \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_INJ_ALTERN)                       \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_REG_SIM_INJ_SIM)                  \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_REG_SIM_INJ_ALT)                  \
   || ((__MULTI_MODE__) == LL_ADC_MULTI_DUAL_REG_INT_INJ_SIM)                  \
  )

#define IS_LL_ADC_MULTI_DMA_TRANSFER(__MULTI_DMA_TRANSFER__)                   \
  (   ((__MULTI_DMA_TRANSFER__) == LL_ADC_MULTI_REG_DMA_EACH_ADC)              \
   || ((__MULTI_DMA_TRANSFER__) == LL_ADC_MULTI_REG_DMA_LIMIT_RES12_10B)       \
   || ((__MULTI_DMA_TRANSFER__) == LL_ADC_MULTI_REG_DMA_LIMIT_RES8_6B)         \
   || ((__MULTI_DMA_TRANSFER__) == LL_ADC_MULTI_REG_DMA_UNLMT_RES12_10B)       \
   || ((__MULTI_DMA_TRANSFER__) == LL_ADC_MULTI_REG_DMA_UNLMT_RES8_6B)         \
  )

#define IS_LL_ADC_MULTI_TWOSMP_DELAY(__MULTI_TWOSMP_DELAY__)                   \
  (   ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_1CYCLE)           \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_2CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_3CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_4CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_5CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_6CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_7CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_8CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_9CYCLES)          \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_10CYCLES)         \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_11CYCLES)         \
   || ((__MULTI_TWOSMP_DELAY__) == LL_ADC_MULTI_TWOSMP_DELAY_12CYCLES)         \
  )

#define IS_LL_ADC_MULTI_MASTER_SLAVE(__MULTI_MASTER_SLAVE__)                   \
  (   ((__MULTI_MASTER_SLAVE__) == LL_ADC_MULTI_MASTER)                        \
   || ((__MULTI_MASTER_SLAVE__) == LL_ADC_MULTI_SLAVE)                         \
   || ((__MULTI_MASTER_SLAVE__) == LL_ADC_MULTI_MASTER_SLAVE)                  \
  )

#endif /* ADC_MULTIMODE_SUPPORT */
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
  *         Caution: On this STM32 serie, if several ADC instances are available
  *         on the selected device, RCC ADC reset will reset
  *         all ADC instances belonging to the common ADC instance.
  *         To de-initialize only 1 ADC instance, use
  *         function @ref LL_ADC_DeInit().
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
  #if defined(ADC1) && defined(ADC2) && defined(ADC3) && defined(ADC4)
  if(ADCxy_COMMON == ADC12_COMMON)
  {
    LL_AHB1_GRP1_ForceReset  (LL_AHB1_GRP1_PERIPH_ADC12);
  }
  else
  {
    LL_AHB1_GRP1_ForceReset  (LL_AHB1_GRP1_PERIPH_ADC34);
  }
  #elif defined(ADC1) && defined(ADC2)
  LL_AHB1_GRP1_ForceReset  (LL_AHB1_GRP1_PERIPH_ADC12);
  #elif defined(ADC1)
  LL_AHB1_GRP1_ForceReset  (LL_AHB1_GRP1_PERIPH_ADC1);
  #endif

  /* Release reset of ADC clock (core clock) */
  #if defined(ADC1) && defined(ADC2) && defined(ADC3) && defined(ADC4)
  if(ADCxy_COMMON == ADC12_COMMON)
  {
    LL_AHB1_GRP1_ReleaseReset(LL_AHB1_GRP1_PERIPH_ADC12);
  }
  else
  {
    LL_AHB1_GRP1_ReleaseReset(LL_AHB1_GRP1_PERIPH_ADC34);
  }
  #elif defined(ADC1) && defined(ADC2)
  LL_AHB1_GRP1_ReleaseReset(LL_AHB1_GRP1_PERIPH_ADC12);
  #elif defined(ADC1)
  LL_AHB1_GRP1_ReleaseReset(LL_AHB1_GRP1_PERIPH_ADC1);
  #endif

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

#if defined(ADC_MULTIMODE_SUPPORT)
  assert_param(IS_LL_ADC_MULTI_MODE(ADC_CommonInitStruct->Multimode));
  if(ADC_CommonInitStruct->Multimode != LL_ADC_MULTI_INDEPENDENT)
  {
    assert_param(IS_LL_ADC_MULTI_DMA_TRANSFER(ADC_CommonInitStruct->MultiDMATransfer));
    assert_param(IS_LL_ADC_MULTI_TWOSMP_DELAY(ADC_CommonInitStruct->MultiTwoSamplingDelay));
  }
#endif /* ADC_MULTIMODE_SUPPORT */

  /* Note: Hardware constraint (refer to description of functions             */
  /*       "LL_ADC_SetCommonXXX()" and "LL_ADC_SetMultiXXX()"):               */
  /*       On this STM32 serie, setting of these features is conditioned to   */
  /*       ADC state:                                                         */
  /*       All ADC instances of the ADC common group must be disabled.        */
  if(__LL_ADC_IS_ENABLED_ALL_COMMON_INSTANCE(ADCxy_COMMON) == 0U)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - common to several ADC                                               */
    /*    (all ADC instances belonging to the same ADC common instance)       */
    /*    - Set ADC clock (conversion clock)                                  */
    /*  - multimode (if several ADC instances available on the                */
    /*    selected device)                                                    */
    /*    - Set ADC multimode configuration                                   */
    /*    - Set ADC multimode DMA transfer                                    */
    /*    - Set ADC multimode: delay between 2 sampling phases                */
#if defined(ADC_MULTIMODE_SUPPORT)
    if(ADC_CommonInitStruct->Multimode != LL_ADC_MULTI_INDEPENDENT)
    {
      MODIFY_REG(ADCxy_COMMON->CCR,
                   ADC_CCR_CKMODE
                 | ADC_CCR_DUAL
                 | ADC_CCR_MDMA
                 | ADC_CCR_DELAY
                ,
                   ADC_CommonInitStruct->CommonClock
                 | ADC_CommonInitStruct->Multimode
                 | ADC_CommonInitStruct->MultiDMATransfer
                 | ADC_CommonInitStruct->MultiTwoSamplingDelay
                );
    }
    else
    {
      MODIFY_REG(ADCxy_COMMON->CCR,
                   ADC_CCR_CKMODE
                 | ADC_CCR_DUAL
                 | ADC_CCR_MDMA
                 | ADC_CCR_DELAY
                ,
                   ADC_CommonInitStruct->CommonClock
                 | LL_ADC_MULTI_INDEPENDENT
                );
    }
#else
    LL_ADC_SetCommonClock(ADCxy_COMMON, ADC_CommonInitStruct->CommonClock);
#endif
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
  ADC_CommonInitStruct->CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV2;

#if defined(ADC_MULTIMODE_SUPPORT)
  /* Set fields of ADC multimode */
  ADC_CommonInitStruct->Multimode             = LL_ADC_MULTI_INDEPENDENT;
  ADC_CommonInitStruct->MultiDMATransfer      = LL_ADC_MULTI_REG_DMA_EACH_ADC;
  ADC_CommonInitStruct->MultiTwoSamplingDelay = LL_ADC_MULTI_TWOSMP_DELAY_1CYCLE;
#endif /* ADC_MULTIMODE_SUPPORT */
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
  *         Caution: On this STM32 serie, if several ADC instances are available
  *         on the selected device, RCC ADC reset will reset
  *         all ADC instances belonging to the common ADC instance.
  *         Refer to function @ref LL_ADC_CommonDeInit().
  * @param  ADCx ADC instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are de-initialized
  *          - ERROR: ADC registers are not de-initialized
  */
ErrorStatus LL_ADC_DeInit(ADC_TypeDef *ADCx)
{
  ErrorStatus status = SUCCESS;

  __IO uint32_t timeout_cpu_cycles = 0U;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));

  /* Disable ADC instance if not already disabled.                            */
  if(LL_ADC_IsEnabled(ADCx) == 1U)
  {
    /* Set ADC group regular trigger source to SW start to ensure to not      */
    /* have an external trigger event occurring during the conversion stop    */
    /* ADC disable process.                                                   */
    LL_ADC_REG_SetTriggerSource(ADCx, LL_ADC_REG_TRIG_SOFTWARE);

    /* Stop potential ADC conversion on going on ADC group regular.           */
    if(LL_ADC_REG_IsConversionOngoing(ADCx) != 0U)
    {
      if(LL_ADC_REG_IsStopConversionOngoing(ADCx) == 0U)
      {
        LL_ADC_REG_StopConversion(ADCx);
      }
    }

    /* Set ADC group injected trigger source to SW start to ensure to not     */
    /* have an external trigger event occurring during the conversion stop    */
    /* ADC disable process.                                                   */
    LL_ADC_INJ_SetTriggerSource(ADCx, LL_ADC_INJ_TRIG_SOFTWARE);

    /* Stop potential ADC conversion on going on ADC group injected.          */
    if(LL_ADC_INJ_IsConversionOngoing(ADCx) != 0U)
    {
      if(LL_ADC_INJ_IsStopConversionOngoing(ADCx) == 0U)
      {
        LL_ADC_INJ_StopConversion(ADCx);
      }
    }

    /* Wait for ADC conversions are effectively stopped                       */
    timeout_cpu_cycles = ADC_TIMEOUT_STOP_CONVERSION_CPU_CYCLES;
    while ((  LL_ADC_REG_IsStopConversionOngoing(ADCx)
            | LL_ADC_INJ_IsStopConversionOngoing(ADCx)) == 1U)
    {
      if(timeout_cpu_cycles-- == 0U)
      {
        /* Time-out error */
        status = ERROR;
      }
    }

    /* Flush group injected contexts queue (register JSQR):                   */
    /* Note: Bit JQM must be set to empty the contexts queue (otherwise       */
    /*       contexts queue is maintained with the last active context).      */
    LL_ADC_INJ_SetQueueMode(ADCx, LL_ADC_INJ_QUEUE_2CONTEXTS_END_EMPTY);

    /* Disable the ADC instance */
    LL_ADC_Disable(ADCx);

    /* Wait for ADC instance is effectively disabled */
    timeout_cpu_cycles = ADC_TIMEOUT_DISABLE_CPU_CYCLES;
    while (LL_ADC_IsDisableOngoing(ADCx) == 1U)
    {
      if(timeout_cpu_cycles-- == 0U)
      {
        /* Time-out error */
        status = ERROR;
      }
    }
  }

  /* Check whether ADC state is compliant with expected state */
  if(READ_BIT(ADCx->CR,
              (  ADC_CR_JADSTP | ADC_CR_ADSTP | ADC_CR_JADSTART | ADC_CR_ADSTART
               | ADC_CR_ADDIS | ADC_CR_ADEN                                     )
             )
     == 0U)
  {
    /* ========== Reset ADC registers ========== */
    /* Reset register IER */
    CLEAR_BIT(ADCx->IER,
              (  LL_ADC_IT_ADRDY
               | LL_ADC_IT_EOC
               | LL_ADC_IT_EOS
               | LL_ADC_IT_OVR
               | LL_ADC_IT_EOSMP
               | LL_ADC_IT_JEOC
               | LL_ADC_IT_JEOS
               | LL_ADC_IT_JQOVF
               | LL_ADC_IT_AWD1
               | LL_ADC_IT_AWD2
               | LL_ADC_IT_AWD3 )
             );

    /* Reset register ISR */
    SET_BIT(ADCx->ISR,
            (  LL_ADC_FLAG_ADRDY
             | LL_ADC_FLAG_EOC
             | LL_ADC_FLAG_EOS
             | LL_ADC_FLAG_OVR
             | LL_ADC_FLAG_EOSMP
             | LL_ADC_FLAG_JEOC
             | LL_ADC_FLAG_JEOS
             | LL_ADC_FLAG_JQOVF
             | LL_ADC_FLAG_AWD1
             | LL_ADC_FLAG_AWD2
             | LL_ADC_FLAG_AWD3 )
           );

    /* Reset register CR */
    /*  - Bits ADC_CR_JADSTP, ADC_CR_ADSTP, ADC_CR_JADSTART, ADC_CR_ADSTART,  */
    /*    ADC_CR_ADCAL, ADC_CR_ADDIS, ADC_CR_ADEN are in                      */
    /*    access mode "read-set": no direct reset applicable.                 */
    /*  - Reset Calibration mode to default setting (single ended).           */
    /*  - Disable ADC internal voltage regulator.                             */
    /*    Note: ADC internal voltage regulator disable is conditioned to      */
    /*          ADC state disabled: already done above.                       */
    /* Sequence to disable voltage regulator:                                 */
    /* 1. Set the intermediate state before moving the ADC voltage regulator  */
    /*    to disable state.                                                   */
    CLEAR_BIT(ADCx->CR, ADC_CR_ADVREGEN_1 | ADC_CR_ADVREGEN_0 | ADC_CR_ADCALDIF);
    /* 2. Set ADVREGEN bits to 0x10 */
    SET_BIT(ADCx->CR, ADC_CR_ADVREGEN_1);

    /* Reset register CFGR */
    CLEAR_BIT(ADCx->CFGR,
              (  ADC_CFGR_AWD1CH  | ADC_CFGR_JAUTO   | ADC_CFGR_JAWD1EN
               | ADC_CFGR_AWD1EN  | ADC_CFGR_AWD1SGL | ADC_CFGR_JQM
               | ADC_CFGR_JDISCEN | ADC_CFGR_DISCNUM | ADC_CFGR_DISCEN
               | ADC_CFGR_AUTDLY  | ADC_CFGR_CONT    | ADC_CFGR_OVRMOD
               | ADC_CFGR_EXTEN   | ADC_CFGR_EXTSEL  | ADC_CFGR_ALIGN
               | ADC_CFGR_RES     | ADC_CFGR_DMACFG  | ADC_CFGR_DMAEN  )
             );

    /* Reset register SMPR1 */
    CLEAR_BIT(ADCx->SMPR1,
              (  ADC_SMPR1_SMP9 | ADC_SMPR1_SMP8 | ADC_SMPR1_SMP7
               | ADC_SMPR1_SMP6 | ADC_SMPR1_SMP5 | ADC_SMPR1_SMP4
               | ADC_SMPR1_SMP3 | ADC_SMPR1_SMP2 | ADC_SMPR1_SMP1)
             );

    /* Reset register SMPR2 */
    CLEAR_BIT(ADCx->SMPR2,
              (  ADC_SMPR2_SMP18 | ADC_SMPR2_SMP17 | ADC_SMPR2_SMP16
               | ADC_SMPR2_SMP15 | ADC_SMPR2_SMP14 | ADC_SMPR2_SMP13
               | ADC_SMPR2_SMP12 | ADC_SMPR2_SMP11 | ADC_SMPR2_SMP10)
             );

    /* Reset register TR1 */
    MODIFY_REG(ADCx->TR1, ADC_TR1_HT1 | ADC_TR1_LT1, ADC_TR1_HT1);

    /* Reset register TR2 */
    MODIFY_REG(ADCx->TR2, ADC_TR2_HT2 | ADC_TR2_LT2, ADC_TR2_HT2);

    /* Reset register TR3 */
    MODIFY_REG(ADCx->TR3, ADC_TR3_HT3 | ADC_TR3_LT3, ADC_TR3_HT3);

    /* Reset register SQR1 */
    CLEAR_BIT(ADCx->SQR1,
              (  ADC_SQR1_SQ4 | ADC_SQR1_SQ3 | ADC_SQR1_SQ2
               | ADC_SQR1_SQ1 | ADC_SQR1_L)
             );

    /* Reset register SQR2 */
    CLEAR_BIT(ADCx->SQR2,
              (  ADC_SQR2_SQ9 | ADC_SQR2_SQ8 | ADC_SQR2_SQ7
               | ADC_SQR2_SQ6 | ADC_SQR2_SQ5)
             );

    /* Reset register SQR3 */
    CLEAR_BIT(ADCx->SQR3,
              (  ADC_SQR3_SQ14 | ADC_SQR3_SQ13 | ADC_SQR3_SQ12
               | ADC_SQR3_SQ11 | ADC_SQR3_SQ10)
             );

    /* Reset register SQR4 */
    CLEAR_BIT(ADCx->SQR4, ADC_SQR4_SQ16 | ADC_SQR4_SQ15);

    /* Reset register JSQR */
    CLEAR_BIT(ADCx->JSQR,
              (  ADC_JSQR_JL
               | ADC_JSQR_JEXTSEL | ADC_JSQR_JEXTEN
               | ADC_JSQR_JSQ4    | ADC_JSQR_JSQ3
               | ADC_JSQR_JSQ2    | ADC_JSQR_JSQ1  )
             );

    /* Flush ADC group injected contexts queue */
    SET_BIT(ADCx->CFGR, ADC_CFGR_JQM);
    CLEAR_BIT(ADCx->CFGR, ADC_CFGR_JQM);
    /* Reset register ISR bit JQOVF (set by previous operation on JSQR) */
    SET_BIT(ADCx->ISR, LL_ADC_FLAG_JQOVF);

    /* Reset register DR */
    /* Note: bits in access mode read only, no direct reset applicable */

    /* Reset register OFR1 */
    CLEAR_BIT(ADCx->OFR1, ADC_OFR1_OFFSET1_EN | ADC_OFR1_OFFSET1_CH | ADC_OFR1_OFFSET1);
    /* Reset register OFR2 */
    CLEAR_BIT(ADCx->OFR2, ADC_OFR2_OFFSET2_EN | ADC_OFR2_OFFSET2_CH | ADC_OFR2_OFFSET2);
    /* Reset register OFR3 */
    CLEAR_BIT(ADCx->OFR3, ADC_OFR3_OFFSET3_EN | ADC_OFR3_OFFSET3_CH | ADC_OFR3_OFFSET3);
    /* Reset register OFR4 */
    CLEAR_BIT(ADCx->OFR4, ADC_OFR4_OFFSET4_EN | ADC_OFR4_OFFSET4_CH | ADC_OFR4_OFFSET4);

    /* Reset registers JDR1, JDR2, JDR3, JDR4 */
    /* Note: bits in access mode read only, no direct reset applicable */

    /* Reset register AWD2CR */
    CLEAR_BIT(ADCx->AWD2CR, ADC_AWD2CR_AWD2CH);

    /* Reset register AWD3CR */
    CLEAR_BIT(ADCx->AWD3CR, ADC_AWD3CR_AWD3CH);

    /* Reset register DIFSEL */
    CLEAR_BIT(ADCx->DIFSEL, ADC_DIFSEL_DIFSEL);

    /* Reset register CALFACT */
    CLEAR_BIT(ADCx->CALFACT, ADC_CALFACT_CALFACT_D | ADC_CALFACT_CALFACT_S);
  }
  else
  {
    /* ADC instance is in an unknown state */
    /* Need to performing a hard reset of ADC instance, using high level      */
    /* clock source RCC ADC reset.                                            */
    /* Caution: On this STM32 serie, if several ADC instances are available   */
    /*          on the selected device, RCC ADC reset will reset              */
    /*          all ADC instances belonging to the common ADC instance.       */
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
  *         Affects both group regular and group injected (availability
  *         of ADC group injected depends on STM32 families).
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
  *          - Set ADC group regular or group injected sequencer:
  *            map channel on the selected sequencer rank.
  *            Refer to function @ref LL_ADC_REG_SetSequencerRanks().
  *          - Set ADC channel sampling time
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

  assert_param(IS_LL_ADC_RESOLUTION(ADC_InitStruct->Resolution));
  assert_param(IS_LL_ADC_DATA_ALIGN(ADC_InitStruct->DataAlignment));
  assert_param(IS_LL_ADC_LOW_POWER(ADC_InitStruct->LowPowerMode));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0U)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC instance                                                        */
    /*    - Set ADC data resolution                                           */
    /*    - Set ADC conversion data alignment                                 */
    /*    - Set ADC low power mode                                            */
    MODIFY_REG(ADCx->CFGR,
                 ADC_CFGR_RES
               | ADC_CFGR_ALIGN
               | ADC_CFGR_AUTDLY
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
  *          - Set ADC group regular or group injected sequencer:
  *            map channel on the selected sequencer rank.
  *            Refer to function @ref LL_ADC_REG_SetSequencerRanks().
  *          - Set ADC channel sampling time
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
#if defined(ADC1) && defined(ADC2) && defined(ADC3) && defined(ADC4)
  assert_param(IS_LL_ADC_REG_TRIG_SOURCE(ADCx, ADC_REG_InitStruct->TriggerSource));
#else
  assert_param(IS_LL_ADC_REG_TRIG_SOURCE(ADC_REG_InitStruct->TriggerSource));
#endif
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
  if(LL_ADC_IsEnabled(ADCx) == 0U)
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
    if(ADC_REG_InitStruct->SequencerLength != LL_ADC_REG_SEQ_SCAN_DISABLE)
    {
      MODIFY_REG(ADCx->CFGR,
                   ADC_CFGR_EXTSEL
                 | ADC_CFGR_EXTEN
                 | ADC_CFGR_DISCEN
                 | ADC_CFGR_DISCNUM
                 | ADC_CFGR_CONT
                 | ADC_CFGR_DMAEN
                 | ADC_CFGR_DMACFG
                 | ADC_CFGR_OVRMOD
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
      MODIFY_REG(ADCx->CFGR,
                   ADC_CFGR_EXTSEL
                 | ADC_CFGR_EXTEN
                 | ADC_CFGR_DISCEN
                 | ADC_CFGR_DISCNUM
                 | ADC_CFGR_CONT
                 | ADC_CFGR_DMAEN
                 | ADC_CFGR_DMACFG
                 | ADC_CFGR_OVRMOD
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
  * @brief  Initialize some features of ADC group injected.
  * @note   These parameters have an impact on ADC scope: ADC group injected.
  *         Refer to corresponding unitary functions into
  *         @ref ADC_LL_EF_Configuration_ADC_Group_Regular
  *         (functions with prefix "INJ").
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
  *          - Set ADC group injected sequencer:
  *            map channel on the selected sequencer rank.
  *            Refer to function @ref LL_ADC_INJ_SetSequencerRanks().
  *          - Set ADC channel sampling time
  *            Refer to function LL_ADC_SetChannelSamplingTime();
  * @note   Caution to ADC group injected contexts queue: On this STM32 serie,
  *         using successively several times this function will appear has
  *         having no effect.
  *         This is due to ADC group injected contexts queue (this feature
  *         cannot be disabled on this STM32 serie).
  *         To set several features of ADC group injected, use
  *         function @ref LL_ADC_INJ_ConfigQueueContext().
  * @param  ADCx ADC instance
  * @param  ADC_INJ_InitStruct Pointer to a @ref LL_ADC_INJ_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are initialized
  *          - ERROR: ADC registers are not initialized
  */
ErrorStatus LL_ADC_INJ_Init(ADC_TypeDef *ADCx, LL_ADC_INJ_InitTypeDef *ADC_INJ_InitStruct)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));
#if defined(ADC1) && defined(ADC2) && defined(ADC3) && defined(ADC4)
  assert_param(IS_LL_ADC_INJ_TRIG_SOURCE(ADCx, ADC_INJ_InitStruct->TriggerSource));
#else
  assert_param(IS_LL_ADC_INJ_TRIG_SOURCE(ADC_INJ_InitStruct->TriggerSource));
#endif
  assert_param(IS_LL_ADC_INJ_SEQ_SCAN_LENGTH(ADC_INJ_InitStruct->SequencerLength));
  if(ADC_INJ_InitStruct->SequencerLength != LL_ADC_INJ_SEQ_SCAN_DISABLE)
  {
    assert_param(IS_LL_ADC_INJ_SEQ_SCAN_DISCONT_MODE(ADC_INJ_InitStruct->SequencerDiscont));
  }
  assert_param(IS_LL_ADC_INJ_TRIG_AUTO(ADC_INJ_InitStruct->TrigAuto));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0U)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC group injected                                                  */
    /*    - Set ADC group injected trigger source                             */
    /*    - Set ADC group injected sequencer length                           */
    /*    - Set ADC group injected sequencer discontinuous mode               */
    /*    - Set ADC group injected conversion trigger: independent or         */
    /*      from ADC group regular                                            */
    /* Note: On this STM32 serie, ADC trigger edge is set to value 0x0 by     */
    /*       setting of trigger source to SW start.                           */
    if(ADC_INJ_InitStruct->SequencerLength != LL_ADC_REG_SEQ_SCAN_DISABLE)
    {
      MODIFY_REG(ADCx->CFGR,
                   ADC_CFGR_JDISCEN
                 | ADC_CFGR_JAUTO
                ,
                   ADC_INJ_InitStruct->SequencerDiscont
                 | ADC_INJ_InitStruct->TrigAuto
                );
    }
    else
    {
      MODIFY_REG(ADCx->CFGR,
                   ADC_CFGR_JDISCEN
                 | ADC_CFGR_JAUTO
                ,
                   LL_ADC_REG_SEQ_DISCONT_DISABLE
                 | ADC_INJ_InitStruct->TrigAuto
                );
    }

    MODIFY_REG(ADCx->JSQR,
                 ADC_JSQR_JEXTSEL
               | ADC_JSQR_JEXTEN
               | ADC_JSQR_JL
              ,
                 ADC_INJ_InitStruct->TriggerSource
               | ADC_INJ_InitStruct->SequencerLength
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
  * @brief  Set each @ref LL_ADC_INJ_InitTypeDef field to default value.
  * @param  ADC_INJ_InitStruct Pointer to a @ref LL_ADC_INJ_InitTypeDef structure
  *                            whose fields will be set to default values.
  * @retval None
  */
void LL_ADC_INJ_StructInit(LL_ADC_INJ_InitTypeDef *ADC_INJ_InitStruct)
{
  /* Set ADC_INJ_InitStruct fields to default values */
  /* Set fields of ADC group injected */
  ADC_INJ_InitStruct->TriggerSource    = LL_ADC_INJ_TRIG_SOFTWARE;
  ADC_INJ_InitStruct->SequencerLength  = LL_ADC_INJ_SEQ_SCAN_DISABLE;
  ADC_INJ_InitStruct->SequencerDiscont = LL_ADC_INJ_SEQ_DISCONT_DISABLE;
  ADC_INJ_InitStruct->TrigAuto         = LL_ADC_INJ_TRIG_INDEPENDENT;
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

#endif /* ADC1 || ADC2 || ADC3 || ADC4 */


#endif /* STM32F301x8 || STM32F302x8 || STM32F302xC || STM32F302xE || STM32F303x8 || STM32F303xC || STM32F303xE || STM32F318xx || STM32F328xx || STM32F334x8 || STM32F358xx || STM32F398xx */

#if defined (ADC1_V2_5)

#if defined (ADC1)

/** @addtogroup ADC_LL ADC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/** @addtogroup ADC_LL_Private_Macros
  * @{
  */

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* common to several ADC instances.                                           */
/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC instance.                                                              */
#define IS_LL_ADC_DATA_ALIGN(__DATA_ALIGN__)                                   \
  (   ((__DATA_ALIGN__) == LL_ADC_DATA_ALIGN_RIGHT)                            \
   || ((__DATA_ALIGN__) == LL_ADC_DATA_ALIGN_LEFT) )

#define IS_LL_ADC_SCAN_SELECTION(__SCAN_SELECTION__)                           \
  (   ((__SCAN_SELECTION__) == LL_ADC_SEQ_SCAN_DISABLE)                        \
   || ((__SCAN_SELECTION__) == LL_ADC_SEQ_SCAN_ENABLE) )

#define IS_LL_ADC_SEQ_SCAN_MODE(__SEQ_SCAN_MODE__)                             \
  (   ((__SCAN_MODE__) == LL_ADC_SEQ_SCAN_DISABLE)                             \
   || ((__SCAN_MODE__) == LL_ADC_SEQ_SCAN_ENABLE) )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC group regular                                                          */
#define IS_LL_ADC_REG_TRIG_SOURCE(__REG_TRIG_SOURCE__)                         \
  (   ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_SOFTWARE)                      \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM2_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM3_TRGO)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM4_CH2)                  \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM19_TRGO)                \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM19_CH3)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_TIM19_CH4)                 \
   || ((__REG_TRIG_SOURCE__) == LL_ADC_REG_TRIG_EXT_EXTI_LINE11))

#define IS_LL_ADC_REG_CONTINUOUS_MODE(__REG_CONTINUOUS_MODE__)                 \
  (   ((__REG_CONTINUOUS_MODE__) == LL_ADC_REG_CONV_SINGLE)                    \
   || ((__REG_CONTINUOUS_MODE__) == LL_ADC_REG_CONV_CONTINUOUS))

#define IS_LL_ADC_REG_DMA_TRANSFER(__REG_DMA_TRANSFER__)                       \
  (   ((__REG_DMA_TRANSFER__) == LL_ADC_REG_DMA_TRANSFER_NONE)                 \
   || ((__REG_DMA_TRANSFER__) == LL_ADC_REG_DMA_TRANSFER_UNLIMITED))

#define IS_LL_ADC_REG_SEQ_SCAN_LENGTH(__REG_SEQ_SCAN_LENGTH__)                 \
  (   ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_DISABLE)               \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_2RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_3RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_4RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_5RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_6RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_7RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_8RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_9RANKS)         \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_10RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_11RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_12RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_13RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_14RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_15RANKS)        \
   || ((__REG_SEQ_SCAN_LENGTH__) == LL_ADC_REG_SEQ_SCAN_ENABLE_16RANKS))

#define IS_LL_ADC_REG_SEQ_SCAN_DISCONT_MODE(__REG_SEQ_DISCONT_MODE__)          \
  (   ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_DISABLE)           \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_1RANK)             \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_2RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_3RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_4RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_5RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_6RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_7RANKS)            \
   || ((__REG_SEQ_DISCONT_MODE__) == LL_ADC_REG_SEQ_DISCONT_8RANKS) )

/* Check of parameters for configuration of ADC hierarchical scope:           */
/* ADC group injected                                                         */
#define IS_LL_ADC_INJ_TRIG_SOURCE(__INJ_TRIG_SOURCE__)                         \
  (   ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_SOFTWARE)                      \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_TRGO)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM2_CH1)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM3_CH4)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM4_TRGO)                  \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM19_CH1)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_TIM19_CH2)                 \
   || ((__INJ_TRIG_SOURCE__) == LL_ADC_INJ_TRIG_EXT_EXTI_LINE15))

#define IS_LL_ADC_INJ_TRIG_AUTO(__INJ_TRIG_AUTO__)                             \
  (   ((__INJ_TRIG_AUTO__) == LL_ADC_INJ_TRIG_INDEPENDENT)                     \
   || ((__INJ_TRIG_AUTO__) == LL_ADC_INJ_TRIG_FROM_GRP_REGULAR))

#define IS_LL_ADC_INJ_SEQ_SCAN_LENGTH(__INJ_SEQ_SCAN_LENGTH__)                 \
  (   ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_DISABLE)               \
   || ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_ENABLE_2RANKS)         \
   || ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_ENABLE_3RANKS)         \
   || ((__INJ_SEQ_SCAN_LENGTH__) == LL_ADC_INJ_SEQ_SCAN_ENABLE_4RANKS))

#define IS_LL_ADC_INJ_SEQ_SCAN_DISCONT_MODE(__INJ_SEQ_DISCONT_MODE__)          \
  (   ((__INJ_SEQ_DISCONT_MODE__) == LL_ADC_INJ_SEQ_DISCONT_DISABLE)           \
   || ((__INJ_SEQ_DISCONT_MODE__) == LL_ADC_INJ_SEQ_DISCONT_1RANK)  )

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
  LL_APB2_GRP1_ForceReset  (LL_APB2_GRP1_PERIPH_ADC1);

  /* Release reset of ADC clock (core clock) */
  LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_ADC1);

  return SUCCESS;
}

/**
  * @brief  De-initialize registers of the selected ADC instance
  *         to their default reset values.
  * @note   To reset all ADC instances quickly (perform a hard reset),
  *         use function @ref LL_ADC_CommonDeInit().
  * @param  ADCx ADC instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are de-initialized
  *          - ERROR: ADC registers are not de-initialized
  */
ErrorStatus LL_ADC_DeInit(ADC_TypeDef *ADCx)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));

  /* Disable ADC instance if not already disabled.                            */
  if(LL_ADC_IsEnabled(ADCx) == 1U)
  {
    /* Set ADC group regular trigger source to SW start to ensure to not      */
    /* have an external trigger event occurring during the conversion stop    */
    /* ADC disable process.                                                   */
    LL_ADC_REG_SetTriggerSource(ADCx, LL_ADC_REG_TRIG_SOFTWARE);

    /* Set ADC group injected trigger source to SW start to ensure to not     */
    /* have an external trigger event occurring during the conversion stop    */
    /* ADC disable process.                                                   */
    LL_ADC_INJ_SetTriggerSource(ADCx, LL_ADC_INJ_TRIG_SOFTWARE);

    /* Disable the ADC instance */
    LL_ADC_Disable(ADCx);
  }

  /* Check whether ADC state is compliant with expected state */
  /* (hardware requirements of bits state to reset registers below) */
  if(READ_BIT(ADCx->CR2, ADC_CR2_ADON) == 0U)
  {
    /* ========== Reset ADC registers ========== */
    /* Reset register SR */
    CLEAR_BIT(ADCx->SR,
              (  LL_ADC_FLAG_STRT
               | LL_ADC_FLAG_JSTRT
               | LL_ADC_FLAG_EOS
               | LL_ADC_FLAG_JEOS
               | LL_ADC_FLAG_AWD1 )
             );

    /* Reset register CR1 */
    CLEAR_BIT(ADCx->CR1,
              (  ADC_CR1_AWDEN   | ADC_CR1_JAWDEN
               | ADC_CR1_DISCNUM | ADC_CR1_JDISCEN | ADC_CR1_DISCEN
               | ADC_CR1_JAUTO   | ADC_CR1_AWDSGL  | ADC_CR1_SCAN
               | ADC_CR1_JEOCIE  | ADC_CR1_AWDIE   | ADC_CR1_EOCIE
               | ADC_CR1_AWDCH                                     )
             );

    /* Reset register CR2 */
    CLEAR_BIT(ADCx->CR2,
              (  ADC_CR2_TSVREFE
               | ADC_CR2_SWSTART  | ADC_CR2_EXTTRIG  | ADC_CR2_EXTSEL
               | ADC_CR2_JSWSTART | ADC_CR2_JEXTTRIG | ADC_CR2_JEXTSEL
               | ADC_CR2_ALIGN    | ADC_CR2_DMA
               | ADC_CR2_RSTCAL   | ADC_CR2_CAL
               | ADC_CR2_CONT     | ADC_CR2_ADON                      )
             );

    /* Reset register SMPR1 */
    CLEAR_BIT(ADCx->SMPR1,
              (  ADC_SMPR1_SMP17 | ADC_SMPR1_SMP16
               | ADC_SMPR1_SMP15 | ADC_SMPR1_SMP14 | ADC_SMPR1_SMP13
               | ADC_SMPR1_SMP12 | ADC_SMPR1_SMP11 | ADC_SMPR1_SMP10)
             );

    /* Reset register SMPR2 */
    CLEAR_BIT(ADCx->SMPR2,
              (  ADC_SMPR2_SMP9
               | ADC_SMPR2_SMP8 | ADC_SMPR2_SMP7 | ADC_SMPR2_SMP6
               | ADC_SMPR2_SMP5 | ADC_SMPR2_SMP4 | ADC_SMPR2_SMP3
               | ADC_SMPR2_SMP2 | ADC_SMPR2_SMP1 | ADC_SMPR2_SMP0)
             );

    /* Reset register JOFR1 */
    CLEAR_BIT(ADCx->JOFR1, ADC_JOFR1_JOFFSET1);
    /* Reset register JOFR2 */
    CLEAR_BIT(ADCx->JOFR2, ADC_JOFR2_JOFFSET2);
    /* Reset register JOFR3 */
    CLEAR_BIT(ADCx->JOFR3, ADC_JOFR3_JOFFSET3);
    /* Reset register JOFR4 */
    CLEAR_BIT(ADCx->JOFR4, ADC_JOFR4_JOFFSET4);

    /* Reset register HTR */
    SET_BIT(ADCx->HTR, ADC_HTR_HT);
    /* Reset register LTR */
    CLEAR_BIT(ADCx->LTR, ADC_LTR_LT);

    /* Reset register SQR1 */
    CLEAR_BIT(ADCx->SQR1,
              (  ADC_SQR1_L
               | ADC_SQR1_SQ16
               | ADC_SQR1_SQ15 | ADC_SQR1_SQ14 | ADC_SQR1_SQ13)
             );

    /* Reset register SQR2 */
    CLEAR_BIT(ADCx->SQR2,
              (  ADC_SQR2_SQ12 | ADC_SQR2_SQ11 | ADC_SQR2_SQ10
               | ADC_SQR2_SQ9 | ADC_SQR2_SQ8 | ADC_SQR2_SQ7)
             );


    /* Reset register JSQR */
    CLEAR_BIT(ADCx->JSQR,
              (  ADC_JSQR_JL
               | ADC_JSQR_JSQ4 | ADC_JSQR_JSQ3
               | ADC_JSQR_JSQ2 | ADC_JSQR_JSQ1  )
             );

    /* Reset register DR */
    /* bits in access mode read only, no direct reset applicable */

    /* Reset registers JDR1, JDR2, JDR3, JDR4 */
    /* bits in access mode read only, no direct reset applicable */

  }

  return status;
}

/**
  * @brief  Initialize some features of ADC instance.
  * @note   These parameters have an impact on ADC scope: ADC instance.
  *         Affects both group regular and group injected (availability
  *         of ADC group injected depends on STM32 families).
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
  *          - Set ADC group regular or group injected sequencer:
  *            map channel on the selected sequencer rank.
  *            Refer to function @ref LL_ADC_REG_SetSequencerRanks().
  *          - Set ADC channel sampling time
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

  assert_param(IS_LL_ADC_DATA_ALIGN(ADC_InitStruct->DataAlignment));
  assert_param(IS_LL_ADC_SCAN_SELECTION(ADC_InitStruct->SequencersScanMode));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0U)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC instance                                                        */
    /*    - Set ADC conversion data alignment                                 */
    MODIFY_REG(ADCx->CR1,
                 ADC_CR1_SCAN
              ,
                 ADC_InitStruct->SequencersScanMode
              );

    MODIFY_REG(ADCx->CR2,
                 ADC_CR2_ALIGN
              ,
                 ADC_InitStruct->DataAlignment
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
  ADC_InitStruct->DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;

  /* Enable scan mode to have a generic behavior with ADC of other            */
  /* STM32 families, without this setting available:                          */
  /* ADC group regular sequencer and ADC group injected sequencer depend      */
  /* only of their own configuration.                                         */
  ADC_InitStruct->SequencersScanMode      = LL_ADC_SEQ_SCAN_ENABLE;

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
  *          - Set ADC group regular or group injected sequencer:
  *            map channel on the selected sequencer rank.
  *            Refer to function @ref LL_ADC_REG_SetSequencerRanks().
  *          - Set ADC channel sampling time
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

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0U)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC group regular                                                   */
    /*    - Set ADC group regular trigger source                              */
    /*    - Set ADC group regular sequencer length                            */
    /*    - Set ADC group regular sequencer discontinuous mode                */
    /*    - Set ADC group regular continuous mode                             */
    /*    - Set ADC group regular conversion data transfer: no transfer or    */
    /*      transfer by DMA, and DMA requests mode                            */
    /* Note: On this STM32 serie, ADC trigger edge is set when starting        */
    /*       ADC conversion.                                                  */
    /*       Refer to function @ref LL_ADC_REG_StartConversionExtTrig().      */
    if(ADC_REG_InitStruct->SequencerLength != LL_ADC_REG_SEQ_SCAN_DISABLE)
    {
      MODIFY_REG(ADCx->CR1,
                   ADC_CR1_DISCEN
                 | ADC_CR1_DISCNUM
                ,
                   ADC_REG_InitStruct->SequencerLength
                 | ADC_REG_InitStruct->SequencerDiscont
                );
    }
    else
    {
      MODIFY_REG(ADCx->CR1,
                   ADC_CR1_DISCEN
                 | ADC_CR1_DISCNUM
                ,
                   ADC_REG_InitStruct->SequencerLength
                 | LL_ADC_REG_SEQ_DISCONT_DISABLE
                );
    }

    MODIFY_REG(ADCx->CR2,
                 ADC_CR2_EXTSEL
               | ADC_CR2_CONT
               | ADC_CR2_DMA
              ,
                 ADC_REG_InitStruct->TriggerSource
               | ADC_REG_InitStruct->ContinuousMode
               | ADC_REG_InitStruct->DMATransfer
              );

    /* Set ADC group regular sequencer length and scan direction */
    /* Note: Hardware constraint (refer to description of this function):     */
    /* Note: If ADC instance feature scan mode is disabled                    */
    /*       (refer to  ADC instance initialization structure                 */
    /*       parameter @ref SequencersScanMode                                */
    /*       or function @ref LL_ADC_SetSequencersScanMode() ),               */
    /*       this parameter is discarded.                                     */
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
  /* Note: On this STM32 serie, ADC trigger edge is set when starting         */
  /*       ADC conversion.                                                    */
  /*       Refer to function @ref LL_ADC_REG_StartConversionExtTrig().        */
  ADC_REG_InitStruct->TriggerSource    = LL_ADC_REG_TRIG_SOFTWARE;
  ADC_REG_InitStruct->SequencerLength  = LL_ADC_REG_SEQ_SCAN_DISABLE;
  ADC_REG_InitStruct->SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
  ADC_REG_InitStruct->ContinuousMode   = LL_ADC_REG_CONV_SINGLE;
  ADC_REG_InitStruct->DMATransfer      = LL_ADC_REG_DMA_TRANSFER_NONE;
}

/**
  * @brief  Initialize some features of ADC group injected.
  * @note   These parameters have an impact on ADC scope: ADC group injected.
  *         Refer to corresponding unitary functions into
  *         @ref ADC_LL_EF_Configuration_ADC_Group_Regular
  *         (functions with prefix "INJ").
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
  *          - Set ADC group injected sequencer:
  *            map channel on the selected sequencer rank.
  *            Refer to function @ref LL_ADC_INJ_SetSequencerRanks().
  *          - Set ADC channel sampling time
  *            Refer to function LL_ADC_SetChannelSamplingTime();
  * @param  ADCx ADC instance
  * @param  ADC_INJ_InitStruct Pointer to a @ref LL_ADC_INJ_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: ADC registers are initialized
  *          - ERROR: ADC registers are not initialized
  */
ErrorStatus LL_ADC_INJ_Init(ADC_TypeDef *ADCx, LL_ADC_INJ_InitTypeDef *ADC_INJ_InitStruct)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_ADC_ALL_INSTANCE(ADCx));
  assert_param(IS_LL_ADC_INJ_TRIG_SOURCE(ADC_INJ_InitStruct->TriggerSource));
  assert_param(IS_LL_ADC_INJ_SEQ_SCAN_LENGTH(ADC_INJ_InitStruct->SequencerLength));
  if(ADC_INJ_InitStruct->SequencerLength != LL_ADC_INJ_SEQ_SCAN_DISABLE)
  {
    assert_param(IS_LL_ADC_INJ_SEQ_SCAN_DISCONT_MODE(ADC_INJ_InitStruct->SequencerDiscont));
  }
  assert_param(IS_LL_ADC_INJ_TRIG_AUTO(ADC_INJ_InitStruct->TrigAuto));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       ADC instance must be disabled.                                     */
  if(LL_ADC_IsEnabled(ADCx) == 0U)
  {
    /* Configuration of ADC hierarchical scope:                               */
    /*  - ADC group injected                                                  */
    /*    - Set ADC group injected trigger source                             */
    /*    - Set ADC group injected sequencer length                           */
    /*    - Set ADC group injected sequencer discontinuous mode               */
    /*    - Set ADC group injected conversion trigger: independent or         */
    /*      from ADC group regular                                            */
    /* Note: On this STM32 serie, ADC trigger edge is set when starting       */
    /*       ADC conversion.                                                  */
    /*       Refer to function @ref LL_ADC_INJ_StartConversionExtTrig().      */
    if(ADC_INJ_InitStruct->SequencerLength != LL_ADC_REG_SEQ_SCAN_DISABLE)
    {
      MODIFY_REG(ADCx->CR1,
                   ADC_CR1_JDISCEN
                 | ADC_CR1_JAUTO
                ,
                   ADC_INJ_InitStruct->SequencerDiscont
                 | ADC_INJ_InitStruct->TrigAuto
                );
    }
    else
    {
      MODIFY_REG(ADCx->CR1,
                   ADC_CR1_JDISCEN
                 | ADC_CR1_JAUTO
                ,
                   LL_ADC_REG_SEQ_DISCONT_DISABLE
                 | ADC_INJ_InitStruct->TrigAuto
                );
    }

    MODIFY_REG(ADCx->CR2,
               ADC_CR2_JEXTSEL
              ,
               ADC_INJ_InitStruct->TriggerSource
              );

    /* Note: Hardware constraint (refer to description of this function):     */
    /* Note: If ADC instance feature scan mode is disabled                    */
    /*       (refer to  ADC instance initialization structure                 */
    /*       parameter @ref SequencersScanMode                                */
    /*       or function @ref LL_ADC_SetSequencersScanMode() ),               */
    /*       this parameter is discarded.                                     */
    LL_ADC_INJ_SetSequencerLength(ADCx, ADC_INJ_InitStruct->SequencerLength);
  }
  else
  {
    /* Initialization error: ADC instance is not disabled. */
    status = ERROR;
  }
  return status;
}

/**
  * @brief  Set each @ref LL_ADC_INJ_InitTypeDef field to default value.
  * @param  ADC_INJ_InitStruct Pointer to a @ref LL_ADC_INJ_InitTypeDef structure
  *                            whose fields will be set to default values.
  * @retval None
  */
void LL_ADC_INJ_StructInit(LL_ADC_INJ_InitTypeDef *ADC_INJ_InitStruct)
{
  /* Set ADC_INJ_InitStruct fields to default values */
  /* Set fields of ADC group injected */
  ADC_INJ_InitStruct->TriggerSource    = LL_ADC_INJ_TRIG_SOFTWARE;
  ADC_INJ_InitStruct->SequencerLength  = LL_ADC_INJ_SEQ_SCAN_DISABLE;
  ADC_INJ_InitStruct->SequencerDiscont = LL_ADC_INJ_SEQ_DISCONT_DISABLE;
  ADC_INJ_InitStruct->TrigAuto         = LL_ADC_INJ_TRIG_INDEPENDENT;
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


#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
