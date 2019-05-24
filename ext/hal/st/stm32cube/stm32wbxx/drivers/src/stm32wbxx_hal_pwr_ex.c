/**
  ******************************************************************************
  * @file    stm32wbxx_hal_pwr_ex.c
  * @author  MCD Application Team
  * @brief   Extended PWR HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of the Power Controller (PWR) peripheral:
  *           + Extended Initialization and de-initialization functions
  *           + Extended Peripheral Control functions
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

/** @addtogroup PWREx
  * @{
  */

#ifdef HAL_PWR_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @defgroup PWR_Extended_Private_Defines PWR Extended Private Defines
  * @{
  */
#define PWR_PORTE_AVAILABLE_PINS   (PWR_GPIO_BIT_4 | PWR_GPIO_BIT_3 | PWR_GPIO_BIT_2 | PWR_GPIO_BIT_1 | PWR_GPIO_BIT_0)
#define PWR_PORTH_AVAILABLE_PINS   (PWR_GPIO_BIT_3 | PWR_GPIO_BIT_1 | PWR_GPIO_BIT_0)

/** @defgroup PWREx_TimeOut_Value PWR Extended Flag Setting Time Out Value
  * @{
  */
#define PWR_FLAG_SETTING_DELAY_US                      50U   /*!< Time out value for REGLPF and VOSF flags setting */
/**
  * @}
  */

/**
  * @}
  */



/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @addtogroup PWREx_Exported_Functions PWR Extended Exported Functions
  * @{
  */

/** @addtogroup PWREx_Exported_Functions_Group1 Extended Peripheral Control functions
  *  @brief   Extended Peripheral Control functions
  *
@verbatim
 ===============================================================================
              ##### Extended Peripheral Initialization and de-initialization functions #####
 ===============================================================================
    [..]

@endverbatim
  * @{
  */


/**
  * @brief Return Voltage Scaling Range.
  * @retval VOS bit field (PWR_REGULATOR_VOLTAGE_RANGE1 or PWR_REGULATOR_VOLTAGE_RANGE2)
  */
uint32_t HAL_PWREx_GetVoltageRange(void)
{
  return  (PWR->CR1 & PWR_CR1_VOS);
}

/**
  * @brief Configure the main internal regulator output voltage.
  * @param VoltageScaling specifies the regulator output voltage to achieve
  *         a tradeoff between performance and power consumption.
  *          This parameter can be one of the following values:
  *            @arg @ref PWR_REGULATOR_VOLTAGE_SCALE1 Regulator voltage output range 1 mode,
  *                                                typical output voltage at 1.2 V,
  *                                                system frequency up to 64 MHz.
  *            @arg @ref PWR_REGULATOR_VOLTAGE_SCALE2 Regulator voltage output range 2 mode,
  *                                                typical output voltage at 1.0 V,
  *                                                system frequency up to 16 MHz.
  * @note  When moving from Range 1 to Range 2, the system frequency must be decreased to
  *        a value below 16 MHz before calling HAL_PWREx_ControlVoltageScaling() API.
  *        When moving from Range 2 to Range 1, the system frequency can be increased to
  *        a value up to 64 MHz after calling HAL_PWREx_ControlVoltageScaling() API.
  * @note  When moving from Range 2 to Range 1, the API waits for VOSF flag to be
  *        cleared before returning the status. If the flag is not cleared within
  *        50 microseconds, HAL_TIMEOUT status is reported.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_PWREx_ControlVoltageScaling(uint32_t VoltageScaling)
{
  uint32_t wait_loop_index;

  assert_param(IS_PWR_VOLTAGE_SCALING_RANGE(VoltageScaling));

  /* If Set Range 1 */
  if (VoltageScaling == PWR_REGULATOR_VOLTAGE_SCALE1)
  {
    if (READ_BIT(PWR->CR1, PWR_CR1_VOS) != PWR_REGULATOR_VOLTAGE_SCALE1)
    {
      /* Set Range 1 */
      MODIFY_REG(PWR->CR1, PWR_CR1_VOS, PWR_REGULATOR_VOLTAGE_SCALE1);

      /* Wait until VOSF is cleared */
      wait_loop_index = (PWR_FLAG_SETTING_DELAY_US * (SystemCoreClock / 1000000U));
      while ((HAL_IS_BIT_SET(PWR->SR2, PWR_SR2_VOSF)) && (wait_loop_index != 0U))
      {
        wait_loop_index--;
      }
      if (HAL_IS_BIT_SET(PWR->SR2, PWR_SR2_VOSF))
      {
        return HAL_TIMEOUT;
      }
    }
  }
  else
  {
    if (READ_BIT(PWR->CR1, PWR_CR1_VOS) != PWR_REGULATOR_VOLTAGE_SCALE2)
    {
      /* Set Range 2 */
      MODIFY_REG(PWR->CR1, PWR_CR1_VOS, PWR_REGULATOR_VOLTAGE_SCALE2);
      /* No need to wait for VOSF to be cleared for this transition */
    }
  }

  return HAL_OK;
}

/****************************************************************************/

/**
  * @brief Enable battery charging.
  *        When VDD is present, charge the external battery on VBAT thru an internal resistor.
  * @param ResistorSelection specifies the resistor impedance.
  *          This parameter can be one of the following values:
  *            @arg @ref PWR_BATTERY_CHARGING_RESISTOR_5     5 kOhms resistor
  *            @arg @ref PWR_BATTERY_CHARGING_RESISTOR_1_5 1.5 kOhms resistor
  * @retval None
  */
void HAL_PWREx_EnableBatteryCharging(uint32_t ResistorSelection)
{
  assert_param(IS_PWR_BATTERY_RESISTOR_SELECT(ResistorSelection));

  /* Specify resistor selection */
  MODIFY_REG(PWR->CR4, PWR_CR4_VBRS, ResistorSelection);

  /* Enable battery charging */
  SET_BIT(PWR->CR4, PWR_CR4_VBE);
}

/**
  * @brief Disable battery charging.
  * @retval None
  */
void HAL_PWREx_DisableBatteryCharging(void)
{
  CLEAR_BIT(PWR->CR4, PWR_CR4_VBE);
}

/****************************************************************************/

/**
  * @brief Enable VDDUSB supply.
  * @note  Remove VDDUSB electrical and logical isolation, once VDDUSB supply is present.
  * @retval None
  */
void HAL_PWREx_EnableVddUSB(void)
{
  SET_BIT(PWR->CR2, PWR_CR2_USV);
}

/**
  * @brief Disable VDDUSB supply.
  * @retval None
  */
void HAL_PWREx_DisableVddUSB(void)
{
  CLEAR_BIT(PWR->CR2, PWR_CR2_USV);
}

/****************************************************************************/

/**
  * @brief Enable Internal Wake-up Line.
  * @retval None
  */
void HAL_PWREx_EnableInternalWakeUpLine(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EIWUL);
}

/**
  * @brief Disable Internal Wake-up Line.
  * @retval None
  */
void HAL_PWREx_DisableInternalWakeUpLine(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EIWUL);
}


/**
  * @brief Enable BORH and SMPS step down converter forced in bypass mode
  *        interrupt for CPU1
  * @retval None
  */
void HAL_PWREx_EnableBORH_SMPSBypassIT(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EBORHSMPSFB);
}

/**
  * @brief Disable BORH and SMPS step down converter forced in bypass mode
  *        interrupt for CPU1
  * @retval None
  */
void HAL_PWREx_DisableBORH_SMPSBypassIT(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EBORHSMPSFB);
}


/**
  * @brief Enable RF Phase interrupt.
  * @retval None
  */
void HAL_PWREx_EnableRFPhaseIT(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_ECRPE_Msk);
}

/**
  * @brief Disable RF Phase interrupt.
  * @retval None
  */
void HAL_PWREx_DisableRFPhaseIT(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_ECRPE_Msk);
}


/**
  * @brief Enable BLE Activity interrupt.
  * @retval None
  */
void HAL_PWREx_EnableBLEActivityIT(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EBLEA);
}

/**
  * @brief Disable BLE Activity interrupt.
  * @retval None
  */
void HAL_PWREx_DisableBLEActivityIT(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EBLEA);
}


/**
  * @brief Enable 802.15.4 Activity interrupt.
  * @retval None
  */
void HAL_PWREx_Enable802ActivityIT(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_E802A);
}

/**
  * @brief Disable 802.15.4 Activity interrupt.
  * @retval None
  */
void HAL_PWREx_Disable802ActivityIT(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_E802A);
}

/**
  * @brief Enable CPU2 on-Hold interrupt.
  * @retval None
  */
void HAL_PWREx_EnableHOLDC2IT(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_EC2H);
}

/**
  * @brief Disable CPU2 on-Hold interrupt.
  * @retval None
  */
void HAL_PWREx_DisableHOLDC2IT(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_EC2H);
}

/****************************************************************************/

/**
  * @brief Enable GPIO pull-up state in Standby and Shutdown modes.
  * @note  Set the relevant PUy bits of PWR_PUCRx register to configure the I/O in
  *        pull-up state in Standby and Shutdown modes.
  * @note  This state is effective in Standby and Shutdown modes only if APC bit
  *        is set through HAL_PWREx_EnablePullUpPullDownConfig() API.
  * @note  The configuration is lost when exiting the Shutdown mode due to the
  *        power-on reset, maintained when exiting the Standby mode.
  * @note  To avoid any conflict at Standby and Shutdown modes exits, the corresponding
  *        PDy bit of PWR_PDCRx register is cleared unless it is reserved.
  * @note  Even if a PUy bit to set is reserved, the other PUy bits entered as input
  *        parameter at the same time are set.
  * @param GPIO Specify the IO port. This parameter can be PWR_GPIO_A, ..., PWR_GPIO_H
  *         to select the GPIO peripheral.
  * @param GPIONumber Specify the I/O pins numbers.
  *         This parameter can be one of the following values:
  *         PWR_GPIO_BIT_0, ..., PWR_GPIO_BIT_15 (except for PORTH where less
  *         I/O pins are available) or the logical OR of several of them to set
  *         several bits for a given port in a single API call.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber)
{
  HAL_StatusTypeDef status = HAL_OK;

  assert_param(IS_PWR_GPIO(GPIO));
  assert_param(IS_PWR_GPIO_BIT_NUMBER(GPIONumber));

  switch (GPIO)
  {
    case PWR_GPIO_A:
       SET_BIT(PWR->PUCRA, GPIONumber);
       CLEAR_BIT(PWR->PDCRA, GPIONumber);
       break;
    case PWR_GPIO_B:
       SET_BIT(PWR->PUCRB, GPIONumber);
       CLEAR_BIT(PWR->PDCRB, GPIONumber);
       break;
    case PWR_GPIO_C:
       SET_BIT(PWR->PUCRC, GPIONumber);
       CLEAR_BIT(PWR->PDCRC, GPIONumber);
       break;
    case PWR_GPIO_D:
       SET_BIT(PWR->PUCRD, GPIONumber);
       CLEAR_BIT(PWR->PDCRD, GPIONumber);
       break;
    case PWR_GPIO_E:
       SET_BIT(PWR->PUCRE, (GPIONumber & PWR_PORTE_AVAILABLE_PINS));
       CLEAR_BIT(PWR->PDCRE, (GPIONumber & PWR_PORTE_AVAILABLE_PINS));
       break;
    case PWR_GPIO_H:
       SET_BIT(PWR->PUCRH, (GPIONumber & PWR_PORTH_AVAILABLE_PINS));
       CLEAR_BIT(PWR->PDCRH, (GPIONumber & PWR_PORTH_AVAILABLE_PINS));
       break;
    default:
      status = HAL_ERROR;
      break;
  }

  return status;
}

/**
  * @brief Disable GPIO pull-up state in Standby mode and Shutdown modes.
  * @note  Reset the relevant PUy bits of PWR_PUCRx register used to configure the I/O
  *        in pull-up state in Standby and Shutdown modes.
  * @note  Even if a PUy bit to reset is reserved, the other PUy bits entered as input
  *        parameter at the same time are reset.
  * @param GPIO Specifies the IO port. This parameter can be PWR_GPIO_A, ..., PWR_GPIO_H
  *         to select the GPIO peripheral.
  * @param GPIONumber Specify the I/O pins numbers.
  *         This parameter can be one of the following values:
  *         PWR_GPIO_BIT_0, ..., PWR_GPIO_BIT_15 (except for PORTH where less
  *         I/O pins are available) or the logical OR of several of them to reset
  *         several bits for a given port in a single API call.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber)
{
  HAL_StatusTypeDef status = HAL_OK;

  assert_param(IS_PWR_GPIO(GPIO));
  assert_param(IS_PWR_GPIO_BIT_NUMBER(GPIONumber));

  switch (GPIO)
  {
    case PWR_GPIO_A:
       CLEAR_BIT(PWR->PUCRA, GPIONumber);
       break;
    case PWR_GPIO_B:
       CLEAR_BIT(PWR->PUCRB, GPIONumber);
       break;
    case PWR_GPIO_C:
       CLEAR_BIT(PWR->PUCRC, GPIONumber);
       break;
    case PWR_GPIO_D:
       CLEAR_BIT(PWR->PUCRD, GPIONumber);
       break;
    case PWR_GPIO_E:
       CLEAR_BIT(PWR->PUCRE, (GPIONumber & PWR_PORTE_AVAILABLE_PINS));
       break;
    case PWR_GPIO_H:
       CLEAR_BIT(PWR->PUCRH, (GPIONumber & PWR_PORTH_AVAILABLE_PINS));
       break;
    default:
       status = HAL_ERROR;
       break;
  }

  return status;
}



/**
  * @brief Enable GPIO pull-down state in Standby and Shutdown modes.
  * @note  Set the relevant PDy bits of PWR_PDCRx register to configure the I/O in
  *        pull-down state in Standby and Shutdown modes.
  * @note  This state is effective in Standby and Shutdown modes only if APC bit
  *        is set through HAL_PWREx_EnablePullUpPullDownConfig() API.
  * @note  The configuration is lost when exiting the Shutdown mode due to the
  *        power-on reset, maintained when exiting the Standby mode.
  * @note  To avoid any conflict at Standby and Shutdown modes exits, the corresponding
  *        PUy bit of PWR_PUCRx register is cleared unless it is reserved.
  * @note  Even if a PDy bit to set is reserved, the other PDy bits entered as input
  *        parameter at the same time are set.
  * @param GPIO Specify the IO port. This parameter can be PWR_GPIO_A..PWR_GPIO_H
  *         to select the GPIO peripheral.
  * @param GPIONumber Specify the I/O pins numbers.
  *         This parameter can be one of the following values:
  *         PWR_GPIO_BIT_0, ..., PWR_GPIO_BIT_15 (except for PORTH where less
  *         I/O pins are available) or the logical OR of several of them to set
  *         several bits for a given port in a single API call.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber)
{
  HAL_StatusTypeDef status = HAL_OK;

  assert_param(IS_PWR_GPIO(GPIO));
  assert_param(IS_PWR_GPIO_BIT_NUMBER(GPIONumber));

  switch (GPIO)
  {
    case PWR_GPIO_A:
       SET_BIT(PWR->PDCRA, GPIONumber);
       CLEAR_BIT(PWR->PUCRA, GPIONumber);
       break;
    case PWR_GPIO_B:
       SET_BIT(PWR->PDCRB, GPIONumber);
       CLEAR_BIT(PWR->PUCRB, GPIONumber);
       break;
    case PWR_GPIO_C:
       SET_BIT(PWR->PDCRC, GPIONumber);
       CLEAR_BIT(PWR->PUCRC, GPIONumber);
       break;
  case PWR_GPIO_D:
       SET_BIT(PWR->PDCRD, GPIONumber);
       CLEAR_BIT(PWR->PUCRD, GPIONumber);
       break;
    case PWR_GPIO_E:
       SET_BIT(PWR->PDCRE, (GPIONumber & PWR_PORTE_AVAILABLE_PINS));
       CLEAR_BIT(PWR->PUCRE, (GPIONumber & PWR_PORTE_AVAILABLE_PINS));
       break;
    case PWR_GPIO_H:
       SET_BIT(PWR->PDCRH, (GPIONumber & PWR_PORTH_AVAILABLE_PINS));
       CLEAR_BIT(PWR->PUCRH, (GPIONumber & PWR_PORTH_AVAILABLE_PINS));
       break;
    default:
      status = HAL_ERROR;
      break;
  }

  return status;
}

/**
  * @brief Disable GPIO pull-down state in Standby and Shutdown modes.
  * @note  Reset the relevant PDy bits of PWR_PDCRx register used to configure the I/O
  *        in pull-down state in Standby and Shutdown modes.
  * @note  Even if a PDy bit to reset is reserved, the other PDy bits entered as input
  *        parameter at the same time are reset.
  * @param GPIO Specifies the IO port. This parameter can be PWR_GPIO_A..PWR_GPIO_H
  *         to select the GPIO peripheral.
  * @param GPIONumber Specify the I/O pins numbers.
  *         This parameter can be one of the following values:
  *         PWR_GPIO_BIT_0, ..., PWR_GPIO_BIT_15 (except for PORTH where less
  *         I/O pins are available) or the logical OR of several of them to reset
  *         several bits for a given port in a single API call.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber)
{
  HAL_StatusTypeDef status = HAL_OK;

  assert_param(IS_PWR_GPIO(GPIO));
  assert_param(IS_PWR_GPIO_BIT_NUMBER(GPIONumber));

  switch (GPIO)
  {
    case PWR_GPIO_A:
       CLEAR_BIT(PWR->PDCRA, GPIONumber);
       break;
    case PWR_GPIO_B:
       CLEAR_BIT(PWR->PDCRB, GPIONumber);
       break;
    case PWR_GPIO_C:
       CLEAR_BIT(PWR->PDCRC, GPIONumber);
       break;
    case PWR_GPIO_D:
       CLEAR_BIT(PWR->PDCRD, GPIONumber);
       break;
    case PWR_GPIO_E:
       CLEAR_BIT(PWR->PDCRE, (GPIONumber & PWR_PORTE_AVAILABLE_PINS));
       break;
    case PWR_GPIO_H:
      CLEAR_BIT(PWR->PDCRH, (GPIONumber & PWR_PORTH_AVAILABLE_PINS));
       break;
    default:
      status = HAL_ERROR;
      break;
  }

  return status;
}

/**
  * @brief Enable pull-up and pull-down configuration.
  * @note  When APC bit is set, the I/O pull-up and pull-down configurations defined in
  *        PWR_PUCRx and PWR_PDCRx registers are applied in Standby and Shutdown modes.
  * @note  Pull-up set by PUy bit of PWR_PUCRx register is not activated if the corresponding
  *        PDy bit of PWR_PDCRx register is also set (pull-down configuration priority is higher).
  *        HAL_PWREx_EnableGPIOPullUp() and HAL_PWREx_EnableGPIOPullDown() API's ensure there
  *        is no conflict when setting PUy or PDy bit.
  * @retval None
  */
void HAL_PWREx_EnablePullUpPullDownConfig(void)
{
  SET_BIT(PWR->CR3, PWR_CR3_APC);
}

/**
  * @brief Disable pull-up and pull-down configuration.
  * @note  When APC bit is cleared, the I/O pull-up and pull-down configurations defined in
  *        PWR_PUCRx and PWR_PDCRx registers are not applied in Standby and Shutdown modes.
  * @retval None
  */
void HAL_PWREx_DisablePullUpPullDownConfig(void)
{
  CLEAR_BIT(PWR->CR3, PWR_CR3_APC);
}

/****************************************************************************/

/**
  * @brief  Set BOR configuration
  * @param  BORConfiguration This parameter can be one of the following values:
  *         @arg @ref PWR_BOR_SYSTEM_RESET
  *         @arg @ref PWR_BOR_SMPS_FORCE_BYPASS
  */
void HAL_PWREx_SetBORConfig(uint32_t BORConfiguration)
{
  LL_PWR_SetBORConfig(BORConfiguration);
}

/**
  * @brief  Get BOR configuration
  * @retval Returned value can be one of the following values:
  *         @arg @ref PWR_BOR_SYSTEM_RESET
  *         @arg @ref PWR_BOR_SMPS_FORCE_BYPASS
  */
uint32_t HAL_PWREx_GetBORConfig(void)
{
  return LL_PWR_GetBORConfig();
}

/****************************************************************************/
/**
  * @brief  Hold the CPU and their allocated peripherals after reset or wakeup from stop or standby.
  * @param  CPU: Specifies the core to be held.
  *              This parameter can be one of the following values:
  *             @arg PWR_CORE_CPU2: Hold CPU2 and set CPU1 as master.
  * @note   Hold CPU2 with CPU1 as master by default.
  * @retval None
  */
void HAL_PWREx_HoldCore(uint32_t CPU)
{
  /* Check the parameters */
  assert_param(IS_PWR_CORE_HOLD_RELEASE(CPU));

  LL_PWR_DisableBootC2();
}

/**
  * @brief  Release Cortex CPU2 and allocated peripherals after reset or wakeup from stop or standby.
  * @param  CPU: Specifies the core to be released.
  *              This parameter can be one of the following values:
  *             @arg PWR_CORE_CPU2: Release the CPU2 from holding.
  * @retval None
  */
void HAL_PWREx_ReleaseCore(uint32_t CPU)
{
  /* Check the parameters */
  assert_param(IS_PWR_CORE_HOLD_RELEASE(CPU));

  LL_PWR_EnableBootC2();
}

/****************************************************************************/
/**
  * @brief Enable BKRAM content retention in Standby mode.
  * @note  When RRS bit is set, SRAM is powered by the low-power regulator in
  *         Standby mode and its content is kept.
  * @retval None
  */
void HAL_PWREx_EnableSRAMRetention(void)
{
  LL_PWR_EnableSRAM2Retention();
}

/**
  * @brief Disable BKRAM content retention in Standby mode.
  * @note  When RRS bit is reset, SRAM is powered off in Standby mode
  *        and its content is lost.
  * @retval None
  */
void HAL_PWREx_DisableSRAMRetention(void)
{
  LL_PWR_DisableSRAM2Retention();
}

/****************************************************************************/
/**
  * @brief  Enable Flash Power Down.
  * @note   This API allows to enable flash power down capabilities in low power
  *         run and low power sleep modes.
  * @param  PowerMode this can be a combination of following values:
  *           @arg @ref PWR_FLASHPD_LPRUN
  *           @arg @ref PWR_FLASHPD_LPSLEEP
  * @retval None
  */
void HAL_PWREx_EnableFlashPowerDown(uint32_t PowerMode)
{
  assert_param(IS_PWR_FLASH_POWERDOWN(PowerMode));

  if((PowerMode & PWR_FLASHPD_LPRUN) != 0U)
  {
    /* Unlock bit FPDR */
    WRITE_REG(PWR->CR1, 0x0000C1B0U);
  }

  /* Set flash power down mode */
  SET_BIT(PWR->CR1, PowerMode);
}

/**
  * @brief  Disable Flash Power Down.
  * @note   This API allows to disable flash power down capabilities in low power
  *         run and low power sleep modes.
  * @param  PowerMode this can be a combination of following values:
  *           @arg @ref PWR_FLASHPD_LPRUN
  *           @arg @ref PWR_FLASHPD_LPSLEEP
  * @retval None
  */
void HAL_PWREx_DisableFlashPowerDown(uint32_t PowerMode)
{
  assert_param(IS_PWR_FLASH_POWERDOWN(PowerMode));

  /* Set flash power down mode */
  CLEAR_BIT(PWR->CR1, PowerMode);
}

/****************************************************************************/
/**
  * @brief Enable the Power Voltage Monitoring 1: VDDUSB versus 1.2V.
  * @retval None
  */
void HAL_PWREx_EnablePVM1(void)
{
  SET_BIT(PWR->CR2, PWR_PVM_1);
}

/**
  * @brief Disable the Power Voltage Monitoring 1: VDDUSB versus 1.2V.
  * @retval None
  */
void HAL_PWREx_DisablePVM1(void)
{
  CLEAR_BIT(PWR->CR2, PWR_PVM_1);
}
/**
  * @brief Enable the Power Voltage Monitoring 3: VDDA versus 1.62V.
  * @retval None
  */
void HAL_PWREx_EnablePVM3(void)
{
  SET_BIT(PWR->CR2, PWR_PVM_3);
}

/**
  * @brief Disable the Power Voltage Monitoring 3: VDDA versus 1.62V.
  * @retval None
  */
void HAL_PWREx_DisablePVM3(void)
{
  CLEAR_BIT(PWR->CR2, PWR_PVM_3);
}




/**
  * @brief Configure the Peripheral Voltage Monitoring (PVM).
  * @param sConfigPVM pointer to a PWR_PVMTypeDef structure that contains the
  *        PVM configuration information.
  * @note The API configures a single PVM according to the information contained
  *       in the input structure. To configure several PVMs, the API must be singly
  *       called for each PVM used.
  * @note Refer to the electrical characteristics of your device datasheet for
  *         more details about the voltage thresholds corresponding to each
  *         detection level and to each monitored supply.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_ConfigPVM(PWR_PVMTypeDef *sConfigPVM)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_PWR_PVM_TYPE(sConfigPVM->PVMType));
  assert_param(IS_PWR_PVM_MODE(sConfigPVM->Mode));

  /* Configure EXTI 31 and 33 interrupts if so required:
     scan thru PVMType to detect which PVMx is set and
     configure the corresponding EXTI line accordingly. */
  switch (sConfigPVM->PVMType)
  {
   case PWR_PVM_1:
      /* Clear any previous config. Keep it clear if no event or IT mode is selected */
      __HAL_PWR_PVM1_EXTI_DISABLE_EVENT();
      __HAL_PWR_PVM1_EXTI_DISABLE_IT();
      __HAL_PWR_PVM1_EXTI_DISABLE_FALLING_EDGE();
      __HAL_PWR_PVM1_EXTI_DISABLE_RISING_EDGE();

      /* Configure interrupt mode */
      if((sConfigPVM->Mode & PVM_MODE_IT) == PVM_MODE_IT)
      {
        __HAL_PWR_PVM1_EXTI_ENABLE_IT();
      }

      /* Configure event mode */
      if((sConfigPVM->Mode & PVM_MODE_EVT) == PVM_MODE_EVT)
      {
        __HAL_PWR_PVM1_EXTI_ENABLE_EVENT();
      }

      /* Configure the edge */
      if((sConfigPVM->Mode & PVM_RISING_EDGE) == PVM_RISING_EDGE)
      {
        __HAL_PWR_PVM1_EXTI_ENABLE_RISING_EDGE();
      }

      if((sConfigPVM->Mode & PVM_FALLING_EDGE) == PVM_FALLING_EDGE)
      {
        __HAL_PWR_PVM1_EXTI_ENABLE_FALLING_EDGE();
      }
      break;

    case PWR_PVM_3:
      /* Clear any previous config. Keep it clear if no event or IT mode is selected */
      __HAL_PWR_PVM3_EXTI_DISABLE_EVENT();
      __HAL_PWR_PVM3_EXTI_DISABLE_IT();
      __HAL_PWR_PVM3_EXTI_DISABLE_FALLING_EDGE();
      __HAL_PWR_PVM3_EXTI_DISABLE_RISING_EDGE();

      /* Configure interrupt mode */
      if((sConfigPVM->Mode & PVM_MODE_IT) == PVM_MODE_IT)
      {
        __HAL_PWR_PVM3_EXTI_ENABLE_IT();
      }

      /* Configure event mode */
      if((sConfigPVM->Mode & PVM_MODE_EVT) == PVM_MODE_EVT)
      {
        __HAL_PWR_PVM3_EXTI_ENABLE_EVENT();
      }

      /* Configure the edge */
      if((sConfigPVM->Mode & PVM_RISING_EDGE) == PVM_RISING_EDGE)
      {
        __HAL_PWR_PVM3_EXTI_ENABLE_RISING_EDGE();
      }

      if((sConfigPVM->Mode & PVM_FALLING_EDGE) == PVM_FALLING_EDGE)
      {
        __HAL_PWR_PVM3_EXTI_ENABLE_FALLING_EDGE();
      }
      break;

    default:
      status = HAL_ERROR;
      break;

  }

  return status;
}

/**
  * @brief Configure the SMPS step down converter.
  * @note   SMPS output voltage is calibrated in production,
  *         calibration parameters are applied to the voltage level parameter
  *         to reach the requested voltage value.
  * @param sConfigSMPS pointer to a PWR_SMPSTypeDef structure that contains the
  *        SMPS configuration information.
  * @note  To set and enable SMPS operating mode, refer to function
  *        "HAL_PWREx_SMPS_SetMode()".
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_ConfigSMPS(PWR_SMPSTypeDef *sConfigSMPS)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Check the parameters */
  assert_param(IS_PWR_SMPS_STARTUP_CURRENT(sConfigSMPS->StartupCurrent));
  assert_param(IS_PWR_SMPS_OUTPUT_VOLTAGE(sConfigSMPS->OutputVoltage));

  __IO const uint32_t OutputVoltageLevel_calibration = (((*SMPS_VOLTAGE_CAL_ADDR) & SMPS_VOLTAGE_CAL) >> SMPS_VOLTAGE_CAL_POS);  /* SMPS output voltage level calibrated in production */
  int32_t TrimmingSteps;                         /* Trimming steps between theorical output voltage and calibrated output voltage */
  int32_t OutputVoltageLevelTrimmed;             /* SMPS output voltage level after calibration: trimming value added to required level */

  if(OutputVoltageLevel_calibration == 0UL)
  {
    /* Device with SMPS output voltage not calibrated in production: Apply output voltage value directly */

    /* Update register */
    MODIFY_REG(PWR->CR5, PWR_CR5_SMPSVOS, (sConfigSMPS->StartupCurrent | sConfigSMPS->OutputVoltage));
  }
  else
  {
    /* Device with SMPS output voltage calibrated in production: Apply output voltage value after correction by calibration value */

    TrimmingSteps = ((int32_t)OutputVoltageLevel_calibration - (int32_t)(LL_PWR_SMPS_OUTPUT_VOLTAGE_1V50 >> PWR_CR5_SMPSVOS_Pos));
    OutputVoltageLevelTrimmed = ((int32_t)((uint32_t)(sConfigSMPS->OutputVoltage >> PWR_CR5_SMPSVOS_Pos)) + (int32_t)TrimmingSteps);

    /* Clamp value to voltage trimming bitfield range */
    if(OutputVoltageLevelTrimmed < 0)
    {
      OutputVoltageLevelTrimmed = 0;
      status = HAL_ERROR;
    }
    else if(OutputVoltageLevelTrimmed > (int32_t)PWR_CR5_SMPSVOS)
    {
      OutputVoltageLevelTrimmed = (int32_t)PWR_CR5_SMPSVOS;
      status = HAL_ERROR;
    }
    else
    {
    }

    /* Update register */
    MODIFY_REG(PWR->CR5, (PWR_CR5_SMPSSC | PWR_CR5_SMPSVOS), (sConfigSMPS->StartupCurrent | ((uint32_t) OutputVoltageLevelTrimmed)));
  }

  return status;
}

/**
  * @brief Set SMPS operating mode.
  * @param  OperatingMode This parameter can be one of the following values:
  *         @arg @ref PWR_SMPS_BYPASS
  *         @arg @ref PWR_SMPS_STEP_DOWN (1)
  *
  *         (1) SMPS operating mode step down or open depends on system low-power mode:
  *              - step down mode if system low power mode is run, LP run or stop,
  *              - open mode if system low power mode is stop1, stop2, standby or shutdown
  * @retval None
  */
void HAL_PWREx_SMPS_SetMode(uint32_t OperatingMode)
{
  MODIFY_REG(PWR->CR5, PWR_CR5_SMPSEN, (OperatingMode & PWR_SR2_SMPSF) << (PWR_CR5_SMPSEN_Pos - PWR_SR2_SMPSF_Pos));
}

/**
  * @brief  Get SMPS effective operating mode
  * @note   SMPS operating mode can be changed by hardware, therefore
  *         requested operating mode can differ from effective low power mode.
  *         - dependency on system low-power mode:
  *           - step down mode if system low power mode is run, LP run or stop,
  *           - open mode if system low power mode is stop1, stop2, standby or shutdown
  *         - dependency on BOR level:
  *           - bypass mode if supply voltage drops below BOR level
  * @note   This functions check flags of SMPS operating modes step down
  *         and bypass. If the SMPS is not among these 2 operating modes,
  *         then it can be in mode off or open.
  * @retval Returned value can be one of the following values:
  *         @arg @ref PWR_SMPS_BYPASS
  *         @arg @ref PWR_SMPS_STEP_DOWN (1)
  *
  *         (1) SMPS operating mode step down or open depends on system low-power mode:
  *              - step down mode if system low power mode is run, LP run or stop,
  *              - open mode if system low power mode is stop1, stop2, standby or shutdown
  */
uint32_t HAL_PWREx_SMPS_GetEffectiveMode(void)
{
  return (uint32_t)(READ_BIT(PWR->SR2, (PWR_SR2_SMPSF | PWR_SR2_SMPSBF)));
}

/****************************************************************************/

/**
  * @brief Enable the WakeUp PINx functionality.
  * @param WakeUpPinPolarity Specifies which Wake-Up pin to enable.
  *         This parameter can be one of the following legacy values which set the default polarity
  *         i.e. detection on high level (rising edge):
  *           @arg @ref PWR_WAKEUP_PIN1, PWR_WAKEUP_PIN2, PWR_WAKEUP_PIN3, PWR_WAKEUP_PIN4, PWR_WAKEUP_PIN5
  *
  *         or one of the following value where the user can explicitly specify the enabled pin and
  *         the chosen polarity:
  *           @arg @ref PWR_WAKEUP_PIN1_HIGH or PWR_WAKEUP_PIN1_LOW
  *           @arg @ref PWR_WAKEUP_PIN2_HIGH or PWR_WAKEUP_PIN2_LOW
  *           @arg @ref PWR_WAKEUP_PIN3_HIGH or PWR_WAKEUP_PIN3_LOW
  *           @arg @ref PWR_WAKEUP_PIN4_HIGH or PWR_WAKEUP_PIN4_LOW
  *           @arg @ref PWR_WAKEUP_PIN5_HIGH or PWR_WAKEUP_PIN5_LOW
  * @param wakeupTarget Specifies the wake-up target
  *         @arg @ref PWR_CORE_CPU1
  *         @arg @ref PWR_CORE_CPU2
  * @note  PWR_WAKEUP_PINx and PWR_WAKEUP_PINx_HIGH are equivalent.
  * @retval None
  */
void HAL_PWREx_EnableWakeUpPin(uint32_t WakeUpPinPolarity, uint32_t wakeupTarget)
{
  assert_param(IS_PWR_WAKEUP_PIN(WakeUpPinPolarity));

  /* Specifies the Wake-Up pin polarity for the event detection
    (rising or falling edge) */
  MODIFY_REG(PWR->CR4, (PWR_C2CR3_EWUP & WakeUpPinPolarity), (WakeUpPinPolarity >> PWR_WUP_POLARITY_SHIFT));

  /* Enable wake-up pin */
  if(PWR_CORE_CPU2 == wakeupTarget)
  {
    SET_BIT(PWR->C2CR3, (PWR_C2CR3_EWUP & WakeUpPinPolarity));
  }
  else
  {
    SET_BIT(PWR->CR3, (PWR_CR3_EWUP & WakeUpPinPolarity));
  }
}

/**
  * @brief  Get the Wake-Up pin flag.
  * @param WakeUpFlag specifies the Wake-Up PIN flag to check.
  *          This parameter can be one of the following values:
  *            @arg PWR_FLAG_WUF1: A wakeup event was received from PA0.
  *            @arg PWR_FLAG_WUF2: A wakeup event was received from PC13.
  *            @arg PWR_FLAG_WUF3: A wakeup event was received from PC12.
  *            @arg PWR_FLAG_WUF4: A wakeup event was received from PA2.
  *            @arg PWR_FLAG_WUF5: A wakeup event was received from PC5.
  * @retval The Wake-Up pin flag.
  */
uint32_t  HAL_PWREx_GetWakeupFlag(uint32_t WakeUpFlag)
{
  return (PWR->SR1 & (1UL << ((WakeUpFlag) & 31U)));
}

/**
  * @brief  Clear the Wake-Up pin flag.
  * @param WakeUpFlag specifies the Wake-Up PIN flag to clear.
  *          This parameter can be one of the following values:
  *            @arg PWR_FLAG_WUF1: A wakeup event was received from PA0.
  *            @arg PWR_FLAG_WUF2: A wakeup event was received from PC13.
  *            @arg PWR_FLAG_WUF3: A wakeup event was received from PC12.
  *            @arg PWR_FLAG_WUF4: A wakeup event was received from PA2.
  *            @arg PWR_FLAG_WUF5: A wakeup event was received from PC5.
  * @retval HAL status.
  */
HAL_StatusTypeDef HAL_PWREx_ClearWakeupFlag(uint32_t WakeUpFlag)
{
  PWR->SCR = (1UL << ((WakeUpFlag) & 31U));

  if((PWR->SR1 & (1UL << ((WakeUpFlag) & 31U))) != 0U)
  {
    return HAL_ERROR;
  }
  return HAL_OK;
}

/****************************************************************************/

/**
  * @brief Enter Low-power Run mode
  * @note  In Low-power Run mode, all I/O pins keep the same state as in Run mode.
  * @note  When Regulator is set to PWR_LOWPOWERREGULATOR_ON, the user can optionally configure the
  *        Flash in power-down mode in setting the RUN_PD bit in FLASH_ACR register.
  *        Additionally, the clock frequency must be reduced below 2 MHz.
  *        Setting RUN_PD in FLASH_ACR then appropriately reducing the clock frequency must
  *        be done before calling HAL_PWREx_EnableLowPowerRunMode() API.
  * @retval None
  */
void HAL_PWREx_EnableLowPowerRunMode(void)
{
  /* Set Regulator parameter */
  SET_BIT(PWR->CR1, PWR_CR1_LPR);
}


/**
  * @brief Exit Low-power Run mode.
  * @note  Before HAL_PWREx_DisableLowPowerRunMode() completion, the function checks that
  *        REGLPF has been properly reset (otherwise, HAL_PWREx_DisableLowPowerRunMode
  *        returns HAL_TIMEOUT status). The system clock frequency can then be
  *        increased above 2 MHz.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_PWREx_DisableLowPowerRunMode(void)
{
  uint32_t wait_loop_index;

  /* Clear LPR bit */
  CLEAR_BIT(PWR->CR1, PWR_CR1_LPR);

  /* Wait until REGLPF is reset */
  wait_loop_index = (PWR_FLAG_SETTING_DELAY_US * (SystemCoreClock / 1000000U));
  while ((HAL_IS_BIT_SET(PWR->SR2, PWR_SR2_REGLPF)) && (wait_loop_index != 0U))
  {
    wait_loop_index--;
  }
  if (HAL_IS_BIT_SET(PWR->SR2, PWR_SR2_REGLPF))
  {
    return HAL_TIMEOUT;
  }

  return HAL_OK;
}

/****************************************************************************/

/**
  * @brief Enter Stop 0 mode.
  * @note  In Stop 0 mode, main and low voltage regulators are ON.
  * @note  In Stop 0 mode, all I/O pins keep the same state as in Run mode.
  * @note  All clocks in the VCORE domain are stopped; the PLL, the MSI,
  *        the HSI and the HSE oscillators are disabled. Some peripherals with the wakeup capability
  *        (I2Cx, USARTx and LPUART) can switch on the HSI to receive a frame, and switch off the HSI
  *        after receiving the frame if it is not a wakeup frame. In this case, the HSI clock is propagated
  *        only to the peripheral requesting it.
  *        SRAM1, SRAM2 and register contents are preserved.
  *        The BOR is available.
  * @note  When exiting Stop 0 mode by issuing an interrupt or a wakeup event,
  *         the HSI RC oscillator is selected as system clock if STOPWUCK bit in RCC_CFGR register
  *         is set; the MSI oscillator is selected if STOPWUCK is cleared.
  * @note  By keeping the internal regulator ON during Stop 0 mode, the consumption
  *         is higher although the startup time is reduced.
  * @note  According to system power policy, system entering in Stop mode
  *        is depending on other CPU power mode.
  * @param STOPEntry  specifies if Stop mode in entered with WFI or WFE instruction.
  *          This parameter can be one of the following values:
  *            @arg @ref PWR_STOPENTRY_WFI  Enter Stop mode with WFI instruction
  *            @arg @ref PWR_STOPENTRY_WFE  Enter Stop mode with WFE instruction
  * @retval None
  */
void HAL_PWREx_EnterSTOP0Mode(uint8_t STOPEntry)
{
  /* Check the parameters */
  assert_param(IS_PWR_STOP_ENTRY(STOPEntry));

  /* Stop 0 mode with Main Regulator */
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_STOP0);


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
  * @brief Enter Stop 1 mode.
  * @note  In Stop 1 mode, only low power voltage regulator is ON.
  * @note  In Stop 1 mode, all I/O pins keep the same state as in Run mode.
  * @note  All clocks in the VCORE domain are stopped; the PLL, the MSI,
  *        the HSI and the HSE oscillators are disabled. Some peripherals with the wakeup capability
  *        (I2Cx, USARTx and LPUART) can switch on the HSI to receive a frame, and switch off the HSI
  *        after receiving the frame if it is not a wakeup frame. In this case, the HSI clock is propagated
  *        only to the peripheral requesting it.
  *        SRAM1, SRAM2 and register contents are preserved.
  *        The BOR is available.
  * @note  When exiting Stop 1 mode by issuing an interrupt or a wakeup event,
  *         the HSI RC oscillator is selected as system clock if STOPWUCK bit in RCC_CFGR register
  *         is set; the MSI oscillator is selected if STOPWUCK is cleared.
  * @note  Due to low power mode, an additional startup delay is incurred when waking up from Stop 1 mode.
  * @note  According to system power policy, system entering in Stop mode
  *        is depending on other CPU power mode.
  * @param STOPEntry  specifies if Stop mode in entered with WFI or WFE instruction.
  *          This parameter can be one of the following values:
  *            @arg @ref PWR_STOPENTRY_WFI  Enter Stop mode with WFI instruction
  *            @arg @ref PWR_STOPENTRY_WFE  Enter Stop mode with WFE instruction
  * @retval None
  */
void HAL_PWREx_EnterSTOP1Mode(uint8_t STOPEntry)
{
  /* Check the parameters */
  assert_param(IS_PWR_STOP_ENTRY(STOPEntry));

  /* Stop 1 mode with Low-Power Regulator */
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_STOP1);

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
  * @brief Enter Stop 2 mode.
  * @note  In Stop 2 mode, only low power voltage regulator is ON.
  * @note  In Stop 2 mode, all I/O pins keep the same state as in Run mode.
  * @note  All clocks in the VCORE domain are stopped, the PLL, the MSI,
  *        the HSI and the HSE oscillators are disabled. Some peripherals with wakeup capability
  *        (LCD, LPTIM1, I2C3 and LPUART) can switch on the HSI to receive a frame, and switch off the HSI after
  *        receiving the frame if it is not a wakeup frame. In this case the HSI clock is propagated only
  *        to the peripheral requesting it.
  *        SRAM1, SRAM2 and register contents are preserved.
  *        The BOR is available.
  *        The voltage regulator is set in low-power mode but LPR bit must be cleared to enter stop 2 mode.
  *        Otherwise, Stop 1 mode is entered.
  * @note  When exiting Stop 2 mode by issuing an interrupt or a wakeup event,
  *         the HSI RC oscillator is selected as system clock if STOPWUCK bit in RCC_CFGR register
  *         is set; the MSI oscillator is selected if STOPWUCK is cleared.
  * @note  According to system power policy, system entering in Stop mode
  *        is depending on other CPU power mode.
  * @param STOPEntry  specifies if Stop mode in entered with WFI or WFE instruction.
  *          This parameter can be one of the following values:
  *            @arg @ref PWR_STOPENTRY_WFI  Enter Stop mode with WFI instruction
  *            @arg @ref PWR_STOPENTRY_WFE  Enter Stop mode with WFE instruction
  * @retval None
  */
void HAL_PWREx_EnterSTOP2Mode(uint8_t STOPEntry)
{
  /* Check the parameter */
  assert_param(IS_PWR_STOP_ENTRY(STOPEntry));

  /* Set Stop mode 2 */
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_STOP2);

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
  * @brief Enter Shutdown mode.
  * @note  In Shutdown mode, the PLL, the HSI, the MSI, the LSI and the HSE oscillators are switched
  *        off. The voltage regulator is disabled and Vcore domain is powered off.
  *        SRAM1, SRAM2, BKRAM and registers contents are lost except for registers in the Backup domain.
  *        The BOR is not available.
  * @note  The I/Os can be configured either with a pull-up or pull-down or can be kept in analog state.
  * @note  According to system power policy, system entering in Shutdown mode
  *        is depending on other CPU power mode.
  * @retval None
  */
void HAL_PWREx_EnterSHUTDOWNMode(void)
{
  /* Set Shutdown mode */
  MODIFY_REG(PWR->CR1, PWR_CR1_LPMS, PWR_LOWPOWERMODE_SHUTDOWN);

  /* Set SLEEPDEEP bit of Cortex System Control Register */
  SET_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));

/* This option is used to ensure that store operations are completed */
#if defined ( __CC_ARM)
  __force_stores();
#endif

  /* Request Wait For Interrupt */
  __WFI();

  /* Following code is executed after wake up if system didn't go to SHUTDOWN
   * or STANDBY mode according to power policy */

  /* Reset SLEEPDEEP bit of Cortex System Control Register */
  CLEAR_BIT(SCB->SCR, ((uint32_t)SCB_SCR_SLEEPDEEP_Msk));
}


/**
  * @brief This function handles the PWR PVD/PVMx interrupt request.
  * @note This API should be called under the PVD_PVM_IRQHandler().
  * @retval None
  */
void HAL_PWREx_PVD_PVM_IRQHandler(void)
{
  /* Check PWR exti flag */
  if(__HAL_PWR_PVD_EXTI_GET_FLAG() != 0U)
  {
    /* PWR PVD interrupt user callback */
    HAL_PWR_PVDCallback();

    /* Clear PVD exti pending bit */
    __HAL_PWR_PVD_EXTI_CLEAR_FLAG();
  }


  /* Next, successively check PVMx exti flags */
  if(__HAL_PWR_PVM1_EXTI_GET_FLAG() != 0U)
  {
    /* PWR PVM1 interrupt user callback */
    HAL_PWREx_PVM1Callback();

    /* Clear PVM1 exti pending bit */
    __HAL_PWR_PVM1_EXTI_CLEAR_FLAG();
  }

  if(__HAL_PWR_PVM3_EXTI_GET_FLAG() != 0U)
  {
    /* PWR PVM3 interrupt user callback */
    HAL_PWREx_PVM3Callback();

    /* Clear PVM3 exti pending bit */
    __HAL_PWR_PVM3_EXTI_CLEAR_FLAG();
  }
}


/**
  * @brief PWR PVM1 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_PVM1Callback(void)
{
  /* NOTE : This function should not be modified; when the callback is needed,
            HAL_PWREx_PVM1Callback() API can be implemented in the user file
   */
}

/**
  * @brief PWR PVM3 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_PVM3Callback(void)
{
  /* NOTE : This function should not be modified; when the callback is needed,
            HAL_PWREx_PVM3Callback() API can be implemented in the user file
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
