/**
  ******************************************************************************
  * @file    stm32f7xx_ll_rcc.c
  * @author  MCD Application Team
  * @brief   RCC LL module driver.
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_ll_rcc.h"
#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif

/** @addtogroup STM32F7xx_LL_Driver
  * @{
  */

#if defined(RCC)

/** @addtogroup RCC_LL
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/** @addtogroup RCC_LL_Private_Macros
  * @{
  */
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART2_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART3_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART6_CLKSOURCE))

#define IS_LL_RCC_UART_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_UART4_CLKSOURCE) \
                                             || ((__VALUE__) == LL_RCC_UART5_CLKSOURCE) \
                                             || ((__VALUE__) == LL_RCC_UART7_CLKSOURCE) \
                                             || ((__VALUE__) == LL_RCC_UART8_CLKSOURCE))

#if defined(I2C4)
#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2C1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C2_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C3_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C4_CLKSOURCE))
#else
#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2C1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C2_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C3_CLKSOURCE))
#endif /* I2C4 */

#define IS_LL_RCC_LPTIM_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_LPTIM1_CLKSOURCE))

#define IS_LL_RCC_SAI_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_SAI1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_SAI2_CLKSOURCE))

#if defined(SDMMC2)
#define IS_LL_RCC_SDMMC_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_SDMMC1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_SDMMC2_CLKSOURCE))
#else
#define IS_LL_RCC_SDMMC_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_SDMMC1_CLKSOURCE))
#endif /* SDMMC2 */

#define IS_LL_RCC_RNG_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_RNG_CLKSOURCE))

#define IS_LL_RCC_USB_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_USB_CLKSOURCE))

#if defined(DFSDM1_Channel0)
#define IS_LL_RCC_DFSDM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_DFSDM1_CLKSOURCE))

#define IS_LL_RCC_DFSDM_AUDIO_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_DFSDM1_AUDIO_CLKSOURCE))
#endif /* DFSDM1_Channel0 */

#define IS_LL_RCC_I2S_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2S1_CLKSOURCE))

#if defined(CEC)
#define IS_LL_RCC_CEC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_CEC_CLKSOURCE))
#endif /* CEC */

#if defined(DSI)
#define IS_LL_RCC_DSI_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_DSI_CLKSOURCE))
#endif /* DSI */

#if defined(LTDC)
#define IS_LL_RCC_LTDC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_LTDC_CLKSOURCE))
#endif /* LTDC */

#if defined(SPDIFRX)
#define IS_LL_RCC_SPDIFRX_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_SPDIFRX1_CLKSOURCE))
#endif /* SPDIFRX */

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
uint32_t RCC_GetPCLK2ClockFreq(uint32_t HCLK_Frequency);
uint32_t RCC_PLL_GetFreqDomain_SYS(void);
uint32_t RCC_PLL_GetFreqDomain_SAI(void);
uint32_t RCC_PLL_GetFreqDomain_48M(void);
#if defined(DSI)
uint32_t RCC_PLL_GetFreqDomain_DSI(void);
#endif /* DSI */
uint32_t RCC_PLLSAI_GetFreqDomain_SAI(void);
uint32_t RCC_PLLSAI_GetFreqDomain_48M(void);
#if defined(LTDC)
uint32_t RCC_PLLSAI_GetFreqDomain_LTDC(void);
#endif /* LTDC */
uint32_t RCC_PLLI2S_GetFreqDomain_I2S(void);
uint32_t RCC_PLLI2S_GetFreqDomain_SAI(void);
#if defined(SPDIFRX)
uint32_t RCC_PLLI2S_GetFreqDomain_SPDIFRX(void);
#endif /* SPDIFRX */
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
  *         - AHB, APB1 and APB2 prescaler set to 1.
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

  /* Reset CFGR register */
  LL_RCC_WriteReg(CFGR, 0x00000000U);

  vl_mask = 0xFFFFFFFFU;

  /* Reset HSEON, PLLSYSON bits */
  CLEAR_BIT(vl_mask, (RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_PLLON | RCC_CR_CSSON));

  /* Reset PLLSAION bit */
  CLEAR_BIT(vl_mask, RCC_CR_PLLSAION);

  /* Reset PLLI2SON bit */
  CLEAR_BIT(vl_mask, RCC_CR_PLLI2SON);

  /* Write new mask in CR register */
  LL_RCC_WriteReg(CR, vl_mask);

  /* Set HSITRIM bits to the reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x10U);

  /* Reset PLLCFGR register */
  LL_RCC_WriteReg(PLLCFGR, 0x24003010U);

  /* Reset PLLI2SCFGR register */
  LL_RCC_WriteReg(PLLI2SCFGR, 0x24003000U);

  /* Reset PLLSAICFGR register */
  LL_RCC_WriteReg(PLLSAICFGR, 0x24003000U);

  /* Reset HSEBYP bit */
  LL_RCC_HSE_DisableBypass();

  /* Disable all interrupts */
  LL_RCC_WriteReg(CIR, 0x00000000U);

  return SUCCESS;
}

/**
  * @}
  */

/** @addtogroup RCC_LL_EF_Get_Freq
  * @brief  Return the frequencies of different on chip clocks;  System, AHB, APB1 and APB2 buses clocks
  *         and different peripheral clocks available on the device.
  * @note   If SYSCLK source is HSI, function returns values based on HSI_VALUE(**)
  * @note   If SYSCLK source is HSE, function returns values based on HSE_VALUE(***)
  * @note   If SYSCLK source is PLL, function returns values based on HSE_VALUE(***)
  *         or HSI_VALUE(**) multiplied/divided by the PLL factors.
  * @note   (**) HSI_VALUE is a constant defined in this file (default value
  *              16 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  * @note   (***) HSE_VALUE is a constant defined in this file (default value
  *               25 MHz), user has to ensure that HSE_VALUE is same as the real
  *               frequency of the crystal used. Otherwise, this function may
  *               have wrong result.
  * @note   The result of this function could be incorrect when using fractional
  *         value for HSE crystal.
  * @note   This function can be used by the user application to compute the
  *         baud-rate for the communication peripherals or configure other parameters.
  * @{
  */

/**
  * @brief  Return the frequencies of different on chip clocks;  System, AHB, APB1 and APB2 buses clocks
  * @note   Each time SYSCLK, HCLK, PCLK1 and/or PCLK2 clock changes, this function
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

  /* PCLK2 clock frequency */
  RCC_Clocks->PCLK2_Frequency  = RCC_GetPCLK2ClockFreq(RCC_Clocks->HCLK_Frequency);
}

/**
  * @brief  Return USARTx clock frequency
  * @param  USARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  *         @arg @ref LL_RCC_USART2_CLKSOURCE
  *         @arg @ref LL_RCC_USART3_CLKSOURCE
  *         @arg @ref LL_RCC_USART6_CLKSOURCE
  * @retval USART clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetUSARTClockFreq(uint32_t USARTxSource)
{
  uint32_t usart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_USART_CLKSOURCE(USARTxSource));

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

      case LL_RCC_USART1_CLKSOURCE_PCLK2:  /* USART1 Clock is PCLK2 */
      default:
        usart_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else if (USARTxSource == LL_RCC_USART2_CLKSOURCE)
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
  else if (USARTxSource == LL_RCC_USART6_CLKSOURCE)
  {
    /* USART6CLK clock frequency */
    switch (LL_RCC_GetUSARTClockSource(USARTxSource))
    {
      case LL_RCC_USART6_CLKSOURCE_SYSCLK: /* USART6 Clock is System Clock */
        usart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_USART6_CLKSOURCE_HSI:    /* USART6 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          usart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_USART6_CLKSOURCE_LSE:    /* USART6 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          usart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_USART6_CLKSOURCE_PCLK2:  /* USART6 Clock is PCLK2 */
      default:
        usart_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else
  {
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
  }
  return usart_frequency;
}

/**
  * @brief  Return UARTx clock frequency
  * @param  UARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_UART4_CLKSOURCE
  *         @arg @ref LL_RCC_UART5_CLKSOURCE
  *         @arg @ref LL_RCC_UART7_CLKSOURCE
  *         @arg @ref LL_RCC_UART8_CLKSOURCE
  * @retval UART clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetUARTClockFreq(uint32_t UARTxSource)
{
  uint32_t uart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_UART_CLKSOURCE(UARTxSource));

  if (UARTxSource == LL_RCC_UART4_CLKSOURCE)
  {
    /* UART4CLK clock frequency */
    switch (LL_RCC_GetUARTClockSource(UARTxSource))
    {
      case LL_RCC_UART4_CLKSOURCE_SYSCLK: /* UART4 Clock is System Clock */
        uart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_UART4_CLKSOURCE_HSI:    /* UART4 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          uart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_UART4_CLKSOURCE_LSE:    /* UART4 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          uart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_UART4_CLKSOURCE_PCLK1:  /* UART4 Clock is PCLK1 */
      default:
        uart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else if (UARTxSource == LL_RCC_UART5_CLKSOURCE)
  {
    /* UART5CLK clock frequency */
    switch (LL_RCC_GetUARTClockSource(UARTxSource))
    {
      case LL_RCC_UART5_CLKSOURCE_SYSCLK: /* UART5 Clock is System Clock */
        uart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_UART5_CLKSOURCE_HSI:    /* UART5 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          uart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_UART5_CLKSOURCE_LSE:    /* UART5 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          uart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_UART5_CLKSOURCE_PCLK1:  /* UART5 Clock is PCLK1 */
      default:
        uart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else if (UARTxSource == LL_RCC_UART7_CLKSOURCE)
  {
    /* UART7CLK clock frequency */
    switch (LL_RCC_GetUARTClockSource(UARTxSource))
    {
      case LL_RCC_UART7_CLKSOURCE_SYSCLK: /* UART7 Clock is System Clock */
        uart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_UART7_CLKSOURCE_HSI:    /* UART7 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          uart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_UART7_CLKSOURCE_LSE:    /* UART7 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          uart_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_UART7_CLKSOURCE_PCLK1:  /* UART7 Clock is PCLK1 */
      default:
        uart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else
  {
    if (UARTxSource == LL_RCC_UART8_CLKSOURCE)
    {
      /* UART8CLK clock frequency */
      switch (LL_RCC_GetUARTClockSource(UARTxSource))
      {
        case LL_RCC_UART8_CLKSOURCE_SYSCLK: /* UART8 Clock is System Clock */
          uart_frequency = RCC_GetSystemClockFreq();
          break;

        case LL_RCC_UART8_CLKSOURCE_HSI:    /* UART8 Clock is HSI Osc. */
          if (LL_RCC_HSI_IsReady())
          {
            uart_frequency = HSI_VALUE;
          }
          break;

        case LL_RCC_UART8_CLKSOURCE_LSE:    /* UART8 Clock is LSE Osc. */
          if (LL_RCC_LSE_IsReady())
          {
            uart_frequency = LSE_VALUE;
          }
          break;

        case LL_RCC_UART8_CLKSOURCE_PCLK1:  /* UART8 Clock is PCLK1 */
        default:
          uart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
          break;
      }
    }
  }
  return uart_frequency;
}

/**
  * @brief  Return I2Cx clock frequency
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE
  *         @arg @ref LL_RCC_I2C4_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices.
  * @retval I2C clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that HSI oscillator is not ready
  */
uint32_t LL_RCC_GetI2CClockFreq(uint32_t I2CxSource)
{
  uint32_t i2c_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_I2C_CLKSOURCE(I2CxSource));

  if (I2CxSource == LL_RCC_I2C1_CLKSOURCE)
  {
    /* I2C1 CLK clock frequency */
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C1_CLKSOURCE_SYSCLK: /* I2C1 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_I2C1_CLKSOURCE_HSI:    /* I2C1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          i2c_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_I2C1_CLKSOURCE_PCLK1:  /* I2C1 Clock is PCLK1 */
      default:
        i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else if (I2CxSource == LL_RCC_I2C2_CLKSOURCE)
  {
    /* I2C2 CLK clock frequency */
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C2_CLKSOURCE_SYSCLK: /* I2C2 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_I2C2_CLKSOURCE_HSI:    /* I2C2 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          i2c_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_I2C2_CLKSOURCE_PCLK1:  /* I2C2 Clock is PCLK1 */
      default:
        i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else if (I2CxSource == LL_RCC_I2C3_CLKSOURCE)
  {
    /* I2C3 CLK clock frequency */
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C3_CLKSOURCE_SYSCLK: /* I2C3 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_I2C3_CLKSOURCE_HSI:    /* I2C3 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          i2c_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_I2C3_CLKSOURCE_PCLK1:  /* I2C3 Clock is PCLK1 */
      default:
        i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
#if defined(I2C4)
  else
  {
    if (I2CxSource == LL_RCC_I2C4_CLKSOURCE)
    {
      /* I2C4 CLK clock frequency */
      switch (LL_RCC_GetI2CClockSource(I2CxSource))
      {
        case LL_RCC_I2C4_CLKSOURCE_SYSCLK: /* I2C4 Clock is System Clock */
          i2c_frequency = RCC_GetSystemClockFreq();
          break;

        case LL_RCC_I2C4_CLKSOURCE_HSI:    /* I2C4 Clock is HSI Osc. */
          if (LL_RCC_HSI_IsReady())
          {
            i2c_frequency = HSI_VALUE;
          }
          break;

        case LL_RCC_I2C4_CLKSOURCE_PCLK1:  /* I2C4 Clock is PCLK1 */
        default:
          i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
          break;
      }
    }
  }
#endif /* I2C4 */

  return i2c_frequency;
}

/**
  * @brief  Return I2Sx clock frequency
  * @param  I2SxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE
  * @retval I2S clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that PLLI2S oscillator is not ready
  */
uint32_t LL_RCC_GetI2SClockFreq(uint32_t I2SxSource)
{
  uint32_t i2s_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_I2S_CLKSOURCE(I2SxSource));

  if (I2SxSource == LL_RCC_I2S1_CLKSOURCE)
  {
    /* I2S1 CLK clock frequency */
    switch (LL_RCC_GetI2SClockSource(I2SxSource))
    {
      case LL_RCC_I2S1_CLKSOURCE_PLLI2S:       /* I2S1 Clock is PLLI2S */
        if (LL_RCC_PLLI2S_IsReady())
        {
          i2s_frequency = RCC_PLLI2S_GetFreqDomain_I2S();
        }
        break;

      case LL_RCC_I2S1_CLKSOURCE_PIN:          /* I2S1 Clock is External clock */
      default:
        i2s_frequency = EXTERNAL_CLOCK_VALUE;
        break;
    }
  }

  return i2s_frequency;
}

/**
  * @brief  Return LPTIMx clock frequency
  * @param  LPTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  * @retval LPTIM clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI, LSI or LSE) is not ready
  */
uint32_t LL_RCC_GetLPTIMClockFreq(uint32_t LPTIMxSource)
{
  uint32_t lptim_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_LPTIM_CLKSOURCE(LPTIMxSource));

  if (LPTIMxSource == LL_RCC_LPTIM1_CLKSOURCE)
  {
    /* LPTIM1CLK clock frequency */
    switch (LL_RCC_GetLPTIMClockSource(LPTIMxSource))
    {
      case LL_RCC_LPTIM1_CLKSOURCE_LSI:    /* LPTIM1 Clock is LSI Osc. */
        if (LL_RCC_LSI_IsReady())
        {
          lptim_frequency = LSI_VALUE;
        }
        break;

      case LL_RCC_LPTIM1_CLKSOURCE_HSI:    /* LPTIM1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady())
        {
          lptim_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_LPTIM1_CLKSOURCE_LSE:    /* LPTIM1 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady())
        {
          lptim_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_LPTIM1_CLKSOURCE_PCLK1:  /* LPTIM1 Clock is PCLK1 */
      default:
        lptim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }

  return lptim_frequency;
}

/**
  * @brief  Return SAIx clock frequency
  * @param  SAIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE
  *         @arg @ref LL_RCC_SAI2_CLKSOURCE
  * @retval SAI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that PLL is not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that external clock is used
  */
uint32_t LL_RCC_GetSAIClockFreq(uint32_t SAIxSource)
{
  uint32_t sai_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_SAI_CLKSOURCE(SAIxSource));

  if (SAIxSource == LL_RCC_SAI1_CLKSOURCE)
  {
    /* SAI1CLK clock frequency */
    switch (LL_RCC_GetSAIClockSource(SAIxSource))
    {
      case LL_RCC_SAI1_CLKSOURCE_PLLSAI:    /* PLLSAI clock used as SAI1 clock source */
        if (LL_RCC_PLLSAI_IsReady())
        {
          sai_frequency = RCC_PLLSAI_GetFreqDomain_SAI();
        }
        break;

      case LL_RCC_SAI1_CLKSOURCE_PLLI2S:    /* PLLI2S clock used as SAI1 clock source */
        if (LL_RCC_PLLI2S_IsReady())
        {
          sai_frequency = RCC_PLLI2S_GetFreqDomain_SAI();
        }
        break;

#if defined(RCC_SAI1SEL_PLLSRC_SUPPORT)
      case LL_RCC_SAI1_CLKSOURCE_PLLSRC:
        switch (LL_RCC_PLL_GetMainSource())
        {
           case LL_RCC_PLLSOURCE_HSE:       /* HSE clock used as SAI1 clock source */
             if (LL_RCC_HSE_IsReady())
             {
               sai_frequency = HSE_VALUE;
             }
             break;

           case LL_RCC_PLLSOURCE_HSI:       /* HSI clock used as SAI1 clock source */
           default:
             if (LL_RCC_HSI_IsReady())
             {
               sai_frequency = HSI_VALUE;
             }
             break;
        }
        break;
#endif /* RCC_SAI1SEL_PLLSRC_SUPPORT */
      case LL_RCC_SAI1_CLKSOURCE_PIN:        /* External input clock used as SAI1 clock source */
      default:
        sai_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
        break;
    }
  }
  else
  {
    if (SAIxSource == LL_RCC_SAI2_CLKSOURCE)
    {
      /* SAI2CLK clock frequency */
      switch (LL_RCC_GetSAIClockSource(SAIxSource))
      {
      case LL_RCC_SAI2_CLKSOURCE_PLLSAI:    /* PLLSAI clock used as SAI2 clock source */
        if (LL_RCC_PLLSAI_IsReady())
        {
          sai_frequency = RCC_PLLSAI_GetFreqDomain_SAI();
        }
        break;

      case LL_RCC_SAI2_CLKSOURCE_PLLI2S:    /* PLLI2S clock used as SAI2 clock source */
        if (LL_RCC_PLLI2S_IsReady())
        {
          sai_frequency = RCC_PLLI2S_GetFreqDomain_SAI();
        }
        break;

#if defined(RCC_SAI2SEL_PLLSRC_SUPPORT)
      case LL_RCC_SAI2_CLKSOURCE_PLLSRC:
        switch (LL_RCC_PLL_GetMainSource())
        {
           case LL_RCC_PLLSOURCE_HSE:       /* HSE clock used as SAI2 clock source */
             if (LL_RCC_HSE_IsReady())
             {
               sai_frequency = HSE_VALUE;
             }
             break;

           case LL_RCC_PLLSOURCE_HSI:       /* HSI clock used as SAI2 clock source */
           default:
             if (LL_RCC_HSI_IsReady())
             {
               sai_frequency = HSI_VALUE;
             }
             break;
        }
        break;
#endif /* RCC_SAI2SEL_PLLSRC_SUPPORT */
        case LL_RCC_SAI2_CLKSOURCE_PIN:      /* External input clock used as SAI2 clock source */
        default:
          sai_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
          break;
      }
    }
  }

  return sai_frequency;
}

/**
  * @brief  Return SDMMCx clock frequency
  * @param  SDMMCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDMMC1_CLKSOURCE
  *         @arg @ref LL_RCC_SDMMC2_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices.
  * @retval SDMMC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator PLL is not ready
  */
uint32_t LL_RCC_GetSDMMCClockFreq(uint32_t SDMMCxSource)
{
  uint32_t sdmmc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_SDMMC_CLKSOURCE(SDMMCxSource));

  if (SDMMCxSource == LL_RCC_SDMMC1_CLKSOURCE)
  {
    /* SDMMC1CLK clock frequency */
    switch (LL_RCC_GetSDMMCClockSource(SDMMCxSource))
    {
      case LL_RCC_SDMMC1_CLKSOURCE_PLL48CLK:        /* PLL48 clock used as SDMMC1 clock source */
        switch (LL_RCC_GetCK48MClockSource(LL_RCC_CK48M_CLKSOURCE))
        {
          case LL_RCC_CK48M_CLKSOURCE_PLL:         /* PLL clock used as 48Mhz domain clock */
            if (LL_RCC_PLL_IsReady())
            {
              sdmmc_frequency = RCC_PLL_GetFreqDomain_48M();
            }
          break;

          case LL_RCC_CK48M_CLKSOURCE_PLLSAI:      /* PLLSAI clock used as 48Mhz domain clock */
          default:
            if (LL_RCC_PLLSAI_IsReady())
            {
              sdmmc_frequency = RCC_PLLSAI_GetFreqDomain_48M();
            }
            break;
        }
        break;

      case LL_RCC_SDMMC1_CLKSOURCE_SYSCLK:        /* PLL clock used as SDMMC1 clock source */
      default:
      sdmmc_frequency = RCC_GetSystemClockFreq();
      break;
    }
  }
#if defined(SDMMC2)
  else
  {
     /* SDMMC2CLK clock frequency */
     switch (LL_RCC_GetSDMMCClockSource(SDMMCxSource))
     {
       case LL_RCC_SDMMC2_CLKSOURCE_PLL48CLK:        /* PLL48 clock used as SDMMC2 clock source */
         switch (LL_RCC_GetCK48MClockSource(LL_RCC_CK48M_CLKSOURCE))
         {
           case LL_RCC_CK48M_CLKSOURCE_PLL:         /* PLL clock used as 48Mhz domain clock */
             if (LL_RCC_PLL_IsReady())
             {
               sdmmc_frequency = RCC_PLL_GetFreqDomain_48M();
             }
           break;

           case LL_RCC_CK48M_CLKSOURCE_PLLSAI:      /* PLLSAI clock used as 48Mhz domain clock */
           default:
             if (LL_RCC_PLLSAI_IsReady())
             {
               sdmmc_frequency = RCC_PLLSAI_GetFreqDomain_48M();
             }
             break;
         }
         break;

       case LL_RCC_SDMMC2_CLKSOURCE_SYSCLK:        /* PLL clock used as SDMMC2 clock source */
       default:
       sdmmc_frequency = RCC_GetSystemClockFreq();
       break;
     }
  }
#endif /* SDMMC2 */

  return sdmmc_frequency;
}

/**
  * @brief  Return RNGx clock frequency
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE
  * @retval RNG clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetRNGClockFreq(uint32_t RNGxSource)
{
  uint32_t rng_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_RNG_CLKSOURCE(RNGxSource));

  /* RNGCLK clock frequency */
  switch (LL_RCC_GetRNGClockSource(RNGxSource))
  {
    case LL_RCC_RNG_CLKSOURCE_PLL:           /* PLL clock used as RNG clock source */
      if (LL_RCC_PLL_IsReady())
      {
        rng_frequency = RCC_PLL_GetFreqDomain_48M();
      }
      break;

    case LL_RCC_RNG_CLKSOURCE_PLLSAI:       /* PLLSAI clock used as RNG clock source */
    default:
      if (LL_RCC_PLLSAI_IsReady())
      {
        rng_frequency = RCC_PLLSAI_GetFreqDomain_48M();
      }
      break;
  }

  return rng_frequency;
}

#if defined(CEC)
/**
  * @brief  Return CEC clock frequency
  * @param  CECxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CEC_CLKSOURCE
  * @retval CEC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetCECClockFreq(uint32_t CECxSource)
{
  uint32_t cec_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_CEC_CLKSOURCE(CECxSource));

  /* CECCLK clock frequency */
  switch (LL_RCC_GetCECClockSource(CECxSource))
  {
    case LL_RCC_CEC_CLKSOURCE_LSE:           /* CEC Clock is LSE Osc. */
      if (LL_RCC_LSE_IsReady())
      {
        cec_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_CEC_CLKSOURCE_HSI_DIV488:    /* CEC Clock is HSI Osc. */
    default:
      if (LL_RCC_HSI_IsReady())
      {
        cec_frequency = HSI_VALUE/488U;
      }
      break;
  }

  return cec_frequency;
}
#endif /* CEC */

/**
  * @brief  Return USBx clock frequency
  * @param  USBxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USB_CLKSOURCE
  * @retval USB clock frequency (in Hz)
  */
uint32_t LL_RCC_GetUSBClockFreq(uint32_t USBxSource)
{
  uint32_t usb_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_USB_CLKSOURCE(USBxSource));

  /* USBCLK clock frequency */
  switch (LL_RCC_GetUSBClockSource(USBxSource))
  {
    case LL_RCC_USB_CLKSOURCE_PLL:           /* PLL clock used as USB clock source */
      if (LL_RCC_PLL_IsReady())
      {
        usb_frequency = RCC_PLL_GetFreqDomain_48M();
      }
      break;

    case LL_RCC_USB_CLKSOURCE_PLLSAI:       /* PLLSAI clock used as USB clock source */
    default:
      if (LL_RCC_PLLSAI_IsReady())
      {
        usb_frequency = RCC_PLLSAI_GetFreqDomain_48M();
      }
      break;
  }

  return usb_frequency;
}

#if defined(DFSDM1_Channel0)
/**
  * @brief  Return DFSDMx clock frequency
  * @param  DFSDMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_CLKSOURCE
  * @retval DFSDM clock frequency (in Hz)
  */
uint32_t LL_RCC_GetDFSDMClockFreq(uint32_t DFSDMxSource)
{
  uint32_t dfsdm_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_DFSDM_CLKSOURCE(DFSDMxSource));

  /* DFSDM1CLK clock frequency */
  switch (LL_RCC_GetDFSDMClockSource(DFSDMxSource))
  {
    case LL_RCC_DFSDM1_CLKSOURCE_SYSCLK:     /* DFSDM1 Clock is SYSCLK */
      dfsdm_frequency = RCC_GetSystemClockFreq();
      break;

    case LL_RCC_DFSDM1_CLKSOURCE_PCLK2:      /* DFSDM1 Clock is PCLK2 */
    default:
      dfsdm_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
      break;
  }

  return dfsdm_frequency;
}

/**
  * @brief  Return DFSDMx Audio clock frequency
  * @param  DFSDMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DFSDM1_AUDIO_CLKSOURCE
  * @retval DFSDM clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetDFSDMAudioClockFreq(uint32_t DFSDMxSource)
{
  uint32_t dfsdm_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_DFSDM_AUDIO_CLKSOURCE(DFSDMxSource));

  /* DFSDM1CLK clock frequency */
  switch (LL_RCC_GetDFSDMAudioClockSource(DFSDMxSource))
  {
    case LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI1:     /* SAI1 clock used as DFSDM1 audio clock */
      dfsdm_frequency = LL_RCC_GetSAIClockFreq(LL_RCC_SAI1_CLKSOURCE);
      break;

    case LL_RCC_DFSDM1_AUDIO_CLKSOURCE_SAI2:     /* SAI2 clock used as DFSDM1 audio clock */
    default:
      dfsdm_frequency = LL_RCC_GetSAIClockFreq(LL_RCC_SAI2_CLKSOURCE);
      break;
  }

  return dfsdm_frequency;
}
#endif /* DFSDM1_Channel0 */

#if defined(DSI)
/**
  * @brief  Return DSI clock frequency
  * @param  DSIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_DSI_CLKSOURCE
  * @retval DSI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that external clock is used
  */
uint32_t LL_RCC_GetDSIClockFreq(uint32_t DSIxSource)
{
  uint32_t dsi_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_DSI_CLKSOURCE(DSIxSource));

  /* DSICLK clock frequency */
  switch (LL_RCC_GetDSIClockSource(DSIxSource))
  {
    case LL_RCC_DSI_CLKSOURCE_PLL:     /* DSI Clock is PLL Osc. */
      if (LL_RCC_PLL_IsReady())
      {
        dsi_frequency = RCC_PLL_GetFreqDomain_DSI();
      }
      break;

    case LL_RCC_DSI_CLKSOURCE_PHY:    /* DSI Clock is DSI physical clock. */
    default:
      dsi_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }

  return dsi_frequency;
}
#endif /* DSI */

#if defined(LTDC)
/**
  * @brief  Return LTDC clock frequency
  * @param  LTDCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LTDC_CLKSOURCE
  * @retval LTDC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator PLLSAI is not ready
  */
uint32_t LL_RCC_GetLTDCClockFreq(uint32_t LTDCxSource)
{
  uint32_t ltdc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_LTDC_CLKSOURCE(LTDCxSource));

  if (LL_RCC_PLLSAI_IsReady())
  {
     ltdc_frequency = RCC_PLLSAI_GetFreqDomain_LTDC();
  }

  return ltdc_frequency;
}
#endif /* LTDC */

#if defined(SPDIFRX)
/**
  * @brief  Return SPDIFRX clock frequency
  * @param  SPDIFRXxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SPDIFRX1_CLKSOURCE
  * @retval SPDIFRX clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
  */
uint32_t LL_RCC_GetSPDIFRXClockFreq(uint32_t SPDIFRXxSource)
{
  uint32_t spdifrx_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_SPDIFRX_CLKSOURCE(SPDIFRXxSource));

  if (LL_RCC_PLLI2S_IsReady())
  {
     spdifrx_frequency = RCC_PLLI2S_GetFreqDomain_SPDIFRX();
  }

  return spdifrx_frequency;
}
#endif /* SPDIFRX */

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
  * @brief  Return PCLK2 clock frequency
  * @param  HCLK_Frequency HCLK clock frequency
  * @retval PCLK2 clock frequency (in Hz)
  */
uint32_t RCC_GetPCLK2ClockFreq(uint32_t HCLK_Frequency)
{
  /* PCLK2 clock frequency */
  return __LL_RCC_CALC_PCLK2_FREQ(HCLK_Frequency, LL_RCC_GetAPB2Prescaler());
}

/**
  * @brief  Return PLL clock frequency used for system domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_SYS(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
     SYSCLK = PLL_VCO / PLLP
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLCLK_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLL_GetN(), LL_RCC_PLL_GetP());
}

/**
  * @brief  Return PLL clock frequency used for 48 MHz domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_48M(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM ) * PLLN
     48M Domain clock = PLL_VCO / PLLQ
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLCLK_48M_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLL_GetN(), LL_RCC_PLL_GetQ());
}

#if defined(DSI)
/**
  * @brief  Return PLL clock frequency used for DSI clock
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_DSI(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
     DSICLK = PLL_VCO / PLLR
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
      pllinputfreq = HSE_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLCLK_DSI_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLL_GetN(), LL_RCC_PLL_GetR());
}
#endif /* DSI */

/**
  * @brief  Return PLLSAI clock frequency used for SAI1 and SAI2 domains
  * @retval PLLSAI clock frequency (in Hz)
  */
uint32_t RCC_PLLSAI_GetFreqDomain_SAI(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLLSAI_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLSAIN
     SAI1 and SAI2 domains clock  = (PLLSAI_VCO / PLLSAIQ) / PLLSAIDIVQ
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLSAI clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLSAI clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLSAI_SAI_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLSAI_GetN(), LL_RCC_PLLSAI_GetQ(), LL_RCC_PLLSAI_GetDIVQ());
}

/**
  * @brief  Return PLLSAI clock frequency used for 48Mhz domain
  * @retval PLLSAI clock frequency (in Hz)
  */
uint32_t RCC_PLLSAI_GetFreqDomain_48M(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLLSAI_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLSAIN
     48M Domain clock  = PLLSAI_VCO / PLLSAIP
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLSAI clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLSAI clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLSAI_48M_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLSAI_GetN(), LL_RCC_PLLSAI_GetP());
}

#if defined(LTDC)
/**
  * @brief  Return PLLSAI clock frequency used for LTDC domain
  * @retval PLLSAI clock frequency (in Hz)
  */
uint32_t RCC_PLLSAI_GetFreqDomain_LTDC(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLLSAI_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLSAIN
     LTDC Domain clock  = (PLLSAI_VCO / PLLSAIR) / PLLSAIDIVR
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLSAI clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLSAI clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLSAI_LTDC_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLSAI_GetN(), LL_RCC_PLLSAI_GetR(), LL_RCC_PLLSAI_GetDIVR());
}
#endif /* LTDC */

/**
  * @brief  Return PLLI2S clock frequency used for SAI1 and SAI2 domains
  * @retval PLLI2S clock frequency (in Hz)
  */
uint32_t RCC_PLLI2S_GetFreqDomain_SAI(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLLI2S_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLI2SN
     SAI1 and SAI2 domains clock  = (PLLI2S_VCO / PLLI2SQ) / PLLI2SDIVQ
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLI2S clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLI2S clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLI2S_SAI_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLI2S_GetN(), LL_RCC_PLLI2S_GetQ(), LL_RCC_PLLI2S_GetDIVQ());
}

#if defined(SPDIFRX)
/**
  * @brief  Return PLLI2S clock frequency used for SPDIFRX domain
  * @retval PLLI2S clock frequency (in Hz)
  */
uint32_t RCC_PLLI2S_GetFreqDomain_SPDIFRX(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLLI2S_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLI2SN
     SPDIFRX Domain clock  = PLLI2S_VCO / PLLI2SP
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLI2S clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLI2S clock source */
      pllinputfreq = HSE_VALUE;
      break;

    default:
      pllinputfreq = HSI_VALUE;
      break;
  }

  return __LL_RCC_CALC_PLLI2S_SPDIFRX_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLI2S_GetN(), LL_RCC_PLLI2S_GetP());
}
#endif /* SPDIFRX */

/**
  * @brief  Return PLLI2S clock frequency used for I2S domain
  * @retval PLLI2S clock frequency (in Hz)
  */
uint32_t RCC_PLLI2S_GetFreqDomain_I2S(void)
{
  uint32_t pllinputfreq = 0U, pllsource = 0U;

  /* PLLI2S_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLI2SN
     I2S Domain clock  = PLLI2S_VCO / PLLI2SR
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLI2S clock source */
      pllinputfreq = HSE_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLI2S clock source */
    default:
      pllinputfreq = HSI_VALUE;
      break;
  }
  return __LL_RCC_CALC_PLLI2S_I2S_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLI2S_GetN(), LL_RCC_PLLI2S_GetR());
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
