/**
  ******************************************************************************
  * @file    stm32g0xx_hal_pwr.c
  * @author  MCD Application Team
  * @brief   PWR HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Power Controller (PWR) peripheral:
  *           + Initialization/de-initialization functions
  *           + Peripheral Control functions
  *
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

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @addtogroup PWR
  * @{
  */

#ifdef HAL_PWR_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup PWR_Private_Defines PWR Private Defines
  * @{
  */

#if defined(PWR_PVD_SUPPORT)
/** @defgroup PWR_PVD_Mode_Mask PWR PVD Mode Mask
  * @{
  */
#define PVD_MODE_IT           0x00010000U  /*!< Mask for interruption yielded
                                                by PVD threshold crossing     */
#define PVD_MODE_EVT          0x00020000U  /*!< Mask for event yielded
                                                by PVD threshold crossing     */
#define PVD_RISING_EDGE       0x00000001U  /*!< Mask for rising edge set as
                                                PVD trigger                   */
#define PVD_FALLING_EDGE      0x00000002U  /*!< Mask for falling edge set as
                                                PVD trigger                   */
/**
  * @}
  */
#endif

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @addtogroup PWR_Exported_Functions  PWR Exported Functions
  * @{
  */

/** @addtogroup PWR_Exported_Functions_Group1  Initialization and de-initialization functions
  * @brief  Initialization and de-initialization functions
  *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]

@endverbatim
  * @{
  */

/**
  * @brief  Deinitialize the HAL PWR peripheral registers to their default reset
            values.
  * @retval None
  */
void HAL_PWR_DeInit(void)
{
  __HAL_RCC_PWR_FORCE_RESET();
  __HAL_RCC_PWR_RELEASE_RESET();
}

/**
  * @}
  */

/** @addtogroup PWR_Exported_Functions_Group2  Peripheral Control functions
  *  @brief Low Power modes configuration functions
  *
@verbatim

 ===============================================================================
                 ##### Peripheral Control functions #####
 ===============================================================================

    [..]
     *** PVD configuration ***
    =========================
    [..]
      (+) The PVD is used to monitor the VDD power supply by comparing it to a
          threshold selected by the PVD Level (PVDRT[2:0] & PVDFT[2:0] bits in
          PWR CR2 register).
      (+) PVDO flag is available to indicate if VDD/VDDA is higher or lower
          than the PVD threshold. This event is internally connected to the EXTI
          line 16 and can generate an interrupt if enabled.
      (+) The PVD is stopped in Standby & Shutdown mode.

    *** WakeUp pin configuration ***
    ================================
    [..]
      (+) WakeUp pins are used to wakeup the system from Standby mode or
          Shutdown mode. WakeUp pins polarity can be set to configure event
          detection on high level (rising edge) or low level (falling edge).

    *** Low Power mode configuration ***
    =====================================
    [..]
      The devices feature 7 low-power modes:
      (+) Low-power run mode: core and peripherals are running at low frequency.
          Regulator is in low power mode.
      (+) Sleep mode: Cortex-M0+ core stopped, peripherals kept running,
          regulator is main mode.
      (+) Low-power Sleep mode: Cortex-M0+ core stopped, peripherals kept running
          and regulator in low power mode.
      (+) Stop 0 mode: all clocks are stopped except LSI and LSE, regulator is
           main mode.
      (+) Stop 1 mode: all clocks are stopped except LSI and LSE, main regulator
          off, low power regulator on.
      (+) Standby mode: all clocks are stopped except LSI and LSE, regulator is
          disable.
      (+) Shutdown mode: all clocks are stopped except LSE, regulator is
          disable.

   *** Low-power run mode ***
   ==========================
    [..]
      (+) Entry: (from main run mode)
          (++) set LPR bit with HAL_PWREx_EnableLowPowerRunMode() API after
               having decreased the system clock below 2 MHz.
      (+) Exit:
          (++) clear LPR bit then wait for REGLPF bit to be reset with
               HAL_PWREx_DisableLowPowerRunMode() API. Only then can the
               system clock frequency be increased above 2 MHz.

   *** Sleep mode / Low-power sleep mode ***
   =========================================
    [..]
      (+) Entry:
          The Sleep & Low-power Sleep modes are entered through
          HAL_PWR_EnterSLEEPMode() API specifying whether or not the regulator
          is forced to low-power mode and if exit is interrupt or event
          triggered.
          (++) PWR_MAINREGULATOR_ON: Sleep mode (regulator in main mode).
          (++) PWR_LOWPOWERREGULATOR_ON: Low-power Sleep mode (regulator in low
               power mode). In this case, the system clock frequency must have
               been decreased below 2 MHz beforehand.
          (++) PWR_SLEEPENTRY_WFI: Core enters sleep mode with WFI instruction
          (++) PWR_SLEEPENTRY_WFE: Core enters sleep mode with WFE instruction
      (+) WFI Exit:
        (++) Any interrupt enabled in nested vectored interrupt controller (NVIC)
      (+) WFE Exit:
        (++) Any wakeup event if cortex is configured with SEVONPEND = 0
        (++) Interrupt even when disabled in NVIC if cortex is configured with
             SEVONPEND = 1
    [..]  When exiting the Low-power Sleep mode by issuing an interrupt or a wakeup event,
          the MCU is in Low-power Run mode.

   *** Stop 0 & Stop 1 modes ***
   =============================
    [..]
      (+) Entry:
          The Stop modes are entered through the following APIs:
          (++) HAL_PWR_EnterSTOPMode() with following settings:
              (+++) PWR_MAINREGULATOR_ON to enter STOP0 mode.
              (+++) PWR_LOWPOWERREGULATOR_ON to enter STOP1 mode.
      (+) Exit (interrupt or event-triggered, specified when entering STOP mode):
          (++) PWR_STOPENTRY_WFI: enter Stop mode with WFI instruction
          (++) PWR_STOPENTRY_WFE: enter Stop mode with WFE instruction
      (+) WFI Exit:
          (++) Any EXTI line (internal or external) configured in interrupt mode
               with corresponding interrupt enable in NVIC
      (+) WFE Exit:
          (++) Any EXTI line (internal or external) configured in event mode if
               cortex is configured with SEVONPEND = 0
          (++) Any EXTI line configured in interrupt mode (even if the
               corresponding EXTI Interrupt vector is disabled in the NVIC) if
               cortex is configured with SEVONPEND = 0. The interrupt source can
               be external interrupts or peripherals with wakeup capability.
    [..]  When exiting Stop, the MCU is either in Run mode or in Low-power Run mode
          depending on the LPR bit setting.

   *** Standby mode ***
   ====================
    [..] In Standby mode, it is possible to keep backup SRAM content (defined as
         full SRAM) keeping low power regulator on. This is achievable by setting
         Ram retention bit calling HAL_PWREx_EnableSRAMRetention API. This increases
         power consumption.
         Its also possible to define I/O states using APIs:
         HAL_PWREx_EnableGPIOPullUp, HAL_PWREx_EnableGPIOPullDown &
         HAL_PWREx_EnablePullUpPullDownConfig
      (+) Entry:
          (++) The Standby mode is entered through HAL_PWR_EnterSTANDBYMode() API, by
               setting SLEEPDEEP in Cortex control register.
      (+) Exit:
          (++) WKUP pin edge detection, RTC event (wakeup, alarm, timestamp),
               tamper event (internal & external), LSE CSS detection, reset on
               NRST pin, IWDG reset & BOR reset.
    [..] Exiting Standby generates a power reset: Cortex is reset and execute
         Reset handler vector, all registers in the Vcore domain are set to
         their reset value. Registers outside the VCORE domain (RTC, WKUP, IWDG,
         and Standby/Shutdown modes control) are not impacted.

    *** Shutdown mode ***
   ======================
    [..]
      In Shutdown mode,
        voltage regulator is disabled, all clocks are off except LSE, RRS bit is
        cleared. SRAM and registers contents are lost except for backup domain
        registers.
      (+) Entry:
          (++) The Shutdown mode is entered thru HAL_PWREx_EnterSHUTDOWNMode() API,
               by setting SLEEPDEEP in Cortex control register.
      (+) Exit:
          (++) WKUP pin edge detection, RTC event (wakeup, alarm, timestamp),
               tamper event (internal & external), LSE CSS detection, reset on
               NRST pin.
    [..] Exiting Shutdown generates a brown out reset: Cortex is reset and execute
         Reset handler vector, all registers are set to their reset value but ones
         in backup domain.

@endverbatim
  * @{
  */

/**
  * @brief  Enable access to the backup domain
  *         (RTC & TAMP registers, backup registers, RCC BDCR register).
  * @note   After reset, the backup domain is protected against
  *         possible unwanted write accesses. All RTC & TAMP registers (backup
  *         registers included) and RCC BDCR register are concerned.
  * @retval None
  */
void HAL_PWR_EnableBkUpAccess(void)
{
  SET_BIT(PWR->CR1, PWR_CR1_DBP);
}


/**
  * @brief  Disable access to the backup domain
  * @retval None
  */
void HAL_PWR_DisableBkUpAccess(void)
{
  CLEAR_BIT(PWR->CR1, PWR_CR1_DBP);
}


#if defined(PWR_PVD_SUPPORT)
/**
  * @brief  Configure the Power Voltage Detector (PVD).
  * @param  sConfigPVD pointer to a PWR_PVDTypeDef structure that contains the
            PVD configuration information: threshold levels, operating mode.
  * @note   Refer to the electrical characteristics of your device datasheet for
  *         more details about the voltage thresholds corresponding to each
  *         detection level.
  * @note   User should take care that rising threshold is higher than falling
  *         one in order to avoid having always PVDO output set.
  * @retval HAL_OK
  */
HAL_StatusTypeDef HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD)
{
  /* Check the parameters */
  assert_param(IS_PWR_PVD_LEVEL(sConfigPVD->PVDLevel));
  assert_param(IS_PWR_PVD_MODE(sConfigPVD->Mode));

  /* Set PVD level bits only according to PVDLevel value */
  MODIFY_REG(PWR->CR2, (PWR_CR2_PVDFT | PWR_CR2_PVDRT), sConfigPVD->PVDLevel);

  /* Clear any previous config, in case no event or IT mode is selected */
  __HAL_PWR_PVD_EXTI_DISABLE_EVENT();
  __HAL_PWR_PVD_EXTI_DISABLE_IT();
  __HAL_PWR_PVD_EXTI_DISABLE_FALLING_EDGE();
  __HAL_PWR_PVD_EXTI_DISABLE_RISING_EDGE();

  /* Configure interrupt mode */
  if((sConfigPVD->Mode & PVD_MODE_IT) == PVD_MODE_IT)
  {
    __HAL_PWR_PVD_EXTI_ENABLE_IT();
  }

  /* Configure event mode */
  if((sConfigPVD->Mode & PVD_MODE_EVT) == PVD_MODE_EVT)
  {
    __HAL_PWR_PVD_EXTI_ENABLE_EVENT();
  }

  /* Configure the edge */
  if((sConfigPVD->Mode & PVD_RISING_EDGE) == PVD_RISING_EDGE)
  {
    __HAL_PWR_PVD_EXTI_ENABLE_RISING_EDGE();
  }

  if((sConfigPVD->Mode & PVD_FALLING_EDGE) == PVD_FALLING_EDGE)
  {
    __HAL_PWR_PVD_EXTI_ENABLE_FALLING_EDGE();
  }

  return HAL_OK;
}


/**
  * @brief  Enable the Power Voltage Detector (PVD).
  * @retval None
  */
void HAL_PWR_EnablePVD(void)
{
  SET_BIT(PWR->CR2, PWR_CR2_PVDE);
}


/**
  * @brief  Disable the Power Voltage Detector (PVD).
  * @retval None
  */
void HAL_PWR_DisablePVD(void)
{
  CLEAR_BIT(PWR->CR2, PWR_CR2_PVDE);
}
#endif

/**
  * @brief  Enable the WakeUp PINx functionality.
  * @param  WakeUpPinPolarity Specifies which Wake-Up pin to enable.
  *         This parameter can be one of the following legacy values which set
  *         the default polarity i.e. detection on high level (rising edge):
  *           @arg @ref PWR_WAKEUP_PIN1, PWR_WAKEUP_PIN2, PWR_WAKEUP_PIN4,
  *                PWR_WAKEUP_PIN5,PWR_WAKEUP_PIN6
  *         or one of the following value where the user can explicitly specify
  *         the enabled pin and the chosen polarity:
  *           @arg @ref PWR_WAKEUP_PIN1_HIGH or PWR_WAKEUP_PIN1_LOW
  *           @arg @ref PWR_WAKEUP_PIN2_HIGH or PWR_WAKEUP_PIN2_LOW
  *           @arg @ref PWR_WAKEUP_PIN4_HIGH or PWR_WAKEUP_PIN4_LOW
  *           @arg @ref PWR_WAKEUP_PIN5_HIGH or PWR_WAKEUP_PIN5_LOW
  *           @arg @ref PWR_WAKEUP_PIN6_HIGH or PWR_WAKEUP_PIN6_LOW
  * @note  PWR_WAKEUP_PINx and PWR_WAKEUP_PINx_HIGH are equivalent.
  * @retval None
  */
void HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinPolarity)
{
  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinPolarity));

  /* Specifies the Wake-Up pin polarity for the event detection
    (rising or falling edge) */
  MODIFY_REG(PWR->CR4, (PWR_CR4_WP & WakeUpPinPolarity), (WakeUpPinPolarity >> PWR_WUP_POLARITY_SHIFT));

  /* Enable wake-up pin */
  SET_BIT(PWR->CR3, (PWR_CR3_EWUP & WakeUpPinPolarity));
}


/**
  * @brief  Disable the WakeUp PINx functionality.
  * @param  WakeUpPinx Specifies the Power Wake-Up pin to disable.
  *         This parameter can be one of the following values:
  *           @arg @ref PWR_WAKEUP_PIN1, PWR_WAKEUP_PIN2, PWR_WAKEUP_PIN4,
  *                PWR_WAKEUP_PIN5,PWR_WAKEUP_PIN6
  * @retval None
  */
void HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx)
{
  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinx));

  CLEAR_BIT(PWR->CR3, (PWR_CR3_EWUP & WakeUpPinx));
}


/**
  * @brief  Enter Sleep or Low-power Sleep mode.
  * @note   In Sleep/Low-power Sleep mode, all I/O pins keep the same state as
  *         in Run mode.
  * @param  Regulator Specifies the regulator state in Sleep/Low-power Sleep
  *         mode. This parameter can be one of the following values:
  *           @arg @ref PWR_MAINREGULATOR_ON Sleep mode (regulator in main mode)
  *           @arg @ref PWR_LOWPOWERREGULATOR_ON Low-power Sleep mode (regulator
  *                     in low-power mode)
  * @note   Low-power Sleep mode is entered from Low-power Run mode only. In
  *         case Regulator parameter is set to Low Power but MCU is in Run mode,
  *         we will first enter in Low-power Run mode. Therefore, user should
  *         take care that HCLK frequency is less than 2 MHz.
  * @note   When exiting Low-power Sleep mode, the MCU is in Low-power Run mode.
  *         To switch back to Run mode, user must call
  *         HAL_PWREx_DisableLowPowerRunMode() API.
  * @param  SLEEPEntry Specifies if Sleep mode is entered with WFI or WFE
  *         instruction. This parameter can be one of the following values:
  *           @arg @ref PWR_SLEEPENTRY_WFI enter Sleep or Low-power Sleep
  *                     mode with WFI instruction
  *           @arg @ref PWR_SLEEPENTRY_WFE enter Sleep or Low-power Sleep
  *                     mode with WFE instruction
  * @note   When WFI entry is used, tick interrupt have to be disabled if not
  *         desired as the interrupt wake up source.
  * @retval None
  */
void HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry)
{
  /* Check the parameters */
  assert_param(IS_PWR_REGULATOR(Regulator));
  assert_param(IS_PWR_SLEEP_ENTRY(SLEEPEntry));

  /* Set Regulator parameter */
  if(Regulator != PWR_MAINREGULATOR_ON)
  {
    /* If in run mode, first move to low-power run mode.
       The system clock frequency must be below 2 MHz at this point. */
    if((PWR->SR2 & PWR_SR2_REGLPF) == 0x00u)
    {
      HAL_PWREx_EnableLowPowerRunMode();
    }
  }
  else
  {
    /* If in low-power run mode at this point, exit it */
    if((PWR->SR2 & PWR_SR2_REGLPF) != 0x00u)
    {
      if (HAL_PWREx_DisableLowPowerRunMode() != HAL_OK)
      {
        return ;
      }
    }
  }

  /* Clear SLEEPDEEP bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

  /* Select SLEEP mode entry -------------------------------------------------*/
  if(SLEEPEntry == PWR_SLEEPENTRY_WFI)
  {
    /* Request Wait For Interrupt */
    __WFI();
  }
  else
  {
    /* Request Wait For Event */
    __SEV();
    __WFE();
    __WFE();
  }
}


/**
  * @brief  Enter Stop mode
  * @note   This API is named HAL_PWR_EnterSTOPMode to ensure compatibility with
  *         legacy code running on devices where only "Stop mode" is mentioned
  *         with main or low power regulator ON.
  * @note   In Stop mode, all I/O pins keep the same state as in Run mode.
  * @note   All clocks in the VCORE domain are stopped; the PLL, the HSI and the
  *         HSE oscillators are disabled. Some peripherals with the wakeup
  *         capability can switch on the HSI to receive a frame, and switch off
  *         the HSI after receiving the frame if it is not a wakeup frame.
  *         SRAM and register contents are preserved.
  *         The BOR is available.
  *         The voltage regulator can be configured either in normal (Stop 0) or
  *         low-power mode (Stop 1).
  * @note   When exiting Stop 0 or Stop 1 mode by issuing an interrupt or a
  *         wakeup event, the HSI RC oscillator is selected as system clock
  * @note   When the voltage regulator operates in low power mode (Stop 1),
  *         an additional startup delay is incurred when waking up. By keeping
  *         the internal regulator ON during Stop mode (Stop 0), the consumption
  *         is higher although the startup time is reduced.
  * @param  Regulator Specifies the regulator state in Stop mode
  *         This parameter can be one of the following values:
  *            @arg @ref PWR_MAINREGULATOR_ON  Stop 0 mode (main regulator ON)
  *            @arg @ref PWR_LOWPOWERREGULATOR_ON  Stop 1 mode (low power
  *                                                regulator ON)
  * @param  STOPEntry Specifies Stop 0 or Stop 1 mode is entered with WFI or
  *         WFE instruction. This parameter can be one of the following values:
  *            @arg @ref PWR_STOPENTRY_WFI  Enter Stop 0 or Stop 1 mode with WFI
  *                                         instruction.
  *            @arg @ref PWR_STOPENTRY_WFE  Enter Stop 0 or Stop 1 mode with WFE
  *                                         instruction.
  * @retval None
  */
void HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry)
{
  /* Check the parameters */
  assert_param(IS_PWR_REGULATOR(Regulator));
  assert_param(IS_PWR_STOP_ENTRY(STOPEntry));

  if (Regulator != PWR_MAINREGULATOR_ON)
  {
    /* Stop mode with Low-Power Regulator */
    MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_STOP1);
  }
  else
  {
    /* Stop mode with Main Regulator */
    MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_STOP0);
  }

  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

  /* Select Stop mode entry --------------------------------------------------*/
  if(STOPEntry == PWR_STOPENTRY_WFI)
  {
    /* Request Wait For Interrupt */
    __WFI();
  }
  else
  {
    /* Request Wait For Event */
    __SEV();
    __WFE();
    __WFE();
  }

  /* Reset SLEEPDEEP bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));
}


/**
  * @brief  Enter Standby mode.
  * @note   In Standby mode, the PLL, the HSI and the HSE oscillators are
  *         switched off. The voltage regulator is disabled. SRAM and register
  *         contents are lost except for registers in the Backup domain and
  *         Standby circuitry. BOR is available.
  * @note   The I/Os can be configured either with a pull-up or pull-down or can
  *         be kept in analog state.
  *         HAL_PWREx_EnableGPIOPullUp() and HAL_PWREx_EnableGPIOPullDown()
  *         respectively enable Pull Up and PullDown state.
  *         HAL_PWREx_DisableGPIOPullUp() & HAL_PWREx_DisableGPIOPullDown()
  *         disable the same. These states are effective in Standby mode only if
  *         APC bit is set through HAL_PWREx_EnablePullUpPullDownConfig() API.
  * @note   Sram content can be kept setting RRS through HAL_PWREx_EnableSRAMRetention()
  * @retval None
  */
void HAL_PWR_EnterSTANDBYMode(void)
{
  /* Set Stand-by mode */
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_STANDBY);

  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

/* This option is used to ensure that store operations are completed */
#if defined ( __CC_ARM)
  __force_stores();
#endif

  /* Request Wait For Interrupt */
  __WFI();
}


/**
  * @brief  Enable Sleep-On-Exit Cortex feature
  * @note   Set SLEEPONEXIT bit of SCR register. When this bit is set, the
  *         processor enters SLEEP or DEEPSLEEP mode when an interruption
  *         handling is over returning to thread mode. Setting this bit is
  *         useful when the processor is expected to run only on interruptions
  *         handling.
  * @retval None
  */
void HAL_PWR_EnableSleepOnExit(void)
{
  /* Set SLEEPONEXIT bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
}


/**
  * @brief  Disable Sleep-On-Exit Cortex feature
  * @note   Clear SLEEPONEXIT bit of SCR register. When this bit is set, the
  *         processor enters SLEEP or DEEPSLEEP mode when an interruption
  *         handling is over.
  * @retval None
  */
void HAL_PWR_DisableSleepOnExit(void)
{
  /* Clear SLEEPONEXIT bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
}


/**
  * @brief  Enable Cortex Sev On Pending feature.
  * @note   Set SEVONPEND bit of SCR register. When this bit is set, enabled
  *         events and all interrupts, including disabled ones can wakeup
  *         processor from WFE.
  * @retval None
  */
void HAL_PWR_EnableSEVOnPend(void)
{
  /* Set SEVONPEND bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk));
}


/**
  * @brief  Disable Cortex Sev On Pending feature.
  * @note   Clear SEVONPEND bit of SCR register. When this bit is clear, only
  *         enable interrupts or events can wakeup processor from WFE
  * @retval None
  */
void HAL_PWR_DisableSEVOnPend(void)
{
  /* Clear SEVONPEND bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk));
}

#if defined(PWR_PVD_SUPPORT)
/**
  * @brief  This function handles the PWR PVD interrupt request.
  * @note   This API should be called under the PVD_IRQHandler().
  * @retval None
  */
void HAL_PWR_PVD_IRQHandler(void)
{
  /* Check PWR exti Rising flag */
  if(__HAL_PWR_PVD_EXTI_GET_RISING_FLAG() != 0x0U)
  {
    /* Clear PVD exti pending bit */
    __HAL_PWR_PVD_EXTI_CLEAR_RISING_FLAG();

    /* PWR PVD interrupt rising user callback */
    HAL_PWR_PVD_Rising_Callback();
  }

  /* Check PWR exti fallling flag */
  if(__HAL_PWR_PVD_EXTI_GET_FALLING_FLAG() != 0x0U)
  {
    /* Clear PVD exti pending bit */
    __HAL_PWR_PVD_EXTI_CLEAR_FALLING_FLAG();

    /* PWR PVD interrupt falling user callback */
    HAL_PWR_PVD_Falling_Callback();
  }
}

/**
  * @brief  PWR PVD interrupt rising callback
  * @retval None
  */
__weak void HAL_PWR_PVD_Rising_Callback(void)
{
  /* NOTE : This function should not be modified; when the callback is needed,
            the HAL_PWR_PVD_Rising_Callback can be implemented in the user file
  */
}

/**
  * @brief  PWR PVD interrupt Falling callback
  * @retval None
  */
__weak void HAL_PWR_PVD_Falling_Callback(void)
{
  /* NOTE : This function should not be modified; when the callback is needed,
            the HAL_PWR_PVD_Falling_Callback can be implemented in the user file
  */
}

#endif

/**
  * @}
  */

/**
  * @}
  */

#endif /* HAL_PWR_MODULE_ENABLED */
/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
