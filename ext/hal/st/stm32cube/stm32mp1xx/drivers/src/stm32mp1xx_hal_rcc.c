/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_rcc.c
  * @author  MCD Application Team
  * @brief   RCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Reset and Clock Control (RCC) peripheral:
  *           + Initialization and de-initialization functions
  *           + Peripheral Control functions
  *
  @verbatim
  ==============================================================================
                      ##### RCC specific features #####
  ==============================================================================
    [..]
      After reset the device is running from Internal High Speed oscillator
      (HSI 64MHz) and all peripherals are off except
      internal SRAM1, SRAM2, SRAM3 and PWR
      (+) There is no pre-scaler on High speed (AHB) and Low speed (APB) buses;
          all peripherals mapped on these buses are running at HSI speed.
      (+) The clock for all peripherals is switched off, except the SRAM

    [..]
      Once the device started from reset, the user application has to:
      (+) Configure the clock source to be used to drive the MPU, AXI and MCU
          (if the application needs higher frequency/performance)
      (+) Configure the AHB and APB buses pre-scalers
      (+) Enable the clock for the peripheral(s) to be used
      (+) Configure the clock kernel source(s) for peripherals which clocks are not
          derived from Bus clocks

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

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup RCC RCC
  * @brief RCC HAL module driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define LSE_MASK (RCC_BDCR_LSEON | RCC_BDCR_LSEBYP | RCC_BDCR_DIGBYP)
#define HSE_MASK (RCC_OCENSETR_HSEON | RCC_OCENSETR_HSEBYP | RCC_OCENSETR_DIGBYP)
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCC_Private_Function_Prototypes RCC Private Functions Prototypes
  * @{
  */
HAL_StatusTypeDef RCC_MPUConfig(RCC_MPUInitTypeDef *RCC_MPUInitStruct);
HAL_StatusTypeDef RCC_AXISSConfig(RCC_AXISSInitTypeDef *RCC_AXISSInitStruct);
HAL_StatusTypeDef RCC_MCUConfig(RCC_MCUInitTypeDef *MCUInitStruct);
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

/** @defgroup RCC_Exported_Functions RCC Exported Functions
  * @{
  */

/** @defgroup RCC_Exported_Functions_Group1 Initialization and de-initialization functions
 *  @brief    Initialization and Configuration functions
 *
@verbatim
 ===============================================================================
           ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]
      This section provides functions allowing to configure the internal/external oscillators
      (HSE, HSI, LSE,CSI, LSI) PLL, HSECSS, LSECSS and MCO and the System busses clocks (
       MPUSS,AXISS, MCUSS, AHB[1:4], AHB[5:6], APB1, APB2, APB3, APB4 and APB5).

    [..] Internal/external clock and PLL configuration
         (#) HSI (high-speed internal), 64 MHz factory-trimmed RC used directly or through
             the PLL as System clock source.

         (#) CSI is a low-power RC oscillator which can be used directly as system clock, peripheral
             clock, or PLL input.But even with frequency calibration, is less accurate than an
             external crystal oscillator or ceramic resonator.

         (#) LSI (low-speed internal), 32 KHz low consumption RC used as IWDG and/or RTC
             clock source.

         (#) HSE (high-speed external), 4 to 48 MHz crystal oscillator used directly or
             through the PLL as System clock source. Can be used also as RTC clock source.

         (#) LSE (low-speed external), 32 KHz oscillator used as RTC clock source.

         (#) PLL , The RCC features four independent PLLs (clocked by HSI , HSE or CSI),
             featuring three different output clocks and able  to work either in integer, Fractional or
             Spread Sprectum mode.
           (++) PLL1, which is used to provide clock to the A7 sub-system
           (++) PLL2, which is used to provide clock to the AXI sub-system, DDR and GPU
           (++) PLL3, which is used to provide clock to the MCU sub-system and to generate the kernel
                clock for peripherals.
           (++) PLL4, which is used to generate the kernel clock for peripherals.


         (#) HSECSS (HSE Clock security system), once enabled and if a HSE clock failure occurs
             HSECSS will generate a system reset, and protect sensitive data of the BKPSRAM

         (#) LSECSS (LSE Clock security system), once enabled and if a LSE clock failure occurs
             (++) The clock provided to the RTC is disabled immediately by the hardware
             (++) A failure event is generated (rcc_lsecss_fail). This event is connected to the TAMP
                  block, allowing to wake up from Standby, and allowing the protection of backup
                  registers and BKPSRAM
             (++) An interrupt flag (LSECSSF) is activated in order to generate an interrupt to the
                  MCU or MPU if needed.

         (#) MCO1 (micro controller clock output), used to output HSI, HSE, CSI,LSI or LSE clock
             (through a configurable pre-scaler).

         (#) MCO2 (micro controller clock output), used to output MPUSS, AXISS, MCUSS, PLL4(PLL4_P),
             HSE, or HSI clock (through a configurable pre-scaler).


    [..] Sub-system clock configuration

         (#) The MPUDIV divider located into RCC MPU Clock Divider Register (RCC_MPCKDIVR) can be used
             to adjust dynamically the clock for the MPU.

         (#) The AXIDIV divider located into RCC AXI Clock Divider Register (RCC_AXIDIVR) can be used
             to adjust dynamically the clock for the AXI matrix. The value of this divider also impacts
             the clock frequency of AHB5, AHB6, APB4 and APB5.

         (#) The APB4DIV divider located into RCC APB4 Clock Divider Register (RCC_APB4DIVR) can be used
             to adjust dynamically the clock for the APB4 bridge.

         (#) In the same way, the APB5DIV divider located into RCC APB5 Clock Divider Register
             (RCC_APB5DIVR), can be used to adjust dynamically the clock for the APB5 bridge.

         (#) In the MCU clock tree, the MCUDIV divider located into RCC MCU Clock Divider Register
             (RCC_MCUDIVR), can be used to adjust dynamically the clock for the MCU. The value of
             this divider also impacts the clock frequency of AHB bridges, APB1, APB2 and APB3.

         (#) The APB1DIV divider located into RCC APB1 Clock Divider Register (RCC_APB1DIVR), can be used
             to adjust dynamically the clock for the APB1 bridge. Similar implementation is available for
             APB2 and APB3.

@endverbatim
  * @{
  */

/**
  * @brief  Resets the RCC clock configuration to the default reset state.
  * @note   The default reset state of the clock configuration is given below:
  *            - HSI ON and used as system clock source
  *            - HSE, PLL1, PLL2 and PLL3 and PLL4 OFF
  *            - AHB, APB Bus pre-scaler set to 1.
  *            - MCO1 and MCO2 OFF
  *            - All interrupts disabled (except Wake-up from CSTOP Interrupt Enable)
  * @note   This function doesn't modify the configuration of the
  *            - Peripheral clocks
  *            - LSI, LSE and RTC clock
  *            - HSECSS and LSECSS
  * @retval None
  */
HAL_StatusTypeDef HAL_RCC_DeInit(void)
{
  uint32_t tickstart;

  /* Set HSION bit to enable HSI oscillator */
  SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSION);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till HSI is ready */
  while ((RCC->OCRDYR & RCC_OCRDYR_HSIRDY) == 0U)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Reset MCO1 Configuration Register */
  CLEAR_REG(RCC->MCO1CFGR);

  /* Reset MCO2 Configuration Register */
  CLEAR_REG(RCC->MCO2CFGR);

  /* Reset MPU Clock Selection Register */
  MODIFY_REG(RCC->MPCKSELR, (RCC_MPCKSELR_MPUSRC), RCC_MPCKSELR_MPUSRC_0);

  /* Reset AXI Sub-System Clock Selection Register */
  MODIFY_REG(RCC->ASSCKSELR, (RCC_ASSCKSELR_AXISSRC), RCC_ASSCKSELR_AXISSRC_0);

  /* Reset MCU Sub-System Clock Selection Register */
  MODIFY_REG(RCC->MSSCKSELR, (RCC_MSSCKSELR_MCUSSRC), RCC_MSSCKSELR_MCUSSRC_0);

  /* Reset RCC MPU Clock Divider Register */
  MODIFY_REG(RCC->MPCKDIVR, (RCC_MPCKDIVR_MPUDIV), RCC_MPCKDIVR_MPUDIV_1);

  /* Reset RCC AXI Clock Divider Register */
  MODIFY_REG(RCC->AXIDIVR, (RCC_AXIDIVR_AXIDIV), RCC_AXIDIVR_AXIDIV_0);

  /* Reset RCC APB4 Clock Divider Register */
  MODIFY_REG(RCC->APB4DIVR, (RCC_APB4DIVR_APB4DIV), RCC_APB4DIVR_APB4DIV_0);

  /* Reset RCC APB5 Clock Divider Register */
  MODIFY_REG(RCC->APB5DIVR, (RCC_APB5DIVR_APB5DIV), RCC_APB5DIVR_APB5DIV_0);

  /* Reset RCC MCU Clock Divider Register */
  MODIFY_REG(RCC->MCUDIVR, (RCC_MCUDIVR_MCUDIV), RCC_MCUDIVR_MCUDIV_0);

  /* Reset RCC APB1 Clock Divider Register */
  MODIFY_REG(RCC->APB1DIVR, (RCC_APB1DIVR_APB1DIV), RCC_APB1DIVR_APB1DIV_0);

  /* Reset RCC APB2 Clock Divider Register */
  MODIFY_REG(RCC->APB2DIVR, (RCC_APB2DIVR_APB2DIV), RCC_APB2DIVR_APB2DIV_0);

  /* Reset RCC APB3 Clock Divider Register */
  MODIFY_REG(RCC->APB3DIVR, (RCC_APB3DIVR_APB3DIV), RCC_APB3DIVR_APB3DIV_0);

  /* Disable PLL1 outputs */
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_DIVPEN | RCC_PLL1CR_DIVQEN |
            RCC_PLL1CR_DIVREN);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Disable PLL1 */
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_PLLON);

  /* Wait till PLL is disabled */
  while ((RCC->PLL1CR & RCC_PLL1CR_PLL1RDY) != 0U)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Clear remaining SSCG_CTRL bit */
  CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_SSCG_CTRL);

  /* Disable PLL2 outputs */
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_DIVPEN | RCC_PLL2CR_DIVQEN |
            RCC_PLL2CR_DIVREN);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Disable PLL2 */
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_PLLON);

  /* Wait till PLL is disabled */
  while ((RCC->PLL2CR & RCC_PLL2CR_PLL2RDY) != 0U)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Clear remaining SSCG_CTRL bit */
  CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_SSCG_CTRL);

  /* Disable PLL3 outputs */
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_DIVPEN | RCC_PLL3CR_DIVQEN |
            RCC_PLL3CR_DIVREN);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Disable PLL3 */
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_PLLON);

  /* Wait till PLL is disabled */
  while ((RCC->PLL3CR & RCC_PLL3CR_PLL3RDY) != 0U)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Clear remaining SSCG_CTRL bit */
  CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_SSCG_CTRL);

  /* Disable PLL4 outputs */
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_DIVPEN | RCC_PLL4CR_DIVQEN |
            RCC_PLL4CR_DIVREN);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Disable PLL4 */
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_PLLON);

  /* Wait till PLL is disabled */
  while ((RCC->PLL4CR & RCC_PLL4CR_PLL4RDY) != 0U)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Clear remaining SSCG_CTRL bit */
  CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL);

  /* Reset PLL 1 and 2 Ref. Clock Selection Register */
  MODIFY_REG(RCC->RCK12SELR, (RCC_RCK12SELR_PLL12SRC), RCC_RCK12SELR_PLL12SRC_0);

  /* Reset RCC PLL 3 Ref. Clock Selection Register */
  MODIFY_REG(RCC->RCK3SELR, (RCC_RCK3SELR_PLL3SRC), RCC_RCK3SELR_PLL3SRC_0);

  /* Reset PLL4 Ref. Clock Selection Register */
  MODIFY_REG(RCC->RCK4SELR, (RCC_RCK4SELR_PLL4SRC), RCC_RCK4SELR_PLL4SRC_0);

  /* Reset RCC PLL1 Configuration Register 1 */
  WRITE_REG(RCC->PLL1CFGR1, 0x00010031U);

  /* Reset RCC PLL1 Configuration Register 2 */
  WRITE_REG(RCC->PLL1CFGR2, 0x00010100U);

  /* Reset RCC PLL1 Fractional Register */
  CLEAR_REG(RCC->PLL1FRACR);

  /* Reset RCC PLL1 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL1CSGR);

  /* Reset RCC PLL2 Configuration Register 1 */
  WRITE_REG(RCC->PLL2CFGR1, 0x00010063U);

  /* Reset RCC PLL2 Configuration Register 2 */
  WRITE_REG(RCC->PLL2CFGR2, 0x00010101U);

  /* Reset RCC PLL2 Fractional Register */
  CLEAR_REG(RCC->PLL2FRACR);

  /* Reset RCC PLL2 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL2CSGR);

  /* Reset RCC PLL3 Configuration Register 1 */
  WRITE_REG(RCC->PLL3CFGR1, 0x00010031U);

  /* Reset RCC PLL3 Configuration Register 2 */
  WRITE_REG(RCC->PLL3CFGR2, 0x00010101U);

  /* Reset RCC PLL3 Fractional Register */
  CLEAR_REG(RCC->PLL3FRACR);

  /* Reset RCC PLL3 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL3CSGR);

  /* Reset RCC PLL4 Configuration Register 1 */
  WRITE_REG(RCC->PLL4CFGR1, 0x00010031U);

  /* Reset RCC PLL4 Configuration Register 2 */
  WRITE_REG(RCC->PLL4CFGR2, 0x00000000U);

  /* Reset RCC PLL4 Fractional Register */
  CLEAR_REG(RCC->PLL4FRACR);

  /* Reset RCC PLL4 Clock Spreading Generator Register */
  CLEAR_REG(RCC->PLL4CSGR);

  /* Reset HSIDIV once PLLs are off */
  CLEAR_BIT(RCC->HSICFGR, RCC_HSICFGR_HSIDIV);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till HSIDIV is ready */
  while ((RCC->OCRDYR & RCC_OCRDYR_HSIDIVRDY) == 0U)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Reset HSITRIM value */
  CLEAR_BIT(RCC->HSICFGR, RCC_HSICFGR_HSITRIM);

  /* Reset the Oscillator Enable Control registers */
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSIKERON | RCC_OCENCLRR_CSION |
            RCC_OCENCLRR_CSIKERON | RCC_OCENCLRR_DIGBYP | RCC_OCENCLRR_HSEON |
            RCC_OCENCLRR_HSEKERON | RCC_OCENCLRR_HSEBYP);

  /* Clear LSION bit */
  CLEAR_BIT(RCC->RDLSICR, RCC_RDLSICR_LSION);

  /* Reset CSITRIM value */
  CLEAR_BIT(RCC->CSICFGR, RCC_CSICFGR_CSITRIM);

#ifdef CORE_CA7
  /* Reset RCC Clock Source Interrupt Enable Register */
  CLEAR_BIT(RCC->MP_CIER, (RCC_MP_CIER_LSIRDYIE | RCC_MP_CIER_LSERDYIE |
                           RCC_MP_CIER_HSIRDYIE | RCC_MP_CIER_HSERDYIE | RCC_MP_CIER_CSIRDYIE |
                           RCC_MP_CIER_PLL1DYIE | RCC_MP_CIER_PLL2DYIE | RCC_MP_CIER_PLL3DYIE |
                           RCC_MP_CIER_PLL4DYIE | RCC_MP_CIER_LSECSSIE | RCC_MP_CIER_WKUPIE));

  /* Clear all RCC MPU interrupt flags */
  SET_BIT(RCC->MP_CIFR, (RCC_MP_CIFR_LSIRDYF | RCC_MP_CIFR_LSERDYF |
                         RCC_MP_CIFR_HSIRDYF | RCC_MP_CIFR_HSERDYF | RCC_MP_CIFR_CSIRDYF |
                         RCC_MP_CIFR_PLL1DYF | RCC_MP_CIFR_PLL2DYF | RCC_MP_CIFR_PLL3DYF |
                         RCC_MP_CIFR_PLL4DYF | RCC_MP_CIFR_LSECSSF | RCC_MP_CIFR_WKUPF));

  /* Clear all RCC MPU Reset Flags */
  SET_BIT(RCC->MP_RSTSCLRR, (RCC_MP_RSTSCLRR_MPUP1RSTF |
                             RCC_MP_RSTSCLRR_MPUP0RSTF | RCC_MP_RSTSCLRR_CSTDBYRSTF |
                             RCC_MP_RSTSCLRR_STDBYRSTF | RCC_MP_RSTSCLRR_IWDG2RSTF |
                             RCC_MP_RSTSCLRR_IWDG1RSTF | RCC_MP_RSTSCLRR_MCSYSRSTF |
                             RCC_MP_RSTSCLRR_MPSYSRSTF | RCC_MP_RSTSCLRR_VCORERSTF |
                             RCC_MP_RSTSCLRR_HCSSRSTF | RCC_MP_RSTSCLRR_PADRSTF |
                             RCC_MP_RSTSCLRR_BORRSTF | RCC_MP_RSTSCLRR_PORRSTF));
#endif

#ifdef CORE_CM4
  /* Reset RCC Clock Source Interrupt Enable Register */
  CLEAR_BIT(RCC->MC_CIER, (RCC_MC_CIER_LSIRDYIE | RCC_MC_CIER_LSERDYIE |
                           RCC_MC_CIER_HSIRDYIE | RCC_MC_CIER_HSERDYIE | RCC_MC_CIER_CSIRDYIE |
                           RCC_MC_CIER_PLL1DYIE | RCC_MC_CIER_PLL2DYIE | RCC_MC_CIER_PLL3DYIE |
                           RCC_MC_CIER_PLL4DYIE | RCC_MC_CIER_LSECSSIE | RCC_MC_CIER_WKUPIE));

  /* Clear all RCC MCU interrupt flags */
  SET_BIT(RCC->MC_CIFR, (RCC_MC_CIFR_LSIRDYF | RCC_MC_CIFR_LSERDYF |
                         RCC_MC_CIFR_HSIRDYF | RCC_MC_CIFR_HSERDYF | RCC_MC_CIFR_CSIRDYF |
                         RCC_MC_CIFR_PLL1DYF | RCC_MC_CIFR_PLL2DYF | RCC_MC_CIFR_PLL3DYF |
                         RCC_MC_CIFR_PLL4DYF | RCC_MC_CIFR_LSECSSF | RCC_MC_CIFR_WKUPF));

  /* Clear all RCC MCU Reset Flags */
  SET_BIT(RCC->MC_RSTSCLRR, RCC_MC_RSTSCLRR_WWDG1RSTF |
          RCC_MC_RSTSCLRR_IWDG2RSTF | RCC_MC_RSTSCLRR_IWDG1RSTF |
          RCC_MC_RSTSCLRR_MCSYSRSTF | RCC_MC_RSTSCLRR_MPSYSRSTF |
          RCC_MC_RSTSCLRR_MCURSTF | RCC_MC_RSTSCLRR_VCORERSTF |
          RCC_MC_RSTSCLRR_HCSSRSTF | RCC_MC_RSTSCLRR_PADRSTF |
          RCC_MC_RSTSCLRR_BORRSTF | RCC_MC_RSTSCLRR_PORRSTF);
#endif

  /* Update the SystemCoreClock global variable */
  SystemCoreClock = HSI_VALUE;

  /* Adapt Systick interrupt period */
  if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/**
  * @brief  Initializes the RCC Oscillators according to the specified parameters in the
  *         RCC_OscInitTypeDef.
  * @param  RCC_OscInitStruct: pointer to an RCC_OscInitTypeDef structure that
  *         contains the configuration information for the RCC Oscillators.
  * @note   The PLL is not disabled when used as system clock.
  * @retval HAL status
  */
__weak HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{
  uint32_t tickstart;
  HAL_StatusTypeDef result = HAL_OK;

  /* Check Null pointer */
  if (RCC_OscInitStruct == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_RCC_OSCILLATORTYPE(RCC_OscInitStruct->OscillatorType));
  /*------------------------------- HSE Configuration ------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSE(RCC_OscInitStruct->HSEState));
    /* When the HSE is used somewhere in the system it will not be disabled */
    if (IS_HSE_IN_USE())
    {
      if ((__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) != RESET) && (RCC_OscInitStruct->HSEState != RCC_HSE_ON))
      {
        return HAL_ERROR;
      }
    }
    else
    {
      /* Configure HSE oscillator */
      result = HAL_RCC_HSEConfig(RCC_OscInitStruct->HSEState);
      if (result != HAL_OK)
      {
        return result;
      }
    }
  }
  /*----------------------------- HSI Configuration --------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSI(RCC_OscInitStruct->HSIState));
    assert_param(IS_RCC_HSICALIBRATION_VALUE(RCC_OscInitStruct->HSICalibrationValue));
    assert_param(IS_RCC_HSIDIV(RCC_OscInitStruct->HSIDivValue));

    /* When the HSI is used as system clock it will not disabled */
    if (IS_HSI_IN_USE())
    {
      /* When HSI is used as system clock it will not disabled */
      if ((__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) != RESET) && (RCC_OscInitStruct->HSIState != RCC_HSI_ON))
      {
        return HAL_ERROR;
      }
      /* Otherwise, just the calibration is allowed */
      else
      {
        /* Adjusts the Internal High Speed oscillator (HSI) calibration value.*/
        __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->HSICalibrationValue);

        /* It is not allowed to change HSIDIV if HSI is currently used as
         * reference clock for a PLL
         */
        if (((__HAL_RCC_GET_PLL12_SOURCE() != RCC_PLL12SOURCE_HSI) ||
             ((!__HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY)) &&
              ((!__HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY))))) &&
            ((__HAL_RCC_GET_PLL3_SOURCE() != RCC_PLL3SOURCE_HSI) ||
             (!__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY))) &&
            ((__HAL_RCC_GET_PLL4_SOURCE() != RCC_PLL4SOURCE_HSI) ||
             (!__HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY))))
        {
          /* Update HSIDIV value */
          __HAL_RCC_HSI_DIV(RCC_OscInitStruct->HSIDivValue);

          /* Get Start Tick*/
          tickstart = HAL_GetTick();

          /* Wait till HSIDIV is ready */
          while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) == RESET)
          {
            if ((HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
            {
              return HAL_TIMEOUT;
            }
          }
        }

        /* Update the SystemCoreClock global variable */
        SystemCoreClock =  HAL_RCC_GetSystemCoreClockFreq();

        /* Adapt Systick interrupt period */
        if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
        {
          return HAL_ERROR;
        }
      }
    }
    else
    {
      /* Check the HSI State */
      if ((RCC_OscInitStruct->HSIState) != RCC_HSI_OFF)
      {
        /* Enable the Internal High Speed oscillator (HSI). */
        __HAL_RCC_HSI_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSI is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Update HSIDIV value */
        __HAL_RCC_HSI_DIV(RCC_OscInitStruct->HSIDivValue);

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSIDIV is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Adjusts the Internal High Speed oscillator (HSI) calibration value.*/
        __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->HSICalibrationValue);
      }
      else
      {
        /* Disable the Internal High Speed oscillator (HSI). */
        __HAL_RCC_HSI_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSI is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) != RESET)
        {
          if ((HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*----------------------------- CSI Configuration --------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_CSI) == RCC_OSCILLATORTYPE_CSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_CSI(RCC_OscInitStruct->CSIState));
    assert_param(IS_RCC_CSICALIBRATION_VALUE(RCC_OscInitStruct->CSICalibrationValue));

    /* When the CSI is used as system clock it will not disabled */
    if (IS_CSI_IN_USE())
    {
      /* When CSI is used as system clock it will not disabled */
      if ((__HAL_RCC_GET_FLAG(RCC_FLAG_CSIRDY) != RESET) && (RCC_OscInitStruct->CSIState != RCC_CSI_ON))
      {
        return HAL_ERROR;
      }
      /* Otherwise, just the calibration is allowed */
      else
      {
        /* Adjusts the Internal High Speed oscillator (CSI) calibration value.*/
        __HAL_RCC_CSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->CSICalibrationValue);
      }
    }
    else
    {
      /* Check the CSI State */
      if ((RCC_OscInitStruct->CSIState) != RCC_CSI_OFF)
      {
        /* Enable the Internal High Speed oscillator (CSI). */
        __HAL_RCC_CSI_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till CSI is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_CSIRDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > CSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Adjusts the Internal High Speed oscillator (CSI) calibration value.*/
        __HAL_RCC_CSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->CSICalibrationValue);
      }
      else
      {
        /* Disable the Internal High Speed oscillator (CSI). */
        __HAL_RCC_CSI_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till CSI is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_CSIRDY) != RESET)
        {
          if ((HAL_GetTick() - tickstart) > CSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*------------------------------ LSI Configuration -------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LSI(RCC_OscInitStruct->LSIState));

    /* Check the LSI State */
    if ((RCC_OscInitStruct->LSIState) != RCC_LSI_OFF)
    {
      /* Enable the Internal Low Speed oscillator (LSI). */
      __HAL_RCC_LSI_ENABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till LSI is ready */
      while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET)
      {
        if ((HAL_GetTick() - tickstart) > LSI_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    else
    {
      /* Disable the Internal Low Speed oscillator (LSI). */
      __HAL_RCC_LSI_DISABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till LSI is ready */
      while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) != RESET)
      {
        if ((HAL_GetTick() - tickstart) > LSI_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
  }

  /*------------------------------ LSE Configuration -------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LSE(RCC_OscInitStruct->LSEState));

    /* Enable write access to Backup domain */
    SET_BIT(PWR->CR1, PWR_CR1_DBP);

    /* Wait for Backup domain Write protection disable */
    tickstart = HAL_GetTick();

    while ((PWR->CR1 & PWR_CR1_DBP) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > DBP_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }

    result = HAL_RCC_LSEConfig(RCC_OscInitStruct->LSEState);
    if (result != HAL_OK)
    {
      return result;
    }

  } /* Close LSE Configuration */

  /*-------------------------------- PLL Configuration -----------------------*/

  /* Configure PLL1 */
  result = RCC_PLL1_Config(&(RCC_OscInitStruct->PLL));
  if (result != HAL_OK)
  {
    return result;
  }

  /* Configure PLL2 */
  result = RCCEx_PLL2_Config(&(RCC_OscInitStruct->PLL2));
  if (result != HAL_OK)
  {
    return result;
  }

  /* Configure PLL3 */
  result = RCCEx_PLL3_Config(&(RCC_OscInitStruct->PLL3));
  if (result != HAL_OK)
  {
    return result;
  }

  /* Configure PLL4 */
  result = RCCEx_PLL4_Config(&(RCC_OscInitStruct->PLL4));
  if (result != HAL_OK)
  {
    return result;
  }

  return HAL_OK;
}


/**
  * @brief  Initializes the RCC HSE Oscillator according to the specified
  *         parameter State
  * @note   Beware HSE oscillator is not used as clock before using this function.
  *         If this is the case, you have to select another source clock for item
  *         using HSE then change the HSE state (Eg.: disable it).
  * @note   The HSE is stopped by hardware when entering STOP and STANDBY modes.
  * @note   This function reset the CSSON bit, so if the clock security system(CSS)
  *         was previously enabled you have to enable it again after calling this
  *         function.
  * @param  State   contains the configuration for the RCC HSE Oscillator.
  *               This parameter can be one of the following values:
  *               @arg RCC_HSE_OFF: turn OFF the HSE oscillator
  *               @arg RCC_HSE_ON: turn ON the HSE oscillator using an external
  *                    crystal/ceramic resonator
  *               @arg RCC_HSE_BYPASS: HSE oscillator bypassed with external
  *                    clock using a low-swing analog signal provided to OSC_IN
  *               @arg RCC_HSE_BYPASS_DIG: HSE oscillator bypassed with external
  *                    clock using a full-swing digital signal provided to OSC_IN
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCC_HSEConfig(uint32_t State)
{
  uint32_t tickstart;

  /* Check parameter */
  assert_param(IS_RCC_HSE(State));

  /* Disable HSEON before configuring the HSE --------------*/
  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSEON);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till HSE is disabled */
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) != RESET)
  {
    if ((int32_t)(HAL_GetTick() - tickstart) > HSE_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Clear remaining bits */
  WRITE_REG(RCC->OCENCLRR, (RCC_OCENCLRR_DIGBYP | RCC_OCENSETR_HSEBYP));

  /* Enable HSE if needed ---------------------------------------*/
  if (State != RCC_HSE_OFF)
  {
    if (State == RCC_HSE_BYPASS)
    {
      SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSEBYP);
    }
    else if (State == RCC_HSE_BYPASS_DIG)
    {
      SET_BIT(RCC->OCENSETR, RCC_OCENCLRR_DIGBYP);
      SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSEBYP);
    }

    /* Enable oscillator */
    SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSEON);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till HSE is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > HSE_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  return HAL_OK;
}

/**
  * @brief  Initializes the RCC External Low Speed oscillator (LSE) according
  *         to the specified parameter State
  * @note   Beware LSE oscillator is not used as clock before using this function.
  *         If this is the case, you have to select another source clock for item
  *         using LSE then change the LSE state (Eg.: disable it).
  * @note   As the LSE is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         HAL_PWR_EnableBkUpAccess() function (set PWR_CR1_DBP bit) before
  *         to configure the LSE (to be done once after reset).
  * @param  State   contains the configuration for the RCC LSE Oscillator
  *         This parameter can be one of the following values:
  *            @arg RCC_LSE_OFF: turn OFF the LSE oscillator
  *            @arg RCC_LSE_ON: turn ON the LSE oscillator using an external
  *                 crystal/ceramic resonator
  *            @arg RCC_LSE_BYPASS: LSE oscillator bypassed with external clock
  *                 using a low-swing analog signal provided to OSC32_IN
  *            @arg RCC_LSE_BYPASS_DIG: LSE oscillator bypassed with external
  *                 clock using a full-swing digital signal provided to OSC32_IN
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCC_LSEConfig(uint32_t State)
{
  uint32_t tickstart;

  /* Check parameter */
  assert_param(IS_RCC_LSE(State));

  /* Turning LSE off is needed before configuring */
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till LSE is disabled */
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) != RESET)
  {
    if ((HAL_GetTick() - tickstart) > LSE_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Clear remaining bits */
  CLEAR_BIT(RCC->BDCR, (RCC_BDCR_LSEBYP | RCC_BDCR_DIGBYP));

  /* Enable LSE if needed */
  if (State != RCC_LSE_OFF)
  {
    if (State == RCC_LSE_BYPASS)
    {
      SET_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);
    }
    else if (State == RCC_LSE_BYPASS_DIG)
    {
      SET_BIT(RCC->BDCR, RCC_BDCR_DIGBYP);
      SET_BIT(RCC->BDCR, RCC_BDCR_LSEBYP);
    }

    /* Enable oscillator */
    SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till LSE is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > LSE_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  } /* Enable LSE if needed */

  return HAL_OK;
}

HAL_StatusTypeDef RCC_PLL1_Config(RCC_PLLInitTypeDef *pll1)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RCC_PLL(pll1->PLLState));
  if ((pll1->PLLState) != RCC_PLL_NONE)
  {
    /* Check if the PLL is used as system clock or not (MPU, MCU, AXISS)*/
    if (!__IS_PLL1_IN_USE()) /* If not used then */
    {
      if ((pll1->PLLState) == RCC_PLL_ON)
      {
        /* Check the parameters */
        assert_param(IS_RCC_PLLMODE(pll1->PLLMODE));
        assert_param(IS_RCC_PLL12SOURCE(pll1->PLLSource));
        assert_param(IS_RCC_PLLM1_VALUE(pll1->PLLM));
        if (pll1->PLLMODE == RCC_PLL_FRACTIONAL)
        {
          assert_param(IS_RCC_PLLN1_FRAC_VALUE(pll1->PLLN));
        }
        else
        {
          assert_param(IS_RCC_PLLN1_INT_VALUE(pll1->PLLN));
        }
        assert_param(IS_RCC_PLLP1_VALUE(pll1->PLLP));
        assert_param(IS_RCC_PLLQ1_VALUE(pll1->PLLQ));
        assert_param(IS_RCC_PLLR1_VALUE(pll1->PLLR));

        /*Disable the post-dividers*/
        __HAL_RCC_PLL1CLKOUT_DISABLE(RCC_PLL1_DIVP | RCC_PLL1_DIVQ | RCC_PLL1_DIVR);
        /* Disable the main PLL. */
        __HAL_RCC_PLL1_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY) != RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }


        /*The PLL configuration below must be done before enabling the PLL:
        -Selection of PLL clock entry (HSI or HSE)
        -Frequency input range (PLLxRGE)
        -Division factors (DIVMx, DIVNx, DIVPx, DIVQx & DIVRx)
        Once the PLL is enabled, these parameters can not be changed.
        If the User wants to change the PLL parameters he must disable the concerned PLL (PLLxON=0) and wait for the
        PLLxRDY flag to be at 0.
        The PLL configuration below can be done at any time:
        -Enable/Disable of output clock dividers (DIVPxEN, DIVQxEN & DIVRxEN)
        -Fractional Division Enable (PLLxFRACNEN)
        -Fractional Division factor (FRACNx)*/

        /* Do not change pll src if already in use */
        if (__IS_PLL2_IN_USE())
        {
          if (pll1->PLLSource != __HAL_RCC_GET_PLL12_SOURCE())
          {
            return HAL_ERROR;
          }
        }
        else
        {
          /* Configure PLL1 and PLL2 clock source */
          __HAL_RCC_PLL12_SOURCE(pll1->PLLSource);
        }

        /* Wait till PLL SOURCE is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL12SRCRDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Configure the PLL1 multiplication and division factors. */
        __HAL_RCC_PLL1_CONFIG(
          pll1->PLLM,
          pll1->PLLN,
          pll1->PLLP,
          pll1->PLLQ,
          pll1->PLLR);


        /* Configure the Fractional Divider */
        __HAL_RCC_PLL1FRACV_DISABLE(); /*Set FRACLE to '0' */
        /* In integer or clock spreading mode the application shall ensure that a 0 is loaded into the SDM */
        if ((pll1->PLLMODE == RCC_PLL_SPREAD_SPECTRUM) || (pll1->PLLMODE == RCC_PLL_INTEGER))
        {
          /* Do not use the fractional divider */
          __HAL_RCC_PLL1FRACV_CONFIG(0U); /* Set FRACV to '0' */
        }
        else
        {
          /* Configure PLL  PLL1FRACV  in fractional mode*/
          __HAL_RCC_PLL1FRACV_CONFIG(pll1->PLLFRACV);
        }
        __HAL_RCC_PLL1FRACV_ENABLE(); /* Set FRACLE to 1 */


        /* Configure the Spread Control */
        if (pll1->PLLMODE == RCC_PLL_SPREAD_SPECTRUM)
        {
          assert_param(IS_RCC_INC_STEP(pll1->INC_STEP));
          assert_param(IS_RCC_SSCG_MODE(pll1->SSCG_MODE));
          assert_param(IS_RCC_RPDFN_DIS(pll1->RPDFN_DIS));
          assert_param(IS_RCC_TPDFN_DIS(pll1->TPDFN_DIS));
          assert_param(IS_RCC_MOD_PER(pll1->MOD_PER));

          __HAL_RCC_PLL1CSGCONFIG(pll1->MOD_PER, pll1->TPDFN_DIS, pll1->RPDFN_DIS,
                                  pll1->SSCG_MODE, pll1->INC_STEP);

          __HAL_RCC_PLL1_SSMODE_ENABLE();
        }
        else
        {
          __HAL_RCC_PLL1_SSMODE_DISABLE();
        }

        /* Enable the PLL1. */
        __HAL_RCC_PLL1_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /* Enable post-dividers */
        __HAL_RCC_PLL1CLKOUT_ENABLE(RCC_PLL1_DIVP | RCC_PLL1_DIVQ | RCC_PLL1_DIVR);
      }
      else
      {
        /*Disable the post-dividers*/
        __HAL_RCC_PLL1CLKOUT_DISABLE(RCC_PLL1_DIVP | RCC_PLL1_DIVQ | RCC_PLL1_DIVR);
        /* Disable the PLL1. */
        __HAL_RCC_PLL1_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY) != RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
    else
    {
      return HAL_ERROR;
    }
  }
  return HAL_OK;

}



/**
  * @brief  Initializes the MPU,AXI, AHB and APB busses clocks according to the specified
  *         parameters in the RCC_ClkInitStruct.
  * @param  RCC_ClkInitStruct: pointer to an RCC_OscInitTypeDef structure that
  *         contains the configuration information for the RCC peripheral.
  *
  * @note   The SystemCoreClock CCSIS variable is used to store System Clock Frequency
  *         and updated by HAL_RCC_GetHCLKFreq() function called within this function
  *
  * @note   The HSI is used (enabled by hardware) as system clock source after
  *         startup from Reset, wake-up from STOP and STANDBY mode, or in case
  *         of failure of the HSE used directly or indirectly as system clock
  *         (if the Clock Security System CSS is enabled).
  *
  * @note   A switch from one clock source to another occurs only if the target
  *         clock source is ready (clock stable after startup delay or PLL locked).
  *         If a clock source which is not yet ready is selected, the switch will
  *         occur when the clock source will be ready.
  *
  * @note   Depending on the device voltage range, the software has to set correctly
  *         HPRE[3:0] bits to ensure that HCLK not exceed the maximum allowed frequency
  *         (for more details refer to section above "Initialization/de-initialization functions")
  * @retval None
  */
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct)
{

  HAL_StatusTypeDef status = HAL_OK;
  uint32_t tickstart;

  /* Check Null pointer */
  if (RCC_ClkInitStruct == NULL)
  {
    return HAL_ERROR;
  }

  assert_param(IS_RCC_CLOCKTYPETYPE(RCC_ClkInitStruct->ClockType));

  /* Configure MPU block if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_MPU) == RCC_CLOCKTYPE_MPU)
  {
    status = RCC_MPUConfig(&(RCC_ClkInitStruct->MPUInit));
    if (status  != HAL_OK)
    {
      return status;
    }
  }

  /* Configure AXISS block if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_ACLK) == RCC_CLOCKTYPE_ACLK)
  {
    status = RCC_AXISSConfig(&(RCC_ClkInitStruct->AXISSInit));
    if (status  != HAL_OK)
    {
      return status;
    }
  }

  /* Configure MCU block if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_HCLK) == RCC_CLOCKTYPE_HCLK)
  {
    status = RCC_MCUConfig(&(RCC_ClkInitStruct->MCUInit));
    if (status  != HAL_OK)
    {
      return status;
    }
  }

  /* Configure APB4 divisor if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_PCLK4) == RCC_CLOCKTYPE_PCLK4)
  {
    assert_param(IS_RCC_APB4DIV(RCC_ClkInitStruct->APB4_Div));
    /* Set APB4 division factor */
    __HAL_RCC_APB4_DIV(RCC_ClkInitStruct->APB4_Div);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till APB4 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_APB4DIVRDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Configure APB5 divisor if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_PCLK5) == RCC_CLOCKTYPE_PCLK5)
  {
    assert_param(IS_RCC_APB5DIV(RCC_ClkInitStruct->APB5_Div));
    /* Set APB5 division factor */
    __HAL_RCC_APB5_DIV(RCC_ClkInitStruct->APB5_Div);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till APB5 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_APB5DIVRDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Configure APB1 divisor if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_PCLK1) == RCC_CLOCKTYPE_PCLK1)
  {
    assert_param(IS_RCC_APB1DIV(RCC_ClkInitStruct->APB1_Div));
    /* Set APB1 division factor */
    __HAL_RCC_APB1_DIV(RCC_ClkInitStruct->APB1_Div);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till APB1 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_APB1DIVRDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Configure APB2 divisor if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_PCLK2) == RCC_CLOCKTYPE_PCLK2)
  {
    assert_param(IS_RCC_APB2DIV(RCC_ClkInitStruct->APB2_Div));
    /* Set APB2 division factor */
    __HAL_RCC_APB2_DIV(RCC_ClkInitStruct->APB2_Div);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till APB2 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_APB2DIVRDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Configure APB3 divisor if needed */
  if ((RCC_ClkInitStruct->ClockType & RCC_CLOCKTYPE_PCLK3) == RCC_CLOCKTYPE_PCLK3)
  {
    assert_param(IS_RCC_APB3DIV(RCC_ClkInitStruct->APB3_Div));
    /* Set APB3 division factor */
    __HAL_RCC_APB3_DIV(RCC_ClkInitStruct->APB3_Div);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till APB3 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_APB3DIVRDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef RCC_MPUConfig(RCC_MPUInitTypeDef *RCC_MPUInitStruct)
{
  uint32_t tickstart;

  assert_param(IS_RCC_MPUSOURCE(RCC_MPUInitStruct->MPU_Clock));

  /* Ensure clock source is ready*/
  switch (RCC_MPUInitStruct->MPU_Clock)
  {
    case (RCC_MPUSOURCE_HSI):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_MPUSOURCE_HSE):
    {
      /* Check the HSE ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_MPUSOURCE_PLL1):
    {
      /* Check the PLL1 ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_MPUSOURCE_MPUDIV):
    {
      assert_param(IS_RCC_MPUDIV(RCC_MPUInitStruct->MPU_Div));

      /* Check the PLL1 ready flag (as PLL1_P is the MPUDIV source */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY) == RESET)
      {
        return HAL_ERROR;
      }

      /* Set MPU division factor */
      __HAL_RCC_MPU_DIV(RCC_MPUInitStruct->MPU_Div);

      break;
    }

    default:
      /* This case is impossible */
      return HAL_ERROR;
      break;

  }

  /* Set MPU clock source */
  __HAL_RCC_MPU_SOURCE(RCC_MPUInitStruct->MPU_Clock);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till MPU is ready */
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_MPUSRCRDY) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }
#ifdef CORE_CA7
  /* Update the SystemCoreClock global variable */
  SystemCoreClock = HAL_RCC_GetSystemCoreClockFreq();

  /* Configure the source of time base considering new system clocks settings*/
  HAL_InitTick(TICK_INT_PRIORITY);
#endif

  return HAL_OK;
}


HAL_StatusTypeDef RCC_AXISSConfig(RCC_AXISSInitTypeDef *RCC_AXISSInitStruct)
{
  uint32_t tickstart;

  assert_param(IS_RCC_AXISSOURCE(RCC_AXISSInitStruct->AXI_Clock));
  assert_param(IS_RCC_AXIDIV(RCC_AXISSInitStruct->AXI_Div));

  /* Ensure clock source is ready*/
  switch (RCC_AXISSInitStruct->AXI_Clock)
  {
    case (RCC_AXISSOURCE_HSI):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_AXISSOURCE_HSE):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_AXISSOURCE_PLL2):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    default:
      break;

  }

  /* Set AXISS clock source */
  __HAL_RCC_AXISS_SOURCE(RCC_AXISSInitStruct->AXI_Clock);

  if (RCC_AXISSInitStruct->AXI_Clock != RCC_AXISSOURCE_OFF)
  {
    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till AXISS is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_AXISSRCRDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }
  else
  {
    // RCC_AXISSOURCE_OFF case
    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till AXISS is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_AXISSRCRDY) != RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Set AXISS division factor */
  __HAL_RCC_AXI_DIV(RCC_AXISSInitStruct->AXI_Div);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till AXISS is ready */
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_AXIDIVRDY) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}


HAL_StatusTypeDef RCC_MCUConfig(RCC_MCUInitTypeDef *MCUInitStruct)
{
  uint32_t tickstart;

  assert_param(IS_RCC_MCUSSOURCE(MCUInitStruct->MCU_Clock));
  assert_param(IS_RCC_MCUDIV(MCUInitStruct->MCU_Div));

  /* Ensure clock source is ready*/
  switch (MCUInitStruct->MCU_Clock)
  {
    case (RCC_MCUSSOURCE_HSI):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_MCUSSOURCE_HSE):
    {
      /* Check the HSE ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_MCUSSOURCE_CSI):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_CSIRDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    case (RCC_MCUSSOURCE_PLL3):
    {
      /* Check the HSI ready flag */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) == RESET)
      {
        return HAL_ERROR;
      }
      break;
    }

    default:
      break;

  }

  /* Set MCU clock source */
  __HAL_RCC_MCU_SOURCE(MCUInitStruct->MCU_Clock);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till MCU is ready */

  while (__HAL_RCC_GET_FLAG(RCC_FLAG_MCUSSRCRDY) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

#ifdef CORE_CM4
  /* Update the SystemCoreClock global variable */
  SystemCoreClock = HAL_RCC_GetSystemCoreClockFreq();

  /* Configure the source of time base considering new system clocks settings*/
  HAL_InitTick(TICK_INT_PRIORITY);
#endif

  /* Set MCU division factor */
  __HAL_RCC_MCU_DIV(MCUInitStruct->MCU_Div);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till MCU is ready */
  while (__HAL_RCC_GET_FLAG(RCC_FLAG_MCUDIVRDY) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }
#ifdef CORE_CM4
  /* Update the SystemCoreClock global variable */
  SystemCoreClock = HAL_RCC_GetSystemCoreClockFreq();

  /* Configure the source of time base considering new system clocks settings*/
  HAL_InitTick(TICK_INT_PRIORITY);
#endif

  return HAL_OK;
}

/**
  * @}
  */

/** @defgroup RCC_Exported_Functions_Group2 Peripheral Control functions
  *  @brief   RCC clocks control functions
  *
  @verbatim
  ===============================================================================
                       ##### Peripheral Control functions #####
  ===============================================================================
     [..]
     This subsection provides a set of functions allowing to control the RCC Clocks
     frequencies.
  @endverbatim
  * @{
  */

/**
  * @brief  Selects the clock source to output on MCO1 pin or on MCO2 pin
  * @note   BSP_MCO_Init shall be called before to configure output pins depending on the board
  * @param  RCC_MCOx: specifies the output direction for the clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCO1: Clock source to output on MCO1
  *            @arg RCC_MCO2: Clock source to output on MCO2
  * @param  RCC_MCOSource: specifies the clock source to output.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCO1SOURCE_HSI: HSI clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_HSE: HSE clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_CSI: CSI clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_LSI: LSI clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_LSE: LSE clock selected as MCO1 clock entry
  *            @arg RCC_MCO2SOURCE_MPU: MPU clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_AXI: AXI clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_MCU: MCU clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_PLL4: PLL4 clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_HSE: HSE clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_HSI: HSI clock selected as MCO2 clock entry
  *
  * @param  RCC_MCODiv: specifies the MCOx prescaler.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCODIV_1 up to RCC_MCODIV_16  : divider applied to MCOx clock
  * @retval None
  */
void HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv)
{
  /* Check the parameters */
  assert_param(IS_RCC_MCO(RCC_MCOx));
  assert_param(IS_RCC_MCODIV(RCC_MCODiv));
  /* RCC_MCO1 */
  if (RCC_MCOx == RCC_MCO1)
  {
    assert_param(IS_RCC_MCO1SOURCE(RCC_MCOSource));

    /* Configure MCO1 */
    __HAL_RCC_MCO1_CONFIGURE(RCC_MCOSource, RCC_MCODiv);

    /* Enable MCO1 output */
    __HAL_RCC_MCO1_ENABLE();
  }
  else
  {
    assert_param(IS_RCC_MCO2SOURCE(RCC_MCOSource));

    /* Configure MCO2 */
    __HAL_RCC_MCO2_CONFIGURE(RCC_MCOSource, RCC_MCODiv);

    /* Enable MCO2 output */
    __HAL_RCC_MCO2_ENABLE();
  }
}


/**
  * @brief  Enables the HSE Clock Security System.
  * @note   If a failure is detected on the HSE:
  *         - The RCC generates a system reset (nreset). The flag HCSSRSTF is set in order to
  *           allow the application to identify the reset root cause.
  *         - A failure event is generated (rcc_hsecss_fail). This event is connected to the TAMP
  *           block, allowing the protection of backup registers and BKPSRAM.
  *
  * @note   HSECSSON can be activated even when the HSEON is set to ‘0’. The CSS on HSE will be
  *         enabled by the hardware when the HSE is enabled and ready, and HSECSSON is set to ‘1’.
  *
  * @note   The HSECSS is disabled when the HSE is disabled (i.e. when the system is STOP or
  *         STANDBY).
  *
  * @note   It is not possible to clear the bit HSECSSON by software.
  *
  * @note   The bit HSECSSON will be cleared by the hardware when a system reset occurs or when
            the system enters in STANDBY mode
  * @retval None
  */
void HAL_RCC_EnableHSECSS(void)
{
  SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSECSSON);
}

/**
  * @brief  Configures the RCC_OscInitStruct according to the internal
  * RCC configuration registers.
  * @param  RCC_OscInitStruct: pointer to an RCC_OscInitTypeDef structure that
  * will be configured.
  * @retval None
  */
void HAL_RCC_GetOscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{

  /* Set all possible values for the Oscillator type parameter ---------------*/
  RCC_OscInitStruct->OscillatorType =
    RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_CSI |
    RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_LSI;

  /* Get the HSE configuration -----------------------------------------------*/
  if ((RCC->OCENSETR & RCC_OCENSETR_HSEBYP) == RCC_OCENSETR_HSEBYP)
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_BYPASS;
  }
  else if ((RCC->OCENSETR & RCC_OCENSETR_HSEON) == RCC_OCENSETR_HSEON)
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
  }
  else
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_OFF;
  }

  /* Get the CSI configuration -----------------------------------------------*/
  if ((RCC->OCENSETR  & RCC_OCENSETR_CSION) == RCC_OCENSETR_CSION)
  {
    RCC_OscInitStruct->CSIState = RCC_CSI_ON;
  }
  else
  {
    RCC_OscInitStruct->CSIState = RCC_CSI_OFF;
  }

  RCC_OscInitStruct->CSICalibrationValue =
    (uint32_t)((RCC->CSICFGR & RCC_CSICFGR_CSITRIM) >> RCC_CSICFGR_CSITRIM_Pos);


  /* Get the HSI configuration -----------------------------------------------*/
  if ((RCC->OCENSETR  & RCC_OCENSETR_HSION) == RCC_OCENSETR_HSION)
  {
    RCC_OscInitStruct->HSIState = RCC_HSI_ON;
  }
  else
  {
    RCC_OscInitStruct->HSIState = RCC_HSI_OFF;
  }

  RCC_OscInitStruct->HSICalibrationValue =
    (uint32_t)((RCC->HSICFGR & RCC_HSICFGR_HSITRIM) >> RCC_HSICFGR_HSITRIM_Pos);


  /* Get the LSE configuration -----------------------------------------------*/
  if ((RCC->BDCR & RCC_BDCR_LSEBYP) == RCC_BDCR_LSEBYP)
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_BYPASS;
  }
  else if ((RCC->BDCR & RCC_BDCR_LSEON) == RCC_BDCR_LSEON)
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_ON;
  }
  else
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_OFF;
  }

  /* Get the LSI configuration -----------------------------------------------*/
  if ((RCC->RDLSICR & RCC_RDLSICR_LSION) == RCC_RDLSICR_LSION)
  {
    RCC_OscInitStruct->LSIState = RCC_LSI_ON;
  }
  else
  {
    RCC_OscInitStruct->LSIState = RCC_LSI_OFF;
  }


  /* Get the PLL1 configuration -----------------------------------------------*/
  if ((RCC->PLL1CR & RCC_PLL1CR_PLLON) == RCC_PLL1CR_PLLON)
  {
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
  }
  else
  {
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_OFF;
  }

  RCC_OscInitStruct->PLL.PLLSource = (uint32_t)(RCC->RCK12SELR & RCC_RCK12SELR_PLL12SRC);
  RCC_OscInitStruct->PLL.PLLM = (uint32_t)((RCC->PLL1CFGR1 & RCC_PLL1CFGR1_DIVM1) >> RCC_PLL1CFGR1_DIVM1_Pos) + 1;
  RCC_OscInitStruct->PLL.PLLN = (uint32_t)((RCC->PLL1CFGR1 & RCC_PLL1CFGR1_DIVN) >> RCC_PLL1CFGR1_DIVN_Pos) + 1;
  RCC_OscInitStruct->PLL.PLLR = (uint32_t)((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVR) >> RCC_PLL1CFGR2_DIVR_Pos) + 1;
  RCC_OscInitStruct->PLL.PLLP = (uint32_t)((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVP) >> RCC_PLL1CFGR2_DIVP_Pos) + 1;
  RCC_OscInitStruct->PLL.PLLQ = (uint32_t)((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVQ) >> RCC_PLL1CFGR2_DIVQ_Pos) + 1;
  RCC_OscInitStruct->PLL.PLLFRACV = (uint32_t)((RCC->PLL1FRACR & RCC_PLL1FRACR_FRACV) >> RCC_PLL1FRACR_FRACV_Pos);

  if ((RCC->PLL1FRACR & RCC_PLL1FRACR_FRACV) == 0)
  {
    if ((RCC->PLL1CR & RCC_PLL1CR_SSCG_CTRL) ==  RCC_PLL1CR_SSCG_CTRL)
    {
      //SSMODE enabled
      RCC_OscInitStruct->PLL.PLLMODE = RCC_PLL_SPREAD_SPECTRUM;
    }
    else
    {
      RCC_OscInitStruct->PLL.PLLMODE = RCC_PLL_INTEGER;
    }
  }
  else
  {
    RCC_OscInitStruct->PLL.PLLMODE = RCC_PLL_FRACTIONAL;
  }

  RCC_OscInitStruct->PLL.MOD_PER = (uint32_t)(RCC->PLL1CSGR & RCC_PLL1CSGR_MOD_PER);
  RCC_OscInitStruct->PLL.TPDFN_DIS = (uint32_t)(RCC->PLL1CSGR & RCC_PLL1CSGR_TPDFN_DIS);
  RCC_OscInitStruct->PLL.RPDFN_DIS = (uint32_t)(RCC->PLL1CSGR & RCC_PLL1CSGR_RPDFN_DIS);
  RCC_OscInitStruct->PLL.SSCG_MODE = (uint32_t)(RCC->PLL1CSGR & RCC_PLL1CSGR_SSCG_MODE);
  RCC_OscInitStruct->PLL.INC_STEP = (uint32_t)((RCC->PLL1CSGR & RCC_PLL1CSGR_INC_STEP) >> RCC_PLL1CSGR_INC_STEP_Pos);


  /* Get the PLL2 configuration -----------------------------------------------*/
  if ((RCC->PLL2CR & RCC_PLL2CR_PLLON) == RCC_PLL2CR_PLLON)
  {
    RCC_OscInitStruct->PLL2.PLLState = RCC_PLL_ON;
  }
  else
  {
    RCC_OscInitStruct->PLL2.PLLState = RCC_PLL_OFF;
  }

  RCC_OscInitStruct->PLL2.PLLSource = (uint32_t)(RCC->RCK12SELR & RCC_RCK12SELR_PLL12SRC);
  RCC_OscInitStruct->PLL2.PLLM = (uint32_t)((RCC->PLL2CFGR1 & RCC_PLL2CFGR1_DIVM2) >> RCC_PLL2CFGR1_DIVM2_Pos) + 1;
  RCC_OscInitStruct->PLL2.PLLN = (uint32_t)((RCC->PLL2CFGR1 & RCC_PLL2CFGR1_DIVN) >> RCC_PLL2CFGR1_DIVN_Pos) + 1;
  RCC_OscInitStruct->PLL2.PLLR = (uint32_t)((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVR) >> RCC_PLL2CFGR2_DIVR_Pos) + 1;
  RCC_OscInitStruct->PLL2.PLLP = (uint32_t)((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVP) >> RCC_PLL2CFGR2_DIVP_Pos) + 1;
  RCC_OscInitStruct->PLL2.PLLQ = (uint32_t)((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVQ) >> RCC_PLL2CFGR2_DIVQ_Pos) + 1;
  RCC_OscInitStruct->PLL2.PLLFRACV = (uint32_t)((RCC->PLL2FRACR & RCC_PLL2FRACR_FRACV) >> RCC_PLL2FRACR_FRACV_Pos);

  if ((RCC->PLL2FRACR & RCC_PLL2FRACR_FRACV) == 0)
  {
    if ((RCC->PLL2CR & RCC_PLL2CR_SSCG_CTRL) ==  RCC_PLL2CR_SSCG_CTRL)
    {
      //SSMODE enabled
      RCC_OscInitStruct->PLL2.PLLMODE = RCC_PLL_SPREAD_SPECTRUM;
    }
    else
    {
      RCC_OscInitStruct->PLL2.PLLMODE = RCC_PLL_INTEGER;
    }
  }
  else
  {
    RCC_OscInitStruct->PLL2.PLLMODE = RCC_PLL_FRACTIONAL;
  }

  RCC_OscInitStruct->PLL2.MOD_PER = (uint32_t)(RCC->PLL2CSGR & RCC_PLL2CSGR_MOD_PER);
  RCC_OscInitStruct->PLL2.TPDFN_DIS = (uint32_t)(RCC->PLL2CSGR & RCC_PLL2CSGR_TPDFN_DIS);
  RCC_OscInitStruct->PLL2.RPDFN_DIS = (uint32_t)(RCC->PLL2CSGR & RCC_PLL2CSGR_RPDFN_DIS);
  RCC_OscInitStruct->PLL2.SSCG_MODE = (uint32_t)(RCC->PLL2CSGR & RCC_PLL2CSGR_SSCG_MODE);
  RCC_OscInitStruct->PLL2.INC_STEP = (uint32_t)((RCC->PLL2CSGR & RCC_PLL2CSGR_INC_STEP) >> RCC_PLL2CSGR_INC_STEP_Pos);


  /* Get the PLL3 configuration -----------------------------------------------*/
  if ((RCC->PLL3CR & RCC_PLL3CR_PLLON) == RCC_PLL3CR_PLLON)
  {
    RCC_OscInitStruct->PLL3.PLLState = RCC_PLL_ON;
  }
  else
  {
    RCC_OscInitStruct->PLL3.PLLState = RCC_PLL_OFF;
  }

  RCC_OscInitStruct->PLL3.PLLSource = (uint32_t)(RCC->RCK3SELR & RCC_RCK3SELR_PLL3SRC);
  RCC_OscInitStruct->PLL3.PLLM = (uint32_t)((RCC->PLL3CFGR1 & RCC_PLL3CFGR1_DIVM3) >> RCC_PLL3CFGR1_DIVM3_Pos) + 1;
  RCC_OscInitStruct->PLL3.PLLN = (uint32_t)((RCC->PLL3CFGR1 & RCC_PLL3CFGR1_DIVN) >> RCC_PLL3CFGR1_DIVN_Pos) + 1;
  RCC_OscInitStruct->PLL3.PLLR = (uint32_t)((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVR) >> RCC_PLL3CFGR2_DIVR_Pos) + 1;
  RCC_OscInitStruct->PLL3.PLLP = (uint32_t)((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVP) >> RCC_PLL3CFGR2_DIVP_Pos) + 1;
  RCC_OscInitStruct->PLL3.PLLQ = (uint32_t)((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVQ) >> RCC_PLL3CFGR2_DIVQ_Pos) + 1;
  RCC_OscInitStruct->PLL3.PLLRGE = (uint32_t)(RCC->PLL3CFGR1 & RCC_PLL3CFGR1_IFRGE);
  RCC_OscInitStruct->PLL3.PLLFRACV = (uint32_t)((RCC->PLL3FRACR & RCC_PLL3FRACR_FRACV) >> RCC_PLL3FRACR_FRACV_Pos);

  if ((RCC->PLL3FRACR & RCC_PLL3FRACR_FRACV) == 0)
  {
    if ((RCC->PLL3CR & RCC_PLL3CR_SSCG_CTRL) ==  RCC_PLL3CR_SSCG_CTRL)
    {
      //SSMODE enabled
      RCC_OscInitStruct->PLL3.PLLMODE = RCC_PLL_SPREAD_SPECTRUM;
    }
    else
    {
      RCC_OscInitStruct->PLL3.PLLMODE = RCC_PLL_INTEGER;
    }
  }
  else
  {
    RCC_OscInitStruct->PLL3.PLLMODE = RCC_PLL_FRACTIONAL;
  }

  RCC_OscInitStruct->PLL3.MOD_PER = (uint32_t)(RCC->PLL3CSGR & RCC_PLL3CSGR_MOD_PER);
  RCC_OscInitStruct->PLL3.TPDFN_DIS = (uint32_t)(RCC->PLL3CSGR & RCC_PLL3CSGR_TPDFN_DIS);
  RCC_OscInitStruct->PLL3.RPDFN_DIS = (uint32_t)(RCC->PLL3CSGR & RCC_PLL3CSGR_RPDFN_DIS);
  RCC_OscInitStruct->PLL3.SSCG_MODE = (uint32_t)(RCC->PLL3CSGR & RCC_PLL3CSGR_SSCG_MODE);
  RCC_OscInitStruct->PLL3.INC_STEP = (uint32_t)((RCC->PLL3CSGR & RCC_PLL3CSGR_INC_STEP) >> RCC_PLL3CSGR_INC_STEP_Pos);

  /* Get the PLL4 configuration -----------------------------------------------*/
  if ((RCC->PLL4CR & RCC_PLL4CR_PLLON) == RCC_PLL4CR_PLLON)
  {
    RCC_OscInitStruct->PLL4.PLLState = RCC_PLL_ON;
  }
  else
  {
    RCC_OscInitStruct->PLL4.PLLState = RCC_PLL_OFF;
  }

  RCC_OscInitStruct->PLL4.PLLSource = (uint32_t)(RCC->RCK4SELR & RCC_RCK4SELR_PLL4SRC);
  RCC_OscInitStruct->PLL4.PLLM = (uint32_t)((RCC->PLL4CFGR1 & RCC_PLL4CFGR1_DIVM4) >> RCC_PLL4CFGR1_DIVM4_Pos) + 1;
  RCC_OscInitStruct->PLL4.PLLN = (uint32_t)((RCC->PLL4CFGR1 & RCC_PLL4CFGR1_DIVN) >> RCC_PLL4CFGR1_DIVN_Pos) + 1;
  RCC_OscInitStruct->PLL4.PLLR = (uint32_t)((RCC->PLL4CFGR2 & RCC_PLL4CFGR2_DIVR) >> RCC_PLL4CFGR2_DIVR_Pos) + 1;
  RCC_OscInitStruct->PLL4.PLLP = (uint32_t)((RCC->PLL4CFGR2 & RCC_PLL4CFGR2_DIVP) >> RCC_PLL4CFGR2_DIVP_Pos) + 1;
  RCC_OscInitStruct->PLL4.PLLQ = (uint32_t)((RCC->PLL4CFGR2 & RCC_PLL4CFGR2_DIVQ) >> RCC_PLL4CFGR2_DIVQ_Pos) + 1;
  RCC_OscInitStruct->PLL4.PLLRGE = (uint32_t)(RCC->PLL4CFGR1 & RCC_PLL4CFGR1_IFRGE);
  RCC_OscInitStruct->PLL4.PLLFRACV = (uint32_t)((RCC->PLL4FRACR & RCC_PLL4FRACR_FRACV) >> RCC_PLL4FRACR_FRACV_Pos);

  if ((RCC->PLL4FRACR & RCC_PLL4FRACR_FRACV) == 0)
  {
    if ((RCC->PLL4CR & RCC_PLL4CR_SSCG_CTRL) ==  RCC_PLL4CR_SSCG_CTRL)
    {
      //SSMODE enabled
      RCC_OscInitStruct->PLL4.PLLMODE = RCC_PLL_SPREAD_SPECTRUM;
    }
    else
    {
      RCC_OscInitStruct->PLL4.PLLMODE = RCC_PLL_INTEGER;
    }
  }
  else
  {
    RCC_OscInitStruct->PLL4.PLLMODE = RCC_PLL_FRACTIONAL;
  }

  RCC_OscInitStruct->PLL4.MOD_PER = (uint32_t)(RCC->PLL4CSGR & RCC_PLL4CSGR_MOD_PER);
  RCC_OscInitStruct->PLL4.TPDFN_DIS = (uint32_t)(RCC->PLL4CSGR & RCC_PLL4CSGR_TPDFN_DIS);
  RCC_OscInitStruct->PLL4.RPDFN_DIS = (uint32_t)(RCC->PLL4CSGR & RCC_PLL4CSGR_RPDFN_DIS);
  RCC_OscInitStruct->PLL4.SSCG_MODE = (uint32_t)(RCC->PLL4CSGR & RCC_PLL4CSGR_SSCG_MODE);
  RCC_OscInitStruct->PLL4.INC_STEP = (uint32_t)((RCC->PLL4CSGR & RCC_PLL4CSGR_INC_STEP) >> RCC_PLL4CSGR_INC_STEP_Pos);

}

/**
  * @brief  Configures the RCC_ClkInitStruct according to the internal
  * RCC configuration registers.
  * @param  RCC_ClkInitStruct: pointer to an RCC_ClkInitTypeDef structure that
  * will be configured.
  * @param  pFLatency: Pointer on the Flash Latency.
  * @retval None
  */
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t *pFLatency)
{
  /* Prevent unused argument compilation warning */
  UNUSED(pFLatency);

  /* Set all possible values for the Clock type parameter --------------------*/
  RCC_ClkInitStruct->ClockType = RCC_CLOCKTYPE_MPU | RCC_CLOCKTYPE_ACLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK4 |
                                 RCC_CLOCKTYPE_PCLK5 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_PCLK3;

  RCC_ClkInitStruct->MPUInit.MPU_Clock =    __HAL_RCC_GET_MPU_SOURCE();
  RCC_ClkInitStruct->MPUInit.MPU_Div =      __HAL_RCC_GET_MPU_DIV();

  RCC_ClkInitStruct->AXISSInit.AXI_Clock =  __HAL_RCC_GET_AXIS_SOURCE();
  RCC_ClkInitStruct->AXISSInit.AXI_Div =    __HAL_RCC_GET_AXI_DIV();

  RCC_ClkInitStruct->MCUInit.MCU_Clock =    __HAL_RCC_GET_MCU_SOURCE();
  RCC_ClkInitStruct->MCUInit.MCU_Div =      __HAL_RCC_GET_MCU_DIV();

  RCC_ClkInitStruct->APB1_Div = __HAL_RCC_GET_APB1_DIV();
  RCC_ClkInitStruct->APB2_Div = __HAL_RCC_GET_APB2_DIV();
  RCC_ClkInitStruct->APB3_Div = __HAL_RCC_GET_APB3_DIV();
  RCC_ClkInitStruct->APB4_Div = __HAL_RCC_GET_APB4_DIV();
  RCC_ClkInitStruct->APB5_Div = __HAL_RCC_GET_APB5_DIV();
}


/**
* @brief  Returns the PLL1 clock frequencies :PLL1_P_Frequency,PLL1_R_Frequency and PLL1_Q_Frequency
  * @note   The PLL1 clock frequencies computed by this function is not the real
  *         frequency in the chip. It is calculated based on the predefined
  *         constant and the selected clock source:
  * @note     The function returns values based on HSE_VALUE or HSI_VALUE Value multiplied/divided by the PLL factors.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  *
  * @note   Each time PLL1CLK changes, this function must be called to update the
  *         right PLL1CLK value. Otherwise, any configuration based on this function will be incorrect.
  * @param  PLL1_Clocks structure.
  * @retval None
  */
__weak void HAL_RCC_GetPLL1ClockFreq(PLL1_ClocksTypeDef *PLL1_Clocks)
{
  uint32_t   pllsource = 0U, pll1m = 1U, pll1fracen = 0U, hsivalue = 0U;
  float fracn1, pll1vco = 0;

  pllsource = __HAL_RCC_GET_PLL12_SOURCE();
  pll1m = ((RCC->PLL1CFGR1 & RCC_PLL1CFGR1_DIVM1) >> RCC_PLL1CFGR1_DIVM1_Pos) + 1U;
  pll1fracen = (RCC->PLL1FRACR & RCC_PLL1FRACR_FRACLE) >> RCC_PLL1FRACR_FRACLE_Pos;
  fracn1 = (pll1fracen * ((RCC->PLL1FRACR & RCC_PLL1FRACR_FRACV) >> RCC_PLL1FRACR_FRACV_Pos));
  pll1vco = (((RCC->PLL1CFGR1 & RCC_PLL1CFGR1_DIVN) + 1U) + (fracn1 / 0x1FFF));//Intermediary value
  switch (pllsource)
  {
    case RCC_PLL12SOURCE_HSI:  /* HSI used as PLL clock source */

      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) != 0U)
      {
        hsivalue = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
        pll1vco *= (hsivalue / pll1m);
      }
      else
      {
        pll1vco *= (HSI_VALUE / pll1m);
      }
      break;

    case RCC_PLL12SOURCE_HSE:  /* HSE used as PLL clock source */
      pll1vco *= (HSE_VALUE / pll1m);
      break;

    case RCC_PLL12SOURCE_OFF:  /* No clock source for PLL */
      pll1vco = 0;
      break;

    default:
      pll1vco = 0;
      break;
  }

  PLL1_Clocks->PLL1_P_Frequency = (uint32_t)(pll1vco / (((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVP) >> RCC_PLL1CFGR2_DIVP_Pos) + 1U));
  PLL1_Clocks->PLL1_Q_Frequency = (uint32_t)(pll1vco / (((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVQ) >> RCC_PLL1CFGR2_DIVQ_Pos) + 1U));
  PLL1_Clocks->PLL1_R_Frequency = (uint32_t)(pll1vco / (((RCC->PLL1CFGR2 & RCC_PLL1CFGR2_DIVR) >> RCC_PLL1CFGR2_DIVR_Pos) + 1U));
}


/**
* @brief  Returns the PLL2 clock frequencies :PLL2_P_Frequency,PLL2_R_Frequency and PLL2_Q_Frequency
  * @note   The PLL2 clock frequencies computed by this function is not the real
  *         frequency in the chip. It is calculated based on the predefined
  *         constant and the selected clock source:
  * @note     The function returns values based on HSE_VALUE or HSI_VALUE Value multiplied/divided by the PLL factors.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  *
  * @note   Each time PLL2CLK changes, this function must be called to update the
  *         right PLL2CLK value. Otherwise, any configuration based on this function will be incorrect.
  * @param  PLL2_Clocks structure.
  * @retval None
  */
__weak void HAL_RCC_GetPLL2ClockFreq(PLL2_ClocksTypeDef *PLL2_Clocks)
{
  uint32_t   pllsource = 0U, pll2m = 1U, pll2fracen = 0U, hsivalue = 0U;
  float fracn1, pll2vco = 0;

  pllsource = __HAL_RCC_GET_PLL12_SOURCE();
  pll2m = ((RCC->PLL2CFGR1 & RCC_PLL2CFGR1_DIVM2) >> RCC_PLL2CFGR1_DIVM2_Pos) + 1U;
  pll2fracen = (RCC->PLL2FRACR & RCC_PLL2FRACR_FRACLE) >> RCC_PLL2FRACR_FRACLE_Pos;
  fracn1 = (pll2fracen * ((RCC->PLL2FRACR & RCC_PLL2FRACR_FRACV) >> RCC_PLL2FRACR_FRACV_Pos));
  pll2vco = (((RCC->PLL2CFGR1 & RCC_PLL2CFGR1_DIVN) + 1U) + (fracn1 / 0x1FFF)); //Intermediary value
  switch (pllsource)
  {
    case RCC_PLL12SOURCE_HSI:  /* HSI used as PLL clock source */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) != 0U)
      {
        hsivalue = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
        pll2vco *= (hsivalue / pll2m);
      }
      else
      {
        pll2vco *= (HSI_VALUE / pll2m);
      }
      break;

    case RCC_PLL12SOURCE_HSE:  /* HSE used as PLL clock source */
      pll2vco *= (HSE_VALUE / pll2m);
      break;

    case RCC_PLL12SOURCE_OFF:  /* No clock source for PLL */
      pll2vco = 0;
      break;

    default:
      pll2vco = 0;
      break;
  }

  PLL2_Clocks->PLL2_P_Frequency = (uint32_t)(pll2vco / (((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVP) >> RCC_PLL2CFGR2_DIVP_Pos) + 1U));
  PLL2_Clocks->PLL2_Q_Frequency = (uint32_t)(pll2vco / (((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVQ) >> RCC_PLL2CFGR2_DIVQ_Pos) + 1U));
  PLL2_Clocks->PLL2_R_Frequency = (uint32_t)(pll2vco / (((RCC->PLL2CFGR2 & RCC_PLL2CFGR2_DIVR) >> RCC_PLL2CFGR2_DIVR_Pos) + 1U));
}


/**
* @brief  Returns the PLL3 clock frequencies :PLL3_P_Frequency,PLL3_R_Frequency and PLL3_Q_Frequency
  * @note   The PLL3 clock frequencies computed by this function is not the real
  *         frequency in the chip. It is calculated based on the predefined
  *         constant and the selected clock source:
  * @note     The function returns values based on HSE_VALUE, HSI_VALUE or CSI Value multiplied/divided by the PLL factors.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  *
  * @note   Each time PLL3CLK changes, this function must be called to update the
  *         right PLL3CLK value. Otherwise, any configuration based on this function will be incorrect.
  * @param  PLL3_Clocks structure.
  * @retval None
  */
__weak void HAL_RCC_GetPLL3ClockFreq(PLL3_ClocksTypeDef *PLL3_Clocks)
{
  uint32_t   pllsource = 0, pll3m = 1, pll3fracen = 0, hsivalue = 0;
  float fracn1, pll3vco = 0;

  pllsource = __HAL_RCC_GET_PLL3_SOURCE();
  pll3m = ((RCC->PLL3CFGR1 & RCC_PLL3CFGR1_DIVM3) >> RCC_PLL3CFGR1_DIVM3_Pos) + 1U;
  pll3fracen = (RCC->PLL3FRACR & RCC_PLL3FRACR_FRACLE) >> RCC_PLL3FRACR_FRACLE_Pos;
  fracn1 = (pll3fracen * ((RCC->PLL3FRACR & RCC_PLL3FRACR_FRACV) >> RCC_PLL3FRACR_FRACV_Pos));
  pll3vco = (((RCC->PLL3CFGR1 & RCC_PLL3CFGR1_DIVN) + 1U) + (fracn1 / 0x1FFF)); //Intermediary value
  switch (pllsource)
  {
    case RCC_PLL3SOURCE_HSI:  /* HSI used as PLL clock source */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) != 0U)
      {
        hsivalue = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
        pll3vco *= (hsivalue / pll3m);
      }
      else
      {
        pll3vco *= (HSI_VALUE / pll3m);
      }
      break;

    case RCC_PLL3SOURCE_HSE:  /* HSE used as PLL clock source */
      pll3vco *= (HSE_VALUE / pll3m);
      break;



    case RCC_PLL3SOURCE_CSI:  /* CSI used as PLL clock source */
      pll3vco *= (CSI_VALUE / pll3m);
      break;

    case RCC_PLL3SOURCE_OFF:  /* No clock source for PLL */
      pll3vco = 0;
      break;
  }

  PLL3_Clocks->PLL3_P_Frequency = (uint32_t)(pll3vco / (((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVP) >> RCC_PLL3CFGR2_DIVP_Pos) + 1U));
  PLL3_Clocks->PLL3_Q_Frequency = (uint32_t)(pll3vco / (((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVQ) >> RCC_PLL3CFGR2_DIVQ_Pos) + 1U));
  PLL3_Clocks->PLL3_R_Frequency = (uint32_t)(pll3vco / (((RCC->PLL3CFGR2 & RCC_PLL3CFGR2_DIVR) >> RCC_PLL3CFGR2_DIVR_Pos) + 1U));
}


/**
* @brief  Returns the PLL4 clock frequencies :PLL4_P_Frequency,PLL4_R_Frequency and PLL4_Q_Frequency
  * @note   The PLL4 clock frequencies computed by this function is not the real
  *         frequency in the chip. It is calculated based on the predefined
  *         constant and the selected clock source:
  * @note     The function returns values based on HSE_VALUE, HSI_VALUE or CSI Value multiplied/divided by the PLL factors.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  *
  * @note   Each time PLL4CLK changes, this function must be called to update the
  *         right PLL4CLK value. Otherwise, any configuration based on this function will be incorrect.
  * @param  PLL4_Clocks structure.
  * @retval None
  */
__weak void HAL_RCC_GetPLL4ClockFreq(PLL4_ClocksTypeDef *PLL4_Clocks)
{
  uint32_t   pllsource = 0U, pll4m = 1U, pll4fracen = 0U, hsivalue = 0U;
  float fracn1, pll4vco = 0;

  pllsource = __HAL_RCC_GET_PLL4_SOURCE();
  pll4m = ((RCC->PLL4CFGR1 & RCC_PLL4CFGR1_DIVM4) >> RCC_PLL4CFGR1_DIVM4_Pos) + 1U;
  pll4fracen = (RCC->PLL4FRACR & RCC_PLL4FRACR_FRACLE) >> RCC_PLL4FRACR_FRACLE_Pos;
  fracn1 = (pll4fracen * ((RCC->PLL4FRACR & RCC_PLL4FRACR_FRACV) >> RCC_PLL4FRACR_FRACV_Pos));
  pll4vco = (((RCC->PLL4CFGR1 & RCC_PLL4CFGR1_DIVN) + 1U) + (fracn1 / 0x1FFF)); //Intermediary value
  switch (pllsource)
  {
    case RCC_PLL4SOURCE_HSI:  /* HSI used as PLL clock source */
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) != 0U)
      {
        hsivalue = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
        pll4vco *= (hsivalue / pll4m);
      }
      else
      {
        pll4vco *= (HSI_VALUE / pll4m);
      }
      break;

    case RCC_PLL4SOURCE_HSE:  /* HSE used as PLL clock source */
      pll4vco *= (HSE_VALUE / pll4m);
      break;

    case RCC_PLL4SOURCE_CSI:  /* CSI used as PLL clock source */
      pll4vco *= (CSI_VALUE / pll4m);
      break;

    case RCC_PLL4SOURCE_I2S_CKIN:  /* Signal I2S_CKIN used as reference clock */
      pll4vco *= (EXTERNAL_CLOCK_VALUE / pll4m);
      break;
  }

  PLL4_Clocks->PLL4_P_Frequency = (uint32_t)(pll4vco / (((RCC->PLL4CFGR2 & RCC_PLL4CFGR2_DIVP) >> RCC_PLL4CFGR2_DIVP_Pos) + 1U));
  PLL4_Clocks->PLL4_Q_Frequency = (uint32_t)(pll4vco / (((RCC->PLL4CFGR2 & RCC_PLL4CFGR2_DIVQ) >> RCC_PLL4CFGR2_DIVQ_Pos) + 1U));
  PLL4_Clocks->PLL4_R_Frequency = (uint32_t)(pll4vco / (((RCC->PLL4CFGR2 & RCC_PLL4CFGR2_DIVR) >> RCC_PLL4CFGR2_DIVR_Pos) + 1U));
}

/**
  * @brief  Returns the PCLK1 frequency
  * @note   Each time PCLK1 changes, this function must be called to update the
  *         right PCLK1 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK1 frequency
  */
uint32_t HAL_RCC_GetPCLK1Freq(void)
{
  uint32_t apb1div = 0;

  /* Compute PCLK1 frequency ---------------------------*/
  apb1div = __HAL_RCC_GET_APB1_DIV();
  if (apb1div > RCC_APB1_DIV16)
  {
    apb1div = RCC_APB1_DIV16;
  }

  return (HAL_RCC_GetMCUFreq() >> apb1div);
}


/**
  * @brief  Returns the PCLK2 frequency
  * @note   Each time PCLK2 changes, this function must be called to update the
  *         right PCLK2 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK2 frequency
  */
uint32_t HAL_RCC_GetPCLK2Freq(void)
{
  uint32_t apb2div = 0;

  /* Compute PCLK2 frequency ---------------------------*/
  apb2div = __HAL_RCC_GET_APB2_DIV();
  if (apb2div > RCC_APB2_DIV16)
  {
    apb2div = RCC_APB2_DIV16;
  }

  return (HAL_RCC_GetMCUFreq() >> apb2div);
}

/**
  * @brief  Returns the PCLK3 frequency
  * @note   Each time PCLK3 changes, this function must be called to update the
  *         right PCLK3 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK3 frequency
  */
uint32_t HAL_RCC_GetPCLK3Freq(void)
{
  uint32_t apb3div = 0;

  /* Compute PCLK3 frequency ---------------------------*/
  apb3div = __HAL_RCC_GET_APB3_DIV();
  if (apb3div > RCC_APB3_DIV16)
  {
    apb3div = RCC_APB3_DIV16;
  }

  return (HAL_RCC_GetMCUFreq() >> apb3div);
}

/**
  * @brief  Returns the PCLK4 frequency
  * @note   Each time PCLK4 changes, this function must be called to update the
  *         right PCLK4 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK4 frequency
  */
uint32_t HAL_RCC_GetPCLK4Freq(void)
{
  uint32_t apb4div = 0;

  /* Compute PCLK4 frequency ---------------------------*/
  apb4div = __HAL_RCC_GET_APB4_DIV();
  if (apb4div > RCC_APB4_DIV16)
  {
    apb4div = RCC_APB4_DIV16;
  }

  return (HAL_RCC_GetACLKFreq() >> apb4div);
}

/**
  * @brief  Returns the PCLK5 frequency
  * @note   Each time PCLK5 changes, this function must be called to update the
  *         right PCLK5 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK5 frequency
  */
uint32_t HAL_RCC_GetPCLK5Freq(void)
{
  uint32_t apb5div = 0;

  /* Compute PCLK5 frequency ---------------------------*/
  apb5div = __HAL_RCC_GET_APB5_DIV();
  if (apb5div > RCC_APB5_DIV16)
  {
    apb5div = RCC_APB5_DIV16;
  }

  return (HAL_RCC_GetACLKFreq() >> apb5div);
}

/**
  * @brief  Returns the ACLK frequency
  * @note   Each time ACLK changes, this function must be called to update the
  *         right ACLK value. Otherwise, any configuration based on this function will be incorrect.
  * @retval ACLK frequency
  */
uint32_t HAL_RCC_GetACLKFreq(void)
{
  uint32_t axidiv = 0;

  /* Compute ACLK frequency ---------------------------*/
  axidiv = __HAL_RCC_GET_AXI_DIV();
  if (axidiv > RCC_AXI_DIV4)
  {
    axidiv = RCC_AXI_DIV4;
  }
  axidiv += 1;

  return HAL_RCC_GetAXISSFreq() / axidiv;
}

/**
  * @brief  Returns the HCLK5 frequency
  * @note   Each time HCLK5 changes, this function must be called to update the
  *         right HCLK5 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK5 frequency
  */
uint32_t HAL_RCC_GetHCLK5Freq(void)
{
  return HAL_RCC_GetACLKFreq();
}

/**
  * @brief  Returns the HCLK6 frequency
  * @note   Each time HCLK6 changes, this function must be called to update the
  *         right HCLK6 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK6 frequency
  */
uint32_t HAL_RCC_GetHCLK6Freq(void)
{
  return HAL_RCC_GetACLKFreq();
}

/**
  * @brief  Returns the HCLK1 frequency
  * @note   Each time HCLK1 changes, this function must be called to update the
  *         right HCLK1 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK1 frequency
  */
uint32_t HAL_RCC_GetHCLK1Freq(void)
{
  return HAL_RCC_GetMCUFreq();
}

/**
  * @brief  Returns the HCLK2 frequency
  * @note   Each time HCLK1 changes, this function must be called to update the
  *         right HCLK1 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK2 frequency
  */
uint32_t HAL_RCC_GetHCLK2Freq(void)
{
  return HAL_RCC_GetMCUFreq();
}

/**
  * @brief  Returns the HCLK3 frequency
  * @note   Each time HCLK3 changes, this function must be called to update the
  *         right HCLK3 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK3 frequency
  */
uint32_t HAL_RCC_GetHCLK3Freq(void)
{
  return HAL_RCC_GetMCUFreq();
}

/**
  * @brief  Returns the HCLK4 frequency
  * @note   Each time HCLK4 changes, this function must be called to update the
  *         right HCLK4 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK4 frequency
  */
uint32_t HAL_RCC_GetHCLK4Freq(void)
{
  return HAL_RCC_GetMCUFreq();
}

/**
  * @brief  Returns the MLHCLK frequency
  * @note   Each time MLHCLK changes, this function must be called to update the
  *         right MLHCLK value. Otherwise, any configuration based on this function will be incorrect.
  * @retval HCLK4 frequency
  */
uint32_t HAL_RCC_GetMLHCLKFreq(void)
{
  return HAL_RCC_GetMCUFreq();
}


/**
  * @brief  Returns the MCU frequency
  * @note   Each time MCU changes, this function must be called to update the
  *         right MCU value. Otherwise, any configuration based on this function will be incorrect.
  * @retval MCU frequency
  */
uint32_t HAL_RCC_GetMCUFreq(void)
{
  uint32_t mcudiv = 0;

  /* Compute MCU frequency ---------------------------*/
  mcudiv = __HAL_RCC_GET_MCU_DIV();
  if (mcudiv > RCC_MCU_DIV512)
  {
    mcudiv = RCC_MCU_DIV512;
  }

  return HAL_RCC_GetMCUSSFreq() >> mcudiv;
}


/**
  * @brief  Returns the FCLK frequency
  * @note   Each time FCLK changes, this function must be called to update the
  *         right FCLK value. Otherwise, any configuration based on this function will be incorrect.
  * @retval FCLK frequency
  */
uint32_t HAL_RCC_GetFCLKFreq(void)
{
  return HAL_RCC_GetMCUFreq();
}

/**
  * @brief  Returns the CKPER frequency
  * @note   Each time CKPER changes, this function must be called to update the
  *         right CKPER value. Otherwise, any configuration based on this function will be incorrect.
  * @retval CKPER frequency
  */
uint32_t RCC_GetCKPERFreq(void)
{
  uint32_t ckperclocksource = 0, frequency = 0;

  ckperclocksource = __HAL_RCC_GET_CKPER_SOURCE();

  if (ckperclocksource == RCC_CKPERCLKSOURCE_HSI)
  {
    /* In Case the main PLL Source is HSI */
    frequency = HSI_VALUE;
  }

  else if (ckperclocksource == RCC_CKPERCLKSOURCE_CSI)
  {
    /* In Case the main PLL Source is CSI */
    frequency = CSI_VALUE;
  }

  else if (ckperclocksource == RCC_CKPERCLKSOURCE_HSE)
  {
    /* In Case the main PLL Source is HSE */
    frequency = HSE_VALUE;
  }
  else
  {
    frequency = 0;
  }

  return frequency;
}

/**
  * @brief  Returns the System Core frequency
  *
  * @note   The system frequency computed by this function is not the real
  *         frequency in the chip. It is calculated based on the predefined
  *         constants and the selected clock source
  * @retval System Core frequency
  */
uint32_t HAL_RCC_GetSystemCoreClockFreq(void)
{
#ifdef CORE_CA7
  return HAL_RCC_GetMPUSSFreq();
#else /* CORE_CM4 */
  return HAL_RCC_GetMCUFreq();
#endif
}

uint32_t HAL_RCC_GetMPUSSFreq()
{
  uint32_t mpussfreq = 0, mpudiv = 0;
  PLL1_ClocksTypeDef pll1_clocks;

  switch (__HAL_RCC_GET_MPU_SOURCE())
  {
    case RCC_MPUSOURCE_PLL1:
      HAL_RCC_GetPLL1ClockFreq(&pll1_clocks);
      mpussfreq = pll1_clocks.PLL1_P_Frequency;
      break;

    case RCC_MPUSOURCE_MPUDIV:
      mpudiv = __HAL_RCC_GET_MPU_DIV();
      if (mpudiv == RCC_MPU_DIV_OFF)
      {
        mpussfreq = 0;
      }
      else if (mpudiv <= RCC_MPU_DIV16)
      {
        HAL_RCC_GetPLL1ClockFreq(&pll1_clocks);
        mpussfreq = (pll1_clocks.PLL1_P_Frequency >> mpudiv);
      }
      else
      {
        HAL_RCC_GetPLL1ClockFreq(&pll1_clocks);
        mpussfreq = (pll1_clocks.PLL1_P_Frequency >> 16); /* divided by 16 */
      }
      break;

    case RCC_MPUSOURCE_HSI:
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) != 0U)
      {
        mpussfreq = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
      }
      else
      {
        mpussfreq = HSI_VALUE;
      }
      break;

    case RCC_MPUSOURCE_HSE:
      mpussfreq = HSE_VALUE;
      break;
  }

  return mpussfreq;
}


uint32_t HAL_RCC_GetAXISSFreq()
{
  uint32_t axissfreq = 0;
  PLL2_ClocksTypeDef pll2_clocks;

  switch (__HAL_RCC_GET_AXIS_SOURCE())
  {
    case RCC_AXISSOURCE_PLL2:
      HAL_RCC_GetPLL2ClockFreq(&pll2_clocks);
      axissfreq = pll2_clocks.PLL2_P_Frequency;
      break;

    case RCC_AXISSOURCE_HSI:
      if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIDIVRDY) != 0U)
      {
        axissfreq = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
      }
      else
      {
        axissfreq = HSI_VALUE;
      }
      break;

    case RCC_AXISSOURCE_HSE:
      axissfreq = HSE_VALUE;
      break;

    case RCC_AXISSOURCE_OFF:
    default:
      axissfreq = 0; /* ck_axiss is gated */
      break;
  }

  return axissfreq;
}

uint32_t HAL_RCC_GetMCUSSFreq()
{
  uint32_t mcussfreq = 0;
  PLL3_ClocksTypeDef pll3_clocks;

  switch (__HAL_RCC_GET_MCU_SOURCE())
  {
    case RCC_MCUSSOURCE_PLL3:
      HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
      mcussfreq = pll3_clocks.PLL3_P_Frequency;
      break;

    case RCC_MCUSSOURCE_HSI:
      mcussfreq = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());

      break;

    case RCC_MCUSSOURCE_HSE:
      mcussfreq = HSE_VALUE;
      break;

    case RCC_MCUSSOURCE_CSI:
      mcussfreq = CSI_VALUE;
      break;
  }

  return mcussfreq;
}



/**
  * @brief This function handles the RCC global interrupt (rcc_mcu_irq/rcc_mpu_irq)
  * @note This API should be called under the RCC_Handler().
  * @retval None
  */
void HAL_RCC_IRQHandler(void)
{
#if defined(CORE_CM4)
  uint32_t flags = (READ_REG(RCC->MC_CIFR) & RCC_IT_ALL);
#endif /* CORE_CM4 */
#if defined(CORE_CA7)
  uint32_t flags = (READ_REG(RCC->MP_CIFR) & RCC_IT_ALL);
#endif
  /* Clear the RCC interrupt bits */
  __HAL_RCC_CLEAR_IT(flags);

  if ((flags & RCC_IT_LSECSS) != RESET)
  {
    /* Enable write access to Backup domain */
    SET_BIT(PWR->CR1, PWR_CR1_DBP);

    /* Clear LSECSSON bit */
    HAL_RCCEx_DisableLSECSS();
    /* Clear LSEON bit */
    CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSEON);
  }

  /* RCC interrupt user callback */
  HAL_RCC_Callback(flags);
}


/**
  * @brief  RCC global interrupt callback
  * @param  Flags: It contains the flags which were set during the RCC_IRQHandler
  *                before cleaning them
  * @retval None
  */
__weak void HAL_RCC_Callback(uint32_t Flags)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_RCC_Callback could be implemented in the user file
  */
}


/**
  * @brief This function handles the RCC Wake up interrupt (rcc_mcu_wkup_irq/rcc_mpu_wkup_irq)
  * @note This API should be called under the RCC_WAKEUP_Handler().
  * @retval None
  */
void HAL_RCC_WAKEUP_IRQHandler(void)
{
  /* Check RCC WKUP flag is set */
  if (__HAL_RCC_GET_IT(RCC_IT_WKUP) != RESET)
  {
    /* Clear the RCC WKUP flag bit */
    __HAL_RCC_CLEAR_IT(RCC_IT_WKUP);

    /* RCC WKUP interrupt user callback */
    HAL_RCC_WAKEUP_Callback();
  }
}


/**
  * @brief  RCC WAKEUP interrupt callback
  * @retval None
  */
__weak void HAL_RCC_WAKEUP_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_RCC_WAKEUP_Callback could be implemented in the user file
  */
}


/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_RCC_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
