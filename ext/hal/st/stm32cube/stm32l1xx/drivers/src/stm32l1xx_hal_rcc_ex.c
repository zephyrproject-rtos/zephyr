/**
  ******************************************************************************
  * @file    stm32l1xx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @brief   Extended RCC HAL module driver.
  *          This file provides firmware functions to manage the following 
  *          functionalities RCC extension peripheral:
  *           + Extended Peripheral Control functions
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

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/** @addtogroup STM32L1xx_HAL_Driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/** @defgroup RCCEx RCCEx
  * @brief RCC Extension HAL module driver
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup RCCEx_Private_Constants RCCEx Private Constants
  * @{
  */
/**
  * @}
  */
  
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
        the backup registers) are set to their reset values.
      
@endverbatim
  * @{
  */

/**
  * @brief  Initializes the RCC extended peripherals clocks according to the specified
  *         parameters in the RCC_PeriphCLKInitTypeDef.
  * @param  PeriphClkInit pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         contains the configuration information for the Extended Peripherals clocks(RTC/LCD clock).
  * @retval HAL status
  * @note   If HAL_ERROR returned, first switch-OFF HSE clock oscillator with @ref HAL_RCC_OscConfig()
  *         to possibly update HSE divider.
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t tickstart = 0U;
  uint32_t temp_reg = 0U;
  
  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClkInit->PeriphClockSelection));
  
  /*------------------------------- RTC/LCD Configuration ------------------------*/ 
  if ((((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC) 
#if defined(LCD)
   || (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD)
#endif /* LCD */
     )
  {
    /* check for RTC Parameters used to output RTCCLK */
    if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC)
    {
      assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->RTCClockSelection));
    }

#if defined(LCD)
    if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD)
    {
      assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->LCDClockSelection));
    }
#endif /* LCD */

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

    /* Check if user wants to change HSE RTC prescaler whereas HSE is enabled */ 
    temp_reg = (RCC->CR & RCC_CR_RTCPRE);
    if ((temp_reg != (PeriphClkInit->RTCClockSelection & RCC_CR_RTCPRE))
#if defined (LCD)
     || (temp_reg != (PeriphClkInit->LCDClockSelection & RCC_CR_RTCPRE))
#endif /* LCD */
       )
    { /* Check HSE State */
      if (((PeriphClkInit->RTCClockSelection & RCC_CSR_RTCSEL) == RCC_CSR_RTCSEL_HSE) && HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY))
      {
        /* To update HSE divider, first switch-OFF HSE clock oscillator*/
        return HAL_ERROR; 
      }
    }
    
    /* Reset the Backup domain only if the RTC Clock source selection is modified from reset value */ 
    temp_reg = (RCC->CSR & RCC_CSR_RTCSEL);
    
    if((temp_reg != 0x00000000U) && (((temp_reg != (PeriphClkInit->RTCClockSelection & RCC_CSR_RTCSEL)) \
      && (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC))
#if defined(LCD)
      || ((temp_reg != (PeriphClkInit->LCDClockSelection & RCC_CSR_RTCSEL)) \
       && (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD))
#endif /* LCD */
     ))
    {
      /* Store the content of CSR register before the reset of Backup Domain */
      temp_reg = (RCC->CSR & ~(RCC_CSR_RTCSEL));
      
      /* RTC Clock selection can be changed only if the Backup Domain is reset */
      __HAL_RCC_BACKUPRESET_FORCE();
      __HAL_RCC_BACKUPRESET_RELEASE();
      
      /* Restore the Content of CSR register */
      RCC->CSR = temp_reg;
      
       /* Wait for LSERDY if LSE was enabled */
      if (HAL_IS_BIT_SET(temp_reg, RCC_CSR_LSEON))
      {
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till LSE is ready */  
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET)
        {
          if((HAL_GetTick() - tickstart ) > RCC_LSE_TIMEOUT_VALUE)
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
  
  return HAL_OK;
}

/**
  * @brief  Get the PeriphClkInit according to the internal RCC configuration registers.
  * @param  PeriphClkInit pointer to an RCC_PeriphCLKInitTypeDef structure that 
  *         returns the configuration information for the Extended Peripherals clocks(RTC/LCD clocks).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t srcclk = 0U;
  
  /* Set all possible values for the extended clock type parameter------------*/
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_RTC;
#if defined(LCD)   
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_LCD;
#endif /* LCD */

  /* Get the RTC/LCD configuration -----------------------------------------------*/
  srcclk = __HAL_RCC_GET_RTC_SOURCE();
  if (srcclk != RCC_RTCCLKSOURCE_HSE_DIV2)
  {
    /* Source clock is LSE or LSI*/
    PeriphClkInit->RTCClockSelection = srcclk;
  }
  else
  {
    /* Source clock is HSE. Need to get the prescaler value*/
    PeriphClkInit->RTCClockSelection = srcclk | (READ_BIT(RCC->CR, RCC_CR_RTCPRE));
  }
#if defined(LCD)
  PeriphClkInit->LCDClockSelection = PeriphClkInit->RTCClockSelection;
#endif /* LCD */
}

/**
  * @brief  Return the peripheral clock frequency
  * @note   Return 0 if peripheral clock is unknown
  * @param  PeriphClk Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_PERIPHCLK_RTC      RTC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LCD      LCD peripheral clock (*)
  * @note   (*) means that this peripheral is not present on all the devices
  * @retval Frequency in Hz (0: means that no available frequency for the peripheral)
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
  uint32_t temp_reg = 0U, clkprediv = 0U, frequency = 0U;
  uint32_t srcclk = 0U;

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClk));
  
  switch (PeriphClk)
  {
  case RCC_PERIPHCLK_RTC:
#if defined(LCD)
  case RCC_PERIPHCLK_LCD:
#endif /* LCD */
    {
      /* Get RCC CSR configuration ------------------------------------------------------*/
      temp_reg = RCC->CSR;

      /* Get the current RTC source */
      srcclk = __HAL_RCC_GET_RTC_SOURCE();

      /* Check if LSE is ready if RTC clock selection is LSE */
      if ((srcclk == RCC_RTCCLKSOURCE_LSE) && (HAL_IS_BIT_SET(temp_reg, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Check if LSI is ready if RTC clock selection is LSI */
      else if ((srcclk == RCC_RTCCLKSOURCE_LSI) && (HAL_IS_BIT_SET(temp_reg, RCC_CSR_LSIRDY)))
      {
        frequency = LSI_VALUE;
      }
      /* Check if HSE is ready and if RTC clock selection is HSE */
      else if ((srcclk == RCC_RTCCLKSOURCE_HSE_DIVX) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY)))
      {
        /* Get the current HSE clock divider */
        clkprediv = __HAL_RCC_GET_RTC_HSE_PRESCALER();

        switch (clkprediv)
        {
          case RCC_RTC_HSE_DIV_16:  /* HSE DIV16 has been selected */
          {
            frequency = HSE_VALUE / 16U;
            break;
          }
          case RCC_RTC_HSE_DIV_8:   /* HSE DIV8 has been selected  */
          {
            frequency = HSE_VALUE / 8U;
            break;
          }
          case RCC_RTC_HSE_DIV_4:   /* HSE DIV4 has been selected  */
          {
            frequency = HSE_VALUE / 4U;
            break;
          }
          default:                  /* HSE DIV2 has been selected  */
          {
            frequency = HSE_VALUE / 2U;
            break;
          }
        }
      }
      /* Clock not enabled for RTC */
      else
      {
        frequency = 0U;
      }
      break;
    }
  default: 
    {
      break;
    }
  }
  return(frequency);
}

#if defined(RCC_LSECSS_SUPPORT)
/**
  * @brief  Enables the LSE Clock Security System.
  * @note   If a failure is detected on the external 32 kHz oscillator, the LSE clock is no longer supplied
  *         to the RTC but no hardware action is made to the registers.
  *         In Standby mode a wakeup is generated. In other modes an interrupt can be sent to wakeup
  *         the software (see Section 5.3.4: Clock interrupt register (RCC_CIR) on page 104).
  *         The software MUST then disable the LSECSSON bit, stop the defective 32 kHz oscillator
  *         (disabling LSEON), and can change the RTC clock source (no clock or LSI or HSE, with
  *         RTCSEL), or take any required action to secure the application.  
  * @note   LSE CSS available only for high density and medium+ devices
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS(void)
{
  *(__IO uint32_t *) CSR_LSECSSON_BB = (uint32_t)ENABLE;
}

/**
  * @brief  Disables the LSE Clock Security System.
  * @note   Once enabled this bit cannot be disabled, except after an LSE failure detection 
  *         (LSECSSD=1). In that case the software MUST disable the LSECSSON bit.
  *         Reset by power on reset and RTC software reset (RTCRST bit).
  * @note   LSE CSS available only for high density and medium+ devices
  * @retval None
  */
void HAL_RCCEx_DisableLSECSS(void)
{
  /* Disable LSE CSS */
  *(__IO uint32_t *) CSR_LSECSSON_BB = (uint32_t)DISABLE;

  /* Disable LSE CSS IT */
  __HAL_RCC_DISABLE_IT(RCC_IT_LSECSS);
}

/**
  * @brief  Enable the LSE Clock Security System IT & corresponding EXTI line.
  * @note   LSE Clock Security System IT is mapped on RTC EXTI line 19
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS_IT(void)
{
  /* Enable LSE CSS */
  *(__IO uint32_t *) CSR_LSECSSON_BB = (uint32_t)ENABLE;

  /* Enable LSE CSS IT */
  __HAL_RCC_ENABLE_IT(RCC_IT_LSECSS);
  
  /* Enable IT on EXTI Line 19 */
  __HAL_RCC_LSECSS_EXTI_ENABLE_IT();
  __HAL_RCC_LSECSS_EXTI_ENABLE_RISING_EDGE();
}

/**
  * @brief Handle the RCC LSE Clock Security System interrupt request.
  * @retval None
  */
void HAL_RCCEx_LSECSS_IRQHandler(void)
{
  /* Check RCC LSE CSSF flag  */
  if(__HAL_RCC_GET_IT(RCC_IT_LSECSS))
  {
    /* RCC LSE Clock Security System interrupt user callback */
    HAL_RCCEx_LSECSS_Callback();

    /* Clear RCC LSE CSS pending bit */
    __HAL_RCC_CLEAR_IT(RCC_IT_LSECSS);
  }
}                                                                            

/**
  * @brief  RCCEx LSE Clock Security System interrupt callback.
  * @retval none
  */
__weak void HAL_RCCEx_LSECSS_Callback(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the @ref HAL_RCCEx_LSECSS_Callback should be implemented in the user file
   */
}
#endif /* RCC_LSECSS_SUPPORT */
  
/**
  * @}
  */

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
