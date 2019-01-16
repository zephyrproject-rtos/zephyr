/**
  ******************************************************************************
  * @file    stm32g0xx_ll_rcc.c
  * @author  MCD Application Team
  * @brief   RCC LL module driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_ll_rcc.h"
#ifdef  USE_FULL_ASSERT
#include "stm32_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif
/** @addtogroup STM32G0xx_LL_Driver
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
#if defined(STM32G081xx) || defined(STM32G071xx) || defined(STM32G070xx)
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_USART2_CLKSOURCE))
#endif

#if defined(LPUART1)
#define IS_LL_RCC_LPUART_CLKSOURCE(__VALUE__) (((__VALUE__) == LL_RCC_LPUART1_CLKSOURCE))
#endif

#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)     ((__VALUE__) == LL_RCC_I2C1_CLKSOURCE)

#if defined(LPTIM1) || defined(LPTIM2)
#define IS_LL_RCC_LPTIM_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_LPTIM1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_LPTIM2_CLKSOURCE))
#endif

#if defined(RNG)
#define IS_LL_RCC_RNG_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_RNG_CLKSOURCE))
#endif

#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_ADC_CLKSOURCE))


#define IS_LL_RCC_I2S_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2S1_CLKSOURCE))

#if defined(CEC)
#define IS_LL_RCC_CEC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_CEC_CLKSOURCE))
#endif

#if defined(RCC_CCIPR_TIM1SEL) && defined(RCC_CCIPR_TIM15SEL)
#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_TIM15_CLKSOURCE))
#elif defined(RCC_CCIPR_TIM1SEL)
#define IS_LL_RCC_TIM_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_TIM1_CLKSOURCE))
#endif



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
uint32_t RCC_PLL_GetFreqDomain_ADC(void);
uint32_t RCC_PLL_GetFreqDomain_I2S1(void);
uint32_t RCC_PLL_GetFreqDomain_RNG(void);
uint32_t RCC_PLL_GetFreqDomain_TIM1(void);
uint32_t RCC_PLL_GetFreqDomain_TIM15(void);
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
  * @note   This function does not modify the configuration of the
  *         - Peripheral clocks
  *         - LSI, LSE and RTC clocks
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RCC registers are de-initialized
  *          - ERROR: not applicable
  */
ErrorStatus LL_RCC_DeInit(void)
{
  /* Set HSION bit and wait for HSI READY bit */
  LL_RCC_HSI_Enable();
  while (LL_RCC_HSI_IsReady() != 1U)
  {}

  /* Set HSITRIM bits to reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x40U);

  /* Reset CFGR register */
  LL_RCC_WriteReg(CFGR, 0x00000000U);

  /* Reset whole CR register but HSI in 2 steps in case HSEBYP is set */
  LL_RCC_WriteReg(CR, RCC_CR_HSION);
  while (LL_RCC_HSE_IsReady() != 0U)
  {}
  LL_RCC_WriteReg(CR, RCC_CR_HSION);

  /* Wait for PLL READY bit to be reset */
  while (LL_RCC_PLL_IsReady() != 0U)
  {}

  /* Reset PLLCFGR register */
  LL_RCC_WriteReg(PLLCFGR, 16U << RCC_PLLCFGR_PLLN_Pos);

  /* Disable all interrupts */
  LL_RCC_WriteReg(CIER, 0x00000000U);

  /* Clear all interrupts flags */
  LL_RCC_WriteReg(CICR, 0xFFFFFFFFU);

  return SUCCESS;
}

/**
  * @}
  */

/** @addtogroup RCC_LL_EF_Get_Freq
  * @brief  Return the frequencies of different on chip clocks;  System, AHB and APB1 buses clocks
  *         and different peripheral clocks available on the device.
  * @note   If SYSCLK source is HSI, function returns values based on HSI_VALUE divided by HSI division factor(**)
  * @note   If SYSCLK source is HSE, function returns values based on HSE_VALUE(***)
  * @note   If SYSCLK source is PLL, function returns values based on HSE_VALUE(***)
  *         or HSI_VALUE(**) multiplied/divided by the PLL factors.
  * @note   (**) HSI_VALUE is a constant defined in this file (default value
  *              16 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  * @note   (***) HSE_VALUE is a constant defined in this file (default value
  *               8 MHz), user has to ensure that HSE_VALUE is same as the real
  *               frequency of the crystal used. Otherwise, this function may
  *               have wrong result.
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
  *         @arg @ref LL_RCC_USART2_CLKSOURCE
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
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          usart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_USART1_CLKSOURCE_LSE:    /* USART1 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady() == 1U)
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
#if defined(RCC_CCIPR_USART2SEL)
  else if (USARTxSource == LL_RCC_USART2_CLKSOURCE)
  {
    /* USART2CLK clock frequency */
    switch (LL_RCC_GetUSARTClockSource(USARTxSource))
    {
      case LL_RCC_USART2_CLKSOURCE_SYSCLK: /* USART2 Clock is System Clock */
        usart_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_USART2_CLKSOURCE_HSI:    /* USART2 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          usart_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_USART2_CLKSOURCE_LSE:    /* USART2 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady() == 1U)
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
#endif
  else
  {
  }
  return usart_frequency;
}

/**
  * @brief  Return I2Cx clock frequency
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
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
        if (LL_RCC_HSI_IsReady() == 1U)
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
  else
  {
  }

  return i2c_frequency;
}

/**
  * @brief  Return I2Sx clock frequency
  * @param  I2SxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2S1_CLKSOURCE
  * @retval I2S clock frequency (in Hz)
  *         @arg @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator is not ready
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
      case LL_RCC_I2S1_CLKSOURCE_HSI:    /* I2S1 Clock is HSI */
        i2s_frequency = HSI_VALUE;
        break;

      case LL_RCC_I2S1_CLKSOURCE_PLL:    /* I2S1 Clock is PLL"P" */
        if (LL_RCC_PLL_IsReady() == 1U)
        {
          i2s_frequency = RCC_PLL_GetFreqDomain_I2S1();
        }
        break;


      case LL_RCC_I2S1_CLKSOURCE_PIN:          /* I2S1 Clock is External clock */
        i2s_frequency = EXTERNAL_CLOCK_VALUE;
        break;

      case LL_RCC_I2S1_CLKSOURCE_SYSCLK: /* I2S1 Clock is System Clock */
      default:
        i2s_frequency = RCC_GetSystemClockFreq();
        break;
    }
  }

  return i2s_frequency;
}

#if defined(LPUART1)
/**
  * @brief  Return LPUARTx clock frequency
  * @param  LPUARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE
  * @retval LPUART clock frequency (in Hz)
  *         @arg @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetLPUARTClockFreq(uint32_t LPUARTxSource)
{
  uint32_t lpuart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_LPUART_CLKSOURCE(LPUARTxSource));

  /* LPUART1CLK clock frequency */
  switch (LL_RCC_GetLPUARTClockSource(LPUARTxSource))
  {
    case LL_RCC_LPUART1_CLKSOURCE_SYSCLK: /* LPUART1 Clock is System Clock */
      lpuart_frequency = RCC_GetSystemClockFreq();
      break;

    case LL_RCC_LPUART1_CLKSOURCE_HSI:    /* LPUART1 Clock is HSI Osc. */
      if (LL_RCC_HSI_IsReady() == 1U)
      {
        lpuart_frequency = HSI_VALUE;
      }
      break;

    case LL_RCC_LPUART1_CLKSOURCE_LSE:    /* LPUART1 Clock is LSE Osc. */
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        lpuart_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_LPUART1_CLKSOURCE_PCLK1:  /* LPUART1 Clock is PCLK1 */
    default:
      lpuart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
      break;
  }

  return lpuart_frequency;
}
#endif /* LPUART1 */

#if defined(LPTIM1) && defined(LPTIM2)
/**
  * @brief  Return LPTIMx clock frequency
  * @param  LPTIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPTIM1_CLKSOURCE
  *         @arg @ref LL_RCC_LPTIM2_CLKSOURCE
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
        if (LL_RCC_LSI_IsReady() == 1U)
        {
          lptim_frequency = LSI_VALUE;
        }
        break;

      case LL_RCC_LPTIM1_CLKSOURCE_HSI:    /* LPTIM1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          lptim_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_LPTIM1_CLKSOURCE_LSE:    /* LPTIM1 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady() == 1U)
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
  else
  {
    /* LPTIM2CLK clock frequency */
    switch (LL_RCC_GetLPTIMClockSource(LPTIMxSource))
    {
      case LL_RCC_LPTIM2_CLKSOURCE_LSI:    /* LPTIM2 Clock is LSI Osc. */
        if (LL_RCC_LSI_IsReady() == 1U)
        {
          lptim_frequency = LSI_VALUE;
        }
        break;

      case LL_RCC_LPTIM2_CLKSOURCE_HSI:    /* LPTIM2 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          lptim_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_LPTIM2_CLKSOURCE_LSE:    /* LPTIM2 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady() == 1U)
        {
          lptim_frequency = LSE_VALUE;
        }
        break;

      case LL_RCC_LPTIM2_CLKSOURCE_PCLK1:  /* LPTIM2 Clock is PCLK1 */
      default:
        lptim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }

  return lptim_frequency;
}
#endif /* LPTIM1 && LPTIM2 */

#if defined(RCC_CCIPR_TIM1SEL) || defined(RCC_CCIPR_TIM15SEL)
/**
  * @brief  Return TIMx clock frequency
  * @param  TIMxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_TIM1_CLKSOURCE
  * @if defined(STM32G081xx)
  *         @arg @ref LL_RCC_TIM15_CLKSOURCE
  * @endif
  * @retval TIMx clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetTIMClockFreq(uint32_t TIMxSource)
{
  uint32_t tim_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_TIM_CLKSOURCE(TIMxSource));

  if (TIMxSource == LL_RCC_TIM1_CLKSOURCE)
  {
    /* TIM1CLK clock frequency */
    switch (LL_RCC_GetTIMClockSource(TIMxSource))
    {
      case LL_RCC_TIM1_CLKSOURCE_PLL:    /* TIM1 Clock is PLLQ */
        if (LL_RCC_PLL_IsReady() == 1U)
        {
          tim_frequency = RCC_PLL_GetFreqDomain_TIM1();
        }
        break;

      case LL_RCC_TIM1_CLKSOURCE_PCLK1:  /* TIM1 Clock is PCLK1 */
      default:
        tim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
#if defined(TIM15)
  else
  {
    if (TIMxSource == LL_RCC_TIM15_CLKSOURCE)
    {
      /* TIM15CLK clock frequency */
      switch (LL_RCC_GetTIMClockSource(TIMxSource))
      {
        case LL_RCC_TIM15_CLKSOURCE_PLL:    /* TIM1 Clock is PLLQ */
          if (LL_RCC_PLL_IsReady() == 1U)
          {
            tim_frequency = RCC_PLL_GetFreqDomain_TIM15();
          }
          break;

        case LL_RCC_TIM15_CLKSOURCE_PCLK1:  /* TIM15 Clock is PCLK1 */
        default:
          tim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLKClockFreq(RCC_GetSystemClockFreq()));
          break;
      }
    }
  }
#endif
  return tim_frequency;
}
#endif /* RCC_CCIPR_TIM1SEL && RCC_CCIPR_TIM15SEL */


#if defined(RNG)
/**
  * @brief  Return RNGx clock frequency
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE
  * @retval RNG clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI) or PLL is not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that no clock source selected
  */
uint32_t LL_RCC_GetRNGClockFreq(uint32_t RNGxSource)
{
  uint32_t rng_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  uint32_t rngdiv;

  /* Check parameter */
  assert_param(IS_LL_RCC_RNG_CLKSOURCE(RNGxSource));

  /* RNGCLK clock frequency */
  switch (LL_RCC_GetRNGClockSource(RNGxSource))
  {
    case LL_RCC_RNG_CLKSOURCE_PLL:           /* PLL clock used as RNG clock source */
      if (LL_RCC_PLL_IsReady() == 1U)
      {
        rng_frequency = RCC_PLL_GetFreqDomain_RNG();
        rngdiv = (1UL << ((READ_BIT(RCC->CCIPR, RCC_CCIPR_RNGDIV)) >> RCC_CCIPR_RNGDIV_Pos));
        rng_frequency = (rng_frequency / rngdiv);
      }
      break;

    case LL_RCC_RNG_CLKSOURCE_HSI_DIV8:      /* HSI clock divided by 8 used as RNG clock source */
      rng_frequency = HSI_VALUE / 8U;
      rngdiv = (1UL << ((READ_BIT(RCC->CCIPR, RCC_CCIPR_RNGDIV)) >> RCC_CCIPR_RNGDIV_Pos));
      rng_frequency = (rng_frequency / rngdiv);
      break;
    case LL_RCC_RNG_CLKSOURCE_SYSCLK:        /* SYSCLK clock used as RNG clock source */
      rng_frequency = RCC_GetSystemClockFreq();
      rngdiv = (1UL << ((READ_BIT(RCC->CCIPR, RCC_CCIPR_RNGDIV)) >> RCC_CCIPR_RNGDIV_Pos));
      rng_frequency = (rng_frequency / rngdiv);
      break;

    case LL_RCC_RNG_CLKSOURCE_NONE:          /* No clock used as RNG clock source */
    default:
      rng_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;

  }

  return rng_frequency;
}
#endif /* RNG */

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
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        cec_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_CEC_CLKSOURCE_HSI_DIV488:    /* CEC Clock is HSI Osc. */
    default:
      if (LL_RCC_HSI_IsReady() == 1U)
      {
        cec_frequency = (HSI_VALUE / 488U);
      }
      break;
  }

  return cec_frequency;
}
#endif /* CEC */

/**
  * @brief  Return ADCx clock frequency
  * @param  ADCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE
  * @retval ADC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI) or PLL is not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that no clock source selected
  */
uint32_t LL_RCC_GetADCClockFreq(uint32_t ADCxSource)
{
  uint32_t adc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_ADC_CLKSOURCE(ADCxSource));

  /* ADCCLK clock frequency */
  switch (LL_RCC_GetADCClockSource(ADCxSource))
  {
    case LL_RCC_ADC_CLKSOURCE_SYSCLK:        /* SYSCLK clock used as ADC clock source */
      adc_frequency = RCC_GetSystemClockFreq();
      break;
    case LL_RCC_ADC_CLKSOURCE_HSI  :        /* HSI clock used as ADC clock source */
      adc_frequency = HSI_VALUE;
      break;

    case LL_RCC_ADC_CLKSOURCE_PLL:         /* PLLP clock used as ADC clock source */
      if (LL_RCC_PLL_IsReady() == 1U)
      {
        adc_frequency = RCC_PLL_GetFreqDomain_ADC();
      }
      break;
    default:
      adc_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }

  return adc_frequency;
}

/**
  * @brief  Return RTC clock frequency
  * @retval RTC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillators (LSI, LSE or HSE) are not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that no clock source selected
  */
uint32_t LL_RCC_GetRTCClockFreq(void)
{
  uint32_t rtc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* RTCCLK clock frequency */
  switch (LL_RCC_GetRTCClockSource())
  {
    case LL_RCC_RTC_CLKSOURCE_LSE:       /* LSE clock used as RTC clock source */
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        rtc_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_RTC_CLKSOURCE_LSI:       /* LSI clock used as RTC clock source */
      if (LL_RCC_LSI_IsReady() == 1U)
      {
        rtc_frequency = LSI_VALUE;
      }
      break;

    case LL_RCC_RTC_CLKSOURCE_HSE_DIV32:        /* HSE clock used as ADC clock source */
      rtc_frequency = HSE_VALUE / 32U;
      break;

    case LL_RCC_RTC_CLKSOURCE_NONE:          /* No clock used as RTC clock source */
    default:
      rtc_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }

  return rtc_frequency;
}

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
  uint32_t frequency;
  uint32_t hsidiv;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (LL_RCC_GetSysClkSource())
  {
    case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:  /* HSE used as system clock  source */
      frequency = HSE_VALUE;
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_PLL:  /* PLL used as system clock  source */
      frequency = RCC_PLL_GetFreqDomain_SYS();
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_HSI:  /* HSI used as system clock  source */
    default:
      hsidiv = (1UL << ((READ_BIT(RCC->CR, RCC_CR_HSIDIV)) >> RCC_CR_HSIDIV_Pos));
      frequency = (HSI_VALUE / hsidiv);
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
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
     SYSCLK = PLL_VCO / PLLR
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
                                   LL_RCC_PLL_GetN(), LL_RCC_PLL_GetR());
}
/**
  * @brief  Return PLL clock frequency used for ADC domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_ADC(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
     ADC Domain clock = PLL_VCO / PLLP
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
  return __LL_RCC_CALC_PLLCLK_ADC_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                       LL_RCC_PLL_GetN(), LL_RCC_PLL_GetP());
}

/**
  * @brief  Return PLL clock frequency used for I2S1 domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_I2S1(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM) * PLLN
     I2S1 Domain clock = PLL_VCO / PLLP
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
  return __LL_RCC_CALC_PLLCLK_I2S1_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLL_GetN(), LL_RCC_PLL_GetP());
}

#if defined(RNG)
/**
  * @brief  Return PLL clock frequency used for RNG domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_RNG(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM ) * PLLN

     RNG Domain clock = PLL_VCO / PLLQ
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
  return __LL_RCC_CALC_PLLCLK_RNG_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                       LL_RCC_PLL_GetN(), LL_RCC_PLL_GetQ());
}
#endif /* RNG */

#if defined(RCC_PLLQ_SUPPORT)
/**
  * @brief  Return PLL clock frequency used for TIM1 domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_TIM1(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM ) * PLLN

     TIM1 Domain clock = PLL_VCO / PLLQ
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
  return __LL_RCC_CALC_PLLCLK_TIM1_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLL_GetN(), LL_RCC_PLL_GetQ());
}
#endif /* RCC_PLLQ_SUPPORT */

#if defined(RCC_PLLQ_SUPPORT) && defined(TIM15)
/**
  * @brief  Return PLL clock frequency used for TIM15 domain
  * @retval PLL clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_TIM15(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLLM ) * PLLN

     TIM15 Domain clock = PLL_VCO / PLLQ
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
  return __LL_RCC_CALC_PLLCLK_TIM15_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                         LL_RCC_PLL_GetN(), LL_RCC_PLL_GetQ());
}
#endif /* RCC_PLLQ_SUPPORT && TIM15 */
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
