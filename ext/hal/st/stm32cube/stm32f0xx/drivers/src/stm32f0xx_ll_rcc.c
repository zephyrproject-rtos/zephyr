/**
  ******************************************************************************
  * @file    stm32f0xx_ll_rcc.c
  * @author  MCD Application Team
  * @brief   RCC LL module driver.
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
#include "stm32f0xx_ll_rcc.h"
#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */
/** @addtogroup STM32F0xx_LL_Driver
  * @{
  */

#if defined(RCC)

/** @defgroup RCC_LL RCC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @addtogroup RCC_LL_Private_Macros
  * @{
  */
#if defined(RCC_CFGR3_USART2SW) && defined(RCC_CFGR3_USART3SW)
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART2_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART3_CLKSOURCE))
#elif defined(RCC_CFGR3_USART2SW) && !defined(RCC_CFGR3_USART3SW)
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART2_CLKSOURCE))
#elif defined(RCC_CFGR3_USART3SW) && !defined(RCC_CFGR3_USART2SW)
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART3_CLKSOURCE))
#else
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE))
#endif /* RCC_CFGR3_USART2SW && RCC_CFGR3_USART3SW */

#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)     ((__VALUE__) == LL_RCC_I2C1_CLKSOURCE)

#if defined(USB)
#define IS_LL_RCC_USB_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_USB_CLKSOURCE))
#endif /* USB */

#if defined(CEC)
#define IS_LL_RCC_CEC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_CEC_CLKSOURCE))
#endif /* CEC */

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCC_LL_Private_Functions RCC Private functions
  * @{
  */
uint32_t RCC_GetSystemClockFreq(void);
uint32_t RCC_GetHCLKClockFreq(uint32_t SYSCLK_Frequency);
uint32_t RCC_GetPCLK1ClockFreq(uint32_t HCLK_Frequency);
uint32_t RCC_PLL_GetFreqDomain_SYS(void);
/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCC_LL_Exported_Functions
  * @{
  */

/** @addtogroup RCC_LL_EF_Init
  * @{
  */

/**
  * @brief  Reset the RCC clock configuration to the default reset state.
  * @note   The default reset state of the clock configuration is given below:
  *         - HSI ON and used as system clock source
  *         - HSE and PLL OFF
  *         - AHB and APB1 prescaler set to 1.
  *         - CSS, MCO OFF
  *         - All interrupts disabled
  * @note   This function doesn't modify the configuration of the
  *         - Peripheral clocks
  *         - LSI, LSE and RTC clocks
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RCC registers are de-initialized
  *          - ERROR: not applicable
  */
ErrorStatus LL_RCC_DeInit(void)
{
  uint32_t vl_mask = 0U;

  /* Set HSION bit */
  LL_RCC_HSI_Enable();

  /* Set HSITRIM bits to the reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x10U);

  /* Reset SW, HPRE, PPRE and MCOSEL bits */
  vl_mask = 0xFFFFFFFFU;
  CLEAR_BIT(vl_mask, (RCC_CFGR_SW | RCC_CFGR_HPRE | RCC_CFGR_PPRE | RCC_CFGR_MCOSEL));
  LL_RCC_WriteReg(CFGR, vl_mask);

  /* Reset HSEON, CSSON, PLLON bits */
  vl_mask = 0xFFFFFFFFU;
  CLEAR_BIT(vl_mask, (RCC_CR_PLLON | RCC_CR_CSSON | RCC_CR_HSEON));
  LL_RCC_WriteReg(CR, vl_mask);

  /* Reset HSEBYP bit */
  LL_RCC_HSE_DisableBypass();

  /* Reset CFGR register */
  LL_RCC_WriteReg(CFGR, 0x00000000U);

#if defined(RCC_HSI48_SUPPORT)
  /* Reset CR2 register */
  LL_RCC_WriteReg(CR2, 0x00000000U);

  /* Disable HSI48 */
  LL_RCC_HSI48_Disable();

#endif /*RCC_HSI48_SUPPORT*/
  /* Set HSI14TRIM/HSI14ON/HSI14DIS bits to the reset value*/
  LL_RCC_HSI14_SetCalibTrimming(0x10U);
  LL_RCC_HSI14_Disable();
  LL_RCC_HSI14_EnableADCControl();

  /* Reset CFGR2 register */
  LL_RCC_WriteReg(CFGR2, 0x00000000U);

  /* Reset CFGR3 register */
  LL_RCC_WriteReg(CFGR3, 0x00000000U);

  /* Clear pending flags */
#if defined(RCC_HSI48_SUPPORT)
  vl_mask = (LL_RCC_CIR_LSIRDYC | LL_RCC_CIR_LSERDYC | LL_RCC_CIR_HSIRDYC | LL_RCC_CIR_HSERDYC | LL_RCC_CIR_PLLRDYC | LL_RCC_CIR_HSI14RDYC | LL_RCC_CIR_HSI48RDYC | LL_RCC_CIR_CSSC);
#else
  vl_mask = (LL_RCC_CIR_LSIRDYC | LL_RCC_CIR_LSERDYC | LL_RCC_CIR_HSIRDYC | LL_RCC_CIR_HSERDYC | LL_RCC_CIR_PLLRDYC | LL_RCC_CIR_HSI14RDYC | LL_RCC_CIR_CSSC);
#endif /* RCC_HSI48_SUPPORT */
  SET_BIT(RCC->CIR, vl_mask);

  /* Disable all interrupts */
  LL_RCC_WriteReg(CIR, 0x00000000U);

  return SUCCESS;
}

/**
  * @}
  */

/** @addtogroup RCC_LL_EF_Get_Freq
  * @brief  Return the frequencies of different on chip clocks;  System, AHB and APB1 buses clocks
  *         and different peripheral clocks available on the device.
  * @note   If SYSCLK source is HSI, function returns values based on HSI_VALUE(**)
  * @note   If SYSCLK source is HSE, function returns values based on HSE_VALUE(***)
  * @note   If SYSCLK source is PLL, function returns values based on
  *         HSI_VALUE(**) or HSE_VALUE(***) multiplied/divided by the PLL factors.
  * @note   (**) HSI_VALUE is a defined constant but the real value may vary
  *              depending on the variations in voltage and temperature.
  * @note   (***) HSE_VALUE is a defined constant, user has to ensure that
  *               HSE_VALUE is same as the real frequency of the crystal used.
  *               Otherwise, this function may have wrong result.
  * @note   The result of this function could be incorrect when using fractional
  *         value for HSE crystal.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  * @{
  */

/**
  * @brief  Return the frequencies of different on chip clocks;  System, AHB and APB1 buses clocks
  * @note   Each time SYSCLK, HCLK and/or PCLK1 clock changes, this function
  *         must be called to update structure fields. Otherwise, any
  *         configuration based on this function will be incorrect.
  * @param  RCC_Clocks pointer to a @ref LL_RCC_ClocksTypeDef structure which will hold the clocks frequencies
  * @retval None
  */
void LL_RCC_GetSystemClocksFreq(LL_RCC_ClocksTypeDef *RCC_Clocks)
{
  /* Get SYSCLK frequency */
  RCC_Clocks->SYSCLK_Frequency = RCC_GetSystemClockFreq();

  /* HCLK clock frequency */
  RCC_Clocks->HCLK_Frequency   = RCC_GetHCLKClockFreq(RCC_Clocks->SYSCLK_Frequency);

  /* PCLK1 clock frequency */
  RCC_Clocks->PCLK1_Frequency  = RCC_GetPCLK1ClockFreq(RCC_Clocks->HCLK_Frequency);
}

/**
  * @brief  Return USARTx clock frequency
  * @param  USARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE (*)
  *         @arg @ref LL_RCC_USART3_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices.
  * @retval USART clock frequency (in Hz)
  *         @arg @ref LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetUSARTClockFreq(uint32_t USARTxSource)
{
  uint32_t usart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_USART_CLKSOURCE(USARTxSource));
#if defined(RCC_CFGR3_USART1SW)
  if (USARTxSource == LL_RCC_USART1_CLKSOURCE)
  {
    /* USART1CLK clock frequency */
    switch (LL_RCC_GetUSARTClockSource(USARTxSource))
    {
      case LL_RCC_USART1_CLKSOURCE_SYSCLK: /* USART1 Clock is System Clock */
        usart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_USART1_CLKSOURCE_HSI:    /* USART1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          usart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_USART1_CLKSOURCE_LSE:    /* USART1 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          usart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_USART1_CLKSOURCE_PCLK1:  /* USART1 Clock is PCLK1 */
      default:
        usart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
#endif /* RCC_CFGR3_USART1SW  */

#if defined(RCC_CFGR3_USART2SW)
  if (USARTxSource == LL_RCC_USART2_CLKSOURCE)
  {
    /* USART2CLK clock frequency */
    switch (LL_RCC_GetUSARTClockSource(USARTxSource))
    {
      case LL_RCC_USART2_CLKSOURCE_SYSCLK: /* USART2 Clock is System Clock */
        usart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_USART2_CLKSOURCE_HSI:    /* USART2 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          usart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_USART2_CLKSOURCE_LSE:    /* USART2 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          usart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_USART2_CLKSOURCE_PCLK1:  /* USART2 Clock is PCLK1 */
      default:
        usart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
#endif /* RCC_CFGR3_USART2SW */

#if defined(RCC_CFGR3_USART3SW)
  if (USARTxSource == LL_RCC_USART3_CLKSOURCE)
  {
    /* USART3CLK clock frequency */
    switch (LL_RCC_GetUSARTClockSource(USARTxSource))
    {
      case LL_RCC_USART3_CLKSOURCE_SYSCLK: /* USART3 Clock is System Clock */
        usart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_USART3_CLKSOURCE_HSI:    /* USART3 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          usart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_USART3_CLKSOURCE_LSE:    /* USART3 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          usart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_USART3_CLKSOURCE_PCLK1:  /* USART3 Clock is PCLK1 */
      default:
        usart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }

#endif /* RCC_CFGR3_USART3SW */
  return usart_frequency;
}

/**
  * @brief  Return I2Cx clock frequency
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  * @retval I2C clock frequency (in Hz)
  *         @arg @ref LL_RCC_PERIPH_FREQUENCY_NO indicates that HSI oscillator is not ready
  */
uint32_t LL_RCC_GetI2CClockFreq(uint32_t I2CxSource)
{
  uint32_t i2c_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_I2C_CLKSOURCE(I2CxSource));

  /* I2C1 CLK clock frequency */
  if (I2CxSource == LL_RCC_I2C1_CLKSOURCE)
  {
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C1_CLKSOURCE_SYSCLK: /* I2C1 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_I2C1_CLKSOURCE_HSI:    /* I2C1 Clock is HSI Osc. */
      default:
        if (LL_RCC_HSI_IsReady())
        {
          i2c_frequency = HSI_VALUE;
        }
        break;
    }
  }

  return i2c_frequency;
}

#if defined(USB)
/**
  * @brief  Return USBx clock frequency
  * @param  USBxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE
  * @retval USB clock frequency (in Hz)
  *         @arg @ref LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI48) or PLL is not ready
  *         @arg @ref LL_RCC_PERIPH_FREQUENCY_NA indicates that no clock source selected
  */
uint32_t LL_RCC_GetUSBClockFreq(uint32_t USBxSource)
{
  uint32_t usb_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_USB_CLKSOURCE(USBxSource));

  /* USBCLK clock frequency */
  switch (LL_RCC_GetUSBClockSource(USBxSource))
  {
    case LL_RCC_USB_CLKSOURCE_PLL:        /* PLL clock used as USB clock source */
      if (LL_RCC_PLL_IsReady())
      {
        usb_frequency = RCC_PLL_GetFreqDomain_SYS();
      }
      break;

#if defined(RCC_CFGR3_USBSW_HSI48)
    case LL_RCC_USB_CLKSOURCE_HSI48:      /* HSI48 clock used as USB clock source */
    default:
      if (LL_RCC_HSI48_IsReady())
      {
        usb_frequency = HSI48_VALUE;
      }
      break;
#else
    case LL_RCC_USB_CLKSOURCE_NONE:       /* No clock used as USB clock source */
    default:
      usb_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
#endif /* RCC_CFGR3_USBSW_HSI48 */
  }

  return usb_frequency;
}
#endif /* USB */

#if defined(CEC)
/**
  * @brief  Return CECx clock frequency
  * @param  CECxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval CEC clock frequency (in Hz)
  *        @arg @ref LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillators (HSI or LSE) are not ready
  */
uint32_t LL_RCC_GetCECClockFreq(uint32_t CECxSource)
{
  uint32_t cec_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_CEC_CLKSOURCE(CECxSource));

  /* CECCLK clock frequency */
  switch (LL_RCC_GetCECClockSource(CECxSource))
  {
    case LL_RCC_CEC_CLKSOURCE_HSI_DIV244:   /* HSI / 244 clock used as CEC clock source */
      if (LL_RCC_HSI_IsReady())
      {
        cec_frequency = HSI_VALUE / 244U;
      }
      break;

    case LL_RCC_CEC_CLKSOURCE_LSE:          /* LSE clock used as CEC clock source */
    default:
      if (LL_RCC_LSE_IsReady())
      {
        cec_frequency = LSE_VALUE;
      }
      break;
  }

  return cec_frequency;
}
#endif /* CEC */

/**
  * @}
  */

/**
  * @}
  */

/** @addtogroup RCC_LL_Private_Functions
  * @{
  */

/**
  * @brief  Return SYSTEM clock frequency
  * @retval SYSTEM clock frequency (in Hz)
  */
uint32_t RCC_GetSystemClockFreq(void)
{
  uint32_t frequency = 0U;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (LL_RCC_GetSysClkSource())
  {
    case LL_RCC_SYS_CLKSOURCE_STATUS_HSI:  /* HSI used as system clock  source */
      frequency = HSI_VALUE;
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:  /* HSE used as system clock  source */
      frequency = HSE_VALUE;
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_PLL:  /* PLL used as system clock  source */
      frequency = RCC_PLL_GetFreqDomain_SYS();
      break;

#if defined(RCC_HSI48_SUPPORT)
    case LL_RCC_SYS_CLKSOURCE_STATUS_HSI48:/* HSI48 used as system clock  source */
      frequency = HSI48_VALUE;
      break;
#endif /* RCC_HSI48_SUPPORT */

    default:
      frequency = HSI_VALUE;
      break;
  }

  return frequency;
}

/**
  * @brief  Return HCLK clock frequency
  * @param  SYSCLK_Frequency SYSCLK clock frequency
  * @retval HCLK clock frequency (in Hz)
  */
uint32_t RCC_GetHCLKClockFreq(uint32_t SYSCLK_Frequency)
{
  /* HCLK clock frequency */
  return __LL_RCC_CALC_HCLK_FREQ(SYSCLK_Frequency, LL_RCC_GetAHBPrescaler());
}

/**
  * @brief  Return PCLK1 clock frequency
  * @param  HCLK_Frequency HCLK clock frequency
  * @retval PCLK1 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK1ClockFreq(uint32_t HCLK_Frequency)
{
  /* PCLK1 clock frequency */
  return __LL_RCC_CALC_PCLK1_FREQ(HCLK_Frequency, LL_RCC_GetAPB1Prescaler());
}
/**
  * @brief  Return PLL clock frequency used for system domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_SYS(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL divider) * PLL Multiplicator */

  /* Get PLL source */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
    case LL_RCC_PLLSOURCE_HSI:       /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
#else
    case LL_RCC_PLLSOURCE_HSI_DIV_2: /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE / 2U;
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
      break;

#if defined(RCC_HSI48_SUPPORT)
    case LL_RCC_PLLSOURCE_HSI48:     /* HSI48 used as PLL clock source */
      pllinputfreq = HSI48_VALUE;
      break;
#endif /* RCC_HSI48_SUPPORT */

    case LL_RCC_PLLSOURCE_HSE:       /* HSE used as PLL clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
      pllinputfreq = HSI_VALUE;
#else
      pllinputfreq = HSI_VALUE / 2U;
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
      break;
  }
#if defined(RCC_PLLSRC_PREDIV1_SUPPORT)
  return __LL_RCC_CALC_PLLCLK_FREQ(pllinputfreq, LL_RCC_PLL_GetMultiplicator(), LL_RCC_PLL_GetPrediv());
#else
  return __LL_RCC_CALC_PLLCLK_FREQ((pllinputfreq / (LL_RCC_PLL_GetPrediv() + 1U)), LL_RCC_PLL_GetMultiplicator());
#endif /* RCC_PLLSRC_PREDIV1_SUPPORT */
}
/**
  * @}
  */

/**
  * @}
  */

#endif /* defined(RCC) */

/**
  * @}
  */

#endif /* USE_FULL_LL_DRIVER */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
