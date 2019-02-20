/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @brief   Extended RCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities RCC extension peripheral:
  *           + Extended Peripheral Control functions
  *
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

/** @defgroup RCCEx RCCEx
  * @brief RCC HAL module driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
#define LSE_MASK (RCC_BDCR_LSEON | RCC_BDCR_LSEBYP | RCC_BDCR_DIGBYP)
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCCEx_Private_Function_Prototypes RCCEx Private Functions Prototypes
  * @{
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Functions RCCEx Exported Functions
  * @{
  */

/** @defgroup RCCEx_Exported_Functions_Group1 Extended Peripheral Control functions
 *  @brief  Extended Peripheral Control functions
 *
@verbatim
 ===============================================================================
                ##### Extended Peripheral Control functions  #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to control the RCC Clocks
    frequencies.
    [..]
    (@) Important note: Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to
        select the RTC clock source; in this case the Backup domain will be reset in
        order to modify the RTC Clock source, as consequence RTC registers (including
        the backup registers) and RCC_BDCR register are set to their reset values.

@endverbatim
  * @{
  */
/**
  * @brief  Configures PLL2
  * @param  pll2: pointer to an RCC_PLLInitTypeDef structure
  *
  * @retval HAL status
  */

HAL_StatusTypeDef RCCEx_PLL2_Config(RCC_PLLInitTypeDef *pll2)
{
  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RCC_PLL(pll2->PLLState));
  if ((pll2->PLLState) != RCC_PLL_NONE)
  {
    /* Check if the PLL is used as system clock or not (MPU, MCU, AXISS)*/
    if (!__IS_PLL2_IN_USE()) /* If not used then */
    {
      if ((pll2->PLLState) == RCC_PLL_ON)
      {
        /* Check the parameters */
        assert_param(IS_RCC_PLLMODE(pll2->PLLMODE));
        assert_param(IS_RCC_PLL12SOURCE(pll2->PLLSource));
        assert_param(IS_RCC_PLLM2_VALUE(pll2->PLLM));
        if (pll2->PLLMODE == RCC_PLL_FRACTIONAL)
        {
          assert_param(IS_RCC_PLLN2_FRAC_VALUE(pll2->PLLN));
        }
        else
        {
          assert_param(IS_RCC_PLLN2_INT_VALUE(pll2->PLLN));
        }
        assert_param(IS_RCC_PLLP2_VALUE(pll2->PLLP));
        assert_param(IS_RCC_PLLQ2_VALUE(pll2->PLLQ));
        assert_param(IS_RCC_PLLR2_VALUE(pll2->PLLR));

        /* Check that PLL2 OSC clock source is already set */
        if ((__HAL_RCC_GET_PLL12_SOURCE() != RCC_PLL12SOURCE_HSI) &&
            (__HAL_RCC_GET_PLL12_SOURCE() != RCC_PLL12SOURCE_HSE))
        {
          return HAL_ERROR;
        }

        /*Disable the post-dividers*/
        __HAL_RCC_PLL2CLKOUT_DISABLE(RCC_PLL2_DIVP | RCC_PLL2_DIVQ | RCC_PLL2_DIVR);
        /* Disable the main PLL. */
        __HAL_RCC_PLL2_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY) != RESET)
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
        if (__IS_PLL1_IN_USE())
        {
          if (pll2->PLLSource != __HAL_RCC_GET_PLL12_SOURCE())
          {
            return HAL_ERROR;
          }
        }
        else
        {
          /* Configure PLL1 and PLL2 clock source */
          __HAL_RCC_PLL12_SOURCE(pll2->PLLSource);
        }

        /* Configure the PLL2 multiplication and division factors. */
        __HAL_RCC_PLL2_CONFIG(
          pll2->PLLM,
          pll2->PLLN,
          pll2->PLLP,
          pll2->PLLQ,
          pll2->PLLR);


        /* Configure the Fractional Divider */
        __HAL_RCC_PLL2FRACV_DISABLE(); //Set FRACLE to ‘0’
        /* In integer or clock spreading mode the application shall ensure that a 0 is loaded into the SDM */
        if ((pll2->PLLMODE == RCC_PLL_SPREAD_SPECTRUM) || (pll2->PLLMODE == RCC_PLL_INTEGER))
        {
          /* Do not use the fractional divider */
          __HAL_RCC_PLL2FRACV_CONFIG(0); //Set FRACV to '0'
        }
        else
        {
          /* Configure PLL  PLL2FRACV  in fractional mode*/
          __HAL_RCC_PLL2FRACV_CONFIG(pll2->PLLFRACV);
        }
        __HAL_RCC_PLL2FRACV_ENABLE(); //Set FRACLE to ‘1’


        /* Configure the Spread Control */
        if (pll2->PLLMODE == RCC_PLL_SPREAD_SPECTRUM)
        {
          assert_param(IS_RCC_INC_STEP(pll2->INC_STEP));
          assert_param(IS_RCC_SSCG_MODE(pll2->SSCG_MODE));
          assert_param(IS_RCC_RPDFN_DIS(pll2->RPDFN_DIS));
          assert_param(IS_RCC_TPDFN_DIS(pll2->TPDFN_DIS));
          assert_param(IS_RCC_MOD_PER(pll2->MOD_PER));

          __HAL_RCC_PLL2CSGCONFIG(pll2->MOD_PER, pll2->TPDFN_DIS, pll2->RPDFN_DIS,
                                  pll2->SSCG_MODE, pll2->INC_STEP);
          __HAL_RCC_PLL2_SSMODE_ENABLE();
        }
        else
        {
          __HAL_RCC_PLL2_SSMODE_DISABLE();
        }


        /* Enable the PLL2. */
        __HAL_RCC_PLL2_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /*Enable the post-dividers*/
        __HAL_RCC_PLL2CLKOUT_ENABLE(RCC_PLL2_DIVP | RCC_PLL2_DIVQ | RCC_PLL2_DIVR);
      }
      else
      {
        /*Disable the post-dividers*/
        __HAL_RCC_PLL2CLKOUT_DISABLE(RCC_PLL2_DIVP | RCC_PLL2_DIVQ | RCC_PLL2_DIVR);
        /* Disable the PLL2. */
        __HAL_RCC_PLL2_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY) != RESET)
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
  * @brief  Configures PLL3
  * @param  pll3: pointer to a RCC_PLLInitTypeDef structure
  *
  * @retval HAL status
  */
HAL_StatusTypeDef RCCEx_PLL3_Config(RCC_PLLInitTypeDef *pll3)
{

  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RCC_PLL(pll3->PLLState));
  if ((pll3->PLLState) != RCC_PLL_NONE)
  {
    /* Check if the PLL is used as system clock or not (MPU, MCU, AXISS)*/
    if (!__IS_PLL3_IN_USE()) /* If not used then*/
    {
      if ((pll3->PLLState) == RCC_PLL_ON)
      {
        /* Check the parameters */
        assert_param(IS_RCC_PLLMODE(pll3->PLLMODE));
        assert_param(IS_RCC_PLL3SOURCE(pll3->PLLSource));
        assert_param(IS_RCC_PLLM1_VALUE(pll3->PLLM));
        if (pll3->PLLMODE == RCC_PLL_FRACTIONAL)
        {
          assert_param(IS_RCC_PLLN3_FRAC_VALUE(pll3->PLLN));
        }
        else
        {
          assert_param(IS_RCC_PLLN3_INT_VALUE(pll3->PLLN));
        }
        assert_param(IS_RCC_PLLP3_VALUE(pll3->PLLP));
        assert_param(IS_RCC_PLLQ3_VALUE(pll3->PLLQ));
        assert_param(IS_RCC_PLLR3_VALUE(pll3->PLLR));

        /*Disable the post-dividers*/
        __HAL_RCC_PLL3CLKOUT_DISABLE(RCC_PLL3_DIVP | RCC_PLL3_DIVQ | RCC_PLL3_DIVR);
        /* Disable the main PLL. */
        __HAL_RCC_PLL3_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) != RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }


        /*The PLL configuration below must be done before enabling the PLL:
        -Selection of PLL clock entry (HSI or CSI or HSE)
        -Frequency input range (PLLxRGE)
        -Division factors (DIVMx, DIVNx, DIVPx, DIVQx & DIVRx)
        Once the PLL is enabled, these parameters can not be changed.
        If the User wants to change the PLL parameters he must disable the concerned PLL (PLLxON=0) and wait for the
        PLLxRDY flag to be at 0.
        The PLL configuration below can be done at any time:
        -Enable/Disable of output clock dividers (DIVPxEN, DIVQxEN & DIVRxEN)
        -Fractional Division Enable (PLLxFRACNEN)
        -Fractional Division factor (FRACNx)*/

        /* Configure PLL3 clock source */
        __HAL_RCC_PLL3_SOURCE(pll3->PLLSource);

        /* Wait till PLL SOURCE is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3SRCRDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Select PLL3 input reference frequency range */
        __HAL_RCC_PLL3_IFRANGE(pll3->PLLRGE) ;

        /* Configure the PLL3 multiplication and division factors. */
        __HAL_RCC_PLL3_CONFIG(
          pll3->PLLM,
          pll3->PLLN,
          pll3->PLLP,
          pll3->PLLQ,
          pll3->PLLR);

        /* Configure the Fractional Divider */
        __HAL_RCC_PLL3FRACV_DISABLE(); //Set FRACLE to ‘0’
        /* In integer or clock spreading mode the application shall ensure that a 0 is loaded into the SDM */
        if ((pll3->PLLMODE == RCC_PLL_SPREAD_SPECTRUM) || (pll3->PLLMODE == RCC_PLL_INTEGER))
        {
          /* Do not use the fractional divider */
          __HAL_RCC_PLL3FRACV_CONFIG(0); //Set FRACV to '0'
        }
        else
        {
          /* Configure PLL  PLL3FRACV  in fractional mode*/
          __HAL_RCC_PLL3FRACV_CONFIG(pll3->PLLFRACV);
        }
        __HAL_RCC_PLL3FRACV_ENABLE(); //Set FRACLE to ‘1’


        /* Configure the Spread Control */
        if (pll3->PLLMODE == RCC_PLL_SPREAD_SPECTRUM)
        {
          assert_param(IS_RCC_INC_STEP(pll3->INC_STEP));
          assert_param(IS_RCC_SSCG_MODE(pll3->SSCG_MODE));
          assert_param(IS_RCC_RPDFN_DIS(pll3->RPDFN_DIS));
          assert_param(IS_RCC_TPDFN_DIS(pll3->TPDFN_DIS));
          assert_param(IS_RCC_MOD_PER(pll3->MOD_PER));

          __HAL_RCC_PLL3CSGCONFIG(pll3->MOD_PER, pll3->TPDFN_DIS, pll3->RPDFN_DIS,
                                  pll3->SSCG_MODE, pll3->INC_STEP);
          __HAL_RCC_PLL3_SSMODE_ENABLE();
        }
        else
        {
          __HAL_RCC_PLL3_SSMODE_DISABLE();
        }


        /* Enable the PLL3. */
        __HAL_RCC_PLL3_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) == RESET)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /* Enable the post-dividers */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVP | RCC_PLL3_DIVQ | RCC_PLL3_DIVR);
      }
      else
      {
        /*Disable the post-dividers*/
        __HAL_RCC_PLL3CLKOUT_DISABLE(RCC_PLL3_DIVP | RCC_PLL3_DIVQ | RCC_PLL3_DIVR);
        /* Disable the PLL3. */
        __HAL_RCC_PLL3_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) != RESET)
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
  * @brief  Configures PLL4
  * @param  pll4: pointer to a RCC_PLLInitTypeDef structure
  *
  * @retval HAL status
  */
HAL_StatusTypeDef RCCEx_PLL4_Config(RCC_PLLInitTypeDef *pll4)
{

  uint32_t tickstart;

  /* Check the parameters */
  assert_param(IS_RCC_PLL(pll4->PLLState));
  if ((pll4->PLLState) != RCC_PLL_NONE)
  {

    if ((pll4->PLLState) == RCC_PLL_ON)
    {
      /* Check the parameters */
      assert_param(IS_RCC_PLLMODE(pll4->PLLMODE));
      assert_param(IS_RCC_PLL4SOURCE(pll4->PLLSource));
      assert_param(IS_RCC_PLLM4_VALUE(pll4->PLLM));
      if (pll4->PLLMODE == RCC_PLL_FRACTIONAL)
      {
        assert_param(IS_RCC_PLLN4_FRAC_VALUE(pll4->PLLN));
      }
      else
      {
        assert_param(IS_RCC_PLLN4_INT_VALUE(pll4->PLLN));
      }
      assert_param(IS_RCC_PLLP4_VALUE(pll4->PLLP));
      assert_param(IS_RCC_PLLQ4_VALUE(pll4->PLLQ));
      assert_param(IS_RCC_PLLR4_VALUE(pll4->PLLR));

      /*Disable the post-dividers*/
      __HAL_RCC_PLL4CLKOUT_DISABLE(RCC_PLL4_DIVP | RCC_PLL4_DIVQ | RCC_PLL4_DIVR);
      /* Disable the main PLL. */
      __HAL_RCC_PLL4_DISABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till PLL is ready */
      while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY) != RESET)
      {
        if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }


      /*The PLL configuration below must be done before enabling the PLL:
      -Selection of PLL clock entry (HSI or CSI or HSE)
      -Frequency input range (PLLxRGE)
      -Division factors (DIVMx, DIVNx, DIVPx, DIVQx & DIVRx)
      Once the PLL is enabled, these parameters can not be changed.
      If the User wants to change the PLL parameters he must disable the concerned PLL (PLLxON=0) and wait for the
      PLLxRDY flag to be at 0.
      The PLL configuration below can be done at any time:
      -Enable/Disable of output clock dividers (DIVPxEN, DIVQxEN & DIVRxEN)
      -Fractional Division Enable (PLLxFRACNEN)
      -Fractional Division factor (FRACNx)*/

      /* Configure PLL4 and PLL4 clock source */
      __HAL_RCC_PLL4_SOURCE(pll4->PLLSource);

      /* Wait till PLL SOURCE is ready */
      while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL4SRCRDY) == RESET)
      {
        if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }

      /* Select PLL4 input reference frequency range */
      __HAL_RCC_PLL4_IFRANGE(pll4->PLLRGE) ;

      /* Configure the PLL4 multiplication and division factors. */
      __HAL_RCC_PLL4_CONFIG(
        pll4->PLLM,
        pll4->PLLN,
        pll4->PLLP,
        pll4->PLLQ,
        pll4->PLLR);

      /* Configure the Fractional Divider */
      __HAL_RCC_PLL4FRACV_DISABLE(); //Set FRACLE to ‘0’
      /* In integer or clock spreading mode the application shall ensure that a 0 is loaded into the SDM */
      if ((pll4->PLLMODE == RCC_PLL_SPREAD_SPECTRUM) || (pll4->PLLMODE == RCC_PLL_INTEGER))
      {
        /* Do not use the fractional divider */
        __HAL_RCC_PLL4FRACV_CONFIG(0); //Set FRACV to '0'
      }
      else
      {
        /* Configure PLL  PLL4FRACV  in fractional mode*/
        __HAL_RCC_PLL4FRACV_CONFIG(pll4->PLLFRACV);
      }
      __HAL_RCC_PLL4FRACV_ENABLE(); //Set FRACLE to ‘1’

      /* Configure the Spread Control */
      if (pll4->PLLMODE == RCC_PLL_SPREAD_SPECTRUM)
      {
        assert_param(IS_RCC_INC_STEP(pll4->INC_STEP));
        assert_param(IS_RCC_SSCG_MODE(pll4->SSCG_MODE));
        assert_param(IS_RCC_RPDFN_DIS(pll4->RPDFN_DIS));
        assert_param(IS_RCC_TPDFN_DIS(pll4->TPDFN_DIS));
        assert_param(IS_RCC_MOD_PER(pll4->MOD_PER));

        __HAL_RCC_PLL4CSGCONFIG(pll4->MOD_PER, pll4->TPDFN_DIS, pll4->RPDFN_DIS,
                                pll4->SSCG_MODE, pll4->INC_STEP);
        __HAL_RCC_PLL4_SSMODE_ENABLE();
      }
      else
      {
        __HAL_RCC_PLL4_SSMODE_DISABLE();
      }

      /* Enable the PLL4. */
      __HAL_RCC_PLL4_ENABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till PLL is ready */
      while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY) == RESET)
      {
        if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
      /* Enable PLL4P Clock output. */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP | RCC_PLL4_DIVQ | RCC_PLL4_DIVR);
    }
    else
    {
      /*Disable the post-dividers*/
      __HAL_RCC_PLL4CLKOUT_DISABLE(RCC_PLL4_DIVP | RCC_PLL4_DIVQ | RCC_PLL4_DIVR);
      /* Disable the PLL4. */
      __HAL_RCC_PLL4_DISABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till PLL is ready */
      while (__HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY) != RESET)
      {
        if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
  }
  return HAL_OK;
}

/**
  * @brief  Initializes the RCC extended peripherals clocks according to the
  *         specified parameters in the RCC_PeriphCLKInitTypeDef.
  * @param  PeriphClkInit: pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         contains a field PeriphClockSelection which can be a combination of
  *         the following values:
  *         @arg @ref RCC_PERIPHCLK_USART1 USART1 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_UART24 USART2 and UART4 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_UART35 USART3 and UART5 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_I2C12 I2C1 and I2C2 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_I2C35 I2C3 and I2C5 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_LPTIM1 LPTIM1 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SAI1 SAI1 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SAI2 SAI2 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_USBPHY USBPHY peripheral clock
  *         @arg @ref RCC_PERIPHCLK_ADC ADC peripheral clock
  *         @arg @ref RCC_PERIPHCLK_RTC RTC peripheral clock
  *         @arg @ref RCC_PERIPHCLK_CEC CEC peripheral clock
  *         @arg @ref RCC_PERIPHCLK_USART6 USART6 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_UART78 UART7 and UART8 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_I2C46 I2C4 and I2C6 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_LPTIM23 LPTIM2 and LPTIM3 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_LPTIM45 LPTIM4 and LPTIM5 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SAI3 SAI3 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_FMC FMC peripheral clock
  *         @arg @ref RCC_PERIPHCLK_QSPI QSPI peripheral clock
  *         @arg @ref RCC_PERIPHCLK_DSI DSI peripheral clock
  *         @arg @ref RCC_PERIPHCLK_CKPER CKPER peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SPDIFRX SPDIFRX peripheral clock
  *         @arg @ref RCC_PERIPHCLK_FDCAN FDCAN peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SPI1 SPI/I2S1 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SPI23 SPI/I2S2 and SPI/I2S3 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SPI45 SPI4 and SPI5 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SPI6 SPI6 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SAI4 SAI4 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SDMMC12 SDMMC1 and SDMMC2 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_SDMMC3 SDMMC3 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_ETH ETH peripheral clock
  *         @arg @ref RCC_PERIPHCLK_RNG1 RNG1 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_RNG2 RNG2 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_USBO USBO peripheral clock
  *         @arg @ref RCC_PERIPHCLK_STGEN STGEN peripheral clock
  *         @arg @ref RCC_PERIPHCLK_TIMG1 TIMG1 peripheral clock
  *         @arg @ref RCC_PERIPHCLK_TIMG2 TIMG2 peripheral clock
  *
  * @note   Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to
  *         select the RTC clock source; in this case the Backup domain will be
  *         reset in order to modify the RTC Clock source, as consequence RTC
  *         registers (including the backup registers) are set to their reset
  *         values.
  *
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef
                                            *PeriphClkInit)
{
  uint32_t tmpreg = 0, RESERVED_BDCR_MASK = 0;
  uint32_t tickstart;
  HAL_StatusTypeDef ret = HAL_OK;      /* Intermediate status */
  HAL_StatusTypeDef status = HAL_OK;   /* Final status */

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClkInit->PeriphClockSelection));

  /*---------------------------- CKPER configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_CKPER) ==
      RCC_PERIPHCLK_CKPER)
  {
    /* Check the parameters */
    assert_param(IS_RCC_CKPERCLKSOURCE(PeriphClkInit->CkperClockSelection));

    __HAL_RCC_CKPER_CONFIG(PeriphClkInit->CkperClockSelection);
  }

  /*------------------------------ I2C12 Configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) &  RCC_PERIPHCLK_I2C12) ==
      RCC_PERIPHCLK_I2C12)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C12CLKSOURCE(PeriphClkInit->I2c12ClockSelection));

    if ((PeriphClkInit->I2c12ClockSelection) == RCC_I2C12CLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
    }

    __HAL_RCC_I2C12_CONFIG(PeriphClkInit->I2c12ClockSelection);
  }

  /*------------------------------ I2C35 Configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) &  RCC_PERIPHCLK_I2C35) ==
      RCC_PERIPHCLK_I2C35)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C35CLKSOURCE(PeriphClkInit->I2c35ClockSelection));

    if ((PeriphClkInit->I2c35ClockSelection) == RCC_I2C35CLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
    }

    __HAL_RCC_I2C35_CONFIG(PeriphClkInit->I2c35ClockSelection);
  }

  /*------------------------------ I2C46 Configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C46) ==
      RCC_PERIPHCLK_I2C46)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C46CLKSOURCE(PeriphClkInit->I2c46ClockSelection));

    if ((PeriphClkInit->I2c46ClockSelection) == RCC_I2C46CLKSOURCE_PLL3)
    {
      status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
      if (status != HAL_OK)
      {
        return status;
      }
      __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);
    }

    __HAL_RCC_I2C46_CONFIG(PeriphClkInit->I2c46ClockSelection);
  }

  /*---------------------------- SAI1 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SAI1) ==
      RCC_PERIPHCLK_SAI1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SAI1CLKSOURCE(PeriphClkInit->Sai1ClockSelection));

    switch (PeriphClkInit->Sai1ClockSelection)
    {
      case RCC_SAI1CLKSOURCE_PLL4:  /* PLL4 is used as clock source for SAI1*/

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;

      case RCC_SAI1CLKSOURCE_PLL3_Q:  /* PLL3_Q is used as clock source for SAI1*/

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3 */

        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_SAI1CLKSOURCE_PLL3_R:  /* PLL3_R is used as clock source for SAI1*/

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3 */

        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;
    }

    /* Set the source of SAI1 clock*/
    __HAL_RCC_SAI1_CONFIG(PeriphClkInit->Sai1ClockSelection);
  }

  /*---------------------------- SAI2 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SAI2) ==
      RCC_PERIPHCLK_SAI2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SAI2CLKSOURCE(PeriphClkInit->Sai2ClockSelection));

    switch (PeriphClkInit->Sai2ClockSelection)
    {
      case RCC_SAI2CLKSOURCE_PLL4:  /* PLL4 is used as clock source for SAI2*/

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;

      case RCC_SAI2CLKSOURCE_PLL3_Q: /* PLL3_Q is used as clock source for SAI2 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_SAI2CLKSOURCE_PLL3_R: /* PLL3_R is used as clock source for SAI2 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;
    }

    /* Set the source of SAI2 clock*/
    __HAL_RCC_SAI2_CONFIG(PeriphClkInit->Sai2ClockSelection);
  }

  /*---------------------------- SAI3 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SAI3) ==
      RCC_PERIPHCLK_SAI3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SAI3CLKSOURCE(PeriphClkInit->Sai3ClockSelection));

    switch (PeriphClkInit->Sai3ClockSelection)
    {
      case RCC_SAI3CLKSOURCE_PLL4: /* PLL4 is used as clock source for SAI3*/

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;

      case RCC_SAI3CLKSOURCE_PLL3_Q: /* PLL3_Q is used as clock source for SAI3 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_SAI3CLKSOURCE_PLL3_R: /* PLL3_R is used as clock source for SAI3 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;
    }

    /* Set the source of SAI3 clock*/
    __HAL_RCC_SAI3_CONFIG(PeriphClkInit->Sai3ClockSelection);
  }

  /*---------------------------- SAI4 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SAI4) ==
      RCC_PERIPHCLK_SAI4)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SAI4CLKSOURCE(PeriphClkInit->Sai4ClockSelection));

    switch (PeriphClkInit->Sai4ClockSelection)
    {
      case RCC_SAI4CLKSOURCE_PLL4: /* PLL4 is used as clock source for SAI4 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;


      case RCC_SAI4CLKSOURCE_PLL3_Q: /* PLL3_Q is used as clock source for SAI4 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3_Q */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_SAI4CLKSOURCE_PLL3_R: /* PLL3_R is used as clock source for SAI4 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SAI Clock output generated on PLL3_R */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;
    }

    /* Set the source of SAI4 clock*/
    __HAL_RCC_SAI4_CONFIG(PeriphClkInit->Sai4ClockSelection);
  }

  /*---------------------------- SPI1 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SPI1) ==
      RCC_PERIPHCLK_SPI1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SPI1CLKSOURCE(PeriphClkInit->Spi1ClockSelection));

    switch (PeriphClkInit->Spi1ClockSelection)
    {
      case RCC_SPI1CLKSOURCE_PLL4: /* PLL4 is used as clock source for SPI1 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;

      case RCC_SPI1CLKSOURCE_PLL3_Q: /* PLL3_Q is used as clock source for SPI1*/

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_SPI1CLKSOURCE_PLL3_R: /* PLL3_R is used as clock source for SPI1 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;

    }

    /* Set the source of SPI1 clock*/
    __HAL_RCC_SPI1_CONFIG(PeriphClkInit->Spi1ClockSelection);
  }

  /*---------------------------- SPI23 configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SPI23) ==
      RCC_PERIPHCLK_SPI23)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SPI23CLKSOURCE(PeriphClkInit->Spi23ClockSelection));

    switch (PeriphClkInit->Spi23ClockSelection)
    {
      case RCC_SPI23CLKSOURCE_PLL4: /* PLL4 is used as clock source for SPI23 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;

      case RCC_SPI23CLKSOURCE_PLL3_Q: /* PLL3_Q is used as clock source for SPI23 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_SPI23CLKSOURCE_PLL3_R: /* PLL3_R is used as clock source for SPI23 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;
    }

    /* Set the source of SPI2 clock*/
    __HAL_RCC_SPI23_CONFIG(PeriphClkInit->Spi23ClockSelection);
  }

  /*---------------------------- SPI45 configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SPI45) ==
      RCC_PERIPHCLK_SPI45)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SPI45CLKSOURCE(PeriphClkInit->Spi45ClockSelection));

    if (PeriphClkInit->Spi45ClockSelection == RCC_SPI45CLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable SPI Clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);
    }

    /* Set the source of SPI45 clock*/
    __HAL_RCC_SPI45_CONFIG(PeriphClkInit->Spi45ClockSelection);
  }

  /*---------------------------- SPI6 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SPI6) ==
      RCC_PERIPHCLK_SPI6)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SPI6CLKSOURCE(PeriphClkInit->Spi6ClockSelection));

    switch (PeriphClkInit->Spi6ClockSelection)
    {
      case RCC_SPI6CLKSOURCE_PLL4: /* PLL4 is used as clock source for SPI6 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;

      case RCC_SPI6CLKSOURCE_PLL3: /* PLL3 is used as clock source for SPI6 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPI Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;
    }

    /* Set the source of SPI6 clock*/
    __HAL_RCC_SPI6_CONFIG(PeriphClkInit->Spi6ClockSelection);
  }

  /*---------------------------- USART6 configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART6) ==
      RCC_PERIPHCLK_USART6)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART6CLKSOURCE(PeriphClkInit->Usart6ClockSelection));

    if (PeriphClkInit->Usart6ClockSelection == RCC_USART6CLKSOURCE_PLL4)
    {
      /* PLL4 is used as clock source for USART6 */
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable USART Clock output generated on PLL4 */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);
    }

    /* Set the source of USART6 clock*/
    __HAL_RCC_USART6_CONFIG(PeriphClkInit->Usart6ClockSelection);
  }

  /*---------------------------- UART24 configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_UART24) ==
      RCC_PERIPHCLK_UART24)
  {
    /* Check the parameters */
    assert_param(IS_RCC_UART24CLKSOURCE(PeriphClkInit->Uart24ClockSelection));

    if (PeriphClkInit->Uart24ClockSelection == RCC_UART24CLKSOURCE_PLL4)
    {
      /* PLL4 is used as clock source for UART24 */
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable UART Clock output generated on PLL4 */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);
    }

    /* Set the source of UART24 clock*/
    __HAL_RCC_UART24_CONFIG(PeriphClkInit->Uart24ClockSelection);
  }

  /*---------------------------- UART35 configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_UART35) ==
      RCC_PERIPHCLK_UART35)
  {
    /* Check the parameters */
    assert_param(IS_RCC_UART35CLKSOURCE(PeriphClkInit->Uart35ClockSelection));

    if (PeriphClkInit->Uart35ClockSelection == RCC_UART35CLKSOURCE_PLL4)
    {
      /* PLL4 is used as clock source for UART35 */
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable UART Clock output generated on PLL4 */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);
    }

    /* Set the source of UART35 clock*/
    __HAL_RCC_UART35_CONFIG(PeriphClkInit->Uart35ClockSelection);
  }

  /*---------------------------- UAUART78 configuration ----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_UART78) ==
      RCC_PERIPHCLK_UART78)
  {
    /* Check the parameters */
    assert_param(IS_RCC_UART78CLKSOURCE(PeriphClkInit->Uart78ClockSelection));

    if (PeriphClkInit->Uart78ClockSelection == RCC_UART78CLKSOURCE_PLL4)
    {
      /* PLL4 is used as clock source for UART78 */
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable UART Clock output generated on PLL4 */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);
    }

    /* Set the source of UART78 clock*/
    __HAL_RCC_UART78_CONFIG(PeriphClkInit->Uart78ClockSelection);
  }

  /*---------------------------- USART1 configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART1) ==
      RCC_PERIPHCLK_USART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART1CLKSOURCE(PeriphClkInit->Usart1ClockSelection));

    switch (PeriphClkInit->Usart1ClockSelection)
    {
      case RCC_USART1CLKSOURCE_PLL3:  /* PLL3 is used as clock source for USART1 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable UART Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_USART1CLKSOURCE_PLL4: /* PLL4 is used as clock source for USART1 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable USART Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;
    }

    /* Set the source of USART1 clock*/
    __HAL_RCC_USART1_CONFIG(PeriphClkInit->Usart1ClockSelection);
  }

  /*---------------------------- SDMMC12 configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SDMMC12) ==
      RCC_PERIPHCLK_SDMMC12)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SDMMC12CLKSOURCE(PeriphClkInit->Sdmmc12ClockSelection));

    switch (PeriphClkInit->Sdmmc12ClockSelection)
    {
      case RCC_SDMMC12CLKSOURCE_PLL3: /* PLL3 is used as clock source for SDMMC12 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SDMMC12 Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;

      case RCC_SDMMC12CLKSOURCE_PLL4: /* PLL4 is used as clock source for SDMMC12 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SDMMC12 Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;
    }

    /* Set the source of SDMMC12 clock*/
    __HAL_RCC_SDMMC12_CONFIG(PeriphClkInit->Sdmmc12ClockSelection);
  }

  /*---------------------------- SDMMC3 configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SDMMC3) ==
      RCC_PERIPHCLK_SDMMC3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SDMMC3CLKSOURCE(PeriphClkInit->Sdmmc3ClockSelection));

    switch (PeriphClkInit->Sdmmc3ClockSelection)
    {
      case RCC_SDMMC3CLKSOURCE_PLL3:  /* PLL3 is used as clock source for SDMMC3 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SDMMC3 Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;

      case RCC_SDMMC3CLKSOURCE_PLL4:  /* PLL4 is used as clock source for SDMMC3 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SDMMC3 Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;
    }

    /* Set the source of SDMMC3 clock*/
    __HAL_RCC_SDMMC3_CONFIG(PeriphClkInit->Sdmmc3ClockSelection);
  }

  /*---------------------------- ETH configuration ---------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ETH) ==
      RCC_PERIPHCLK_ETH)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ETHCLKSOURCE(PeriphClkInit->EthClockSelection));

    switch (PeriphClkInit->EthClockSelection)
    {
      case RCC_ETHCLKSOURCE_PLL4:     /* PLL4 is used as clock source for ETH */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable ETH Clock output generated on PLL2 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;

      case RCC_ETHCLKSOURCE_PLL3:     /* PLL3 is used as clock source for ETH */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable ETH Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;
    }

    /* Set the source of ETH clock*/
    __HAL_RCC_ETH_CONFIG(PeriphClkInit->EthClockSelection);
  }

  /*---------------------------- QSPI configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_QSPI) ==
      RCC_PERIPHCLK_QSPI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_QSPICLKSOURCE(PeriphClkInit->QspiClockSelection));

    switch (PeriphClkInit->QspiClockSelection)
    {
      case RCC_QSPICLKSOURCE_PLL3:   /* PLL3 is used as clock source for QSPI */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable QSPI Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;

      case RCC_QSPICLKSOURCE_PLL4:   /* PLL4 is used as clock source for QSPI */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable QSPI Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;
    }

    /* Set the source of QSPI clock*/
    __HAL_RCC_QSPI_CONFIG(PeriphClkInit->QspiClockSelection);
  }

  /*---------------------------- FMC configuration ---------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_FMC) ==
      RCC_PERIPHCLK_FMC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_FMCCLKSOURCE(PeriphClkInit->FmcClockSelection));

    switch (PeriphClkInit->FmcClockSelection)
    {
      case RCC_FMCCLKSOURCE_PLL3: /* PLL3 is used as clock source for FMC */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable FMC Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVR);

        break;

      case RCC_FMCCLKSOURCE_PLL4: /* PLL4 is used as clock source for FMC */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable FMC Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;
    }

    /* Set the source of FMC clock*/
    __HAL_RCC_FMC_CONFIG(PeriphClkInit->FmcClockSelection);
  }

#if defined(FDCAN1)
  /*---------------------------- FDCAN configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_FDCAN) ==
      RCC_PERIPHCLK_FDCAN)
  {
    /* Check the parameters */
    assert_param(IS_RCC_FDCANCLKSOURCE(PeriphClkInit->FdcanClockSelection));

    switch (PeriphClkInit->FdcanClockSelection)
    {
      case RCC_FDCANCLKSOURCE_PLL3: /* PLL3 is used as clock source for FDCAN */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable FDCAN Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_FDCANCLKSOURCE_PLL4_Q: /* PLL4_Q is used as clock source for FDCAN */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable FDCAN Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);

        break;

      case RCC_FDCANCLKSOURCE_PLL4_R: /* PLL4_R is used as clock source for FDCAN */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable FDCAN Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);

        break;
    }

    /* Set the source of FDCAN clock*/
    __HAL_RCC_FDCAN_CONFIG(PeriphClkInit->FdcanClockSelection);
  }
#endif /*FDCAN1*/

  /*---------------------------- SPDIFRX configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SPDIFRX) ==
      RCC_PERIPHCLK_SPDIFRX)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SPDIFRXCLKSOURCE(PeriphClkInit->SpdifrxClockSelection));

    switch (PeriphClkInit->SpdifrxClockSelection)
    {
      case RCC_SPDIFRXCLKSOURCE_PLL4: /* PLL4 is used as clock source for SPDIF */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPDIF Clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;

      case RCC_SPDIFRXCLKSOURCE_PLL3: /* PLL3 is used as clock source for SPDIF */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable SPDIF Clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;
    }

    /* Set the source of SPDIF clock*/
    __HAL_RCC_SPDIFRX_CONFIG(PeriphClkInit->SpdifrxClockSelection);
  }

  /*---------------------------- CEC configuration ---------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_CEC) ==
      RCC_PERIPHCLK_CEC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_CECCLKSOURCE(PeriphClkInit->CecClockSelection));

    __HAL_RCC_CEC_CONFIG(PeriphClkInit->CecClockSelection);
  }

  /*---------------------------- USBPHY configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USBPHY) ==
      RCC_PERIPHCLK_USBPHY)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USBPHYCLKSOURCE(PeriphClkInit->UsbphyClockSelection));

    if (PeriphClkInit->UsbphyClockSelection == RCC_USBPHYCLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable USB PHY Clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
    }

    __HAL_RCC_USBPHY_CONFIG(PeriphClkInit->UsbphyClockSelection);
  }

  /*---------------------------- USBO configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USBO) ==
      RCC_PERIPHCLK_USBO)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USBOCLKSOURCE(PeriphClkInit->UsboClockSelection));

    if (PeriphClkInit->UsboClockSelection == RCC_USBOCLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable USB OTG Clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
    }

    __HAL_RCC_USBO_CONFIG(PeriphClkInit->UsboClockSelection);
  }

  /*---------------------------- RNG1 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RNG1) ==
      RCC_PERIPHCLK_RNG1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_RNG1CLKSOURCE(PeriphClkInit->Rng1ClockSelection));

    if (PeriphClkInit->Rng1ClockSelection == RCC_RNG1CLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable RNG1 Clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
    }

    /* Set the source of RNG1 clock*/
    __HAL_RCC_RNG1_CONFIG(PeriphClkInit->Rng1ClockSelection);
  }

  /*---------------------------- RNG2 configuration --------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RNG2) ==
      RCC_PERIPHCLK_RNG2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_RNG2CLKSOURCE(PeriphClkInit->Rng2ClockSelection));

    if (PeriphClkInit->Rng2ClockSelection == RCC_RNG2CLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable RNG2 Clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
    }

    /* Set the source of RNG2 clock*/
    __HAL_RCC_RNG2_CONFIG(PeriphClkInit->Rng2ClockSelection);
  }

  /*---------------------------- STGEN configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_STGEN) ==
      RCC_PERIPHCLK_STGEN)
  {
    /* Check the parameters */
    assert_param(IS_RCC_STGENCLKSOURCE(PeriphClkInit->StgenClockSelection));

    __HAL_RCC_STGEN_CONFIG(PeriphClkInit->StgenClockSelection);
  }

#if defined(DSI)
  /*---------------------------- DSI configuration ---------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_DSI) ==
      RCC_PERIPHCLK_DSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_DSICLKSOURCE(PeriphClkInit->DsiClockSelection));

    if (PeriphClkInit->DsiClockSelection == RCC_DSICLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable DSI Clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);
    }

    __HAL_RCC_DSI_CONFIG(PeriphClkInit->DsiClockSelection);
  }
#endif /*DSI*/

  /*---------------------------- ADC configuration ---------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ADC) ==
      RCC_PERIPHCLK_ADC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ADCCLKSOURCE(PeriphClkInit->AdcClockSelection));

    switch (PeriphClkInit->AdcClockSelection)
    {
      case RCC_ADCCLKSOURCE_PLL4: /* PLL4 is used as clock source for ADC */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable ADC Clock output generated on PLL4 */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVR);
        break;

      case RCC_ADCCLKSOURCE_PLL3: /* PLL3 is used as clock source for ADC */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable ADC Clock output generated on PLL3 */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;
    }

    /* Set the source of ADC clock*/
    __HAL_RCC_ADC_CONFIG(PeriphClkInit->AdcClockSelection);
  }

  /*---------------------------- LPTIM45 configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM45) ==
      RCC_PERIPHCLK_LPTIM45)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LPTIM45CLKSOURCE(PeriphClkInit->Lptim45ClockSelection));

    switch (PeriphClkInit->Lptim45ClockSelection)
    {
      case RCC_LPTIM45CLKSOURCE_PLL3: /* PLL3 is used as clock source for LPTIM45 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_LPTIM45CLKSOURCE_PLL4: /* PLL4 is used as clock source for LPTIM45 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;
    }

    /* Set the source of LPTIM45 clock*/
    __HAL_RCC_LPTIM45_CONFIG(PeriphClkInit->Lptim45ClockSelection);
  }

  /*---------------------------- LPTIM23 configuration -----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM23) ==
      RCC_PERIPHCLK_LPTIM23)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LPTIM23CLKSOURCE(PeriphClkInit->Lptim23ClockSelection));

    if (PeriphClkInit->Lptim23ClockSelection == RCC_LPTIM23CLKSOURCE_PLL4)
    {
      status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
      if (status != HAL_OK)
      {
        return status;
      }
      /* Enable clock output generated on PLL4 . */
      __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVQ);
    }

    /* Set the source of LPTIM23 clock*/
    __HAL_RCC_LPTIM23_CONFIG(PeriphClkInit->Lptim23ClockSelection);
  }

  /*---------------------------- LPTIM1 configuration ------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM1) ==
      RCC_PERIPHCLK_LPTIM1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LPTIM1CLKSOURCE(PeriphClkInit->Lptim1ClockSelection));

    switch (PeriphClkInit->Lptim1ClockSelection)
    {
      case RCC_LPTIM1CLKSOURCE_PLL3:  /* PLL3 is used as clock source for LPTIM1 */

        status = RCCEx_PLL3_Config(&(PeriphClkInit->PLL3));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable clock output generated on PLL3 . */
        __HAL_RCC_PLL3CLKOUT_ENABLE(RCC_PLL3_DIVQ);

        break;

      case RCC_LPTIM1CLKSOURCE_PLL4:  /* PLL4 is used as clock source for LPTIM1 */

        status = RCCEx_PLL4_Config(&(PeriphClkInit->PLL4));
        if (status != HAL_OK)
        {
          return status;
        }
        /* Enable clock output generated on PLL4 . */
        __HAL_RCC_PLL4CLKOUT_ENABLE(RCC_PLL4_DIVP);

        break;
    }

    /* Set the source of LPTIM1 clock*/
    __HAL_RCC_LPTIM1_CONFIG(PeriphClkInit->Lptim1ClockSelection);
  }

  /*---------------------------- RTC configuration ---------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) ==
      RCC_PERIPHCLK_RTC)
  {
    /* check for RTC Parameters used to output RTCCLK */
    assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->RTCClockSelection));

    /* Enable write access to Backup domain */
    SET_BIT(PWR->CR1, PWR_CR1_DBP);

    /* Wait for Backup domain Write protection disable */
    tickstart = HAL_GetTick();

    while ((PWR->CR1 & PWR_CR1_DBP) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > DBP_TIMEOUT_VALUE)
      {
        ret = HAL_TIMEOUT;
      }
    }

    if (ret == HAL_OK)
    {
      /* Reset the Backup domain only if the RTC Clock source selection is modified */
      if ((RCC->BDCR & RCC_BDCR_RTCSRC) != (PeriphClkInit->RTCClockSelection & RCC_BDCR_RTCSRC))
      {
        /* Store the content of BDCR register before the reset of Backup Domain */
        tmpreg = READ_BIT(RCC->BDCR, ~(RCC_BDCR_RTCSRC));
        /* RTC Clock selection can be changed only if the Backup Domain is reset */
        __HAL_RCC_BACKUPRESET_FORCE();
        __HAL_RCC_BACKUPRESET_RELEASE();

        /* Set the LSEDrive value */
        __HAL_RCC_LSEDRIVE_CONFIG(tmpreg & RCC_BDCR_LSEDRV);

        /* RCC_BDCR_LSEON can be enabled for RTC or another IP, re-enable it */
        RCC_OscInitTypeDef RCC_OscInitStructure;
        /* Configure LSE Oscillator*/
        RCC_OscInitStructure.OscillatorType = RCC_OSCILLATORTYPE_LSE;
        RCC_OscInitStructure.LSEState = (tmpreg & LSE_MASK);

        RCC_OscInitStructure.PLL.PLLState = RCC_PLL_NONE;
        RCC_OscInitStructure.PLL2.PLLState = RCC_PLL_NONE;
        RCC_OscInitStructure.PLL3.PLLState = RCC_PLL_NONE;
        RCC_OscInitStructure.PLL4.PLLState = RCC_PLL_NONE;
        ret = HAL_RCC_OscConfig(&RCC_OscInitStructure);
        if (ret != HAL_OK)
        {
          return ret;
        }

        /* Write the RTCSRC */
        __HAL_RCC_RTC_CONFIG(PeriphClkInit->RTCClockSelection);

        /* Fill up Reserved register mask for BDCR
         * All already filled up or what shouldn't be modified must be put on the mask */
        RESERVED_BDCR_MASK = ~(RCC_BDCR_VSWRST | RCC_BDCR_RTCCKEN | RCC_BDCR_RTCSRC |
                               RCC_BDCR_LSECSSD | RCC_BDCR_LSEDRV | RCC_BDCR_DIGBYP |
                               RCC_BDCR_LSERDY | RCC_BDCR_LSEBYP | RCC_BDCR_LSEON);

        /* Restore the BDCR context: RESERVED registers plus RCC_BDCR_LSECSSON */
        WRITE_REG(RCC->BDCR, (READ_REG(RCC->BDCR) | (tmpreg & RESERVED_BDCR_MASK)));

      }/* End RTCSRC changed */

      /*Enable RTC clock   */
      __HAL_RCC_RTC_ENABLE();
    }
    else
    {
      // Enable write access to Backup domain failed
      /* return the error */
      return ret;
    }
  }

  /*---------------------------- TIMG1 configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIMG1) ==
      RCC_PERIPHCLK_TIMG1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIMG1PRES(PeriphClkInit->TIMG1PresSelection));

    /* Set TIMG1 division factor */
    __HAL_RCC_TIMG1PRES(PeriphClkInit->TIMG1PresSelection);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till TIMG1 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_TIMG1PRERDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*---------------------------- TIMG2 configuration -------------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIMG2) ==
      RCC_PERIPHCLK_TIMG2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIMG2PRES(PeriphClkInit->TIMG2PresSelection));

    /* Set TIMG1 division factor */
    __HAL_RCC_TIMG2PRES(PeriphClkInit->TIMG2PresSelection);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till TIMG1 is ready */
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_TIMG2PRERDY) == RESET)
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  return HAL_OK;
}



/**
  * @brief  Get the RCC_ClkInitStruct according to the internal RCC
  *         configuration registers.
  * @param  PeriphClkInit: pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         returns the configuration information for the Extended Peripherals
  *         clocks : CKPER, I2C12, I2C35, I2C46, I2C5, SAI1, SAI2, SAI3, SAI4,
  *         SPI1, SPI2, SPI3, SPI45, SPI6, USART1, UART24, USART3, UART24, UART35,
  *         USART6, UART78, SDMMC12, SDMMC3, ETH, QSPI,FMC, FDCAN,
  *         SPDIFRX, CEC, USBPHY, USBO, RNG1, RNG2, STGEN, DSI, ADC, RTC,
  *         LPTIM1, LPTIM23, LPTIM45
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  /* Set all possible values for the extended clock type parameter------------*/
  PeriphClkInit->PeriphClockSelection =
    RCC_PERIPHCLK_CKPER | RCC_PERIPHCLK_I2C12 | RCC_PERIPHCLK_I2C35 |
    RCC_PERIPHCLK_I2C46 | RCC_PERIPHCLK_SAI1 | RCC_PERIPHCLK_SAI2 |
    RCC_PERIPHCLK_SAI3 | RCC_PERIPHCLK_SAI4 | RCC_PERIPHCLK_SPI1 |
    RCC_PERIPHCLK_SPI23 | RCC_PERIPHCLK_SPI45 | RCC_PERIPHCLK_SPI6 |
    RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_UART24 | RCC_PERIPHCLK_UART35 |
    RCC_PERIPHCLK_USART6 | RCC_PERIPHCLK_UART78 | RCC_PERIPHCLK_SDMMC12 |
    RCC_PERIPHCLK_SDMMC3 | RCC_PERIPHCLK_ETH | RCC_PERIPHCLK_QSPI |
    RCC_PERIPHCLK_FMC | RCC_PERIPHCLK_FDCAN | RCC_PERIPHCLK_SPDIFRX |
    RCC_PERIPHCLK_CEC | RCC_PERIPHCLK_USBPHY | RCC_PERIPHCLK_USBO |
    RCC_PERIPHCLK_RNG1 | RCC_PERIPHCLK_RNG2 | RCC_PERIPHCLK_STGEN |
    RCC_PERIPHCLK_DSI | RCC_PERIPHCLK_ADC | RCC_PERIPHCLK_RTC |
    RCC_PERIPHCLK_LPTIM1 | RCC_PERIPHCLK_LPTIM23 | RCC_PERIPHCLK_LPTIM45;

  /* Get the CKPER clock source ----------------------------------------------*/
  PeriphClkInit->CkperClockSelection    = __HAL_RCC_GET_CKPER_SOURCE();

  /* Get the I2C12 clock source ----------------------------------------------*/
  PeriphClkInit->I2c12ClockSelection     = __HAL_RCC_GET_I2C12_SOURCE();
  /* Get the I2C35 clock source ----------------------------------------------*/
  PeriphClkInit->I2c35ClockSelection     = __HAL_RCC_GET_I2C35_SOURCE();
  /* Get the I2C46 clock source -----------------------------------------------*/
  PeriphClkInit->I2c46ClockSelection     = __HAL_RCC_GET_I2C46_SOURCE();

  /* Get the SAI1 clock source -----------------------------------------------*/
  PeriphClkInit->Sai1ClockSelection     = __HAL_RCC_GET_SAI1_SOURCE();
  /* Get the SAI2 clock source -----------------------------------------------*/
  PeriphClkInit->Sai2ClockSelection     = __HAL_RCC_GET_SAI2_SOURCE();
  /* Get the SAI3 clock source -----------------------------------------------*/
  PeriphClkInit->Sai3ClockSelection     = __HAL_RCC_GET_SAI3_SOURCE();
  /* Get the SAI4 clock source -----------------------------------------------*/
  PeriphClkInit->Sai4ClockSelection     = __HAL_RCC_GET_SAI4_SOURCE();

  /* Get the SPI1 clock source -----------------------------------------------*/
  PeriphClkInit->Spi1ClockSelection     = __HAL_RCC_GET_SPI1_SOURCE();
  /* Get the SPI23 clock source ----------------------------------------------*/
  PeriphClkInit->Spi23ClockSelection    = __HAL_RCC_GET_SPI23_SOURCE();
  /* Get the SPI45 clock source ----------------------------------------------*/
  PeriphClkInit->Spi45ClockSelection     = __HAL_RCC_GET_SPI45_SOURCE();
  /* Get the SPI6 clock source -----------------------------------------------*/
  PeriphClkInit->Spi6ClockSelection     = __HAL_RCC_GET_SPI6_SOURCE();

  /* Get the USART1 configuration --------------------------------------------*/
  PeriphClkInit->Usart1ClockSelection   = __HAL_RCC_GET_USART1_SOURCE();
  /* Get the UART24 clock source ----------------------------------------------*/
  PeriphClkInit->Uart24ClockSelection    = __HAL_RCC_GET_UART24_SOURCE();
  /* Get the UART35 clock source ---------------------------------------------*/
  PeriphClkInit->Uart35ClockSelection    = __HAL_RCC_GET_UART35_SOURCE();
  /* Get the USART6 clock source ---------------------------------------------*/
  PeriphClkInit->Usart6ClockSelection   = __HAL_RCC_GET_USART6_SOURCE();
  /* Get the UART78 clock source ---------------------------------------------*/
  PeriphClkInit->Uart78ClockSelection    = __HAL_RCC_GET_UART78_SOURCE();

  /* Get the SDMMC12 clock source --------------------------------------------*/
  PeriphClkInit->Sdmmc12ClockSelection   = __HAL_RCC_GET_SDMMC12_SOURCE();
  /* Get the SDMMC3 clock source ---------------------------------------------*/
  PeriphClkInit->Sdmmc3ClockSelection   = __HAL_RCC_GET_SDMMC3_SOURCE();
  /* Get the SDMMC3 clock source ---------------------------------------------*/
  PeriphClkInit->EthClockSelection      = __HAL_RCC_GET_ETH_SOURCE();

  /* Get the QSPI clock source -----------------------------------------------*/
  PeriphClkInit->QspiClockSelection     = __HAL_RCC_GET_QSPI_SOURCE();
  /* Get the FMC clock source ------------------------------------------------*/
  PeriphClkInit->FmcClockSelection      = __HAL_RCC_GET_FMC_SOURCE();
#if defined(FDCAN1)
  /* Get the FDCAN clock source ----------------------------------------------*/
  PeriphClkInit->FdcanClockSelection    = __HAL_RCC_GET_FDCAN_SOURCE();
#endif /*FDCAN1*/
  /* Get the SPDIFRX clock source --------------------------------------------*/
  PeriphClkInit->SpdifrxClockSelection  = __HAL_RCC_GET_SPDIFRX_SOURCE();
  /* Get the CEC clock source ------------------------------------------------*/
  PeriphClkInit->CecClockSelection      = __HAL_RCC_GET_CEC_SOURCE();
  /* Get the USBPHY clock source ---------------------------------------------*/
  PeriphClkInit-> UsbphyClockSelection  = __HAL_RCC_GET_USBPHY_SOURCE();
  /* Get the USBO clock source -----------------------------------------------*/
  PeriphClkInit-> UsboClockSelection    = __HAL_RCC_GET_USBO_SOURCE();
  /* Get the RNG1 clock source -----------------------------------------------*/
  PeriphClkInit->Rng1ClockSelection     = __HAL_RCC_GET_RNG1_SOURCE();
  /* Get the RNG2 clock source -----------------------------------------------*/
  PeriphClkInit->Rng2ClockSelection     = __HAL_RCC_GET_RNG2_SOURCE();
  /* Get the STGEN clock source ----------------------------------------------*/
  PeriphClkInit->StgenClockSelection    = __HAL_RCC_GET_STGEN_SOURCE();
#if defined(DSI)
  /* Get the DSI clock source ------------------------------------------------*/
  PeriphClkInit->DsiClockSelection      = __HAL_RCC_GET_DSI_SOURCE();
#endif /*DSI*/
  /* Get the ADC clock source ------------------------------------------------*/
  PeriphClkInit->AdcClockSelection      = __HAL_RCC_GET_ADC_SOURCE();
  /* Get the RTC clock source ------------------------------------------------*/
  PeriphClkInit->RTCClockSelection      = __HAL_RCC_GET_RTC_SOURCE();

  /* Get the LPTIM1 clock source ---------------------------------------------*/
  PeriphClkInit->Lptim1ClockSelection  = __HAL_RCC_GET_LPTIM1_SOURCE();
  /* Get the LPTIM23 clock source ---------------------------------------------*/
  PeriphClkInit->Lptim23ClockSelection  = __HAL_RCC_GET_LPTIM23_SOURCE();
  /* Get the LPTIM45 clock source ---------------------------------------------*/
  PeriphClkInit->Lptim45ClockSelection  = __HAL_RCC_GET_LPTIM45_SOURCE();

  /* Get the TIM1 Prescaler configuration ------------------------------------*/
  PeriphClkInit->TIMG1PresSelection = __HAL_RCC_GET_TIMG1PRES();
  /* Get the TIM2 Prescaler configuration ------------------------------------*/
  PeriphClkInit->TIMG2PresSelection = __HAL_RCC_GET_TIMG2PRES();
}

/**
  * @brief  Enables the LSE Clock Security System.
  * @note   After reset BDCR register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  * @note   Prior to enable the LSE Clock Security System, LSE oscillator is to be enabled
  *         with HAL_RCC_OscConfig() and the LSE oscillator clock is to be selected as RTC
  *         clock with HAL_RCCEx_PeriphCLKConfig().
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS(void)
{
  /* Set LSECSSON bit */
  SET_BIT(RCC->BDCR, RCC_BDCR_LSECSSON);
}

/**
  * @brief  Disables the LSE Clock Security System.
  * @note   LSE Clock Security System can only be disabled after a LSE failure detection.
  * @note   After reset BDCR register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  * @retval None
  */
void HAL_RCCEx_DisableLSECSS(void)
{
  /* Unset LSECSSON bit */
  CLEAR_BIT(RCC->BDCR, RCC_BDCR_LSECSSON);
}


/**
  * @brief  Returns the peripheral clock frequency
  * @note   Returns 0 if peripheral clock is unknown
  * @param  PeriphClk: Peripheral clock identifier
  *         This parameter can be a value of :
  *          @ref RCCEx_Periph_Clock_Selection
  *          @ref RCCEx_Periph_One_Clock
  * @retval Frequency in Hz
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint64_t PeriphClk)
{
  uint32_t frequency = 0, clksource = 0;

  PLL2_ClocksTypeDef pll2_clocks;
  PLL3_ClocksTypeDef pll3_clocks;
  PLL4_ClocksTypeDef pll4_clocks;

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClk) || IS_RCC_PERIPHONECLOCK(PeriphClk));

  switch (PeriphClk)
  {

    case RCC_PERIPHCLK_DAC:
    {
      frequency = LSI_VALUE;
    }
      break; /*RCC_PERIPHCLK_DAC*/


    case RCC_PERIPHCLK_WWDG:
    {
      frequency = HAL_RCC_GetPCLK1Freq();
    }
      break; /* RCC_PERIPHCLK_WWDG */


    case RCC_PERIPHCLK_CEC:
    {
      clksource = __HAL_RCC_GET_CEC_SOURCE();

      switch (clksource)
      {
        case RCC_CECCLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_CECCLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;

        case RCC_CECCLKSOURCE_CSI122:
          frequency = (CSI_VALUE / 122);
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_CEC */


    case RCC_PERIPHCLK_I2C12:
    {
      clksource = __HAL_RCC_GET_I2C12_SOURCE();

      switch (clksource)
      {
        case RCC_I2C12CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK1Freq();
          break;

        case RCC_I2C12CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_I2C12CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_I2C12CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_I2C12 */


    case RCC_PERIPHCLK_I2C35:
    {
      clksource = __HAL_RCC_GET_I2C35_SOURCE();

      switch (clksource)
      {
        case RCC_I2C35CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK1Freq();
          break;

        case RCC_I2C35CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_I2C35CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_I2C35CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_I2C35 */


    case RCC_PERIPHCLK_LPTIM1:
    {
      clksource = __HAL_RCC_GET_LPTIM1_SOURCE();

      switch (clksource)
      {
        case RCC_LPTIM1CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK1Freq();
          break;

        case RCC_LPTIM1CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_LPTIM1CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_LPTIM1CLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_LPTIM1CLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;

        case RCC_LPTIM1CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_LPTIM1 */


    case RCC_PERIPHCLK_SPDIFRX:
    {
      clksource = __HAL_RCC_GET_SPDIFRX_SOURCE();

      switch (clksource)
      {
        case RCC_SPDIFRXCLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_SPDIFRXCLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SPDIFRXCLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_SPDIFRX */

    case RCC_PERIPHCLK_SPI23:
    {
      clksource = __HAL_RCC_GET_SPI23_SOURCE();

      switch (clksource)
      {
        case RCC_SPI23CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_SPI23CLKSOURCE_PLL3_Q:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SPI23CLKSOURCE_PLL3_R:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SPI23CLKSOURCE_I2SCKIN:
          frequency = EXTERNAL_CLOCK_VALUE;
          break;

        case RCC_SPI23CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_SPI23 */


    case RCC_PERIPHCLK_UART24:
    {
      clksource = __HAL_RCC_GET_UART24_SOURCE();

      switch (clksource)
      {
        case RCC_UART24CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK1Freq();
          break;

        case RCC_UART24CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_UART24CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_UART24CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_UART24CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_UART24 */


    case RCC_PERIPHCLK_UART35:
    {
      clksource = __HAL_RCC_GET_UART35_SOURCE();

      switch (clksource)
      {
        case RCC_UART35CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK1Freq();
          break;

        case RCC_UART35CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_UART35CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_UART35CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_UART35CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_USART35 */


    case RCC_PERIPHCLK_UART78:
    {
      clksource = __HAL_RCC_GET_UART78_SOURCE();

      switch (clksource)
      {
        case RCC_UART78CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK1Freq();
          break;

        case RCC_UART78CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_UART78CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_UART78CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_UART78CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /*RCC_PERIPHCLK_UART78 */


    case RCC_PERIPHCLK_DFSDM1:
    {
      frequency = HAL_RCC_GetMLHCLKFreq();
    }
    break;//RCC_PERIPHCLK_DFSDM1

#if defined(FDCAN1)
    case RCC_PERIPHCLK_FDCAN:
    {
      clksource = __HAL_RCC_GET_FDCAN_SOURCE();

      switch (clksource)
      {
        case RCC_FDCANCLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        case RCC_FDCANCLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_FDCANCLKSOURCE_PLL4_Q:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_FDCANCLKSOURCE_PLL4_R:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_FDCAN
#endif /*FDCAN1*/

    case RCC_PERIPHCLK_SAI1:
    {
      clksource = __HAL_RCC_GET_SAI1_SOURCE();

      switch (clksource)
      {
        case RCC_SAI1CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SAI1CLKSOURCE_PLL3_Q:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SAI1CLKSOURCE_PLL3_R:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SAI1CLKSOURCE_I2SCKIN:
          frequency = EXTERNAL_CLOCK_VALUE;
          break;

        case RCC_SAI1CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SAI1


    case RCC_PERIPHCLK_SAI2:
    {
      clksource = __HAL_RCC_GET_SAI2_SOURCE();

      switch (clksource)
      {
        case RCC_SAI2CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SAI2CLKSOURCE_PLL3_Q:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SAI2CLKSOURCE_PLL3_R:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SAI2CLKSOURCE_I2SCKIN:
          frequency = EXTERNAL_CLOCK_VALUE;
          break;

        case RCC_SAI2CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        case RCC_SAI2CLKSOURCE_SPDIF:
          frequency = 0; //SAI2 manage this SPDIF_CKSYMB_VALUE
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SAI2


    case RCC_PERIPHCLK_SAI3:
    {
      clksource = __HAL_RCC_GET_SAI3_SOURCE();

      switch (clksource)
      {
        case RCC_SAI3CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SAI3CLKSOURCE_PLL3_Q:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SAI3CLKSOURCE_PLL3_R:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SAI3CLKSOURCE_I2SCKIN:
          frequency = EXTERNAL_CLOCK_VALUE;
          break;

        case RCC_SAI2CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SAI3


    case RCC_PERIPHCLK_SPI1:
    {
      clksource = __HAL_RCC_GET_SPI1_SOURCE();

      switch (clksource)
      {
        case RCC_SPI1CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SPI1CLKSOURCE_PLL3_Q:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SPI1CLKSOURCE_PLL3_R:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SPI1CLKSOURCE_I2SCKIN:
          frequency = EXTERNAL_CLOCK_VALUE;
          break;

        case RCC_SPI1CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SPI1


    case RCC_PERIPHCLK_SPI45:
    {
      clksource = __HAL_RCC_GET_SPI45_SOURCE();

      switch (clksource)
      {
        case RCC_SPI45CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK2Freq();
          break;

        case RCC_SPI45CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SPI45CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_SPI45CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_SPI45CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_SPI45 */


    case RCC_PERIPHCLK_USART6:
    {
      clksource = __HAL_RCC_GET_USART6_SOURCE();

      switch (clksource)
      {
        case RCC_USART6CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK2Freq();
          break;

        case RCC_USART6CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_USART6CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_USART6CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_USART6CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_USART6

    case RCC_PERIPHCLK_LPTIM23:
    {
      clksource = __HAL_RCC_GET_LPTIM23_SOURCE();

      switch (clksource)
      {
        case RCC_LPTIM23CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK3Freq();
          break;

        case RCC_LPTIM23CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_LPTIM23CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        case RCC_LPTIM23CLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_LPTIM23CLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_LPTIM23 */

    case RCC_PERIPHCLK_LPTIM45:
    {
      clksource = __HAL_RCC_GET_LPTIM45_SOURCE();

      switch (clksource)
      {
        case RCC_LPTIM45CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK3Freq();
          break;

        case RCC_LPTIM45CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_LPTIM45CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_LPTIM45CLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_LPTIM45CLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;

        case RCC_LPTIM45CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_LPTIM45 */


    case RCC_PERIPHCLK_SAI4:
    {
      clksource = __HAL_RCC_GET_SAI4_SOURCE();

      switch (clksource)
      {
        case RCC_SAI4CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SAI4CLKSOURCE_PLL3_Q:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_SAI4CLKSOURCE_PLL3_R:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SAI4CLKSOURCE_I2SCKIN:
          frequency = EXTERNAL_CLOCK_VALUE;
          break;

        case RCC_SAI4CLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SAI4


    case RCC_PERIPHCLK_TEMP:
    {
      frequency = LSE_VALUE;
    }
    break;//RCC_PERIPHCLK_TEMP


#if defined(DSI)
    case RCC_PERIPHCLK_DSI:
    {
      clksource = __HAL_RCC_GET_DSI_SOURCE();

      switch (clksource)
      {
        case RCC_DSICLKSOURCE_PHY:
          /* It has no sense to ask for DSIPHY freq. because it is generated
           * by DSI itself, so send back 0 as kernel frequency
           */
          frequency = 0;
          break;

        case RCC_DSICLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;
      }
    }
    break;//RCC_PERIPHCLK_DSI
#endif /*DSI*/

    case RCC_PERIPHCLK_LTDC:
    {
      HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
      frequency = pll4_clocks.PLL4_Q_Frequency;
    }
    break;//RCC_PERIPHCLK_LTDC


    case RCC_PERIPHCLK_USBPHY:
    {
      clksource = __HAL_RCC_GET_USBPHY_SOURCE();

      switch (clksource)
      {
        case RCC_USBPHYCLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        case RCC_USBPHYCLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_USBPHYCLKSOURCE_HSE2:
          frequency = (HSE_VALUE / 2);
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_USBPHY


    case RCC_PERIPHCLK_IWDG2:
    {
      frequency = LSI_VALUE;
    }
    break;//RCC_PERIPHCLK_IWDG2


    case RCC_PERIPHCLK_DDRPHYC:
    {
      HAL_RCC_GetPLL2ClockFreq(&pll2_clocks);
      frequency = pll2_clocks.PLL2_R_Frequency;
    }
    break;//RCC_PERIPHCLK_DDRPHYC


    case RCC_PERIPHCLK_RTC:
    {
      clksource = __HAL_RCC_GET_RTC_SOURCE();

      switch (clksource)
      {
        case RCC_RTCCLKSOURCE_OFF:
          frequency = 0;
          break;

        case RCC_RTCCLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_RTCCLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;

        case RCC_RTCCLKSOURCE_HSE_DIV:
          frequency = (HSE_VALUE / __HAL_RCC_GET_RTC_HSEDIV());
          break;
      }
    }
    break;//RCC_PERIPHCLK_RTC


    case RCC_PERIPHCLK_IWDG1:
    {
      frequency = LSI_VALUE;
    }
    break;//RCC_PERIPHCLK_IWDG1


    case RCC_PERIPHCLK_I2C46:
    {
      clksource = __HAL_RCC_GET_I2C46_SOURCE();

      switch (clksource)
      {
        case RCC_I2C46CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK5Freq();
          break;

        case RCC_I2C46CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_I2C46CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_I2C46CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_I2C46 */


    case RCC_PERIPHCLK_SPI6:
    {
      clksource = __HAL_RCC_GET_SPI6_SOURCE();

      switch (clksource)
      {
        case RCC_SPI6CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK5Freq();
          break;

        case RCC_SPI6CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_SPI6CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_SPI6CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_SPI6CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        case RCC_SPI6CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SPI6

    case RCC_PERIPHCLK_USART1:
    {
      clksource = __HAL_RCC_GET_USART1_SOURCE();

      switch (clksource)
      {
        case RCC_USART1CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetPCLK5Freq();
          break;

        case RCC_USART1CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        case RCC_USART1CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_USART1CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_USART1CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_Q_Frequency;
          break;

        case RCC_USART1CLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_USART1


    case RCC_PERIPHCLK_STGEN:
    {
      clksource = __HAL_RCC_GET_STGEN_SOURCE();

      switch (clksource)
      {
        case RCC_STGENCLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        case RCC_STGENCLKSOURCE_HSE:
          frequency = HSE_VALUE;
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_STGEN


    case RCC_PERIPHCLK_QSPI:
    {
      clksource = __HAL_RCC_GET_QSPI_SOURCE();

      switch (clksource)
      {
        case RCC_QSPICLKSOURCE_BCLK:
          frequency = HAL_RCC_GetACLKFreq();
          break;

        case RCC_QSPICLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_QSPICLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_QSPICLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_QSPI


    case RCC_PERIPHCLK_ETH:
    {
      clksource = __HAL_RCC_GET_ETH_SOURCE();

      switch (clksource)
      {
        case RCC_ETHCLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_ETHCLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;
          break;

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_ETH


    case RCC_PERIPHCLK_FMC:
    {
      clksource = __HAL_RCC_GET_FMC_SOURCE();

      switch (clksource)
      {
        case RCC_FMCCLKSOURCE_BCLK:
          frequency = HAL_RCC_GetACLKFreq();
          break;

        case RCC_FMCCLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_FMCCLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_FMCCLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

      }
    }
    break;//RCC_PERIPHCLK_FMC


    case RCC_PERIPHCLK_GPU:
    {
      HAL_RCC_GetPLL2ClockFreq(&pll2_clocks);
      frequency = pll2_clocks.PLL2_Q_Frequency;
    }
    break;//RCC_PERIPHCLK_GPU


    case RCC_PERIPHCLK_USBO:
    {
      clksource = __HAL_RCC_GET_USBO_SOURCE();

      switch (clksource)
      {
        case RCC_USBOCLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_USBOCLKSOURCE_PHY:
          frequency = USB_PHY_VALUE;
          break;
      }
    }
    break;//RCC_PERIPHCLK_USBO


    case RCC_PERIPHCLK_SDMMC3:
    {
      clksource = __HAL_RCC_GET_SDMMC3_SOURCE();

      switch (clksource)
      {
        case RCC_SDMMC3CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetHCLK2Freq();
          break;

        case RCC_SDMMC3CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SDMMC3CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_SDMMC3CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());

        default:
          frequency = 0;
          break;
      }
    }
    break;//RCC_PERIPHCLK_SDMMC3


    case RCC_PERIPHCLK_ADC:
    {
      clksource = __HAL_RCC_GET_ADC_SOURCE();

      switch (clksource)
      {
        case RCC_ADCCLKSOURCE_PLL4:
            HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
            frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_ADCCLKSOURCE_PER:
          frequency = RCC_GetCKPERFreq();
          break;

        case RCC_ADCCLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_Q_Frequency;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_ADC */


    case RCC_PERIPHCLK_RNG2:
    {
      clksource = __HAL_RCC_GET_RNG2_SOURCE();

      switch (clksource)
      {
        case RCC_RNG2CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_RNG2CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_RNG2CLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_RNG2CLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;
      }
    }
    break;//RCC_PERIPHCLK_RNG2


    case RCC_PERIPHCLK_RNG1:
    {
      clksource = __HAL_RCC_GET_RNG1_SOURCE();

      switch (clksource)
      {
        case RCC_RNG1CLKSOURCE_CSI:
          frequency = CSI_VALUE;
          break;

        case RCC_RNG1CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_R_Frequency;
          break;

        case RCC_RNG1CLKSOURCE_LSE:
          frequency = LSE_VALUE;
          break;

        case RCC_RNG1CLKSOURCE_LSI:
          frequency = LSI_VALUE;
          break;
      }
    }
    break;//RCC_PERIPHCLK_RNG1

    case RCC_PERIPHCLK_SDMMC12:
    {
      clksource = __HAL_RCC_GET_SDMMC12_SOURCE();

      switch (clksource)
      {
        case RCC_SDMMC12CLKSOURCE_BCLK:
          frequency = HAL_RCC_GetHCLK6Freq();
          break;

        case RCC_SDMMC12CLKSOURCE_PLL3:
          HAL_RCC_GetPLL3ClockFreq(&pll3_clocks);
          frequency = pll3_clocks.PLL3_R_Frequency;
          break;

        case RCC_SDMMC12CLKSOURCE_PLL4:
          HAL_RCC_GetPLL4ClockFreq(&pll4_clocks);
          frequency = pll4_clocks.PLL4_P_Frequency;
          break;

        case RCC_SDMMC12CLKSOURCE_HSI:
          frequency = (HSI_VALUE >> __HAL_RCC_GET_HSI_DIV());
          break;

        default:
          frequency = 0;
          break;
      }
    }
      break; /* RCC_PERIPHCLK_SDMMC12 */

    case RCC_PERIPHCLK_TIMG1:
    {
      frequency = HAL_RCC_GetMCUFreq();
      if (__HAL_RCC_GET_TIMG1PRES() == RCC_TIMG1PRES_ACTIVATED)
      {
        switch (__HAL_RCC_GET_APB1_DIV())
        {
          case RCC_APB1_DIV1:
          case RCC_APB1_DIV2:
          case RCC_APB1_DIV4:
            break;
          case RCC_APB1_DIV8:
            frequency /= 2;
            break;
          case RCC_APB1_DIV16:
            frequency /= 4;
            break;
        }
      }
      else
      {
        switch (__HAL_RCC_GET_APB1_DIV())
        {
          case RCC_APB1_DIV1:
          case RCC_APB1_DIV2:
            break;
          case RCC_APB1_DIV4:
            frequency /= 2;
            break;
          case RCC_APB1_DIV8:
            frequency /= 4;
            break;
          case RCC_APB1_DIV16:
            frequency /= 8;
            break;
        }
      }
    }
    break;


    case RCC_PERIPHCLK_TIMG2:
    {
      frequency = HAL_RCC_GetMCUFreq();
      if (__HAL_RCC_GET_TIMG2PRES() == RCC_TIMG2PRES_ACTIVATED)
      {
        switch (__HAL_RCC_GET_APB2_DIV())
        {
          case RCC_APB2_DIV1:
          case RCC_APB2_DIV2:
          case RCC_APB2_DIV4:
            break;
          case RCC_APB2_DIV8:
            frequency /= 2;
            break;
          case RCC_APB2_DIV16:
            frequency /= 4;
            break;
        }
      }
      else
      {
        switch (__HAL_RCC_GET_APB2_DIV())
        {
          case RCC_APB2_DIV1:
          case RCC_APB2_DIV2:
            break;
          case RCC_APB2_DIV4:
            frequency /= 2;
            break;
          case RCC_APB2_DIV8:
            frequency /= 4;
            break;
          case RCC_APB2_DIV16:
            frequency /= 8;
            break;
        }
      }
    }
    break;

  }

  return (frequency);
}

#ifdef CORE_CA7
/**
  * @brief  Control the enable boot function when the system exits from STANDBY
  * @param  RCC_BootCx: Boot Core to be enabled (set to 1)
  *         This parameter can be one (or both) of the following values:
  *            @arg RCC_BOOT_C1: CA7 core selection
  *            @arg RCC_BOOT_C2: CM4 core selection
  *
  * @note   Next combinations are possible:
  *           RCC_BOOT_C1   RCC_BOOT_C2  Expected result
  *               0              0       MPU boots, MCU does not boot
  *               0              1       Only MCU boots
  *               1              0       Only MPU boots
  *               1              1       MPU and MCU boot
  *
  * @note   This function is reset when a system reset occurs, but not when the
  *         circuit exits from STANDBY (rst_app reset)
  * @note   This function can only be called by the CA7
  * @retval None
  */
void HAL_RCCEx_EnableBootCore(uint32_t RCC_BootCx)
{
  assert_param(IS_RCC_BOOT_CORE(RCC_BootCx));
  SET_BIT(RCC->MP_BOOTCR, RCC_BootCx);
}

/**
  * @brief  Control the disable boot function when the system exits from STANDBY
  * @param  RCC_BootCx: Boot Core to be disabled (set to 0)
  *         This parameter can be one (or both) of the following values:
  *            @arg RCC_BOOT_C1: CA7 core selection
  *            @arg RCC_BOOT_C2: CM4 core selection
  *
  * @note   Next combinations are possible:
  *           RCC_BOOT_C1   RCC_BOOT_C2  Expected result
  *               0              0       MPU boots, MCU does not boot
  *               0              1       Only MCU boots
  *               1              0       Only MPU boots
  *               1              1       MPU and MCU boot
  *
  * @note   This function is reset when a system reset occurs, but not when the
  *         circuit exits from STANDBY (rst_app reset)
  * @note   This function can only be called by the CA7
  * @retval None
  */
void HAL_RCCEx_DisableBootCore(uint32_t RCC_BootCx)
{
  assert_param(IS_RCC_BOOT_CORE(RCC_BootCx));
  CLEAR_BIT(RCC->MP_BOOTCR, RCC_BootCx);
}


/**
  * @brief  The MCU will be set in HOLD_BOOT when the next MCU core reset occurs
  * @retval None
  */
void HAL_RCCEx_HoldBootMCU(void)
{
  CLEAR_BIT(RCC->MP_GCR, RCC_MP_GCR_BOOT_MCU);
}

/**
  * @brief  The MCU will not be in HOLD_BOOT mode when the next MCU core reset occurs.
  * @note   If the MCU was in HOLD_BOOT it will cause the MCU to boot.
  * @retval None
  */
void HAL_RCCEx_BootMCU(void)
{
  SET_BIT(RCC->MP_GCR, RCC_MP_GCR_BOOT_MCU);
}

#endif /*CORE_CA7*/

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

