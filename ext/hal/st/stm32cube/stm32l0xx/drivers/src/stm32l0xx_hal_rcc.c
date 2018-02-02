/**
  ******************************************************************************
  * @file    stm32l0xx_hal_rcc.c
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
      After reset the device is running from multispeed internal oscillator clock 
      (MSI 2.097MHz) with Flash 0 wait state and Flash prefetch buffer is disabled, 
      and all peripherals are off except internal SRAM, Flash and JTAG.
      (+) There is no prescaler on High speed (AHB) and Low speed (APB) buses;
          all peripherals mapped on these buses are running at MSI speed.
      (+) The clock for all peripherals is switched off, except the SRAM and FLASH.
      (+) All GPIOs are in input floating state, except the JTAG pins which
          are assigned to be used for debug purpose.
    [..] Once the device started from reset, the user application has to:
      (+) Configure the clock source to be used to drive the System clock
          (if the application needs higher frequency/performance)
      (+) Configure the System clock frequency and Flash settings  
      (+) Configure the AHB and APB buses prescalers
      (+) Enable the clock for the peripheral(s) to be used
      (+) Configure the clock source(s) for peripherals whose clocks are not
          derived from the System clock (I2S, RTC, ADC, USB OTG FS/SDIO/RNG) 
          (*) SDIO only for STM32L0xxxD devices

                      ##### RCC Limitations #####
  ==============================================================================
    [..]  
      A delay between an RCC peripheral clock enable and the effective peripheral 
      enabling should be taken into account in order to manage the peripheral read/write 
      from/to registers.
      (+) This delay depends on the peripheral mapping.
        (++) AHB & APB peripherals, 1 dummy read is necessary

    [..]  
      Workarounds:
      (#) For AHB & APB peripherals, a dummy read to the peripheral register has been
          inserted in each __HAL_RCC_PPP_CLK_ENABLE() macro.

  @endverbatim
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
/* Bits position in  in the CFGR register */
#define RCC_CFGR_PLLMUL_BITNUMBER         RCC_CFGR_PLLMUL_Pos
#define RCC_CFGR_PLLDIV_BITNUMBER         RCC_CFGR_PLLDIV_Pos
#define RCC_CFGR_HPRE_BITNUMBER           RCC_CFGR_HPRE_Pos
#define RCC_CFGR_PPRE1_BITNUMBER          RCC_CFGR_PPRE1_Pos
#define RCC_CFGR_PPRE2_BITNUMBER          RCC_CFGR_PPRE2_Pos
/* Bits position in  in the ICSCR register */
#define RCC_ICSCR_MSIRANGE_BITNUMBER      RCC_ICSCR_MSIRANGE_Pos
#define RCC_ICSCR_MSITRIM_BITNUMBER       RCC_ICSCR_MSITRIM_Pos
/**
  * @}
  */
/* Private macro -------------------------------------------------------------*/
/** @defgroup RCC_Private_Macros RCC Private Macros
  * @{
  */

#define MCO1_CLK_ENABLE()     __HAL_RCC_GPIOA_CLK_ENABLE()
#define MCO1_GPIO_PORT        GPIOA
#define MCO1_PIN              GPIO_PIN_8

#define MCO2_CLK_ENABLE()     __HAL_RCC_GPIOA_CLK_ENABLE()
#define MCO2_GPIO_PORT        GPIOA
#define MCO2_PIN              GPIO_PIN_9

#if  defined(STM32L031xx) || defined(STM32L041xx) || defined(STM32L073xx) || defined(STM32L083xx) \
  || defined(STM32L072xx) || defined(STM32L082xx) || defined(STM32L071xx) || defined(STM32L081xx) 
#define MCO3_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()
#define MCO3_GPIO_PORT        GPIOB
#define MCO3_PIN              GPIO_PIN_13
#endif

/**
  * @}
  */

/* Private variables ---------------------------------------------------------*/
/** @defgroup RCC_Private_Variables RCC Private Variables
  * @{
  */
extern const uint8_t PLLMulTable[];          /* Defined in CMSIS (system_stm32l0xx.c)*/
/**
  * @}
  */

/* Private function prototypes -----------------------------------------------*/
/** @defgroup RCC_Private_Functions RCC Private Functions
  * @{
  */
static HAL_StatusTypeDef RCC_SetFlashLatencyFromMSIRange(uint32_t MSIrange);
/**
  * @}
  */

/* Exported functions ---------------------------------------------------------*/

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
      This section provides functions allowing to configure the internal/external oscillators
      (MSI, HSE, HSI, LSE, LSI, PLL, CSS and MCO) and the System buses clocks (SYSCLK, AHB, APB1 
      and APB2).

    [..] Internal/external clock and PLL configuration
      (#) MSI (Multispeed internal), Seven frequency ranges are available: 65.536 kHz, 
          131.072 kHz, 262.144 kHz, 524.288 kHz, 1.048 MHz, 2.097 MHz (default value) and 4.194 MHz.

      (#) HSI (high-speed internal), 16 MHz factory-trimmed RC used directly or through
          the PLL as System clock source.
      (#) LSI (low-speed internal), ~37 KHz low consumption RC used as IWDG and/or RTC
          clock source.

      (#) HSE (high-speed external), 1 to 24 MHz crystal oscillator used directly or
          through the PLL as System clock source. Can be used also as RTC clock source.

      (#) LSE (low-speed external), 32 KHz oscillator used as RTC clock source.   

      (#) PLL (clocked by HSI or HSE), featuring different output clocks:
        (++) The first output is used to generate the high speed system clock (up to 32 MHz)
        (++) The second output is used to generate the clock for the USB OTG FS (48 MHz)

      (#) CSS (Clock security system), once enable using the macro __HAL_RCC_CSS_ENABLE()
          and if a HSE clock failure occurs(HSE used directly or through PLL as System 
          clock source), the System clocks automatically switched to MSI and an interrupt
          is generated if enabled. The interrupt is linked to the Cortex-M0+ NMI 
          (Non-Maskable Interrupt) exception vector.   

      (#) MCO1/MCO2/MCO3 (microcontroller clock output), used to output SYSCLK, HSI, LSI, MSI, LSE, 
          HSE, HSI48 or PLL clock (through a configurable prescaler) on PA8/PA9/PB13 pins.

    [..] System, AHB and APB buses clocks configuration
      (#) Several clock sources can be used to drive the System clock (SYSCLK): MSI, HSI,
          HSE and PLL.
          The AHB clock (HCLK) is derived from System clock through configurable
          prescaler and used to clock the CPU, memory and peripherals mapped
          on AHB bus (DMA, GPIO...). APB1 (PCLK1) and APB2 (PCLK2) clocks are derived
          from AHB clock through configurable prescalers and used to clock
          the peripherals mapped on these buses. You can use
          "@ref HAL_RCC_GetSysClockFreq()" function to retrieve the frequencies of these clocks.

      -@- All the peripheral clocks are derived from the System clock (SYSCLK) except:
          (+@) RTC: RTC clock can be derived either from the LSI, LSE or HSE clock
              divided by 2 to 16. You have to use @ref __HAL_RCC_RTC_CONFIG() and @ref __HAL_RCC_RTC_ENABLE()
              macros to configure this clock. 
          (+@) LCD: LCD clock can be derived either from the LSI, LSE or HSE clock
              divided by 2 to 16. You have to use @ref __HAL_RCC_LCD_CONFIG()
              macros to configure this clock. 
          (+@) USB FS and RNG: USB FS require a frequency equal to 48 MHz to work correctly. 
               This clock is derived of the main PLL through PLL Multiplier or HSI48 RC oscillator.

          (+@) IWDG clock which is always the LSI clock.

      (#) The maximum frequency of the SYSCLK and HCLK is 32 MHz, PCLK2 32 MHz 
          and PCLK1 32 MHz. Depending on the device voltage range, the maximum 
          frequency should be adapted accordingly.
  @endverbatim
  * @{
  */
  
/*
  Additional consideration on the HCLK based on Latency settings:
  +----------------------------------------------------------------------+     
  | Latency       |                HCLK clock frequency (MHz)            |
  |               |------------------------------------------------------|     
  |               | voltage range 1  | voltage range 2 | voltage range 3 |
  |               |      1.8 V       |     1.5 V       |      1.2 V      |
  |---------------|------------------|-----------------|-----------------|              
  |0WS(1CPU cycle)| 0 < HCLK <= 16   | 0 < HCLK <= 8   | 0 < HCLK <= 2   |
  |---------------|------------------|-----------------|-----------------| 
  |1WS(2CPU cycle)| 16 < HCLK <= 32  | 8 < HCLK <= 16  | 2 < HCLK <= 4   | 
  +----------------------------------------------------------------------+     

  The following table gives the different clock source frequencies depending on the product
  voltage range:
  +------------------------------------------------------------------------------------------+     
  | Product voltage |                    Clock frequency                                     |
  |                 |------------------|-----------------------------|-----------------------|              
  |      range      |   MSI   |   HSI  |              HSE            |          PLL          |
  |-----------------|---------|--------|-----------------------------|-----------------------|              
  | Range 1 (1.8 V) | 4.2 MHz | 16 MHz | HSE 32 MHz (external clock) |         32 MHz        |
  |                 |         |        |      or 24 MHz (crystal)    | (PLLVCO max = 96 MHz) |
  |-----------------|---------|--------|-----------------------------|-----------------------|              
  | Range 2 (1.5 V) | 4.2 MHz | 16 MHz |         16 MHz              |         16 MHz        |
  |                 |         |        |                             | (PLLVCO max = 48 MHz) |
  |-----------------|---------|--------|-----------------------------|-----------------------|              
  | Range 3 (1.2 V) | 4.2 MHz |   NA   |         8 MHz               |           4 MHz       |
  |                 |         |        |                             | (PLLVCO max = 24 MHz) |
  +------------------------------------------------------------------------------------------+     
  */

/**
  * @brief  Resets the RCC clock configuration to the default reset state.
  * @note   The default reset state of the clock configuration is given below:
  *            - MSI ON and used as system clock source
  *            - HSI, HSE and PLL  OFF
  *            - AHB, APB1 and APB2 prescaler set to 1.
  *            - CSS and MCO1/MCO2/MCO3 OFF
  *            - All interrupts disabled
  * @note   This function does not modify the configuration of the
  *            - Peripheral clocks
  *            - LSI, LSE and RTC clocks
  *            - HSI48 clock
  * @retval None
  */
void HAL_RCC_DeInit(void)
{
  __IO uint32_t tmpreg;

  /* Set MSION bit */
  SET_BIT(RCC->CR, RCC_CR_MSION);
  
  /* Switch SYSCLK to MSI*/
  CLEAR_BIT(RCC->CFGR, RCC_CFGR_SW);

  /* Reset HSE, HSI, CSS, PLL */
#if defined(RCC_CR_CSSHSEON) && defined(RCC_CR_HSIOUTEN)
  CLEAR_BIT(RCC->CR, RCC_CR_HSION| RCC_CR_HSIKERON| RCC_CR_HSIDIVEN | RCC_CR_HSIOUTEN | \
                     RCC_CR_HSEON | RCC_CR_CSSHSEON | RCC_CR_PLLON); 
#elif !defined(RCC_CR_CSSHSEON) && defined(RCC_CR_HSIOUTEN)
  CLEAR_BIT(RCC->CR, RCC_CR_HSION| RCC_CR_HSIKERON| RCC_CR_HSIDIVEN | RCC_CR_HSIOUTEN | \
                     RCC_CR_HSEON | RCC_CR_PLLON);    
#elif defined(RCC_CR_CSSHSEON) && !defined(RCC_CR_HSIOUTEN)
  CLEAR_BIT(RCC->CR, RCC_CR_HSION| RCC_CR_HSIKERON| RCC_CR_HSIDIVEN | \
                     RCC_CR_HSEON | RCC_CR_CSSHSEON | RCC_CR_PLLON); 
#endif

  /* Delay after an RCC peripheral clock */ \
  tmpreg = READ_BIT(RCC->CR, RCC_CR_HSEON);      \
  UNUSED(tmpreg); 

  /* Reset HSEBYP bit */
  CLEAR_BIT(RCC->CR, RCC_CR_HSEBYP);
  
  /* Reset CFGR register */
  CLEAR_REG(RCC->CFGR);
  
  /* Set MSIClockRange & MSITRIM[4:0] bits to the reset value */
  MODIFY_REG(RCC->ICSCR, (RCC_ICSCR_MSIRANGE | RCC_ICSCR_MSITRIM), (((uint32_t)0 << RCC_ICSCR_MSITRIM_BITNUMBER) | RCC_ICSCR_MSIRANGE_5));
  
  /* Set HSITRIM bits to the reset value */
  MODIFY_REG(RCC->ICSCR, RCC_ICSCR_HSITRIM, ((uint32_t)0x10 << 8));
  
  /* Disable all interrupts */
  CLEAR_REG(RCC->CIER); 

  /* Update the SystemCoreClock global variable */
  SystemCoreClock = MSI_VALUE;
}

/**
  * @brief  Initializes the RCC Oscillators according to the specified parameters in the
  *         RCC_OscInitTypeDef.
  * @param  RCC_OscInitStruct pointer to an RCC_OscInitTypeDef structure that
  *         contains the configuration information for the RCC Oscillators.
  * @note   The PLL is not disabled when used as system clock.
  * @note   Transitions LSE Bypass to LSE On and LSE On to LSE Bypass are not
  *         supported by this macro. User should request a transition to LSE Off
  *         first and then LSE On or LSE Bypass.
  * @note   Transition HSE Bypass to HSE On and HSE On to HSE Bypass are not
  *         supported by this macro. User should request a transition to HSE Off
  *         first and then HSE On or HSE Bypass.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{
   uint32_t tickstart = 0U;
  
  /* Check the parameters */
  assert_param(RCC_OscInitStruct != NULL);
  assert_param(IS_RCC_OSCILLATORTYPE(RCC_OscInitStruct->OscillatorType));

  /*------------------------------- HSE Configuration ------------------------*/ 
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSE(RCC_OscInitStruct->HSEState));

    /* When the HSE is used as system clock or clock source for PLL in these cases it is not allowed to be disabled */
    if((__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_HSE) 
       || ((__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_PLLCLK) && (__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSE)))
    {
      if((__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) != RESET) && (RCC_OscInitStruct->HSEState == RCC_HSE_OFF))
      {
        return HAL_ERROR;
      }
    }
    else
    {
      /* Set the new HSE configuration ---------------------------------------*/
      __HAL_RCC_HSE_CONFIG(RCC_OscInitStruct->HSEState);
      

       /* Check the HSE State */
      if(RCC_OscInitStruct->HSEState != RCC_HSE_OFF)
      {
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till HSE is ready */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
        {
          if((HAL_GetTick() - tickstart ) > HSE_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
      else
      {
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till HSE is disabled */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) != RESET)
        {
           if((HAL_GetTick() - tickstart ) > HSE_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*----------------------------- HSI Configuration --------------------------*/ 
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSI(RCC_OscInitStruct->HSIState));
    assert_param(IS_RCC_CALIBRATION_VALUE(RCC_OscInitStruct->HSICalibrationValue));
    
    /* Check if HSI is used as system clock or as PLL source when PLL is selected as system clock */ 
    if((__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_HSI) 
       || ((__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_SYSCLKSOURCE_STATUS_PLLCLK) && (__HAL_RCC_GET_PLL_OSCSOURCE() == RCC_PLLSOURCE_HSI)))
    {
      /* When HSI is used as system clock it will not disabled */
      if((__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) != RESET) && (RCC_OscInitStruct->HSIState != RCC_HSI_ON))
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
      if(RCC_OscInitStruct->HSIState != RCC_HSI_OFF)
      {
        /* Enable the Internal High Speed oscillator (HSI or HSIdiv4) */
        __HAL_RCC_HSI_CONFIG(RCC_OscInitStruct->HSIState);
        
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till HSI is ready */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
        {
          if((HAL_GetTick() - tickstart ) > HSI_TIMEOUT_VALUE)
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
        
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till HSI is disabled */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) != RESET)
        {
          if((HAL_GetTick() - tickstart ) > HSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }
  /*----------------------------- MSI Configuration --------------------------*/ 
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_MSI) == RCC_OSCILLATORTYPE_MSI)
  {
    /* When the MSI is used as system clock it will not be disabled */
    if((__HAL_RCC_GET_SYSCLK_SOURCE() == RCC_CFGR_SWS_MSI) )
    {
      if((__HAL_RCC_GET_FLAG(RCC_FLAG_MSIRDY) != RESET) && (RCC_OscInitStruct->MSIState == RCC_MSI_OFF))
      {
        return HAL_ERROR;
      }
      /* Otherwise, just the calibration and MSI range change are allowed */
      else
      {
       /* Check MSICalibrationValue and MSIClockRange input parameters */
        assert_param(IS_RCC_MSICALIBRATION_VALUE(RCC_OscInitStruct->MSICalibrationValue));
        assert_param(IS_RCC_MSI_CLOCK_RANGE(RCC_OscInitStruct->MSIClockRange));

        /* To correctly read data from FLASH memory, the number of wait states (LATENCY)
           must be correctly programmed according to the frequency of the CPU clock
           (HCLK) and the supply voltage of the device. */
        if(RCC_OscInitStruct->MSIClockRange > __HAL_RCC_GET_MSI_RANGE())
        {
          /* First increase number of wait states update if necessary */
          if(RCC_SetFlashLatencyFromMSIRange(RCC_OscInitStruct->MSIClockRange) != HAL_OK)
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
          if(RCC_SetFlashLatencyFromMSIRange(RCC_OscInitStruct->MSIClockRange) != HAL_OK)
          {
            return HAL_ERROR;
          }          
        }

        /* Update the SystemCoreClock global variable */
        SystemCoreClock =  (32768U * (1U << ((RCC_OscInitStruct->MSIClockRange >> RCC_ICSCR_MSIRANGE_BITNUMBER) + 1U))) 
                           >> AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_BITNUMBER)];

        /* Configure the source of time base considering new system clocks settings*/
        HAL_InitTick (TICK_INT_PRIORITY);
      }
    }
    else
    {
      /* Check MSI State */
      assert_param(IS_RCC_MSI(RCC_OscInitStruct->MSIState));

      /* Check the MSI State */
      if(RCC_OscInitStruct->MSIState != RCC_MSI_OFF)
      {
        /* Enable the Multi Speed oscillator (MSI). */
        __HAL_RCC_MSI_ENABLE();

        /* Get Start Tick */
        tickstart = HAL_GetTick();

        /* Wait till MSI is ready */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_MSIRDY) == RESET)
        {
          if((HAL_GetTick() - tickstart) > MSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
        /* Check MSICalibrationValue and MSIClockRange input parameters */
        assert_param(IS_RCC_MSICALIBRATION_VALUE(RCC_OscInitStruct->MSICalibrationValue));
        assert_param(IS_RCC_MSI_CLOCK_RANGE(RCC_OscInitStruct->MSIClockRange));

        /* Selects the Multiple Speed oscillator (MSI) clock range .*/
        __HAL_RCC_MSI_RANGE_CONFIG(RCC_OscInitStruct->MSIClockRange);
         /* Adjusts the Multiple Speed oscillator (MSI) calibration value.*/
        __HAL_RCC_MSI_CALIBRATIONVALUE_ADJUST(RCC_OscInitStruct->MSICalibrationValue);

      }
      else
      {
        /* Disable the Multi Speed oscillator (MSI). */
        __HAL_RCC_MSI_DISABLE();

        /* Get Start Tick */
        tickstart = HAL_GetTick();

        /* Wait till MSI is ready */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_MSIRDY) != RESET)
        {
          if((HAL_GetTick() - tickstart) > MSI_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
  }  
  /*------------------------------ LSI Configuration -------------------------*/ 
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI)
  {
    /* Check the parameters */
    assert_param(IS_RCC_LSI(RCC_OscInitStruct->LSIState));
    
    /* Check the LSI State */
    if(RCC_OscInitStruct->LSIState != RCC_LSI_OFF)
    {
      /* Enable the Internal Low Speed oscillator (LSI). */
      __HAL_RCC_LSI_ENABLE();
      
      /* Get Start Tick */
      tickstart = HAL_GetTick();
      
      /* Wait till LSI is ready */  
      while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET)
      {
        if((HAL_GetTick() - tickstart ) > LSI_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    else
    {
      /* Disable the Internal Low Speed oscillator (LSI). */
      __HAL_RCC_LSI_DISABLE();
      
      /* Get Start Tick */
      tickstart = HAL_GetTick();
      
      /* Wait till LSI is disabled */  
      while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) != RESET)
      {
        if((HAL_GetTick() - tickstart ) > LSI_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
  }
  /*------------------------------ LSE Configuration -------------------------*/ 
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE)
  {
    FlagStatus       pwrclkchanged = RESET;
    
    /* Check the parameters */
    assert_param(IS_RCC_LSE(RCC_OscInitStruct->LSEState));

    /* Update LSE configuration in Backup Domain control register    */
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

    /* Set the new LSE configuration -----------------------------------------*/
    __HAL_RCC_LSE_CONFIG(RCC_OscInitStruct->LSEState);
    /* Check the LSE State */
    if(RCC_OscInitStruct->LSEState != RCC_LSE_OFF)
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
    else
    {
      /* Get Start Tick */
      tickstart = HAL_GetTick();
      
      /* Wait till LSE is disabled */  
      while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) != RESET)
      {
        if((HAL_GetTick() - tickstart ) > RCC_LSE_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }

    /* Require to disable power clock if necessary */
    if(pwrclkchanged == SET)
    {
      __HAL_RCC_PWR_CLK_DISABLE();
    }
  }

#if defined(RCC_HSI48_SUPPORT)
  /*----------------------------- HSI48 Configuration --------------------------*/
  if(((RCC_OscInitStruct->OscillatorType) & RCC_OSCILLATORTYPE_HSI48) == RCC_OSCILLATORTYPE_HSI48)
  {
    /* Check the parameters */
    assert_param(IS_RCC_HSI48(RCC_OscInitStruct->HSI48State));

      /* Check the HSI48 State */
      if(RCC_OscInitStruct->HSI48State != RCC_HSI48_OFF)
      {
        /* Enable the Internal High Speed oscillator (HSI48). */
        __HAL_RCC_HSI48_ENABLE();

        /* Get Start Tick */
        tickstart = HAL_GetTick();
      
        /* Wait till HSI48 is ready */  
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSI48RDY) == RESET)
        {
          if((HAL_GetTick() - tickstart) > HSI48_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        } 
      }
      else
      {
        /* Disable the Internal High Speed oscillator (HSI48). */
        __HAL_RCC_HSI48_DISABLE();

        /* Get Start Tick */
        tickstart = HAL_GetTick();
      
        /* Wait till HSI48 is ready */  
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_HSI48RDY) != RESET)
        {
          if((HAL_GetTick() - tickstart) > HSI48_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
  }
#endif /* RCC_HSI48_SUPPORT */
       
  /*-------------------------------- PLL Configuration -----------------------*/
  /* Check the parameters */
  assert_param(IS_RCC_PLL(RCC_OscInitStruct->PLL.PLLState));
  if ((RCC_OscInitStruct->PLL.PLLState) != RCC_PLL_NONE)
  {
    /* Check if the PLL is used as system clock or not */
    if(__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK)
    { 
      if((RCC_OscInitStruct->PLL.PLLState) == RCC_PLL_ON)
      {
        /* Check the parameters */
        assert_param(IS_RCC_PLLSOURCE(RCC_OscInitStruct->PLL.PLLSource));
        assert_param(IS_RCC_PLL_MUL(RCC_OscInitStruct->PLL.PLLMUL));
        assert_param(IS_RCC_PLL_DIV(RCC_OscInitStruct->PLL.PLLDIV));
  
        /* Disable the main PLL. */
        __HAL_RCC_PLL_DISABLE();
        
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till PLL is disabled */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  != RESET)
        {
          if((HAL_GetTick() - tickstart ) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }

        /* Configure the main PLL clock source, multiplication and division factors. */
        __HAL_RCC_PLL_CONFIG(RCC_OscInitStruct->PLL.PLLSource,
                             RCC_OscInitStruct->PLL.PLLMUL,
                             RCC_OscInitStruct->PLL.PLLDIV);
        /* Enable the main PLL. */
        __HAL_RCC_PLL_ENABLE();
        
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till PLL is ready */
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  == RESET)
        {
          if((HAL_GetTick() - tickstart ) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
      else
      {
        /* Disable the main PLL. */
        __HAL_RCC_PLL_DISABLE();
 
        /* Get Start Tick */
        tickstart = HAL_GetTick();
        
        /* Wait till PLL is disabled */  
        while(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY)  != RESET)
        {
          if((HAL_GetTick() - tickstart ) > PLL_TIMEOUT_VALUE)
          {
            return HAL_TIMEOUT;
          }
        }
      }
    }
    else
    {
      return HAL_ERROR;
    }
  }
  
  return HAL_OK;
}

/**
  * @brief  Initializes the CPU, AHB and APB buses clocks according to the specified 
  *         parameters in the RCC_ClkInitStruct.
  * @param  RCC_ClkInitStruct pointer to an RCC_OscInitTypeDef structure that
  *         contains the configuration information for the RCC peripheral.
  * @param  FLatency FLASH Latency                   
  *          The value of this parameter depend on device used within the same series
  * @note   The SystemCoreClock CMSIS variable is used to store System Clock Frequency 
  *         and updated by @ref HAL_RCC_GetHCLKFreq() function called within this function
  *
  * @note   The MSI is used (enabled by hardware) as system clock source after
  *         start-up from Reset, wake-up from STOP and STANDBY mode, or in case
  *         of failure of the HSE used directly or indirectly as system clock
  *         (if the Clock Security System CSS is enabled).
  *           
  * @note   A switch from one clock source to another occurs only if the target
  *         clock source is ready (clock stable after start-up delay or PLL locked). 
  *         If a clock source which is not yet ready is selected, the switch will
  *         occur when the clock source will be ready. 
  *         You can use @ref HAL_RCC_GetClockConfig() function to know which clock is
  *         currently used as system clock source.
  * @note   Depending on the device voltage range, the software has to set correctly
  *         HPRE[3:0] bits to ensure that HCLK not exceed the maximum allowed frequency
  *         (for more details refer to section above "Initialization/de-initialization functions")
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t FLatency)
{
  uint32_t tickstart = 0U;
  
  /* Check the parameters */
  assert_param(RCC_ClkInitStruct != NULL);
  assert_param(IS_RCC_CLOCKTYPE(RCC_ClkInitStruct->ClockType));
  assert_param(IS_FLASH_LATENCY(FLatency));

  /* To correctly read data from FLASH memory, the number of wait states (LATENCY) 
  must be correctly programmed according to the frequency of the CPU clock 
  (HCLK) and the supply voltage of the device. */

  /* Increasing the number of wait states because of higher CPU frequency */
  if(FLatency > (FLASH->ACR & FLASH_ACR_LATENCY))
  {    
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
    __HAL_FLASH_SET_LATENCY(FLatency);
    
    /* Check that the new number of wait states is taken into account to access the Flash
    memory by reading the FLASH_ACR register */
    if((FLASH->ACR & FLASH_ACR_LATENCY) != FLatency)
    {
      return HAL_ERROR;
    }
  }

  /*-------------------------- HCLK Configuration --------------------------*/
  if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_HCLK) == RCC_CLOCKTYPE_HCLK)
  {
    assert_param(IS_RCC_HCLK(RCC_ClkInitStruct->AHBCLKDivider));
    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_ClkInitStruct->AHBCLKDivider);
  }

  /*------------------------- SYSCLK Configuration ---------------------------*/ 
  if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_SYSCLK) == RCC_CLOCKTYPE_SYSCLK)
  {    
    assert_param(IS_RCC_SYSCLKSOURCE(RCC_ClkInitStruct->SYSCLKSource));
    
    /* HSE is selected as System Clock Source */
    if(RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_HSE)
    {
      /* Check the HSE ready flag */  
      if(__HAL_RCC_GET_FLAG(RCC_FLAG_HSERDY) == RESET)
      {
        return HAL_ERROR;
      }
    }
    /* PLL is selected as System Clock Source */
    else if(RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK)
    {
      /* Check the PLL ready flag */  
      if(__HAL_RCC_GET_FLAG(RCC_FLAG_PLLRDY) == RESET)
      {
        return HAL_ERROR;
      }
    }
    /* HSI is selected as System Clock Source */
    else if(RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_HSI)
    {
      /* Check the HSI ready flag */  
      if(__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
      {
        return HAL_ERROR;
      }
    }
    /* MSI is selected as System Clock Source */
    else
    {
      /* Check the MSI ready flag */  
      if(__HAL_RCC_GET_FLAG(RCC_FLAG_MSIRDY) == RESET)
      {
        return HAL_ERROR;
      }
    }
    __HAL_RCC_SYSCLK_CONFIG(RCC_ClkInitStruct->SYSCLKSource);

    /* Get Start Tick */
    tickstart = HAL_GetTick();
    
    if(RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_HSE)
    {
      while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_HSE)
      {
        if((HAL_GetTick() - tickstart ) > CLOCKSWITCH_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    else if(RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_PLLCLK)
    {
      while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_PLLCLK)
      {
        if((HAL_GetTick() - tickstart ) > CLOCKSWITCH_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
    else if(RCC_ClkInitStruct->SYSCLKSource == RCC_SYSCLKSOURCE_HSI)
    {
      while (__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_HSI)
      {
        if((HAL_GetTick() - tickstart ) > CLOCKSWITCH_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }      
    else
    {
      while(__HAL_RCC_GET_SYSCLK_SOURCE() != RCC_SYSCLKSOURCE_STATUS_MSI)
      {
        if((HAL_GetTick() - tickstart ) > CLOCKSWITCH_TIMEOUT_VALUE)
        {
          return HAL_TIMEOUT;
        }
      }
    }
  }    
  /* Decreasing the number of wait states because of lower CPU frequency */
  if(FLatency < (FLASH->ACR & FLASH_ACR_LATENCY))
  {    
    /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
    __HAL_FLASH_SET_LATENCY(FLatency);
    
    /* Check that the new number of wait states is taken into account to access the Flash
    memory by reading the FLASH_ACR register */
    if((FLASH->ACR & FLASH_ACR_LATENCY) != FLatency)
    {
      return HAL_ERROR;
    }
  }    

  /*-------------------------- PCLK1 Configuration ---------------------------*/ 
  if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_PCLK1) == RCC_CLOCKTYPE_PCLK1)
  {
    assert_param(IS_RCC_PCLK(RCC_ClkInitStruct->APB1CLKDivider));
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_ClkInitStruct->APB1CLKDivider);
  }
  
  /*-------------------------- PCLK2 Configuration ---------------------------*/ 
  if(((RCC_ClkInitStruct->ClockType) & RCC_CLOCKTYPE_PCLK2) == RCC_CLOCKTYPE_PCLK2)
  {
    assert_param(IS_RCC_PCLK(RCC_ClkInitStruct->APB2CLKDivider));
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, ((RCC_ClkInitStruct->APB2CLKDivider) << 3));
  }
 
  /* Update the SystemCoreClock global variable */
  SystemCoreClock = HAL_RCC_GetSysClockFreq() >> AHBPrescTable[(RCC->CFGR & RCC_CFGR_HPRE)>> RCC_CFGR_HPRE_BITNUMBER];

  /* Configure the source of time base considering new system clocks settings*/
  HAL_InitTick (TICK_INT_PRIORITY);
  
  return HAL_OK;
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
    This subsection provides a set of functions allowing to control the RCC Clocks 
    frequencies.

  @endverbatim
  * @{
  */

/**
  * @brief  Selects the clock source to output on MCO pin.
  * @note   MCO pin should be configured in alternate function mode.
  * @param  RCC_MCOx specifies the output direction for the clock source.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1 Clock source to output on MCO1 pin(PA8).
  *            @arg @ref RCC_MCO2 Clock source to output on MCO2 pin(PA9).
  @if STM32L031xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L041xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L073xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L083xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L072xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L082xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L071xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @elseif STM32L081xx
  *            @arg @ref RCC_MCO3 Clock source to output on MCO3 pin(PB13)
  @endif
  * @param  RCC_MCOSource specifies the clock source to output.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCO1SOURCE_NOCLOCK     No clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_SYSCLK      System clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSI         HSI selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_HSE         HSE selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_MSI         MSI oscillator clock selected as MCO clock 
  *            @arg @ref RCC_MCO1SOURCE_PLLCLK      PLL clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSI         LSI clock selected as MCO clock
  *            @arg @ref RCC_MCO1SOURCE_LSE         LSE clock selected as MCO clock
  @if STM32L052xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L053xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L062xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L063xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L072xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L073xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L082xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @elseif STM32L083xx
  *            @arg @ref RCC_MCO1SOURCE_HSI48       HSI48 clock selected as MCO clock
  @endif
  * @param  RCC_MCODiv specifies the MCO DIV.
  *          This parameter can be one of the following values:
  *            @arg @ref RCC_MCODIV_1 no division applied to MCO clock
  *            @arg @ref RCC_MCODIV_2  division by 2 applied to MCO clock
  *            @arg @ref RCC_MCODIV_4  division by 4 applied to MCO clock
  *            @arg @ref RCC_MCODIV_8  division by 8 applied to MCO clock
  *            @arg @ref RCC_MCODIV_16 division by 16 applied to MCO clock
  * @retval None
  */
void HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv)
{
  GPIO_InitTypeDef gpio = {0};

  /* Check the parameters */
  assert_param(IS_RCC_MCO(RCC_MCOx));
  assert_param(IS_RCC_MCODIV(RCC_MCODiv));
  assert_param(IS_RCC_MCO1SOURCE(RCC_MCOSource));
  
  /* Configure the MCO1 pin in alternate function mode */
  gpio.Mode      = GPIO_MODE_AF_PP;
  gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio.Pull      = GPIO_NOPULL;
  if(RCC_MCOx == RCC_MCO1)
  {    
    gpio.Pin       = MCO1_PIN;
    gpio.Alternate = GPIO_AF0_MCO;
    
    /* MCO1 Clock Enable */
    MCO1_CLK_ENABLE();
    HAL_GPIO_Init(MCO1_GPIO_PORT, &gpio);
  }
#if  defined(STM32L031xx) || defined(STM32L041xx) || defined(STM32L073xx) || defined(STM32L083xx) \
  || defined(STM32L072xx) || defined(STM32L082xx) || defined(STM32L071xx) || defined(STM32L081xx) 
  else if (RCC_MCOx == RCC_MCO3)
  {
    gpio.Pin       = MCO3_PIN;
    gpio.Alternate = GPIO_AF2_MCO;
    
    /* MCO3 Clock Enable */
    MCO3_CLK_ENABLE();    
    HAL_GPIO_Init(MCO3_GPIO_PORT, &gpio);
  }
#endif
  else
  {    
    gpio.Pin       = MCO2_PIN;
    gpio.Alternate = GPIO_AF0_MCO;
    
    /* MCO2 Clock Enable */
    MCO2_CLK_ENABLE();
    HAL_GPIO_Init(MCO2_GPIO_PORT, &gpio);
  }    
  
  /* Configure the MCO clock source */
  __HAL_RCC_MCO1_CONFIG(RCC_MCOSource, RCC_MCODiv);
}

#if defined(RCC_HSECSS_SUPPORT)
/**
  * @brief  Enables the Clock Security System.
  * @note   If a failure is detected on the HSE oscillator clock, this oscillator
  *         is automatically disabled and an interrupt is generated to inform the
  *         software about the failure (Clock Security System Interrupt, CSSI),
  *         allowing the MCU to perform rescue operations. The CSSI is linked to 
  *         the Cortex-M0+ NMI (Non-Maskable Interrupt) exception vector.  
  * @retval None
  */
void HAL_RCC_EnableCSS(void)
{
  SET_BIT(RCC->CR, RCC_CR_CSSON) ;
}

#endif /* RCC_HSECSS_SUPPORT */
/**
  * @brief  Returns the SYSCLK frequency     
  * @note   The system frequency computed by this function is not the real 
  *         frequency in the chip. It is calculated based on the predefined 
  *         constant and the selected clock source:
  * @note     If SYSCLK source is MSI, function returns a value based on MSI
  *             Value as defined by the MSI range.
  * @note     If SYSCLK source is HSI, function returns values based on HSI_VALUE(*)
  * @note     If SYSCLK source is HSE, function returns a value based on HSE_VALUE(**)
  * @note     If SYSCLK source is PLL, function returns a value based on HSE_VALUE(**) 
  *           or HSI_VALUE(*) multiplied/divided by the PLL factors.         
  * @note     (*) HSI_VALUE is a constant defined in stm32l0xx_hal_conf.h file (default value
  *               16 MHz) but the real value may vary depending on the variations
  *               in voltage and temperature.
  * @note     (**) HSE_VALUE is a constant defined in stm32l0xx_hal_conf.h file (default value
  *                8 MHz), user has to ensure that HSE_VALUE is same as the real
  *                frequency of the crystal used. Otherwise, this function may
  *                have wrong result.
  *                  
  * @note   The result of this function could be not correct when using fractional
  *         value for HSE crystal.
  *           
  * @note   This function can be used by the user application to compute the 
  *         baud-rate for the communication peripherals or configure other parameters.
  *           
  * @note   Each time SYSCLK changes, this function must be called to update the
  *         right SYSCLK value. Otherwise, any configuration based on this function will be incorrect.
  *         
  * @retval SYSCLK frequency
  */
uint32_t HAL_RCC_GetSysClockFreq(void)
{
  uint32_t tmpreg = 0, pllm = 0, plld = 0, pllvco = 0, msiclkrange = 0;
  uint32_t sysclockfreq = 0;
  
  tmpreg = RCC->CFGR;
  
  /* Get SYSCLK source -------------------------------------------------------*/
  switch (tmpreg & RCC_CFGR_SWS)
  {
    case RCC_SYSCLKSOURCE_STATUS_HSI:  /* HSI used as system clock source */
    {
      if ((RCC->CR & RCC_CR_HSIDIVF) != 0)
      {
        sysclockfreq =  (HSI_VALUE >> 2);
      }
      else 
      {
        sysclockfreq =  HSI_VALUE;
      }
      break;
    }
    case RCC_SYSCLKSOURCE_STATUS_HSE:  /* HSE used as system clock */
    {
      sysclockfreq = HSE_VALUE;
      break;
    }
    case RCC_SYSCLKSOURCE_STATUS_PLLCLK:  /* PLL used as system clock */
    {
      pllm = PLLMulTable[(uint32_t)(tmpreg & RCC_CFGR_PLLMUL) >> RCC_CFGR_PLLMUL_BITNUMBER];
      plld = ((uint32_t)(tmpreg & RCC_CFGR_PLLDIV) >> RCC_CFGR_PLLDIV_BITNUMBER) + 1;
      if (__HAL_RCC_GET_PLL_OSCSOURCE() != RCC_PLLSOURCE_HSI)
      {
        /* HSE used as PLL clock source */
        pllvco = (HSE_VALUE * pllm) / plld;
      }
      else
      {
        if ((RCC->CR & RCC_CR_HSIDIVF) != 0)
        {
          pllvco = ((HSI_VALUE >> 2) * pllm) / plld;
        }
        else 
        {
         pllvco = (HSI_VALUE * pllm) / plld;
        }
      }
      sysclockfreq = pllvco;
      break;
    }
    case RCC_SYSCLKSOURCE_STATUS_MSI:  /* MSI used as system clock source */
    default: /* MSI used as system clock */
    {
      msiclkrange = (RCC->ICSCR & RCC_ICSCR_MSIRANGE ) >> RCC_ICSCR_MSIRANGE_BITNUMBER;
      sysclockfreq = (32768 * (1 << (msiclkrange + 1)));
      break;
    }
  }
  return sysclockfreq;
}

/**
  * @brief  Returns the HCLK frequency     
  * @note   Each time HCLK changes, this function must be called to update the
  *         right HCLK value. Otherwise, any configuration based on this function will be incorrect.
  * 
  * @note   The SystemCoreClock CMSIS variable is used to store System Clock Frequency 
  *         and updated within this function
  * @retval HCLK frequency
  */
uint32_t HAL_RCC_GetHCLKFreq(void)
{
  return SystemCoreClock;
}

/**
  * @brief  Returns the PCLK1 frequency     
  * @note   Each time PCLK1 changes, this function must be called to update the
  *         right PCLK1 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK1 frequency
  */
uint32_t HAL_RCC_GetPCLK1Freq(void)
{
  /* Get HCLK source and Compute PCLK1 frequency ---------------------------*/
  return (HAL_RCC_GetHCLKFreq() >> APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_BITNUMBER]);
}    

/**
  * @brief  Returns the PCLK2 frequency     
  * @note   Each time PCLK2 changes, this function must be called to update the
  *         right PCLK2 value. Otherwise, any configuration based on this function will be incorrect.
  * @retval PCLK2 frequency
  */
uint32_t HAL_RCC_GetPCLK2Freq(void)
{
  /* Get HCLK source and Compute PCLK2 frequency ---------------------------*/
  return (HAL_RCC_GetHCLKFreq()>> APBPrescTable[(RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_BITNUMBER]);
} 

/**
  * @brief  Configures the RCC_OscInitStruct according to the internal 
  * RCC configuration registers.
  * @param  RCC_OscInitStruct pointer to an RCC_OscInitTypeDef structure that 
  * will be configured.
  * @retval None
  */
void HAL_RCC_GetOscConfig(RCC_OscInitTypeDef  *RCC_OscInitStruct)
{
  /* Check the parameters */
  assert_param(RCC_OscInitStruct != NULL);

  /* Set all possible values for the Oscillator type parameter ---------------*/
  RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI  \
                  | RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_MSI;
#if defined(RCC_HSI48_SUPPORT)
  RCC_OscInitStruct->OscillatorType |= RCC_OSCILLATORTYPE_HSI48;
#endif /* RCC_HSI48_SUPPORT */


  /* Get the HSE configuration -----------------------------------------------*/
  if((RCC->CR &RCC_CR_HSEBYP) == RCC_CR_HSEBYP)
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_BYPASS;
  }
  else if((RCC->CR &RCC_CR_HSEON) == RCC_CR_HSEON)
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
  }
  else
  {
    RCC_OscInitStruct->HSEState = RCC_HSE_OFF;
  }

  /* Get the HSI configuration -----------------------------------------------*/
  if((RCC->CR &RCC_CR_HSION) == RCC_CR_HSION)
  {
    RCC_OscInitStruct->HSIState = RCC_HSI_ON;
  }
  else
  {
    RCC_OscInitStruct->HSIState = RCC_HSI_OFF;
  }
  
  RCC_OscInitStruct->HSICalibrationValue = (uint32_t)((RCC->ICSCR & RCC_ICSCR_HSITRIM) >> 8);
  
  /* Get the MSI configuration -----------------------------------------------*/
  if((RCC->CR &RCC_CR_MSION) == RCC_CR_MSION)
  {
    RCC_OscInitStruct->MSIState = RCC_MSI_ON;
  }
  else
  {
    RCC_OscInitStruct->MSIState = RCC_MSI_OFF;
  }
  
  RCC_OscInitStruct->MSICalibrationValue = (uint32_t)((RCC->ICSCR & RCC_ICSCR_MSITRIM) >> RCC_ICSCR_MSITRIM_BITNUMBER);
  RCC_OscInitStruct->MSIClockRange = (uint32_t)((RCC->ICSCR & RCC_ICSCR_MSIRANGE));
  
  /* Get the LSE configuration -----------------------------------------------*/
  if((RCC->CSR &RCC_CSR_LSEBYP) == RCC_CSR_LSEBYP)
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_BYPASS;
  }
  else if((RCC->CSR &RCC_CSR_LSEON) == RCC_CSR_LSEON)
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_ON;
  }
  else
  {
    RCC_OscInitStruct->LSEState = RCC_LSE_OFF;
  }
  
  /* Get the LSI configuration -----------------------------------------------*/
  if((RCC->CSR &RCC_CSR_LSION) == RCC_CSR_LSION)
  {
    RCC_OscInitStruct->LSIState = RCC_LSI_ON;
  }
  else
  {
    RCC_OscInitStruct->LSIState = RCC_LSI_OFF;
  }
  
#if defined(RCC_HSI48_SUPPORT)
  /* Get the HSI48 configuration if any-----------------------------------------*/
  RCC_OscInitStruct->HSI48State = __HAL_RCC_GET_HSI48_STATE();
#endif /* RCC_HSI48_SUPPORT */

  /* Get the PLL configuration -----------------------------------------------*/
  if((RCC->CR &RCC_CR_PLLON) == RCC_CR_PLLON)
  {
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
  }
  else
  {
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_OFF;
  }
  RCC_OscInitStruct->PLL.PLLSource = (uint32_t)(RCC->CFGR & RCC_CFGR_PLLSRC);
  RCC_OscInitStruct->PLL.PLLMUL = (uint32_t)(RCC->CFGR & RCC_CFGR_PLLMUL);
  RCC_OscInitStruct->PLL.PLLDIV = (uint32_t)(RCC->CFGR & RCC_CFGR_PLLDIV);
}

/**
  * @brief  Get the RCC_ClkInitStruct according to the internal 
  * RCC configuration registers.
  * @param  RCC_ClkInitStruct pointer to an RCC_ClkInitTypeDef structure that 
  * contains the current clock configuration.
  * @param  pFLatency Pointer on the Flash Latency.
  * @retval None
  */
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInitStruct, uint32_t *pFLatency)
{
  /* Check the parameters */
  assert_param(RCC_ClkInitStruct != NULL);
  assert_param(pFLatency != NULL);

  /* Set all possible values for the Clock type parameter --------------------*/
  RCC_ClkInitStruct->ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  
  /* Get the SYSCLK configuration --------------------------------------------*/ 
  RCC_ClkInitStruct->SYSCLKSource = (uint32_t)(RCC->CFGR & RCC_CFGR_SW);
  
  /* Get the HCLK configuration ----------------------------------------------*/ 
  RCC_ClkInitStruct->AHBCLKDivider = (uint32_t)(RCC->CFGR & RCC_CFGR_HPRE); 
  
  /* Get the APB1 configuration ----------------------------------------------*/ 
  RCC_ClkInitStruct->APB1CLKDivider = (uint32_t)(RCC->CFGR & RCC_CFGR_PPRE1);   
  
  /* Get the APB2 configuration ----------------------------------------------*/ 
  RCC_ClkInitStruct->APB2CLKDivider = (uint32_t)((RCC->CFGR & RCC_CFGR_PPRE2) >> 3);
  
  /* Get the Flash Wait State (Latency) configuration ------------------------*/   
  *pFLatency = (uint32_t)(FLASH->ACR & FLASH_ACR_LATENCY); 
}

#if defined(RCC_HSECSS_SUPPORT)
/**
  * @brief This function handles the RCC CSS interrupt request.
  * @note This API should be called under the NMI_Handler().
  * @retval None
  */
void HAL_RCC_NMI_IRQHandler(void)
{
  /* Check RCC CSSF flag  */
  if(__HAL_RCC_GET_IT(RCC_IT_CSS))
  {
    /* RCC Clock Security System interrupt user callback */
    HAL_RCC_CSSCallback();
    
    /* Clear RCC CSS pending bit */
    __HAL_RCC_CLEAR_IT(RCC_IT_CSS);
  }
}

/**
  * @brief  RCC Clock Security System interrupt callback
  * @retval none
  */
__weak void HAL_RCC_CSSCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_RCC_CSSCallback could be implemented in the user file
    */ 
}

#endif /* RCC_HSECSS_SUPPORT */
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
            voltage range
  * @param  MSIrange  MSI range value from RCC_MSIRANGE_0 to RCC_MSIRANGE_6
  * @retval HAL status
  */
static HAL_StatusTypeDef RCC_SetFlashLatencyFromMSIRange(uint32_t MSIrange)
{
  uint32_t vos = 0;
  uint32_t latency = FLASH_LATENCY_0;  /* default value 0WS */

  /* HCLK can reach 4 MHz only if AHB prescaler = 1 */
  if (READ_BIT(RCC->CFGR, RCC_CFGR_HPRE) == RCC_SYSCLK_DIV1)
  {
    if(__HAL_RCC_PWR_IS_CLK_ENABLED())
    {
      vos = READ_BIT(PWR->CR, PWR_CR_VOS);
    }
    else
    {
      __HAL_RCC_PWR_CLK_ENABLE();
      vos = READ_BIT(PWR->CR, PWR_CR_VOS);
      __HAL_RCC_PWR_CLK_DISABLE();
    }
    
    /* Check if need to set latency 1 only for Range 3 & HCLK = 4MHz */
    if((vos == PWR_REGULATOR_VOLTAGE_SCALE3) && (MSIrange == RCC_MSIRANGE_6))
    {
      latency = FLASH_LATENCY_1; /* 1WS */
    }
  }
  
  __HAL_FLASH_SET_LATENCY(latency);
  
  /* Check that the new number of wait states is taken into account to access the Flash
     memory by reading the FLASH_ACR register */
  if((FLASH->ACR & FLASH_ACR_LATENCY) != latency)
  {
    return HAL_ERROR;
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
