/**
  ******************************************************************************
  * @file    stm32f3xx_ll_comp.c
  * @author  MCD Application Team
  * @brief   COMP LL module driver
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
#include "stm32f3xx_ll_comp.h"

#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

/* Note: Devices of STM32F3 serie embed 1 out of 2 different comparator IP.   */
/*       - STM32F30x, STM32F31x, STM32F32x, STM32F33x, STM32F35x, STM32F39x:  */
/*         COMP IP from 3 to 7 instances and other specific features          */
/*         (comparator output blanking, ...) (refer to reference manual).     */
/*       - STM32F37x:                                                         */
/*         COMP IP with 2 instances                                           */
/*       This file contains the drivers of these COMP IP, located in 2 area    */
/*       delimited by compilation switches.                                   */

#if defined(COMP_V1_3_0_0)

#if defined (COMP1) || defined (COMP2) || defined (COMP3) || defined (COMP4) || defined (COMP5) || defined (COMP6) || defined (COMP7)

/** @addtogroup COMP_LL COMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/** @addtogroup COMP_LL_Private_Macros
  * @{
  */

/* Check of parameters for configuration of COMP hierarchical scope:          */
/* COMP instance.                                                             */

#if defined(COMP_CSR_COMPxMODE)
#define IS_LL_COMP_POWER_MODE(__POWER_MODE__)                                  \
  (   ((__POWER_MODE__) == LL_COMP_POWERMODE_HIGHSPEED)                        \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_MEDIUMSPEED)                      \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_LOWPOWER)                         \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_ULTRALOWPOWER)                    \
  )
#else
#define IS_LL_COMP_POWER_MODE(__POWER_MODE__)                                  \
  ((__POWER_MODE__) == LL_COMP_POWERMODE_HIGHSPEED)
#endif

/* Note: On this STM32 serie, comparator input plus parameters are            */
/*       the same on all COMP instances.                                      */
/*       However, comparator instance kept as macro parameter for             */
/*       compatibility with other STM32 families.                             */
#if defined(COMP_CSR_COMPxNONINSEL) && defined(LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1)
/* Note: On devices where bit COMP_CSR_COMPxNONINSEL is available,            */
/*       feature LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 is also available.         */
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (   ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                             \
   || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                             \
   || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1)                  \
  )
#elif defined(COMP_CSR_COMPxNONINSEL) && defined(LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2)
/* Note: On devices where bit COMP_CSR_COMPxNONINSEL is available,            */
/*       feature LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 is also available.         */
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (   ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                             \
   || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                             \
   || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2)                  \
  )
#elif defined(COMP_CSR_COMPxNONINSEL)
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (   ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                             \
   || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO2)                             \
  )
#elif defined(LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2)
/* Note: On devices where bit COMP_CSR_COMPxNONINSEL is available,            */
/*       feature LL_COMP_INPUT_PLUS_DAC1_CH1_COMP1 is also available.         */
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (   ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                             \
   || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_DAC1_CH1_COMP2)                  \
  )
#else
/* Note: Device without comparator input plus configurable: corresponds to    */
/*       setting "LL_COMP_INPUT_PLUS_IO1" or "LL_COMP_INPUT_PLUS_IO2"         */
/*       compared to other STM32F3 devices, depending on comparator instance  */
/*       (refer to reference manual).                                         */
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)
#endif

#if defined(STM32F303xC) || defined(STM32F358xx) || defined(STM32F303xE) || defined(STM32F398xx)
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (   ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_2VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_3_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                       \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH2)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO2)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO3)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO4)                           \
  )
#elif defined(STM32F303x8) || defined(STM32F328xx) || defined(STM32F334x8)
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (   ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_2VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_3_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                       \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH2)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO2)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO4)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC2_CH1)                      \
  )
#elif defined(STM32F302xC) || defined(STM32F302xE)
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (   ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_2VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_3_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                       \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO2)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO3)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO4)                           \
  )
#else /* STM32F301x8 || STM32F318xx || STM32F302x8 */
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (   ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_2VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_3_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                       \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO2)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO4)                           \
  )
#endif

#if defined(COMP_CSR_COMPxHYST)
#define IS_LL_COMP_INPUT_HYSTERESIS(__INPUT_HYSTERESIS__)                      \
  (   ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_NONE)                      \
   || ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_LOW)                       \
   || ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_MEDIUM)                    \
   || ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_HIGH)                      \
  )
#else
#define IS_LL_COMP_INPUT_HYSTERESIS(__INPUT_HYSTERESIS__)                      \
  ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_NONE)
#endif

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
#define IS_LL_COMP_OUTPUT_SELECTION(__COMP_INSTANCE__, __OUTPUT_SELECTION__)   \
  ((    ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                        \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN)                   \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN2)                  \
   )                                                                           \
    ? (                                                                        \
       (1U)                                                                    \
      )                                                                        \
      :                                                                        \
      (((__COMP_INSTANCE__) == COMP2)                                          \
        ? (                                                                    \
              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC1_COMP2)        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4_COMP2)        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP2)      \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP2)      \
          )                                                                    \
          :                                                                    \
          (((__COMP_INSTANCE__) == COMP4)                                      \
            ? (                                                                \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC2_COMP4)   \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_OCCLR_COMP4) \
              )                                                                \
              :                                                                \
              (                                                                \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC2_COMP6)    \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP6)  \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_IC1_COMP6)   \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_OCCLR_COMP6) \
              )                                                                \
          )                                                                    \
      )                                                                        \
  )
#elif defined(STM32F303x8) || defined(STM32F328xx) || defined(STM32F334x8)
#define IS_LL_COMP_OUTPUT_SELECTION(__COMP_INSTANCE__, __OUTPUT_SELECTION__)   \
  ((    ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                        \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN)                   \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN2)                  \
   )                                                                           \
    ? (                                                                        \
       (1U)                                                                    \
      )                                                                        \
      :                                                                        \
      (((__COMP_INSTANCE__) == COMP2)                                          \
        ? (                                                                    \
              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP2_4)    \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC1_COMP2)        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4_COMP2)        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP2)      \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP2)      \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC1_COMP2)        \
          )                                                                    \
          :                                                                    \
          (((__COMP_INSTANCE__) == COMP4)                                      \
            ? (                                                                \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP2_4)\
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC3_COMP4)    \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC2_COMP4)   \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_OCCLR_COMP4) \
              )                                                                \
              :                                                                \
              (                                                                \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC2_COMP6)    \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP6)  \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_IC1_COMP6)   \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_OCCLR_COMP6) \
              )                                                                \
          )                                                                    \
      )                                                                        \
  )
#elif defined(STM32F302xC) || defined(STM32F302xE)
#define IS_LL_COMP_OUTPUT_SELECTION(__COMP_INSTANCE__, __OUTPUT_SELECTION__)      \
  ((    ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                           \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN)                      \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN2)                     \
   )                                                                              \
    ? (                                                                           \
       (1U)                                                                       \
      )                                                                           \
      :                                                                           \
      ((((__COMP_INSTANCE__) == COMP1) || ((__COMP_INSTANCE__) == COMP2))         \
        ? (                                                                       \
              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4)     \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC1_COMP1_2)         \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4_COMP1_2)         \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2)       \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2)       \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC1_COMP1_2)         \
          )                                                                       \
          :                                                                       \
          (((__COMP_INSTANCE__) == COMP4)                                         \
            ? (                                                                   \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4) \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC3_COMP4)       \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC2_COMP4)       \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC2_COMP4)      \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_OCCLR_COMP4)    \
              )                                                                   \
              :                                                                   \
              (                                                                   \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4) \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC2_COMP6)       \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP6)     \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC4_COMP6)       \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_IC1_COMP6)      \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_OCCLR_COMP6)    \
              )                                                                   \
          )                                                                       \
      )                                                                           \
  )
#elif defined(STM32F303xC) || defined(STM32F358xx)
#define IS_LL_COMP_OUTPUT_SELECTION(__COMP_INSTANCE__, __OUTPUT_SELECTION__)                     \
  ((    ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                                          \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN)                                     \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN2)                                    \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_TIM8_BKIN2)                               \
   )                                                                                             \
    ? (                                                                                          \
       (1U)                                                                                      \
      )                                                                                          \
      :                                                                                          \
      ((((__COMP_INSTANCE__) == COMP1) || ((__COMP_INSTANCE__) == COMP2))                        \
        ? (                                                                                      \
              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7)                  \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3)                    \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5)                  \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC1_COMP1_2)                        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4_COMP1_2)                        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC1_COMP1_2)                        \
          )                                                                                      \
          :                                                                                      \
          (((__COMP_INSTANCE__) == COMP3)                                                        \
            ? (                                                                                  \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7)              \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3)                \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC2_COMP3)                      \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC1_COMP3)                      \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC1_COMP3)                     \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_BKIN)                          \
              )                                                                                  \
              :                                                                                  \
              (((__COMP_INSTANCE__) == COMP4)                                                    \
                ? (                                                                              \
                      ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5)          \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC3_COMP4)                  \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC2_COMP4)                  \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC2_COMP4)                 \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_OCCLR_COMP4)               \
                  )                                                                              \
                  :                                                                              \
                  (((__COMP_INSTANCE__) == COMP5)                                                \
                    ? (                                                                          \
                          ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5)      \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7)      \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC1_COMP5)              \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC3_COMP5)              \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM17_IC1_COMP5)             \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_BKIN)                  \
                      )                                                                          \
                      :                                                                          \
                      (((__COMP_INSTANCE__) == COMP6)                                            \
                        ? (                                                                      \
                              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7)  \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC2_COMP6)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP6)        \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC4_COMP6)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_IC1_COMP6)         \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_OCCLR_COMP6)       \
                          )                                                                      \
                          :                                                                      \
                          (                                                                      \
                              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7)  \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7)  \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC2_COMP7)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC3_COMP7)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM17_OCCLR_COMP7)       \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM17_BKIN)              \
                          )                                                                      \
                      )                                                                          \
                  )                                                                              \
              )                                                                                  \
          )                                                                                      \
      )                                                                                          \
  )
#elif defined(STM32F303xE) || defined(STM32F398xx)
#define IS_LL_COMP_OUTPUT_SELECTION(__COMP_INSTANCE__, __OUTPUT_SELECTION__)                     \
  ((    ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                                          \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN)                                     \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_BKIN2)                                    \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_TIM8_BKIN2)                               \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM20_BKIN)                                    \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM20_BKIN2)                                   \
     || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_TIM8_TIM20_BKIN2)                         \
   )                                                                                             \
    ? (                                                                                          \
       (1U)                                                                                      \
      )                                                                                          \
      :                                                                                          \
      ((((__COMP_INSTANCE__) == COMP1) || ((__COMP_INSTANCE__) == COMP2))                        \
        ? (                                                                                      \
              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7)                  \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3)                    \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5)                  \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC1_COMP1_2)                        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4_COMP1_2)                        \
           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC1_COMP1_2)                        \
           || (((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM20_OCCLR_COMP2)                      \
               && ((__COMP_INSTANCE__) == COMP2)                     )                           \
          )                                                                                      \
          :                                                                                      \
          (((__COMP_INSTANCE__) == COMP3)                                                        \
            ? (                                                                                  \
                  ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7)              \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP1_2_3)                \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC2_COMP3)                      \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC1_COMP3)                      \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC1_COMP3)                     \
               || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_BKIN)                          \
              )                                                                                  \
              :                                                                                  \
              (((__COMP_INSTANCE__) == COMP4)                                                    \
                ? (                                                                              \
                      ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5)          \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC3_COMP4)                  \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC2_COMP4)                  \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_IC2_COMP4)                 \
                   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM15_OCCLR_COMP4)               \
                  )                                                                              \
                  :                                                                              \
                  (((__COMP_INSTANCE__) == COMP5)                                                \
                    ? (                                                                          \
                          ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP1_2_4_5)      \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7)      \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC1_COMP5)              \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC3_COMP5)              \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM17_IC1_COMP5)             \
                       || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_BKIN)                  \
                      )                                                                          \
                      :                                                                          \
                      (((__COMP_INSTANCE__) == COMP6)                                            \
                        ? (                                                                      \
                              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7)  \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC2_COMP6)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR_COMP6)        \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC4_COMP6)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_IC1_COMP6)         \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_OCCLR_COMP6)       \
                          )                                                                      \
                          :                                                                      \
                          (                                                                      \
                              ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_OCCLR_COMP1_2_3_7)  \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM8_OCCLR_COMP4_5_6_7)  \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM1_IC2_COMP7)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC3_COMP7)          \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM17_OCCLR_COMP7)       \
                           || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM17_BKIN)              \
                          )                                                                      \
                      )                                                                          \
                  )                                                                              \
              )                                                                                  \
          )                                                                                      \
      )                                                                                          \
  )
#endif

#define IS_LL_COMP_OUTPUT_POLARITY(__POLARITY__)                               \
  (   ((__POLARITY__) == LL_COMP_OUTPUTPOL_NONINVERTED)                        \
   || ((__POLARITY__) == LL_COMP_OUTPUTPOL_INVERTED)                           \
  )

#if defined(COMP_CSR_COMPxBLANKING)
#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx) || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)
#define IS_LL_COMP_OUTPUT_BLANKING_SOURCE(__COMP_INSTANCE__, __OUTPUT_BLANKING_SOURCE__)       \
  (((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_NONE)                                  \
    ? (                                                                                        \
       (1U)                                                                                    \
      )                                                                                        \
      :                                                                                        \
      (((__COMP_INSTANCE__) == COMP2)                                                          \
        ? (                                                                                    \
              ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM1_OC5_COMP2)             \
           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC3_COMP2)             \
           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM3_OC3_COMP2)             \
          )                                                                                    \
          :                                                                                    \
          (((__COMP_INSTANCE__) == COMP4)                                                      \
            ? (                                                                                \
                  ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4)         \
               || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4)        \
              )                                                                                \
              :                                                                                \
              (                                                                                \
                (((__COMP_INSTANCE__) == COMP6)                                                \
                  ? (                                                                          \
                        ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6)   \
                     || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6)  \
                    )                                                                          \
                    :                                                                          \
                    (                                                                          \
                     (0U)                                                                      \
                    )                                                                          \
                )                                                                              \
              )                                                                                \
          )                                                                                    \
      )                                                                                        \
  )
#elif defined(STM32F302xE) || defined(STM32F302xC)
#define IS_LL_COMP_OUTPUT_BLANKING_SOURCE(__COMP_INSTANCE__, __OUTPUT_BLANKING_SOURCE__)       \
  (((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_NONE)                                  \
    ? (                                                                                        \
       (1U)                                                                                    \
      )                                                                                        \
      :                                                                                        \
      ((((__COMP_INSTANCE__) == COMP1) || ((__COMP_INSTANCE__) == COMP2))                      \
        ? (                                                                                    \
              ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2)           \
           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2)           \
           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2)           \
          )                                                                                    \
          :                                                                                    \
          (((__COMP_INSTANCE__) == COMP4)                                                      \
            ? (                                                                                \
                  ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4)         \
               || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4)        \
              )                                                                                \
              :                                                                                \
              (                                                                                \
                (((__COMP_INSTANCE__) == COMP6)                                                \
                  ? (                                                                          \
                        ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC4_COMP6)   \
                     || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6)  \
                    )                                                                          \
                    :                                                                          \
                    (                                                                          \
                     (0U)                                                                      \
                    )                                                                          \
                )                                                                              \
              )                                                                                \
          )                                                                                    \
      )                                                                                        \
  )
#elif defined(STM32F303xE) || defined(STM32F398xx) || defined(STM32F303xC) || defined(STM32F358xx)
#define IS_LL_COMP_OUTPUT_BLANKING_SOURCE(__COMP_INSTANCE__, __OUTPUT_BLANKING_SOURCE__)                 \
  (((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_NONE)                                            \
    ? (                                                                                                  \
       (1U)                                                                                              \
      )                                                                                                  \
      :                                                                                                  \
      ((((__COMP_INSTANCE__) == COMP1) || ((__COMP_INSTANCE__) == COMP2))                                \
        ? (                                                                                              \
              ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2_7)                   \
           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC3_COMP1_2)                     \
           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM3_OC3_COMP1_2)                     \
          )                                                                                              \
          :                                                                                              \
          (((__COMP_INSTANCE__) == COMP3)                                                                \
            ? (                                                                                          \
               ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC4_COMP3_6)                    \
              )                                                                                          \
              :                                                                                          \
              (((__COMP_INSTANCE__) == COMP4)                                                            \
                ? (                                                                                      \
                      ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7)         \
                   || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM3_OC4_COMP4)               \
                   || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC1_COMP4)              \
                  )                                                                                      \
                  :                                                                                      \
                  (((__COMP_INSTANCE__) == COMP5)                                                        \
                    ? (                                                                                  \
                          ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7)     \
                      )                                                                                  \
                      :                                                                                  \
                      (((__COMP_INSTANCE__) == COMP6)                                                    \
                        ? (                                                                              \
                              ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7) \
                           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM2_OC4_COMP3_6)     \
                           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6_7)    \
                          )                                                                              \
                          :                                                                              \
                          (                                                                              \
                              ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM8_OC5_COMP4_5_6_7) \
                           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM1_OC5_COMP1_2_7)   \
                           || ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_TIM15_OC2_COMP6_7)    \
                          )                                                                              \
                      )                                                                                  \
                  )                                                                                      \
              )                                                                                          \
          )                                                                                              \
      )                                                                                                  \
  )
#endif
#else
#define IS_LL_COMP_OUTPUT_BLANKING_SOURCE(__COMP_INSTANCE__, __OUTPUT_BLANKING_SOURCE__) \
  ((__OUTPUT_BLANKING_SOURCE__) == LL_COMP_BLANKINGSRC_NONE)
#endif
/**
  * @}
  */


/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMP_LL_Exported_Functions
  * @{
  */

/** @addtogroup COMP_LL_EF_Init
  * @{
  */

/**
  * @brief  De-initialize registers of the selected COMP instance
  *         to their default reset values.
  * @note   If comparator is locked, de-initialization by software is
  *         not possible.
  *         The only way to unlock the comparator is a device hardware reset.
  * @param  COMPx COMP instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: COMP registers are de-initialized
  *          - ERROR: COMP registers are not de-initialized
  */
ErrorStatus LL_COMP_DeInit(COMP_TypeDef *COMPx)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_COMP_ALL_INSTANCE(COMPx));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       COMP instance must not be locked.                                  */
  if(LL_COMP_IsLocked(COMPx) == 0U)
  {
    LL_COMP_WriteReg(COMPx, CSR, 0x00000000U);
  }
  else
  {
    /* Comparator instance is locked: de-initialization by software is         */
    /* not possible.                                                           */
    /* The only way to unlock the comparator is a device hardware reset.       */
    status = ERROR;
  }

  return status;
}

/**
  * @brief  Initialize some features of COMP instance.
  * @note   This function configures features of the selected COMP instance.
  *         Some features are also available at scope COMP common instance
  *         (common to several COMP instances).
  *         Refer to functions having argument "COMPxy_COMMON" as parameter.
  * @param  COMPx COMP instance
  * @param  COMP_InitStruct Pointer to a @ref LL_COMP_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: COMP registers are initialized
  *          - ERROR: COMP registers are not initialized
  */
ErrorStatus LL_COMP_Init(COMP_TypeDef *COMPx, LL_COMP_InitTypeDef *COMP_InitStruct)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_COMP_ALL_INSTANCE(COMPx));
  assert_param(IS_LL_COMP_POWER_MODE(COMP_InitStruct->PowerMode));
  assert_param(IS_LL_COMP_INPUT_PLUS(COMPx, COMP_InitStruct->InputPlus));
  assert_param(IS_LL_COMP_INPUT_MINUS(COMPx, COMP_InitStruct->InputMinus));
  assert_param(IS_LL_COMP_INPUT_HYSTERESIS(COMP_InitStruct->InputHysteresis));
  assert_param(IS_LL_COMP_OUTPUT_SELECTION(COMPx, COMP_InitStruct->OutputSelection));
  assert_param(IS_LL_COMP_OUTPUT_POLARITY(COMP_InitStruct->OutputPolarity));
  assert_param(IS_LL_COMP_OUTPUT_BLANKING_SOURCE(COMPx, COMP_InitStruct->OutputBlankingSource));

  /* Note: Hardware constraint (refer to description of this function)        */
  /*       COMP instance must not be locked.                                  */
  if(LL_COMP_IsLocked(COMPx) == 0U)
  {
    /* Configuration of comparator instance :                                 */
    /*  - PowerMode                                                           */
    /*  - InputPlus                                                           */
    /*  - InputMinus                                                          */
    /*  - InputHysteresis                                                     */
    /*  - OutputSelection                                                     */
    /*  - OutputPolarity                                                      */
    /*  - OutputBlankingSource                                                */
    MODIFY_REG(COMPx->CSR,
                 ((uint32_t)0x00000000U)
#if defined(COMP_CSR_COMPxMODE)
               | COMP_CSR_COMPxMODE
#endif
#if defined(COMP_CSR_COMPxNONINSEL)
               | COMP_CSR_COMPxNONINSEL
#endif
               | COMP_CSR_COMPxINSEL
#if defined(COMP_CSR_COMPxHYST)
               | COMP_CSR_COMPxHYST
#endif
               | COMP_CSR_COMPxOUTSEL
               | COMP_CSR_COMPxPOL
               | COMP_CSR_COMPxBLANKING
              ,
                 ((uint32_t)0x00000000U)
#if defined(COMP_CSR_COMPxMODE)
               | COMP_InitStruct->PowerMode
#endif
#if defined(COMP_CSR_COMPxNONINSEL)
               | COMP_InitStruct->InputPlus
#endif
               | COMP_InitStruct->InputMinus
#if defined(COMP_CSR_COMPxHYST)
               | COMP_InitStruct->InputHysteresis
#endif
               | COMP_InitStruct->OutputSelection
               | COMP_InitStruct->OutputPolarity
               | COMP_InitStruct->OutputBlankingSource
              );

  }
  else
  {
    /* Initialization error: COMP instance is locked.                         */
    status = ERROR;
  }

  return status;
}

/**
  * @brief Set each @ref LL_COMP_InitTypeDef field to default value.
  * @param COMP_InitStruct pointer to a @ref LL_COMP_InitTypeDef structure
  *                         whose fields will be set to default values.
  * @retval None
  */
void LL_COMP_StructInit(LL_COMP_InitTypeDef *COMP_InitStruct)
{
  /* Set COMP_InitStruct fields to default values */
  /* Note: Comparator power mode "high speed" is the only mode                */
  /*       available on all STMF3 devices.                                    */
  COMP_InitStruct->PowerMode            = LL_COMP_POWERMODE_HIGHSPEED;
  COMP_InitStruct->InputPlus            = LL_COMP_INPUT_PLUS_IO1;
  COMP_InitStruct->InputMinus           = LL_COMP_INPUT_MINUS_VREFINT;
  COMP_InitStruct->InputHysteresis      = LL_COMP_HYSTERESIS_NONE;
  COMP_InitStruct->OutputSelection      = LL_COMP_OUTPUT_NONE;
  COMP_InitStruct->OutputPolarity       = LL_COMP_OUTPUTPOL_NONINVERTED;
  COMP_InitStruct->OutputBlankingSource = LL_COMP_BLANKINGSRC_NONE;
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

#endif /* COMP1 || COMP2 || COMP3 || COMP4 || COMP5 || COMP6 || COMP7 */


#endif /* STM32F301x8 || STM32F302x8 || STM32F302xC || STM32F302xE || STM32F303x8 || STM32F303xC || STM32F303xE || STM32F318xx || STM32F328xx || STM32F334x8 || STM32F358xx || STM32F398xx */

#if defined (COMP_V1_1_0_0)

#if defined (COMP1) || defined (COMP2)

/** @addtogroup COMP_LL COMP
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/** @addtogroup COMP_LL_Private_Macros
  * @{
  */

/* Check of parameters for configuration of COMP hierarchical scope:          */
/* COMP instance.                                                             */

#define IS_LL_COMP_POWER_MODE(__POWER_MODE__)                                  \
  (   ((__POWER_MODE__) == LL_COMP_POWERMODE_HIGHSPEED)                        \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_MEDIUMSPEED)                      \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_LOWPOWER)                         \
   || ((__POWER_MODE__) == LL_COMP_POWERMODE_ULTRALOWPOWER)                    \
  )

/* Note: On this STM32 serie, comparator input plus parameters are            */
/*       the different depending on COMP instances.                           */
#define IS_LL_COMP_INPUT_PLUS(__COMP_INSTANCE__, __INPUT_PLUS__)               \
  (((__COMP_INSTANCE__) == COMP1)                                              \
    ? (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                         \
       || ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_DAC1_CH1)                    \
      )                                                                        \
      :                                                                        \
      (                                                                        \
          ((__INPUT_PLUS__) == LL_COMP_INPUT_PLUS_IO1)                         \
      )                                                                        \
  )

/* Note: On this STM32 serie, comparator input minus parameters are           */
/*       the same on all COMP instances.                                      */
/*       However, comparator instance kept as macro parameter for             */
/*       compatibility with other STM32 families.                             */
#define IS_LL_COMP_INPUT_MINUS(__COMP_INSTANCE__, __INPUT_MINUS__)             \
  (   ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_1_2VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_3_4VREFINT)                    \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_VREFINT)                       \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH1)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC1_CH2)                      \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO1)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO2)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO3)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_IO4)                           \
   || ((__INPUT_MINUS__) == LL_COMP_INPUT_MINUS_DAC2_CH1)                      \
  )

#define IS_LL_COMP_INPUT_HYSTERESIS(__INPUT_HYSTERESIS__)                      \
  (   ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_NONE)                      \
   || ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_LOW)                       \
   || ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_MEDIUM)                    \
   || ((__INPUT_HYSTERESIS__) == LL_COMP_HYSTERESIS_HIGH)                      \
  )

/* Note: Output redirection is specific to COMP instances but is checked      */
/*       with literals of instance COMP2 (no differentiation possible since   */
/*       literals of COMP1 and COMP2 share the same values range).            */
#define IS_LL_COMP_OUTPUT_SELECTION(__OUTPUT_SELECTION__)                      \
  (   ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_NONE)                          \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM16_BKIN)                    \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_IC1)                      \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM4_OCCLR)                    \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_IC4)                      \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM2_OCCLR)                    \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_IC1_COMP2)                \
   || ((__OUTPUT_SELECTION__) == LL_COMP_OUTPUT_TIM3_OCCLR_COMP2)              \
  )

#define IS_LL_COMP_OUTPUT_POLARITY(__POLARITY__)                               \
  (   ((__POLARITY__) == LL_COMP_OUTPUTPOL_NONINVERTED)                        \
   || ((__POLARITY__) == LL_COMP_OUTPUTPOL_INVERTED)                           \
  )

/**
  * @}
  */


/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @addtogroup COMP_LL_Exported_Functions
  * @{
  */

/** @addtogroup COMP_LL_EF_Init
  * @{
  */

/**
  * @brief  De-initialize registers of the selected COMP instance
  *         to their default reset values.
  * @note   If comparator is locked, de-initialization by software is
  *         not possible.
  *         The only way to unlock the comparator is a device hardware reset.
  * @param  COMPx COMP instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: COMP registers are de-initialized
  *          - ERROR: COMP registers are not de-initialized
  */
ErrorStatus LL_COMP_DeInit(COMP_TypeDef *COMPx)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_COMP_ALL_INSTANCE(COMPx));

  /* Note: Hardware constraint (refer to description of this function):       */
  /*       COMP instance must not be locked.                                  */
  if(LL_COMP_IsLocked(COMPx) == 0U)
  {
    /* Note: Connection switch is applicable only to COMP instance COMP1,     */
    /*       therefore is COMP2 is selected the equivalent bit is             */
    /*       kept unmodified.                                                 */
    if(COMPx == COMP1)
    {
      CLEAR_BIT(COMP->CSR,
                (  COMP_CSR_COMP1MODE
                 | COMP_CSR_COMP1INSEL
                 | COMP_CSR_COMP1SW1
                 | COMP_CSR_COMP1OUTSEL
                 | COMP_CSR_COMP1HYST
                 | COMP_CSR_COMP1POL
                 | COMP_CSR_COMP1EN
                ) << __COMP_BITOFFSET_INSTANCE(COMPx)
               );
    }
    else
    {
      CLEAR_BIT(COMP->CSR,
                (  COMP_CSR_COMP1MODE
                 | COMP_CSR_COMP1INSEL
                 | COMP_CSR_COMP1OUTSEL
                 | COMP_CSR_COMP1HYST
                 | COMP_CSR_COMP1POL
                 | COMP_CSR_COMP1EN
                ) << __COMP_BITOFFSET_INSTANCE(COMPx)
               );
    }

  }
  else
  {
    /* Comparator instance is locked: de-initialization by software is         */
    /* not possible.                                                           */
    /* The only way to unlock the comparator is a device hardware reset.       */
    status = ERROR;
  }

  return status;
}

/**
  * @brief  Initialize some features of COMP instance.
  * @note   This function configures features of the selected COMP instance.
  *         Some features are also available at scope COMP common instance
  *         (common to several COMP instances).
  *         Refer to functions having argument "COMPxy_COMMON" as parameter.
  * @param  COMPx COMP instance
  * @param  COMP_InitStruct Pointer to a @ref LL_COMP_InitTypeDef structure
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: COMP registers are initialized
  *          - ERROR: COMP registers are not initialized
  */
ErrorStatus LL_COMP_Init(COMP_TypeDef *COMPx, LL_COMP_InitTypeDef *COMP_InitStruct)
{
  ErrorStatus status = SUCCESS;

  /* Check the parameters */
  assert_param(IS_COMP_ALL_INSTANCE(COMPx));
  assert_param(IS_LL_COMP_POWER_MODE(COMP_InitStruct->PowerMode));
  assert_param(IS_LL_COMP_INPUT_PLUS(COMPx, COMP_InitStruct->InputPlus));
  assert_param(IS_LL_COMP_INPUT_MINUS(COMPx, COMP_InitStruct->InputMinus));
  assert_param(IS_LL_COMP_INPUT_HYSTERESIS(COMP_InitStruct->InputHysteresis));
  assert_param(IS_LL_COMP_OUTPUT_SELECTION(COMP_InitStruct->OutputSelection));
  assert_param(IS_LL_COMP_OUTPUT_POLARITY(COMP_InitStruct->OutputPolarity));

  /* Note: Hardware constraint (refer to description of this function)        */
  /*       COMP instance must not be locked.                                  */
  if(LL_COMP_IsLocked(COMPx) == 0U)
  {
    /* Configuration of comparator instance :                                 */
    /*  - PowerMode                                                           */
    /*  - InputPlus                                                           */
    /*  - InputMinus                                                          */
    /*  - InputHysteresis                                                     */
    /*  - OutputSelection                                                     */
    /*  - OutputPolarity                                                      */
    /* Note: Connection switch is applicable only to COMP instance COMP1,     */
    /*       therefore is COMP2 is selected the equivalent bit is             */
    /*       kept unmodified.                                                 */
    if(COMPx == COMP1)
    {
      MODIFY_REG(COMP->CSR,
                 (  COMP_CSR_COMP1MODE
                  | COMP_CSR_COMP1INSEL
                  | COMP_CSR_COMP1SW1
                  | COMP_CSR_COMP1OUTSEL
                  | COMP_CSR_COMP1HYST
                  | COMP_CSR_COMP1POL
                 ) << __COMP_BITOFFSET_INSTANCE(COMPx)
                ,
                 (  COMP_InitStruct->PowerMode
                  | COMP_InitStruct->InputPlus
                  | COMP_InitStruct->InputMinus
                  | COMP_InitStruct->InputHysteresis
                  | COMP_InitStruct->OutputSelection
                  | COMP_InitStruct->OutputPolarity
                 ) << __COMP_BITOFFSET_INSTANCE(COMPx)
                );
    }
    else
    {
      MODIFY_REG(COMP->CSR,
                 (  COMP_CSR_COMP1MODE
                  | COMP_CSR_COMP1INSEL
                  | COMP_CSR_COMP1OUTSEL
                  | COMP_CSR_COMP1HYST
                  | COMP_CSR_COMP1POL
                 ) << __COMP_BITOFFSET_INSTANCE(COMPx)
                ,
                 (  COMP_InitStruct->PowerMode
                  | COMP_InitStruct->InputPlus
                  | COMP_InitStruct->InputMinus
                  | COMP_InitStruct->InputHysteresis
                  | COMP_InitStruct->OutputSelection
                  | COMP_InitStruct->OutputPolarity
                 ) << __COMP_BITOFFSET_INSTANCE(COMPx)
                );
    }

  }
  else
  {
    /* Initialization error: COMP instance is locked.                         */
    status = ERROR;
  }

  return status;
}

/**
  * @brief Set each @ref LL_COMP_InitTypeDef field to default value.
  * @param COMP_InitStruct pointer to a @ref LL_COMP_InitTypeDef structure
  *                         whose fields will be set to default values.
  * @retval None
  */
void LL_COMP_StructInit(LL_COMP_InitTypeDef *COMP_InitStruct)
{
  /* Set COMP_InitStruct fields to default values */
  COMP_InitStruct->PowerMode            = LL_COMP_POWERMODE_ULTRALOWPOWER;
  COMP_InitStruct->InputPlus            = LL_COMP_INPUT_PLUS_IO1;
  COMP_InitStruct->InputMinus           = LL_COMP_INPUT_MINUS_VREFINT;
  COMP_InitStruct->InputHysteresis      = LL_COMP_HYSTERESIS_NONE;
  COMP_InitStruct->OutputSelection      = LL_COMP_OUTPUT_NONE;
  COMP_InitStruct->OutputPolarity       = LL_COMP_OUTPUTPOL_NONINVERTED;
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

#endif /* COMP1 || COMP2 */


#endif /* STM32F373xC || STM32F378xx */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
