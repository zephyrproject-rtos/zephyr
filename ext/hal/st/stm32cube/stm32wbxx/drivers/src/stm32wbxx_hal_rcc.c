/**
  ******************************************************************************
  * @file    stm32wbxx_hal_rcc.c
  * @author  MCD Application Team
  * @brief   RCC HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Reset and Clock Control (RCC) peripheral:
  *           + Initialization and de-initialization functions
  *           + Peripheral Control functions
  *
  @verbatim
  ==============================================================================
                      ##### RCC specific features #####
  ==============================================================================
    [..]
      After reset the device is running from Multiple Speed Internal oscillator
      (4 MHz) with Flash 0 wait state. Flash prefetch buffer, D-Cache
      and I-Cache are disabled, and all peripherals are off except internal
      SRAM, Flash and JTAG.

      (+) There is no prescaler on High speed (AHBs) and Low speed (APBs) busses:
          all peripherals mapped on these busses are running at MSI speed.
      (+) The clock for all peripherals is switched off, except the SRAM and FLASH.
      (+) All GPIOs are in analog mode, except the JTAG pins which
          are assigned to be used for debug purpose.

    [..]
      Once the device started from reset, the user application has to:
      (+) Configure the clock source to be used to drive the System clock
          (if the application needs higher frequency/performance)
      (+) Configure the System clock frequency and Flash settings
      (+) Configure the AHB and APB busses prescalers
      (+) Enable the clock for the peripheral(s) to be used
      (+) Configure the clock source(s) for peripherals which clocks are not
          derived from the System clock (SAI1, RTC, ADC, USB/RNG, USART1, LPUART1, LPTIMx, I2Cx, SMPS)

  @endverbatim
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

/** @defgroup RCC RCC
  * @brief RCC HAL module driver
  * @{
  */

#ifdef HAL_RCC_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup RCC_Private_Constants RCC Private Constants
 * @{
 */
#define HSE_TIMEOUT_VALUE          HSE_STARTUP_TIMEOUT
#define HSI_TIMEOUT_VALUE          (2U)    /* 2 ms (minimum Tick + 1)   */
#define MSI_TIMEOUT_VALUE          (2U)    /* 2 ms (minimum Tick + 1)   */
#define LSI1_TIMEOUT_VALUE         (2U)    /* 2 ms (minimum Tick + 1)   */
#define LSI2_TIMEOUT_VALUE         (3U)    /* to be adjusted with DS    */
#define HSI48_TIMEOUT_VALUE        (2U)    /* 2 ms (minimum Tick + 1)   */
#define PLL_TIMEOUT_VALUE          (2U)    /* 2 ms (minimum Tick + 1)   */
#define PLLSAI1_TIMEOUT_VALUE      (2U)    /* 2 ms (minimum Tick + 1)   */
#define PRESCALER_TIMEOUT_VALUE    (2U)    /* 2 ms (minimum Tick + 1)   */
#define LATENCY_TIMEOUT_VALUE      (2U)    /* 2 ms (minimum Tick + 1)   */
#define CLOCKSWITCH_TIMEOUT_VALUE  (5000U) /* 5 s                       */

#define PLLSOURCE_NONE             (0U)
#define MEGA_HZ                     1000000U /* Division factor to convert Hz in Mhz */
/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/** @defgroup RCC_Private_Macros RCC Private Macros
  * @{
  */
#define __MCO1_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define MCO1_GPIO_PORT        GPIOA
#define MCO1_PIN              GPIO_PIN_8

#define __MCO2_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#define MCO2_GPIO_PORT        GPIOB
#define MCO2_PIN              GPIO_PIN_6

#define __MCO3_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()
#define MCO3_GPIO_PORT        GPIOA
#define MCO3_PIN              GPIO_PIN_15

#define RCC_PLL_OSCSOURCE_CONFIG(__HAL_RCC_PLLSOURCE__) \
            (MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC, (uint32_t)(__HAL_RCC_PLLSOURCE__)))

#define __COUNTOF(_A_)   (sizeof(_A_) / sizeof(*(_A_)))
/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/** @defgroup RCC_Private_Variables RCC Private Variables
  * @{
  */


/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCC_Private_Functions RCC Private Functions
  * @{
  */
static HAL_StatusTypeDef RCC_SetFlashLatencyFromMSIRange(uint32_t MSI_Range);
static HAL_StatusTypeDef RCC_SetFlashLatency(uint32_t Flash_ClkSrcFreq, uint32_t VCORE_Voltage);
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

/** @defgroup RCC_Exported_Functions RCC Exported Functions
  * @{
  */

/** @defgroup RCC_Exported_Functions_Group1 Initialization and de-initialization functions
  *  @brief    Initialization and Configuration functions
  *
  @verbatim
 ===============================================================================
           ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]
      This section provides functions allowing to configure the internal and external oscillators
      (HSE, HSI, LSE, MSI, LSI1, LSI2, PLL, CSS and MCO) and the System busses clocks (SYSCLK, HCLK1, HCLK2, HCLK4, PCLK1
       and PCLK2).

    [..] Internal/external clock and PLL configuration
         (+) HSI (high-speed internal): 16 MHz factory-trimmed RC used directly or through
             the PLL as System clock source.

         (+) MSI (Mutiple Speed Internal): Its frequency is software trimmable from 100KHZ to 48MHZ.
             It can be used to generate the clock for the USB FS (48 MHz).
             The number of flash wait states is automatically adjusted when MSI range is updated with
             @ref HAL_RCC_OscConfig() and the MSI is used as System clock source.

         (+) LSI1/LSI2 (low-speed internal): 32 KHz low consumption RC used as IWDG and/or RTC
             clock source.

         (+) HSE (high-speed external): 32 MHz crystal oscillator used directly or
             through the PLL as System clock source. Can be used also optionally as RTC clock source.

         (+) LSE (low-speed external): 32.768 KHz oscillator used optionally as RTC clock source
             or the RF system Auto-wakeup from Stop and Standby modes.

         (+) PLL (clocked by HSI, HSE or MSI) providing up to three independent output clocks:
           (++) The first output is used to generate the high speed system clock (up to 64MHz).
           (++) The second output is used to generate the clock for the USB FS (48 MHz),
                the random analog generator (<=48 MHz)
           (++) The third output is used to generate an accurate clock to achieve
                high-quality audio performance on SAI interface.

         (+) PLLSAI1 (clocked by HSI, HSE or MSI) providing up to three independent output clocks:
           (++) The first output is used to generate SAR ADC clock.
           (++) The second output is used to generate the clock for the USB FS (48 MHz),
                the random analog generator (<=48 MHz).
           (++) The Third output is used to generate an accurate clock to achieve
                high-quality audio performance on SAI interface.


         (+) CSS (Clock security system): once enabled, if a HSE clock failure occurs
            (HSE used directly or through PLL as System clock source), the System clock
             is automatically switched to MSI or the HSI oscillator (depending on the
             STOPWUCK configuration) and an interrupt is generated if enabled.
             The interrupt is linked to the CPU1 and CPU2 NMI (Non-Maskable Interrupt) exception vector.

         (+) LSECSS: once enabled, if a LSE clock failure occurs, the LSE
             clock is no longer supplied to the RTC but no hardware action is made to the registers. If the
             MSI was in PLL-mode, this mode is disabled.
             In Standby mode a wakeup is generated. In other modes an interrupt can be sent to wakeup
             the software

         (+) MCO (microcontroller clock output): used to output MSI, LSI1, LSI2, HSI, LSE, HSE (before and
             after stabilization), SYSCLK, HSI48 or main PLL clock (through a configurable prescaler) on PA8, PB6 & PA15 pins.

    [..] System, AHB and APB busses clocks configuration
         (+) Several clock sources can be used to drive the System clock (SYSCLK): MSI, HSI,
             HSE and main PLL.
             The AHB clock (HCLK1) is derived from System clock through configurable
             prescaler and used to clock the CPU, memory and peripherals mapped
             on AHB bus (DMA, GPIO...). APB1 (PCLK1) and APB2 (PCLK2) clocks are derived
             from AHB clock through configurable prescalers and used to clock
             the peripherals mapped on these busses. You can use
             "@ref HAL_RCC_GetSysClockFreq()" function to retrieve the frequencies of these clocks.
             The AHB4 clock (HCLK4) is derived from System clock through configurable
             prescaler and used to clock the FLASH

         -@- All the peripheral clocks are derived from the System clock (SYSCLK) except:

           (+@) SAI: the SAI clock can be derived either from a specific PLL (PLLSAI1) or (PLLSYS) or
                from an external clock mapped on the SAI_CKIN pin.
                You have to use @ref HAL_RCCEx_PeriphCLKConfig() function to configure this clock.
           (+@) RTC: the RTC clock can be derived either from the LSI, LSE or HSE clock
                divided by 32.
                You have to use @ref __HAL_RCC_RTC_ENABLE() and @ref HAL_RCCEx_PeriphCLKConfig() function
                to configure this clock.
           (+@) USB FS and RNG: USB FS requires a frequency equal to 48 MHz
                to work correctly, while RNG peripherals requires a frequency
                equal or lower than to 48 MHz. This clock is derived of the main PLL or PLLSAI1
                through PLLQ divider. You have to enable the peripheral clock and use
                @ref HAL_RCCEx_PeriphCLKConfig() function to configure this clock.
           (+@) IWDG clock which is always the LSI clock.


         (+) The maximum frequency of the SYSCLK, HCLK1, HCLK4, PCLK1 and PCLK2 is 64 MHz.
             The maximum frequency of the HCLK2 is 32 MHz.
             The clock source frequency should be adapted depending on the device voltage range
             as listed in the Reference Manual "Clock source frequency versus voltage scaling" chapter.

  @endverbatim

           Table 1. HCLK4 clock frequency.
           +-------------------------------------------------------+
           | Latency         |    HCLK4 clock frequency (MHz)      |
           |                 |-------------------------------------|
           |                 | voltage range 1  | voltage range 2  |
           |                 |      1.2 V       |     1.0 V        |
           |-----------------|------------------|------------------|
           |0WS(1 CPU cycles)|   HCLK4 <= 18    |   HCLK4 <= 6     |
           |-----------------|------------------|------------------|
           |1WS(2 CPU cycles)|   HCLK4 <= 36    |   HCLK4 <= 12    |
           |-----------------|------------------|------------------|
           |2WS(3 CPU cycles)|   HCLK4 <= 54    |   HCLK4 <= 16    |
           |-----------------|------------------|------------------|
           |3WS(4 CPU cycles)|   HCLK4 <= 64    |   HCLK4 <= n.a.  |
           |-----------------|------------------|------------------|

  * @{
  */

/**
  * @brief  Reset the RCC clock configuration to the default reset state.
  * @note   The default reset state of the clock configuration is given below:
  *            - MSI ON and used as system clock source
  *            - HSE, HSI, PLL, PLLSAI1
  *            - HCLK1, HCLK2, HCLK4, PCLK1 and PCLK2 prescalers set to 1.
  *            - CSS, MCO OFF
  *            - All interrupts disabled
  * @note   This function doesn't modify the configuration of the
  *            - Peripheral clocks
  *            - LSI, LSE and RTC clocks
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCC_DeInit(void)
{
  uint32_t tickstart;

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Set MSION bit */
  LL_RCC_MSI_Enable();

  /* Wait till MSI is ready */
  while (LL_RCC_MSI_IsReady() == 0U)
  {
    if ((HAL_GetTick() - tickstart) > MSI_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Set MSIRANGE default value */
  LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_6);

  /* Set MSITRIM bits to the reset value*/
  LL_RCC_MSI_SetCalibTrimming(0);

  /* Set HSITRIM bits to the reset value*/
  LL_RCC_HSI_SetCalibTrimming(0x40U);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Reset CFGR register (MSI is selected as system clock source) */
  CLEAR_REG(RCC->CFGR);

  /* Wait till MSI is ready */
  while (READ_BIT(RCC->CFGR, RCC_CFGR_SWS) != 0U)
  {
    if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Reset HSION, HSIKERON, HSIASFS, HSEON, PLLON, PLLSAI11ON, HSEPRE bits */
  CLEAR_BIT(RCC->CR, RCC_CR_HSION | RCC_CR_HSIKERON | RCC_CR_HSIASFS | RCC_CR_HSEON | RCC_CR_HSEPRE | RCC_CR_PLLON | RCC_CR_PLLSAI1ON);

  /* Reset HSEBYP bit once HSE is OFF */
  LL_RCC_HSE_DisableBypass();

  /* Get Start Tick*/
    tickstart = HAL_GetTick();

  /* Wait till PLL is ready */
  while (LL_RCC_PLL_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* once PLL is OFF, reset PLLCFGR register to default value */
  WRITE_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLR_0 | RCC_PLLCFGR_PLLQ_0 | RCC_PLLCFGR_PLLP_1 | RCC_PLLCFGR_PLLN_0);


  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Wait till PLL is ready */
  while (LL_RCC_PLLSAI1_IsReady() != 0U)
  {
    if ((HAL_GetTick() - tickstart) > PLLSAI1_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }
  /* once PLLSAI1 is OFF, reset PLLSAI1CFGR register to default value */
  WRITE_REG(RCC->PLLSAI1CFGR, RCC_PLLSAI1CFGR_PLLR_0 | RCC_PLLSAI1CFGR_PLLQ_0 | RCC_PLLSAI1CFGR_PLLP_1 | RCC_PLLSAI1CFGR_PLLN_0);

  /* Disable all interrupts */
  CLEAR_REG(RCC->CIER);

  /* Clear all interrupt flags */
  WRITE_REG(RCC->CICR, 0xFFFFFFFFU);

  /* EXTCFGR reset*/
  LL_RCC_WriteReg(EXTCFGR, 0x00030000U);

  /* Update the SystemCoreClock global variable */
  SystemCoreClock = MSI_VALUE;

  /* Adapt Systick interrupt period */
  if(HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
  {
    return HAL_ERROR;
  }
  else
  {
    return HAL_OK;
  }
}

/**
  * @brief  Initialize the RCC Oscillators according to the specified parameters in the
  *         @ref RCC_OscInitTypeDef.
  * @param  RCC_OscInitStruct  pointer to a @ref RCC_OscInitTypeDef structure that
  *         contains the configuration information for the RCC Oscillators.
  * @note   The PLL is not disabled when used as system clock.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{
  uint32_t tickstart;

  /* Check Null pointer */
  if(RCC_OscInitStruct == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_RCC_OSCILLATORTYPE(RCC_OscInitStruct->OscillatorType));

  /*----------------------------- MSI Configuration --------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_MSI) == RCC_OSCILLATORTYPE_MSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_MSI(RCC_OscInitStruct->MSIState));
    assert_param(IS_RCC_MSICALIBRATION_VALUE(RCC_OscInitStruct->MSICalibrationValue));
    assert_param(IS_RCC_MSI_CLOCK_RANGE(RCC_OscInitStruct->MSIClockRange));

    /* When the MSI is used as system clock it will not be disabled */
    const uint32_t temp_sysclksrc = __HAL_RCC_GET_SYSCLK_SOURCE();
    const uint32_t temp_plloscsrc = __HAL_RCC_GET_PLL_OSCSOURCE();
    if ((temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_MSI) ||
        ((temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_PLLCLK) && (temp_plloscsrc == RCC_PLLSOURCE_MSI)))
    {
      if ((LL_RCC_MSI_IsReady() != 0U) && (RCC_OscInitStruct->MSIState == RCC_MSI_OFF))
      {
        return HAL_ERROR;
      }
      /* Otherwise, just the calibration and MSI range change are allowed */
      else
      {
        /* To correctly read data from FLASH memory, the number of wait states (LATENCY)
           must be correctly programmed according to the frequency of the AHB4 clock
           and the supply voltage of the device. */
        if (RCC_OscInitStruct->MSIClockRange > __HAL_RCC_GET_MSI_RANGE())
        {
          /* First increase number of wait states update if necessary */
          if (RCC_SetFlashLatencyFromMSIRange(RCC_OscInitStruct->MSIClockRange) != HAL_OK)
          {
            return HAL_ERROR;
          }

          /* Selects the Multiple Speed oscillator (MSI) clock range .*/
          __HAL_RCC_MSI_RANGE_CONFIG(RCC_OscInitStruct->MSIClockRange);
          /* Adjusts the Multiple Speed oscillator (MSI) calibration value.*/
          __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->MSICalibrationValue);
        }
        else
        {
          /* Else, keep current flash latency while decreasing applies */
          /* Selects the Multiple Speed oscillator (MSI) clock range .*/
          __HAL_RCC_MSI_RANGE_CONFIG(RCC_OscInitStruct->MSIClockRange);
          /* Adjusts the Multiple Speed oscillator (MSI) calibration value.*/
          __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->MSICalibrationValue);

          /* Decrease number of wait states update if necessary */
          if (RCC_SetFlashLatencyFromMSIRange(RCC_OscInitStruct->MSIClockRange) != HAL_OK)
          {
            return HAL_ERROR;
          }
        }

        /* Update the SystemCoreClock global variable */
        SystemCoreClockUpdate();
        if(HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK)
        {
          return HAL_ERROR;
        }
      }
    }
    else
    {
      /* Check the MSI State */
      if (RCC_OscInitStruct->MSIState != RCC_MSI_OFF)
      {
        /* Enable the Internal High Speed oscillator (MSI). */
        __HAL_RCC_MSI_ENABLE();

        /* Get timeout */
        tickstart = HAL_GetTick();

        /* Wait till MSI is ready */
        while (LL_RCC_MSI_IsReady() == 0U)
        {
          if ((HAL_GetTick() - tickstart) > MSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /* Selects the Multiple Speed oscillator (MSI) clock range .*/
        __HAL_RCC_MSI_RANGE_CONFIG(RCC_OscInitStruct->MSIClockRange);
        /* Adjusts the Multiple Speed oscillator (MSI) calibration value.*/
        __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->MSICalibrationValue);

      }
      else
      {
        /* Disable the Internal High Speed oscillator (MSI). */
        __HAL_RCC_MSI_DISABLE();

        /* Get timeout */
        tickstart = HAL_GetTick();

        /* Wait till MSI is ready */
        while (LL_RCC_MSI_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > MSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*------------------------------- HSE Configuration ------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSE(RCC_OscInitStruct->HSEState));

    /* When the HSE is used as system clock or clock source for PLL in these cases it is not allowed to be disabled */
    const uint32_t temp_sysclksrc = __HAL_RCC_GET_SYSCLK_SOURCE();
    const uint32_t temp_plloscsrc = __HAL_RCC_GET_PLL_OSCSOURCE();
    if ((temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_HSE) ||
        ((temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_PLLCLK) && (temp_plloscsrc == RCC_PLLSOURCE_HSE)))
    {
      if ((LL_RCC_HSE_IsReady() != 0U) && (RCC_OscInitStruct->HSEState == RCC_HSE_OFF))
      {
        return HAL_ERROR;
      }
    }
    else
    {
      /* Set the new HSE configuration ---------------------------------------*/
      __HAL_RCC_HSE_CONFIG(RCC_OscInitStruct->HSEState);

      /* Check the HSE State */
      if (RCC_OscInitStruct->HSEState != RCC_HSE_OFF)
      {
        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSE is ready */
        while (LL_RCC_HSE_IsReady() == 0U)
        {
          if ((HAL_GetTick() - tickstart) > HSE_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
      else
      {
        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSE is disabled */
        while (LL_RCC_HSE_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > HSE_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*----------------------------- HSI Configuration --------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSI(RCC_OscInitStruct->HSIState));
    assert_param(IS_RCC_HSI_CALIBRATION_VALUE(RCC_OscInitStruct->HSICalibrationValue));

    /* Check if HSI is used as system clock or as PLL source when PLL is selected as system clock */
    const uint32_t temp_sysclksrc = __HAL_RCC_GET_SYSCLK_SOURCE();
    const uint32_t temp_plloscsrc = __HAL_RCC_GET_PLL_OSCSOURCE();
    if ((temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_HSI) ||
        ((temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_PLLCLK) && (temp_plloscsrc == RCC_PLLSOURCE_HSI)))
    {
      /* When HSI is used as system clock it will not be disabled */
      if ((LL_RCC_HSI_IsReady() != 0U) && (RCC_OscInitStruct->HSIState == RCC_HSI_OFF))
      {
        return HAL_ERROR;
      }
      /* Otherwise, just the calibration is allowed */
      else
      {
        /* Adjusts the Internal High Speed oscillator (HSI) calibration value.*/
        __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->HSICalibrationValue);
      }
    }
    else
    {
      /* Check the HSI State */
      if (RCC_OscInitStruct->HSIState != RCC_HSI_OFF)
      {
        /* Enable the Internal High Speed oscillator (HSI). */
        __HAL_RCC_HSI_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSI is ready */
        while (LL_RCC_HSI_IsReady() == 0U)
        {
          if ((HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Adjusts the Internal High Speed oscillator (HSI) calibration value.*/
        __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->HSICalibrationValue);
      }
      else
      {
        /* Disable the Internal High Speed oscillator (HSI). */
        __HAL_RCC_HSI_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till HSI is disabled */
        while (LL_RCC_HSI_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > HSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*------------------------------ LSI Configuration (LSI1 or LSI2) -------------------------*/

  if ((((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSI1) == RCC_OSCILLATORTYPE_LSI1) || \
      (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSI2) == RCC_OSCILLATORTYPE_LSI2))
  {
    /* Check the parameters */
    assert_param(IS_RCC_LSI(RCC_OscInitStruct->LSIState));

    /* Check the LSI State */
    if (RCC_OscInitStruct->LSIState != RCC_LSI_OFF)
    {
      /*------------------------------ LSI2 selected by default (when Switch ON) -------------------------*/
      if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSI2) == RCC_OSCILLATORTYPE_LSI2)
      {
        assert_param(IS_RCC_LSI2_CALIBRATION_VALUE(RCC_OscInitStruct->LSI2CalibrationValue));

        /* 1. Check LSI1 state and enable if required */
        if (LL_RCC_LSI1_IsReady() == 0U)
        {
         /* This is required to enable LSI1 before enabling LSI2 */
          __HAL_RCC_LSI1_ENABLE();

          /* Get Start Tick*/
          tickstart = HAL_GetTick();

          /* Wait till LSI1 is ready */
          while(LL_RCC_LSI1_IsReady() == 0U)
          {
            if((HAL_GetTick() - tickstart) > LSI1_TIMEOUT_VALUE)
            {
              return HAL_TIMEOUT;
            }
          }
        }

        /* 2. Enable the Internal Low Speed oscillator (LSI2) and set trimming value */
        __HAL_RCC_LSI2_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till LSI2 is ready */
        while (LL_RCC_LSI2_IsReady() == 0U)
        {
          if ((HAL_GetTick() - tickstart) > LSI2_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /* Adjusts the Internal Low Spee oscillator (LSI2) calibration value */
        __HAL_RCC_LSI2_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->LSI2CalibrationValue);

        /* 3. Disable LSI1 */

        /* LSI1 was initially not enable, require to disable it */
        __HAL_RCC_LSI1_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till LSI1 is disabled */
        while (LL_RCC_LSI1_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > LSI1_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
      else
      {
        /*------------------------------ LSI1 selected (only if LSI2 OFF)-------------------------*/

        /* 1. Enable the Internal Low Speed oscillator (LSI1). */
        __HAL_RCC_LSI1_ENABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till LSI1 is ready */
        while (LL_RCC_LSI1_IsReady() == 0U)
        {
          if ((HAL_GetTick() - tickstart) > LSI1_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /*2. Switch OFF LSI2*/

        /* Disable the Internal Low Speed oscillator (LSI2). */
        __HAL_RCC_LSI2_DISABLE();

        /* Wait till LSI2 is disabled */
        while (LL_RCC_LSI2_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > LSI2_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
    else
    {

      /* Disable the Internal Low Speed oscillator (LSI2). */
      __HAL_RCC_LSI2_DISABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till LSI2 is disabled */
      while (LL_RCC_LSI2_IsReady() != 0U)
      {
        if ((HAL_GetTick() - tickstart) > LSI2_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }

      /* Disable the Internal Low Speed oscillator (LSI1). */
      __HAL_RCC_LSI1_DISABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till LSI1 is disabled */
      while (LL_RCC_LSI1_IsReady() != 0U)
      {
        if ((HAL_GetTick() - tickstart) > LSI1_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
  }
  /*------------------------------ LSE Configuration -------------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE)
  {

    /* Check the parameters */
    assert_param(IS_RCC_LSE(RCC_OscInitStruct->LSEState));

    /* Update LSE configuration in Backup Domain control register    */
    /* Requires to enable write access to Backup Domain of necessary */

    if (HAL_IS_BIT_CLR(PWR->CR1, PWR_CR1_DBP))
    {
      /* Enable write access to Backup domain */
      HAL_PWR_EnableBkUpAccess();

      /* Wait for Backup domain Write protection disable */
      tickstart = HAL_GetTick();

      while (HAL_IS_BIT_CLR(PWR->CR1, PWR_CR1_DBP))
      {
        if ((HAL_GetTick() - tickstart) > RCC_DBP_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }

    /* Set the new LSE configuration -----------------------------------------*/
    __HAL_RCC_LSE_CONFIG(RCC_OscInitStruct->LSEState);

    /* Check the LSE State */
    if (RCC_OscInitStruct->LSEState != RCC_LSE_OFF)
    {
      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till LSE is ready */
      while (LL_RCC_LSE_IsReady() == 0U)
      {
        if ((HAL_GetTick() - tickstart) > RCC_LSE_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    else
    {
      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till LSE is disabled */
      while (LL_RCC_LSE_IsReady() != 0U)
      {
        if ((HAL_GetTick() - tickstart) > RCC_LSE_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }

  }
  /*------------------------------ HSI48 Configuration -----------------------*/
  if (((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSI48) == RCC_OSCILLATORTYPE_HSI48)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSI48(RCC_OscInitStruct->HSI48State));

    /* Check the LSI State */
    if (RCC_OscInitStruct->HSI48State != RCC_HSI48_OFF)
    {
      /* Enable the Internal Low Speed oscillator (HSI48). */
      __HAL_RCC_HSI48_ENABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till HSI48 is ready */
      while (LL_RCC_HSI48_IsReady() == 0U)
      {
        if ((HAL_GetTick() - tickstart) > HSI48_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    else
    {
      /* Disable the Internal Low Speed oscillator (HSI48). */
      __HAL_RCC_HSI48_DISABLE();

      /* Get Start Tick*/
      tickstart = HAL_GetTick();

      /* Wait till HSI48 is disabled */
      while (LL_RCC_HSI48_IsReady() != 0U)
      {
        if ((HAL_GetTick() - tickstart) > HSI48_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
  }
  /*-------------------------------- PLL Configuration -----------------------*/
  /* Check the parameters */
  assert_param(IS_RCC_PLL(RCC_OscInitStruct->PLL.PLLState));

  if (RCC_OscInitStruct->PLL.PLLState != RCC_PLL_NONE)
  {
    /* Check if the PLL is used as system clock or not */
    if (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK)
    {
      if (RCC_OscInitStruct->PLL.PLLState == RCC_PLL_ON)
      {
        /* Check the parameters */
        assert_param(IS_RCC_PLLSOURCE(RCC_OscInitStruct->PLL.PLLSource));
        assert_param(IS_RCC_PLLM_VALUE(RCC_OscInitStruct->PLL.PLLM));
        assert_param(IS_RCC_PLLN_VALUE(RCC_OscInitStruct->PLL.PLLN));
        assert_param(IS_RCC_PLLP_VALUE(RCC_OscInitStruct->PLL.PLLP));
        assert_param(IS_RCC_PLLQ_VALUE(RCC_OscInitStruct->PLL.PLLQ));
        assert_param(IS_RCC_PLLR_VALUE(RCC_OscInitStruct->PLL.PLLR));

        /* Disable the main PLL. */
        __HAL_RCC_PLL_DISABLE();

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (LL_RCC_PLL_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Configure the main PLL clock source, multiplication and division factors. */
        __HAL_RCC_PLL_CONFIG(RCC_OscInitStruct->PLL.PLLSource,
                             RCC_OscInitStruct->PLL.PLLM,
                             RCC_OscInitStruct->PLL.PLLN,
                             RCC_OscInitStruct->PLL.PLLP,
                             RCC_OscInitStruct->PLL.PLLQ,
                             RCC_OscInitStruct->PLL.PLLR);

        /* Enable the main PLL. */
        __HAL_RCC_PLL_ENABLE();

        /* Enable PLL System Clock output. */
        __HAL_RCC_PLLCLKOUT_ENABLE(RCC_PLL_SYSCLK);

        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is ready */
        while (LL_RCC_PLL_IsReady() == 0U)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
      else
      {
        /* Disable the main PLL. */
        __HAL_RCC_PLL_DISABLE();

        /* Disable all PLL outputs to save power */
        MODIFY_REG(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC, PLLSOURCE_NONE);

        __HAL_RCC_PLLCLKOUT_DISABLE(RCC_PLL_SYSCLK | RCC_PLL_USBCLK | RCC_PLL_SAI1CLK);


        /* Get Start Tick*/
        tickstart = HAL_GetTick();

        /* Wait till PLL is disabled */
        while (LL_RCC_PLL_IsReady() != 0U)
        {
          if ((HAL_GetTick() - tickstart) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
    else
    {
      /* Do not return HAL_ERROR if request repeats the current configuration */
      uint32_t pllcfgr = RCC->PLLCFGR;

      if((READ_BIT(pllcfgr, RCC_PLLCFGR_PLLSRC) != RCC_OscInitStruct->PLL.PLLSource) ||
         (READ_BIT(pllcfgr, RCC_PLLCFGR_PLLM) != RCC_OscInitStruct->PLL.PLLM) ||
         ((READ_BIT(pllcfgr, RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos) != RCC_OscInitStruct->PLL.PLLN) ||
         (READ_BIT(pllcfgr, RCC_PLLCFGR_PLLP) != RCC_OscInitStruct->PLL.PLLP) ||
         (READ_BIT(pllcfgr, RCC_PLLCFGR_PLLQ) != RCC_OscInitStruct->PLL.PLLQ) ||
         (READ_BIT(pllcfgr, RCC_PLLCFGR_PLLR) != RCC_OscInitStruct->PLL.PLLR))
      {
        return HAL_ERROR;
      }
    }
  }
  return HAL_OK;
}


/**
  * @brief  Initialize the CPU, AHB and APB busses clocks according to the specified
  *         parameters in the RCC_ClkInitStruct.
  * @param  RCC_ClkInitStruct  pointer to a @ref RCC_ClkInitTypeDef structure that
  *         contains the configuration information for the RCC peripheral.
  * @param  FLatency  FLASH Latency
  *          This parameter can be one of the following values:
  *            @arg FLASH_LATENCY_0   FLASH 0 Latency cycle
  *            @arg FLASH_LATENCY_1   FLASH 1 Latency cycle
  *            @arg FLASH_LATENCY_2   FLASH 2 Latency cycle
  *            @arg FLASH_LATENCY_3   FLASH 3 Latency cycle
  *
  * @note   The SystemCoreClock CMSIS variable is used to store System Clock Frequency
  *         and updated by @ref HAL_RCC_GetHCLKFreq() function called within this function
  *
  * @note   The MSI is used by default as system clock source after
  *         startup from Reset, wake-up from STANDBY mode. After restart from Reset,
  *         the MSI frequency is set to its default value 4 MHz.
  *
  * @note   The HSI can be selected as system clock source after
  *         from STOP modes or in case of failure of the HSE used directly or indirectly
  *         as system clock (if the Clock Security System CSS is enabled).
  *
  * @note   A switch from one clock source to another occurs only if the target
  *         clock source is ready (clock stable after startup delay or PLL locked).
  *         If a clock source which is not yet ready is selected, the switch will
  *         occur when the clock source is ready.
  *
  * @note   You can use @ref HAL_RCC_GetClockConfig() function to know which clock is
  *         currently used as system clock source.
  *
  * @note   Depending on the device voltage range, the software has to set correctly
  *         HPRE[3:0] bits to ensure that HCLK1 not exceed the maximum allowed frequency
  *         (for more details refer to section above "Initialization/de-initialization functions")
  * @retval None
  */
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t FLatency)
{
  uint32_t tickstart;

  /* Check Null pointer */
  if(RCC_ClkInitStruct == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_RCC_CLOCKTYPE(RCC_ClkInitStruct->ClockType));
  assert_param(IS_FLASH_LATENCY(FLatency));

  /* To correctly read data from FLASH memory, the number of wait states (LATENCY)
    must be correctly programmed according to the frequency of the FLASH clock
    (HCLK4) and the supply voltage of the device. */

  /* Increasing the number of wait states because of higher CPU frequency */
  if (FLatency > __HAL_FLASH_GET_LATENCY())
  {
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
    __HAL_FLASH_SET_LATENCY(FLatency);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Check that the new number of wait states is taken into account to access the Flash
       memory by reading the FLASH_ACR register */
    while (__HAL_FLASH_GET_LATENCY() != FLatency)
    {
      if ((HAL_GetTick() - tickstart) > LATENCY_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*-------------------------- HCLK1 Configuration --------------------------*/
  if (((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_HCLK) == RCC_CLOCKTYPE_HCLK)
  {
    assert_param(IS_RCC_HCLKx(RCC_ClkInitStruct->AHBCLKDivider));
    LL_RCC_SetAHBPrescaler(RCC_ClkInitStruct->AHBCLKDivider);

    /* HCLK1 prescaler flag when value applied */
    tickstart = HAL_GetTick();
    while (LL_RCC_IsActiveFlag_HPRE() == 0U)
    {
      if ((HAL_GetTick() - tickstart) > PRESCALER_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*-------------------------- HCLK2 Configuration --------------------------*/
  if (((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_HCLK2) == RCC_CLOCKTYPE_HCLK2)
  {
    assert_param(IS_RCC_HCLKx(RCC_ClkInitStruct->AHBCLK2Divider));
    LL_C2_RCC_SetAHBPrescaler(RCC_ClkInitStruct->AHBCLK2Divider);

    /* HCLK2 prescaler flag when value applied */
    tickstart = HAL_GetTick();
    while (LL_RCC_IsActiveFlag_C2HPRE() == 0U)
    {
      if ((HAL_GetTick() - tickstart) > PRESCALER_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }
  /*-------------------------- HCLK4 Configuration --------------------------*/
  if (((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_HCLK4) == RCC_CLOCKTYPE_HCLK4)
  {
    assert_param(IS_RCC_HCLKx(RCC_ClkInitStruct->AHBCLK4Divider));
    LL_RCC_SetAHB4Prescaler(RCC_ClkInitStruct->AHBCLK4Divider);

    /* AHB shared prescaler flag when value applied */
    tickstart = HAL_GetTick();
    while (LL_RCC_IsActiveFlag_SHDHPRE() == 0U)
    {
      if ((HAL_GetTick() - tickstart) > PRESCALER_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*-------------------------- PCLK1 Configuration ---------------------------*/
  if (((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_PCLK1) == RCC_CLOCKTYPE_PCLK1)
  {
    assert_param(IS_RCC_PCLKx(RCC_ClkInitStruct->APB1CLKDivider));
    LL_RCC_SetAPB1Prescaler(RCC_ClkInitStruct->APB1CLKDivider);

    /* APB1 prescaler flag when value applied */
    tickstart = HAL_GetTick();
    while (LL_RCC_IsActiveFlag_PPRE1() == 0U)
    {
      if ((HAL_GetTick() - tickstart) > PRESCALER_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*-------------------------- PCLK2 Configuration ---------------------------*/
  if (((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_PCLK2) == RCC_CLOCKTYPE_PCLK2)
  {
    assert_param(IS_RCC_PCLKx(RCC_ClkInitStruct->APB2CLKDivider));
    LL_RCC_SetAPB2Prescaler((RCC_ClkInitStruct->APB2CLKDivider) << 3U);

    /* APB2 prescaler flag when value applied */
    tickstart = HAL_GetTick();
    while (LL_RCC_IsActiveFlag_PPRE2() == 0U)
    {
      if ((HAL_GetTick() - tickstart) > PRESCALER_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*------------------------- SYSCLK Configuration ---------------------------*/
  if (((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_SYSCLK) == RCC_CLOCKTYPE_SYSCLK)
  {
    assert_param(IS_RCC_SYSCLKSOURCE(RCC_ClkInitStruct->SYSCLKSource));

    /* HSE is selected as System Clock Source */
    if (RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_HSE)
    {
      /* Check the HSE ready flag */
      if (LL_RCC_HSE_IsReady() == 0U)
      {
        return HAL_ERROR;
      }
    }
    /* PLL is selected as System Clock Source */
    else if (RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK)
    {
      /* Check the PLL ready flag */
      if (LL_RCC_PLL_IsReady() == 0U)
      {
        return HAL_ERROR;
      }
    }
    /* MSI is selected as System Clock Source */
    else if (RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_MSI)
    {
      /* Check the MSI ready flag */
      if (LL_RCC_MSI_IsReady() == 0U)
      {
        return HAL_ERROR;
      }
    }
    /* HSI is selected as System Clock Source */
    else
    {
      /* Check the HSI ready flag */
      if (LL_RCC_HSI_IsReady() == 0U)
      {
        return HAL_ERROR;
      }

    }

    /* apply system clock switch */
    LL_RCC_SetSysClkSource(RCC_ClkInitStruct->SYSCLKSource);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* check system clock source switch status */
    while (__HAL_RCC_GET_SYSCLK_SOURCE() != (RCC_ClkInitStruct->SYSCLKSource << RCC_CFGR_SWS_Pos))
    {
      if ((HAL_GetTick() - tickstart) > CLOCKSWITCH_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /* Decreasing the number of wait states because of lower CPU frequency */
  if (FLatency < __HAL_FLASH_GET_LATENCY())
  {
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
    __HAL_FLASH_SET_LATENCY(FLatency);

    /* Get Start Tick*/
    tickstart = HAL_GetTick();

    /* Check that the new number of wait states is taken into account to access the Flash
    memory by reading the FLASH_ACR register */
    while (__HAL_FLASH_GET_LATENCY() != FLatency)
    {
      if ((HAL_GetTick() - tickstart) > LATENCY_TIMEOUT_VALUE)
      {
        return HAL_TIMEOUT;
      }
    }
  }

  /*---------------------------------------------------------------------------*/

  /* Update the SystemCoreClock global variable */
  SystemCoreClockUpdate();
  /* Configure the source of time base considering new system clocks settings*/
  return HAL_InitTick (HAL_GetTickPrio());
}

/**
  * @}
  */

/** @defgroup RCC_Exported_Functions_Group2 Peripheral Control functions
 *  @brief   RCC clocks control functions
 *
@verbatim
 ===============================================================================
                      ##### Peripheral Control functions #####
 ===============================================================================
    [..]
    This subsection provides a set of functions allowing to:

    (+) Ouput clock to MCO pin.
    (+) Retrieve current clock frequencies.
    (+) Enable the Clock Security System.

@endverbatim
  * @{
  */

/**
  * @brief  Select the clock source to output on MCO1 pin(PA8) or MC02 pin (PB6) or MCO3 pin (PA15).
  * @note   PA8, PB6 or PA15 should be configured in alternate function mode.
  * @param  RCC_MCOx  specifies the output direction for the clock source.
  *            @arg @ref RCC_MCO1  Clock source to output on MCO1 pin(PA8)
  *            @arg @ref RCC_MCO2  Clock source to output on MCO2 pin(PB6)
  *            @arg @ref RCC_MCO3  Clock source to output on MCO3 pin(PA15)
  * @param  RCC_MCOSource  specifies the clock source to output.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1SOURCE_NOCLOCK  MCO output disabled, no clock on MCO
  *            @arg @ref RCC_MCO1SOURCE_SYSCLK  system  clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_MSI  MSI clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_HSI  HSI clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_HSE  HSE clock selected as MCO sourcee
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK  main PLL clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_LSI1  LSI1 clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_LSI2  LSI2 clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_LSE  LSE clock selected as MCO source
  *            @arg @ref RCC_MCO1SOURCE_HSI48  HSI48 clock selected as MCO source for devices with HSI48
  *            @arg @ref RCC_MCO1SOURCE_HSE_BEFORE_STAB  HSE clock before stabilization selected as MCO source
  * @param  RCC_MCODiv  specifies the MCO prescaler.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCODIV_1  no division applied to MCO clock
  *            @arg @ref RCC_MCODIV_2  division by 2 applied to MCO clock
  *            @arg @ref RCC_MCODIV_4  division by 4 applied to MCO clock
  *            @arg @ref RCC_MCODIV_8  division by 8 applied to MCO clock
  *            @arg @ref RCC_MCODIV_16  division by 16 applied to MCO clock
  * @retval None
  */
void HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Check the parameters */
  assert_param(IS_RCC_MCO(RCC_MCOx));
  assert_param(IS_RCC_MCODIV(RCC_MCODiv));
  assert_param(IS_RCC_MCO1SOURCE(RCC_MCOSource));

  /* Common GPIO init parameters */
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;

  /* RCC_MCO1 */
  if(RCC_MCOx == RCC_MCO1)
  {
    /* MCO1 Clock Enable */
    __MCO1_CLK_ENABLE();
    /* Configue the MCO1 pin in alternate function mode */
    GPIO_InitStruct.Pin       = MCO1_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
    HAL_GPIO_Init(MCO1_GPIO_PORT, &GPIO_InitStruct);

  }
  else if (RCC_MCOx == RCC_MCO2)
  {
    /* MCO2 Clock Enable */
    __MCO2_CLK_ENABLE();
    /* Configue the MCO2 pin in alternate function mode */
    GPIO_InitStruct.Pin       = MCO2_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
    HAL_GPIO_Init(MCO2_GPIO_PORT, &GPIO_InitStruct);

  }
  else
  {
    /* MCO3 Clock Enable */
    __MCO3_CLK_ENABLE();
    /* Configue the MCO3 pin in alternate function mode */
    GPIO_InitStruct.Pin       = MCO3_PIN;
    GPIO_InitStruct.Alternate = GPIO_AF6_MCO;
    HAL_GPIO_Init(MCO3_GPIO_PORT, &GPIO_InitStruct);
  }

    /* Mask MCOSEL[] and MCOPRE[] bits then set MCO clock source and prescaler */
    LL_RCC_ConfigMCO(RCC_MCOSource, RCC_MCODiv);
}

/**
  * @brief  Return the SYSCLK frequency.
  *
  * @note   The system  computed by this function is not the real
  *         frequency in the chip. It is calculated based on the predefined
  *         constant and the selected clock source:
  * @note     If SYSCLK source is MSI, function returns values based on MSI range
  * @note     If SYSCLK source is HSI, function returns values based on HSI_VALUE(*)
  * @note     If SYSCLK source is HSE, function returns values based on HSE_VALUE(**)
  * @note     If SYSCLK source is PLL, function returns values based on HSE_VALUE(**),
  *           HSI_VALUE(*) or MSI Value multiplied/divided by the PLL factors.
  * @note     (*) HSI_VALUE is a constant defined in stm32wbxx_hal_conf.h file (default value
  *               16 MHz) but the real value may vary depending on the variations
  *               in voltage and temperature.
  * @note     (**) HSE_VALUE is a constant defined in stm32wbxx_hal_conf.h file (default value
  *                32 MHz), user has to ensure that HSE_VALUE is same as the real
  *                frequency of the crystal used. Otherwise, this function may
  *                have wrong result.
  *
  * @note   The result of this function could be not correct when using fractional
  *         value for HSE crystal.
  *
  * @note   This function can be used by the user application to compute the
  *         baudrate for the communication peripherals or configure other parameters.
  *
  * @note   Each time SYSCLK changes, this function must be called to update the
  *         right SYSCLK value. Otherwise, any configuration based on this function will be incorrect.
  *
  *
  * @retval SYSCLK frequency
  */
uint32_t HAL_RCC_GetSysClockFreq(void)
{
  uint32_t pllsource;
  uint32_t sysclockfreq, pllinputfreq;
  const uint32_t temp_sysclksrc = __HAL_RCC_GET_SYSCLK_SOURCE();

  if (temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_MSI)
  {
    /* Retrieve MSI frequency range in HZ*/
    /* MSI used as system clock source */
    sysclockfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
  }
  else if (temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_HSI)
  {
    /* HSI used as system clock source */
    sysclockfreq = HSI_VALUE;
  }
  else if (temp_sysclksrc == RCC_SYSCLKSOURCE_STATUS_HSE)
  {
    /* HSE used as system clock source */
    if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
     {
       sysclockfreq = HSE_VALUE / 2U;
     }
     else
     {
       sysclockfreq = HSE_VALUE;
     }
  }
  else
  {
    /* PLL used as system clock  source */
    pllsource = LL_RCC_PLL_GetMainSource();
    switch (pllsource)
    {
      case RCC_PLLSOURCE_HSI:  /* HSI used as PLL clock source */
        pllinputfreq = HSI_VALUE;
        break;
      case RCC_PLLSOURCE_HSE:  /* HSE used as PLL clock source */
      if (LL_RCC_HSE_IsEnabledDiv2() == 1U)
      {
        pllinputfreq = HSE_VALUE / 2U;
      }
      else
      {
        pllinputfreq = HSE_VALUE;
      }
        break;
      case RCC_PLLSOURCE_MSI:  /* MSI used as PLL clock source */
      default:
        pllinputfreq = __LL_RCC_CALC_MSI_FREQ(LL_RCC_MSI_GetRange());
        break;
    }
    sysclockfreq = __LL_RCC_CALC_PLLCLK_FREQ(pllinputfreq, LL_RCC_PLL_GetDivider(), LL_RCC_PLL_GetN(), LL_RCC_PLL_GetR());
  }

  return sysclockfreq;
}

/**
  * @brief  Return the HCLK frequency.
  * @retval HCLK frequency in Hz
  */
uint32_t HAL_RCC_GetHCLKFreq(void)
{
  /* Get SysClock and Compute HCLK1 frequency ---------------------------*/
  return ((uint32_t)(__LL_RCC_CALC_HCLK1_FREQ(HAL_RCC_GetSysClockFreq(), LL_RCC_GetAHBPrescaler())));
}

/**
  * @brief  Return the HCLK2 frequency.
  * @retval HCLK2 frequency in Hz
  */
uint32_t HAL_RCC_GetHCLK2Freq(void)
{
  /* Get SysClock and Compute HCLK2 frequency ---------------------------*/
  return ((uint32_t)(__LL_RCC_CALC_HCLK2_FREQ(HAL_RCC_GetSysClockFreq(), LL_C2_RCC_GetAHBPrescaler())));
}

/**
  * @brief  Return the HCLK4 frequency.
  * @retval HCLK4 frequency in Hz
  */
uint32_t HAL_RCC_GetHCLK4Freq(void)
{
  /* Get SysClock and Compute AHB4 frequency ---------------------------*/
  return ((uint32_t)(__LL_RCC_CALC_HCLK4_FREQ(HAL_RCC_GetSysClockFreq(), LL_RCC_GetAHB4Prescaler())));
}

/**
  * @brief  Return the PCLK1 frequency.
  * @note   Each time PCLK1 changes, this function must be called to update the
  *         right PCLK1 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK1 frequency in Hz
  */
uint32_t HAL_RCC_GetPCLK1Freq(void)
{
  /* Get HCLK source and Compute PCLK1 frequency ---------------------------*/
  return ((uint32_t)(__LL_RCC_CALC_PCLK1_FREQ(HAL_RCC_GetHCLKFreq(), LL_RCC_GetAPB1Prescaler())));
}

/**
  * @brief  Return the PCLK2 frequency.
  * @note   Each time PCLK2 changes, this function must be called to update the
  *         right PCLK2 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK2 frequency in Hz
  */
uint32_t HAL_RCC_GetPCLK2Freq(void)
{
  /* Get HCLK source and Compute PCLK2 frequency ---------------------------*/
  return ((uint32_t)(__LL_RCC_CALC_PCLK2_FREQ(HAL_RCC_GetHCLKFreq(), LL_RCC_GetAPB2Prescaler())));
}

/**
  * @brief  Configure the RCC_OscInitStruct according to the internal
  *         RCC configuration registers.
  * @param  RCC_OscInitStruct  pointer to an RCC_OscInitTypeDef structure that
  *         will be configured.
  * @retval None
  */
void HAL_RCC_GetOscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{
  /* Check the parameters */
  assert_param(RCC_OscInitStruct != (void *)NULL);

  /* Set all possible values for the Oscillator type parameter ---------------*/
  RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_MSI | \
                                      RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_LSI1 | RCC_OSCILLATORTYPE_LSI2 | RCC_OSCILLATORTYPE_HSI48;


  /* Get the HSE configuration -----------------------------------------------*/
  if ((RCC->CR & RCC_CR_HSEBYP) == RCC_CR_HSEBYP)
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_BYPASS;
  }
  else if ((RCC->CR & RCC_CR_HSEON) == RCC_CR_HSEON)
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
  }
  else
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_OFF;
  }

  /* Get the MSI configuration -----------------------------------------------*/
  if ((RCC->CR & RCC_CR_MSION) == RCC_CR_MSION)
  {
    RCC_OscInitStruct->MSIState = RCC_MSI_ON;
  }
  else
  {
    RCC_OscInitStruct->MSIState = RCC_MSI_OFF;
  }
  RCC_OscInitStruct->MSICalibrationValue  = LL_RCC_MSI_GetCalibTrimming();
  RCC_OscInitStruct->MSIClockRange        = LL_RCC_MSI_GetRange();

  /* Get the HSI configuration -----------------------------------------------*/
  if ((RCC->CR & RCC_CR_HSION) == RCC_CR_HSION)
  {
    RCC_OscInitStruct->HSIState = RCC_HSI_ON;
  }
  else
  {
    RCC_OscInitStruct->HSIState = RCC_HSI_OFF;
  }

  RCC_OscInitStruct->HSICalibrationValue = LL_RCC_HSI_GetCalibTrimming();

  /* Get the LSE configuration -----------------------------------------------*/
  if ((RCC->BDCR & RCC_BDCR_LSEBYP) == RCC_BDCR_LSEBYP)
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_BYPASS;
  }
  else if ((RCC->BDCR & RCC_BDCR_LSEON) == RCC_BDCR_LSEON)
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_ON;
  }
  else
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_OFF;
  }

  /* Get the LSI configuration -----------------------------------------------*/
  const uint32_t temp_lsi1on = (RCC->CSR & RCC_CSR_LSI1ON);
  const uint32_t temp_lsi2on = (RCC->CSR & RCC_CSR_LSI2ON);
  if ((temp_lsi1on == RCC_CSR_LSI1ON) || (temp_lsi2on == RCC_CSR_LSI2ON))
  {
    RCC_OscInitStruct->LSIState = RCC_LSI_ON;
  }
  else
  {
    RCC_OscInitStruct->LSIState = RCC_LSI_OFF;
  }

  /* Get the HSI48 configuration ---------------------------------------------*/
  if ((RCC->CRRCR & RCC_CRRCR_HSI48ON) == RCC_CRRCR_HSI48ON)
  {
    RCC_OscInitStruct->HSI48State = RCC_HSI48_ON;
  }
  else
  {
    RCC_OscInitStruct->HSI48State = RCC_HSI48_OFF;
  }


  /* Get the PLL configuration -----------------------------------------------*/
  if ((RCC->CR & RCC_CR_PLLON) == RCC_CR_PLLON)
  {
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
  }
  else
  {
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_OFF;
  }
  RCC_OscInitStruct->PLL.PLLSource = LL_RCC_PLL_GetMainSource();
  RCC_OscInitStruct->PLL.PLLM      = LL_RCC_PLL_GetDivider();
  RCC_OscInitStruct->PLL.PLLN      = LL_RCC_PLL_GetN();
  RCC_OscInitStruct->PLL.PLLP      = LL_RCC_PLL_GetP();
  RCC_OscInitStruct->PLL.PLLQ      = LL_RCC_PLL_GetQ();
  RCC_OscInitStruct->PLL.PLLR      = LL_RCC_PLL_GetR();
}

/**
  * @brief  Configure the RCC_ClkInitStruct according to the internal
  *         RCC configuration registers.
  * @param  RCC_ClkInitStruct Pointer to a @ref RCC_ClkInitTypeDef structure that
  *                           will be configured.
  * @param  pFLatency         Pointer on the Flash Latency.
  * @retval None
  */
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t *pFLatency)
{
  /* Check the parameters */
  assert_param(RCC_ClkInitStruct != (void *)NULL);
  assert_param(pFLatency != (void *)NULL);

  /* Set all possible values for the Clock type parameter --------------------*/
  RCC_ClkInitStruct->ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |  \
                                  RCC_CLOCKTYPE_HCLK2 | RCC_CLOCKTYPE_HCLK4);

  /* Get the SYSCLK configuration --------------------------------------------*/
  RCC_ClkInitStruct->SYSCLKSource = LL_RCC_GetSysClkSource();

  /* Get the HCLK configuration ----------------------------------------------*/
  RCC_ClkInitStruct->AHBCLKDivider = LL_RCC_GetAHBPrescaler();

  /* Get the APB1 configuration ----------------------------------------------*/
  RCC_ClkInitStruct->APB1CLKDivider = LL_RCC_GetAPB1Prescaler();

  /* Get the APB2 configuration ----------------------------------------------*/
  RCC_ClkInitStruct->APB2CLKDivider = LL_RCC_GetAPB2Prescaler();

  /* Get the AHBCLK2Divider configuration ------------------------------------*/
  RCC_ClkInitStruct->AHBCLK2Divider = LL_C2_RCC_GetAHBPrescaler();

  /* Get the AHBCLK4Divider configuration ------------------------------------*/
  RCC_ClkInitStruct->AHBCLK4Divider = LL_RCC_GetAHB4Prescaler();

  /* Get the Flash Wait State (Latency) configuration ------------------------*/
  *pFLatency = __HAL_FLASH_GET_LATENCY();
}

/**
  * @brief  Enable the Clock Security System.
  * @note   If a failure is detected on the HSE oscillator clock, this oscillator
  *         is automatically disabled and an interrupt is generated to inform the
  *         software about the failure (Clock Security System Interrupt, CSSI),
  *         allowing the MCU to perform rescue operations. The CSSI is linked to
  *         CPU1 and CPU2 NMI (Non-Maskable Interrupt) exception vector.
  * @note   The Clock Security System can only be cleared by reset.
  * @retval None
  */
void HAL_RCC_EnableCSS(void)
{
  LL_RCC_HSE_EnableCSS();
}

/**
  * @brief Handle the RCC HSE Clock Security System interrupt request.
  * @note  This API should be called under the NMI_Handler().
  * @retval None
  */
void HAL_RCC_NMI_IRQHandler(void)
{
  /* Check RCC CSSF interrupt flag  */
  if (__HAL_RCC_GET_IT(RCC_IT_HSECSS))
  {
    /* RCC Clock Security System interrupt user callback */
    HAL_RCC_CSSCallback();

    /* Clear RCC CSS pending bit */
    __HAL_RCC_CLEAR_IT(RCC_IT_HSECSS);
  }
}

/**
  * @brief Handle the RCC HSE Clock Security System interrupt callback.
  * @retval none
  */
__weak void HAL_RCC_CSSCallback(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the @ref HAL_RCC_CSSCallback should be implemented in the user file
   */
}

/**
  * @}
  */

/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @addtogroup RCC_Private_Functions
  * @{
  */


/**
  * @brief  Update number of Flash wait states in line with MSI range and current
            voltage range.
  * @param  MSI_Range  MSI range value from @ref RCC_MSIRANGE_0 to @ref RCC_MSIRANGE_11
  * @retval HAL status
  */
static HAL_StatusTypeDef RCC_SetFlashLatencyFromMSIRange(uint32_t MSI_Range)
{
  uint32_t flash_clksrcfreq, msifreq;

  /* Check the parameters */
  assert_param(IS_RCC_MSI_CLOCK_RANGE(MSI_Range));

  /* MSI frequency range in Hz */
  if(MSI_Range > RCC_MSIRANGE_11)
  {
    msifreq = __LL_RCC_CALC_MSI_FREQ(RCC_MSIRANGE_11);
  }
  else
  {
    msifreq = __LL_RCC_CALC_MSI_FREQ(MSI_Range);
  }

  flash_clksrcfreq = __LL_RCC_CALC_HCLK4_FREQ(msifreq, LL_RCC_GetAHB4Prescaler());

  return RCC_SetFlashLatency((flash_clksrcfreq / MEGA_HZ), HAL_PWREx_GetVoltageRange());
}


/**
  * @brief  Update number of Flash wait states.
  * @param  Flash_ClkSrcFreq  Flash Clock Source (in MHz)
  * @param  VCORE_Voltage     Current Vcore voltage (PWR_REGULATOR_VOLTAGE_SCALE1 or PWR_REGULATOR_VOLTAGE_SCALE2)
  * @retval HAL status
  */
static HAL_StatusTypeDef RCC_SetFlashLatency(uint32_t Flash_ClkSrcFreq, uint32_t VCORE_Voltage)
{
  /* Flash Clock source (HCLK4) range in MHz with a VCORE is range1 */
  const uint32_t FLASH_CLK_SRC_RANGE_VOS1[] = {18UL, 36UL, 54UL, 64UL};
  /* Flash Clock source (HCLK4) range in MHz with a VCORE is range2 */
  const uint32_t FLASH_CLK_SRC_RANGE_VOS2[] = {6UL, 12UL, 16UL};
  /* Flash Latency range */
  const uint32_t FLASH_LATENCY_RANGE[] = {FLASH_LATENCY_0, FLASH_LATENCY_1, FLASH_LATENCY_2, FLASH_LATENCY_3};
  uint32_t latency   = FLASH_LATENCY_0;  /* default value 0WS */
  uint32_t tickstart;

  if (VCORE_Voltage == PWR_REGULATOR_VOLTAGE_SCALE1)
  {
    for (uint32_t index = 0; index < __COUNTOF(FLASH_CLK_SRC_RANGE_VOS1); index++)
    {
      if (Flash_ClkSrcFreq <= FLASH_CLK_SRC_RANGE_VOS1[index])
      {
        latency = FLASH_LATENCY_RANGE[index];
        break;
      }
    }
  }
  else  /* PWR_REGULATOR_VOLTAGE_SCALE2 */
  {
    for (uint32_t index = 0; index < __COUNTOF(FLASH_CLK_SRC_RANGE_VOS2); index++)
    {
      if (Flash_ClkSrcFreq <= FLASH_CLK_SRC_RANGE_VOS2[index])
      {
        latency = FLASH_LATENCY_RANGE[index];
        break;
      }
    }
  }

  __HAL_FLASH_SET_LATENCY(latency);

  /* Get Start Tick*/
  tickstart = HAL_GetTick();

  /* Check that the new number of wait states is taken into account to access the Flash
     memory by reading the FLASH_ACR register */
  while (__HAL_FLASH_GET_LATENCY() != latency)
  {
    if ((HAL_GetTick() - tickstart) > LATENCY_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
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
