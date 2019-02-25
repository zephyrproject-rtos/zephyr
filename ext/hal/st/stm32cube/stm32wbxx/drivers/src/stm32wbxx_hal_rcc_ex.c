/**
  ******************************************************************************
  * @file    stm32wbxx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @brief   Extended RCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities RCC extended peripheral:
  *           + Extended Peripheral Control functions
  *           + Extended Clock management functions
  *           + Extended Clock Recovery System Control functions
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
#include "stm32wbxx_hal.h"

/** @addtogroup STM32WBxx_HAL_Driver
  * @{
  */

/** @defgroup RCCEx RCCEx
  * @brief RCC Extended HAL module driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/** @defgroup RCCEx_Private_Constants RCCEx Private Constants
 * @{
 */
#define PLLSAI1_TIMEOUT_VALUE    (2U)    /* 2 ms (minimum Tick + 1) */
#define PLL_TIMEOUT_VALUE        (2U)    /* 2 ms (minimum Tick + 1) */

#define CLOCKSMPS_TIMEOUT_VALUE  (5000U) /* 5 s    */

#define __LSCO1_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define LSCO1_GPIO_PORT        GPIOA
#define LSCO1_PIN              GPIO_PIN_2

#define __LSCO2_CLK_ENABLE()   __HAL_RCC_GPIOH_CLK_ENABLE()
#define LSCO2_GPIO_PORT        GPIOH
#define LSCO2_PIN              GPIO_PIN_3

#define __LSCO3_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()
#define LSCO3_GPIO_PORT        GPIOC
#define LSCO3_PIN              GPIO_PIN_12

#define LSI2_TIMEOUT_VALUE         (3U)    /* to be adjusted with DS    */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCCEx_Private_Functions RCCEx Private Functions
 * @{
 */
static HAL_StatusTypeDef RCCEx_PLLSAI1_ConfigNP(RCC_PLLSAI1InitTypeDef *PLLSAI1);
static HAL_StatusTypeDef RCCEx_PLLSAI1_ConfigNQ(RCC_PLLSAI1InitTypeDef *PLLSAI1);
static HAL_StatusTypeDef RCCEx_PLLSAI1_ConfigNR(RCC_PLLSAI1InitTypeDef *PLLSAI1);

static uint32_t          RCC_PLL_GetFreqDomain_P(void);
static uint32_t          RCC_PLL_GetFreqDomain_Q(void);

static uint32_t          RCC_PLLSAI1_GetFreqDomain_R(void);
static uint32_t          RCC_PLLSAI1_GetFreqDomain_P(void);
static uint32_t          RCC_PLLSAI1_GetFreqDomain_Q(void);


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
    (@) Important note: Care must be taken when @ref HAL_RCCEx_PeriphCLKConfig() is used to
        select the RTC clock source; in this case the Backup domain will be reset in
        order to modify the RTC Clock source, as consequence RTC registers (including
        the backup registers) and RCC_BDCR register are set to their reset values.

@endverbatim
  * @{
  */

/**
  * @brief  Initialize the RCC extended peripherals clocks according to the specified
  *         parameters in the @ref RCC_PeriphCLKInitTypeDef.
  * @param  PeriphClkInit  pointer to a @ref RCC_PeriphCLKInitTypeDef structure that
  *         contains a field PeriphClockSelection which can be a combination of the following values:
  *
  *            @arg @ref RCC_PERIPHCLK_USART1   USART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPUART1  LPUART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C1     I2C1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3     I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPTIM1   LPTIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPTIM2   LPTIM2 peripheral clock
  *
  *            @arg @ref RCC_PERIPHCLK_SAI1     SAI1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB      USB peripheral clock
  *            @arg @ref RCC_PERIPHCLK_RNG      RNG peripheral clock
  *
  *            @arg @ref RCC_PERIPHCLK_ADC      ADC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_RTC      RTC peripheral clock
  *
  *            @arg @ref RCC_PERIPHCLK_RFWAKEUP RFWKP peripheral clock
  *            @arg @ref RCC_PERIPHCLK_SMPS     SMPS peripheral clock
  *
  *
  * @note   Care must be taken when @ref HAL_RCCEx_PeriphCLKConfig() is used to select
  *         the RTC clock source: in this case the access to Backup domain is enabled.
  *
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t tickstart;
  HAL_StatusTypeDef ret     = HAL_OK;      /* Intermediate status */
  HAL_StatusTypeDef status  = HAL_OK;   /* Final status */

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClkInit->PeriphClockSelection));

  /*-------------------------- SAI1 clock source configuration ---------------------*/
  if ((((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SAI1) == RCC_PERIPHCLK_SAI1))
  {
    /* Check the parameters */
    assert_param(IS_RCC_SAI1CLK(PeriphClkInit->Sai1ClockSelection));

    switch (PeriphClkInit->Sai1ClockSelection)
    {
      case RCC_SAI1CLKSOURCE_PLL:      /* PLL is used as clock source for SAI1 */
        /* Enable SAI1 Clock output generated form System PLL . */
        __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL_SAI1CLK);

        /* SAI1 clock source config set later after clock selection check */
        break;

      case RCC_SAI1CLKSOURCE_PLLSAI1: /* PLLSAI1 is used as clock source for SAI1 */
        /* PLLSAI1 parameters N & P configuration and clock output (PLLSAI1ClockOut) */
        ret = RCCEx_PLLSAI1_ConfigNP(&(PeriphClkInit->PLLSAI1));
        /* SAI1 clock source config set later after clock selection check */
        break;


      case RCC_SAI1CLKSOURCE_PIN:      /* External clock is used as source of SAI1 clock*/
        /* SAI1 clock source config set later after clock selection check */
        break;

      case RCC_SAI1CLKSOURCE_HSI:

        break;

      default:
        ret = HAL_ERROR;
        break;
    }

    if (ret == HAL_OK)
    {
      /* Set the source of SAI1 clock*/
      __HAL_RCC_SAI1_CONFIG(PeriphClkInit->Sai1ClockSelection);
    }
    else
    {
      /* set overall return value */
      status = ret;
    }
  }

  /*-------------------------- RTC clock source configuration ----------------------*/
  if ((PeriphClkInit->PeriphClockSelection & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC)
  {
    uint32_t rtcclocksource = LL_RCC_GetRTCClockSource();

    /* Check for RTC Parameters used to output RTCCLK */
    assert_param(IS_RCC_RTCCLKSOURCE(PeriphClkInit->RTCClockSelection));

    /* Configure the clock source only if a different source is expected */
    if(rtcclocksource != PeriphClkInit->RTCClockSelection)
    {
      /* Enable write access to Backup domain */
      HAL_PWR_EnableBkUpAccess();

      /* If a clock source is not yet selected */
      if(rtcclocksource == RCC_RTCCLKSOURCE_NONE)
      {
        /* Directly set the configuration of the clock source selection */
        LL_RCC_SetRTCClockSource(PeriphClkInit->RTCClockSelection);
      }
      else /* A clock source is already selected */
      {
        /* Store the content of BDCR register before the reset of Backup Domain */
        uint32_t bdcr = LL_RCC_ReadReg(BDCR);

        /* RTC Clock selection can be changed only if the Backup Domain is reset */
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();

        /* Set the value of the clock source selection */
        MODIFY_REG(bdcr, RCC_BDCR_RTCSEL, PeriphClkInit->RTCClockSelection);

        /* Restore the content of BDCR register */
        LL_RCC_WriteReg(BDCR, bdcr);

        /* Wait for LSE reactivation if LSE was enable prior to Backup Domain reset */
        if (LL_RCC_LSE_IsEnabled() == 1U)
        {
          /* Get Start Tick*/
          tickstart = HAL_GetTick();

          /* Wait till LSE is ready */
          while (LL_RCC_LSE_IsReady() != 1U)
          {
            if ((HAL_GetTick() - tickstart) > RCC_LSE_TIMEOUT_VALUE)
            {
              ret = HAL_TIMEOUT;
              break;
            }
          }
        }
      }

      /* set overall return value */
      status = ret;
    }
    else
    {
      /* set overall return value */
      status = ret;
    }

  }

  /*-------------------------- USART1 clock source configuration -------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART1) == RCC_PERIPHCLK_USART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART1CLKSOURCE(PeriphClkInit->Usart1ClockSelection));

    /* Configure the USART1 clock source */
    __HAL_RCC_USART1_CONFIG(PeriphClkInit->Usart1ClockSelection);
  }


  /*-------------------------- LPUART1 clock source configuration ------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPUART1) == RCC_PERIPHCLK_LPUART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LPUART1CLKSOURCE(PeriphClkInit->Lpuart1ClockSelection));

    /* Configure the LPUAR1 clock source */
    __HAL_RCC_LPUART1_CONFIG(PeriphClkInit->Lpuart1ClockSelection);
  }

  /*-------------------------- LPTIM1 clock source configuration -------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM1) == (RCC_PERIPHCLK_LPTIM1))
  {
    assert_param(IS_RCC_LPTIM1CLK(PeriphClkInit->Lptim1ClockSelection));
    __HAL_RCC_LPTIM1_CONFIG(PeriphClkInit->Lptim1ClockSelection);
  }

  /*-------------------------- LPTIM2 clock source configuration -------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM2) == (RCC_PERIPHCLK_LPTIM2))
  {
    assert_param(IS_RCC_LPTIM2CLK(PeriphClkInit->Lptim2ClockSelection));
    __HAL_RCC_LPTIM2_CONFIG(PeriphClkInit->Lptim2ClockSelection);
  }

  /*-------------------------- I2C1 clock source configuration ---------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C1) == RCC_PERIPHCLK_I2C1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C1CLKSOURCE(PeriphClkInit->I2c1ClockSelection));

    /* Configure the I2C1 clock source */
    __HAL_RCC_I2C1_CONFIG(PeriphClkInit->I2c1ClockSelection);
  }

  /*-------------------------- I2C3 clock source configuration ---------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C3) == RCC_PERIPHCLK_I2C3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C3CLKSOURCE(PeriphClkInit->I2c3ClockSelection));

    /* Configure the I2C3 clock source */
    __HAL_RCC_I2C3_CONFIG(PeriphClkInit->I2c3ClockSelection);
  }

  /*-------------------------- USB clock source configuration ----------------------*/

  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USB) == (RCC_PERIPHCLK_USB))
  {
    assert_param(IS_RCC_USBCLKSOURCE(PeriphClkInit->UsbClockSelection));
    __HAL_RCC_USB_CONFIG(PeriphClkInit->UsbClockSelection);

    if (PeriphClkInit->UsbClockSelection == RCC_USBCLKSOURCE_PLL)
    {
      /* Enable PLLQ output */
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL_USBCLK);
    }

    if (PeriphClkInit->UsbClockSelection == RCC_USBCLKSOURCE_PLLSAI1)
    {
      /* PLLSAI1 parameters N & Q configuration and clock output (PLLSAI1ClockOut) */
      ret = RCCEx_PLLSAI1_ConfigNQ(&(PeriphClkInit->PLLSAI1));

      if (ret != HAL_OK)
      {
        /* set overall return value */
        status = ret;
      }
    }
  }

  /*-------------------------- RNG clock source configuration ----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RNG) == (RCC_PERIPHCLK_RNG))
  {
    assert_param(IS_RCC_RNGCLKSOURCE(PeriphClkInit->RngClockSelection));
    __HAL_RCC_RNG_CONFIG(PeriphClkInit->RngClockSelection);

  }

  /*-------------------------- ADC clock source configuration ----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_ADC) == RCC_PERIPHCLK_ADC)
  {
    /* Check the parameters */
    assert_param(IS_RCC_ADCCLKSOURCE(PeriphClkInit->AdcClockSelection));

    /* Configure the ADC interface clock source */
    __HAL_RCC_ADC_CONFIG(PeriphClkInit->AdcClockSelection);

    if (PeriphClkInit->AdcClockSelection == RCC_ADCCLKSOURCE_PLL)
    {
      /* Enable RCC_PLL_RNGCLK output */
      __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL_ADCCLK);
    }

    if (PeriphClkInit->AdcClockSelection == RCC_ADCCLKSOURCE_PLLSAI1)
    {
      /* PLLSAI1 parameters N & R configuration and clock output (PLLSAI1ClockOut) */
      ret = RCCEx_PLLSAI1_ConfigNR(&(PeriphClkInit->PLLSAI1));

      if (ret != HAL_OK)
      {
        /* set overall return value */
        status = ret;
      }
    }
  }

  /*-------------------------- RFWKP clock source configuration ----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RFWAKEUP) == RCC_PERIPHCLK_RFWAKEUP)
  {
    /* Check the parameters */
    assert_param(IS_RCC_RFWKPCLKSOURCE(PeriphClkInit->RFWakeUpClockSelection));

    /* Configure the RFWKP interface clock source */
    __HAL_RCC_RFWAKEUP_CONFIG(PeriphClkInit->RFWakeUpClockSelection);

  }

  /*-------------------------- SMPS clock source configuration ----------------------*/
  if (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_SMPS) == RCC_PERIPHCLK_SMPS)
  {
    /* Check the parameters */
    assert_param(IS_RCC_SMPSCLKDIV(PeriphClkInit->SmpsDivSelection));
    assert_param(IS_RCC_SMPSCLKSOURCE(PeriphClkInit->SmpsClockSelection));

    /* Configure the SMPS interface clock division factor */
    __HAL_RCC_SMPS_DIV_CONFIG(PeriphClkInit->SmpsDivSelection);

    /* Configure the SMPS interface clock source */
    __HAL_RCC_SMPS_CONFIG(PeriphClkInit->SmpsClockSelection);

  }

  return status;
}


/**
  * @brief  Get the RCC_ClkInitStruct according to the internal RCC configuration registers.
  * @param  PeriphClkInit  pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         returns the configuration information for the Extended Peripherals
  *         clocks(SAI1, LPTIM1, LPTIM2, I2C1, I2C3, LPUART1,
  *         USART1, RTC, ADCx, USB, RNG, RFWKP, SMPS).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  /* Set all possible values for the extended clock type parameter------------*/
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_LPUART1 | RCC_PERIPHCLK_I2C1  | RCC_PERIPHCLK_I2C3     | \
                                        RCC_PERIPHCLK_LPTIM1 | RCC_PERIPHCLK_LPTIM2  | RCC_PERIPHCLK_SAI1  | RCC_PERIPHCLK_USB      | \
                                        RCC_PERIPHCLK_RNG    | RCC_PERIPHCLK_ADC     | RCC_PERIPHCLK_RTC   | RCC_PERIPHCLK_RFWAKEUP | \
                                        RCC_PERIPHCLK_SMPS;

  /* Get the PLLSAI1 Clock configuration -----------------------------------------------*/
  PeriphClkInit->PLLSAI1.PLLN = LL_RCC_PLLSAI1_GetN();
  PeriphClkInit->PLLSAI1.PLLP = LL_RCC_PLLSAI1_GetP();
  PeriphClkInit->PLLSAI1.PLLR = LL_RCC_PLLSAI1_GetR();
  PeriphClkInit->PLLSAI1.PLLQ = LL_RCC_PLLSAI1_GetQ();

  /* Get the USART1 clock source ---------------------------------------------*/
  PeriphClkInit->Usart1ClockSelection   = __HAL_RCC_GET_USART1_SOURCE();

  /* Get the LPUART1 clock source --------------------------------------------*/
  PeriphClkInit->Lpuart1ClockSelection  = __HAL_RCC_GET_LPUART1_SOURCE();

  /* Get the I2C1 clock source -----------------------------------------------*/
  PeriphClkInit->I2c1ClockSelection     = __HAL_RCC_GET_I2C1_SOURCE();

  /* Get the I2C3 clock source -----------------------------------------------*/
  PeriphClkInit->I2c3ClockSelection     = __HAL_RCC_GET_I2C3_SOURCE();

  /* Get the LPTIM1 clock source ---------------------------------------------*/
  PeriphClkInit->Lptim1ClockSelection   = __HAL_RCC_GET_LPTIM1_SOURCE();

  /* Get the LPTIM2 clock source ---------------------------------------------*/
  PeriphClkInit->Lptim2ClockSelection   = __HAL_RCC_GET_LPTIM2_SOURCE();

  /* Get the SAI1 clock source -----------------------------------------------*/
  PeriphClkInit->Sai1ClockSelection     = __HAL_RCC_GET_SAI1_SOURCE();

  /* Get the RTC clock source ------------------------------------------------*/
  PeriphClkInit->RTCClockSelection      = __HAL_RCC_GET_RTC_SOURCE();

  /* Get the USB clock source ------------------------------------------------*/
  PeriphClkInit->UsbClockSelection      = __HAL_RCC_GET_USB_SOURCE();

  /* Get the RNG clock source ------------------------------------------------*/
  PeriphClkInit->RngClockSelection      = __HAL_RCC_GET_RNG_SOURCE();

  /* Get the ADC clock source ------------------------------------------------*/
  PeriphClkInit->AdcClockSelection      = __HAL_RCC_GET_ADC_SOURCE();

  /* Get the RFWKP clock source ----------------------------------------------*/
  PeriphClkInit->RFWakeUpClockSelection = __HAL_RCC_GET_RFWAKEUP_SOURCE();

  /* Get the SMPS clock division factor --------------------------------------*/
  PeriphClkInit->SmpsDivSelection       = __HAL_RCC_GET_SMPS_DIV();

  /* Get the SMPS clock source -----------------------------------------------*/
  PeriphClkInit->SmpsClockSelection     = __HAL_RCC_GET_SMPS_SOURCE();
}

/**
  * @brief  Return the peripheral clock frequency for peripherals with clock source
  * @note   Return 0 if peripheral clock identifier not managed by this API
  * @param  PeriphClk  Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_PERIPHCLK_RTC  RTC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_ADC  ADC peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C1  I2C1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_I2C3  I2C3 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPTIM1  LPTIM1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPTIM2  LPTIM2 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_LPUART1  LPUART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_RNG  RNG peripheral clock
  *            @arg @ref RCC_PERIPHCLK_SAI1  SAI1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USART1  USART1 peripheral clock
  *            @arg @ref RCC_PERIPHCLK_USB  USB peripheral clock
  *
  *            @arg @ref RCC_PERIPHCLK_RFWAKEUP  RFWKP peripheral clock
  *            @arg @ref RCC_PERIPHCLK_SMPS  SMPS peripheral clock
  * @retval Frequency in Hz
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{
  uint32_t frequency;
  uint32_t smps_prescaler_index = ((LL_RCC_GetSMPSPrescaler()) >> RCC_SMPSCR_SMPSDIV_Pos);

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClk));

  if(PeriphClk == RCC_PERIPHCLK_RTC)
  {
    uint32_t rtcClockSource = LL_RCC_GetRTCClockSource();

    if(rtcClockSource == LL_RCC_RTC_CLKSOURCE_LSE) /* LSE clock used as RTC clock source */
    {
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        frequency = LSE_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if(rtcClockSource == LL_RCC_RTC_CLKSOURCE_LSI) /* LSI clock used as RTC clock source */
    {
      const uint32_t temp_lsi1ready = LL_RCC_LSI1_IsReady();
      const uint32_t temp_lsi2ready = LL_RCC_LSI2_IsReady();
      if ((temp_lsi1ready == 1U) || (temp_lsi2ready == 1U))
      {
        frequency = LSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if (rtcClockSource == LL_RCC_RTC_CLKSOURCE_HSE_DIV32) /* HSE clock used as RTC clock source */
    {
      frequency = HSE_VALUE / 32U;
    }
    else /* No clock used as RTC clock source */
    {
      frequency = 0;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_SAI1)
  {
    switch (LL_RCC_GetSAIClockSource(LL_RCC_SAI1_CLKSOURCE))
    {
      case LL_RCC_SAI1_CLKSOURCE_HSI:        /* HSI clock used as SAI1 clock source */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          frequency = HSI_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_SAI1_CLKSOURCE_PLLSAI1:    /* PLLSAI1 clock used as SAI1 clock source */
        if (LL_RCC_PLLSAI1_IsReady() == 1U)
        {
          frequency = RCC_PLLSAI1_GetFreqDomain_P();
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_SAI1_CLKSOURCE_PLL:        /* PLL clock used as SAI1 clock source */
        if (LL_RCC_PLL_IsReady() == 1U)
        {
          frequency = RCC_PLL_GetFreqDomain_P();
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* External input clock used as SAI1 clock source */
        frequency = EXTERNAL_SAI1_CLOCK_VALUE;
        break;
    }
  }
  else if(PeriphClk == RCC_PERIPHCLK_RNG)
  {
    uint32_t rngClockSource = LL_RCC_GetRNGClockSource(LL_RCC_RNG_CLKSOURCE);

    if(rngClockSource == LL_RCC_RNG_CLKSOURCE_LSI) /* LSI clock used as RNG clock source */
    {
      const uint32_t temp_lsi1ready = LL_RCC_LSI1_IsReady();
      const uint32_t temp_lsi2ready = LL_RCC_LSI2_IsReady();
      if ((temp_lsi1ready == 1U) || (temp_lsi2ready == 1U))
      {
        frequency = LSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if(rngClockSource == LL_RCC_RNG_CLKSOURCE_LSE) /* LSE clock used as RNG clock source */
    {
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        frequency = LSE_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else /* CLK48 clock used as RNG clock source */
    {
      switch (LL_RCC_GetUSBClockSource(LL_RCC_USB_CLKSOURCE))
      {
        case LL_RCC_USB_CLKSOURCE_PLLSAI1: /* PLLSAI1 clock used as USB clock source */
          if (LL_RCC_PLLSAI1_IsReady() == 1U)
          {
            frequency = RCC_PLLSAI1_GetFreqDomain_Q();
          }
          else
          {
            frequency = 0U;
          }
          break;

        case LL_RCC_USB_CLKSOURCE_PLL: /* PLL clock used as USB clock source */
          if (LL_RCC_PLL_IsReady() == 1U)
          {
            frequency = RCC_PLL_GetFreqDomain_Q();
          }
          else
          {
            frequency = 0U;
          }
          break;

        case LL_RCC_USB_CLKSOURCE_MSI: /* MSI clock used as USB clock source */
          if (LL_RCC_MSI_IsReady() == 1U)
          {
            frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
          }
          else
          {
            frequency = 0U;
          }
          break;

        default: /* HSI48 clock used as USB clock source */
          if (LL_RCC_HSI48_IsReady() == 1U)
          {
            frequency = HSI48_VALUE;
          }
          else
          {
            frequency = 0U;
          }
        break;
      }
      frequency = frequency / 3U;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_USB)
  {
    switch (LL_RCC_GetUSBClockSource(LL_RCC_USB_CLKSOURCE))
    {
      case LL_RCC_USB_CLKSOURCE_PLLSAI1:       /* PLLSAI1 clock used as USB clock source */
        if (LL_RCC_PLLSAI1_IsReady() == 1U)
        {
          frequency = RCC_PLLSAI1_GetFreqDomain_Q();
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_USB_CLKSOURCE_PLL:           /* PLL clock used as USB clock source */
        if (LL_RCC_PLL_IsReady() == 1U)
        {
          frequency = RCC_PLL_GetFreqDomain_Q();
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_USB_CLKSOURCE_MSI:           /* MSI clock used as USB clock source */
        if (LL_RCC_MSI_IsReady() == 1U)
        {
          frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* HSI48 clock used as USB clock source */
        if (LL_RCC_HSI48_IsReady() == 1U)
        {
          frequency = HSI48_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_USART1)
  {
    switch (LL_RCC_GetUSARTClockSource(LL_RCC_USART1_CLKSOURCE))
    {
      case LL_RCC_USART1_CLKSOURCE_SYSCLK: /* USART1 Clock is System Clock */
        frequency = HAL_RCC_GetSysClockFreq();
        break;

      case LL_RCC_USART1_CLKSOURCE_HSI:    /* USART1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          frequency = HSI_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_USART1_CLKSOURCE_LSE:    /* USART1 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady() == 1U)
        {
          frequency = LSE_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* USART1 Clock is PCLK2 */
        frequency = __LL_RCC_CALC_PCLK2_FREQ(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), \
                                             LL_RCC_GetAHBPrescaler()), LL_RCC_GetAPB2Prescaler());
        break;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_LPUART1)
  {
    switch (LL_RCC_GetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE))
    {
      case LL_RCC_LPUART1_CLKSOURCE_SYSCLK: /* LPUART1 Clock is System Clock */
        frequency = HAL_RCC_GetSysClockFreq();
        break;

      case LL_RCC_LPUART1_CLKSOURCE_HSI:    /* LPUART1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          frequency = HSI_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_LPUART1_CLKSOURCE_LSE:    /* LPUART1 Clock is LSE Osc. */
        if (LL_RCC_LSE_IsReady() == 1U)
        {
          frequency = LSE_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* LPUART1 Clock is PCLK1 */
        frequency = __LL_RCC_CALC_PCLK1_FREQ(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), \
                                             LL_RCC_GetAHBPrescaler()), LL_RCC_GetAPB1Prescaler());
        break;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_ADC)
  {
    switch (LL_RCC_GetADCClockSource(LL_RCC_ADC_CLKSOURCE))
    {
      case LL_RCC_ADC_CLKSOURCE_PLLSAI1:       /* PLLSAI1 clock used as ADC clock source */
        if (LL_RCC_PLLSAI1_IsReady() == 1U)
        {
          frequency = RCC_PLLSAI1_GetFreqDomain_R();
        }
        else
        {
          frequency = 0U;
        }
        break;

      case LL_RCC_ADC_CLKSOURCE_SYSCLK:        /* SYSCLK clock used as ADC clock source */
        frequency = HAL_RCC_GetSysClockFreq();
        break;

      case LL_RCC_ADC_CLKSOURCE_PLL:           /* PLL clock used as USB clock source */
        if (LL_RCC_PLL_IsReady() == 1U)
        {
          frequency = RCC_PLL_GetFreqDomain_P();
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* No clock used as ADC clock source */
        frequency = 0;
        break;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_I2C1)
  {
    switch (LL_RCC_GetI2CClockSource(LL_RCC_I2C1_CLKSOURCE))
    {
      case LL_RCC_I2C1_CLKSOURCE_SYSCLK: /* I2C1 Clock is System Clock */
        frequency = HAL_RCC_GetSysClockFreq();
        break;

      case LL_RCC_I2C1_CLKSOURCE_HSI:    /* I2C1 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          frequency = HSI_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* I2C1 Clock is PCLK1 */
        frequency = __LL_RCC_CALC_PCLK1_FREQ(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), \
                                             LL_RCC_GetAHBPrescaler()), LL_RCC_GetAPB1Prescaler());
        break;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_I2C3)
  {
    switch (LL_RCC_GetI2CClockSource(LL_RCC_I2C3_CLKSOURCE))
    {
      case LL_RCC_I2C3_CLKSOURCE_SYSCLK: /* I2C3 Clock is System Clock */
        frequency = HAL_RCC_GetSysClockFreq();
        break;

      case LL_RCC_I2C3_CLKSOURCE_HSI: /* I2C3 Clock is HSI Osc. */
        if (LL_RCC_HSI_IsReady() == 1U)
        {
          frequency = HSI_VALUE;
        }
        else
        {
          frequency = 0U;
        }
        break;

      default: /* I2C3 Clock is PCLK1 */
        frequency = __LL_RCC_CALC_PCLK1_FREQ(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), \
                                             LL_RCC_GetAHBPrescaler()), LL_RCC_GetAPB1Prescaler());
        break;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_LPTIM1)
  {
    uint32_t lptimClockSource = LL_RCC_GetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE);

    if(lptimClockSource == LL_RCC_LPTIM1_CLKSOURCE_LSI) /* LPTIM1 Clock is LSI Osc. */
    {
      const uint32_t temp_lsi1ready = LL_RCC_LSI1_IsReady();
      const uint32_t temp_lsi2ready = LL_RCC_LSI2_IsReady();
      if ((temp_lsi1ready == 1U) || (temp_lsi2ready == 1U))
      {
        frequency = LSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if( lptimClockSource == LL_RCC_LPTIM1_CLKSOURCE_HSI) /* LPTIM1 Clock is HSI Osc. */
    {
      if (LL_RCC_HSI_IsReady() == 1U)
      {
        frequency = HSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if(lptimClockSource == LL_RCC_LPTIM1_CLKSOURCE_LSE) /* LPTIM1 Clock is LSE Osc. */
    {
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        frequency = LSE_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else /* LPTIM1 Clock is PCLK1 */
    {
      frequency = __LL_RCC_CALC_PCLK1_FREQ(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), LL_RCC_GetAHBPrescaler()), LL_RCC_GetAPB1Prescaler());
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_LPTIM2)
  {
    uint32_t lptimClockSource = LL_RCC_GetLPTIMClockSource(LL_RCC_LPTIM2_CLKSOURCE);

    if( lptimClockSource == LL_RCC_LPTIM2_CLKSOURCE_LSI) /* LPTIM2 Clock is LSI Osc. */
    {
      const uint32_t temp_lsi1ready = LL_RCC_LSI1_IsReady();
      const uint32_t temp_lsi2ready = LL_RCC_LSI2_IsReady();
      if ((temp_lsi1ready == 1U) || (temp_lsi2ready == 1U))
      {
        frequency = LSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if (lptimClockSource == LL_RCC_LPTIM2_CLKSOURCE_HSI) /* LPTIM2 Clock is HSI Osc. */
    {
      if (LL_RCC_HSI_IsReady() == 1U)
      {
        frequency = HSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if (lptimClockSource == LL_RCC_LPTIM2_CLKSOURCE_LSE) /* LPTIM2 Clock is LSE Osc. */
    {
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        frequency = LSE_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else /* LPTIM2 Clock is PCLK1 */
    {
      frequency = __LL_RCC_CALC_PCLK1_FREQ(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), LL_RCC_GetAHBPrescaler()), LL_RCC_GetAPB1Prescaler());
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_RFWAKEUP)
  {
    uint32_t rfwkpClockSource = LL_RCC_GetRFWKPClockSource();

    if (rfwkpClockSource == LL_RCC_RFWKP_CLKSOURCE_LSE) /* LSE clock used as RF Wakeup clock source */
    {
      if (LL_RCC_LSE_IsReady() == 1U)
      {
        frequency = LSE_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if (rfwkpClockSource == LL_RCC_RFWKP_CLKSOURCE_LSI) /* LSI clock used as RF Wakeup clock source */
    {
      const uint32_t temp_lsi1ready = LL_RCC_LSI1_IsReady();
      const uint32_t temp_lsi2ready = LL_RCC_LSI2_IsReady();
      if ((temp_lsi1ready == 1U) || (temp_lsi2ready == 1U))
      {
        frequency = LSI_VALUE;
      }
      else
      {
        frequency = 0U;
      }
    }
    else if (rfwkpClockSource == LL_RCC_RFWKP_CLKSOURCE_HSE_DIV1024) /* HSE clock used as RF Wakeup clock source */
    {
      frequency = HSE_VALUE / 1024U;
    }
    else /* No clock used as RF Wakeup clock source */
    {
      frequency = 0;
    }
  }
  else if (PeriphClk == RCC_PERIPHCLK_SMPS)
  {
    uint32_t smpsClockSource = LL_RCC_GetSMPSClockSource();

    if(smpsClockSource == LL_RCC_SMPS_CLKSOURCE_STATUS_HSI) /* SMPS Clock source is HSI Osc. */
    {
      if (LL_RCC_HSI_IsReady() == 1U)
      {
        frequency = HSI_VALUE / SmpsPrescalerTable[smps_prescaler_index][0];
        frequency = frequency >> 1U; /* Systematic Div by 2 */
      }
      else
      {
        frequency = 0U;
      }
    }
    else if(smpsClockSource == LL_RCC_SMPS_CLKSOURCE_STATUS_HSE) /* SMPS Clock source is HSE Osc. */
    {
      if (LL_RCC_HSE_IsReady() == 1U)
      {
        frequency = HSE_VALUE / SmpsPrescalerTable[smps_prescaler_index][5];
        frequency = frequency >> 1U; /* Systematic Div by 2 */
      }
      else
      {
        frequency = 0U;
      }
    }
    else if(smpsClockSource == LL_RCC_SMPS_CLKSOURCE_STATUS_MSI) /* SMPS Clock source is MSI Osc. */
    {
      switch (LL_RCC_MSI_GetRange())
      {
        case LL_RCC_MSIRANGE_8:
          frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_8) / SmpsPrescalerTable[smps_prescaler_index][4];
          break;
        case LL_RCC_MSIRANGE_9:
          frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_9) / SmpsPrescalerTable[smps_prescaler_index][3];
          break;
        case LL_RCC_MSIRANGE_10:
          frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_10) / SmpsPrescalerTable[smps_prescaler_index][2];
          break;
        case LL_RCC_MSIRANGE_11:
          frequency = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSIRANGE_11) / SmpsPrescalerTable[smps_prescaler_index][1];
          break;
        default:
          frequency = 0U;
          break;
      }
      frequency = frequency >> 1U; /* Systematic Div by 2 */
    }
    else /* SMPS has no Clock */
    {
      frequency = 0U;
    }
  }
  else
  {
    frequency = 0U;
  }
  return (frequency);
}

/**
  * @}
  */

/** @defgroup RCCEx_Exported_Functions_Group2 Extended Clock management functions
 *  @brief  Extended Clock management functions
 *
@verbatim
 ===============================================================================
                ##### Extended clock management functions  #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to control the
    activation or deactivation of MSI PLL-mode, PLLSAI1, PLLSAI12, LSE CSS,
    Low speed clock output and clock after wake-up from STOP mode.
@endverbatim
  * @{
  */

/**
  * @brief  Enable PLLSAI1.
  * @param  PLLSAI1Init  pointer to an RCC_PLLSAI1InitTypeDef structure that
  *         contains the configuration information for the PLLSAI1
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_EnablePLLSAI1(RCC_PLLSAI1InitTypeDef  *PLLSAI1Init)
{
  uint32_t tickstart;
  HAL_StatusTypeDef status = HAL_OK;

  /* check for PLLSAI1 Parameters used to output PLLSAI1CLK */
  assert_param(IS_RCC_PLLN_VALUE(PLLSAI1Init->PLLN));
  assert_param(IS_RCC_PLLP_VALUE(PLLSAI1Init->PLLP));
  assert_param(IS_RCC_PLLQ_VALUE(PLLSAI1Init->PLLQ));
  assert_param(IS_RCC_PLLR_VALUE(PLLSAI1Init->PLLR));
  assert_param(IS_RCC_PLLSAI1CLOCKOUT_VALUE(PLLSAI1Init->PLLSAI1ClockOut));

  /* Disable the PLLSAI1 */
  __HAL_RCC_PLLSAI1_DISABLE();

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till PLLSAI1 is ready to be updated */
  while (LL_RCC_PLLSAI1_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
    {
      status = HAL_TIMEOUT;
      break;
    }
  }

  if (status == HAL_OK)
  {
    /* Configure the PLLSAI1 Multiplication factor N */
    /* Configure the PLLSAI1 Division factors P, Q and R */
    __HAL_RCC_PLLSAI1_CONFIG(PLLSAI1Init->PLLN, PLLSAI1Init->PLLP, PLLSAI1Init->PLLQ, PLLSAI1Init->PLLR);
    /* Configure the PLLSAI1 Clock output(s) */
    __HAL_RCC_PLLSAI1CLKOUT_ENABLE(PLLSAI1Init->PLLSAI1ClockOut);

    /* Enable the PLLSAI1 again by setting PLLSAI1ON to 1*/
    __HAL_RCC_PLLSAI1_ENABLE();

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till PLLSAI1 is ready */
    while (LL_RCC_PLLSAI1_IsReady() != 1U)
    {
      if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
      {
        status = HAL_TIMEOUT;
        break;
      }
    }
  }

  return status;
}

/**
  * @brief  Disable PLLSAI1.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_DisablePLLSAI1(void)
{
  uint32_t tickstart;
  HAL_StatusTypeDef status = HAL_OK;

  /* Disable the PLLSAI1 */
  __HAL_RCC_PLLSAI1_DISABLE();

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till PLLSAI1 is ready */
  while (LL_RCC_PLLSAI1_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
    {
      status = HAL_TIMEOUT;
      break;
    }
  }

  /* Disable the PLLSAI1 Clock outputs */
  __HAL_RCC_PLLSAI1CLKOUT_DISABLE(RCC_PLLSAI1_SAI1CLK | RCC_PLLSAI1_USBCLK | RCC_PLLSAI1_RNGCLK | RCC_PLLSAI1_ADCCLK);

  return status;
}

/***********************************************************************************************/

/**
  * @brief  Configure the oscillator clock source for wakeup from Stop and CSS backup clock.
  * @param  WakeUpClk  Wakeup clock
  *         This parameter can be one of the following values:
  *            @arg @ref RCC_STOP_WAKEUPCLOCK_MSI  MSI oscillator selection
  *            @arg @ref RCC_STOP_WAKEUPCLOCK_HSI  HSI oscillator selection
  * @note   This function shall not be called after the Clock Security System on HSE has been
  *         enabled.
  * @retval None
  */
void HAL_RCCEx_WakeUpStopCLKConfig(uint32_t WakeUpClk)
{
  assert_param(IS_RCC_STOP_WAKEUPCLOCK(WakeUpClk));

  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(WakeUpClk);
}

/**
  * @brief  Enable the LSE Clock Security System.
  * @note   Prior to enable the LSE Clock Security System, LSE oscillator is to be enabled
  *         with HAL_RCC_OscConfig() and the LSE oscillator clock is to be selected as RTC
  *         clock with HAL_RCCEx_PeriphCLKConfig().
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS(void)
{
  LL_RCC_LSE_EnableCSS();
}

/**
  * @brief  Disable the LSE Clock Security System.
  * @note   LSE Clock Security System can only be disabled after a LSE failure detection.
  * @retval None
  */
void HAL_RCCEx_DisableLSECSS(void)
{
  LL_RCC_LSE_DisableCSS();

  /* Disable LSE CSS IT if any */
  __HAL_RCC_DISABLE_IT(RCC_IT_LSECSS);
}

/**
  * @brief  Enable the LSE Clock Security System Interrupt & corresponding EXTI line.
  * @note   LSE Clock Security System Interrupt is mapped on RTC EXTI line 18
  * @retval None
  */
void HAL_RCCEx_EnableLSECSS_IT(void)
{
  /* Enable LSE CSS */
  LL_RCC_LSE_EnableCSS();

  /* Enable LSE CSS IT */
  __HAL_RCC_ENABLE_IT(RCC_IT_LSECSS);

  /* Enable IT on EXTI Line 18 */
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
  if (__HAL_RCC_GET_IT(RCC_IT_LSECSS))
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

/**
  * @brief  Select the clock source to output on LSCO1 pin(PA2) or LSC02 pin (PH3) or LSCO3 pin (PC12).
  * @note   PA2, PH3 or PC12 should be configured in alternate function mode.
  * @param  RCC_LSCOx  specifies the output direction for the clock source.
  *            @arg @ref RCC_LSCO1  Clock source to output on LSCO1 pin(PA2)
  *            @arg @ref RCC_LSCO2  Clock source to output on LSCO2 pin(PH13)
  *            @arg @ref RCC_LSCO3  Clock source to output on LSCO3 pin(PC12)
  * @param  RCC_LSCOSource  specifies the clock source to output.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LSCOSOURCE_LSI  LSI clock selected as LSCO source
  *            @arg @ref RCC_LSCOSOURCE_LSE  LSE clock selected as LSCO source
  * @retval None
  * @note   LSCO should be disable with @ref HAL_RCCEx_DisableLSCO
  */
void HAL_RCCEx_LSCOConfig(uint32_t RCC_LSCOx, uint32_t RCC_LSCOSource)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  FlagStatus backupchanged;

  /* Check the parameters */
  assert_param(IS_RCC_LSCO(RCC_LSCOx));
  assert_param(IS_RCC_LSCOSOURCE(RCC_LSCOSource));

  /* Common GPIO init parameters */
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;

  /* RCC_LSCO1 */
  if(RCC_LSCOx == RCC_LSCO1)
  {
    /* LSCO1 Clock Enable */
    __LSCO1_CLK_ENABLE();
    /* Configue the LSCO1 pin in alternate function mode */
    GPIO_InitStruct.Pin       = LSCO1_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF0_LSCO;
    HAL_GPIO_Init(LSCO1_GPIO_PORT, &GPIO_InitStruct);

  }
  else if (RCC_LSCOx == RCC_LSCO2)
  {
    /* LSCO2 Clock Enable */
    __LSCO2_CLK_ENABLE();
    /* Configue the LSCO2 pin in alternate function mode */
    GPIO_InitStruct.Pin       = LSCO2_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF0_LSCO;
    HAL_GPIO_Init(LSCO2_GPIO_PORT, &GPIO_InitStruct);

  }
  else
  {
    /* LSCO3 Clock Enable */
    __LSCO3_CLK_ENABLE();
    /* Configue the LSCO3 pin in alternate function mode */
    GPIO_InitStruct.Pin       = LSCO3_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF6_LSCO;
    HAL_GPIO_Init(LSCO3_GPIO_PORT, &GPIO_InitStruct);
  }

    /* Update LSCOSEL clock source in Backup Domain control register */
    if (HAL_IS_BIT_CLR(PWR->CR1, PWR_CR1_DBP))
    {
      HAL_PWR_EnableBkUpAccess();
      backupchanged = SET;
    }
    else
    {
      backupchanged = RESET;
    }

    MODIFY_REG(RCC->BDCR, RCC_BDCR_LSCOSEL | RCC_BDCR_LSCOEN, RCC_LSCOSource | RCC_BDCR_LSCOEN);

    if (backupchanged == SET)
    {
      HAL_PWR_DisableBkUpAccess();
    }

}



/**
  * @brief  Select the Low Speed clock source to output on LSCO pin (PA2).
  * @param  LSCOSource  specifies the Low Speed clock source to output.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_LSCOSOURCE_LSI  LSI clock selected as LSCO source
  *            @arg @ref RCC_LSCOSOURCE_LSE  LSE clock selected as LSCO source
  * @retval None
  */
void HAL_RCCEx_EnableLSCO(uint32_t LSCOSource)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  FlagStatus backupchanged;

  /* Check the parameters */
  assert_param(IS_RCC_LSCOSOURCE(LSCOSource));

  /* LSCO Pin Clock Enable */
  __LSCO1_CLK_ENABLE();

  /* Configure the LSCO pin in analog mode */
  GPIO_InitStruct.Pin   = LSCO1_PIN;
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Alternate = GPIO_AF0_LSCO;
  HAL_GPIO_Init(LSCO1_GPIO_PORT, &GPIO_InitStruct);

  /* Update LSCOSEL clock source in Backup Domain control register */
  if (HAL_IS_BIT_CLR(PWR->CR1, PWR_CR1_DBP))
  {
    HAL_PWR_EnableBkUpAccess();
    backupchanged = SET;
  }
  else
  {
    backupchanged = RESET;
  }

  MODIFY_REG(RCC->BDCR, RCC_BDCR_LSCOSEL | RCC_BDCR_LSCOEN, LSCOSource | RCC_BDCR_LSCOEN);

  if (backupchanged == SET)
  {
    HAL_PWR_DisableBkUpAccess();
  }
}

/**
  * @brief  Disable the Low Speed clock output.
  * @retval None
  */
void HAL_RCCEx_DisableLSCO(void)
{
  FlagStatus backupchanged;

  if (HAL_IS_BIT_CLR(PWR->CR1, PWR_CR1_DBP))
  {
    /* Enable access to the backup domain */
    HAL_PWR_EnableBkUpAccess();
    backupchanged = SET;
  }
  else
  {
    backupchanged = RESET;
  }

  LL_RCC_LSCO_Disable();

  /* Restore previous configuration */
  if (backupchanged == SET)
  {
    /* Disable access to the backup domain */
    HAL_PWR_DisableBkUpAccess();
  }
}

/**
  * @brief  Enable the PLL-mode of the MSI.
  * @note   Prior to enable the PLL-mode of the MSI for automatic hardware
  *         calibration LSE oscillator is to be enabled with @ref HAL_RCC_OscConfig().
  * @retval None
  */
void HAL_RCCEx_EnableMSIPLLMode(void)
{
  LL_RCC_MSI_EnablePLLMode() ;
}

/**
  * @brief  Disable the PLL-mode of the MSI.
  * @note   PLL-mode of the MSI is automatically reset when LSE oscillator is disabled.
  * @retval None
  */
void HAL_RCCEx_DisableMSIPLLMode(void)
{
  LL_RCC_MSI_DisablePLLMode() ;
}

/**
  * @brief Set trimming value
  * @param OscillatorType Specifies the oscillator to be trimmed
  *   This parameter can be one of the following values:
  *     @arg @ref RCC_OSCILLATORTYPE_LSI2 LSI2 oscillator selected.
  *       When disabling and re-enabling the LSI2 there is no need for re-trimming
  *       Trimming is only needed once after a NRST reset.
  *       Trimming values comes from factory trimmed flash location (0x1FFF7548).
  * @note The LSI2 oscillator must be disabled before calling this trimming function through @ref HAL_RCC_OscConfig
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_TrimOsc(uint32_t OscillatorType)
{
  #define FTLSI2TRIM (0xFUL)
  HAL_StatusTypeDef status = HAL_OK;

  assert_param(IS_RCC_TRIMOSC(OscillatorType));

  if(OscillatorType == RCC_OSCILLATORTYPE_LSI2)
  {
    if (LL_RCC_LSI2_IsReady() == 1U)
    {
      status = HAL_ERROR;
    }
    else
    {
      /* Copy the LSI2 trimming information from the factory trimmed Flash location */
      uint32_t factoryTrimming = ((*(uint32_t *)(0x1FFF7548)) & FTLSI2TRIM);
      LL_RCC_LSI2_SetTrimming(factoryTrimming);
    }
  }
  else
  {
    status = HAL_ERROR;
  }
  return status;
}


/**
  * @}
  */


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
                        directly reload value with target and sychronization frequencies values
          (##) Call function @ref HAL_RCCEx_CRSConfig which
              (+++) Resets CRS registers to their default values.
              (+++) Configures CRS registers with synchronization configuration
              (+++) Enables automatic calibration and frequency error counter feature
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
          through CRS Handler (CRS_IRQn/CRS_IRQHandler)
              (++) Call function @ref HAL_RCCEx_CRSConfig()
              (++) Enable CRS_IRQn (thanks to NVIC functions)
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
  uint32_t value;

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
  value |= (pInit->ErrorLimitValue << CRS_CFGR_FELIM_Pos);
  WRITE_REG(CRS->CFGR, value);

  /* Adjust HSI48 oscillator smooth trimming */
  /* Set the TRIM[5:0] bits according to RCC_CRS_HSI48CalibrationValue value */
  MODIFY_REG(CRS->CR, CRS_CR_TRIM, (pInit->HSI48CalibrationValue << CRS_CR_TRIM_Pos));

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
  LL_CRS_GenerateEvent_SWSYNC();
}

/**
  * @brief  Return synchronization info
  * @param  pSynchroInfo Pointer on @ref RCC_CRSSynchroInfoTypeDef structure
  * @retval None
  */
void HAL_RCCEx_CRSGetSynchronizationInfo(RCC_CRSSynchroInfoTypeDef *pSynchroInfo)
{
  /* Check the parameter */
  assert_param(pSynchroInfo != (void *)NULL);

  /* Get the reload value */
  pSynchroInfo->ReloadValue = LL_CRS_GetReloadCounter();

  /* Get HSI48 oscillator smooth trimming */
  pSynchroInfo->HSI48CalibrationValue = LL_CRS_GetHSI48SmoothTrimming();

  /* Get Frequency error capture */
  pSynchroInfo->FreqErrorCapture = LL_CRS_GetFreqErrorCapture();

  /* Get Frequency error direction */
  pSynchroInfo->FreqErrorDirection = LL_CRS_GetFreqErrorDirection();
}

/**
* @brief  Wait for CRS Synchronization status.
* @param  Timeout  Duration of the timeout
* @note   Timeout is based on the maximum time to receive a SYNC event based on synchronization
*         frequency.
* @note   If Timeout set to HAL_MAX_DELAY, HAL_TIMEOUT will be never returned.
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
  uint32_t tickstart;

  /* Get timeout */
  tickstart = HAL_GetTick();

  /* Wait for CRS flag or timeout detection */
  do
  {
    if (Timeout != HAL_MAX_DELAY)
    {
      if(((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U))
      {
        crsstatus = RCC_CRS_TIMEOUT;
      }
    }
    /* Check CRS SYNCOK flag  */
    if (__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCOK))
    {
      /* CRS SYNC event OK */
      crsstatus |= RCC_CRS_SYNCOK;

      /* Clear CRS SYNC event OK bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCOK);
    }

    /* Check CRS SYNCWARN flag  */
    if (__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCWARN))
    {
      /* CRS SYNC warning */
      crsstatus |= RCC_CRS_SYNCWARN;

      /* Clear CRS SYNCWARN bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCWARN);
    }

    /* Check CRS TRIM overflow flag  */
    if (__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_TRIMOVF))
    {
      /* CRS SYNC Error */
      crsstatus |= RCC_CRS_TRIMOVF;

      /* Clear CRS Error bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_TRIMOVF);
    }

    /* Check CRS Error flag  */
    if (__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCERR))
    {
      /* CRS SYNC Error */
      crsstatus |= RCC_CRS_SYNCERR;

      /* Clear CRS Error bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCERR);
    }

    /* Check CRS SYNC Missed flag  */
    if (__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_SYNCMISS))
    {
      /* CRS SYNC Missed */
      crsstatus |= RCC_CRS_SYNCMISS;

      /* Clear CRS SYNC Missed bit */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_SYNCMISS);
    }

    /* Check CRS Expected SYNC flag  */
    if (__HAL_RCC_CRS_GET_FLAG(RCC_CRS_FLAG_ESYNC))
    {
      /* frequency error counter reached a zero value */
      __HAL_RCC_CRS_CLEAR_FLAG(RCC_CRS_FLAG_ESYNC);
    }
  }
  while (RCC_CRS_NONE == crsstatus);

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
  if (((itflags & RCC_CRS_FLAG_SYNCOK) != 0U) && ((itsources & RCC_CRS_IT_SYNCOK) != 0U))
  {
    /* Clear CRS SYNC event OK flag */
    LL_CRS_ClearFlag_SYNCOK();

    /* user callback */
    HAL_RCCEx_CRS_SyncOkCallback();
  }
  /* Check CRS SYNCWARN flag  */
  else if (((itflags & RCC_CRS_FLAG_SYNCWARN) != 0U) && ((itsources & RCC_CRS_IT_SYNCWARN) != 0U))
  {
    /* Clear CRS SYNCWARN flag */
    LL_CRS_ClearFlag_SYNCWARN();

    /* user callback */
    HAL_RCCEx_CRS_SyncWarnCallback();
  }
  /* Check CRS Expected SYNC flag  */
  else if (((itflags & RCC_CRS_FLAG_ESYNC) != 0U) && ((itsources & RCC_CRS_IT_ESYNC) != 0U))
  {
    /* frequency error counter reached a zero value */
    LL_CRS_ClearFlag_ESYNC();

    /* user callback */
    HAL_RCCEx_CRS_ExpectedSyncCallback();
  }
  /* Check CRS Error flags  */
  else
  {
    if (((itflags & RCC_CRS_FLAG_ERR) != 0U) && ((itsources & RCC_CRS_IT_ERR) != 0U))
    {
      if ((itflags & RCC_CRS_FLAG_SYNCERR) != 0U)
      {
        crserror |= RCC_CRS_SYNCERR;
      }
      if ((itflags & RCC_CRS_FLAG_SYNCMISS) != 0U)
      {
        crserror |= RCC_CRS_SYNCMISS;
      }
      if ((itflags & RCC_CRS_FLAG_TRIMOVF) != 0U)
      {
        crserror |= RCC_CRS_TRIMOVF;
      }

      /* Clear CRS Error flags */
      LL_CRS_ClearFlag_ERR();

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

/**
  * @}
  */

/** @addtogroup RCCEx_Private_Functions
 * @{
 */

/**
  * @brief  Configure the parameters N & P of PLLSAI1 and enable PLLSAI1 output clock(s).
  * @param  PLLSAI1  pointer to an RCC_PLLSAI1InitTypeDef structure that
  *         contains the configuration parameters N & P as well as PLLSAI1 output clock(s)
  *
  * @note   PLLSAI1 is temporary disable to apply new parameters
  *
  * @retval HAL status
  */
static HAL_StatusTypeDef RCCEx_PLLSAI1_ConfigNP(RCC_PLLSAI1InitTypeDef *PLLSAI1)
{
  uint32_t tickstart;
  HAL_StatusTypeDef status = HAL_OK;

  /* check for PLLSAI1 Parameters used to output PLLSAI1CLK */
  assert_param(IS_RCC_PLLN_VALUE(PLLSAI1->PLLN));
  assert_param(IS_RCC_PLLP_VALUE(PLLSAI1->PLLP));
  assert_param(IS_RCC_PLLSAI1CLOCKOUT_VALUE(PLLSAI1->PLLSAI1ClockOut));

  /* Disable the PLLSAI1 */
  __HAL_RCC_PLLSAI1_DISABLE();

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till PLLSAI1 is ready to be updated */
  while (LL_RCC_PLLSAI1_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
    {
      status = HAL_TIMEOUT;
      break;
    }
  }

  if (status == HAL_OK)
  {
    /* Configure the PLLSAI1 Multiplication factor N */
    __HAL_RCC_PLLSAI1_MULN_CONFIG(PLLSAI1->PLLN);

    /* Configure the PLLSAI1 Division factor P */
    __HAL_RCC_PLLSAI1_DIVP_CONFIG(PLLSAI1->PLLP);

    /* Enable the PLLSAI1 again by setting PLLSAI1ON to 1*/
    __HAL_RCC_PLLSAI1_ENABLE();

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till PLLSAI1 is ready */
    while (LL_RCC_PLLSAI1_IsReady() != 1U)
    {
      if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
      {
        status = HAL_TIMEOUT;
        break;
      }
    }

    if (status == HAL_OK)
    {
      /* Configure the PLLSAI1 Clock output(s) */
      __HAL_RCC_PLLSAI1CLKOUT_ENABLE(PLLSAI1->PLLSAI1ClockOut);
    }
  }

  return status;
}

/**
  * @brief  Configure the parameters N & Q of PLLSAI1 and enable PLLSAI1 output clock(s).
  * @param  PLLSAI1  pointer to an RCC_PLLSAI1InitTypeDef structure that
  *         contains the configuration parameters N & Q as well as PLLSAI1 output clock(s)
  *
  * @note   PLLSAI1 is temporary disable to apply new parameters
  *
  * @retval HAL status
  */
static HAL_StatusTypeDef RCCEx_PLLSAI1_ConfigNQ(RCC_PLLSAI1InitTypeDef *PLLSAI1)
{
  uint32_t tickstart;
  HAL_StatusTypeDef status = HAL_OK;

  /* check for PLLSAI1 Parameters used to output PLLSAI1CLK */
  assert_param(IS_RCC_PLLN_VALUE(PLLSAI1->PLLN));
  assert_param(IS_RCC_PLLQ_VALUE(PLLSAI1->PLLQ));
  assert_param(IS_RCC_PLLSAI1CLOCKOUT_VALUE(PLLSAI1->PLLSAI1ClockOut));

  /* Disable the PLLSAI1 */
  __HAL_RCC_PLLSAI1_DISABLE();

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till PLLSAI1 is ready to be updated */
  while (LL_RCC_PLLSAI1_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
    {
      status = HAL_TIMEOUT;
      break;
    }
  }

  if (status == HAL_OK)
  {
    /* Configure the PLLSAI1 Multiplication factor N */
    __HAL_RCC_PLLSAI1_MULN_CONFIG(PLLSAI1->PLLN);
    /* Configure the PLLSAI1 Division factor Q */
    __HAL_RCC_PLLSAI1_DIVQ_CONFIG(PLLSAI1->PLLQ);

    /* Enable the PLLSAI1 again by setting PLLSAI1ON to 1*/
    __HAL_RCC_PLLSAI1_ENABLE();

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till PLLSAI1 is ready */
    while (LL_RCC_PLLSAI1_IsReady() != 1U)
    {
      if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
      {
        status = HAL_TIMEOUT;
        break;
      }
    }

    if (status == HAL_OK)
    {
      /* Configure the PLLSAI1 Clock output(s) */
      __HAL_RCC_PLLSAI1CLKOUT_ENABLE(PLLSAI1->PLLSAI1ClockOut);
    }
  }

  return status;
}

/**
  * @brief  Configure the parameters N & R of PLLSAI1 and enable PLLSAI1 output clock(s).
  * @param  PLLSAI1  pointer to an RCC_PLLSAI1InitTypeDef structure that
  *         contains the configuration parameters N & R as well as PLLSAI1 output clock(s)
  *
  * @note   PLLSAI1 is temporary disable to apply new parameters
  *
  * @retval HAL status
  */
static HAL_StatusTypeDef RCCEx_PLLSAI1_ConfigNR(RCC_PLLSAI1InitTypeDef *PLLSAI1)
{
  uint32_t tickstart;
  HAL_StatusTypeDef status = HAL_OK;

  /* check for PLLSAI1 Parameters used to output PLLSAI1CLK */
  assert_param(IS_RCC_PLLN_VALUE(PLLSAI1->PLLN));
  assert_param(IS_RCC_PLLR_VALUE(PLLSAI1->PLLR));
  assert_param(IS_RCC_PLLSAI1CLOCKOUT_VALUE(PLLSAI1->PLLSAI1ClockOut));

  /* Disable the PLLSAI1 */
  __HAL_RCC_PLLSAI1_DISABLE();

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till PLLSAI1 is ready to be updated */
  while (LL_RCC_PLLSAI1_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
    {
      status = HAL_TIMEOUT;
      break;
    }
  }

  if (status == HAL_OK)
  {
    /* Configure the PLLSAI1 Multiplication factor N */
    __HAL_RCC_PLLSAI1_MULN_CONFIG(PLLSAI1->PLLN);
    /* Configure the PLLSAI1 Division factor R */
    __HAL_RCC_PLLSAI1_DIVR_CONFIG(PLLSAI1->PLLR);

    /* Enable the PLLSAI1 again by setting PLLSAI1ON to 1*/
    __HAL_RCC_PLLSAI1_ENABLE();

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Wait till PLLSAI1 is ready */
    while (LL_RCC_PLLSAI1_IsReady() != 1U)
    {
      if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
      {
        status = HAL_TIMEOUT;
        break;
      }
    }

    if (status == HAL_OK)
    {
      /* Configure the PLLSAI1 Clock output(s) */
      __HAL_RCC_PLLSAI1CLKOUT_ENABLE(PLLSAI1->PLLSAI1ClockOut);
    }
  }

  return status;
}

/**
  * @brief  Return PLL clock (PLLPCLK) frequency used for SAI domain
  * @retval PLLPCLK clock frequency (in Hz)
  */
static uint32_t RCC_PLL_GetFreqDomain_P(void)
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
  * @brief  Return PLL clock (PLLQCLK) frequency used for 48 MHz domain
  * @retval PLLQCLK clock frequency (in Hz)
  */
static uint32_t RCC_PLL_GetFreqDomain_Q(void)
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
  * @brief  Return PLLSAI1 clock (PLLSAI1RCLK) frequency used for ADC domain
  * @retval PLLSAI1RCLK clock frequency (in Hz)
  */
static uint32_t RCC_PLLSAI1_GetFreqDomain_R(void)
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
  * @brief  Return PLLSAI1 clock (PLLSAI1PCLK) frequency used for SAI domain
  * @retval PLLSAI1PCLK clock frequency (in Hz)
  */
static uint32_t RCC_PLLSAI1_GetFreqDomain_P(void)
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
static uint32_t RCC_PLLSAI1_GetFreqDomain_Q(void)
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

