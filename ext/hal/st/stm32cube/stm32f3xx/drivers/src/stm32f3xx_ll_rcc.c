/**
  ******************************************************************************
  * @file    stm32f3xx_ll_rcc.c
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
#include "stm32f3xx_ll_rcc.h"
#ifdef  USE_FULL_ASSERT
  #include "stm32_assert.h"
#else
  #define assert_param(expr) ((void)0U)
#endif /* USE_FULL_ASSERT */
/** @addtogroup STM32F3xx_LL_Driver
  * @{
  */

#if defined(RCC)

/** @defgroup RCC_LL RCC
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/** @addtogroup RCC_LL_Private_Variables
  * @{
  */
#if defined(RCC_CFGR2_ADC1PRES) || defined(RCC_CFGR2_ADCPRE12) || defined(RCC_CFGR2_ADCPRE34)
const uint16_t aADCPrescTable[16]       = {1U, 2U, 4U, 6U, 8U, 10U, 12U, 16U, 32U, 64U, 128U, 256U, 256U, 256U, 256U, 256U};
#endif /* RCC_CFGR2_ADC1PRES || RCC_CFGR2_ADCPRE12 || RCC_CFGR2_ADCPRE34 */
#if defined(RCC_CFGR_SDPRE)
const uint8_t aSDADCPrescTable[16]       = {2U, 4U, 6U, 8U, 10U, 12U, 14U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U};
#endif /* RCC_CFGR_SDPRE */
/**
  * @}
  */


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

#if defined(UART4) && defined(UART5)
#define IS_LL_RCC_UART_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_UART4_CLKSOURCE) \
                                             || ((__VALUE__) == LL_RCC_UART5_CLKSOURCE))
#elif defined(UART4)
#define IS_LL_RCC_UART_INSTANCE(__VALUE__)     ((__VALUE__) == LL_RCC_UART4_CLKSOURCE)
#elif defined(UART5)
#define IS_LL_RCC_UART_INSTANCE(__VALUE__)     ((__VALUE__) == LL_RCC_UART5_CLKSOURCE)
#endif /* UART4 && UART5*/

#if defined(RCC_CFGR3_I2C2SW) && defined(RCC_CFGR3_I2C3SW)
#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2C1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C2_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C3_CLKSOURCE))

#elif defined(RCC_CFGR3_I2C2SW) && !defined(RCC_CFGR3_I2C3SW)
#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2C1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C2_CLKSOURCE))

#elif defined(RCC_CFGR3_I2C3SW) && !defined(RCC_CFGR3_I2C2SW)
#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2C1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C3_CLKSOURCE))

#else
#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)     ((__VALUE__) == LL_RCC_I2C1_CLKSOURCE)
#endif /* RCC_CFGR3_I2C2SW && RCC_CFGR3_I2C3SW */

#define IS_LL_RCC_I2S_CLKSOURCE(__VALUE__)     ((__VALUE__) == LL_RCC_I2S_CLKSOURCE)

#if defined(USB)
#define IS_LL_RCC_USB_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_USB_CLKSOURCE))
#endif /* USB */

#if defined(RCC_CFGR_ADCPRE)
#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_ADC_CLKSOURCE))
#else
#if defined(RCC_CFGR2_ADC1PRES)
#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_ADC1_CLKSOURCE))
#elif  defined(RCC_CFGR2_ADCPRE12) && defined(RCC_CFGR2_ADCPRE34)
#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_ADC12_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_ADC34_CLKSOURCE))
#else /* RCC_CFGR2_ADCPRE12 */
#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_ADC12_CLKSOURCE))
#endif /* RCC_CFGR2_ADC1PRES */
#endif /* RCC_CFGR_ADCPRE */

#if defined(RCC_CFGR_SDPRE)
#define IS_LL_RCC_SDADC_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_SDADC_CLKSOURCE))
#endif /* RCC_CFGR_SDPRE */

#if defined(CEC)
#define IS_LL_RCC_CEC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_CEC_CLKSOURCE))
#endif /* CEC */

#if defined(RCC_CFGR3_TIMSW)
#if defined(RCC_CFGR3_TIM8SW) && defined(RCC_CFGR3_TIM15SW) && defined(RCC_CFGR3_TIM16SW) \
 && defined(RCC_CFGR3_TIM17SW) && defined(RCC_CFGR3_TIM20SW) && defined(RCC_CFGR3_TIM2SW) \
 && defined(RCC_CFGR3_TIM34SW)

#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM2_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM8_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM15_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM16_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM17_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM20_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM34_CLKSOURCE))

#elif !defined(RCC_CFGR3_TIM8SW) && defined(RCC_CFGR3_TIM15SW) && defined(RCC_CFGR3_TIM16SW) \
 && defined(RCC_CFGR3_TIM17SW) && !defined(RCC_CFGR3_TIM20SW) && defined(RCC_CFGR3_TIM2SW) \
 && defined(RCC_CFGR3_TIM34SW)

#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM2_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM15_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM16_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM17_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM34_CLKSOURCE))

#elif defined(RCC_CFGR3_TIM8SW) && !defined(RCC_CFGR3_TIM15SW) && !defined(RCC_CFGR3_TIM16SW) \
 && !defined(RCC_CFGR3_TIM17SW) && !defined(RCC_CFGR3_TIM20SW) && !defined(RCC_CFGR3_TIM2SW) \
 && !defined(RCC_CFGR3_TIM34SW)

#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM8_CLKSOURCE))

#elif !defined(RCC_CFGR3_TIM8SW) && defined(RCC_CFGR3_TIM15SW) && defined(RCC_CFGR3_TIM16SW) \
 && defined(RCC_CFGR3_TIM17SW) && !defined(RCC_CFGR3_TIM20SW) && !defined(RCC_CFGR3_TIM2SW) \
 && !defined(RCC_CFGR3_TIM34SW)

#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE)  \
                                            || ((__VALUE__) == LL_RCC_TIM15_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM16_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM17_CLKSOURCE))

#elif !defined(RCC_CFGR3_TIM8SW) && !defined(RCC_CFGR3_TIM15SW) && !defined(RCC_CFGR3_TIM16SW) \
 && !defined(RCC_CFGR3_TIM17SW) && !defined(RCC_CFGR3_TIM20SW) && !defined(RCC_CFGR3_TIM2SW) \
 && !defined(RCC_CFGR3_TIM34SW)

#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE))

#else
#error "Miss macro"
#endif /* RCC_CFGR3_TIMxSW */
#endif /* RCC_CFGR3_TIMSW */

#if defined(HRTIM1)
#define IS_LL_RCC_HRTIM_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_HRTIM1_CLKSOURCE))
#endif /* HRTIM1 */

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

  /* Set HSITRIM bits to the reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x10U);

  /* Reset SW, HPRE, PPRE and MCOSEL bits */
  vl_mask = 0xFFFFFFFFU;
  CLEAR_BIT(vl_mask, (RCC_CFGR_SW | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | RCC_CFGR_MCOSEL));
  LL_RCC_WriteReg(CFGR, vl_mask);

  /* Reset HSEON, CSSON, PLLON bits */
  vl_mask = 0xFFFFFFFFU;
  CLEAR_BIT(vl_mask, (RCC_CR_PLLON | RCC_CR_CSSON | RCC_CR_HSEON));
  LL_RCC_WriteReg(CR, vl_mask);

  /* Reset HSEBYP bit */
  LL_RCC_HSE_DisableBypass();

  /* Reset CFGR register */
  LL_RCC_WriteReg(CFGR, 0x00000000U);

  /* Reset CFGR2 register */
  LL_RCC_WriteReg(CFGR2, 0x00000000U);

  /* Reset CFGR3 register */
  LL_RCC_WriteReg(CFGR3, 0x00000000U);

  /* Clear pending flags */
  vl_mask = (LL_RCC_CIR_LSIRDYC | LL_RCC_CIR_LSERDYC | LL_RCC_CIR_HSIRDYC | LL_RCC_CIR_HSERDYC | LL_RCC_CIR_PLLRDYC | LL_RCC_CIR_CSSC);
  SET_BIT(RCC->CIR, vl_mask);

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

#if defined(RCC_CFGR3_USART1SW_PCLK1)
      case LL_RCC_USART1_CLKSOURCE_PCLK1:  /* USART1 Clock is PCLK1 */
      default:
        usart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
#else
      case LL_RCC_USART1_CLKSOURCE_PCLK2:  /* USART1 Clock is PCLK2 */
      default:
        usart_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
#endif /* RCC_CFGR3_USART1SW_PCLK1 */
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

#if defined(UART4) || defined(UART5)
/**
  * @brief  Return UARTx clock frequency
  * @param  UARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_UART4_CLKSOURCE
  *         @arg @ref LL_RCC_UART5_CLKSOURCE
  * @retval UART clock frequency (in Hz)
  *         @arg @ref LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetUARTClockFreq(uint32_t UARTxSource)
{
  uint32_t uart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_UART_CLKSOURCE(UARTxSource));

#if defined(UART4)
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
#endif /* UART4 */

#if defined(UART5)
  if (UARTxSource == LL_RCC_UART5_CLKSOURCE)
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
#endif /* UART5 */

  return uart_frequency;
}
#endif /* UART4 || UART5 */

/**
  * @brief  Return I2Cx clock frequency
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  *         @arg @ref LL_RCC_I2C2_CLKSOURCE (*)
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices
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

#if defined(RCC_CFGR3_I2C2SW)
  /* I2C2 CLK clock frequency */
  if (I2CxSource == LL_RCC_I2C2_CLKSOURCE)
  {
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C2_CLKSOURCE_SYSCLK: /* I2C2 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;
	
      case LL_RCC_I2C2_CLKSOURCE_HSI:    /* I2C2 Clock is HSI Osc. */
      default:
        if (LL_RCC_HSI_IsReady())
        {
          i2c_frequency = HSI_VALUE;
        }
        break;
    }
  }
#endif /*RCC_CFGR3_I2C2SW*/

#if defined(RCC_CFGR3_I2C3SW)
  /* I2C3 CLK clock frequency */
  if (I2CxSource == LL_RCC_I2C3_CLKSOURCE)
  {
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C3_CLKSOURCE_SYSCLK: /* I2C3 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_I2C3_CLKSOURCE_HSI:    /* I2C3 Clock is HSI Osc. */
      default:
        if (LL_RCC_HSI_IsReady())
        {
          i2c_frequency = HSI_VALUE;
        }
        break;
    }
  }
#endif /*RCC_CFGR3_I2C3SW*/

  return i2c_frequency;
}

#if  defined(RCC_CFGR_I2SSRC)
/**
  * @brief  Return I2Sx clock frequency
  * @param  I2SxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S_CLKSOURCE
  * @retval I2S clock frequency (in Hz)
  *         @arg @ref LL_RCC_PERIPH_FREQUENCY_NA indicates that external clock is used */
uint32_t LL_RCC_GetI2SClockFreq(uint32_t I2SxSource)
{
  uint32_t i2s_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_I2S_CLKSOURCE(I2SxSource));

  /* I2S1CLK clock frequency */
  switch (LL_RCC_GetI2SClockSource(I2SxSource))
  {
    case LL_RCC_I2S_CLKSOURCE_SYSCLK: /*!< System clock selected as I2S clock source */
      i2s_frequency = RCC_GetSystemClockFreq();
      break;

    case LL_RCC_I2S_CLKSOURCE_PIN:    /*!< External clock selected as I2S clock source */
    default:
      i2s_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }

  return i2s_frequency;
}
#endif /* RCC_CFGR_I2SSRC */
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

    case LL_RCC_USB_CLKSOURCE_PLL_DIV_1_5:        /* PLL clock used as USB clock source */
    default:
      if (LL_RCC_PLL_IsReady())
      {
        usb_frequency = (RCC_PLL_GetFreqDomain_SYS() * 3U) / 2U;
      }
      break;
  }

  return usb_frequency;
}
#endif /* USB */

#if defined(RCC_CFGR_ADCPRE) || defined(RCC_CFGR2_ADC1PRES) || defined(RCC_CFGR2_ADCPRE12) || defined(RCC_CFGR2_ADCPRE34)
/**
  * @brief  Return ADCx clock frequency
  * @param  ADCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE   (*)
  *         @arg @ref LL_RCC_ADC1_CLKSOURCE  (*)
  *         @arg @ref LL_RCC_ADC12_CLKSOURCE (*)
  *         @arg @ref LL_RCC_ADC34_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices
  * @retval ADC clock frequency (in Hz)
  */
uint32_t LL_RCC_GetADCClockFreq(uint32_t ADCxSource)
{
  uint32_t adc_prescaler = 0U;
  uint32_t adc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_ADC_CLKSOURCE(ADCxSource));

  /* Get ADC prescaler */
  adc_prescaler = LL_RCC_GetADCClockSource(ADCxSource);

#if defined(RCC_CFGR_ADCPRE)
  /* ADC frequency = PCLK2 frequency / ADC prescaler (2, 4, 6 or 8) */
  adc_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()))
                  / (((adc_prescaler >> POSITION_VAL(ADCxSource)) + 1U) * 2U);
#else
  if ((adc_prescaler & 0x0000FFFFU) == ((uint32_t)0x00000000U))
  {
    /* ADC frequency = HCLK frequency */
    adc_frequency = RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq());
  }
  else
  {
    /* ADC frequency = PCLK2 frequency / ADC prescaler (from 1 to 256) */
    adc_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()))
                    / (aADCPrescTable[((adc_prescaler & 0x0000FFFFU) >> POSITION_VAL(ADCxSource)) & 0xFU]);
  }
#endif /* RCC_CFGR_ADCPRE */

  return adc_frequency;
}
#endif /*RCC_CFGR_ADCPRE || RCC_CFGR2_ADC1PRES || RCC_CFGR2_ADCPRE12 || RCC_CFGR2_ADCPRE34 */

#if defined(RCC_CFGR_SDPRE)
/**
  * @brief  Return SDADCx clock frequency
  * @param  SDADCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SDADC_CLKSOURCE
  * @retval SDADC clock frequency (in Hz)
  */
uint32_t LL_RCC_GetSDADCClockFreq(uint32_t SDADCxSource)
{
  uint32_t sdadc_prescaler = 0U;
  uint32_t sdadc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_SDADC_CLKSOURCE(SDADCxSource));

  /* Get SDADC prescaler */
  sdadc_prescaler = LL_RCC_GetSDADCClockSource(SDADCxSource);

  /* SDADC frequency = SYSTEM frequency / SDADC prescaler (from 2 to 48) */
  sdadc_frequency = RCC_GetSystemClockFreq()
                    / (aSDADCPrescTable[(sdadc_prescaler >> POSITION_VAL(SDADCxSource)) & 0xFU]);

  return sdadc_frequency;
}
#endif /*RCC_CFGR_SDPRE */

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

#if defined(RCC_CFGR3_TIMSW)
/**
  * @brief  Return TIMx clock frequency
  * @param  TIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE
  *         @arg @ref LL_RCC_TIM8_CLKSOURCE (*)
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE (*)
  *         @arg @ref LL_RCC_TIM16_CLKSOURCE (*)
  *         @arg @ref LL_RCC_TIM17_CLKSOURCE (*)
  *         @arg @ref LL_RCC_TIM20_CLKSOURCE (*)
  *         @arg @ref LL_RCC_TIM2_CLKSOURCE (*)
  *         @arg @ref LL_RCC_TIM34_CLKSOURCE (*)
  *
  *         (*) value not defined in all devices
  * @retval TIM clock frequency (in Hz)
  */
uint32_t LL_RCC_GetTIMClockFreq(uint32_t TIMxSource)
{
  uint32_t tim_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_TIM_CLKSOURCE(TIMxSource));

  if (TIMxSource == LL_RCC_TIM1_CLKSOURCE)
  {
    /* TIM1CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM1_CLKSOURCE) == LL_RCC_TIM1_CLKSOURCE_PCLK2)
    {
      /* PCLK2 used as TIM1 clock source */
      tim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM1_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM1 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }

#if defined(RCC_CFGR3_TIM8SW)
  if (TIMxSource == LL_RCC_TIM8_CLKSOURCE)
  {
    /* TIM8CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM8_CLKSOURCE) == LL_RCC_TIM8_CLKSOURCE_PCLK2)
    {
      /* PCLK2 used as TIM8 clock source */
      tim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM8_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM8 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM8SW*/

#if defined(RCC_CFGR3_TIM15SW)
  if (TIMxSource == LL_RCC_TIM15_CLKSOURCE)
  {
    /* TIM15CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM15_CLKSOURCE) == LL_RCC_TIM15_CLKSOURCE_PCLK2)
    {
      /* PCLK2 used as TIM15 clock source */
      tim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM15_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM15 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM15SW*/

#if defined(RCC_CFGR3_TIM16SW)
  if (TIMxSource == LL_RCC_TIM16_CLKSOURCE)
  {
    /* TIM16CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM16_CLKSOURCE) == LL_RCC_TIM16_CLKSOURCE_PCLK2)
    {
      /* PCLK2 used as TIM16 clock source */
      tim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM16_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM16 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM16SW*/

#if defined(RCC_CFGR3_TIM17SW)
  if (TIMxSource == LL_RCC_TIM17_CLKSOURCE)
  {
    /* TIM17CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM17_CLKSOURCE) == LL_RCC_TIM17_CLKSOURCE_PCLK2)
    {
      /* PCLK2 used as TIM17 clock source */
      tim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM17_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM17 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM17SW*/

#if defined(RCC_CFGR3_TIM20SW)
  if (TIMxSource == LL_RCC_TIM20_CLKSOURCE)
  {
    /* TIM20CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM20_CLKSOURCE) == LL_RCC_TIM20_CLKSOURCE_PCLK2)
    {
      /* PCLK2 used as TIM20 clock source */
      tim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM20_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM20 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM20SW*/

#if defined(RCC_CFGR3_TIM2SW)
  if (TIMxSource == LL_RCC_TIM2_CLKSOURCE)
  {
    /* TIM2CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM2_CLKSOURCE) == LL_RCC_TIM2_CLKSOURCE_PCLK1)
    {
      /* PCLK1 used as TIM2 clock source */
      tim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM2_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM2 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM2SW*/

#if defined(RCC_CFGR3_TIM34SW)
  if (TIMxSource == LL_RCC_TIM34_CLKSOURCE)
  {
    /* TIM3/4 CLK clock frequency */
    if (LL_RCC_GetTIMClockSource(LL_RCC_TIM34_CLKSOURCE) == LL_RCC_TIM34_CLKSOURCE_PCLK1)
    {
      /* PCLK1 used as TIM3/4 clock source */
      tim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
    }
    else /* LL_RCC_TIM34_CLKSOURCE_PLL */
    {
      /* PLL clock used as TIM3/4 clock source */
      tim_frequency = RCC_PLL_GetFreqDomain_SYS();
    }
  }
#endif /*RCC_CFGR3_TIM34SW*/

  return tim_frequency;
}
#endif /*RCC_CFGR3_TIMSW*/

#if defined(HRTIM1)
/**
  * @brief  Return HRTIMx clock frequency
  * @param  HRTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_HRTIM1_CLKSOURCE
  * @retval HRTIM clock frequency (in Hz)
  */
uint32_t LL_RCC_GetHRTIMClockFreq(uint32_t HRTIMxSource)
{
  uint32_t hrtim_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_HRTIM_CLKSOURCE(HRTIMxSource));

  /* HRTIM1CLK clock frequency */
  if (LL_RCC_GetHRTIMClockSource(LL_RCC_HRTIM1_CLKSOURCE) == LL_RCC_HRTIM1_CLKSOURCE_PCLK2)
  {
    /* PCLK2 used as HRTIM1 clock source */
    hrtim_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
  }
  else /* LL_RCC_HRTIM1_CLKSOURCE_PLL */
  {
    /* PLL clock used as HRTIM1 clock source */
    hrtim_frequency = RCC_PLL_GetFreqDomain_SYS();
  }

  return hrtim_frequency;
}
#endif /* HRTIM1 */

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
