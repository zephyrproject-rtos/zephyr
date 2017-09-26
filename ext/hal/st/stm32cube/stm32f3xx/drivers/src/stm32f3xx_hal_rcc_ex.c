/**
  ******************************************************************************
  * @file    stm32f3xx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @brief   Extended RCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities RCC extension peripheral:
  *           + Extended Peripheral Control functions
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
#include "stm32f3xx_hal.h"

/** @addtogroup STM32F3xx_HAL_Driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/** @defgroup RCCEx RCCEx
  * @brief RCC Extension HAL module driver.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
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
#if defined(RCC_CFGR2_ADC1PRES) || defined(RCC_CFGR2_ADCPRE12) || defined(RCC_CFGR2_ADCPRE34) || defined(RCC_CFGR_USBPRE) \
 || defined(RCC_CFGR3_TIM1SW) || defined(RCC_CFGR3_TIM2SW) || defined(RCC_CFGR3_TIM8SW) || defined(RCC_CFGR3_TIM15SW)     \
 || defined(RCC_CFGR3_TIM16SW) || defined(RCC_CFGR3_TIM17SW) || defined(RCC_CFGR3_TIM20SW) || defined(RCC_CFGR3_TIM34SW)  \
 || defined(RCC_CFGR3_HRTIM1SW)
/** @defgroup RCCEx_Private_Functions RCCEx Private Functions
  * @{
  */
static uint32_t RCC_GetPLLCLKFreq(void);

/**
  * @}
  */
#endif /* RCC_CFGR2_ADC1PRES || RCC_CFGR2_ADCPRExx || RCC_CFGR3_TIMxSW || RCC_CFGR3_HRTIM1SW || RCC_CFGR_USBPRE */

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
  *         (ADC, CEC, I2C, I2S, SDADC, HRTIM, TIM, USART, RTC and USB).
  *
  * @note   Care must be taken when HAL_RCCEx_PeriphCLKConfig() is used to select
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

#if defined(RCC_CFGR3_USART2SW)
  /*----------------------------- USART2 Configuration --------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART2) == RCC_PERIPHCLK_USART2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART2CLKSOURCE(PeriphClkInit->Usart2ClockSelection));

    /* Configure the USART2 clock source */
    __HAL_RCC_USART2_CONFIG(PeriphClkInit->Usart2ClockSelection);
  }
#endif /* RCC_CFGR3_USART2SW */

#if defined(RCC_CFGR3_USART3SW)
  /*------------------------------ USART3 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART3) == RCC_PERIPHCLK_USART3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART3CLKSOURCE(PeriphClkInit->Usart3ClockSelection));

    /* Configure the USART3 clock source */
    __HAL_RCC_USART3_CONFIG(PeriphClkInit->Usart3ClockSelection);
  }
#endif /* RCC_CFGR3_USART3SW */

  /*------------------------------ I2C1 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C1) == RCC_PERIPHCLK_I2C1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C1CLKSOURCE(PeriphClkInit->I2c1ClockSelection));

    /* Configure the I2C1 clock source */
    __HAL_RCC_I2C1_CONFIG(PeriphClkInit->I2c1ClockSelection);
  }

#if defined(STM32F302xE) || defined(STM32F303xE)\
 || defined(STM32F302xC) || defined(STM32F303xC)\
 || defined(STM32F302x8)                        \
 || defined(STM32F373xC)
  /*------------------------------ USB Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USB) == RCC_PERIPHCLK_USB)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USBCLKSOURCE(PeriphClkInit->USBClockSelection));

    /* Configure the USB clock source */
    __HAL_RCC_USB_CONFIG(PeriphClkInit->USBClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F302x8                || */
       /* STM32F373xC                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
 || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)\
 || defined(STM32F373xC) || defined(STM32F378xx)

  /*------------------------------ I2C2 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C2) == RCC_PERIPHCLK_I2C2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C2CLKSOURCE(PeriphClkInit->I2c2ClockSelection));

    /* Configure the I2C2 clock source */
    __HAL_RCC_I2C2_CONFIG(PeriphClkInit->I2c2ClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  /*------------------------------ I2C3 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C3) == RCC_PERIPHCLK_I2C3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C3CLKSOURCE(PeriphClkInit->I2c3ClockSelection));

    /* Configure the I2C3 clock source */
    __HAL_RCC_I2C3_CONFIG(PeriphClkInit->I2c3ClockSelection);
  }
#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)

  /*------------------------------ UART4 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_UART4) == RCC_PERIPHCLK_UART4)
  {
    /* Check the parameters */
    assert_param(IS_RCC_UART4CLKSOURCE(PeriphClkInit->Uart4ClockSelection));

    /* Configure the UART4 clock source */
    __HAL_RCC_UART4_CONFIG(PeriphClkInit->Uart4ClockSelection);
  }

  /*------------------------------ UART5 Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_UART5) == RCC_PERIPHCLK_UART5)
  {
    /* Check the parameters */
    assert_param(IS_RCC_UART5CLKSOURCE(PeriphClkInit->Uart5ClockSelection));

    /* Configure the UART5 clock source */
    __HAL_RCC_UART5_CONFIG(PeriphClkInit->Uart5ClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
 || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)
  /*------------------------------ I2S Configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2S) == RCC_PERIPHCLK_I2S)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2SCLKSOURCE(PeriphClkInit->I2sClockSelection));

    /* Configure the I2S clock source */
    __HAL_RCC_I2S_CONFIG(PeriphClkInit->I2sClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  /*------------------------------ ADC1 clock Configuration ------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ADC1) == RCC_PERIPHCLK_ADC1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ADC1PLLCLK_DIV(PeriphClkInit->Adc1ClockSelection));

    /* Configure the ADC1 clock source */
    __HAL_RCC_ADC1_CONFIG(PeriphClkInit->Adc1ClockSelection);
  }

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
 || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)

  /*------------------------------ ADC1 & ADC2 clock Configuration -------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ADC12) == RCC_PERIPHCLK_ADC12)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ADC12PLLCLK_DIV(PeriphClkInit->Adc12ClockSelection));

    /* Configure the ADC12 clock source */
    __HAL_RCC_ADC12_CONFIG(PeriphClkInit->Adc12ClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */

#if defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F303xC) || defined(STM32F358xx)

  /*------------------------------ ADC3 & ADC4 clock Configuration -------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ADC34) == RCC_PERIPHCLK_ADC34)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ADC34PLLCLK_DIV(PeriphClkInit->Adc34ClockSelection));

    /* Configure the ADC34 clock source */
    __HAL_RCC_ADC34_CONFIG(PeriphClkInit->Adc34ClockSelection);
  }

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F373xC) || defined(STM32F378xx)

  /*------------------------------ ADC1 clock Configuration ------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ADC1) == RCC_PERIPHCLK_ADC1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ADC1PCLK2_DIV(PeriphClkInit->Adc1ClockSelection));

    /* Configure the ADC1 clock source */
    __HAL_RCC_ADC1_CONFIG(PeriphClkInit->Adc1ClockSelection);
  }

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
 || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)\
 || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  /*------------------------------ TIM1 clock Configuration ----------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM1) == RCC_PERIPHCLK_TIM1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM1CLKSOURCE(PeriphClkInit->Tim1ClockSelection));

    /* Configure the TIM1 clock source */
    __HAL_RCC_TIM1_CONFIG(PeriphClkInit->Tim1ClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F303xE) || defined(STM32F398xx)\
 || defined(STM32F303xC) || defined(STM32F358xx)

  /*------------------------------ TIM8 clock Configuration ----------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM8) == RCC_PERIPHCLK_TIM8)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM8CLKSOURCE(PeriphClkInit->Tim8ClockSelection));

    /* Configure the TIM8 clock source */
    __HAL_RCC_TIM8_CONFIG(PeriphClkInit->Tim8ClockSelection);
  }

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  /*------------------------------ TIM15 clock Configuration ----------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM15) == RCC_PERIPHCLK_TIM15)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM15CLKSOURCE(PeriphClkInit->Tim15ClockSelection));

    /* Configure the TIM15 clock source */
    __HAL_RCC_TIM15_CONFIG(PeriphClkInit->Tim15ClockSelection);
  }

  /*------------------------------ TIM16 clock Configuration ----------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM16) == RCC_PERIPHCLK_TIM16)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM16CLKSOURCE(PeriphClkInit->Tim16ClockSelection));

    /* Configure the TIM16 clock source */
    __HAL_RCC_TIM16_CONFIG(PeriphClkInit->Tim16ClockSelection);
  }

  /*------------------------------ TIM17 clock Configuration ----------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM17) == RCC_PERIPHCLK_TIM17)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM17CLKSOURCE(PeriphClkInit->Tim17ClockSelection));

    /* Configure the TIM17 clock source */
    __HAL_RCC_TIM17_CONFIG(PeriphClkInit->Tim17ClockSelection);
  }

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx */

#if defined(STM32F334x8)

  /*------------------------------ HRTIM1 clock Configuration ----------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_HRTIM1) == RCC_PERIPHCLK_HRTIM1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HRTIM1CLKSOURCE(PeriphClkInit->Hrtim1ClockSelection));

    /* Configure the HRTIM1 clock source */
    __HAL_RCC_HRTIM1_CONFIG(PeriphClkInit->Hrtim1ClockSelection);
  }

#endif /* STM32F334x8 */

#if defined(STM32F373xC) || defined(STM32F378xx)

  /*------------------------------ SDADC clock Configuration -------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SDADC) == RCC_PERIPHCLK_SDADC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SDADCSYSCLK_DIV(PeriphClkInit->SdadcClockSelection));

    /* Configure the SDADC clock prescaler */
    __HAL_RCC_SDADC_CONFIG(PeriphClkInit->SdadcClockSelection);
  }

  /*------------------------------ CEC clock Configuration -------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_CEC) == RCC_PERIPHCLK_CEC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_CECCLKSOURCE(PeriphClkInit->CecClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_CEC_CONFIG(PeriphClkInit->CecClockSelection);
  }

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)

  /*------------------------------ TIM2 clock Configuration -------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM2) == RCC_PERIPHCLK_TIM2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM2CLKSOURCE(PeriphClkInit->Tim2ClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_TIM2_CONFIG(PeriphClkInit->Tim2ClockSelection);
  }

  /*------------------------------ TIM3 clock Configuration -------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM34) == RCC_PERIPHCLK_TIM34)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM3CLKSOURCE(PeriphClkInit->Tim34ClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_TIM34_CONFIG(PeriphClkInit->Tim34ClockSelection);
  }

  /*------------------------------ TIM15 clock Configuration ------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM15) == RCC_PERIPHCLK_TIM15)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM15CLKSOURCE(PeriphClkInit->Tim15ClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_TIM15_CONFIG(PeriphClkInit->Tim15ClockSelection);
  }

  /*------------------------------ TIM16 clock Configuration ------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM16) == RCC_PERIPHCLK_TIM16)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM16CLKSOURCE(PeriphClkInit->Tim16ClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_TIM16_CONFIG(PeriphClkInit->Tim16ClockSelection);
  }

  /*------------------------------ TIM17 clock Configuration ------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM17) == RCC_PERIPHCLK_TIM17)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM17CLKSOURCE(PeriphClkInit->Tim17ClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_TIM17_CONFIG(PeriphClkInit->Tim17ClockSelection);
  }

#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined(STM32F303xE) || defined(STM32F398xx)
  /*------------------------------ TIM20 clock Configuration ------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_TIM20) == RCC_PERIPHCLK_TIM20)
  {
    /* Check the parameters */
    assert_param(IS_RCC_TIM20CLKSOURCE(PeriphClkInit->Tim20ClockSelection));

    /* Configure the CEC clock source */
    __HAL_RCC_TIM20_CONFIG(PeriphClkInit->Tim20ClockSelection);
  }
#endif /* STM32F303xE || STM32F398xx */


  return HAL_OK;
}

/**
  * @brief  Get the RCC_ClkInitStruct according to the internal
  * RCC configuration registers.
  * @param  PeriphClkInit pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         returns the configuration information for the Extended Peripherals clocks
  *         (ADC, CEC, I2C, I2S, SDADC, HRTIM, TIM, USART, RTC and USB clocks).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  /* Set all possible values for the extended clock type parameter------------*/
  /* Common part first */
#if defined(RCC_CFGR3_USART2SW) && defined(RCC_CFGR3_USART3SW)
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_USART3 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_RTC;
#else
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_RTC;
#endif /* RCC_CFGR3_USART2SW && RCC_CFGR3_USART3SW */

  /* Get the RTC configuration --------------------------------------------*/
  PeriphClkInit->RTCClockSelection = __HAL_RCC_GET_RTC_SOURCE();
  /* Get the USART1 clock configuration --------------------------------------------*/
  PeriphClkInit->Usart1ClockSelection = __HAL_RCC_GET_USART1_SOURCE();
#if defined(RCC_CFGR3_USART2SW)
  /* Get the USART2 clock configuration -----------------------------------------*/
  PeriphClkInit->Usart2ClockSelection = __HAL_RCC_GET_USART2_SOURCE();
#endif /* RCC_CFGR3_USART2SW */
#if defined(RCC_CFGR3_USART3SW)
   /* Get the USART3 clock configuration -----------------------------------------*/
  PeriphClkInit->Usart3ClockSelection = __HAL_RCC_GET_USART3_SOURCE();
#endif /* RCC_CFGR3_USART3SW */
  /* Get the I2C1 clock configuration -----------------------------------------*/
  PeriphClkInit->I2c1ClockSelection = __HAL_RCC_GET_I2C1_SOURCE();

#if defined(STM32F302xE) || defined(STM32F303xE)\
    || defined(STM32F302xC) || defined(STM32F303xC)\
    || defined(STM32F302x8)                        \
    || defined(STM32F373xC)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_USB;
  /* Get the USB clock configuration -----------------------------------------*/
  PeriphClkInit->USBClockSelection = __HAL_RCC_GET_USB_SOURCE();

#endif /* STM32F302xE || STM32F303xE || */
       /* STM32F302xC || STM32F303xC || */
       /* STM32F302x8                || */
       /* STM32F373xC                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
    || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)\
    || defined(STM32F373xC) || defined(STM32F378xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_I2C2;
  /* Get the I2C2 clock configuration -----------------------------------------*/
  PeriphClkInit->I2c2ClockSelection = __HAL_RCC_GET_I2C2_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_I2C3;
  /* Get the I2C3 clock configuration -----------------------------------------*/
  PeriphClkInit->I2c3ClockSelection = __HAL_RCC_GET_I2C3_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F302xC) || defined(STM32F303xC) ||defined(STM32F358xx)

  PeriphClkInit->PeriphClockSelection |= (RCC_PERIPHCLK_UART4  | RCC_PERIPHCLK_UART5);
  /* Get the UART4 clock configuration -----------------------------------------*/
  PeriphClkInit->Uart4ClockSelection = __HAL_RCC_GET_UART4_SOURCE();
  /* Get the UART5 clock configuration -----------------------------------------*/
  PeriphClkInit->Uart5ClockSelection = __HAL_RCC_GET_UART5_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
    || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_I2S;
  /* Get the I2S clock configuration -----------------------------------------*/
  PeriphClkInit->I2sClockSelection = __HAL_RCC_GET_I2S_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx || */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)\
    || defined(STM32F373xC) || defined(STM32F378xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_ADC1;
  /* Get the ADC1 clock configuration -----------------------------------------*/
  PeriphClkInit->Adc1ClockSelection = __HAL_RCC_GET_ADC1_SOURCE();

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx || */
       /* STM32F373xC || STM32F378xx                   */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
    || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_ADC12;
  /* Get the ADC1 & ADC2 clock configuration -----------------------------------------*/
  PeriphClkInit->Adc12ClockSelection = __HAL_RCC_GET_ADC12_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx    */

#if defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F303xC) || defined(STM32F358xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_ADC34;
   /* Get the ADC3 & ADC4 clock configuration -----------------------------------------*/
  PeriphClkInit->Adc34ClockSelection = __HAL_RCC_GET_ADC34_SOURCE();

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F302xC) || defined(STM32F303xC) || defined(STM32F358xx)\
    || defined(STM32F303x8) || defined(STM32F334x8) || defined(STM32F328xx)\
    || defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM1;
  /* Get the TIM1 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim1ClockSelection = __HAL_RCC_GET_TIM1_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx || */
       /* STM32F302xC || STM32F303xC || STM32F358xx || */
       /* STM32F303x8 || STM32F334x8 || STM32F328xx || */
       /* STM32F301x8 || STM32F302x8 || STM32F318xx    */

#if defined(STM32F303xE) || defined(STM32F398xx)\
    || defined(STM32F303xC) || defined(STM32F358xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM8;
  /* Get the TIM8 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim8ClockSelection = __HAL_RCC_GET_TIM8_SOURCE();

#endif /* STM32F303xE || STM32F398xx || */
       /* STM32F303xC || STM32F358xx    */

#if defined(STM32F301x8) || defined(STM32F302x8) || defined(STM32F318xx)

  PeriphClkInit->PeriphClockSelection |= (RCC_PERIPHCLK_TIM15 | RCC_PERIPHCLK_TIM16 | RCC_PERIPHCLK_TIM17);
  /* Get the TIM15 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim15ClockSelection = __HAL_RCC_GET_TIM15_SOURCE();
  /* Get the TIM16 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim16ClockSelection = __HAL_RCC_GET_TIM16_SOURCE();
  /* Get the TIM17 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim17ClockSelection = __HAL_RCC_GET_TIM17_SOURCE();

#endif /* STM32F301x8 || STM32F302x8 || STM32F318xx */

#if defined(STM32F334x8)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_HRTIM1;
  /* Get the HRTIM1 clock configuration -----------------------------------------*/
  PeriphClkInit->Hrtim1ClockSelection = __HAL_RCC_GET_HRTIM1_SOURCE();

#endif /* STM32F334x8 */

#if defined(STM32F373xC) || defined(STM32F378xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_SDADC;
  /* Get the SDADC clock configuration -----------------------------------------*/
  PeriphClkInit->SdadcClockSelection = __HAL_RCC_GET_SDADC_SOURCE();

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_CEC;
  /* Get the CEC clock configuration -----------------------------------------*/
  PeriphClkInit->CecClockSelection = __HAL_RCC_GET_CEC_SOURCE();

#endif /* STM32F373xC || STM32F378xx */

#if defined(STM32F302xE) || defined(STM32F303xE) || defined(STM32F398xx)

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM2;
  /* Get the TIM2 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim2ClockSelection = __HAL_RCC_GET_TIM2_SOURCE();

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM34;
  /* Get the TIM3 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim34ClockSelection = __HAL_RCC_GET_TIM34_SOURCE();

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM15;
  /* Get the TIM15 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim15ClockSelection = __HAL_RCC_GET_TIM15_SOURCE();

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM16;
  /* Get the TIM16 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim16ClockSelection = __HAL_RCC_GET_TIM16_SOURCE();

  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM17;
  /* Get the TIM17 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim17ClockSelection = __HAL_RCC_GET_TIM17_SOURCE();

#endif /* STM32F302xE || STM32F303xE || STM32F398xx */

#if defined (STM32F303xE) || defined(STM32F398xx)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_TIM20;
  /* Get the TIM20 clock configuration -----------------------------------------*/
  PeriphClkInit->Tim20ClockSelection = __HAL_RCC_GET_TIM20_SOURCE();
#endif /* STM32F303xE || STM32F398xx */
}

/**
  * @brief  Returns the peripheral clock frequency
  * @note   Returns 0 if peripheral clock is unknown or 0xDEADDEAD if not applicable.
  * @param  PeriphClk Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_PERIPHCLK_RTC     RTC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART1  USART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C1    I2C1 peripheral clock
  @if STM32F301x8
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3    I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC1    ADC1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM15   TIM15 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM16   TIM16 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM17   TIM17 peripheral clock
  @endif
  @if STM32F302x8
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3    I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC1    ADC1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM15   TIM15 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM16   TIM16 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM17   TIM17 peripheral clock
  @endif
  @if STM32F302xC
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART4   UART4 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART5   UART5 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  @endif
  @if STM32F302xE
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART4   UART4 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART5   UART5 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3    I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM2    TIM2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM15   TIM15 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM16   TIM16 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM17   TIM17 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM34   TIM34 peripheral clock
  @endif
  @if STM32F303x8
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  @endif
  @if STM32F303xC
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART4   UART4 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART5   UART5 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC34   ADC34 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM8    TIM8 peripheral clock
  @endif
  @if STM32F303xE
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART4   UART4 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART5   UART5 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3    I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC34   ADC34 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM2    TIM2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM8    TIM8 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM15   TIM15 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM16   TIM16 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM17   TIM17 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM20   TIM20 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM34   TIM34 peripheral clock
  @endif
  @if STM32F318xx
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3    I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC1    ADC1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM15   TIM15 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM16   TIM16 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM17   TIM17 peripheral clock
  @endif
  @if STM32F328xx
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  @endif
  @if STM32F334x8
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_HRTIM1  HRTIM1 peripheral clock
  @endif
  @if STM32F358xx
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART4   UART4 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART5   UART5 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC34   ADC34 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM8    TIM8 peripheral clock
  @endif
  @if STM32F373xC
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB     USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC1    ADC1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_SDADC   SDADC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F378xx
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC1    ADC1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_SDADC   SDADC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_CEC     CEC peripheral clock
  @endif
  @if STM32F398xx
  *            @arg @ref RCC_PERIPHCLK_USART2  USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART3  USART3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART4   UART4 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_UART5   UART5 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2    I2C2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3    I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2S     I2S peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC12   ADC12 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC34   ADC34 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM1    TIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM2    TIM2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM8    TIM8 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM15   TIM15 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM16   TIM16 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM17   TIM17 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM20   TIM20 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_TIM34   TIM34 peripheral clock
  @endif
  * @retval Frequency in Hz (0: means that no available frequency for the peripheral)
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
  /* frequency == 0 : means that no available frequency for the peripheral */
  uint32_t frequency = 0U;

  uint32_t srcclk = 0U;
#if defined(RCC_CFGR2_ADC1PRES) || defined(RCC_CFGR2_ADCPRE12) || defined(RCC_CFGR2_ADCPRE34)
  uint16_t adc_pll_prediv_table[16] = { 1U,  2U,  4U,  6U, 8U, 10U, 12U, 16U, 32U, 64U, 128U, 256U, 256U, 256U, 256U, 256U};
#endif /* RCC_CFGR2_ADC1PRES || RCC_CFGR2_ADCPRE12 || RCC_CFGR2_ADCPRE34 */
#if defined(RCC_CFGR_SDPRE)
  uint8_t sdadc_prescaler_table[16] = { 2U,  4U,  6U, 8U, 10U, 12U, 14U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U};
#endif /* RCC_CFGR_SDPRE */

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
#if defined(RCC_USART1CLKSOURCE_PCLK2)
      if (srcclk == RCC_USART1CLKSOURCE_PCLK2)
      {
        frequency = HAL_RCC_GetPCLK2Freq();
      }
#else
      if (srcclk == RCC_USART1CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
#endif /* RCC_USART1CLKSOURCE_PCLK2 */
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
#if defined(RCC_CFGR3_UART4SW)
  case RCC_PERIPHCLK_UART4:
    {
      /* Get the current UART4 source */
      srcclk = __HAL_RCC_GET_UART4_SOURCE();

      /* Check if UART4 clock selection is PCLK1 */
      if (srcclk == RCC_UART4CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if UART4 clock selection is HSI */
      else if ((srcclk == RCC_UART4CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if UART4 clock selection is SYSCLK */
      else if (srcclk == RCC_UART4CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Check if LSE is ready  and if UART4 clock selection is LSE */
      else if ((srcclk == RCC_UART4CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      break;
    }
#endif /* RCC_CFGR3_UART4SW */
#if defined(RCC_CFGR3_UART5SW)
  case RCC_PERIPHCLK_UART5:
    {
      /* Get the current UART5 source */
      srcclk = __HAL_RCC_GET_UART5_SOURCE();

      /* Check if UART5 clock selection is PCLK1 */
      if (srcclk == RCC_UART5CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if UART5 clock selection is HSI */
      else if ((srcclk == RCC_UART5CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if UART5 clock selection is SYSCLK */
      else if (srcclk == RCC_UART5CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Check if LSE is ready  and if UART5 clock selection is LSE */
      else if ((srcclk == RCC_UART5CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->BDCR, RCC_BDCR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      break;
    }
#endif /* RCC_CFGR3_UART5SW */
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
#if defined(RCC_CFGR3_I2C2SW)
  case RCC_PERIPHCLK_I2C2:
    {
      /* Get the current I2C2 source */
      srcclk = __HAL_RCC_GET_I2C2_SOURCE();

      /* Check if HSI is ready and if I2C2 clock selection is HSI */
      if ((srcclk == RCC_I2C2CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if I2C2 clock selection is SYSCLK */
      else if (srcclk == RCC_I2C2CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      break;
    }
#endif /* RCC_CFGR3_I2C2SW */
#if defined(RCC_CFGR3_I2C3SW)
  case RCC_PERIPHCLK_I2C3:
    {
      /* Get the current I2C3 source */
      srcclk = __HAL_RCC_GET_I2C3_SOURCE();

      /* Check if HSI is ready and if I2C3 clock selection is HSI */
      if ((srcclk == RCC_I2C3CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if I2C3 clock selection is SYSCLK */
      else if (srcclk == RCC_I2C3CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      break;
    }
#endif /* RCC_CFGR3_I2C3SW */
#if defined(RCC_CFGR_I2SSRC)
  case RCC_PERIPHCLK_I2S:
    {
      /* Get the current I2S source */
      srcclk = __HAL_RCC_GET_I2S_SOURCE();

      /* Check if I2S clock selection is External clock mapped on the I2S_CKIN pin */
      if (srcclk == RCC_I2SCLKSOURCE_EXT)
      {
        /* External clock used. Frequency cannot be returned.*/
        frequency = 0xDEADDEADU;
      }
      /* Check if I2S clock selection is SYSCLK */
      else if (srcclk == RCC_I2SCLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      break;
    }
#endif /* RCC_CFGR_I2SSRC */
#if defined(RCC_CFGR_USBPRE)
  case RCC_PERIPHCLK_USB:
    {
      /* Check if PLL is ready */
      if (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY))
      {
        /* Get the current USB source */
        srcclk = __HAL_RCC_GET_USB_SOURCE();

        /* Check if USB clock selection is not divided */
        if (srcclk == RCC_USBCLKSOURCE_PLL)
        {
          frequency = RCC_GetPLLCLKFreq();
        }
        /* Check if USB clock selection is divided by 1.5 */
        else /* RCC_USBCLKSOURCE_PLL_DIV1_5 */
        {
          frequency = (RCC_GetPLLCLKFreq() * 3U) / 2U;
        }
      }
      break;
    }
#endif /* RCC_CFGR_USBPRE */
#if defined(RCC_CFGR2_ADC1PRES) || defined(RCC_CFGR_ADCPRE)
  case RCC_PERIPHCLK_ADC1:
    {
      /* Get the current ADC1 source */
      srcclk = __HAL_RCC_GET_ADC1_SOURCE();
#if defined(RCC_CFGR2_ADC1PRES)
      /* Check if ADC1 clock selection is AHB */
      if (srcclk == RCC_ADC1PLLCLK_OFF)
      {
          frequency = SystemCoreClock;
      }
      /* PLL clock has been selected */
      else
      {
        /* Check if PLL is ready */
        if (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY))
        {
          /* Frequency is the PLL frequency divided by ADC prescaler (1U/2U/4U/6U/8U/10U/12U/16U/32U/64U/128U/256U) */
          frequency = RCC_GetPLLCLKFreq() / adc_pll_prediv_table[(srcclk >> POSITION_VAL(RCC_CFGR2_ADC1PRES)) & 0xFU];
        }
      }
#else /* RCC_CFGR_ADCPRE */
      /* ADC1 is set to PLCK2 frequency divided by 2U/4U/6U/8U */
      frequency = HAL_RCC_GetPCLK2Freq() / (((srcclk  >> POSITION_VAL(RCC_CFGR_ADCPRE)) + 1U) * 2U);
#endif /* RCC_CFGR2_ADC1PRES */
      break;
    }
#endif /* RCC_CFGR2_ADC1PRES || RCC_CFGR_ADCPRE */
#if defined(RCC_CFGR2_ADCPRE12)
  case RCC_PERIPHCLK_ADC12:
    {
      /* Get the current ADC12 source */
      srcclk = __HAL_RCC_GET_ADC12_SOURCE();
      /* Check if ADC12 clock selection is AHB */
      if (srcclk == RCC_ADC12PLLCLK_OFF)
      {
          frequency = SystemCoreClock;
      }
      /* PLL clock has been selected */
      else
      {
        /* Check if PLL is ready */
        if (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY))
        {
          /* Frequency is the PLL frequency divided by ADC prescaler (1U/2U/4U/6/8U/10U/12U/16U/32U/64U/128U/256U) */
          frequency = RCC_GetPLLCLKFreq() / adc_pll_prediv_table[(srcclk >> POSITION_VAL(RCC_CFGR2_ADCPRE12)) & 0xF];
        }
      }
      break;
    }
#endif /* RCC_CFGR2_ADCPRE12 */
#if defined(RCC_CFGR2_ADCPRE34)
  case RCC_PERIPHCLK_ADC34:
    {
      /* Get the current ADC34 source */
      srcclk = __HAL_RCC_GET_ADC34_SOURCE();
      /* Check if ADC34 clock selection is AHB */
      if (srcclk == RCC_ADC34PLLCLK_OFF)
      {
          frequency = SystemCoreClock;
      }
      /* PLL clock has been selected */
      else
      {
        /* Check if PLL is ready */
        if (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY))
        {
          /* Frequency is the PLL frequency divided by ADC prescaler (1U/2U/4U/6U/8U/10U/12U/16U/32U/64U/128U/256U) */
          frequency = RCC_GetPLLCLKFreq() / adc_pll_prediv_table[(srcclk >> POSITION_VAL(RCC_CFGR2_ADCPRE34)) & 0xF];
        }
      }
      break;
    }
#endif /* RCC_CFGR2_ADCPRE34 */
#if defined(RCC_CFGR3_TIM1SW)
  case RCC_PERIPHCLK_TIM1:
    {
      /* Get the current TIM1 source */
      srcclk = __HAL_RCC_GET_TIM1_SOURCE();

      /* Check if PLL is ready and if TIM1 clock selection is PLL */
      if ((srcclk == RCC_TIM1CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM1 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM1CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM1SW */
#if defined(RCC_CFGR3_TIM2SW)
  case RCC_PERIPHCLK_TIM2:
    {
      /* Get the current TIM2 source */
      srcclk = __HAL_RCC_GET_TIM2_SOURCE();

      /* Check if PLL is ready and if TIM2 clock selection is PLL */
      if ((srcclk == RCC_TIM2CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM2 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM2CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM2SW */
#if defined(RCC_CFGR3_TIM8SW)
  case RCC_PERIPHCLK_TIM8:
    {
      /* Get the current TIM8 source */
      srcclk = __HAL_RCC_GET_TIM8_SOURCE();

      /* Check if PLL is ready and if TIM8 clock selection is PLL */
      if ((srcclk == RCC_TIM8CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM8 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM8CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM8SW */
#if defined(RCC_CFGR3_TIM15SW)
  case RCC_PERIPHCLK_TIM15:
    {
      /* Get the current TIM15 source */
      srcclk = __HAL_RCC_GET_TIM15_SOURCE();

      /* Check if PLL is ready and if TIM15 clock selection is PLL */
      if ((srcclk == RCC_TIM15CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM15 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM15CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM15SW */
#if defined(RCC_CFGR3_TIM16SW)
  case RCC_PERIPHCLK_TIM16:
    {
      /* Get the current TIM16 source */
      srcclk = __HAL_RCC_GET_TIM16_SOURCE();

      /* Check if PLL is ready and if TIM16 clock selection is PLL */
      if ((srcclk == RCC_TIM16CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM16 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM16CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM16SW */
#if defined(RCC_CFGR3_TIM17SW)
  case RCC_PERIPHCLK_TIM17:
    {
      /* Get the current TIM17 source */
      srcclk = __HAL_RCC_GET_TIM17_SOURCE();

      /* Check if PLL is ready and if TIM17 clock selection is PLL */
      if ((srcclk == RCC_TIM17CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM17 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM17CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM17SW */
#if defined(RCC_CFGR3_TIM20SW)
  case RCC_PERIPHCLK_TIM20:
    {
      /* Get the current TIM20 source */
      srcclk = __HAL_RCC_GET_TIM20_SOURCE();

      /* Check if PLL is ready and if TIM20 clock selection is PLL */
      if ((srcclk == RCC_TIM20CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM20 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM20CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM20SW */
#if defined(RCC_CFGR3_TIM34SW)
  case RCC_PERIPHCLK_TIM34:
    {
      /* Get the current TIM34 source */
      srcclk = __HAL_RCC_GET_TIM34_SOURCE();

      /* Check if PLL is ready and if TIM34 clock selection is PLL */
      if ((srcclk == RCC_TIM34CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if TIM34 clock selection is SYSCLK */
      else if (srcclk == RCC_TIM34CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
      break;
    }
#endif /* RCC_CFGR3_TIM34SW */
#if defined(RCC_CFGR3_HRTIM1SW)
  case RCC_PERIPHCLK_HRTIM1:
    {
      /* Get the current HRTIM1 source */
      srcclk = __HAL_RCC_GET_HRTIM1_SOURCE();

      /* Check if PLL is ready and if HRTIM1 clock selection is PLL */
      if ((srcclk == RCC_HRTIM1CLK_PLLCLK) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
      {
        frequency = RCC_GetPLLCLKFreq();
      }
      /* Check if HRTIM1 clock selection is SYSCLK */
      else if (srcclk == RCC_HRTIM1CLK_HCLK)
      {
        frequency = SystemCoreClock;
      }
     break;
    }
#endif /* RCC_CFGR3_HRTIM1SW */
#if defined(RCC_CFGR_SDPRE)
  case RCC_PERIPHCLK_SDADC:
    {
      /* Get the current SDADC source */
      srcclk = __HAL_RCC_GET_SDADC_SOURCE();
      /* Frequency is the system frequency divided by SDADC prescaler (2U/4U/6U/8U/10U/12U/14U/16U/20U/24U/28U/32U/36U/40U/44U/48U) */
      frequency = SystemCoreClock / sdadc_prescaler_table[(srcclk >> POSITION_VAL(RCC_CFGR_SDPRE)) & 0xF];
      break;
    }
#endif /* RCC_CFGR_SDPRE */
#if defined(RCC_CFGR3_CECSW)
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
#endif /* RCC_CFGR3_CECSW */
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

/**
  * @}
  */


#if defined(RCC_CFGR2_ADC1PRES) || defined(RCC_CFGR2_ADCPRE12) || defined(RCC_CFGR2_ADCPRE34) || defined(RCC_CFGR_USBPRE) \
 || defined(RCC_CFGR3_TIM1SW) || defined(RCC_CFGR3_TIM2SW) || defined(RCC_CFGR3_TIM8SW) || defined(RCC_CFGR3_TIM15SW)     \
 || defined(RCC_CFGR3_TIM16SW) || defined(RCC_CFGR3_TIM17SW) || defined(RCC_CFGR3_TIM20SW) || defined(RCC_CFGR3_TIM34SW)  \
 || defined(RCC_CFGR3_HRTIM1SW)

/** @addtogroup RCCEx_Private_Functions
  * @{
  */
static uint32_t RCC_GetPLLCLKFreq(void)
{
  uint32_t pllmul = 0U, pllsource = 0U, prediv = 0U, pllclk = 0U;

  pllmul = RCC->CFGR & RCC_CFGR_PLLMUL;
  pllmul = ( pllmul >> 18U) + 2U;
  pllsource = RCC->CFGR & RCC_CFGR_PLLSRC;
#if defined(RCC_CFGR_PLLSRC_HSI_DIV2)
  if (pllsource != RCC_PLLSOURCE_HSI)
  {
    prediv = (RCC->CFGR2 & RCC_CFGR2_PREDIV) + 1U;
    /* HSE used as PLL clock source : PLLCLK = HSE/PREDIV * PLLMUL */
    pllclk = (HSE_VALUE/prediv) * pllmul;
  }
  else
  {
    /* HSI used as PLL clock source : PLLCLK = HSI/2U * PLLMUL */
    pllclk = (HSI_VALUE >> 1U) * pllmul;
  }
#else
  prediv = (RCC->CFGR2 & RCC_CFGR2_PREDIV) + 1U;
  if (pllsource == RCC_CFGR_PLLSRC_HSE_PREDIV)
  {
    /* HSE used as PLL clock source : PLLCLK = HSE/PREDIV * PLLMUL */
    pllclk = (HSE_VALUE/prediv) * pllmul;
  }
  else
  {
    /* HSI used as PLL clock source : PLLCLK = HSI/PREDIV * PLLMUL */
    pllclk = (HSI_VALUE/prediv) * pllmul;
  }
#endif /* RCC_CFGR_PLLSRC_HSI_DIV2 */

  return pllclk;
}
/**
  * @}
  */

#endif /* RCC_CFGR2_ADC1PRES || RCC_CFGR2_ADCPRExx || RCC_CFGR3_TIMxSW || RCC_CFGR3_HRTIM1SW || RCC_CFGR_USBPRE */

/**
  * @}
  */

#endif /* HAL_RCC_MODULE_ENABLED */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
