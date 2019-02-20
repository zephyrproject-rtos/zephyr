/**
  ******************************************************************************
  * @file    system_stm32mp1xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex Device Peripheral Access Layer System Source File.
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32mp1xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock frequency, it can
  *                                  be used by the user application to setup
  *                                  the SysTick timer or configure other
  *                                  parameters.
  *                                     
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32mp1xx_system
  * @{
  */

/** @addtogroup STM32MP1xx_System_Private_Includes
  * @{
  */

#include "stm32mp1xx_hal.h"

/**
  * @}
  */

/** @addtogroup STM32MP1xx_System_Private_TypesDefinitions
  * @{
  */


/**
  * @}
  */

/** @addtogroup STM32MP1xx_System_Private_Defines
  * @{
  */


/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use external SRAM mounted
     on EVAL board as data memory  */
/* #define DATA_IN_ExtSRAM */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field. 
                                   This value must be a multiple of 0x200. */
/******************************************************************************/

/**
  * @}
  */

/** @addtogroup STM32MP1xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32MP1xx_System_Private_Variables
  * @{
  */
  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) each time HAL_RCC_ClockConfig() is called to configure the system clock
         frequency
         Note: If you use this function to configure the system clock;
               then there is no need to call the first functions listed above,
               since SystemCoreClock variable is updated automatically.
  */
  uint32_t SystemCoreClock = HSI_VALUE;
/**
  * @}
  */

/** @addtogroup STM32MP1xx_System_Private_FunctionPrototypes
  * @{
  */

#if defined (DATA_IN_ExtSRAM) 
  static void SystemInit_ExtMemCtl(void); 
#endif /* DATA_IN_ExtSRAM */

/**
  * @}
  */

/** @addtogroup STM32MP1xx_System_Private_Functions
  * @{
  */

  /**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting, vector table location and External memory 
  *         configuration.
  * @param  None
  * @retval None
  */
void SystemInit (void)
{
  /* FPU settings ------------------------------------------------------------*/
#if defined (CORE_CM4)
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
   SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif

  /* Configure the Vector Table location add offset address ------------------*/
#if defined (VECT_TAB_SRAM)
  SCB->VTOR = MCU_AHB_SRAM | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#endif
  /* Disable all interrupts and events */
  CLEAR_REG(EXTI_C2->IMR1);
  CLEAR_REG(EXTI_C2->IMR2);
  CLEAR_REG(EXTI_C2->IMR3);
  CLEAR_REG(EXTI_C2->EMR1);
  CLEAR_REG(EXTI_C2->EMR2);
  CLEAR_REG(EXTI_C2->EMR3);
#else
#error Please #define CORE_CM4
#endif	                         
  SystemCoreClockUpdate();
}

/**
   * @brief Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock frequency (Hz),
  *         it can be used by the user application to setup the SysTick timer or
  *         configure other parameters.
  *
  * @note   Each time the core clock changes, this function must be called to
  *         update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the
  *             HSI_VALUE(*)
  *
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the
  *             HSE_VALUE(**)
  *
  *           - If SYSCLK source is CSI, SystemCoreClock will contain the
  *             CSI_VALUE(***)
  *
  *           - If SYSCLK source is PLL3_P, SystemCoreClock will contain the
  *             HSI_VALUE(*) or the HSE_VALUE(*) or the CSI_VALUE(***)
  *             multiplied/divided by the PLL3 factors.
  *
  *         (*) HSI_VALUE is a constant defined in stm32mp1xx_hal_conf.h file
  *             (default value 64 MHz) but the real value may vary depending
  *             on the variations in voltage and temperature.
  *
  *         (**) HSE_VALUE is a constant defined in stm32mp1xx_hal_conf.h file
  *              (default value 24 MHz), user has to ensure that HSE_VALUE is
  *              same as the real frequency of the crystal used. Otherwise, this
  *              function may have wrong result.
  *
  *         (***) CSI_VALUE is a constant defined in stm32mp1xx_hal_conf.h file
  *              (default value 4 MHz)but the real value may vary depending
  *              on the variations in voltage and temperature.
  *
  *         - The result of this function could be not correct when using
  *           fractional value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate (void)
{
  uint32_t   pllsource, pll3m, pll3fracen;
  float fracn1, pll3vco;

  switch (RCC->MSSCKSELR & RCC_MSSCKSELR_MCUSSRC)
  {
  case 0x00:  /* HSI used as system clock source */
    SystemCoreClock = (HSI_VALUE >> (RCC->HSICFGR & RCC_HSICFGR_HSIDIV));
    break;

  case 0x01:  /* HSE used as system clock source */
    SystemCoreClock = HSE_VALUE;
    break;

  case 0x02:  /* CSI used as system clock source */
    SystemCoreClock = CSI_VALUE;
    break;

  case 0x03:  /* PLL3_P used as system clock source */
    pllsource = (RCC->RCK3SELR & RCC_RCK3SELR_PLL3SRC);
    pll3m = ((RCC->PLL3CFGR1 & RCC_PLL3CFGR1_DIVM3) >> RCC_PLL3CFGR1_DIVM3_Pos) + 1U;
    pll3fracen = (RCC->PLL3FRACR & RCC_PLL3FRACR_FRACLE) >> 16U;
    fracn1 = (pll3fracen * ((RCC->PLL3FRACR & RCC_PLL3FRACR_FRACV) >> 3U));
    pll3vco = (((RCC->PLL3CFGR1 & RCC_PLL3CFGR1_DIVN) + 1U) + (fracn1/0x1FFFU));

    if (pll3m != 0U)
    {
      switch (pllsource)
      {
        case 0x00:  /* HSI used as PLL clock source */
          pll3vco *= ((HSI_VALUE >> (RCC->HSICFGR & RCC_HSICFGR_HSIDIV)) / pll3m);
          break;

        case 0x01:  /* HSE used as PLL clock source */
          pll3vco *= (HSE_VALUE / pll3m);
          break;

        case 0x02:  /* CSI used as PLL clock source */
          pll3vco *= (CSI_VALUE / pll3m);
          break;

        case 0x03:  /* No clock source for PLL */
          pll3vco = 0U;
          break;
       }
      SystemCoreClock = (uint32_t)(pll3vco/((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVP) + 1U));
    }
    else
    {
      SystemCoreClock = 0U;
    }
    break;
  }

  /* Compute mcu_ck */
  SystemCoreClock = SystemCoreClock >> (RCC->MCUDIVR & RCC_MCUDIVR_MCUDIV);
}


#ifdef DATA_IN_ExtSRAM
/**
  * @brief  Setup the external memory controller.
  *         Called in startup_stm32L4xx.s before jump to main.
  *         This function configures the external SRAM mounted on Eval boards
  *         This SRAM will be used as program data memory (including heap and stack).
  * @param  None
  * @retval None
  */
void SystemInit_ExtMemCtl(void)
{
  
}
#endif /* DATA_IN_ExtSRAM */
  
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
