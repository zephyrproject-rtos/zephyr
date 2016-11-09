/**
  ******************************************************************************
  * @file    stm32l0xx_hal_rcc_ex.c
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   Extended RCC HAL module driver.
  *    
  *          This file provides firmware functions to manage the following 
  *          functionalities RCC extension peripheral:
  *           + Extended Peripheral Control functions
  *  
  @verbatim                
  ==============================================================================
                      ##### RCC specific features #####
  ==============================================================================
    For CRS, RCC Extension HAL driver can be used as follows:

      (#) In System clock configuration, HSI48 need to be enabled

      (#] Enable CRS clock in IP MSP init which will use CRS functions

      (#) Call CRS functions like this
          (##) Prepare synchronization configuration necessary for HSI48 calibration
              (+++) Default values can be set for frequency Error Measurement (reload and error limit)
                        and also HSI48 oscillator smooth trimming.
              (+++) Macro __HAL_RCC_CRS_RELOADVALUE_CALCULATE can be also used to calculate
                        directly reload value with target and synchronization frequencies values
          (##) Call function HAL_RCCEx_CRSConfig which
              (+++) Reset CRS registers to their default values.
              (+++) Configure CRS registers with synchronization configuration 
              (+++) Enable automatic calibration and frequency error counter feature

          (##) A polling function is provided to wait for complete Synchronization
              (+++) Call function 'HAL_RCCEx_CRSWaitSynchronization()'
              (+++) According to CRS status, user can decide to adjust again the calibration or continue
                        application if synchronization is OK
              
      (#) User can retrieve information related to synchronization in calling function
            HAL_RCCEx_CRSGetSynchronizationInfo()

      (#) Regarding synchronization status and synchronization information, user can try a new calibration
           in changing synchronization configuration and call again HAL_RCCEx_CRSConfig.
           Note: When the SYNC event is detected during the downcounting phase (before reaching the zero value), 
           it means that the actual frequency is lower than the target (and so, that the TRIM value should be 
           incremented), while when it is detected during the upcounting phase it means that the actual frequency 
           is higher (and that the TRIM value should be decremented).

      (#) To use IT mode, user needs to handle it in calling different macros available to do it
            (__HAL_RCC_CRS_XXX_IT). Interruptions will go through RCC Handler (RCC_IRQn/RCC_CRS_IRQHandler)
              (+++) Call function HAL_RCCEx_CRSConfig()
              (+++) Enable RCC_IRQn (thnaks to NVIC functions)
              (+++) Enable CRS IT (__HAL_RCC_CRS_ENABLE_IT)
              [+++) Implement CRS status management in RCC_CRS_IRQHandler

      (#) To force a SYNC EVENT, user can use function 'HAL_RCCEx_CRSSoftwareSynchronizationGenerate()'. Function can be 
            called before calling HAL_RCCEx_CRSConfig (for instance in Systick handler)
       
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

#ifdef HAL_RCC_MODULE_ENABLED

/** @addtogroup RCCEx 
  * @brief RCC Extension HAL module driver
  * @{
  */

/** @addtogroup RCCEx_Private
  * @{
  */
/* Bit position in register */
#define CRS_CFGR_FELIM_BITNUMBER    16U
#define CRS_CR_TRIM_BITNUMBER       8U
#define CRS_ISR_FECAP_BITNUMBER     16U

#if defined(USB)
extern const uint8_t PLLMulTable[];
#endif //USB
/**
  * @}
  */

/** @addtogroup RCCEx_Exported_Functions
  * @{
  */

/** @addtogroup RCCEx_Exported_Functions_Group1
 *  @brief  Extended Peripheral Initialization and Control functions  
 *
@verbatim   
 ===============================================================================
                      ##### Extended Peripheral Control functions #####
 ===============================================================================  
    [..]
    This subsection provides a set of functions allowing to control the RCC Clocks 
    frequencies.
      
@endverbatim
  * @{
  */

/**
  * @brief  Initializes the RCC extended peripherals clocks 
  * @note   Initializes the RCC extended peripherals clocks according to the specified parameters in the
  *         RCC_PeriphCLKInitTypeDef.
  * @note   If HAL_ERROR returned, first switch-OFF HSE clock oscillator with HAL_RCC_OscConfig()
  *         to possibly update HSE divider.
  * @param  PeriphClkInit: pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         contains the configuration information for the Extended Peripherals clocks(USART1,USART2, LPUART1, 
  *         I2C1, I2C3, RTC, USB/RNG  and LPTIM1 clocks).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_RCCEx_PeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
  uint32_t tickstart = 0U;
  uint32_t tmpreg = 0U;

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClkInit->PeriphClockSelection));

    /*---------------------------- RTC/LCD configuration -------------------------------*/
  if((((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_RTC) == RCC_PERIPHCLK_RTC)
#if defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
    || (((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LCD) == RCC_PERIPHCLK_LCD)
#endif /* defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) */
  )
  {
    /* Reset the Backup domain only if the RTC Clock source selection is modified */ 
    if( ((RCC->CR  & RCC_CR_RTCPRE) != (PeriphClkInit->RTCClockSelection & RCC_CR_RTCPRE))
#if defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
      || ((RCC->CR  & RCC_CR_RTCPRE)  != (PeriphClkInit->LCDClockSelection & RCC_CR_RTCPRE))
#endif /* defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) */
    )
    { /* Check HSE State */
      if (((PeriphClkInit->RTCClockSelection & RCC_CSR_RTCSEL) == RCC_CSR_RTCSEL_HSE) && HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY))
      {
         /*To update HSE divider, first switch-OFF HSE clock oscillator*/
         return HAL_ERROR; 
      }
    }
    
    /* Enable Power Clock*/
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Enable write access to Backup domain */
    SET_BIT(PWR->CR, PWR_CR_DBP);

    /* Wait for Backup domain Write protection disable */
    tickstart = HAL_GetTick();

    while((PWR->CR & PWR_CR_DBP) == RESET)
    {
      if((HAL_GetTick() - tickstart ) > RCC_DBP_TIMEOUT_VALUE)
      {
          return HAL_TIMEOUT;
      }
    }

    /* Reset the Backup domain only if the RTC Clock source selection is modified */ 
    if( ((RCC->CSR & RCC_CSR_RTCSEL) != (PeriphClkInit->RTCClockSelection & RCC_CSR_RTCSEL))
#if defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
      || ((RCC->CSR & RCC_CSR_RTCSEL) != (PeriphClkInit->LCDClockSelection & RCC_CSR_RTCSEL))
#endif /* defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) */
    )
    {
      /* Store the content of CSR register before the reset of Backup Domain */
      tmpreg = (RCC->CSR & ~(RCC_CSR_RTCSEL));
      /* RTC Clock selection can be changed only if the Backup Domain is reset */
      __HAL_RCC_BACKUPRESET_FORCE();
      __HAL_RCC_BACKUPRESET_RELEASE();
      /* Restore the Content of CSR register */
      RCC->CSR = tmpreg;

      /* Wait for LSERDY if LSE was enabled */
      if (HAL_IS_BIT_SET(tmpreg, RCC_CSR_LSERDY))
      {
        /* Get timeout */
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
   
      /* RTC Clock update*/
      __HAL_RCC_RTC_CONFIG(PeriphClkInit->RTCClockSelection);

    }
  }
  
#if !defined (STM32L011xx) && !defined (STM32L021xx) && !defined (STM32L031xx) && !defined (STM32L041xx)
  /*------------------------------- USART1 Configuration ------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USART1) == RCC_PERIPHCLK_USART1)
  {
    /* Check the parameters */
    assert_param(IS_RCC_USART1CLKSOURCE(PeriphClkInit->Usart1ClockSelection));
    
    /* Configure the USART1 clock source */
    __HAL_RCC_USART1_CONFIG(PeriphClkInit->Usart1ClockSelection);
  }
#endif
  
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

#if defined (STM32L071xx) || (STM32L072xx) || defined(STM32L073xx) || \
    defined(STM32L081xx) || defined(STM32L082xx) || defined(STM32L083xx)
    /*------------------------------ I2C3 Configuration ------------------------*/ 
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_I2C3) == RCC_PERIPHCLK_I2C3)
  {
    /* Check the parameters */
    assert_param(IS_RCC_I2C3CLKSOURCE(PeriphClkInit->I2c3ClockSelection));
    
    /* Configure the I2C3 clock source */
    __HAL_RCC_I2C3_CONFIG(PeriphClkInit->I2c3ClockSelection);
  }  
#endif /* defined (STM32L071xx) (STM32L072xx)|| (STM32L073xx)|| (STM32L081xx)|| (STM32L082xx) || (STM32L083xx) */

#if !defined (STM32L011xx) && !defined (STM32L021xx) && !defined (STM32L031xx) && !defined (STM32L041xx) && !defined (STM32L031xx) && !defined (STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx)  
 /*---------------------------- USB and RNG configuration --------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_USB) == (RCC_PERIPHCLK_USB))
  {
    assert_param(IS_RCC_USBCLKSOURCE(PeriphClkInit->UsbClockSelection));
    __HAL_RCC_USB_CONFIG(PeriphClkInit->UsbClockSelection);
  }
#endif
  
  /*---------------------------- LPTIM1 configuration ------------------------*/
  if(((PeriphClkInit->PeriphClockSelection) & RCC_PERIPHCLK_LPTIM1) == (RCC_PERIPHCLK_LPTIM1))
  {
    assert_param(IS_RCC_LPTIMCLK(PeriphClkInit->LptimClockSelection));
    __HAL_RCC_LPTIM1_CONFIG(PeriphClkInit->LptimClockSelection);
  }
  return HAL_OK;
}



/**
  * @brief  Get the RCC_ClkInitStruct according to the internal RCC configuration registers.
  * @param  PeriphClkInit: pointer to an RCC_PeriphCLKInitTypeDef structure that
  *         returns the configuration information for the Extended Peripherals clocks(USART1,USART2, LPUART1, 
  *         I2C1, I2C3, RTC, USB/RNG  and LPTIM1 clocks).
  * @retval None
  */
void HAL_RCCEx_GetPeriphCLKConfig(RCC_PeriphCLKInitTypeDef  *PeriphClkInit)
{
   /* Set all possible values for the extended clock type parameter -----------*/
  /* Common part first */
#if defined(STM32L011xx) || defined(STM32L021xx) || defined(STM32L031xx) || defined(STM32L041xx)   
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | RCC_PERIPHCLK_I2C1   | \
                                        RCC_PERIPHCLK_RTC    | RCC_PERIPHCLK_LPTIM1;
#endif
#if defined(STM32L052xx) || defined(STM32L062xx)
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   |  RCC_PERIPHCLK_RTC   | RCC_PERIPHCLK_USB     | \
                                        RCC_PERIPHCLK_LPTIM1 ;
#endif
#if  defined(STM32L053xx) || defined(STM32L063xx)
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   |  RCC_PERIPHCLK_RTC   | RCC_PERIPHCLK_USB     | \
                                        RCC_PERIPHCLK_LPTIM1 |  RCC_PERIPHCLK_LCD;
#endif
#if defined(STM32L072xx) || defined(STM32L082xx)
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C3   | RCC_PERIPHCLK_RTC     | \
                                        RCC_PERIPHCLK_USB    | RCC_PERIPHCLK_LPTIM1 ;
#endif
#if defined(STM32L073xx) || defined(STM32L083xx)
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C3   | RCC_PERIPHCLK_RTC     | \
                                        RCC_PERIPHCLK_USB    | RCC_PERIPHCLK_LPTIM1 |  RCC_PERIPHCLK_LCD;
  
#endif
#if defined(STM32L051xx) || defined(STM32L061xx)   
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_RTC    | RCC_PERIPHCLK_LPTIM1;
#endif
#if defined(STM32L071xx) || defined(STM32L081xx) 
  PeriphClkInit->PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | \
                                        RCC_PERIPHCLK_I2C1   | RCC_PERIPHCLK_I2C3   | RCC_PERIPHCLK_RTC     | \
                                        RCC_PERIPHCLK_LPTIM1;
#endif 

#if !defined (STM32L011xx) && !defined (STM32L021xx) && !defined (STM32L031xx) && !defined (STM32L041xx)
  /* Get the USART1 configuration --------------------------------------------*/
  PeriphClkInit->Usart1ClockSelection  = __HAL_RCC_GET_USART1_SOURCE();
#endif
  /* Get the USART2 clock source ---------------------------------------------*/
  PeriphClkInit->Usart2ClockSelection  = __HAL_RCC_GET_USART2_SOURCE();
  /* Get the LPUART1 clock source ---------------------------------------------*/
  PeriphClkInit->Lpuart1ClockSelection = __HAL_RCC_GET_LPUART1_SOURCE();
  /* Get the I2C1 clock source -----------------------------------------------*/
  PeriphClkInit->I2c1ClockSelection    = __HAL_RCC_GET_I2C1_SOURCE();
#if defined(STM32L071xx) || defined(STM32L072xx) || defined(STM32L073xx) || \
    defined(STM32L081xx) || defined(STM32L082xx) || defined(STM32L083xx)
/* Get the I2C3 clock source -----------------------------------------------*/
  PeriphClkInit->I2c3ClockSelection    = __HAL_RCC_GET_I2C3_SOURCE();
#endif /* defined (STM32L071xx) || (STM32L073xx) || (STM32L082xx) || (STM32L082xx) || (STM32L083xx)  */
  /* Get the LPTIM1 clock source -----------------------------------------------*/
  PeriphClkInit->LptimClockSelection   = __HAL_RCC_GET_LPTIM1_SOURCE();
  /* Get the RTC clock source -----------------------------------------------*/
  PeriphClkInit->RTCClockSelection     = __HAL_RCC_GET_RTC_SOURCE();
#if defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx)
  PeriphClkInit->LCDClockSelection = PeriphClkInit->RTCClockSelection;
#endif /* defined (STM32L053xx) || defined(STM32L063xx) || defined(STM32L073xx) || defined(STM32L083xx) */

#if !defined (STM32L011xx) && !defined (STM32L021xx) && !defined (STM32L031xx) && !defined (STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx)  
  /* Get the USB/RNG clock source -----------------------------------------------*/
  PeriphClkInit->UsbClockSelection  = __HAL_RCC_GET_USB_SOURCE();
#endif
}

/**
  * @brief  Return the peripheral clock frequency for some peripherals
  * @note   Return 0 if peripheral clock identifier not managed by this API
  * @param  PeriphClk: Peripheral clock identifier
  *         This parameter can be one of the following values:
  *            @arg RCC_PERIPHCLK_RTC: RTC peripheral clock
  *            @arg RCC_PERIPHCLK_LCD: LCD peripheral clock (*)
  *            @arg RCC_PERIPHCLK_USB: USB or RNG peripheral clock (*)
  *            @arg RCC_PERIPHCLK_USART1: USART1 peripheral clock (*)
  *            @arg RCC_PERIPHCLK_USART2: USART2 peripheral clock
  *            @arg RCC_PERIPHCLK_LPUART1: LPUART1 peripheral clock 
  *            @arg RCC_PERIPHCLK_I2C1: I2C1 peripheral clock 
  *            @arg RCC_PERIPHCLK_I2C2: I2C2 peripheral clock (*) 
  *            @arg RCC_PERIPHCLK_I2C3: I2C3 peripheral clock (*) 
  * @note   (*) means that this peripheral is not present on all the STM32L0xx devices
  * @retval Frequency in Hz (0: means that no available frequency for the peripheral)
  */
uint32_t HAL_RCCEx_GetPeriphCLKFreq(uint32_t PeriphClk)
{  
    uint32_t srcclk = 0U, clkprediv = 0U, frequency = 0U;
#if defined(USB)   
    uint32_t pllmul = 0U, plldiv = 0U, pllvco = 0U;
#endif /* USB */

  /* Check the parameters */
  assert_param(IS_RCC_PERIPHCLOCK(PeriphClk));

  switch(PeriphClk)
  {
   case RCC_PERIPHCLK_RTC:
    {  
      /* Get the current RTC source */
      srcclk = __HAL_RCC_GET_RTC_SOURCE();
     
      /* Check if LSE is ready and if RTC clock selection is LSE */
      if ((srcclk == RCC_RTCCLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Check if LSI is ready and if RTC clock selection is LSI */
      else if ((srcclk == RCC_RTCCLKSOURCE_LSI) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSIRDY)))
      {
        frequency = LSI_VALUE;
      }
      /* Check if HSE is ready and if RTC clock selection is HSE*/
      else if ((srcclk == RCC_RTCCLKSOURCE_HSE_DIVX) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY)))
      {
        /* Get the current HSE clock divider*/      
        clkprediv=__HAL_RCC_GET_RTC_HSE_PRESCALER();

        switch (clkprediv)
        {
          case RCC_RTC_HSE_DIV_16:  /* HSE DIV16 has been selected */
          {
            frequency = HSE_VALUE / 16U;
            break;
          }
          case RCC_RTC_HSE_DIV_8:   /* HSE DIV8 has been selected */
          {
            frequency = HSE_VALUE / 8U;
            break;
          }
          case RCC_RTC_HSE_DIV_4:   /* HSE DIV4 has been selected */
          {
            frequency = HSE_VALUE / 4U;
            break;
          }
          default:
          {
            frequency = HSE_VALUE / 2U;
            break;
          }
        }    
      } 
      /* Clock not enabled for RTC*/
      else
      {
        frequency = 0U;
      } 
      break;
   }
   
#if defined(LCD)
   
  case RCC_PERIPHCLK_LCD:
    {  
      /* Get the current LCD source */
      srcclk = __HAL_RCC_GET_LCD_SOURCE();

      /* Check if LSE is ready and if LCD clock selection is LSE */
      if ((srcclk == RCC_RTCCLKSOURCE_LSE) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSERDY)))
      {
        frequency = LSE_VALUE;
      }
      /* Check if LSI is ready and if LCD clock selection is LSI */
      else if ((srcclk == RCC_RTCCLKSOURCE_LSI) && (HAL_IS_BIT_SET(RCC->CSR, RCC_CSR_LSIRDY)))
      {
        frequency = LSI_VALUE;
      }
      /* Check if HSE is ready  and if LCD clock selection is HSE*/
      else if ((srcclk == RCC_RTCCLKSOURCE_HSE_DIVX) && (HAL_IS_BIT_SET(RCC->CR, RCC_CR_HSERDY)))
      {
        /* Get the current HSE clock divider*/     
        clkprediv=__HAL_RCC_GET_RTC_HSE_PRESCALER();
        
        switch (clkprediv)
        {
          case RCC_RTC_HSE_DIV_16:  /* HSE DIV16 has been selected */
          {
            frequency = HSE_VALUE / 16U;
            break;
          }
          case RCC_RTC_HSE_DIV_8:   /* HSE DIV8 has been selected */
          {
            frequency = HSE_VALUE / 8U;
            break;
          }
          case RCC_RTC_HSE_DIV_4:   /* HSE DIV4 has been selected */
          {
            frequency = HSE_VALUE / 4U;
            break;
          }
          default:
          {
            frequency = HSE_VALUE / 2U;
            break;
          }
        }       
      } 
      /* Clock not enabled for LCD*/
      else
      {
        frequency = 0U;
      }
      break;
   }    
#endif /* LCD */  


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
            pllmul = PLLMulTable[(pllmul >> 18U)];
            plldiv = (plldiv >> 22U) + 1U;   
            
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
#if defined(USART1)
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
#endif /* USART1 */
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

#if defined(I2C3)    
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
#endif /* I2C3 */      
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
  * @retval None
  */
void HAL_RCCEx_DisableLSECSS(void)
{
  /* Disable LSE CSS */
   CLEAR_BIT(RCC->CSR, RCC_CSR_LSECSSON) ;

  /* Disable LSE CSS IT */
  __HAL_RCC_DISABLE_IT(RCC_IT_CSSLSE);
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
  __HAL_RCC_ENABLE_IT(RCC_IT_CSSLSE);
  
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
  if(__HAL_RCC_GET_IT(RCC_IT_CSSLSE))
  {
    /* RCC LSE Clock Security System interrupt user callback */
    HAL_RCCEx_LSECSS_Callback();

    /* Clear RCC LSE CSS pending bit */
    __HAL_RCC_CLEAR_IT(RCC_IT_CSSLSE);
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

#if !defined (STM32L011xx) && !defined (STM32L021xx) && !defined (STM32L031xx) && !defined (STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx)
    
/**
  * @brief  Start automatic synchronization using polling mode
  * @param  pInit Pointer on RCC_CRSInitTypeDef structure
  * @retval None
  */
void HAL_RCCEx_CRSConfig(RCC_CRSInitTypeDef *pInit)
{
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

  /* Configure Synchronization input */
  /* Clear SYNCDIV[2:0], SYNCSRC[1:0] & SYNCSPOL bits */
  CRS->CFGR &= ~(CRS_CFGR_SYNCDIV | CRS_CFGR_SYNCSRC | CRS_CFGR_SYNCPOL);

  /* Set the CRS_CFGR_SYNCDIV[2:0] bits according to Prescaler value */
  CRS->CFGR |= pInit->Prescaler;

  /* Set the SYNCSRC[1:0] bits according to Source value */
  CRS->CFGR |= pInit->Source;

  /* Set the SYNCSPOL bits according to Polarity value */
  CRS->CFGR |= pInit->Polarity;

  /* Configure Frequency Error Measurement */
  /* Clear RELOAD[15:0] & FELIM[7:0] bits*/
  CRS->CFGR &= ~(CRS_CFGR_RELOAD | CRS_CFGR_FELIM);

  /* Set the RELOAD[15:0] bits according to ReloadValue value */
  CRS->CFGR |= pInit->ReloadValue;

  /* Set the FELIM[7:0] bits according to ErrorLimitValue value */
  CRS->CFGR |= (pInit->ErrorLimitValue << CRS_CFGR_FELIM_BITNUMBER);

  /* Adjust HSI48 oscillator smooth trimming */
  /* Clear TRIM[5:0] bits */
  CRS->CR &= ~CRS_CR_TRIM;

  /* Set the TRIM[5:0] bits according to RCC_CRS_HSI48CalibrationValue value */
  CRS->CR |= (pInit->HSI48CalibrationValue << CRS_CR_TRIM_BITNUMBER);


  /* START AUTOMATIC SYNCHRONIZATION*/
  
  /* Enable Automatic trimming */
  __HAL_RCC_CRS_AUTOMATIC_CALIB_ENABLE();

  /* Enable Frequency error counter */
  __HAL_RCC_CRS_FREQ_ERROR_COUNTER_ENABLE();

}

/**
  * @brief  Generate the software synchronization event
  * @retval None
  */
void HAL_RCCEx_CRSSoftwareSynchronizationGenerate(void)
{
  CRS->CR |= CRS_CR_SWSYNC;
}


/**
  * @brief  Function to return synchronization info 
  * @param  pSynchroInfo Pointer on RCC_CRSSynchroInfoTypeDef structure
  * @retval None
  */
void HAL_RCCEx_CRSGetSynchronizationInfo(RCC_CRSSynchroInfoTypeDef *pSynchroInfo)
{
  /* Check the parameter */
  assert_param(pSynchroInfo != NULL);
  
  /* Get the reload value */
  pSynchroInfo->ReloadValue = (uint32_t)(CRS->CFGR & CRS_CFGR_RELOAD);
  
  /* Get HSI48 oscillator smooth trimming */
  pSynchroInfo->HSI48CalibrationValue = (uint32_t)((CRS->CR & CRS_CR_TRIM) >> CRS_CR_TRIM_BITNUMBER);

  /* Get Frequency error capture */
  pSynchroInfo->FreqErrorCapture = (uint32_t)((CRS->ISR & CRS_ISR_FECAP) >> CRS_ISR_FECAP_BITNUMBER);

  /* Get Frequency error direction */
  pSynchroInfo->FreqErrorDirection = (uint32_t)(CRS->ISR & CRS_ISR_FEDIR);
  
  
}

/**
* @brief This function handles CRS Synchronization Timeout.
* @param Timeout: Duration of the timeout
* @note  Timeout is based on the maximum time to receive a SYNC event based on synchronization
*        frequency.
* @note    If Timeout set to HAL_MAX_DELAY, HAL_TIMEOUT will be never returned.
* @retval Combination of Synchronization status
*          This parameter can be a combination of the following values:
*            @arg RCC_CRS_TIMEOUT
*            @arg RCC_CRS_SYNCOK
*            @arg RCC_CRS_SYNCWARN
*            @arg RCC_CRS_SYNCERR
*            @arg RCC_CRS_SYNCMISS
*            @arg RCC_CRS_TRIMOVF
*/
uint32_t HAL_RCCEx_CRSWaitSynchronization(uint32_t Timeout)
{
  uint32_t crsstatus = RCC_CRS_NONE;
  uint32_t tickstart = 0U;
  
  /* Get timeout */
  tickstart = HAL_GetTick();
  
  /* Check that if one of CRS flags have been set */
  while(RCC_CRS_NONE == crsstatus)
  {
    if(Timeout != HAL_MAX_DELAY)
    {
      if((Timeout == 0U)||((HAL_GetTick() - tickstart ) > Timeout))
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
  }

  return crsstatus;
}         
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
#endif /* !defined (STM32L011xx) && !defined (STM32L021xx) && !defined (STM32L031xx) && !defined (STM32L041xx) && !defined(STM32L051xx) && !defined(STM32L061xx) && !defined(STM32L071xx) && !defined(STM32L081xx) */

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

