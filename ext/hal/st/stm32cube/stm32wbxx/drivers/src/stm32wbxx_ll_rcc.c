/**
  ******************************************************************************
  * @file    stm32wbxx_ll_rcc.c
  * @author  MCD Application Team
  * @brief   RCC LL module driver.
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
#if defined(USE_FULL_LL_DRIVER)

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_ll_rcc.h"
#ifdef  USE_FULL_ASSERT
#include "stm32_assert.h"
#else
#define assert_param(expr) ((void)0U)
#endif
/** @addtogroup STM32WBxx_LL_Driver
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
#define IS_LL_RCC_USART_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_USART1_CLKSOURCE))

#define IS_LL_RCC_LPUART_CLKSOURCE(__VALUE__) (((__VALUE__) == LL_RCC_LPUART1_CLKSOURCE))

#define IS_LL_RCC_I2C_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_I2C1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_I2C3_CLKSOURCE))

#define IS_LL_RCC_LPTIM_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_LPTIM1_CLKSOURCE) \
                                            || ((__VALUE__) == LL_RCC_LPTIM2_CLKSOURCE))

#define IS_LL_RCC_SAI_CLKSOURCE(__VALUE__)    ((__VALUE__) == LL_RCC_SAI1_CLKSOURCE)

#define IS_LL_RCC_RNG_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_RNG_CLKSOURCE))

#define IS_LL_RCC_CLK48_CLKSOURCE(__VALUE__)  (((__VALUE__) == LL_RCC_CLK48_CLKSOURCE))

#define IS_LL_RCC_USB_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_USB_CLKSOURCE))

#define IS_LL_RCC_ADC_CLKSOURCE(__VALUE__)    (((__VALUE__) == LL_RCC_ADC_CLKSOURCE))

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCC_LL_Private_Functions RCC Private functions
  * @{
  */
uint32_t RCC_PLL_GetFreqDomain_SYS(void);
uint32_t RCC_PLL_GetFreqDomain_SAI(void);
uint32_t RCC_PLL_GetFreqDomain_ADC(void);
uint32_t RCC_PLL_GetFreqDomain_48M(void);

uint32_t RCC_PLLSAI1_GetFreqDomain_SAI(void);
uint32_t RCC_PLLSAI1_GetFreqDomain_48M(void);
uint32_t RCC_PLLSAI1_GetFreqDomain_ADC(void);

uint32_t RCC_GetSystemClockFreq(void);


uint32_t RCC_GetHCLK1ClockFreq(uint32_t SYSCLK_Frequency);
uint32_t RCC_GetHCLK2ClockFreq(uint32_t SYSCLK_Frequency);
uint32_t RCC_GetHCLK4ClockFreq(uint32_t SYSCLK_Frequency);
uint32_t RCC_GetHCLK5ClockFreq(void);


uint32_t RCC_GetPCLK1ClockFreq(uint32_t HCLK_Frequency);
uint32_t RCC_GetPCLK2ClockFreq(uint32_t HCLK_Frequency);
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
  * @brief  Reset the RCC clock  to the default reset state.
  * @note   The default reset state of the clock configuration is given below:
  *         - MSI ON and used as system clock source
  *         - HSE, HSI, HSI48, PLL and PLLSAI1 Source OFF
  *         - CPU1, CPU2, AHB4, APB1 and APB2 prescaler set to 1.
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
  uint32_t vl_mask;

  /* Set MSION bit */
  LL_RCC_MSI_Enable();

  /* Insure MSIRDY bit is set before writing default MSIRANGE value */
  while (LL_RCC_MSI_IsReady() == 0U)
  {}

  /* Set MSIRANGE default value */
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_6);

  /* Set MSITRIM bits to the reset value*/
  LL_RCC_MSI_SetCalibTrimming(0);

  /* Set HSITRIM bits to the reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x40U);

  /* Reset CFGR register */
  LL_RCC_WriteReg(CFGR, 0x00070000U); /* MSI selected as System Clock and all prescaler to not divided */

  /* Reset CR register */
  vl_mask = 0xFFFFFFFFU;
  /* Reset HSION, HSIKERON, HSIASFS, HSEON, PLLSYSON bits */
  CLEAR_BIT(vl_mask, (RCC_CR_HSION | RCC_CR_HSIASFS | RCC_CR_HSIKERON  | RCC_CR_HSEON | RCC_CR_HSEBYP | RCC_CR_HSEPRE | RCC_CR_PLLON | RCC_CR_PLLSAI1ON));

  /* Write new mask in CR register */
  LL_RCC_WriteReg(CR, vl_mask);

  /* Wait for PLL READY bit to be reset */
  while(LL_RCC_PLL_IsReady() != 0U)
  {}

  /* Reset PLLCFGR register */
  LL_RCC_WriteReg(PLLCFGR, 0x22041000U);

  /* Wait for PLLSAI READY bit to be reset */
  while(LL_RCC_PLLSAI1_IsReady() != 0U)
  {}

  /* Reset PLLSAI1CFGR register */
  LL_RCC_WriteReg(PLLSAI1CFGR, 0x22041000U);

  /* Disable all interrupts */
  LL_RCC_WriteReg(CIER, 0x00000000U);

  /* Clear all interrupt flags */
  vl_mask = RCC_CICR_LSI1RDYC | RCC_CICR_LSERDYC | RCC_CICR_MSIRDYC | RCC_CICR_HSIRDYC | RCC_CICR_HSERDYC | RCC_CICR_PLLRDYC | RCC_CICR_PLLSAI1RDYC |\
            RCC_CICR_CSSC | RCC_CICR_HSI48RDYC | RCC_CICR_LSECSSC | RCC_CICR_LSI2RDYC;

  LL_RCC_WriteReg(CICR, vl_mask);

  /* Clear reset flags */
  LL_RCC_ClearResetFlags();

  /* SMPS reset */
  LL_RCC_WriteReg(SMPSCR, 0x00000301U); /* MSI default clock source */

  /* RF Wakeup Clock Source selection */
  LL_RCC_SetRFWKPClockSource(LL_RCC_RFWKP_CLKSOURCE_NONE);

  /* HSI48 reset */
  LL_RCC_HSI48_Disable();

  /* HSECR register write unlock & then reset*/
  LL_RCC_WriteReg(HSECR, HSE_CONTROL_UNLOCK_KEY);
  LL_RCC_WriteReg(HSECR, LL_RCC_HSE_CURRENTMAX_3); /* HSEGMC set to default value 011, current max limit 1.13 mA/V */

  /* EXTCFGR reset*/
  LL_RCC_WriteReg(EXTCFGR, 0x00030000U);

  return SUCCESS;
}

/**
  * @}
  */

/** @addtogroup RCC_LL_EF_Get_Freq
  * @brief  Return the frequencies of different on chip clocks;  System, AHB, APB1 and APB2 buses clocks
  *         and different peripheral clocks available on the device.
  * @note   If SYSCLK source is MSI, function returns values based on MSI values(*)
  * @note   If SYSCLK source is HSI, function returns values based on HSI_VALUE(**)
  * @note   If SYSCLK source is HSE, function returns values based on HSE_VALUE(***)
  * @note   If SYSCLK source is PLL, function returns values based on HSE_VALUE(***)
  *         or HSI_VALUE(**) or MSI values(*) multiplied/divided by the PLL factors.
  * @note   (*)  MSI values are retrieved thanks to __LL_RCC_CALC_MSI_FREQ macro
  * @note   (**) HSI_VALUE is a constant defined in this file (default value
  *              16 MHz) but the real value may vary depending on the variations
  *              in voltage and temperature.
  * @note   (***) HSE_VALUE is a constant defined in this file (default value
  *               32 MHz), user has to ensure that HSE_VALUE is same as the real
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

  /* HCLK1 clock frequency */
  RCC_Clocks->HCLK1_Frequency   = RCC_GetHCLK1ClockFreq(RCC_Clocks->SYSCLK_Frequency);

  /* HCLK2 clock frequency */
  RCC_Clocks->HCLK2_Frequency   = RCC_GetHCLK2ClockFreq(RCC_Clocks->SYSCLK_Frequency);

  /* HCLK4 clock frequency */
  RCC_Clocks->HCLK4_Frequency   = RCC_GetHCLK4ClockFreq(RCC_Clocks->SYSCLK_Frequency);

  /* HCLK5 clock frequency */
  RCC_Clocks->HCLK5_Frequency   = RCC_GetHCLK5ClockFreq();

  /* PCLK1 clock frequency */
  RCC_Clocks->PCLK1_Frequency  = RCC_GetPCLK1ClockFreq(RCC_Clocks->HCLK1_Frequency);

  /* PCLK2 clock frequency */
  RCC_Clocks->PCLK2_Frequency  = RCC_GetPCLK2ClockFreq(RCC_Clocks->HCLK1_Frequency);
}

/**
  * @brief  Return SMPS clock frequency
  * @note   This function is only applicable when CPU runs,
  *         When waking up from Standby mode and powering on the VCODE supply, the HSI is
  *         selected as SMPS Step Down converter clock, independent from the selection in
  *         SMPSSEL.
  * @retval SMPS clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetSMPSClockFreq(void)
{
  uint32_t smps_frequency;
  uint32_t smps_prescaler_index = ((LL_RCC_GetSMPSPrescaler()) >> RCC_SMPSCR_SMPSDIV_Pos);
  uint32_t smpsClockSource = LL_RCC_GetSMPSClockSource();

  if(smpsClockSource == LL_RCC_SMPS_CLKSOURCE_STATUS_HSI)/* SMPS Clock source is HSI Osc. */
  {
    if (LL_RCC_HSI_IsReady() == 1U)
    {
      smps_frequency = HSI_VALUE / SmpsPrescalerTable[smps_prescaler_index][0];
    }
    else
    {
      smps_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
    }
  }
  else if( smpsClockSource == LL_RCC_SMPS_CLKSOURCE_STATUS_HSE) /* SMPS Clock source is HSE Osc. */
  {
    if (LL_RCC_HSE_IsReady() == 1U)
    {
      smps_frequency = HSE_VALUE / SmpsPrescalerTable[smps_prescaler_index][5];
    }
    else
    {
      smps_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
    }
  }
  else if( smpsClockSource == LL_RCC_SMPS_CLKSOURCE_STATUS_MSI) /* SMPS Clock source is MSI Osc. */
  {
    uint32_t msiRange = LL_RCC_MSI_GetRange();

    if(msiRange == LL_RCC_MSIRANGE_8)
    {
      smps_frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_8) / SmpsPrescalerTable[smps_prescaler_index][4];
    }
    else if (msiRange == LL_RCC_MSIRANGE_9)
    {
      smps_frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_9) / SmpsPrescalerTable[smps_prescaler_index][3];
    }
    else if (msiRange == LL_RCC_MSIRANGE_10)
    {
      smps_frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_10) / SmpsPrescalerTable[smps_prescaler_index][2];
    }
    else if (msiRange == LL_RCC_MSIRANGE_11)
    {
      smps_frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_11) / SmpsPrescalerTable[smps_prescaler_index][1];
    }
    else
    {
      smps_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
    }
  }
  else /* SMPS has no Clock */
  {
    smps_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  }

  if (smps_frequency != LL_RCC_PERIPH_FREQUENCY_NO)
  {
    /* Systematic div by 2 */
    smps_frequency = smps_frequency >> 1U;
  }

  return smps_frequency;
}


/**
  * @brief  Return USARTx clock frequency
  * @param  USARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_USART1_CLKSOURCE
  * @retval USART clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
  */
uint32_t LL_RCC_GetUSARTClockFreq(uint32_t USARTxSource)
{
  uint32_t usart_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_USART_CLKSOURCE(USARTxSource));

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

    case LL_RCC_USART1_CLKSOURCE_PCLK2:  /* USART1 Clock is PCLK2 */
    default:
      usart_frequency = RCC_GetPCLK2ClockFreq(RCC_GetHCLK1ClockFreq(RCC_GetSystemClockFreq()));
      break;
  }
  return usart_frequency;
}

/**
  * @brief  Return I2Cx clock frequency
  * @param  I2CxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_I2C1_CLKSOURCE
  *         @arg @ref LL_RCC_I2C3_CLKSOURCE
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
        i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLK1ClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else
  {
    /* I2C3 CLK clock frequency */
    switch (LL_RCC_GetI2CClockSource(I2CxSource))
    {
      case LL_RCC_I2C3_CLKSOURCE_SYSCLK: /* I2C3 Clock is System Clock */
        i2c_frequency = RCC_GetSystemClockFreq();
        break;

      case LL_RCC_I2C3_CLKSOURCE_HSI:    /* I2C3 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          i2c_frequency = HSI_VALUE;
        }
        break;

      case LL_RCC_I2C3_CLKSOURCE_PCLK1:  /* I2C3 Clock is PCLK1 */
      default:
        i2c_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLK1ClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }

  return i2c_frequency;
}


/**
  * @brief  Return LPUARTx clock frequency
  * @param  LPUARTxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_LPUART1_CLKSOURCE
  * @retval LPUART clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (HSI or LSE) is not ready
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
      lpuart_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLK1ClockFreq(RCC_GetSystemClockFreq()));
      break;
  }

  return lpuart_frequency;
}

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
  uint32_t temp = LL_RCC_LSI2_IsReady();

  /* Check parameter */
  assert_param(IS_LL_RCC_LPTIM_CLKSOURCE(LPTIMxSource));

  if (LPTIMxSource == LL_RCC_LPTIM1_CLKSOURCE)
  {
    /* LPTIM1CLK clock frequency */
    switch (LL_RCC_GetLPTIMClockSource(LPTIMxSource))
    {
      case LL_RCC_LPTIM1_CLKSOURCE_LSI:    /* LPTIM1 Clock is LSI Osc. */
        if ((LL_RCC_LSI1_IsReady() == 1UL) || (temp == 1UL))
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
        lptim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLK1ClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }
  else
  {
    /* LPTIM2CLK clock frequency */
    switch (LL_RCC_GetLPTIMClockSource(LPTIMxSource))
    {
      case LL_RCC_LPTIM2_CLKSOURCE_LSI:    /* LPTIM2 Clock is LSI Osc. */
        if ((LL_RCC_LSI1_IsReady() == 1UL) || (temp == 1UL))
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
        lptim_frequency = RCC_GetPCLK1ClockFreq(RCC_GetHCLK1ClockFreq(RCC_GetSystemClockFreq()));
        break;
    }
  }

  return lptim_frequency;
}

/**
  * @brief  Return SAIx clock frequency
  * @param  SAIxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_SAI1_CLKSOURCE
  *
  * @retval SAI clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that PLL is not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that external clock is used
  */
uint32_t LL_RCC_GetSAIClockFreq(uint32_t SAIxSource)
{
  uint32_t sai_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_SAI_CLKSOURCE(SAIxSource));

  switch (LL_RCC_GetSAIClockSource(SAIxSource))
  {
    case LL_RCC_SAI1_CLKSOURCE_HSI:        /* HSI clock used as SAI1 clock source */
      if (LL_RCC_HSI_IsReady() == 1U)
      {
        sai_frequency = HSI_VALUE;
      }
      break;

    case LL_RCC_SAI1_CLKSOURCE_PLLSAI1:    /* PLLSAI1 clock used as SAI1 clock source */
      if (LL_RCC_PLLSAI1_IsReady() == 1U)
      {
        sai_frequency = RCC_PLLSAI1_GetFreqDomain_SAI();
      }
      break;
    case LL_RCC_SAI1_CLKSOURCE_PLL:        /* PLL clock used as SAI1 clock source */
      if (LL_RCC_PLL_IsReady() == 1U)
      {
        sai_frequency = RCC_PLL_GetFreqDomain_SAI();
      }
      break;

    case LL_RCC_SAI1_CLKSOURCE_PIN:        /* External input clock used as SAI1 clock source */
    default:
      sai_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }
  return sai_frequency;
}

/**
  * @brief  Return CLK48x clock frequency
  * @param  CLK48xSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CLK48_CLKSOURCE
  * @retval USB clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (MSI or HSI48) or PLLs (PLL or PLLSAI1) is not ready
  */
uint32_t LL_RCC_GetCLK48ClockFreq(uint32_t CLK48xSource)
{
  uint32_t clk48_frequency = LL_RCC_PERIPH_FREQUENCY_NO;

  /* Check parameter */
  assert_param(IS_LL_RCC_CLK48_CLKSOURCE(CLK48xSource));

  /* CLK48CLK clock frequency */
  switch (LL_RCC_GetUSBClockSource(CLK48xSource))
  {
    case LL_RCC_CLK48_CLKSOURCE_PLLSAI1:       /* PLLSAI1 clock used as CLK48 clock source */
      if (LL_RCC_PLLSAI1_IsReady() == 1U)
      {
        clk48_frequency = RCC_PLLSAI1_GetFreqDomain_48M();
      }
      break;

    case LL_RCC_CLK48_CLKSOURCE_PLL:           /* PLL clock used as CLK48 clock source */
      if (LL_RCC_PLL_IsReady() == 1U)
      {
        clk48_frequency = RCC_PLL_GetFreqDomain_48M();
      }
      break;

    case LL_RCC_CLK48_CLKSOURCE_MSI:           /* MSI clock used as CLK48 clock source */
      if (LL_RCC_MSI_IsReady() == 1U)
      {
        clk48_frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      }
      break;

    case LL_RCC_CLK48_CLKSOURCE_HSI48:       /* HSI48 clock used as CLK48 clock source */
    default:
      if (LL_RCC_HSI48_IsReady() == 1U)
      {
        clk48_frequency = HSI48_VALUE;
      }
      break;
  }

  return clk48_frequency;
}

/**
  * @brief  Return USBx clock frequency
  * @param  USBxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_CLK48_CLKSOURCE
  * @retval USB clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (MSI or HSI48) or PLLs (PLL or PLLSAI1) is not ready
  */
uint32_t LL_RCC_GetUSBClockFreq(uint32_t USBxSource)
{
  return LL_RCC_GetCLK48ClockFreq(USBxSource);
}

/**
  * @brief  Return RNGx clock frequency
  * @param  RNGxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_RNG_CLKSOURCE
  * @retval RNG clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (MSI or HSI48) or PLLs (PLL or PLLSAI1) is not ready
  */
uint32_t LL_RCC_GetRNGClockFreq(uint32_t RNGxSource)
{
  uint32_t rng_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  uint32_t rngClockSource = LL_RCC_GetRNGClockSource(RNGxSource);

  /* Check parameter */
  assert_param(IS_LL_RCC_RNG_CLKSOURCE(RNGxSource));

  /* RNGCLK clock frequency */
  if(rngClockSource == LL_RCC_RNG_CLKSOURCE_LSI) /* LSI clock used as RNG clock source */
  {
    const uint32_t temp_lsi1Status = LL_RCC_LSI1_IsReady();
    const uint32_t temp_lsi2Status = LL_RCC_LSI2_IsReady();
    if ((temp_lsi1Status == 1U) || (temp_lsi2Status == 1U))
    {
      rng_frequency = LSI_VALUE;
    }
  }
  else if(rngClockSource == LL_RCC_RNG_CLKSOURCE_LSE) /* LSE clock used as RNG clock source */
  {
    if (LL_RCC_LSE_IsReady() == 1U)
    {
      rng_frequency = LSE_VALUE;
    }
  }
  else /* CLK48 clock used as RNG clock source */
  {
    /* Systematic Div by 3 */
    rng_frequency =  LL_RCC_GetCLK48ClockFreq(LL_RCC_CLK48_CLKSOURCE)/ 3U;
  }
  return rng_frequency;
}

/**
  * @brief  Return ADCx clock frequency
  * @param  ADCxSource This parameter can be one of the following values:
  *         @arg @ref LL_RCC_ADC_CLKSOURCE
  * @retval ADC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillator (MSI) or PLL is not ready
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
    case LL_RCC_ADC_CLKSOURCE_PLLSAI1:       /* PLLSAI1 clock used as ADC clock source */
      if (LL_RCC_PLLSAI1_IsReady() == 1U)
      {
        adc_frequency = RCC_PLLSAI1_GetFreqDomain_ADC();
      }
      break;

    case LL_RCC_ADC_CLKSOURCE_SYSCLK:        /* SYSCLK clock used as ADC clock source */
      adc_frequency = RCC_GetSystemClockFreq();
      break;

    case LL_RCC_ADC_CLKSOURCE_PLL:           /* PLL clock used as USB clock source */
      if (LL_RCC_PLL_IsReady() == 1U)
      {
        adc_frequency = RCC_PLL_GetFreqDomain_ADC();
      }
      break;

    case LL_RCC_ADC_CLKSOURCE_NONE:          /* No clock used as ADC clock source */
    default:
      adc_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }

  return adc_frequency;
}

/**
  * @brief  Return RTC & LCD clock frequency
  * @retval RTC clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillators (LSI, LSE or HSE) are not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that no clock source selected
  */
uint32_t LL_RCC_GetRTCClockFreq(void)
{
  uint32_t rtc_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  uint32_t temp = LL_RCC_LSI2_IsReady();

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

      if ((LL_RCC_LSI1_IsReady() == 1UL) || (temp == 1UL))
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
  * @brief  Return RF Wakeup clock frequency
  * @retval RFWKP clock frequency (in Hz)
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NO indicates that oscillators (LSI, LSE or HSE) are not ready
  *         - @ref  LL_RCC_PERIPH_FREQUENCY_NA indicates that no clock source selected
  */
uint32_t LL_RCC_GetRFWKPClockFreq(void)
{
  uint32_t rfwkp_frequency = LL_RCC_PERIPH_FREQUENCY_NO;
  uint32_t temp = LL_RCC_LSI2_IsReady();

  /* RTCCLK clock frequency */
  switch (LL_RCC_GetRFWKPClockSource())
  {
    case LL_RCC_RFWKP_CLKSOURCE_LSE:              /* LSE clock used as RF Wakeup clock source */
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        rfwkp_frequency = LSE_VALUE;
      }
      break;

    case LL_RCC_RFWKP_CLKSOURCE_LSI:              /* LSI clock used as RF Wakeup clock source */
      if ((LL_RCC_LSI1_IsReady() == 1UL) || (temp == 1UL))
      {
        rfwkp_frequency = LSI_VALUE;
      }
      break;

    case LL_RCC_RFWKP_CLKSOURCE_HSE_DIV1024:       /* HSE clock used as RF Wakeup clock source */
      rfwkp_frequency = HSE_VALUE / 1024U;
      break;

    case LL_RCC_RFWKP_CLKSOURCE_NONE:              /* No clock used as RF Wakeup clock source */
    default:
      rfwkp_frequency = LL_RCC_PERIPH_FREQUENCY_NA;
      break;
  }

  return rfwkp_frequency;
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
  * @brief  Return SYSTEM clock (SYSCLK) frequency
  * @retval SYSTEM clock frequency (in Hz)
  */
uint32_t RCC_GetSystemClockFreq(void)
{
  uint32_t frequency;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (LL_RCC_GetSysClkSource())
  {
    case LL_RCC_SYS_CLKSOURCE_STATUS_MSI:  /* MSI used as system clock source */
      frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_HSI:  /* HSI used as system clock  source */
      frequency = HSI_VALUE;
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_HSE:  /* HSE used as system clock  source */
      if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
      {
        frequency = HSE_VALUE / 2U;
      }
      else
      {
        frequency = HSE_VALUE;
      }
      break;

    case LL_RCC_SYS_CLKSOURCE_STATUS_PLL:  /* PLL used as system clock  source */
      frequency = RCC_PLL_GetFreqDomain_SYS();
      break;

    default:
      frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }

  return frequency;
}

/**
  * @brief  Return HCLK1 clock frequency
  * @param  SYSCLK_Frequency SYSCLK clock frequency
  * @retval HCLK1 clock frequency (in Hz)
  */
uint32_t RCC_GetHCLK1ClockFreq(uint32_t SYSCLK_Frequency)
{
  /* HCLK clock frequency */
  return __LL_RCC_CALC_HCLK1_FREQ(SYSCLK_Frequency, LL_RCC_GetAHBPrescaler());
}

/**
  * @brief  Return HCLK2 clock frequency
  * @param  SYSCLK_Frequency SYSCLK clock frequency
  * @retval HCLK2 clock frequency (in Hz)
  */
uint32_t RCC_GetHCLK2ClockFreq(uint32_t SYSCLK_Frequency)
{
  /* HCLK clock frequency */
  return __LL_RCC_CALC_HCLK2_FREQ(SYSCLK_Frequency, LL_C2_RCC_GetAHBPrescaler());
}

/**
  * @brief  Return HCLK clock frequency
  * @param  SYSCLK_Frequency SYSCLK clock frequency
  * @retval HCLK4 clock frequency (in Hz)
  */
uint32_t RCC_GetHCLK4ClockFreq(uint32_t SYSCLK_Frequency)
{
  /* HCLK clock frequency */
  return __LL_RCC_CALC_HCLK4_FREQ(SYSCLK_Frequency, LL_RCC_GetAHB4Prescaler());
}

/**
  * @brief  Return HCLK5 clock frequency
  * @retval HCLK5 clock frequency (in Hz)
  */
uint32_t RCC_GetHCLK5ClockFreq(void)
{
  uint32_t frequency;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (LL_RCC_GetRFClockSource())
  {
    case LL_RCC_RF_CLKSOURCE_HSI:  /* HSI used as system clock  source */
      frequency = HSI_VALUE;
      break;

    case LL_RCC_RF_CLKSOURCE_HSE_DIV2:  /* HSE Div2 used as system clock source */
      frequency = HSE_VALUE / 2U;
      break;

    default:
      frequency = HSI_VALUE;
      break;
  }

  return frequency;

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
  * @brief  Return PLL clock (PLLRCLK) frequency used for system domain
  * @retval PLLRCLK clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_SYS(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE or MSI Value/ PLLM) * PLLN
     SYSCLK = PLL_VCO / PLLR
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLL clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
      if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
      {
        pllinputfreq = HSE_VALUE / 2U;
      }
      else
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLCLK_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                   LL_RCC_PLL_GetN(), LL_RCC_PLL_GetR());
}

/**
  * @brief  Return PLL clock (PLLPCLK) frequency used for SAI domain
  * @retval PLLPCLK clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_SAI(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE or MSI Value / PLLM) * PLLN
     SAI Domain clock = PLL_VCO / PLLP
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLL clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
      if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
      {
        pllinputfreq = HSE_VALUE / 2U;
      }
      else
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLCLK_SAI_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                       LL_RCC_PLL_GetN(), LL_RCC_PLL_GetP());
}

/**
  * @brief  Return PLL clock (PLLPCLK) frequency used for ADC domain
  * @retval PLLPCLK clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_ADC(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE or MSI Value / PLLM) * PLLN
     SAI Domain clock = PLL_VCO / PLLP
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLL clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
    if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
    {
      pllinputfreq = HSE_VALUE / 2U;
    }
    else
    {
      pllinputfreq = HSE_VALUE;
    }

      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLCLK_ADC_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                       LL_RCC_PLL_GetN(), LL_RCC_PLL_GetP());
}


/**
  * @brief  Return PLL clock (PLLQCLK) frequency used for 48 MHz domain
  * @retval PLLQCLK clock frequency (in Hz)
  */
uint32_t RCC_PLL_GetFreqDomain_48M(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE or MSI Value/ PLLM) * PLLN
     48M Domain clock = PLL_VCO / PLLQ
  */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLL clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
    if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
    {
      pllinputfreq = HSE_VALUE / 2U;
    }
    else
    {
      pllinputfreq = HSE_VALUE;
    }

      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLCLK_48M_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                       LL_RCC_PLL_GetN(), LL_RCC_PLL_GetQ());
}

/**
  * @brief  Return PLLSAI1 clock (PLLSAI1PCLK) frequency used for SAI domain
  * @retval PLLSAI1PCLK clock frequency (in Hz)
  */
uint32_t RCC_PLLSAI1_GetFreqDomain_SAI(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLLSAI1_VCO = (HSE_VALUE or HSI_VALUE or MSI Value/ PLLM) * PLLSAI1N */
  /* SAI Domain clock  = PLLSAI1_VCO / PLLSAI1P */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLLSAI1 clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLSAI1 clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLSAI1 clock source */
            if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
            {
              pllinputfreq = HSE_VALUE / 2U;
            }
            else
            {
              pllinputfreq = HSE_VALUE;
            }

      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLSAI1_SAI_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLSAI1_GetN(), LL_RCC_PLLSAI1_GetP());
}

/**
  * @brief  Return PLLSAI1 clock (PLLSAI1QCLK) frequency used for 48Mhz domain
  * @retval PLLSAI1QCLK clock frequency (in Hz)
  */
uint32_t RCC_PLLSAI1_GetFreqDomain_48M(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLLSAI1_VCO = (HSE_VALUE or HSI_VALUE or MSI Value/ PLLM) * PLLSAI1N */
  /* 48M Domain clock  = PLLSAI1_VCO / PLLSAI1Q */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLLSAI1 clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLSAI1 clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLSAI1 clock source */
     if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
     {
       pllinputfreq = HSE_VALUE / 2U;
     }
     else
     {
       pllinputfreq = HSE_VALUE;
     }
      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLSAI1_48M_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLSAI1_GetN(), LL_RCC_PLLSAI1_GetQ());
}

/**
  * @brief  Return PLLSAI1 clock (PLLSAI1RCLK) frequency used for ADC domain
  * @retval PLLSAI1RCLK clock frequency (in Hz)
  */
uint32_t RCC_PLLSAI1_GetFreqDomain_ADC(void)
{
  uint32_t pllinputfreq, pllsource;

  /* PLLSAI1_VCO = (HSE_VALUE or HSI_VALUE or MSI Value/ PLLM) * PLLSAI1N */
  /* 48M Domain clock  = PLLSAI1_VCO / PLLSAI1R */
  pllsource = LL_RCC_PLL_GetMainSource();

  switch (pllsource)
  {
    case LL_RCC_PLLSOURCE_MSI:  /* MSI used as PLLSAI1 clock source */
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;

    case LL_RCC_PLLSOURCE_HSI:  /* HSI used as PLLSAI1 clock source */
      pllinputfreq = HSI_VALUE;
      break;

    case LL_RCC_PLLSOURCE_HSE:  /* HSE used as PLLSAI1 clock source */
      if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
      {
        pllinputfreq = HSE_VALUE / 2U;
      }
      else
      {
        pllinputfreq = HSE_VALUE;
      }
      break;

    default:
      pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
      break;
  }
  return __LL_RCC_CALC_PLLSAI1_ADC_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(),
                                        LL_RCC_PLLSAI1_GetN(), LL_RCC_PLLSAI1_GetR());
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
