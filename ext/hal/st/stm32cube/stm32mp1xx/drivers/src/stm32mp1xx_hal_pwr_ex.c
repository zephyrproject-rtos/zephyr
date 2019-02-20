/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_pwr_ex.c
  * @author  MCD Application Team
  * @brief   Extended PWR HAL module driver.
  *          This file provides firmware functions to manage the following
  *          functionalities of PWR extension peripheral:
  *           + Peripheral Extended features functions
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

/** @defgroup PWREx PWREx
  * @brief PWR HAL module driver
  * @{
  */

#ifdef HAL_PWR_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/** @addtogroup PWREx_Private_Constants PWREx Private Constants
  * @{
  */

/** @defgroup PWREx_AVD_Mode_Mask PWR Extended AVD Mode Mask
  * @{
  */
#define AVD_MODE_IT              ((uint32_t)0x00010000U)
#define AVD_MODE_EVT             ((uint32_t)0x00020000U)
#define AVD_RISING_EDGE          ((uint32_t)0x00000001U)
#define AVD_FALLING_EDGE         ((uint32_t)0x00000002U)
#define AVD_RISING_FALLING_EDGE  ((uint32_t)0x00000003U)
/**
  * @}
  */

/** @defgroup PWREx_REG_SET_TIMEOUT PWR Extended Flag Setting Time Out Value
  * @{
  */
#define PWR_FLAG_SETTING_DELAY_US  ((uint32_t)1000U)
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

/** @defgroup PWREx_Private_Functions PWREx Private Functions
  * @{
  */

/** @defgroup PWREx_Group1 Peripheral Extended features functions
  *  @brief Peripheral Extended features functions
  *
@verbatim

 ===============================================================================
                 ##### Peripheral extended features functions #####
 ===============================================================================

    *** Main and Backup Regulators configuration ***
    ================================================
    [..]
      (+) The backup domain includes 4 Kbytes of backup SRAM accessible only from
          the CPU, and address in 32-bit, 16-bit or 8-bit mode. Its content is
          retained even in Standby or VBAT mode when the low power backup regulator
          is enabled. It can be considered as an internal EEPROM when VBAT is
          always present. You can use the HAL_PWR_EnableBkUpReg() function to
          enable the low power backup regulator.

      (+) When the backup domain is supplied by VDD (analog switch connected to VDD)
          the backup SRAM is powered from VDD which replaces the VBAT power supply to
          save battery life.

      (+) The backup SRAM is not mass erased by a tamper event. It is read
          protected to prevent confidential data, such as cryptographic private
          key, from being accessed.

        Refer to the product datasheets for more details.


    *** VBAT battery charging ***
    =============================
    [..]
      (+) When VDD is present, the external battery connected to VBAT can be charged through an
          internal resistance. VBAT charging can be performed either through a 5 KOhm resistor
          or through a 1.5 KOhm resistor.
      (+) VBAT charging is enabled by HAL_PWREx_EnableBatteryCharging(ResistorValue) function
          with:
       (++) ResistorValue:
        (+++) PWR_BATTERY_CHARGING_RESISTOR_5: 5 KOhm resistor.
        (+++) PWR_BATTERY_CHARGING_RESISTOR_1_5: 1.5 KOhm resistor.
      (+) VBAT charging is disabled by HAL_PWREx_DisableBatteryCharging() function.

     *** VBAT and Temperature supervision ***
    ========================================
    [..]
      (+) The VBAT battery voltage supply can be monitored by comparing it with two threshold
          levels: VBAThigh and VBATlow. VBATH flag and VBATL flag in the PWR control register 2
          (PWR_CR2), indicate if VBAT is higher or lower than the threshold.
      (+) The temperature can be monitored by comparing it with two threshold levels, TEMPhigh
          and TEMPlow. TEMPH and TEMPL flags, in the PWR control register 2 (PWR_CR2),
          indicate whether the device temperature is higher or lower than the threshold.
      (+) The VBAT and the temperature monitoring is enabled by HAL_PWREx_EnableMonitoring()
          function and disabled by HAL_PWREx_DisableMonitoring() function.
      (+) The HAL_PWREx_GetVBATLevel() function return the VBAT level which can be:
          PWR_VBAT_BELOW_LOW_THRESHOLD or PWR_VBAT_ABOVE_HIGH_THRESHOLD or
          PWR_VBAT_BETWEEN_HIGH_LOW_THRESHOLD.
      (+) The HAL_PWREx_GetTemperatureLevel() function return the Temperature level which
          can be: PWR_TEMP_BELOW_LOW_THRESHOLD or PWR_TEMP_ABOVE_HIGH_THRESHOLD or
          PWR_TEMP_BETWEEN_HIGH_LOW_THRESHOLD.

    *** AVD configuration ***
    =========================
    [..]
      (+) The AVD is used to monitor the VDDA power supply by comparing it to a
          threshold selected by the AVD Level (ALS[3:0] bits in the PWR_CSR1 register).
      (+) A AVDO flag is available to indicate if VDDA is higher or lower
          than the AVD threshold. This event is internally connected to the EXTI
          line 16 to generate an interrupt if enabled.
          It is configurable through __HAL_PWR_PVD_AVD_EXTI_ENABLE_IT() macro.
      (+) The AVD is stopped in System Standby mode.


    *** USB Regulator supervision ***
    ===================================
    [..]
      (+) When the USB regulator is enabled, the VDD33USB supply level detector shall
          be enabled through  HAL_PWREx_EnableUSBVoltageDetector() function and disabled by
          HAL_PWREx_DisableUSBVoltageDetector() function.

@endverbatim
  * @{
  */



/**
  * @brief  Enables the Backup Regulator.
  * @note   After reset PWR_CR2 register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  *         Use HAL_PWR_EnableBkUpAccess() to do this.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_EnableBkUpReg(void)
{
  uint32_t tickstart = 0;

  /* Enable Backup regulator */
  SET_BIT(PWR->CR2, PWR_CR2_BREN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till Backup regulator ready flag is set */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_BRR) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/**
  * @brief  Disables the Backup Regulator.
  * @note   After reset PWR_CR2 register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  *         Use HAL_PWR_EnableBkUpAccess() to do this.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_DisableBkUpReg(void)
{
  uint32_t tickstart = 0;

  /* Disable Backup regulator */
  CLEAR_BIT(PWR->CR2, PWR_CR2_BREN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till Backup regulator ready flag is reset */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_BRR) != RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}

/**
  * @brief  Enables the Retention Regulator.
  * @note   After reset PWR_CR2 register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  *         Use HAL_PWR_EnableBkUpAccess() to do this.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_EnableRetReg(void)
{
  uint32_t tickstart = 0;

  /* Enable Backup regulator */
  SET_BIT(PWR->CR2, PWR_CR2_RREN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till Retention regulator ready flag is set */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_RRR) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/**
  * @brief  Disables the Retention Regulator.
  * @note   After reset PWR_CR2 register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  *         Use HAL_PWR_EnableBkUpAccess() to do this.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_DisableRetReg(void)
{
  uint32_t tickstart = 0;

  /* Disable Backup regulator */
  CLEAR_BIT(PWR->CR2, PWR_CR2_RREN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till Backup regulator ready flag is set */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_RRR) != RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}


/**
  * @brief  Enables the 1V1 Regulator.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_Enable1V1Reg(void)
{
  uint32_t tickstart = 0;

  /* Enable 1V1 regulator */
  SET_BIT(PWR->CR3, PWR_CR3_REG11EN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till 1V1 regulator ready flag is set */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_11R) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}


/**
  * @brief Disables the 1V1 Regulator.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_Disable1V1Reg(void)
{
  uint32_t tickstart = 0;

  /* Disable 1V1 regulator */
  CLEAR_BIT(PWR->CR3, PWR_CR3_REG11EN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till 1V1 regulator ready flag is reset */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_11R) != RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/**
  * @brief Enables the 1V8 Regulator.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_Enable1V8Reg(void)
{
  uint32_t tickstart = 0;

  /* Enable 1V8 regulator */
  SET_BIT(PWR->CR3, PWR_CR3_REG18EN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till 1V8 regulator ready flag is set */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_18R) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}


/**
  * @brief Disables the 1V8 Regulator.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_Disable1V8Reg(void)
{
  uint32_t tickstart = 0;

  /* Disable 1V8 regulator */
  CLEAR_BIT(PWR->CR3, PWR_CR3_REG18EN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait till 1V8 regulator ready flag is reset */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_18R) != RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}


/**
  * @brief  Enable the USB voltage level detector.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_EnableUSBVoltageDetector(void)
{
  uint32_t tickstart = 0;

  /* Enable the USB voltage detector */
  SET_BIT(PWR->CR3, PWR_CR3_USB33DEN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait until USB33 regulator ready flag is set */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB) == RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}

/**
  * @brief  Disable the USB voltage level detector.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_PWREx_DisableUSBVoltageDetector(void)
{
  uint32_t tickstart = 0;

  /* Disable the USB voltage detector */
  CLEAR_BIT(PWR->CR3, PWR_CR3_USB33DEN);

  /* Get tick */
  tickstart = HAL_GetTick();

  /* Wait until USB33 regulator ready flag is reset */
  while (__HAL_PWR_GET_FLAG(PWR_FLAG_USB) != RESET)
  {
    if ((HAL_GetTick() - tickstart) > PWR_FLAG_SETTING_DELAY_US)
    {
      return HAL_TIMEOUT;
    }
  }
  return HAL_OK;
}


/**
  * @brief  Enable the Battery charging.
  *         When VDD is present, charge the external battery through an internal resistor.
  * @param  ResistorValue: Specifies the charging resistor.
  *          This parameter can be one of the following values:
  *            @arg PWR_BATTERY_CHARGING_RESISTOR_5:   5 KOhm resistor.
  *            @arg PWR_BATTERY_CHARGING_RESISTOR_1_5: 1.5 KOhm resistor.
  * @retval None
  */
void HAL_PWREx_EnableBatteryCharging(uint32_t ResistorValue)
{
  assert_param(IS_PWR_BATTERY_RESISTOR_SELECT(ResistorValue));

  /* Specify the charging resistor */
  MODIFY_REG(PWR->CR3, PWR_CR3_VBRS, ResistorValue);

  /* Enable the Battery charging */
  SET_BIT(PWR->CR3, PWR_CR3_VBE);
}


/**
  * @brief  Disable the Battery charging.
  * @retval None
  */
void HAL_PWREx_DisableBatteryCharging(void)
{
  /* Disable the Battery charging */
  CLEAR_BIT(PWR->CR3, PWR_CR3_VBE);
}


/**
  * @brief  Enable the VBAT and temperature monitoring.
  * @note   After reset PWR_CR2 register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  *         Use HAL_PWR_EnableBkUpAccess() to do this.
  * @retval HAL status
  */
void HAL_PWREx_EnableMonitoring(void)
{
  /* Enable the VBAT and Temperature monitoring */
  SET_BIT(PWR->CR2, PWR_CR2_MONEN);
}

/**
  * @brief  Disable the VBAT and temperature monitoring.
  * @note   After reset PWR_CR2 register is write-protected and the DBP bit in the
  *         PWR control register 1 (PWR_CR1) has to be set before it can be written.
  *         Use HAL_PWR_EnableBkUpAccess() to do this.
  * @retval HAL status
  */
void HAL_PWREx_DisableMonitoring(void)
{
  /* Disable the VBAT and Temperature monitoring */
  CLEAR_BIT(PWR->CR2, PWR_CR2_MONEN);
}

/**
  * @brief  Indicate whether the junction temperature is between, above or below the threshold.
  * @retval Temperature level.
  */
uint32_t HAL_PWREx_GetTemperatureLevel(void)
{
  uint32_t tempLevel;
  uint32_t regValue;

  /* Read the temperature flags */
  regValue = PWR->CR2 & (PWR_CR2_TEMPH | PWR_CR2_TEMPL);

  /* Compare the read value to the temperature threshold */
  if (regValue == PWR_CR2_TEMPL)
  {
    tempLevel = PWR_TEMP_BELOW_LOW_THRESHOLD;
  }
  else if (regValue == PWR_CR2_TEMPH)
  {
    tempLevel = PWR_TEMP_ABOVE_HIGH_THRESHOLD;
  }
  else
  {
    tempLevel = PWR_TEMP_BETWEEN_HIGH_LOW_THRESHOLD;
  }

  return tempLevel;
}


/**
  * @brief  Indicate whether the Battery voltage level is between, above or below the threshold.
  * @retval VBAT level.
  */
uint32_t HAL_PWREx_GetVBATLevel(void)
{
  uint32_t VBATLevel;
  uint32_t regValue;

  /* Read the VBAT flags */
  regValue = PWR->CR2 & (PWR_CR2_VBATH | PWR_CR2_VBATL);

  /* Compare the read value to the VBAT threshold */
  if (regValue == PWR_CR2_VBATL)
  {
    VBATLevel = PWR_VBAT_BELOW_LOW_THRESHOLD;
  }
  else if (regValue == PWR_CR2_VBATH)
  {
    VBATLevel = PWR_VBAT_ABOVE_HIGH_THRESHOLD;
  }
  else
  {
    VBATLevel = PWR_VBAT_BETWEEN_HIGH_LOW_THRESHOLD;
  }

  return VBATLevel;
}


/**
  * @brief  Configure the analog voltage threshold detected by the Analog Voltage Detector(AVD).
  * @param  sConfigAVD: pointer to an PWR_AVDTypeDef structure that contains the configuration
  *                     information for the AVD.
  * @note   Refer to the electrical characteristics of your device datasheet for more details
  *         about the voltage threshold corresponding to each detection level.
  * @retval None
  */
void HAL_PWREx_ConfigAVD(PWREx_AVDTypeDef *sConfigAVD)
{
  /* Check the parameters */
  assert_param(IS_PWR_AVD_LEVEL(sConfigAVD->AVDLevel));
  assert_param(IS_PWR_AVD_MODE(sConfigAVD->Mode));

  /* Set the ALS[18:17] bits according to AVDLevel value */
  MODIFY_REG(PWR->CR1, PWR_CR1_ALS, sConfigAVD->AVDLevel);

  /* Clear any previous config. Keep it clear if no IT mode is selected */
  __HAL_PWR_PVD_AVD_EXTI_DISABLE_IT();
  __HAL_PWR_PVD_AVD_EXTI_DISABLE_RISING_EDGE();
  __HAL_PWR_PVD_AVD_EXTI_DISABLE_FALLING_EDGE();

  /* Configure the interrupt mode */
  if (AVD_MODE_IT == (sConfigAVD->Mode & AVD_MODE_IT))
  {
    __HAL_PWR_PVD_AVD_EXTI_ENABLE_IT();
  }

  /* Configure the edge */
  if (AVD_RISING_EDGE == (sConfigAVD->Mode & AVD_RISING_EDGE))
  {
    __HAL_PWR_PVD_AVD_EXTI_ENABLE_RISING_EDGE();
  }

  if (AVD_FALLING_EDGE == (sConfigAVD->Mode & AVD_FALLING_EDGE))
  {
    __HAL_PWR_PVD_AVD_EXTI_ENABLE_FALLING_EDGE();
  }
}


/**
  * @brief  Enable the Analog Voltage Detector(AVD).
  * @retval None
  */
void HAL_PWREx_EnableAVD(void)
{
  /* Enable the Analog Voltage Detector */
  SET_BIT(PWR->CR1, PWR_CR1_AVDEN);
}

/**
  * @brief  Disable the Analog Voltage Detector(AVD).
  * @retval None
  */
void HAL_PWREx_DisableAVD(void)
{
  /* Disable the Analog Voltage Detector */
  CLEAR_BIT(PWR->CR1, PWR_CR1_AVDEN);
}


/**
  * @brief  This function handles the PWR PVD/AVD interrupt request.
  * @note   This API should be called under the PVD_AVD_IRQHandler().
  * @retval None
  */
void HAL_PWREx_PVD_AVD_IRQHandler(void)
{
  /* PVD EXTI line interrupt detected */
  if (READ_BIT(PWR->CR1, PWR_CR1_PVDEN) != RESET)
  {
    /* PWR PVD interrupt user callback */
    HAL_PWR_PVDCallback();
  }

  /* AVD EXTI line interrupt detected */
  if (READ_BIT(PWR->CR1, PWR_CR1_AVDEN) != RESET)
  {
    /* PWR AVD interrupt user callback */
    HAL_PWREx_AVDCallback();
  }

  /* Clear PWR PVD AVD EXTI pending bit */
  __HAL_PWR_PVD_AVD_EXTI_CLEAR_FLAG();
}

/**
  * @brief  PWR AVD interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_AVDCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_AVDCallback could be implemented in the user file
   */
}


void HAL_PWREx_WAKEUP_PIN_IRQHandler(void)
{

  /* Wakeup pin EXTI line interrupt detected */
  if (READ_BIT(PWR->WKUPFR, PWR_WKUPFR_WKUPF1) != RESET)
  {
    /* Clear PWR WKUPF1 flag */
    SET_BIT(PWR->WKUPCR, PWR_WKUPCR_WKUPC1);

    /* PWR WKUP1 interrupt user callback */
    HAL_PWREx_WKUP1_Callback();
  }

  if (READ_BIT(PWR->WKUPFR, PWR_WKUPFR_WKUPF2)  != RESET)
  {
    /* Clear PWR WKUPF2 flag */
    SET_BIT(PWR->WKUPCR, PWR_WKUPCR_WKUPC2);

    /* PWR WKUP2 interrupt user callback */
    HAL_PWREx_WKUP2_Callback();
  }

  if (READ_BIT(PWR->WKUPFR, PWR_WKUPFR_WKUPF3)  != RESET)
  {
    /* Clear PWR WKUPF3 flag */
    SET_BIT(PWR->WKUPCR, PWR_WKUPCR_WKUPC3);

    /* PWR WKUP3 interrupt user callback */
    HAL_PWREx_WKUP3_Callback();
  }

  if (READ_BIT(PWR->WKUPFR, PWR_WKUPFR_WKUPF4)  != RESET)
  {
    /* Clear PWR WKUPF4 flag */
    SET_BIT(PWR->WKUPCR, PWR_WKUPCR_WKUPC4);

    /* PWR WKUP4 interrupt user callback */
    HAL_PWREx_WKUP4_Callback();
  }

  if (READ_BIT(PWR->WKUPFR, PWR_WKUPFR_WKUPF5)  != RESET)
  {
    /* Clear PWR WKUPF5 flag */
    SET_BIT(PWR->WKUPCR, PWR_WKUPCR_WKUPC5);

    /* PWR WKUP5 interrupt user callback */
    HAL_PWREx_WKUP5_Callback();
  }

  if (READ_BIT(PWR->WKUPFR, PWR_WKUPFR_WKUPF6)  != RESET)
  {
    /* Clear PWR WKUPF6 flag */
    SET_BIT(PWR->WKUPCR, PWR_WKUPCR_WKUPC6);

    /* PWR WKUP6 interrupt user callback */
    HAL_PWREx_WKUP6_Callback();
  }

}

/**
  * @brief  PWR WKUP1 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_WKUP1_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_WKUP1Callback could be implemented in the user file
  */
}

/**
  * @brief  PWR WKUP2 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_WKUP2_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_WKUP2Callback could be implemented in the user file
  */
}

/**
  * @brief  PWR WKUP3 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_WKUP3_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_WKUP3Callback could be implemented in the user file
  */
}

/**
  * @brief  PWR WKUP4 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_WKUP4_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_WKUP4Callback could be implemented in the user file
  */
}

/**
  * @brief  PWR WKUP5 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_WKUP5_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_WKUP5Callback could be implemented in the user file
  */
}

/**
  * @brief  PWR WKUP6 interrupt callback
  * @retval None
  */
__weak void HAL_PWREx_WKUP6_Callback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWREx_WKUP6Callback could be implemented in the user file
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
