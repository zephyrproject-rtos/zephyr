/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_pwr.c
  * @author  MCD Application Team
  * @brief   PWR HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Power Controller (PWR) peripheral:
  *           + Initialization and de-initialization functions
  *           + Peripheral Control functions
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
#include "stm32mp1xx_hal.h"



/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup PWR PWR
  * @brief PWR HAL module driver
  * @{
  */

#ifdef HAL_PWR_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup PWR_Private_Constants PWR Private Constants
  * @{
  */

/** @defgroup PWR_PVD_Mode_Mask PWR PVD Mode Mask
  * @{
  */
#define PVD_MODE_IT              ((uint32_t)0x00010000U)
#define PVD_RISING_EDGE          ((uint32_t)0x00000001U)
#define PVD_FALLING_EDGE         ((uint32_t)0x00000002U)
#define PVD_RISING_FALLING_EDGE  ((uint32_t)0x00000003U)
/**
  * @}
  */

/**
  * @}
  */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup PWR_Private_Functions PWR Private Functions
  * @{
  */

/** @defgroup PWR_Group1 Initialization and de-initialization functions
  *  @brief    Initialization and de-initialization functions
  *
@verbatim
 ===============================================================================
              ##### Initialization and de-initialization functions #####
 ===============================================================================
    [..]
      After reset, the backup domain (RTC registers, RTC backup data
      registers and backup SRAM) is protected against possible unwanted
      write accesses.
      To enable access to the RTC Domain and RTC registers, proceed as follows:
        (+) Enable access to RTC domain using the HAL_PWR_EnableBkUpAccess()
            function.

@endverbatim
  * @{
  */


/**
  * @brief  Enables access to the backup domain
  *         In reset state, the RCC_BDCR, PWR_CR2, RTC, and backup registers are
  *         protected against parasitic write access. DBP bit must be set to
  *         enable write access to these.
  * @note   If the HSE divided by 2, 3, ..31 is used as the RTC clock, the
  *         Backup Domain Access should be kept enabled.
  * @retval None
  */
void HAL_PWR_EnableBkUpAccess(void)
{
  /* Enable access to RTC and backup registers */
  SET_BIT(PWR->CR1, PWR_CR1_DBP);
}

/**
  * @brief Disables access to the backup domain (RTC registers, RTC
  *         backup data registers and backup SRAM).
  * @note If the HSE divided by 2, 3, ..31 is used as the RTC clock, the
  *         Backup Domain Access should be kept enabled.
  * @retval None
  */
void HAL_PWR_DisableBkUpAccess(void)
{
  /* Disable access to RTC and backup registers */
  CLEAR_BIT(PWR->CR1, PWR_CR1_DBP);
}

/**
  * @}
  */

/** @defgroup PWR_Group2 Peripheral Control functions
  *  @brief Low Power modes configuration functions
  *
@verbatim

 ===============================================================================
                 ##### Peripheral Control functions #####
 ===============================================================================

    *** PVD configuration ***
    =========================
    [..]
      (+) The PVD is used to monitor the VDD power supply by comparing it to a
          threshold selected by the PVD Level (PLS[2:0] bits in the PWR_CR1).
      (+) The PVD can also be used to monitor a voltage level on the PVD_IN pin.
          In this case, the voltage level on PVD_IN is compared to the internal
          VREFINT level.
      (+) A PVDO flag is available, in the PWR control status register 1
          (PWR_CSR1), to indicate whether VDD or voltage level on PVD_IN is
          higher or lower than the PVD threshold. This event is internally
          connected to the EXTI line16 and can generate an interrupt if enabled.
          This is done through __HAL_PWR_PVD_AVD_EXTI_ENABLE_IT() macro.
      (+) The PVD is stopped in Standby mode.

    *** WakeUp pins configuration ***
    ================================
    [..]
      (+) WakeUp pins are used to wake up the system from Standby mode. WKUP
          pins, if enabled, the WKUP pin pulls can be configured by WKUPPUPD
          register bits in PWR wakeup control register (PWR_WKUPCR).

    *** Low Power modes configuration ***
    =====================================
    [..]
      Several Low-power modes are available to save power when the MPU and/or
      MCU do not need to execute code (i.e. when waiting for an external event).
      Please refer to Reference Manual for more information.
      MPU and MCU sub-system modes:
      (+) CSleep mode: MPU/MCU clocks stopped and the MPU/MCU sub-system allocated
                       peripheral(s) clocks operate according to RCC PERxLPEN
      (+) CStop mode: MPU/MCU and MPU/MCU sub-system peripheral(s) clock stopped
      (+) CStandby mode: MPU and MPU sub-system peripheral(s) clock stopped and
                         wakeup via reset
      System modes:
      (+) Stop mode: bus matrix clocks stalled, the oscillators can be stopped.
                     VDDCORE is supplied.
                     To enter into this mode:
                     - Both MPU and MCU sub-systems are in CStop or CStandby.
                     - At least one PDDS bit (PWR_MPUCR/PWR_MCUCR) selects Stop.
      (+) LP-Stop mode: bus matrix clocks stalled, the oscillators can be stopped.
                        VDDCORE is supplied.
                        To enter into this mode:
                        - Both MPU and MCU sub-systems are in CStop or CStandby.
                        - The LPDS bit (PWR_CR1) selects LP-Stop.
                        - The LVDS bit (PWR_CR1) selects normal voltage.
                        - At least one PDDS bit (PWR_MPUCR/PWR_MCUCR) selects Stop.
      (+) LPLV-Stop mode: bus matrix clocks stalled, the oscillators can be stopped.
                          VDDCORE may be supplied at a lower level.
                          To enter into this mode:
                          - Both MPU and MCU sub-systems are in CStop or CStandby.
                          - The LPDS and LVDS bits (PWR_CR1) select LPLV-Stop.
                          - At least one PDDS bit (PWR_MPUCR/PWR_MCUCR) selects Stop.
      (+) Standby mode: System powered down.
                        To enter into this mode:
                        - MPU sub-system is in CStandby or CStop with CSTBYDIS = 1
                          and MCU subsystem is in CStop.
                        - All PDDS bits (PWR_MPUCR/PWR_MCUCR) select Standby.

   *** CSleep mode ***
   ==================
    [..]
      (+) Entry:
        The CSleep mode is entered by using the HAL_PWR_EnterSLEEPMode(Regulator, SLEEPEntry)
        functions with:
        (++) STOPEntry:
          (++) PWR_SLEEPENTRY_WFI: enter SLEEP mode with WFI instruction
          (++) PWR_SLEEPENTRY_WFE: enter SLEEP mode with WFE instruction

      -@@- The Regulator parameter is not used for the STM32MP1 family
           and is kept as parameter just to maintain compatibility with the
           lower power families (STM32L). Use i.e. PWR_MAINREGULATOR_ON
      (+) Exit:
          MCU: Any peripheral interrupt acknowledged by the nested vectored interrupt
          controller (NVIC) if entered by WFI or any event if entered by WFE
          MPU: Any Interrupt enabled in GIC if entered by WFI or any event if
          entered by WFE


   *** CStop mode ***
   =================
    [..]
      (+) Entry:
         The CStop mode is entered using the HAL_PWR_EnterSTOP(ModeRegulator, STOPEntry)
         function with:
         (++) Regulator:
          (+++) PWR_MAINREGULATOR_ON: Main regulator ON.
          (+++) PWR_LOWPOWERREGULATOR_ON: Low Power regulator ON.
         (++) STOPEntry:
          (+++) PWR_STOPENTRY_WFI: enter STOP mode with WFI instruction
          (+++) PWR_STOPENTRY_WFE: enter STOP mode with WFE instruction
      (+) Exit:
          MCU: Any EXTI Line (Internal or External) configured in Interrupt/Event mode
          depending of entry mode.
          MPU: Any EXTI Line (Internal or External) configured in Interrupt mode

   *** MPU CStandby mode / MCU CStop allowing Standby mode ***
   ====================
    [..]
    (+)
      The Standby mode allows to achieve the lowest power consumption. On Standby
      mode the voltage regulator is disabled. The PLLs, the HSI oscillator and
      the HSE oscillator are also switched off. SRAM and register contents are
      lost except for the RTC registers, RTC backup registers, backup SRAM and
      Standby circuitry.

      (++) Entry:
        (+++) The MPU CStandby mode / MCU CStop allowing Standby mode is entered
              using the HAL_PWR_EnterSTANDBYMode() function.
      (++) Exit:
           Any EXTI Line (Internal or External) configured in Interrupt mode
           if system is not in Standby mode
        (+++) If system is in Standby mode wake up is generated by reset through:
              WKUP pins, RTC alarm (Alarm A and Alarm B), RTC wakeup, tamper
              event, time-stamp event, external reset in NRST pin, IWDG reset.

@endverbatim
  * @{
  */

/**
  * @brief  Configures the voltage threshold detected by the Power Voltage Detector(PVD).
  * @param  sConfigPVD: pointer to an PWR_PVDTypeDef structure that contains the configuration
  *         information for the PVD.
  * @note   Refer to the electrical characteristics of your device datasheet for
  *         more details about the voltage threshold corresponding to each
  *         detection level.
  * @retval None
  */
void HAL_PWR_ConfigPVD(PWR_PVDTypeDef *sConfigPVD)
{
  /* Check the parameters */
  assert_param(IS_PWR_PVD_LEVEL(sConfigPVD->PVDLevel));
  assert_param(IS_PWR_PVD_MODE(sConfigPVD->Mode));

  /* Set PLS[7:5] bits according to PVDLevel value */
  MODIFY_REG(PWR->CR1, PWR_CR1_PLS, sConfigPVD->PVDLevel);

  /* Clear any previous config. Keep it clear if no IT mode is selected */
  __HAL_PWR_PVD_AVD_EXTI_DISABLE_IT();
  __HAL_PWR_PVD_AVD_EXTI_DISABLE_RISING_FALLING_EDGE();

  /* Configure interrupt mode */
  if ((sConfigPVD->Mode & PVD_MODE_IT) == PVD_MODE_IT)
  {
    __HAL_PWR_PVD_AVD_EXTI_ENABLE_IT();
  }

  /* Configure the edge */
  if ((sConfigPVD->Mode & PVD_RISING_EDGE) == PVD_RISING_EDGE)
  {
    __HAL_PWR_PVD_AVD_EXTI_ENABLE_RISING_EDGE();
  }

  if ((sConfigPVD->Mode & PVD_FALLING_EDGE) == PVD_FALLING_EDGE)
  {
    __HAL_PWR_PVD_AVD_EXTI_ENABLE_FALLING_EDGE();
  }
}

/**
  * @brief Enables the Power Voltage Detector(PVD).
  * @retval None
  */
void HAL_PWR_EnablePVD(void)
{
  /* Enable the power voltage detector */
  SET_BIT(PWR->CR1, PWR_CR1_PVDEN);
}

/**
  * @brief Disables the Power Voltage Detector(PVD).
  * @retval None
  */
void HAL_PWR_DisablePVD(void)
{
  /* Disable the power voltage detector */
  CLEAR_BIT(PWR->CR1, PWR_CR1_PVDEN);
}

/**
  * @brief Enable the WakeUp PINx functionality.
  * @param WakeUpPinPolarity: Specifies which Wake-Up pin to enable.
  *         This parameter can be one of the following legacy values, which sets the default polarity:
  *         detection on high level (rising edge):
  *           @arg PWR_WAKEUP_PIN1, PWR_WAKEUP_PIN2, PWR_WAKEUP_PIN3, PWR_WAKEUP_PIN4, PWR_WAKEUP_PIN5, PWR_WAKEUP_PIN6
  *                or one of the following value where the user can explicitly states the enabled pin and
  *                the chosen polarity
  *           @arg PWR_WAKEUP_PIN1_HIGH or PWR_WAKEUP_PIN1_LOW or
  *           @arg PWR_WAKEUP_PIN2_HIGH or PWR_WAKEUP_PIN2_LOW
  *           @arg PWR_WAKEUP_PIN3_HIGH or PWR_WAKEUP_PIN3_LOW
  *           @arg PWR_WAKEUP_PIN4_HIGH or PWR_WAKEUP_PIN4_LOW
  *           @arg PWR_WAKEUP_PIN5_HIGH or PWR_WAKEUP_PIN5_LOW
  *           @arg PWR_WAKEUP_PIN6_HIGH or PWR_WAKEUP_PIN6_LOW
  * @note  PWR_WAKEUP_PINx and PWR_WAKEUP_PINx_HIGH are equivalent.
  *
  * @note  GPIOs are set as next when WKUP pin is enabled (Additional function)
  *         WKUP1 :  PA0
  *         WKUP2 :  PA2
  *         WKUP3 :  PC13
  *         WKUP4 :  PI8
  *         WKUP5 :  PI11
  *         WKUP6 :  PC1
  * @retval None
  */
void HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinPolarity)
{
  uint32_t clear_mask = 0;

  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinPolarity));

  if ((WakeUpPinPolarity & PWR_WAKEUP_PIN1) == PWR_WAKEUP_PIN1)
  {
    clear_mask  |= (PWR_WKUPCR_WKUPP_1 | PWR_WKUPCR_WKUPPUPD1);
  }
  if ((WakeUpPinPolarity & PWR_WAKEUP_PIN2) == PWR_WAKEUP_PIN2)
  {
    clear_mask  |= (PWR_WKUPCR_WKUPP_2 | PWR_WKUPCR_WKUPPUPD2);
  }
  if ((WakeUpPinPolarity & PWR_WAKEUP_PIN3) == PWR_WAKEUP_PIN3)
  {
    clear_mask  |= (PWR_WKUPCR_WKUPP_3 | PWR_WKUPCR_WKUPPUPD3);
  }
  if ((WakeUpPinPolarity & PWR_WAKEUP_PIN4) == PWR_WAKEUP_PIN4)
  {
    clear_mask  |= (PWR_WKUPCR_WKUPP_4 | PWR_WKUPCR_WKUPPUPD4);
  }
  if ((WakeUpPinPolarity & PWR_WAKEUP_PIN5) == PWR_WAKEUP_PIN5)
  {
    clear_mask  |= (PWR_WKUPCR_WKUPP_5 | PWR_WKUPCR_WKUPPUPD5);
  }
  if ((WakeUpPinPolarity & PWR_WAKEUP_PIN6) == PWR_WAKEUP_PIN6)
  {
    clear_mask  |= (PWR_WKUPCR_WKUPP_6 | PWR_WKUPCR_WKUPPUPD6);
  }

  /* Enables and Specifies the Wake-Up pin polarity and the pull configuration
     for the event detection (rising or falling edge) */
#ifdef CORE_CA7
  CLEAR_BIT(PWR->MPUWKUPENR, (WakeUpPinPolarity & PWR_WAKEUP_PIN_MASK)); /* Disable WKUP pin */
  MODIFY_REG(PWR->WKUPCR, clear_mask,
             (WakeUpPinPolarity & (PWR_WKUPCR_WKUPP | PWR_WKUPCR_WKUPPUPD))); /* Modify polarity and pull configuration */
  SET_BIT(PWR->WKUPCR, (WakeUpPinPolarity & PWR_WKUPCR_WKUPC)); /* Clear wake up flag */
  SET_BIT(PWR->MPUWKUPENR, (WakeUpPinPolarity & PWR_WAKEUP_PIN_MASK)); /* Enable the Wake up pin for CPU1 */
#endif
#ifdef CORE_CM4
  CLEAR_BIT(PWR->MCUWKUPENR, (WakeUpPinPolarity & PWR_WAKEUP_PIN_MASK)); /* Disable WKUP pin */
  MODIFY_REG(PWR->WKUPCR, clear_mask,
             (WakeUpPinPolarity & (PWR_WKUPCR_WKUPP | PWR_WKUPCR_WKUPPUPD))); /* Modify polarity and pull configuration */
  SET_BIT(PWR->WKUPCR, (WakeUpPinPolarity & PWR_WKUPCR_WKUPC)); /* Clear wake up flag */
  SET_BIT(PWR->MCUWKUPENR, (WakeUpPinPolarity & PWR_WAKEUP_PIN_MASK)); /* Enable the Wake up pin for CPU2 */
#endif
}


/**
  * @brief Disables the WakeUp PINx functionality.
  * @param WakeUpPinx: Specifies the Power Wake-Up pin to disable.
  *         This parameter can be one of the following values:
  *           @arg PWR_WAKEUP_PIN1
  *           @arg PWR_WAKEUP_PIN2
  *           @arg PWR_WAKEUP_PIN3
  *           @arg PWR_WAKEUP_PIN4
  *           @arg PWR_WAKEUP_PIN5
  *           @arg PWR_WAKEUP_PIN6
  * @retval None
  */
void HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx)
{
  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinx));
#ifdef CORE_CA7
  CLEAR_BIT(PWR->MPUWKUPENR, (WakeUpPinx & PWR_WAKEUP_PIN_MASK));
#endif
#ifdef CORE_CM4
  CLEAR_BIT(PWR->MCUWKUPENR, (WakeUpPinx & PWR_WAKEUP_PIN_MASK));
#endif
}


/**
  * @brief Enable WakeUp PINx Interrupt on AIEC and NVIC.
  * @param WakeUpPinx: Specifies the Power Wake-Up pin
  *         This parameter can be one of the following values:
  *           @arg PWR_WAKEUP_PIN1
  *           @arg PWR_WAKEUP_PIN2
  *           @arg PWR_WAKEUP_PIN3
  *           @arg PWR_WAKEUP_PIN4
  *           @arg PWR_WAKEUP_PIN5
  *           @arg PWR_WAKEUP_PIN6
  * @retval None
  */
void HAL_PWR_EnableWakeUpPinIT(uint32_t WakeUpPinx)
{
  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinx));
  switch ((WakeUpPinx & PWR_WAKEUP_PIN_MASK))
  {
    case PWR_WAKEUP_PIN1:
      __HAL_WKUP_EXTI_ENABLE_IT(EXTI_IMR2_IM55);
      break;

    case PWR_WAKEUP_PIN2:
      __HAL_WKUP_EXTI_ENABLE_IT(EXTI_IMR2_IM56);
      break;

    case PWR_WAKEUP_PIN3:
      __HAL_WKUP_EXTI_ENABLE_IT(EXTI_IMR2_IM57);
      break;

    case PWR_WAKEUP_PIN4:
      __HAL_WKUP_EXTI_ENABLE_IT(EXTI_IMR2_IM58);
      break;

    case PWR_WAKEUP_PIN5:
      __HAL_WKUP_EXTI_ENABLE_IT(EXTI_IMR2_IM59);
      break;

    case PWR_WAKEUP_PIN6:
      __HAL_WKUP_EXTI_ENABLE_IT(EXTI_IMR2_IM60);
      break;
  }
}


/**
  * @brief Disable WakeUp PINx Interrupt on AIEC and NVIC.
  * @param WakeUpPinx: Specifies the Power Wake-Up pin
  *         This parameter can be one of the following values:
  *           @arg PWR_WAKEUP_PIN1
  *           @arg PWR_WAKEUP_PIN2
  *           @arg PWR_WAKEUP_PIN3
  *           @arg PWR_WAKEUP_PIN4
  *           @arg PWR_WAKEUP_PIN5
  *           @arg PWR_WAKEUP_PIN6
  * @retval None
  */
void HAL_PWR_DisableWakeUpPinIT(uint32_t WakeUpPinx)
{
  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinx));
  switch ((WakeUpPinx & PWR_WAKEUP_PIN_MASK))
  {
    case PWR_WAKEUP_PIN1:
      __HAL_WKUP_EXTI_DISABLE_IT(EXTI_IMR2_IM55);
      break;

    case PWR_WAKEUP_PIN2:
      __HAL_WKUP_EXTI_DISABLE_IT(EXTI_IMR2_IM56);
      break;

    case PWR_WAKEUP_PIN3:
      __HAL_WKUP_EXTI_DISABLE_IT(EXTI_IMR2_IM57);
      break;

    case PWR_WAKEUP_PIN4:
      __HAL_WKUP_EXTI_DISABLE_IT(EXTI_IMR2_IM58);
      break;

    case PWR_WAKEUP_PIN5:
      __HAL_WKUP_EXTI_DISABLE_IT(EXTI_IMR2_IM59);
      break;

    case PWR_WAKEUP_PIN6:
      __HAL_WKUP_EXTI_DISABLE_IT(EXTI_IMR2_IM60);
      break;
  }
}

/**
  * @brief Enters CSleep mode.
  *
  * @note In CSleep mode, all I/O pins keep the same state as in Run mode.
  *
  * @note In CSleep mode, the systick is stopped to avoid exit from this mode with
  *       systick interrupt when used as time base for Timeout
  *
  * @param Regulator: Specifies the regulator state in CSLEEP mode.
  *          This parameter is not used for the STM32MP1 family and is kept as
  *          parameter just to maintain compatibility with the lower power families
  *            This parameter can be one of the following values:
  *            @arg PWR_MAINREGULATOR_ON: CSLEEP mode with regulator ON
  *            @arg PWR_LOWPOWERREGULATOR_ON: CSLEEP mode with low power regulator ON
  * @param SLEEPEntry: Specifies if CSLEEP mode in entered with WFI or WFE instruction.
  *          This parameter can be one of the following values:
  *            @arg PWR_SLEEPENTRY_WFI: enter CSLEEP mode with WFI instruction
  *            @arg PWR_SLEEPENTRY_WFE: enter CSLEEP mode with WFE instruction
  * @retval None
  */
void HAL_PWR_EnterSLEEPMode(uint32_t Regulator, uint8_t SLEEPEntry)
{
  /* Check the parameters */
  assert_param(IS_PWR_REGULATOR(Regulator));
  assert_param(IS_PWR_SLEEP_ENTRY(SLEEPEntry));

#ifdef CORE_CM4
  /* Ensure CM4 do not enter to CSTOP mode */
  /* Clear SLEEPDEEP bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
#endif

  /* Select SLEEP mode entry -------------------------------------------------*/
  if (SLEEPEntry == PWR_SLEEPENTRY_WFI)
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
  * @brief Enters CSTOP
  *        This function puts core domain into CSTOP allowing system STOP mode
  *        if both MPU and MCU cores are on CSTOP mode
  * @note In Stop mode, all I/O pins keep the same state as in Run mode.
  * @note When exiting Stop mode by issuing an interrupt or a wake up event,
  *       the HSI oscillator is selected as CM4 system clock.
  * @note RCC_WAKEUP_IRQn IT must be programmed to have the highest priority and
  *       to be the only one IT having this value before calling HAL_PWR_EnterSTOPMode.
  *       Make sure RCC_WAKEUP_IRQn is the only one IT allowed to wake up core
  *       before calling  HAL_PWR_EnterSTOPMode (BASEPRI)
  *       Reestablish priority level once system is completely waken up (clock
  *       restore and IO compensation)
  * @note When the voltage regulator operates in low power mode, an additional
  *         startup delay is incurred when waking up from Stop mode.
  *         By keeping the internal regulator ON during Stop mode, the consumption
  *         is higher although the startup time is reduced.
  * @param Regulator: Specifies the regulator state in Stop mode.
  *          This parameter is unused on MCU but any value must be provided.
  *          This parameter can be one of the following values:
  *            @arg PWR_MAINREGULATOR_ON: Stop mode with regulator ON
  *            @arg PWR_LOWPOWERREGULATOR_ON: Stop mode with low power regulator ON
  * @param STOPEntry: Specifies if CStop mode is entered with WFI or WFE instruction.
  *          This parameter can be one of the following values:
  *            @arg PWR_STOPENTRY_WFI: Enter CStop mode with WFI instruction
  *            @arg PWR_STOPENTRY_WFE: Enter CStop mode with WFE instruction
  * @retval None
  */
void HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry)
{
  /* Check the parameters */
  assert_param(IS_PWR_REGULATOR(Regulator));
  assert_param(IS_PWR_STOP_ENTRY(STOPEntry));

#ifdef CORE_CM4
  /*Forbid going to STANDBY mode (select STOP mode) */
  CLEAR_BIT(PWR->MCUCR, PWR_MCUCR_PDDS);

  /*Allow CORE_CM4 to enter CSTOP mode
  Set SLEEPDEEP bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, SCB_SCR_SLEEPDEEP_Msk);
#endif/* CORE_CM4 */

#ifdef CORE_CA7
  if (Regulator == PWR_MAINREGULATOR_ON)
  {
    /* Select STOP mode */
    CLEAR_BIT(PWR->CR1, PWR_CR1_LPDS);
  }
  else
  {
    /* Select LP-STOP mode */
    SET_BIT(PWR->CR1, PWR_CR1_LPDS);
  }

  /* Clear MPU STANDBY, STOP and HOLD flags.(Always read as 0) */
  SET_BIT(PWR->MPUCR, PWR_MPUCR_CSSF);

  /* MPU STAY in STOP MODE */
  CLEAR_BIT(PWR->MPUCR, PWR_MPUCR_PDDS);
  /* MPU CSTANDBY mode disabled */
  SET_BIT(PWR->MPUCR, PWR_MPUCR_CSTBYDIS);

  /* RCC Stop Request Set Register */
  RCC->MP_SREQSETR = RCC_MP_SREQSETR_STPREQ_P0 | RCC_MP_SREQSETR_STPREQ_P1;

#else
  /* Prevent unused argument compilation warning */
  UNUSED(Regulator);
#endif /*CORE_CA7*/

  /* Select Stop mode entry --------------------------------------------------*/
  if ((STOPEntry == PWR_STOPENTRY_WFI))
  {
    /* Request Wait For Interrupt */
    __WFI();
  }
  else if (STOPEntry == PWR_STOPENTRY_WFE)
  {
    /* Request Wait For Event */
    __SEV();
    __WFE();
    __WFE();
  }

#ifdef CORE_CM4
  /* Reset SLEEPDEEP bit of Cortex System Control Register */
  SCB->SCR &= (uint32_t)~((uint32_t)SCB_SCR_SLEEPDEEP_Msk);
#endif/* CORE_CM4 */

#ifdef CORE_CA7
  /* RCC Clear Request Set Register */
  RCC->MP_SREQCLRR = RCC_MP_SREQSETR_STPREQ_P0 | RCC_MP_SREQSETR_STPREQ_P1;
#endif
}

/**
  * @brief Enters MPU CStandby / MCU CSTOP allowing system Standby mode.
  * @note In Standby mode, all I/O pins are high impedance except for:
  *          - Reset pad (still available)
  *          - RTC_AF1 pin (PC13) if configured for tamper, time-stamp, RTC
  *            Alarm out, or RTC clock calibration out.
  *          - RTC_AF2 pin (PI8) if configured for tamper or time-stamp.
  *          - WKUP pins if enabled.
  * @retval None
  */
void HAL_PWR_EnterSTANDBYMode(void)
{

#ifdef CORE_CM4
  /*Allow to go to STANDBY mode */
  SET_BIT(PWR->MCUCR, PWR_MCUCR_PDDS);

  /*Allow CORE_CM4 to enter CSTOP mode
  Set SLEEPDEEP bit of Cortex System Control Register */
  SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
#endif /* CORE_CM4 */

#ifdef CORE_CA7

  /* Clear MPU STANDBY, STOP and HOLD flags.(Always read as 0) */
  SET_BIT(PWR->MPUCR, PWR_MPUCR_CSSF);
  /* system Power Down Deepsleep selection */
  /* MPU go in STANDBY MODE */
  SET_BIT(PWR->MPUCR, PWR_MPUCR_PDDS);
  /* MPU CSTANDBY mode enabled */
  CLEAR_BIT(PWR->MPUCR, PWR_MPUCR_CSTBYDIS);

  /* RCC Stop Request Set Register */
  RCC->MP_SREQSETR = RCC_MP_SREQSETR_STPREQ_P0 | RCC_MP_SREQSETR_STPREQ_P1;
#endif

  /* Clear Reset Status */
  __HAL_RCC_CLEAR_RESET_FLAGS();


  /* This option is used to ensure that store operations are completed */
#if defined ( __CC_ARM)
  __force_stores();
#endif
  /* Request Wait For Interrupt */
  __WFI();
}


/**
  * @brief Indicates Sleep-On-Exit when returning from Handler mode to Thread mode.
  * @note Set SLEEPONEXIT bit of SCR register. When this bit is set, the processor
  *       re-enters CSleep mode when an interruption handling is over.
  *       Setting this bit is useful when the processor is expected to run only on
  *       interruptions handling.
  * @retval None
  */
void HAL_PWR_EnableSleepOnExit(void)
{
#ifdef CORE_CM4
  /* Set SLEEPONEXIT bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
#endif
}


/**
  * @brief Disables Sleep-On-Exit feature when returning from Handler mode to Thread mode.
  * @note Clears SLEEPONEXIT bit of SCR register. When this bit is set, the processor
  *       re-enters CSLEEP mode when an interruption handling is over.
  * @retval None
  */
void HAL_PWR_DisableSleepOnExit(void)
{
#ifdef CORE_CM4
  /* Clear SLEEPONEXIT bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPONEXIT_Msk));
#endif
}

/**
  * @brief Enables CORTEX SEVONPEND bit.
  * @note Sets SEVONPEND bit of SCR register. When this bit is set, this causes
  *       WFE to wake up when an interrupt moves from inactive to pended.
  * @retval None
  */
void HAL_PWR_EnableSEVOnPend(void)
{
#ifdef CORE_CM4
  /* Set SEVONPEND bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk));
#endif
}

/**
  * @brief Disables CORTEX SEVONPEND bit.
  * @note Clears SEVONPEND bit of SCR register. When this bit is set, this causes
  *       WFE to wake up when an interrupt moves from inactive to pended.
  * @retval None
  */
void HAL_PWR_DisableSEVOnPend(void)
{
#ifdef CORE_CM4
  /* Clear SEVONPEND bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SEVONPEND_Msk));
#endif
}

/**
  * @brief  PWR PVD interrupt callback
  * @retval None
  */
__weak void HAL_PWR_PVDCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWR_PVDCallback could be implemented in the user file
   */
}

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
