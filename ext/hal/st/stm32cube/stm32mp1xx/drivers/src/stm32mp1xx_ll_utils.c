/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_utils.c
  * @author  MCD Application Team
  * @brief   UTILS LL module driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_ll_utils.h"
#include "stm32mp1xx_ll_system.h"
#include "stm32mp1xx_ll_bus.h"
#include "stm32mp1xx_ll_rcc.h"

#ifdef  USE_FULL_ASSERT
#include "stm32_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */

/** @addtogroup STM32MP1xx_LL_Driver
  * @{
  */

/** @addtogroup UTILS_LL
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @addtogroup UTILS_LL_Private_Constants
  * @{
  */

/* Defines used for PLL type */
#define UTILS_PLL_1600_TYPE       00000000U
#define UTILS_PLL_800_TYPE        00000001U

/* Defines used for PLL range */
#define UTILS_PLL1600_INPUT_MIN    8000000U /*!< Frequency min for PLL1600 type input, in Hz */
#define UTILS_PLL1600_INPUT_MAX   16000000U /*!< Frequency max for PLL1600 type input, in Hz */
#define UTILS_PLL800_INPUT_MIN     4000000U /*!< Frequency min for PLL800 type input, in Hz */
#define UTILS_PLL800_INPUT_MED     8000000U /*!< Frequency medium range for PLL800 type input, in Hz */
#define UTILS_PLL800_INPUT_MAX    16000000U /*!< Frequency max for PLL800 type input, in Hz */

#define UTILS_PLL1600_OUTPUT_MIN    800000000U /*!< Frequency min for PLL1600 type output, in Hz */
#define UTILS_PLL1600_OUTPUT_MAX   1600000000U /*!< Frequency max for PLL1600 type output, in Hz */
#define UTILS_PLL800_OUTPUT_MIN     400000000U /*!< Frequency min for PLL800 type output, in Hz */
#define UTILS_PLL800_OUTPUT_MAX     800000000U /*!< Frequency max for PLL800 type output, in Hz */

/* Defines used for HSE range */
#define UTILS_HSE_FREQUENCY_MIN      8000000U        /*!< Frequency min for HSE frequency, in Hz   */
#define UTILS_HSE_FREQUENCY_MAX     48000000U        /*!< Frequency max for HSE frequency, in Hz   */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @addtogroup UTILS_LL_Private_Macros
  * @{
  */
#define IS_LL_UTILS_APB1_DIV(__VALUE__) (((__VALUE__) == LL_RCC_APB1_DIV_1) \
                                      || ((__VALUE__) == LL_RCC_APB1_DIV_2) \
                                      || ((__VALUE__) == LL_RCC_APB1_DIV_4) \
                                      || ((__VALUE__) == LL_RCC_APB1_DIV_8) \
                                      || ((__VALUE__) == LL_RCC_APB1_DIV_16))

#define IS_LL_UTILS_APB2_DIV(__VALUE__) (((__VALUE__) == LL_RCC_APB2_DIV_1) \
                                      || ((__VALUE__) == LL_RCC_APB2_DIV_2) \
                                      || ((__VALUE__) == LL_RCC_APB2_DIV_4) \
                                      || ((__VALUE__) == LL_RCC_APB2_DIV_8) \
                                      || ((__VALUE__) == LL_RCC_APB2_DIV_16))

#define IS_LL_UTILS_APB3_DIV(__VALUE__) (((__VALUE__) == LL_RCC_APB3_DIV_1) \
                                      || ((__VALUE__) == LL_RCC_APB3_DIV_2) \
                                      || ((__VALUE__) == LL_RCC_APB3_DIV_4) \
                                      || ((__VALUE__) == LL_RCC_APB3_DIV_8) \
                                      || ((__VALUE__) == LL_RCC_APB3_DIV_16))

#define IS_LL_UTILS_APB4_DIV(__VALUE__) (((__VALUE__) == LL_RCC_APB4_DIV_1) \
                                      || ((__VALUE__) == LL_RCC_APB4_DIV_2) \
                                      || ((__VALUE__) == LL_RCC_APB4_DIV_4) \
                                      || ((__VALUE__) == LL_RCC_APB4_DIV_8) \
                                      || ((__VALUE__) == LL_RCC_APB4_DIV_16))

#define IS_LL_UTILS_APB5_DIV(__VALUE__) (((__VALUE__) == LL_RCC_APB5_DIV_1) \
                                      || ((__VALUE__) == LL_RCC_APB5_DIV_2) \
                                      || ((__VALUE__) == LL_RCC_APB5_DIV_4) \
                                      || ((__VALUE__) == LL_RCC_APB5_DIV_8) \
                                      || ((__VALUE__) == LL_RCC_APB5_DIV_16))

#define IS_LL_UTILS_PLLM_VALUE(__VALUE__) ((1U <= (__VALUE__)) && ((__VALUE__) <= 64U))

#define IS_LL_UTILS_PLLN_VALUE(__VALUE__) ((4U <= (__VALUE__)) && ((__VALUE__) <= 512U))

#define IS_LL_UTILS_PLLP_VALUE(__VALUE__) ((1U <= (__VALUE__)) && ((__VALUE__) <= 128U))

#define IS_LL_UTILS_FRACV_VALUE(__VALUE__) ((__VALUE__) <= 0x1FFFU)

#define IS_LL_UTILS_PLLVCO_INPUT(__VALUE__, __TYPE__)  ( \
(((__TYPE__) == UTILS_PLL_1600_TYPE) && (UTILS_PLL1600_INPUT_MIN <= (__VALUE__)) && ((__VALUE__) <= UTILS_PLL1600_INPUT_MAX)) || \
(((__TYPE__) == UTILS_PLL_800_TYPE) && (UTILS_PLL800_INPUT_MIN <= (__VALUE__)) && ((__VALUE__) <= UTILS_PLL800_INPUT_MAX)))

#define IS_LL_UTILS_PLLVCO_OUTPUT(__VALUE__, __TYPE__) ( \
(((__TYPE__) == UTILS_PLL_1600_TYPE) && (UTILS_PLL1600_OUTPUT_MIN <= (__VALUE__)) && ((__VALUE__) <= UTILS_PLL1600_OUTPUT_MAX)) || \
(((__TYPE__) == UTILS_PLL_800_TYPE) && (UTILS_PLL800_OUTPUT_MIN <= (__VALUE__)) && ((__VALUE__) <= UTILS_PLL800_OUTPUT_MAX)))

#define IS_LL_UTILS_HSE_BYPASS(__STATE__) (((__STATE__) == LL_UTILS_HSEBYPASS_ON) || \
                                           ((__STATE__) == LL_UTILS_HSEBYPASSDIG_ON) || \
                                           ((__STATE__) == LL_UTILS_HSEBYPASS_OFF))

#define IS_LL_UTILS_HSE_FREQUENCY(__FREQUENCY__) (((__FREQUENCY__) >= UTILS_HSE_FREQUENCY_MIN) && ((__FREQUENCY__) <= UTILS_HSE_FREQUENCY_MAX))
/**
  * @}
  */
/* Private function prototypes -----------------------------------------------*/
/** @defgroup UTILS_LL_Private_Functions UTILS Private functions
  * @{
  */
static void UTILS_CheckPLLParameters(LL_UTILS_PLLTypeDef *UTILS_PLLStruct);
static uint32_t    UTILS_GetPLLOutputFrequency(uint32_t PLL_InputFrequency, LL_UTILS_PLLTypeDef *UTILS_PLLStruct);
static void UTILS_EnablePLLAndSwitchSystem(uint32_t SYSCLK_Frequency, LL_UTILS_ClkInitTypeDef *UTILS_ClkInitStruct);
static ErrorStatus UTILS_IsPLLsReady(void);
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup UTILS_LL_Exported_Functions
  * @{
  */

/** @addtogroup UTILS_LL_EF_DELAY
  * @{
  */
#if defined(CORE_CM4)
/**
  * @brief  This function configures the Cortex-M SysTick source to have 1ms time base.
  * @note   When a RTOS is used, it is recommended to avoid changing the Systick
  *         configuration by calling this function, for a delay use rather osDelay RTOS service.
  * @param  CPU_Frequency Core frequency in Hz
  * @note   CPU_Frequency can be calculated thanks to RCC helper macro or function
  *         @ref LL_RCC_GetSystemClocksFreq
  *         - Use  MCU_Frequency structure element returned by function above
  * @retval None
  */
void LL_Init1msTick(uint32_t CPU_Frequency)
{
  /* Use frequency provided in argument */
  LL_InitTick(CPU_Frequency, 1000U);
}

/**
  * @brief  This function provides accurate delay (in milliseconds) based
  *         on SysTick counter flag for Cortex-M processor
  * @note   When a RTOS is used, it is recommended to avoid using blocking delay
  *         and use rather osDelay service.
  * @note   To respect 1ms timebase, user should call @ref LL_Init1msTick function which
  *         will configure Systick to 1ms
  * @param  Delay specifies the delay time length, in milliseconds.
  * @retval None
  */
void LL_mDelay(uint32_t Delay)
{
  uint32_t count = Delay;
  __IO uint32_t  tmp = SysTick->CTRL;  /* Clear the COUNTFLAG first */
  /* Add this code to indicate that local variable is not used */
  ((void)tmp);

  /* Add a period to guaranty minimum wait */
  if (count < LL_MAX_DELAY)
  {
    count++;
  }

  while (count != 0U)
  {
    if ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0U)
    {
      count--;
    }
  }
}
#endif /* CORE_CM4 */
/**
  * @}
  */

/** @addtogroup UTILS_EF_SYSTEM
  *  @brief    System Configuration functions
  *
  @verbatim
 ===============================================================================
           ##### System Configuration functions #####
 ===============================================================================
    [..]
         MPUSS core (CA7), MCU core (CM4), ACLK, HCLK and APB buses clocks
         configuration

         (+) The maximum frequency of the SystemCoreClock MPUSS (CA7) is 650 MHz
         (+) The maximum frequency of the ACLK (plus HCLK5 and HCLK6) is 266 MHz.
         (+) The maximum frequency of the SystemCoreClock MCU (CM4) is 209 MHz
         (+) The maximum frequency of the PCLK1, PCLK2 and PCLK3 is 104.5 MHz.
         (+) The maximum frequency of the PCLK4 and PCLK5 is 133 MHz.
  @endverbatim
  * @{
  */
/**
  * @brief  This function sets directly SystemCoreClock CMSIS variable.
  * @note   Variable can be calculated also through SystemCoreClockUpdate function.
  * @param  CPU_Frequency Core frequency in Hz
  * @note   CPU_Frequency can be calculated thanks to RCC helper macro or function
  *         @ref LL_RCC_GetSystemClocksFreq
  *         - When code runs on CM4 core, use  MCU_Frequency structure element
  *           returned by function above
  *         - When code runs on CA7 core, use  MPUSS_Frequency structure element
  *           returned by function above
  * @retval None
  */
void LL_SetSystemCoreClock(uint32_t CPU_Frequency)
{
  /* System core clock  frequency */
  SystemCoreClock = CPU_Frequency;
}

/**
  * @brief  This function configures system clocks with HSE as clock source of the PLLs
  * @note   The application need to ensure that PLLs are disabled.
  * @note   Function is based on the following formula where x = {1,2,3,4} and Y = {P,Q,R}:
  *         - PLLxY output frequency = (((HSE frequency / PLLxM) * (PLLxN + (FRACV / 8192))) / PLLxY)
  *         - PLLxM: ensure that the VCO input frequency ranges from 8 to 16 MHz
  *           for PLL1600 type and from 4 to 16 MHz for PLL_800 type
  *           (PLLxVCO_input = HSE frequency / PLLxM)
  *         - PLLxN: ensure that the VCO output frequency is between 800 and 1600
  *           MHz for PLL_1600 type and between 400 and 800 MHz for PLL_800 type
  *           (PLLxVCO_output = PLLVCO_input * (PLLxN + (FRACV / 8192)))
  *         - PLLxP: ensure that max frequency is reached (PLLxVCO_output / PLLxP)
  *             - PLL1P output max frequency is 650000000 Hz
  *             - PLL2P output max frequency is 266000000 Hz
  *             - PLL2Q output max frequency is 533000000 Hz
  *             - PLL2R output max frequency is 533000000 Hz
  *             - PLL3P output max frequency is 209000000 Hz
  * @param  HSEFrequency Value between 8000000 Hz and 48000000 Hz
  * @param  HSEBypass This parameter can be one of the following values:
  *         @arg @ref LL_UTILS_HSEBYPASS_ON
  *         @arg @ref LL_UTILS_HSEBYPASSDIG_ON
  *         @arg @ref LL_UTILS_HSEBYPASS_OFF
  * @param  UTILS_PLLsInitStruct pointer to a @ref LL_UTILS_PLLsInitTypeDef structure that contains
  *                             the configuration information for the PLL.
  * @param  UTILS_ClkInitStruct pointer to a @ref LL_UTILS_ClkInitTypeDef structure that contains
  *                             the configuration information for the BUS prescalers.
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: Max frequency configuration done
  *          - ERROR: Max frequency configuration not done
  */
ErrorStatus LL_PLL_ConfigSystemClock_HSE(uint32_t HSEFrequency, uint32_t HSEBypass,
                                         LL_UTILS_PLLsInitTypeDef *UTILS_PLLsInitStruct,
                                         LL_UTILS_ClkInitTypeDef *UTILS_ClkInitStruct)
{
  ErrorStatus status = SUCCESS;
  uint32_t pllfreq;
  uint32_t vcoinput_freqPLL3, vcoinput_freqPLL4;
#ifdef  USE_FULL_ASSERT
  uint32_t vcoinput_freq, vcooutput_freq;
#endif

  /* Check the parameters */
  UTILS_CheckPLLParameters(&UTILS_PLLsInitStruct->PLL1);
  UTILS_CheckPLLParameters(&UTILS_PLLsInitStruct->PLL2);
  UTILS_CheckPLLParameters(&UTILS_PLLsInitStruct->PLL3);
  UTILS_CheckPLLParameters(&UTILS_PLLsInitStruct->PLL4);
  assert_param(IS_LL_UTILS_HSE_FREQUENCY(HSEFrequency));
  assert_param(IS_LL_UTILS_HSE_BYPASS(HSEBypass));

  vcoinput_freqPLL3 = HSEFrequency / UTILS_PLLsInitStruct->PLL3.PLLM;
  vcoinput_freqPLL4 = HSEFrequency / UTILS_PLLsInitStruct->PLL4.PLLM;

#ifdef  USE_FULL_ASSERT
  /* Check VCO Input frequency */
  vcoinput_freq = HSEFrequency / UTILS_PLLsInitStruct->PLL1.PLLM;
  assert_param(IS_LL_UTILS_PLLVCO_INPUT(vcoinput_freq, UTILS_PLL_1600_TYPE));

  vcoinput_freq = HSEFrequency / UTILS_PLLsInitStruct->PLL2.PLLM;
  assert_param(IS_LL_UTILS_PLLVCO_INPUT(vcoinput_freq, UTILS_PLL_1600_TYPE));

  assert_param(IS_LL_UTILS_PLLVCO_INPUT(vcoinput_freqPLL3, UTILS_PLL_800_TYPE));
  assert_param(IS_LL_UTILS_PLLVCO_INPUT(vcoinput_freqPLL4, UTILS_PLL_800_TYPE));

  /* Check VCO Output frequency */
  vcooutput_freq = (LL_RCC_CalcPLLClockFreq(
                      HSEFrequency,
                      UTILS_PLLsInitStruct->PLL1.PLLM,
                      UTILS_PLLsInitStruct->PLL1.PLLN,
                      UTILS_PLLsInitStruct->PLL1.PLLFRACV,
                      1U) * 2U);
  assert_param(IS_LL_UTILS_PLLVCO_OUTPUT(vcooutput_freq, UTILS_PLL_1600_TYPE));

  vcooutput_freq = (LL_RCC_CalcPLLClockFreq(
                      HSEFrequency,
                      UTILS_PLLsInitStruct->PLL2.PLLM,
                      UTILS_PLLsInitStruct->PLL2.PLLN,
                      UTILS_PLLsInitStruct->PLL2.PLLFRACV,
                      1U) * 2U);
  assert_param(IS_LL_UTILS_PLLVCO_OUTPUT(vcooutput_freq, UTILS_PLL_1600_TYPE));

  vcooutput_freq = LL_RCC_CalcPLLClockFreq(
                     HSEFrequency,
                     UTILS_PLLsInitStruct->PLL3.PLLM,
                     UTILS_PLLsInitStruct->PLL3.PLLN,
                     UTILS_PLLsInitStruct->PLL3.PLLFRACV,
                     1U);
  assert_param(IS_LL_UTILS_PLLVCO_OUTPUT(vcooutput_freq, UTILS_PLL_800_TYPE));

  vcooutput_freq = LL_RCC_CalcPLLClockFreq(
                     HSEFrequency,
                     UTILS_PLLsInitStruct->PLL4.PLLM,
                     UTILS_PLLsInitStruct->PLL4.PLLN,
                     UTILS_PLLsInitStruct->PLL4.PLLFRACV,
                     1U);
  assert_param(IS_LL_UTILS_PLLVCO_OUTPUT(vcooutput_freq, UTILS_PLL_800_TYPE));
#endif /* USE_FULL_ASSERT */

  /* Check if one of the PLL is enabled */
  if (UTILS_IsPLLsReady() == SUCCESS)
  {
    /* Check if need to enable HSE bypass feature or not */
    if (HSEBypass == LL_UTILS_HSEBYPASS_ON)
    {
      LL_RCC_HSE_EnableBypass();
    }
    else if (HSEBypass == LL_UTILS_HSEBYPASSDIG_ON)
    {
      LL_RCC_HSE_EnableDigBypass();
    }
    else
    {
      /* Clear HSE BYPASS and HSE BYPASS DIGITAL bits */
      LL_RCC_HSE_DisableDigBypass();
    }

    /* Enable HSE if not enabled */
    if (LL_RCC_HSE_IsReady() != 1U)
    {
      LL_RCC_HSE_Enable();
      while (LL_RCC_HSE_IsReady() != 1U)
      {
        /* Wait for HSE ready */
      }
    }

    /* Configure PLL1 */
    LL_RCC_PLL12_SetSource(LL_RCC_PLL12SOURCE_HSE);

    LL_RCC_PLL1_SetM(UTILS_PLLsInitStruct->PLL1.PLLM);
    LL_RCC_PLL1_SetN(UTILS_PLLsInitStruct->PLL1.PLLN);
    LL_RCC_PLL1_SetP(UTILS_PLLsInitStruct->PLL1.PLLP);

    LL_RCC_PLL1FRACV_Disable();
    LL_RCC_PLL1_SetFRACV(UTILS_PLLsInitStruct->PLL1.PLLFRACV);
    LL_RCC_PLL1FRACV_Enable();

    LL_RCC_PLL1CSG_Disable();

    /* Configure PLL2 */
    LL_RCC_PLL2_SetM(UTILS_PLLsInitStruct->PLL2.PLLM);
    LL_RCC_PLL2_SetN(UTILS_PLLsInitStruct->PLL2.PLLN);
    LL_RCC_PLL2_SetP(UTILS_PLLsInitStruct->PLL2.PLLP);
    LL_RCC_PLL2_SetQ(UTILS_PLLsInitStruct->PLL2.PLLQ);
    LL_RCC_PLL2_SetR(UTILS_PLLsInitStruct->PLL2.PLLR);

    LL_RCC_PLL2FRACV_Disable();
    LL_RCC_PLL2_SetFRACV(UTILS_PLLsInitStruct->PLL2.PLLFRACV);
    LL_RCC_PLL2FRACV_Enable();

    LL_RCC_PLL2CSG_Disable();

    /* Configure PLL3 */
    LL_RCC_PLL3_SetSource(LL_RCC_PLL3SOURCE_HSE);

    LL_RCC_PLL3_SetM(UTILS_PLLsInitStruct->PLL3.PLLM);
    LL_RCC_PLL3_SetN(UTILS_PLLsInitStruct->PLL3.PLLN);
    LL_RCC_PLL3_SetP(UTILS_PLLsInitStruct->PLL3.PLLP);
    LL_RCC_PLL3_SetQ(UTILS_PLLsInitStruct->PLL3.PLLQ);
    LL_RCC_PLL3_SetR(UTILS_PLLsInitStruct->PLL3.PLLR);

    LL_RCC_PLL3FRACV_Disable();
    LL_RCC_PLL3_SetFRACV(UTILS_PLLsInitStruct->PLL3.PLLFRACV);
    LL_RCC_PLL3FRACV_Enable();

    LL_RCC_PLL3CSG_Disable();

    if (vcoinput_freqPLL3 >= UTILS_PLL800_INPUT_MED)
    {
      LL_RCC_PLL3_SetIFRGE(LL_RCC_PLL3IFRANGE_1);
    }
    else
    {
      LL_RCC_PLL3_SetIFRGE(LL_RCC_PLL3IFRANGE_0);
    }

    /* Configure PLL4 */
    LL_RCC_PLL4_SetSource(LL_RCC_PLL4SOURCE_HSE);

    LL_RCC_PLL4_SetM(UTILS_PLLsInitStruct->PLL4.PLLM);
    LL_RCC_PLL4_SetN(UTILS_PLLsInitStruct->PLL4.PLLN);
    LL_RCC_PLL4_SetP(UTILS_PLLsInitStruct->PLL4.PLLP);
    LL_RCC_PLL4_SetQ(UTILS_PLLsInitStruct->PLL4.PLLQ);
    LL_RCC_PLL4_SetR(UTILS_PLLsInitStruct->PLL4.PLLR);

    LL_RCC_PLL4FRACV_Disable();
    LL_RCC_PLL4_SetFRACV(UTILS_PLLsInitStruct->PLL4.PLLFRACV);
    LL_RCC_PLL4FRACV_Enable();

    LL_RCC_PLL4CSG_Disable();

    if (vcoinput_freqPLL4 >= UTILS_PLL800_INPUT_MED)
    {
      LL_RCC_PLL4_SetIFRGE(LL_RCC_PLL4IFRANGE_1);
    }
    else
    {
      LL_RCC_PLL4_SetIFRGE(LL_RCC_PLL4IFRANGE_0);
    }

    /* Calculate the new PLL output frequency */
#if defined(CORE_CM4)
    pllfreq = UTILS_GetPLLOutputFrequency(HSEFrequency, &UTILS_PLLsInitStruct->PLL3);
#else
#if defined(CORE_CA7)
    pllfreq = UTILS_GetPLLOutputFrequency(HSEFrequency, &UTILS_PLLsInitStruct->PLL1);
#endif /* CORE_CA7 */
#endif /* CORE_CM4 */

    /* Enable PLL and switch system clock to PLL */
    UTILS_EnablePLLAndSwitchSystem(pllfreq, UTILS_ClkInitStruct);
  }
  else
  {
    /* Current PLL configuration cannot be modified */
    status = ERROR;
  }

  return status;
}

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup UTILS_LL_Private_Functions
  * @{
  */

/**
  * @brief  Function to check PLL parameters
  * @param  UTILS_PLLStruct pointer to a @ref LL_UTILS_PLLTypeDef
  *         structure that contains the configuration information for the PLL.
  * @retval None
  */
void UTILS_CheckPLLParameters(LL_UTILS_PLLTypeDef *UTILS_PLLStruct)
{
  /* Check the parameters */
  assert_param(IS_LL_UTILS_PLLM_VALUE(UTILS_PLLStruct->PLLM));
  assert_param(IS_LL_UTILS_PLLN_VALUE(UTILS_PLLStruct->PLLN));
  assert_param(IS_LL_UTILS_PLLP_VALUE(UTILS_PLLStruct->PLLP));
  assert_param(IS_LL_UTILS_FRACV_VALUE(UTILS_PLLStruct->PLLFRACV));
}

/**
  * @brief  Function to check that PLL can be modified
  * @param  PLL_InputFrequency  PLL input frequency (in Hz)
  * @param  UTILS_PLLStruct pointer to a @ref LL_UTILS_PLLTypeDef
  *         structure that contains the configuration information for the PLL.
  * @retval PLL output frequency (in Hz)
  */
static uint32_t UTILS_GetPLLOutputFrequency(uint32_t PLL_InputFrequency, LL_UTILS_PLLTypeDef *UTILS_PLLStruct)
{
  uint32_t pllfreq;

  /* Check the parameters */
  UTILS_CheckPLLParameters(UTILS_PLLStruct);

  pllfreq = LL_RCC_CalcPLLClockFreq(PLL_InputFrequency,
                                    UTILS_PLLStruct->PLLM,
                                    UTILS_PLLStruct->PLLN,
                                    UTILS_PLLStruct->PLLFRACV,
                                    UTILS_PLLStruct->PLLP);

  return pllfreq;
}

/**
  * @brief  Check that all PLLs are ready therefore configuration can be done
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: All PLLs are ready so configuration can be done
  *          - ERROR: One PLL at least is busy
  */
static ErrorStatus UTILS_IsPLLsReady(void)
{
  ErrorStatus status = SUCCESS;

  /* Check if one of the PLL1 is busy */
  if (LL_RCC_PLL1_IsReady() != 0U)
  {
    /* PLL1 configuration cannot be done */
    status = ERROR;
  }

  /* Check if one of the PLL2 is busy */
  if (LL_RCC_PLL2_IsReady() != 0U)
  {
    /* PLL2 configuration cannot be done */
    status = ERROR;
  }

  /* Check if one of the PLL3 is busy */
  if (LL_RCC_PLL3_IsReady() != 0U)
  {
    /* PLL3 configuration cannot be done */
    status = ERROR;
  }

  /* Check if one of the PLL4 is busy */
  if (LL_RCC_PLL4_IsReady() != 0U)
  {
    /* PLL4 configuration cannot be done */
    status = ERROR;
  }

  return status;
}

/**
  * @brief  Function to enable PLL and switch system core clock to PLL
  * @param  SYSCLK_CoreFrequency SYSCLK frequency
  * @param  UTILS_ClkInitStruct pointer to a @ref LL_UTILS_ClkInitTypeDef
  *         structure that contains the configuration information for the BUS
  *         prescalers.
  * @retval None
  */
static void UTILS_EnablePLLAndSwitchSystem(
  uint32_t SYSCLK_CoreFrequency,
  LL_UTILS_ClkInitTypeDef *UTILS_ClkInitStruct)
{
  assert_param(IS_LL_UTILS_APB1_DIV(UTILS_ClkInitStruct->APB1CLKDivider));
  assert_param(IS_LL_UTILS_APB2_DIV(UTILS_ClkInitStruct->APB2CLKDivider));
  assert_param(IS_LL_UTILS_APB3_DIV(UTILS_ClkInitStruct->APB3CLKDivider));
  assert_param(IS_LL_UTILS_APB4_DIV(UTILS_ClkInitStruct->APB4CLKDivider));
  assert_param(IS_LL_UTILS_APB5_DIV(UTILS_ClkInitStruct->APB5CLKDivider));


  /* Enable PLL1 */
  LL_RCC_PLL1_Enable();
  while (LL_RCC_PLL1_IsReady() != 1U)
  {
    /* Wait for PLL1 ready */
  }
  LL_RCC_PLL1P_Enable();

  /* Enable PLL2 */
  LL_RCC_PLL2_Enable();
  while (LL_RCC_PLL2_IsReady() != 1U)
  {
    /* Wait for PLL2 ready */
  }
  LL_RCC_PLL2P_Enable();
  LL_RCC_PLL2Q_Enable();
  LL_RCC_PLL2R_Enable();

  /* Enable PLL3 */
  LL_RCC_PLL3_Enable();
  while (LL_RCC_PLL3_IsReady() != 1U)
  {
    /* Wait for PLL3 ready */
  }
  LL_RCC_PLL3P_Enable();
  LL_RCC_PLL3Q_Enable();
  LL_RCC_PLL3R_Enable();

  /* Enable PLL4 */
  LL_RCC_PLL4_Enable();
  while (LL_RCC_PLL4_IsReady() != 1U)
  {
    /* Wait for PLL4 ready */
  }
  LL_RCC_PLL4P_Enable();
  LL_RCC_PLL4Q_Enable();
  LL_RCC_PLL4R_Enable();

  /* Set All APBxPrescaler to the Highest Divider */
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_16);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_16);
  LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_16);
  LL_RCC_SetAPB4Prescaler(LL_RCC_APB4_DIV_16);
  LL_RCC_SetAPB5Prescaler(LL_RCC_APB5_DIV_16);

  /* Set MPU Clock Selection */
  LL_RCC_SetMPUClkSource(LL_RCC_MPU_CLKSOURCE_PLL1);
  while (LL_RCC_GetMPUClkSource() != LL_RCC_MPU_CLKSOURCE_PLL1)
  {}

  /* Set AXI Sub-System Clock Selection */
  LL_RCC_SetAXISSClkSource(LL_RCC_AXISS_CLKSOURCE_PLL2);
  while (LL_RCC_GetAXISSClkSource() != LL_RCC_AXISS_CLKSOURCE_PLL2)
  {}

  /* Set MCU Sub-System Clock Selection */
  LL_RCC_SetMCUSSClkSource(LL_RCC_MCUSS_CLKSOURCE_PLL3);
  while (LL_RCC_GetMCUSSClkSource() != LL_RCC_MCUSS_CLKSOURCE_PLL3)
  {}

  /* Set APBn prescaler*/
  LL_RCC_SetAPB1Prescaler(UTILS_ClkInitStruct->APB1CLKDivider);
  LL_RCC_SetAPB2Prescaler(UTILS_ClkInitStruct->APB2CLKDivider);
  LL_RCC_SetAPB3Prescaler(UTILS_ClkInitStruct->APB3CLKDivider);
  LL_RCC_SetAPB4Prescaler(UTILS_ClkInitStruct->APB4CLKDivider);
  LL_RCC_SetAPB5Prescaler(UTILS_ClkInitStruct->APB5CLKDivider);

  /* Update SystemCoreClock variable */
  LL_SetSystemCoreClock(SYSCLK_CoreFrequency);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
