/**
  ******************************************************************************
  * @file    stm32f0xx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @brief   Extended RCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities RCC extension peripheral:
  *           + Extended Peripheral Control functions
  *           + Extended Clock Recovery System Control functions
  *
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

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/** @addtogroup STM32F0xx_HAL_Driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/** @defgroup RCCEx RCCEx
  * @brief RCC Extension HAL module driver.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#if defined(CRS)
/** @defgroup RCCEx_Private_Constants RCCEx Private Constants
  * @{
  */
/* Bit position in register */
#define CRS_CFGR_FELIM_BITNUMBER    16
#define CRS_CR_TRIM_BITNUMBER       8
#define CRS_ISR_FECAP_BITNUMBER     16
/**
  * @}
  */
#endif /* CRS */

/* Private macro -------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Macros RCCEx Private Macros
  * @{
  */
/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup RCCEx_Exported_Functions RCCEx Exported Functions
  * @{
  */

/** @defgroup RCCEx_Exported_Functions_Group1 Extended Peripheral Control functions
  * @brief    Extended Peripheral Control functions
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
        the backup registers) are set to their reset values.

@endverbatim
  * @{
  */

/**
  * @brief  Initializes the RCC extended peripherals clocks according to the specified
  *         parameters in the RCC_PeriphCLKInitTypeDef.
  * @param  PeriphClkInit pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         contains the configuration information for the Extended Peripherals clocks
  *         (USART, RTC, I2C, CEC and USB).
  *
  * @note   Care must be taken when @ref HAL_RCCEx_PeriphCLKConfig() is used to select
  *         the RTC clock source; in this case the Backup domain will be reset in
  *         order to modify the RTC Clock source, as consequence RTC registers (including
  *         the backup registers) and RCC_BDCR register are set to their reset values.
  *
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t tickstart = 0U;
  uint32_t temp_reg = 0U;

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClkInit->PeriphClockSelection));

  /*---------------------------- RTC configuration -------------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == (RCC_PERIPHCLK_RTC))
  {
    /* check for RTC Parameters used to output RTCCLK */
    assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->RTCClockSelection));

    FlagStatus       pwrclkchanged = RESET;

    /* As soon as function is called to change RTC clock source, activation of the
       power domain is done. */
    /* Requires to enable write access to Backup Domain of necessary */
    if(__HAL_RCC_PWR_IS_CLK_DISABLED())
    {
    __HAL_RCC_PWR_CLK_ENABLE();
      pwrclkchanged = SET;
    }

    if(HAL_IS_BIT_CLR(PWR->CR, PWR_CR_DBP))
    {
      /* Enable write access to Backup domain */
      SET_BIT(PWR->CR, PWR_CR_DBP);

      /* Wait for Backup domain Write protection disable */
      tickstart = HAL_GetTick();

      while(HAL_IS_BIT_CLR(PWR->CR, PWR_CR_DBP))
      {
        if((HAL_GetTick() - tickstart) > RCC_DBP_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }

    /* Reset the Backup domain only if the RTC Clock source selection is modified from reset value */
    temp_reg = (RCC->BDCR & RCC_BDCR_RTCSEL);
    if((temp_reg != 0x00000000U) && (temp_reg != (PeriphClkInit->RTCClockSelection & RCC_BDCR_RTCSEL)))
    {
      /* Store the content of BDCR register before the reset of Backup Domain */
      temp_reg = (RCC->BDCR & ~(RCC_BDCR_RTCSEL));
      /* RTC Clock selection can be changed only if the Backup Domain is reset */
      __HAL_RCC_BACKUPRESET_FORCE();
      __HAL_RCC_BACKUPRESET_RELEASE();
      /* Restore the Content of BDCR register */
      RCC->BDCR = temp_reg;

      /* Wait for LSERDY if LSE was enabled */
      if (HAL_IS_BIT_SET(temp_reg, RCC_BDCR_LSEON))
      {
        /* Get Start Tick */
        tickstart = HAL_GetTick();

        /* Wait till LSE is ready */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
        {
          if((HAL_GetTick() - tickstart) > RCC_LSE_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
    __HAL_RCC_RTC_CONFIG(PeriphClkInit->RTCClockSelection);

    /* Require to disable power clock if necessary */
    if(pwrclkchanged == SET)
    {
      __HAL_RCC_PWR_CLK_DISABLE();
    }
  }

  /*------------------------------- USART1 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART1) == RCC_PERIPHCLK_USART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART1CLKSOURCE(PeriphClkInit->Usart1ClockSelection));

    /* Configure the USART1 clock source */
    __HAL_RCC_USART1_CONFIG(PeriphClkInit->Usart1ClockSelection);
  }

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)
  /*----------------------------- USART2 Configuration --------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART2) == RCC_PERIPHCLK_USART2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART2CLKSOURCE(PeriphClkInit->Usart2ClockSelection));

    /* Configure the USART2 clock source */
    __HAL_RCC_USART2_CONFIG(PeriphClkInit->Usart2ClockSelection);
  }
#endif /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F091xC) || defined(STM32F098xx)
  /*----------------------------- USART3 Configuration --------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART3) == RCC_PERIPHCLK_USART3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART3CLKSOURCE(PeriphClkInit->Usart3ClockSelection));

    /* Configure the USART3 clock source */
    __HAL_RCC_USART3_CONFIG(PeriphClkInit->Usart3ClockSelection);
  }
#endif /* STM32F091xC || STM32F098xx */

  /*------------------------------ I2C1 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C1) == RCC_PERIPHCLK_I2C1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C1CLKSOURCE(PeriphClkInit->I2c1ClockSelection));

    /* Configure the I2C1 clock source */
    __HAL_RCC_I2C1_CONFIG(PeriphClkInit->I2c1ClockSelection);
  }

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB) || defined(STM32F070x6)
  /*------------------------------ USB Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USB) == RCC_PERIPHCLK_USB)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USBCLKSOURCE(PeriphClkInit->UsbClockSelection));

    /* Configure the USB clock source */
    __HAL_RCC_USB_CONFIG(PeriphClkInit->UsbClockSelection);
  }
#endif /* STM32F042x6 || STM32F048xx || STM32F072xB || STM32F078xx || STM32F070xB || STM32F070x6 */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)
  /*------------------------------ CEC clock Configuration -------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_CEC) == RCC_PERIPHCLK_CEC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_CECCLKSOURCE(PeriphClkInit->CecClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_CEC_CONFIG(PeriphClkInit->CecClockSelection);
  }
#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

  return HAL_OK;
}

/**
  * @brief  Get the RCC_ClkInitStruct according to the internal
  * RCC configuration registers.
  * @param  PeriphClkInit pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         returns the configuration information for the Extended Peripherals clocks
  *         (USART, RTC, I2C, CEC and USB).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  /* Set all possible values for the extended clock type parameter------------*/
  /* Common part first */
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_RTC;
  /* Get the RTC configuration --------------------------------------------*/
  PeriphClkInit->RTCClockSelection = __HAL_RCC_GET_RTC_SOURCE();
  /* Get the USART1 clock configuration --------------------------------------------*/
  PeriphClkInit->Usart1ClockSelection = __HAL_RCC_GET_USART1_SOURCE();
  /* Get the I2C1 clock source -----------------------------------------------*/
  PeriphClkInit->I2c1ClockSelection = __HAL_RCC_GET_I2C1_SOURCE();

#if defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_USART2;
  /* Get the USART2 clock source ---------------------------------------------*/
  PeriphClkInit->Usart2ClockSelection = __HAL_RCC_GET_USART2_SOURCE();
#endif /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

#if defined(STM32F091xC) || defined(STM32F098xx)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_USART3;
  /* Get the USART3 clock source ---------------------------------------------*/
  PeriphClkInit->Usart3ClockSelection = __HAL_RCC_GET_USART3_SOURCE();
#endif /* STM32F091xC || STM32F098xx */

#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F072xB) || defined(STM32F078xx) || defined(STM32F070xB) || defined(STM32F070x6)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_USB;
  /* Get the USB clock source ---------------------------------------------*/
  PeriphClkInit->UsbClockSelection = __HAL_RCC_GET_USB_SOURCE();
#endif /* STM32F042x6 || STM32F048xx || STM32F072xB || STM32F078xx || STM32F070xB || STM32F070x6 */

#if defined(STM32F042x6) || defined(STM32F048xx)\
 || defined(STM32F051x8) || defined(STM32F058xx)\
 || defined(STM32F071xB) || defined(STM32F072xB) || defined(STM32F078xx)\
 || defined(STM32F091xC) || defined(STM32F098xx)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_CEC;
  /* Get the CEC clock source ------------------------------------------------*/
  PeriphClkInit->CecClockSelection = __HAL_RCC_GET_CEC_SOURCE();
#endif /* STM32F042x6 || STM32F048xx ||                */
       /* STM32F051x8 || STM32F058xx ||                */
       /* STM32F071xB || STM32F072xB || STM32F078xx || */
       /* STM32F091xC || STM32F098xx */

}

/**
  * @brief  Returns the peripheral clock frequency
  * @note   Returns 0 if peripheral clock is unknown
  * @param  PeriphClk Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_PERIPHCLK_RTC     RTC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART1  USART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C1    I2C1 peripheral clock
  @if STM32F042x6
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F048xx
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F051x8
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F058xx
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F070x6
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  @endif
  @if STM32F070xB
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  @endif
  @if STM32F071xB
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F072xB
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F078xx
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F091xC
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F098xx
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  * @retval Frequency in Hz (0: means that no available frequency for the peripheral)
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
  /* frequency == 0 : means that no available frequency for the peripheral */
  uint32_t frequency = 0U;

  uint32_t srcclk = 0U;
#if defined(USB)
  uint32_t pllmull = 0U, pllsource = 0U, predivfactor = 0U;
#endif /* USB */

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClk));

  switch (PeriphClk)
  {
  case RCC_PERIPHCLK_RTC:
    {
      /* Get the current RTC source */
      srcclk = __HAL_RCC_GET_RTC_SOURCE();

      /* Check if LSE is ready and if RTC clock selection is LSE */
      if ((srcclk == RCC_RTCCLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Check if LSI is ready and if RTC clock selection is LSI */
      else if ((srcclk == RCC_RTCCLKSOURCE_LSI) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSIRDY)))
      {
        frequency = LSI_VALUE;
      }
      /* Check if HSE is ready  and if RTC clock selection is HSI_DIV32*/
      else if ((srcclk == RCC_RTCCLKSOURCE_HSE_DIV32) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY)))
      {
        frequency = HSE_VALUE / 32U;
      }
      break;
    }
  case RCC_PERIPHCLK_USART1:
    {
      /* Get the current USART1 source */
      srcclk = __HAL_RCC_GET_USART1_SOURCE();

      /* Check if USART1 clock selection is PCLK1 */
      if (srcclk == RCC_USART1CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if USART1 clock selection is HSI */
      else if ((srcclk == RCC_USART1CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if USART1 clock selection is SYSCLK */
      else if (srcclk == RCC_USART1CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Check if LSE is ready  and if USART1 clock selection is LSE */
      else if ((srcclk == RCC_USART1CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      break;
    }
#if defined(RCC_CFGR3_USART2SW)
  case RCC_PERIPHCLK_USART2:
    {
      /* Get the current USART2 source */
      srcclk = __HAL_RCC_GET_USART2_SOURCE();

      /* Check if USART2 clock selection is PCLK1 */
      if (srcclk == RCC_USART2CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if USART2 clock selection is HSI */
      else if ((srcclk == RCC_USART2CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if USART2 clock selection is SYSCLK */
      else if (srcclk == RCC_USART2CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Check if LSE is ready  and if USART2 clock selection is LSE */
      else if ((srcclk == RCC_USART2CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      break;
    }
#endif /* RCC_CFGR3_USART2SW */
#if defined(RCC_CFGR3_USART3SW)
  case RCC_PERIPHCLK_USART3:
    {
      /* Get the current USART3 source */
      srcclk = __HAL_RCC_GET_USART3_SOURCE();

      /* Check if USART3 clock selection is PCLK1 */
      if (srcclk == RCC_USART3CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if USART3 clock selection is HSI */
      else if ((srcclk == RCC_USART3CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if USART3 clock selection is SYSCLK */
      else if (srcclk == RCC_USART3CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Check if LSE is ready  and if USART3 clock selection is LSE */
      else if ((srcclk == RCC_USART3CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      break;
    }
#endif /* RCC_CFGR3_USART3SW */
  case RCC_PERIPHCLK_I2C1:
    {
      /* Get the current I2C1 source */
      srcclk = __HAL_RCC_GET_I2C1_SOURCE();

      /* Check if HSI is ready and if I2C1 clock selection is HSI */
      if ((srcclk == RCC_I2C1CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if I2C1 clock selection is SYSCLK */
      else if (srcclk == RCC_I2C1CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      break;
    }
#if defined(USB)
  case RCC_PERIPHCLK_USB:
    {
      /* Get the current USB source */
      srcclk = __HAL_RCC_GET_USB_SOURCE();

      /* Check if PLL is ready and if USB clock selection is PLL */
      if ((srcclk == RCC_USBCLKSOURCE_PLL) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        /* Get PLL clock source and multiplication factor ----------------------*/
        pllmull      = RCC->CFGR & RCC_CFGR_PLLMUL;
        pllsource    = RCC->CFGR & RCC_CFGR_PLLSRC;
        pllmull      = (pllmull >> RCC_CFGR_PLLMUL_BITNUMBER) + 2U;
        predivfactor = (RCC->CFGR2 & RCC_CFGR2_PREDIV) + 1U;

        if (pllsource == RCC_CFGR_PLLSRC_HSE_PREDIV)
        {
          /* HSE used as PLL clock source : frequency = HSE/PREDIV * PLLMUL */
          frequency = (HSE_VALUE/predivfactor) * pllmull;
        }
#if defined(RCC_CR2_HSI48ON)
        else if (pllsource == RCC_CFGR_PLLSRC_HSI48_PREDIV)
        {
          /* HSI48 used as PLL clock source : frequency = HSI48/PREDIV * PLLMUL */
          frequency = (HSI48_VALUE / predivfactor) * pllmull;
        }
#endif /* RCC_CR2_HSI48ON */
        else
        {
#if defined(STM32F042x6) || defined(STM32F048xx) || defined(STM32F078xx) || defined(STM32F072xB) || defined(STM32F070xB)
          /* HSI used as PLL clock source : frequency = HSI/PREDIV * PLLMUL */
          frequency = (HSI_VALUE / predivfactor) * pllmull;
#else
          /* HSI used as PLL clock source : frequency = HSI/2U * PLLMUL */
          frequency = (HSI_VALUE >> 1U) * pllmull;
#endif /* STM32F042x6 || STM32F048xx || STM32F072xB || STM32F078xx || STM32F070xB */
        }
      }
#if defined(RCC_CR2_HSI48ON)
      /* Check if HSI48 is ready and if USB clock selection is HSI48 */
      else if ((srcclk == RCC_USBCLKSOURCE_HSI48) && (HAL_IS_BIT_SET(RCC->CR2, RCC_CR2_HSI48RDY)))
      {
        frequency = HSI48_VALUE;
      }
#endif /* RCC_CR2_HSI48ON */
      break;
    }
#endif /* USB */
#if defined(CEC)
  case RCC_PERIPHCLK_CEC:
    {
      /* Get the current CEC source */
      srcclk = __HAL_RCC_GET_CEC_SOURCE();

      /* Check if HSI is ready and if CEC clock selection is HSI */
      if ((srcclk == RCC_CECCLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if LSE is ready  and if CEC clock selection is LSE */
      else if ((srcclk == RCC_CECCLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      break;
    }
#endif /* CEC */
  default:
    {
      break;
    }
  }
  return(frequency);
}

/**
  * @}
  */

#if defined(CRS)

/** @defgroup RCCEx_Exported_Functions_Group3 Extended Clock Recovery System Control functions
 *  @brief  Extended Clock Recovery System Control functions
 *
@verbatim
 ===============================================================================
                ##### Extended Clock Recovery System Control functions  #####
 ===============================================================================
    [..]
      For devices with Clock Recovery System feature (CRS), RCC Extention HAL driver can be used as follows:

      (#) In System clock config, HSI48 needs to be enabled

      (#) Enable CRS clock in IP MSP init which will use CRS functions

      (#) Call CRS functions as follows:
          (##) Prepare synchronization configuration necessary for HSI48 calibration
              (+++) Default values can be set for frequency Error Measurement (reload and error limit)
                        and also HSI48 oscillator smooth trimming.
              (+++) Macro @ref __HAL_RCC_CRS_RELOADVALUE_CALCULATE can be also used to calculate
                        directly reload value with target and synchronization frequencies values
          (##) Call function @ref HAL_RCCEx_CRSConfig which
              (+++) Reset CRS registers to their default values.
              (+++) Configure CRS registers with synchronization configuration
              (+++) Enable automatic calibration and frequency error counter feature
           Note: When using USB LPM (Link Power Management) and the device is in Sleep mode, the
           periodic USB SOF will not be generated by the host. No SYNC signal will therefore be
           provided to the CRS to calibrate the HSI48 on the run. To guarantee the required clock
           precision after waking up from Sleep mode, the LSE or reference clock on the GPIOs
           should be used as SYNC signal.

          (##) A polling function is provided to wait for complete synchronization
              (+++) Call function @ref HAL_RCCEx_CRSWaitSynchronization()
              (+++) According to CRS status, user can decide to adjust again the calibration or continue
                        application if synchronization is OK

      (#) User can retrieve information related to synchronization in calling function
            @ref HAL_RCCEx_CRSGetSynchronizationInfo()

      (#) Regarding synchronization status and synchronization information, user can try a new calibration
           in changing synchronization configuration and call again HAL_RCCEx_CRSConfig.
           Note: When the SYNC event is detected during the downcounting phase (before reaching the zero value),
           it means that the actual frequency is lower than the target (and so, that the TRIM value should be
           incremented), while when it is detected during the upcounting phase it means that the actual frequency
           is higher (and that the TRIM value should be decremented).

      (#) In interrupt mode, user can resort to the available macros (__HAL_RCC_CRS_XXX_IT). Interrupts will go
          through CRS Handler (RCC_IRQn/RCC_IRQHandler)
              (++) Call function @ref HAL_RCCEx_CRSConfig()
              (++) Enable RCC_IRQn (thanks to NVIC functions)
              (++) Enable CRS interrupt (@ref __HAL_RCC_CRS_ENABLE_IT)
              (++) Implement CRS status management in the following user callbacks called from
                   HAL_RCCEx_CRS_IRQHandler():
                   (+++) @ref HAL_RCCEx_CRS_SyncOkCallback()
                   (+++) @ref HAL_RCCEx_CRS_SyncWarnCallback()
                   (+++) @ref HAL_RCCEx_CRS_ExpectedSyncCallback()
                   (+++) @ref HAL_RCCEx_CRS_ErrorCallback()

      (#) To force a SYNC EVENT, user can use the function @ref HAL_RCCEx_CRSSoftwareSynchronizationGenerate().
          This function can be called before calling @ref HAL_RCCEx_CRSConfig (for instance in Systick handler)

@endverbatim
 * @{
 */

/**
  * @brief  Start automatic synchronization for polling mode
  * @param  pInit Pointer on RCC_CRSInitTypeDef structure
  * @retval None
  */
void HAL_RCCEx_CRSConfig(RCC_CRSInitTypeDef *pInit)
{
  uint32_t value = 0U;

  /* Check the parameters */
  assert_param(IS_RCC_CRS_SYNC_DIV(pInit->Prescaler));
  assert_param(IS_RCC_CRS_SYNC_SOURCE(pInit->Source));
  assert_param(IS_RCC_CRS_SYNC_POLARITY(pInit->Polarity));
  assert_param(IS_RCC_CRS_RELOADVALUE(pInit->ReloadValue));
  assert_param(IS_RCC_CRS_ERRORLIMIT(pInit->ErrorLimitValue));
  assert_param(IS_RCC_CRS_HSI48CALIBRATION(pInit->HSI48CalibrationValue));

  /* CONFIGURATION */

  /* Before configuration, reset CRS registers to their default values*/
  __HAL_RCC_CRS_FORCE_RESET();
  __HAL_RCC_CRS_RELEASE_RESET();

  /* Set the SYNCDIV[2:0] bits according to Prescaler value */
  /* Set the SYNCSRC[1:0] bits according to Source value */
  /* Set the SYNCSPOL bit according to Polarity value */
  value = (pInit->Prescaler | pInit->Source | pInit->Polarity);
  /* Set the RELOAD[15:0] bits according to ReloadValue value */
  value |= pInit->ReloadValue;
  /* Set the FELIM[7:0] bits according to ErrorLimitValue value */
  value |= (pInit->ErrorLimitValue << CRS_CFGR_FELIM_BITNUMBER);
  WRITE_REG(CRS->CFGR, value);

  /* Adjust HSI48 oscillator smooth trimming */
  /* Set the TRIM[5:0] bits according to RCC_CRS_HSI48CalibrationValue value */
  MODIFY_REG(CRS->CR, CRS_CR_TRIM, (pInit->HSI48CalibrationValue << CRS_CR_TRIM_BITNUMBER));

  /* START AUTOMATIC SYNCHRONIZATION*/

  /* Enable Automatic trimming & Frequency error counter */
  SET_BIT(CRS->CR, CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
}

/**
  * @brief  Generate the software synchronization event
  * @retval None
  */
void HAL_RCCEx_CRSSoftwareSynchronizationGenerate(void)
{
  SET_BIT(CRS->CR, CRS_CR_SWSYNC);
}

/**
  * @brief  Return synchronization info
  * @param  pSynchroInfo Pointer on RCC_CRSSynchroInfoTypeDef structure
  * @retval None
  */
void HAL_RCCEx_CRSGetSynchronizationInfo(RCC_CRSSynchroInfoTypeDef *pSynchroInfo)
{
  /* Check the parameter */
  assert_param(pSynchroInfo != NULL);

  /* Get the reload value */
  pSynchroInfo->ReloadValue = (uint32_t)(READ_BIT(CRS->CFGR, CRS_CFGR_RELOAD));

  /* Get HSI48 oscillator smooth trimming */
  pSynchroInfo->HSI48CalibrationValue = (uint32_t)(READ_BIT(CRS->CR, CRS_CR_TRIM) >> CRS_CR_TRIM_BITNUMBER);

  /* Get Frequency error capture */
  pSynchroInfo->FreqErrorCapture = (uint32_t)(READ_BIT(CRS->ISR, CRS_ISR_FECAP) >> CRS_ISR_FECAP_BITNUMBER);

  /* Get Frequency error direction */
  pSynchroInfo->FreqErrorDirection = (uint32_t)(READ_BIT(CRS->ISR, CRS_ISR_FEDIR));
}

/**
* @brief Wait for CRS Synchronization status.
* @param Timeout  Duration of the timeout
* @note  Timeout is based on the maximum time to receive a SYNC event based on synchronization
*        frequency.
* @note    If Timeout set to HAL_MAX_DELAY, HAL_TIMEOUT will be never returned.
* @retval Combination of Synchronization status
*          This parameter can be a combination of the following values:
*            @arg @ref RCC_CRS_TIMEOUT
*            @arg @ref RCC_CRS_SYNCOK
*            @arg @ref RCC_CRS_SYNCWARN
*            @arg @ref RCC_CRS_SYNCERR
*            @arg @ref RCC_CRS_SYNCMISS
*            @arg @ref RCC_CRS_TRIMOVF
*/
uint32_t HAL_RCCEx_CRSWaitSynchronization(uint32_t Timeout)
{
  uint32_t crsstatus = RCC_CRS_NONE;
  uint32_t tickstart = 0U;

  /* Get timeout */
  tickstart = HAL_GetTick();

  /* Wait for CRS flag or timeout detection */
  do
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U) || ((HAL_GetTick() - tickstart) > Timeout))
      {
        crsstatus = RCC_CRS_TIMEOUT;
      }
    }
    /* Check CRS SYNCOK flag  */
    if(__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCOK))
    {
      /* CRS SYNC event OK */
      crsstatus |= RCC_CRS_SYNCOK;

      /* Clear CRS SYNC event OK bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCOK);
    }

    /* Check CRS SYNCWARN flag  */
    if(__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCWARN))
    {
      /* CRS SYNC warning */
      crsstatus |= RCC_CRS_SYNCWARN;

      /* Clear CRS SYNCWARN bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCWARN);
    }

    /* Check CRS TRIM overflow flag  */
    if(__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_TRIMOVF))
    {
      /* CRS SYNC Error */
      crsstatus |= RCC_CRS_TRIMOVF;

      /* Clear CRS Error bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_TRIMOVF);
    }

    /* Check CRS Error flag  */
    if(__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCERR))
    {
      /* CRS SYNC Error */
      crsstatus |= RCC_CRS_SYNCERR;

      /* Clear CRS Error bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCERR);
    }

    /* Check CRS SYNC Missed flag  */
    if(__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCMISS))
    {
      /* CRS SYNC Missed */
      crsstatus |= RCC_CRS_SYNCMISS;

      /* Clear CRS SYNC Missed bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCMISS);
    }

    /* Check CRS Expected SYNC flag  */
    if(__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_ESYNC))
    {
      /* frequency error counter reached a zero value */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_ESYNC);
    }
  } while(RCC_CRS_NONE == crsstatus);

  return crsstatus;
}

/**
  * @brief Handle the Clock Recovery System interrupt request.
  * @retval None
  */
void HAL_RCCEx_CRS_IRQHandler(void)
{
  uint32_t crserror = RCC_CRS_NONE;
  /* Get current IT flags and IT sources values */
  uint32_t itflags = READ_REG(CRS->ISR);
  uint32_t itsources = READ_REG(CRS->CR);

  /* Check CRS SYNCOK flag  */
  if(((itflags & RCC_CRS_FLAG_SYNCOK) != RESET) && ((itsources & RCC_CRS_IT_SYNCOK) != RESET))
  {
    /* Clear CRS SYNC event OK flag */
    WRITE_REG(CRS->ICR, CRS_ICR_SYNCOKC);

    /* user callback */
    HAL_RCCEx_CRS_SyncOkCallback();
  }
  /* Check CRS SYNCWARN flag  */
  else if(((itflags & RCC_CRS_FLAG_SYNCWARN) != RESET) && ((itsources & RCC_CRS_IT_SYNCWARN) != RESET))
  {
    /* Clear CRS SYNCWARN flag */
    WRITE_REG(CRS->ICR, CRS_ICR_SYNCWARNC);

    /* user callback */
    HAL_RCCEx_CRS_SyncWarnCallback();
  }
  /* Check CRS Expected SYNC flag  */
  else if(((itflags & RCC_CRS_FLAG_ESYNC) != RESET) && ((itsources & RCC_CRS_IT_ESYNC) != RESET))
  {
    /* frequency error counter reached a zero value */
    WRITE_REG(CRS->ICR, CRS_ICR_ESYNCC);

    /* user callback */
    HAL_RCCEx_CRS_ExpectedSyncCallback();
  }
  /* Check CRS Error flags  */
  else
  {
    if(((itflags & RCC_CRS_FLAG_ERR) != RESET) && ((itsources & RCC_CRS_IT_ERR) != RESET))
    {
      if((itflags & RCC_CRS_FLAG_SYNCERR) != RESET)
      {
        crserror |= RCC_CRS_SYNCERR;
      }
      if((itflags & RCC_CRS_FLAG_SYNCMISS) != RESET)
      {
        crserror |= RCC_CRS_SYNCMISS;
      }
      if((itflags & RCC_CRS_FLAG_TRIMOVF) != RESET)
      {
        crserror |= RCC_CRS_TRIMOVF;
      }

      /* Clear CRS Error flags */
      WRITE_REG(CRS->ICR, CRS_ICR_ERRC);

      /* user error callback */
      HAL_RCCEx_CRS_ErrorCallback(crserror);
    }
  }
}

/**
  * @brief  RCCEx Clock Recovery System SYNCOK interrupt callback.
  * @retval none
  */
__weak void HAL_RCCEx_CRS_SyncOkCallback(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the @ref HAL_RCCEx_CRS_SyncOkCallback should be implemented in the user file
   */
}

/**
  * @brief  RCCEx Clock Recovery System SYNCWARN interrupt callback.
  * @retval none
  */
__weak void HAL_RCCEx_CRS_SyncWarnCallback(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the @ref HAL_RCCEx_CRS_SyncWarnCallback should be implemented in the user file
   */
}

/**
  * @brief  RCCEx Clock Recovery System Expected SYNC interrupt callback.
  * @retval none
  */
__weak void HAL_RCCEx_CRS_ExpectedSyncCallback(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the @ref HAL_RCCEx_CRS_ExpectedSyncCallback should be implemented in the user file
   */
}

/**
  * @brief  RCCEx Clock Recovery System Error interrupt callback.
  * @param  Error Combination of Error status.
  *         This parameter can be a combination of the following values:
  *           @arg @ref RCC_CRS_SYNCERR
  *           @arg @ref RCC_CRS_SYNCMISS
  *           @arg @ref RCC_CRS_TRIMOVF
  * @retval none
  */
__weak void HAL_RCCEx_CRS_ErrorCallback(uint32_t Error)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(Error);

  /* NOTE : This function should not be modified, when the callback is needed,
            the @ref HAL_RCCEx_CRS_ErrorCallback should be implemented in the user file
   */
}

/**
  * @}
  */

#endif /* CRS */

/**
  * @}
  */

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
