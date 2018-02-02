/**
  ******************************************************************************
  * @file    stm32l0xx_hal_rcc_ex.c
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
#include "stm32l0xx_hal.h"

/** @addtogroup STM32L0xx_HAL_Driver
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
#if defined (CRS)
/* Bit position in register */
#define CRS_CFGR_FELIM_BITNUMBER    CRS_CFGR_FELIM_Pos
#define CRS_CR_TRIM_BITNUMBER       CRS_CR_TRIM_Pos
#define CRS_ISR_FECAP_BITNUMBER     CRS_ISR_FECAP_Pos
#endif /* CRS   */

#if defined(USB)
extern const uint8_t PLLMulTable[];
#endif /* USB */
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
  *         contains the configuration information for the Extended Peripherals clocks(USART1,USART2, LPUART1, 
  *         I2C1, I2C3, RTC, USB/RNG  and LPTIM1 clocks).
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
  
#if defined (RCC_CCIPR_USART1SEL)
  /*------------------------------- USART1 Configuration ------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART1) == RCC_PERIPHCLK_USART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART1CLKSOURCE(PeriphClkInit->Usart1ClockSelection));
    
    /* Configure the USART1 clock source */
    __HAL_RCC_USART1_CONFIG(PeriphClkInit->Usart1ClockSelection);
  }
#endif /* RCC_CCIPR_USART1SEL */
  
  /*----------------------------- USART2 Configuration --------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART2) == RCC_PERIPHCLK_USART2)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART2CLKSOURCE(PeriphClkInit->Usart2ClockSelection));
    
    /* Configure the USART2 clock source */
    __HAL_RCC_USART2_CONFIG(PeriphClkInit->Usart2ClockSelection);
  }
  
  /*------------------------------ LPUART1 Configuration ------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPUART1) == RCC_PERIPHCLK_LPUART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LPUART1CLKSOURCE(PeriphClkInit->Lpuart1ClockSelection));
    
    /* Configure the LPUAR1 clock source */
    __HAL_RCC_LPUART1_CONFIG(PeriphClkInit->Lpuart1ClockSelection);
  }

  /*------------------------------ I2C1 Configuration ------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C1) == RCC_PERIPHCLK_I2C1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C1CLKSOURCE(PeriphClkInit->I2c1ClockSelection));
    
    /* Configure the I2C1 clock source */
    __HAL_RCC_I2C1_CONFIG(PeriphClkInit->I2c1ClockSelection);
  }

#if defined (RCC_CCIPR_I2C3SEL)
    /*------------------------------ I2C3 Configuration ------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C3) == RCC_PERIPHCLK_I2C3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C3CLKSOURCE(PeriphClkInit->I2c3ClockSelection));
    
    /* Configure the I2C3 clock source */
    __HAL_RCC_I2C3_CONFIG(PeriphClkInit->I2c3ClockSelection);
  }  
#endif /* RCC_CCIPR_I2C3SEL */

#if defined(USB)
 /*---------------------------- USB and RNG configuration --------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USB) == (RCC_PERIPHCLK_USB))
  {
    assert_param(IS_RCC_USBCLKSOURCE(PeriphClkInit->UsbClockSelection));
    __HAL_RCC_USB_CONFIG(PeriphClkInit->UsbClockSelection);
  }
#endif /* USB */
  
  /*---------------------------- LPTIM1 configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM1) == (RCC_PERIPHCLK_LPTIM1))
  {
    assert_param(IS_RCC_LPTIMCLK(PeriphClkInit->LptimClockSelection));
    __HAL_RCC_LPTIM1_CONFIG(PeriphClkInit->LptimClockSelection);
  }

  return HAL_OK;
}

/**
  * @brief  Get the PeriphClkInit according to the internal RCC configuration registers.
  * @param  PeriphClkInit pointer to an RCC_PeriphCLKInitTypeDef structure that 
  *         returns the configuration information for the Extended Peripherals clocks(USART1,USART2, LPUART1, 
  *         I2C1, I2C3, RTC, USB/RNG  and LPTIM1 clocks).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t srcclk = 0;
  
   /* Set all possible values for the extended clock type parameter -----------*/
  /* Common part first */
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_RTC     | \
                                        RCC_PERIPHCLK_LPTIM1;
#if defined(RCC_CCIPR_USART1SEL)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_USART1;
#endif /* RCC_CCIPR_USART1SEL */
#if  defined(RCC_CCIPR_I2C3SEL)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_I2C3;
#endif /* RCC_CCIPR_I2C3SEL */
#if defined(USB)
  PeriphClkInit->PeriphClockSelection |= RCC_PERIPHCLK_USB;
#endif /* USB */
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
#if defined(RCC_CCIPR_USART1SEL)
  /* Get the USART1 configuration --------------------------------------------*/
  PeriphClkInit->Usart1ClockSelection  = __HAL_RCC_GET_USART1_SOURCE();
#endif /* RCC_CCIPR_USART1SEL */
  /* Get the USART2 clock source ---------------------------------------------*/
  PeriphClkInit->Usart2ClockSelection  = __HAL_RCC_GET_USART2_SOURCE();
  /* Get the LPUART1 clock source ---------------------------------------------*/
  PeriphClkInit->Lpuart1ClockSelection = __HAL_RCC_GET_LPUART1_SOURCE();
  /* Get the I2C1 clock source -----------------------------------------------*/
  PeriphClkInit->I2c1ClockSelection    = __HAL_RCC_GET_I2C1_SOURCE();
#if defined(RCC_CCIPR_I2C3SEL)
/* Get the I2C3 clock source -----------------------------------------------*/
  PeriphClkInit->I2c3ClockSelection    = __HAL_RCC_GET_I2C3_SOURCE();
#endif /* RCC_CCIPR_I2C3SEL */
  /* Get the LPTIM1 clock source -----------------------------------------------*/
  PeriphClkInit->LptimClockSelection   = __HAL_RCC_GET_LPTIM1_SOURCE();
  /* Get the RTC clock source -----------------------------------------------*/
  PeriphClkInit->RTCClockSelection     = __HAL_RCC_GET_RTC_SOURCE();
#if defined(USB)
  /* Get the USB/RNG clock source -----------------------------------------------*/
  PeriphClkInit->UsbClockSelection     = __HAL_RCC_GET_USB_SOURCE();
#endif /* USB */
}

/**
  * @brief  Return the peripheral clock frequency
  * @note   Return 0 if peripheral clock is unknown
  * @param  PeriphClk Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_PERIPHCLK_RTC      RTC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LCD      LCD peripheral clock (*)
  *            @arg @ref RCC_PERIPHCLK_USB      USB or RNG peripheral clock (*)
  *            @arg @ref RCC_PERIPHCLK_USART1   USART1 peripheral clock (*)
  *            @arg @ref RCC_PERIPHCLK_USART2   USART2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPUART1  LPUART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C1     I2C1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C2     I2C2 peripheral clock (*)
  *            @arg @ref RCC_PERIPHCLK_I2C3     I2C3 peripheral clock (*)
  * @note   (*) means that this peripheral is not present on all the devices
  * @retval Frequency in Hz (0: means that no available frequency for the peripheral)
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
  uint32_t temp_reg = 0U, clkprediv = 0U, frequency = 0U;
  uint32_t srcclk = 0U;
#if defined(USB)
    uint32_t pllmul = 0U, plldiv = 0U, pllvco = 0U;
#endif /* USB */

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
#if defined(USB)
   case RCC_PERIPHCLK_USB:
    {  
        /* Get the current USB source */
        srcclk = __HAL_RCC_GET_USB_SOURCE();
        
        if((srcclk == RCC_USBCLKSOURCE_PLL) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_PLLRDY)))
        {
            /* Get PLL clock source and multiplication factor ----------------------*/
            pllmul = RCC->CFGR & RCC_CFGR_PLLMUL;
            plldiv = RCC->CFGR & RCC_CFGR_PLLDIV;
            pllmul = PLLMulTable[(pllmul >> RCC_CFGR_PLLMUL_Pos)];
            plldiv = (plldiv >> RCC_CFGR_PLLDIV_Pos) + 1U;   
            
            /* Compute PLL clock input */
            if(__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSI)
            {
                if (READ_BIT(RCC->CR, RCC_CR_HSIDIVF) != 0U)
                {
                    pllvco =  (HSI_VALUE >> 2U);
                }
                else 
                {
                    pllvco =  HSI_VALUE;
                }
            }
            else /* HSE source */
            {
                pllvco = HSE_VALUE;
            }
            /* pllvco * pllmul / plldiv */
            pllvco = (pllvco * pllmul);
            frequency = (pllvco/ plldiv);
            
        }
        else if((srcclk == RCC_USBCLKSOURCE_HSI48) && (HAL_IS_BIT_SET(RCC->CRRCR, RCC_CRRCR_HSI48RDY)))
        {
            frequency = HSI48_VALUE;
        }
        else /* RCC_USBCLKSOURCE_NONE */
        {
            frequency = 0U;
        }
        break;
    }
#endif /* USB */
#if defined(RCC_CCIPR_USART1SEL)
  case RCC_PERIPHCLK_USART1:
    {
      /* Get the current USART1 source */
      srcclk = __HAL_RCC_GET_USART1_SOURCE();

      /* Check if USART1 clock selection is PCLK2 */
      if (srcclk == RCC_USART1CLKSOURCE_PCLK2)
      {
        frequency = HAL_RCC_GetPCLK2Freq();
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
      else if ((srcclk == RCC_USART1CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Clock not enabled for USART1*/
      else
      {
        frequency = 0U;
      }
      break;
    }
#endif /* RCC_CCIPR_USART1SEL */
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
      else if ((srcclk == RCC_USART2CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Clock not enabled for USART2*/
      else
      {
        frequency = 0U;
      }
      break;
    }
  case RCC_PERIPHCLK_LPUART1:
    {
      /* Get the current LPUART1 source */
      srcclk = __HAL_RCC_GET_LPUART1_SOURCE();

      /* Check if LPUART1 clock selection is PCLK1 */
      if (srcclk == RCC_LPUART1CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if LPUART1 clock selection is HSI */
      else if ((srcclk == RCC_LPUART1CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if LPUART1 clock selection is SYSCLK */
      else if (srcclk == RCC_LPUART1CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Check if LSE is ready  and if LPUART1 clock selection is LSE */
      else if ((srcclk == RCC_LPUART1CLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Clock not enabled for LPUART1*/
      else
      {
        frequency = 0U;
      }
      break;
    }    
  case RCC_PERIPHCLK_I2C1:
    {
      /* Get the current I2C1 source */
      srcclk = __HAL_RCC_GET_I2C1_SOURCE();

      /* Check if I2C1 clock selection is PCLK1 */
      if (srcclk == RCC_I2C1CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if I2C1 clock selection is HSI */
      else if ((srcclk == RCC_I2C1CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if I2C1 clock selection is SYSCLK */
      else if (srcclk == RCC_I2C1CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Clock not enabled for I2C1*/
      else
      {
        frequency = 0U;
      }
      break;
    } 
#if defined(I2C2)
  case RCC_PERIPHCLK_I2C2:
    {

      /* Check if I2C2 on APB1 clock enabled*/
      if (READ_BIT(RCC->APB1ENR, (RCC_APB1ENR_I2C2EN))==RCC_APB1ENR_I2C2EN)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      else
      {
        frequency = 0U;
      }
      break;
    } 
#endif /* I2C2 */

#if defined(RCC_CCIPR_I2C3SEL)
  case RCC_PERIPHCLK_I2C3:
    {
      /* Get the current I2C1 source */
      srcclk = __HAL_RCC_GET_I2C3_SOURCE();

      /* Check if I2C3 clock selection is PCLK1 */
      if (srcclk == RCC_I2C3CLKSOURCE_PCLK1)
      {
        frequency = HAL_RCC_GetPCLK1Freq();
      }
      /* Check if HSI is ready and if I2C3 clock selection is HSI */
      else if ((srcclk == RCC_I2C3CLKSOURCE_HSI) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSIRDY)))
      {
        frequency = HSI_VALUE;
      }
      /* Check if I2C3 clock selection is SYSCLK */
      else if (srcclk == RCC_I2C3CLKSOURCE_SYSCLK)
      {
        frequency = HAL_RCC_GetSysClockFreq();
      }
      /* Clock not enabled for I2C3*/
      else
      {
        frequency = 0U;
      }
      break;
    } 
#endif /* RCC_CCIPR_I2C3SEL */
  default: 
    {
      break;
    }
  }
  return(frequency);
}

/**
  * @brief  Enables the LSE Clock Security System.
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS(void)
{
  SET_BIT(RCC->CSR, RCC_CSR_LSECSSON) ;
}

/**
  * @brief  Disables the LSE Clock Security System.
  * @note   Once enabled this bit cannot be disabled, except after an LSE failure detection 
  *         (LSECSSD=1). In that case the software MUST disable the LSECSSON bit.
  *         Reset by power on reset and RTC software reset (RTCRST bit).
  * @retval None
  */
void HAL_RCCEx_DisableLSECSS(void)
{
  /* Disable LSE CSS */
   CLEAR_BIT(RCC->CSR, RCC_CSR_LSECSSON) ;

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
   SET_BIT(RCC->CSR, RCC_CSR_LSECSSON) ;

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
  
#if defined(SYSCFG_CFGR3_ENREF_HSI48)
/**
  * @brief Enables Vrefint for the HSI48.
  * @note  This is functional only if the LOCK is not set  
  * @retval None
  */
void HAL_RCCEx_EnableHSI48_VREFINT(void)
{
  /* Enable the Buffer for the ADC by setting SYSCFG_CFGR3_ENREF_HSI48 bit in SYSCFG_CFGR3 register   */
  SET_BIT (SYSCFG->CFGR3, SYSCFG_CFGR3_ENREF_HSI48);
}

/**
  * @brief Disables the Vrefint for the HSI48.
  * @note  This is functional only if the LOCK is not set  
  * @retval None
  */
void HAL_RCCEx_DisableHSI48_VREFINT(void)
{
  /* Disable the Vrefint by resetting SYSCFG_CFGR3_ENREF_HSI48 bit in SYSCFG_CFGR3 register */
  CLEAR_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_ENREF_HSI48);
}

#endif /* SYSCFG_CFGR3_ENREF_HSI48 */

/**
  * @}
  */

#if defined (CRS)

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
  uint32_t value = 0;
  
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
