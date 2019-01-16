/**
  ******************************************************************************
  * @file    system_stm32g0xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M0+ Device Peripheral Access Layer System Source File
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32g0xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *   After each device reset the HSI (8 MHz then 16 MHz) is used as system clock source.
  *   Then SystemInit() function is called, in "startup_stm32g0xx.s" file, to
  *   configure the system clock before to branch to main program.
  *
  *   This file configures the system clock as follows:
  *=============================================================================
  *-----------------------------------------------------------------------------
  *        System Clock source                    | HSI
  *-----------------------------------------------------------------------------
  *        SYSCLK(Hz)                             | 16000000
  *-----------------------------------------------------------------------------
  *        HCLK(Hz)                               | 16000000
  *-----------------------------------------------------------------------------
  *        AHB Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        APB Prescaler                          | 1
  *-----------------------------------------------------------------------------
  *        HSI Division factor                    | 1
  *-----------------------------------------------------------------------------
  *        PLL_M                                  | 1
  *-----------------------------------------------------------------------------
  *        PLL_N                                  | 8
  *-----------------------------------------------------------------------------
  *        PLL_P                                  | 7
  *-----------------------------------------------------------------------------
  *        PLL_Q                                  | 2
  *-----------------------------------------------------------------------------
  *        PLL_R                                  | 2
  *-----------------------------------------------------------------------------
  *        Require 48MHz for RNG                  | Disabled
  *-----------------------------------------------------------------------------
  *=============================================================================
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

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32g0xx_system
  * @{
  */

/** @addtogroup STM32G0xx_System_Private_Includes
  * @{
  */

#include "stm32g0xx.h"

#if !defined  (HSE_VALUE)
#define HSE_VALUE    8000000U    /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE  16000000U   /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

#if !defined  (LSI_VALUE)
 #define LSI_VALUE   32000U      /*!< Value of LSI in Hz*/
#endif /* LSI_VALUE */

#if !defined  (LSE_VALUE)
  #define LSE_VALUE  32768U      /*!< Value of LSE in Hz*/
#endif /* LSE_VALUE */

/**
  * @}
  */

/** @addtogroup STM32G0xx_System_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32G0xx_System_Private_Defines
  * @{
  */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x0U /*!< Vector Table base offset field.
                                   This value must be a multiple of 0x100. */
/******************************************************************************/
/**
  * @}
  */

/** @addtogroup STM32G0xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32G0xx_System_Private_Variables
  * @{
  */
  /* The SystemCoreClock variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
  uint32_t SystemCoreClock = 16000000U;

  const uint32_t AHBPrescTable[16] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U};
  const uint32_t APBPrescTable[8] =  {0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U};

/**
  * @}
  */

/** @addtogroup STM32G0xx_System_Private_FunctionPrototypes
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32G0xx_System_Private_Functions
  * @{
  */

/**
  * @brief  Setup the microcontroller system.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**) / HSI division factor
  *
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
  *
  *           - If SYSCLK source is LSI, SystemCoreClock will contain the LSI_VALUE
  *
  *           - If SYSCLK source is LSE, SystemCoreClock will contain the LSE_VALUE
  *
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
  *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
  *
  *         (**) HSI_VALUE is a constant defined in stm32g0xx_hal_conf.h file (default value
  *              16 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  *
  *         (***) HSE_VALUE is a constant defined in stm32g0xx_hal_conf.h file (default value
  *              8 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t tmp;
  uint32_t pllvco;
  uint32_t pllr;
  uint32_t pllsource;
  uint32_t pllm;
  uint32_t hsidiv;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (RCC->CFGR & RCC_CFGR_SWS)
  {
    case RCC_CFGR_SWS_HSE:  /* HSE used as system clock */
      SystemCoreClock = HSE_VALUE;
      break;

    case RCC_CFGR_SWS_LSI:  /* LSI used as system clock */
      SystemCoreClock = LSI_VALUE;
      break;

    case RCC_CFGR_SWS_LSE:  /* LSE used as system clock */
      SystemCoreClock = LSE_VALUE;
      break;

    case RCC_CFGR_SWS_PLL:  /* PLL used as system clock */
      {
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC);
      pllm = ((RCC->PLLCFGR & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1U;
      switch (pllsource)
      {
        case 0x03:  /* HSE used as PLL clock source */
          pllvco = (HSE_VALUE / pllm);
          break;

        case 0x02:  /* HSI used as PLL clock source */
        default:
          pllvco = (HSI_VALUE / pllm);
          break;
      }
      pllvco = pllvco * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos);
      pllr = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1U);
      SystemCoreClock = pllvco/pllr;
      break;
      }
    case RCC_CFGR_SWS_HSI:  /* HSI used as system clock */
    default:                /* HSI used as system clock */
      hsidiv = (1UL << ((READ_BIT(RCC->CR, RCC_CR_HSIDIV))>> RCC_CR_HSIDIV_Pos));
      SystemCoreClock = (HSI_VALUE/hsidiv);
      break;
  }
  /* Compute HCLK clock frequency --------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos)];
  /* HCLK clock frequency */
  SystemCoreClock >>= tmp;
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
