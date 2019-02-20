/**
  ******************************************************************************
  * @file    stm32mp1xx_ll_system.h
  * @author  MCD Application Team
  * @brief   Header file of SYSTEM LL module.
  @verbatim
  ==============================================================================
                     ##### How to use this driver #####
  ==============================================================================
    [..]
    The LL SYSTEM driver contains a set of generic APIs that can be
    used by user:
      (+) Access to DBGCMU registers
      (+) Access to SYSCFG registers

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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32MP1xx_LL_SYSTEM_H
#define STM32MP1xx_LL_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx.h"

/** @addtogroup STM32MP1xx_LL_Driver
  * @{
  */

#if defined (SYSCFG) || defined (DBGMCU)

/** @defgroup SYSTEM_LL SYSTEM
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private constants ---------------------------------------------------------*/
/** @defgroup SYSTEM_LL_Private_Constants SYSTEM Private Constants
  * @{
  */
/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup SYSTEM_LL_Exported_Constants SYSTEM Exported Constants
  * @{
  */

/** @defgroup SYSTEM_LL_EC_BOOT_PIN SYSCFG Boot Pin code selection
  * @{
  */
#define LL_SYSCFG_BOOT0           SYSCFG_BOOTR_BOOT0       /*!< BOOT0 pin connected to VDD */
#define LL_SYSCFG_BOOT1           SYSCFG_BOOTR_BOOT1       /*!< BOOT1 pin connected to VDD */
#define LL_SYSCFG_BOOT2           SYSCFG_BOOTR_BOOT2       /*!< BOOT2 pin connected to VDD */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_BOOT_PIN_PD SYSCFG Boot Pin Pull-Down code selection
  * @{
  */
#define LL_SYSCFG_BOOT0_PD        SYSCFG_BOOTR_BOOT0_PD    /*!< Enable BOOT0 pin pull-down disabled */
#define LL_SYSCFG_BOOT1_PD        SYSCFG_BOOTR_BOOT1_PD    /*!< Enable BOOT1 pin pull-down disabled */
#define LL_SYSCFG_BOOT2_PD        SYSCFG_BOOTR_BOOT2_PD    /*!< Enable BOOT2 pin pull-down disbaled */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_ANALOG_SWITCH Analog Switch control
* @{
*/
#define LL_SYSCFG_ANALOG_SWITCH_PA0            SYSCFG_PMCSETR_ANA0_SEL_SEL   /*!< PA0 Switch Open */
#define LL_SYSCFG_ANALOG_SWITCH_PA1            SYSCFG_PMCSETR_ANA1_SEL_SEL   /*!< PA1 Switch Open */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_I2C_FASTMODEPLUS SYSCFG I2C FASTMODEPLUS
  * @{
  */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C1        SYSCFG_PMCSETR_I2C1_FMP       /*!< Enable Fast Mode Plus for I2C1      */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C2        SYSCFG_PMCSETR_I2C2_FMP       /*!< Enable Fast Mode Plus for I2C2      */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C3        SYSCFG_PMCSETR_I2C3_FMP       /*!< Enable Fast Mode Plus for I2C3      */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C4        SYSCFG_PMCSETR_I2C4_FMP       /*!< Enable Fast Mode Plus for I2C4      */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C5        SYSCFG_PMCSETR_I2C5_FMP       /*!< Enable Fast Mode Plus for I2C5      */
#define LL_SYSCFG_I2C_FASTMODEPLUS_I2C6        SYSCFG_PMCSETR_I2C6_FMP       /*!< Enable Fast Mode Plus for I2C6      */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_ETHERNET_PHY Ethernet PHY Interface Selection
* @{
*/
#define LL_SYSCFG_ETH_GMII              0x00000000U          /*!< Enable ETH Media GMII interface */
#define LL_SYSCFG_ETH_MII               0x00100000U          /*!< Enable ETH Media MII interface */
#define LL_SYSCFG_ETH_RGMII             0x00200000U          /*!< Enable ETH Media RGMII interface */
#define LL_SYSCFG_ETH_RMII              0x00800000U          /*!< Enable ETH Media RMII interface */
/**
  * @}
  */


/** @defgroup SYSTEM_LL_EC_ETHERNET_MII Ethernet MII Mode Selection
* @{
*/
#define LL_SYSCFG_ETH_CLK               SYSCFG_PMCSETR_ETH_CLK_SEL           /*!< Enable ETH clock selection */
#define LL_SYSCFG_ETH_REF_CLK           SYSCFG_PMCSETR_ETH_REF_CLK_SEL       /*!< Enable ETH REF clock selection */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TIMBREAK SYSCFG TIMER BREAK
  * @{
  */
#define LL_SYSCFG_TIMBREAK_PVD             SYSCFG_CBR_PVDL    /*!< Enables and locks the PVD connection
                                                                   with TIM1/8/15/16/17 Break Input
                                                                   and also the PVDE and PLS bits of the Power Control Interface */
#define LL_SYSCFG_TIMBREAK_CM4_LOCKUP      SYSCFG_CBR_CLL     /*!< Enables and locks the Cortex-M4 LOCKUP signal
                                                                   with Break Input of TIM1/8/15/16/17 */

/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_IO_COMPENSATION SYSCFG I/O compensation cell code selection
  * @{
  */
#define LL_SYSCFG_CELL_CODE               0U                     /*!< Disable Compensation Software Control */
#define LL_SYSCFG_REGISTER_CODE           SYSCFG_CMPCR_SW_CTRL   /*!< Enable Compensation Software Control */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_AXI Master code selection
  * @{
  */
#define LL_SYSCFG_AXI_MASTER0_S1        SYSCFG_ICNR_AXI_M0       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER1_S1        SYSCFG_ICNR_AXI_M1       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER2_S1        SYSCFG_ICNR_AXI_M2       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER3_S1        SYSCFG_ICNR_AXI_M3       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER5_S1        SYSCFG_ICNR_AXI_M5       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER6_S1        SYSCFG_ICNR_AXI_M6       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER7_S1        SYSCFG_ICNR_AXI_M7       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER8_S1        SYSCFG_ICNR_AXI_M8       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER9_S1        SYSCFG_ICNR_AXI_M9       /*!< Enable AXI Master access DDR through Slave S1 */
#define LL_SYSCFG_AXI_MASTER10_S1       SYSCFG_ICNR_AXI_M10      /*!< Enable AXI Master access DDR through Slave S1 */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_TRIGGER_OUTPUT DBGMCU TRIGGER output direction
  * @{
  */
#define LL_DBGMCU_TRGIO_INPUT_DIRECTION   0U			/*!< Disable External trigger output */
#define LL_DBGMCU_TRGIO_OUTPUT_DIRECTION  DBGMCU_CR_DBG_TRGOEN  /*!< Enable External trigger output */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_IWDG DBGMCU IWDG Freeze Watchdog
  * @{
  */
#define LL_DBGMCU_IWDG_FREEZE_1CA7_HALT   0U                    /*!< Freeze Watchdog timer when either CA7 core halt in debug mode */
#define LL_DBGMCU_IWDG_FREEZE_2CA7_HALT   DBGMCU_CR_DBG_WDFZCTL /*!< Freeze Watchdog timer when both CA7 cores halt in debug mode */
/**
  * @}
  */


/** @defgroup SYSTEM_LL_EC_APB1_GRP1_STOP_IP DBGMCU APB1 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB1_GRP1_TIM2_STOP      DBGMCU_APB1_FZ_DBG_TIM2_STOP     /*!< TIM2 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM3_STOP      DBGMCU_APB1_FZ_DBG_TIM3_STOP     /*!< TIM3 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM4_STOP      DBGMCU_APB1_FZ_DBG_TIM4_STOP     /*!< TIM4 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM5_STOP      DBGMCU_APB1_FZ_DBG_TIM5_STOP     /*!< TIM5 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM6_STOP      DBGMCU_APB1_FZ_DBG_TIM6_STOP     /*!< TIM6 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM7_STOP      DBGMCU_APB1_FZ_DBG_TIM7_STOP     /*!< TIM7 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM12_STOP     DBGMCU_APB1_FZ_DBG_TIM12_STOP    /*!< TIM12 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM13_STOP     DBGMCU_APB1_FZ_DBG_TIM13_STOP    /*!< TIM13 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_TIM14_STOP     DBGMCU_APB1_FZ_DBG_TIM14_STOP    /*!< TIM14 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_LPTIM1_STOP    DBGMCU_APB1_FZ_DBG_LPTIM1_STOP   /*!< LPTIM1 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_WWDG1_STOP     DBGMCU_APB1_FZ_DBG_WWDG1_STOP    /*!< WWDG1 counter stopped when core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C1_STOP      DBGMCU_APB1_FZ_DBG_I2C1_STOP     /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C2_STOP      DBGMCU_APB1_FZ_DBG_I2C2_STOP     /*!< I2C2 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C3_STOP      DBGMCU_APB1_FZ_DBG_I2C3_STOP     /*!< I2C3 SMBUS timeout mode stopped when Core is halted */
#define LL_DBGMCU_APB1_GRP1_I2C5_STOP      DBGMCU_APB1_FZ_DBG_I2C5_STOP     /*!< I2C5 SMBUS timeout mode stopped when Core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB2_GRP1_STOP_IP DBGMCU APB2 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB2_GRP1_TIM1_STOP     DBGMCU_APB2_FZ_DBG_TIM1_STOP     /*!< TIM1 is frozen while the core is in debug mode */
#define LL_DBGMCU_APB2_GRP1_TIM8_STOP     DBGMCU_APB2_FZ_DBG_TIM8_STOP     /*!< TIM8 is frozen while the core is in debug mode */
#define LL_DBGMCU_APB2_GRP1_TIM15_STOP    DBGMCU_APB2_FZ_DBG_TIM15_STOP    /*!< TIM15 is frozen while the core is in debug mode */
#define LL_DBGMCU_APB2_GRP1_TIM16_STOP    DBGMCU_APB2_FZ_DBG_TIM16_STOP    /*!< TIM16 is frozen while the core is in debug mode */
#define LL_DBGMCU_APB2_GRP1_TIM17_STOP    DBGMCU_APB2_FZ_DBG_TIM17_STOP    /*!< TIM17 is frozen while the core is in debug mode */
#define LL_DBGMCU_APB2_GRP1_FDCAN_STOP    DBGMCU_APB2_FZ_DBG_FDCAN_STOP    /*!< FDCAN is frozen while the core is in debug mode */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB3_GRP1_STOP_IP DBGMCU APB3 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB3_GRP1_LPTIM2_STOP     DBGMCU_APB3_FZ_DBG_LPTIM2_STOP   /*!< LPTIM2 counter stopped when core is halted */
#define LL_DBGMCU_APB3_GRP1_LPTIM3_STOP     DBGMCU_APB3_FZ_DBG_LPTIM3_STOP   /*!< LPTIM3 counter stopped when core is halted */
#define LL_DBGMCU_APB3_GRP1_LPTIM4_STOP     DBGMCU_APB3_FZ_DBG_LPTIM4_STOP   /*!< LPTIM4 counter stopped when core is halted */
#define LL_DBGMCU_APB3_GRP1_LPTIM5_STOP     DBGMCU_APB3_FZ_DBG_LPTIM5_STOP   /*!< LPTIM5 counter stopped when core is halted */
/**
  * @}
  */

/** @defgroup SYSTEM_LL_EC_APB5_GRP1_STOP_IP DBGMCU APB5 GRP1 STOP IP
  * @{
  */
#define LL_DBGMCU_APB5_GRP1_I2C4_STOP       DBGMCU_APB5_FZ_DBG_I2C4_STOP     /*!< I2C4 is frozen while the core is in debug mode */
#define LL_DBGMCU_APB5_GRP1_RTC_STOP        DBGMCU_APB5_FZ_DBG_RTC_STOP      /*!< RTC is frozen while the core is in debug mode */
#define LL_DBGMCU_APB5_GRP1_I2C6_STOP       DBGMCU_APB5_FZ_DBG_I2C6_STOP     /*!< I2C6 is frozen while the core is in debug mode */
/**
  * @}
  */
/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
/** @defgroup SYSTEM_LL_Exported_Functions SYSTEM Exported Functions
  * @{
  */

/** @defgroup SYSTEM_LL_EF_SYSCFG SYSCFG
  * @{
  */

/**
  * @brief  Get boot pin value connected to VDD
  * @rmtoll SYSCFG_BOOTR   BOOTx   LL_SYSCFG_GetBootPinValueToVDD
  * @param  BootPinValue This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_BOOT0
  *         @arg @ref LL_SYSCFG_BOOT1
  *         @arg @ref LL_SYSCFG_BOOT2
  * @retval None
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetBootPinValueToVDD(uint16_t BootPinValue)
{
  return (uint32_t)(READ_BIT(SYSCFG->BOOTR, BootPinValue));
}

/**
  * @brief  Enable boot pin pull-down disabled
  * @rmtoll SYSCFG_BOOTR   BOOTx_PD   LL_SYSCFG_EnableBootPinPullDownDisabled
  * @param  BootPinPDValue This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_BOOT0_PD
  *         @arg @ref LL_SYSCFG_BOOT1_PD
  *         @arg @ref LL_SYSCFG_BOOT2_PD
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableBootPinPullDownDisabled(uint16_t BootPinPDValue)
{
  SET_BIT(SYSCFG->BOOTR, BootPinPDValue);
}

/**
  * @brief  Disable boot pin pull-down disabled
  * @rmtoll SYSCFG_BOOTR   BOOTx_PD   LL_SYSCFG_DisableBootPinPullDownDisabled
  * @param  BootPinPDValue This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_BOOT0_PD
  *         @arg @ref LL_SYSCFG_BOOT1_PD
  *         @arg @ref LL_SYSCFG_BOOT2_PD
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableBootPinPullDownDisabled(uint16_t BootPinPDValue)
{
  CLEAR_BIT(SYSCFG->BOOTR, BootPinPDValue);
}

/**
  * @brief  Set Ethernet PHY interface
  * @rmtoll SYSCFG_PMCSETR   ETH_SEL      LL_SYSCFG_SetPHYInterface
  * @rmtoll SYSCFG_PMCSETR   ETH_SELMII   LL_SYSCFG_SetPHYInterface
  * @param  Interface : Interface This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ETH_MII
  *         @arg @ref LL_SYSCFG_ETH_GMII
  *         @arg @ref LL_SYSCFG_ETH_RGMII
  *         @arg @ref LL_SYSCFG_ETH_RMII
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetPHYInterface(uint32_t Interface)
{
  MODIFY_REG(SYSCFG->PMCCLRR, SYSCFG_PMCCLRR_ETH_SEL_CONF, SYSCFG_PMCCLRR_ETH_SEL_CONF);
  MODIFY_REG(SYSCFG->PMCSETR, SYSCFG_PMCSETR_ETH_SEL_CONF, Interface);
}

/**
  * @brief  Get Ethernet PHY interface
  * @rmtoll SYSCFG_PMCSETR   ETH_SEL      LL_SYSCFG_GetPHYInterface
  * @rmtoll SYSCFG_PMCSETR   ETH_SELMII   LL_SYSCFG_GetPHYInterface
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_SYSCFG_ETH_MII
  *         @arg @ref LL_SYSCFG_ETH_RMII
  *         @arg @ref LL_SYSCFG_ETH_GMII
  *         @arg @ref LL_SYSCFG_ETH_RGMII
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetPHYInterface(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->PMCSETR, SYSCFG_PMCSETR_ETH_SEL_CONF));
}

/**
  * @brief  Clear Ethernet PHY mode
  * @rmtoll SYSCFG_PMCSETR   ETH_SEL      LL_SYSCFG_ClearPHYInterface
  * @rmtoll SYSCFG_PMCSETR   ETH_SELMII   LL_SYSCFG_ClearPHYInterface
  * @param  Interface : Interface This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ETH_MII
  *         @arg @ref LL_SYSCFG_ETH_GMII
  *         @arg @ref LL_SYSCFG_ETH_RGMII
  *         @arg @ref LL_SYSCFG_ETH_RMII
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_ClearPHYInterface(uint32_t Interface)
{
  MODIFY_REG(SYSCFG->PMCCLRR, SYSCFG_PMCCLRR_ETH_SEL_CONF, Interface);
}

/**
  * @brief  Enable Ethernet clock
  * @rmtoll SYSCFG_PMCSETR   ETH_CLK_SEL       LL_SYSCFG_EnablePHYInternalClock\n
  *         SYSCFG_PMCSETR   ETH_REF_CLK_SEL   LL_SYSCFG_EnablePHYInternalClock
  * @param  Clock : This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ETH_CLK
  *         @arg @ref LL_SYSCFG_ETH_REF_CLK
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnablePHYInternalClock(uint32_t Clock)
{
  MODIFY_REG(SYSCFG->PMCSETR, (SYSCFG_PMCSETR_ETH_CLK_SEL | SYSCFG_PMCSETR_ETH_REF_CLK_SEL), Clock);
}

/**
  * @brief  Disable Ethernet clock
  * @rmtoll SYSCFG_PMCCLRR   ETH_CLK_SEL       LL_SYSCFG_DisablePHYInternalClock\n
  *         SYSCFG_PMCCLRR   ETH_REF_CLK_SEL   LL_SYSCFG_DisablePHYInternalClock
  * @param  Clock : This parameter can be one of the following values:
  *         @arg @ref LL_SYSCFG_ETH_CLK
  *         @arg @ref LL_SYSCFG_ETH_REF_CLK
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisablePHYInternalClock(uint32_t Clock)
{
  MODIFY_REG(SYSCFG->PMCCLRR, (SYSCFG_PMCCLRR_ETH_CLK_SEL | SYSCFG_PMCCLRR_ETH_REF_CLK_SEL), Clock);
}


/**
  * @brief  Open an Analog Switch
  * @rmtoll SYSCFG_PMCSETR   ANA0_SEL   LL_SYSCFG_OpenAnalogSwitch
  *         SYSCFG_PMCSETR   ANA1_SEL   LL_SYSCFG_OpenAnalogSwitch
  * @param  AnalogSwitch This parameter can be one of the following values:
  *         @arg LL_SYSCFG_ANALOG_SWITCH_PA0 : PA0 analog switch
  *         @arg LL_SYSCFG_ANALOG_SWITCH_PA1:  PA1 analog switch
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_OpenAnalogSwitch(uint32_t AnalogSwitch)
{
  SET_BIT(SYSCFG->PMCSETR, AnalogSwitch);
}

/**
  * @brief  Close an Analog Switch
  * @rmtoll SYSCFG_PMCCLRR   ANA0_SEL   LL_SYSCFG_CloseAnalogSwitch
  *         SYSCFG_PMCCLRR   ANA1_SEL   LL_SYSCFG_CloseAnalogSwitch
  * @param  AnalogSwitch This parameter can be one of the following values:
  *         @arg LL_SYSCFG_ANALOG_SWITCH_PA0 : PA0 analog switch
  *         @arg LL_SYSCFG_ANALOG_SWITCH_PA1:  PA1 analog switch
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_CloseAnalogSwitch(uint32_t AnalogSwitch)
{
  MODIFY_REG(SYSCFG->PMCCLRR, (SYSCFG_PMCCLRR_ANA0_SEL_SEL | SYSCFG_PMCCLRR_ANA1_SEL_SEL), AnalogSwitch);
}

/**
  * @brief  Enable the Analog GPIO switch to control voltage selection 
  *         when the supply voltage is supplied by VDDA
  * @rmtoll SYSCFG_PMCSETR   ANASWVDD   LL_SYSCFG_EnableAnalogGpioSwitch
  * @note   Activating the gpio switch enable IOs analog switches supplied by VDDA
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableAnalogGpioSwitch(void)
{
  SET_BIT(SYSCFG->PMCSETR, SYSCFG_PMCSETR_ANASWVDD);
}

/**
  * @brief  Disable the Analog GPIO switch to control voltage selection 
  *         when the supply voltage is supplied by VDDA
  * @rmtoll SYSCFG_PMCCLRR   ANASWVDD   LL_SYSCFG_DisableAnalogGpioSwitch
  * @note   Activating the gpio switch enable IOs analog switches supplied by VDDA
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableAnalogGpioSwitch(void)
{
  SET_BIT(SYSCFG->PMCCLRR, SYSCFG_PMCCLRR_ANASWVDD);
}

/**
  * @brief  Enable the Analog booster to reduce the total harmonic distortion
  *         of the analog switch when the supply voltage is lower than 2.7 V
  * @rmtoll SYSCFG_PMCSETR   EN_BOOSTER   LL_SYSCFG_EnableAnalogBooster
  * @note   Activating the booster allows to guaranty the analog switch AC performance
  *         when the supply voltage is below 2.7 V: in this case, the analog switch
  *         performance is the same on the full voltage range
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableAnalogBooster(void)
{
  SET_BIT(SYSCFG->PMCSETR, SYSCFG_PMCSETR_EN_BOOSTER);
}

/**
  * @brief  Disable the Analog booster
  * @rmtoll SYSCFG_PMCCLRR   EN_BOOSTER   LL_SYSCFG_DisableAnalogBooster
  * @note   Activating the booster allows to guaranty the analog switch AC performance
  *         when the supply voltage is below 2.7 V: in this case, the analog switch
  *         performance is the same on the full voltage range
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableAnalogBooster(void)
{
  SET_BIT(SYSCFG->PMCCLRR, SYSCFG_PMCCLRR_EN_BOOSTER);
}

/**
  * @brief  Enable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_PMCSETR   I2Cx_FMP   LL_SYSCFG_EnableFastModePlus\n
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C4
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C5
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C6
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableFastModePlus(uint32_t ConfigFastModePlus)
{
  SET_BIT(SYSCFG->PMCSETR, ConfigFastModePlus);
}

/**
  * @brief  Disable the I2C fast mode plus driving capability.
  * @rmtoll SYSCFG_PMCCLRR   I2Cx_FMP   LL_SYSCFG_DisableFastModePlus\n
  * @param  ConfigFastModePlus This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C1
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C2
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C3
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C4
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C5
  *         @arg @ref LL_SYSCFG_I2C_FASTMODEPLUS_I2C6
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableFastModePlus(uint32_t ConfigFastModePlus)
{
  MODIFY_REG(SYSCFG->PMCCLRR, (SYSCFG_PMCCLRR_I2C1_FMP | SYSCFG_PMCCLRR_I2C2_FMP | SYSCFG_PMCCLRR_I2C3_FMP | SYSCFG_PMCCLRR_I2C4_FMP | SYSCFG_PMCCLRR_I2C5_FMP | SYSCFG_PMCCLRR_I2C6_FMP), ConfigFastModePlus);
}

/**
  * @brief  Enable the Compensation Cell
  * @rmtoll SYSCFG_CMPENSETR   MCU_EN   LL_SYSCFG_EnableCompensationCell
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableCompensationCell(void)
{
  SET_BIT(SYSCFG->CMPENSETR, SYSCFG_CMPENSETR_MCU_EN);
}

/**
  * @brief  Disable the Compensation Cell
  * @rmtoll SYSCFG_CMPENCLRR   MCU_EN   LL_SYSCFG_DisableCompensationCell
  * @note   The I/O compensation cell can be used only when the device supply
  *         voltage ranges from 2.4 to 3.6 V
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableCompensationCell(void)
{
  SET_BIT(SYSCFG->CMPENCLRR, SYSCFG_CMPENCLRR_MCU_EN);
}

/**
  * @brief  Check if the Compensation Cell is enabled
  * @rmtoll SYSCFG_CMPENSETR   MCU_EN   LL_SYSCFG_IsEnabledCompensationCell
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledCompensationCell(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPENSETR, SYSCFG_CMPENSETR_MCU_EN) == SYSCFG_CMPENSETR_MCU_EN);
}

/**
  * @brief  Get Compensation Cell ready Flag
  * @rmtoll SYSCFG_CMPCR   READY   LL_SYSCFG_IsReadyCompensationCell
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsReadyCompensationCell(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_READY) == (SYSCFG_CMPCR_READY));
}

/**
  * @brief  Enable the SPI I/O speed optimization when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_SPI   LL_SYSCFG_EnableIOSpeedOptimizeSPI
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIOSpeedOptimizationSPI(void)
{
  SET_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_SPI);
}

/**
  * @brief  To Disable optimize the SPI I/O speed when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLCLRR   HSLVEN_SPI   LL_SYSCFG_DisableIOSpeedOptimizeSPI
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIOSpeedOptimizationSPI(void)
{
  MODIFY_REG(SYSCFG->IOCTRLCLRR, SYSCFG_IOCTRLCLRR_HSLVEN_SPI, SYSCFG_IOCTRLCLRR_HSLVEN_SPI);
}

/**
  * @brief  Check if the SPI I/O speed optimization is enabled
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_SPI   LL_SYSCFG_IsEnabledIOSpeedOptimizationSPI
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIOSpeedOptimizationSPI(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_SPI) == SYSCFG_IOCTRLSETR_HSLVEN_SPI);
}

/**
  * @brief  Enable the SDMMC I/O speed optimization when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_SDMMC   LL_SYSCFG_EnableIOSpeedOptimizeSDMMC
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIOSpeedOptimizationSDMMC(void)
{
  SET_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_SDMMC);
}

/**
  * @brief  To Disable optimize the SDMMC I/O speed when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLCLRR   HSLVEN_SDMMC   LL_SYSCFG_DisableIOSpeedOptimizeSDMMC
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIOSpeedOptimizationSDMMC(void)
{
  MODIFY_REG(SYSCFG->IOCTRLCLRR, SYSCFG_IOCTRLCLRR_HSLVEN_SDMMC, SYSCFG_IOCTRLCLRR_HSLVEN_SDMMC);
}

/**
  * @brief  Check if the SDMMC I/O speed optimization is enabled
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_SDMMC   LL_SYSCFG_IsEnabledIOSpeedOptimizationSDMMC
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIOSpeedOptimizationSDMMC(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_SDMMC) == SYSCFG_IOCTRLSETR_HSLVEN_SDMMC);
}

/**
  * @brief  Enable the ETH I/O speed optimization when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_ETH   LL_SYSCFG_EnableIOSpeedOptimizeETH
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIOSpeedOptimizationETH(void)
{
  SET_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_ETH);
}

/**
  * @brief  To Disable optimize the ETH I/O speed when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLCLRR   HSLVEN_ETH   LL_SYSCFG_DisableIOSpeedOptimizeETH
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIOSpeedOptimizationETH(void)
{
  MODIFY_REG(SYSCFG->IOCTRLCLRR, SYSCFG_IOCTRLCLRR_HSLVEN_ETH, SYSCFG_IOCTRLCLRR_HSLVEN_ETH);
}

/**
  * @brief  Check if the ETH I/O speed optimization is enabled
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_ETH   LL_SYSCFG_IsEnabledIOSpeedOptimizationETH
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIOSpeedOptimizationETH(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_ETH) == SYSCFG_IOCTRLSETR_HSLVEN_ETH);
}


/**
  * @brief  Enable the QUADSPI I/O speed optimization when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_QUADSPI   LL_SYSCFG_EnableIOSpeedOptimizeQUADSPI
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIOSpeedOptimizationQUADSPI(void)
{
  SET_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_QUADSPI);
}

/**
  * @brief  To Disable optimize the QUADSPI I/O speed when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLCLRR   HSLVEN_QUADSPI   LL_SYSCFG_DisableIOSpeedOptimizeQUADSPI
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIOSpeedOptimizationQUADSPI(void)
{
  MODIFY_REG(SYSCFG->IOCTRLCLRR, SYSCFG_IOCTRLCLRR_HSLVEN_QUADSPI, SYSCFG_IOCTRLCLRR_HSLVEN_QUADSPI);
}

/**
  * @brief  Check if the QUADSPI I/O speed optimization is enabled
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_QUADSPI   LL_SYSCFG_IsEnabledIOSpeedOptimizationQUADSPI
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIOSpeedOptimizationQUADSPI(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_QUADSPI) == SYSCFG_IOCTRLSETR_HSLVEN_QUADSPI);
}

/**
  * @brief  Enable the TRACE I/O speed optimization when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_TRACE   LL_SYSCFG_EnableIOSpeedOptimizeTRACE
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableIOSpeedOptimizationTRACE(void)
{
  SET_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_TRACE);
}

/**
  * @brief  To Disable optimize the TRACE I/O speed when the product voltage is low.
  * @rmtoll SYSCFG_IOCTRLCLRR   HSLVEN_TRACE   LL_SYSCFG_DisableIOSpeedOptimizeTRACE
  * @note   This bit is active only if IO_HSLV user option bit is set. It must be used only if the
  *         product supply voltage is below 2.7 V. Setting this bit when VDD is higher than 2.7 V
  *         might be destructive.
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableIOSpeedOptimizationTRACE(void)
{
  MODIFY_REG(SYSCFG->IOCTRLCLRR, SYSCFG_IOCTRLCLRR_HSLVEN_TRACE, SYSCFG_IOCTRLCLRR_HSLVEN_TRACE);
}

/**
  * @brief  Check if the TRACE I/O speed optimization is enabled
  * @rmtoll SYSCFG_IOCTRLSETR   HSLVEN_TRACE   LL_SYSCFG_IsEnabledIOSpeedOptimizationTRACE
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_SYSCFG_IsEnabledIOSpeedOptimizationTRACE(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->IOCTRLSETR, SYSCFG_IOCTRLSETR_HSLVEN_TRACE) == SYSCFG_IOCTRLSETR_HSLVEN_TRACE);
}


/**
  * @brief  Enable the AXI master access DDR through slave S1
  * @rmtoll SYSCFG_ICNR   AXI_Mx   LL_SYSCFG_EnableAXIMasterDDRAccessS1
  * @param  ConfigAxiMaster This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_AXI_MASTER0_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER1_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER2_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER3_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER5_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER6_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER7_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER8_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER9_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER10_S1
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_EnableAXIMasterDDRAccessS1(uint32_t ConfigAxiMaster)
{
  SET_BIT(SYSCFG->ICNR, ConfigAxiMaster);
}

/**
  * @brief  Disable the AXI master access DDR through slave S1
  * @rmtoll SYSCFG_ICNR   AXI_Mx   LL_SYSCFG_DisableFAXIMasterDDRAccessS1
  * @param  ConfigAxiMaster This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_AXI_MASTER0_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER1_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER2_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER3_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER5_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER6_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER7_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER8_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER9_S1
  *         @arg @ref LL_SYSCFG_AXI_MASTER10_S1
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_DisableAXIMasterDDRAccessS1(uint32_t ConfigAxiMaster)
{
  CLEAR_BIT(SYSCFG->ICNR, ConfigAxiMaster);
}

/**
  * @brief  Set the code selection for the I/O Compensation cell
  * @rmtoll SYSCFG_CMPCR   SW_CTRL   LL_SYSCFG_SetCellCompensationCode
  * @param  CompCode: Selects the code to be applied for the I/O compensation cell
  *   This parameter can be one of the following values:
  *   @arg LL_SYSCFG_CELL_CODE : Select Code from the cell
  *   @arg LL_SYSCFG_REGISTER_CODE: Select Code from the SYSCFG compensation cell code register
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetCellCompensationCode(uint32_t CompCode)
{
  SET_BIT(SYSCFG->CMPCR, CompCode);
}

/**
  * @brief  Get the code selected for the I/O Compensation cell
  * @rmtoll SYSCFG_CMPCR   SW_CTRL   LL_SYSCFG_GetCellCompensationCode
  * @retval Returned value can be one of the following values:
  *   @arg LL_SYSCFG_CELL_CODE : Selected Code is from the cell
  *   @arg LL_SYSCFG_REGISTER_CODE: Selected Code is from the SYSCFG compensation cell code register
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetCellCompensationCode(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_SW_CTRL));
}

/**
  * @brief  Get I/O compensation cell value for PMOS transistors
  * @rmtoll SYSCFG_CMPCR   APSRC   LL_SYSCFG_GetPMOSCompensationValue
  * @retval Returned value is the I/O compensation cell value for PMOS transistors
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetPMOSCompensationValue(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_APSRC));
}

/**
  * @brief  Get I/O compensation cell value for NMOS transistors
  * @rmtoll SYSCFG_CMPCR   ANSRC   LL_SYSCFG_GetNMOSCompensationValue
  * @retval Returned value is the I/O compensation cell value for NMOS transistors
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetNMOSCompensationValue(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_ANSRC));
}

/**
  * @brief  Set I/O compensation cell code for PMOS transistors
  * @rmtoll SYSCFG_CMPCR   RAPSRC   LL_SYSCFG_SetPMOSCompensationCode
  * @param  PMOSCode PMOS compensation code
  *         This code is applied to the I/O compensation cell when the CS bit of the
  *         SYSCFG_CMPCR is set
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetPMOSCompensationCode(uint32_t PMOSCode)
{
  MODIFY_REG(SYSCFG->CMPCR, SYSCFG_CMPCR_RAPSRC, PMOSCode);
}

/**
  * @brief  Get I/O compensation cell code for PMOS transistors
  * @rmtoll SYSCFG_CMPCR   RAPSRC   LL_SYSCFG_GetPMOSCompensationCode
  * @retval Returned value is the I/O compensation cell code for PMOS transistors
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetPMOSCompensationCode(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_RAPSRC));
}

/**
  * @brief  Set I/O compensation cell code for NMOS transistors
  * @rmtoll SYSCFG_CMPCR   RANSRC   LL_SYSCFG_SetNMOSCompensationCode
  * @param  NMOSCode: NMOS compensation code
  *         This code is applied to the I/O compensation cell when the CS bit of the
  *         SYSCFG_CMPCR is set
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetNMOSCompensationCode(uint32_t NMOSCode)
{
  MODIFY_REG(SYSCFG->CMPCR, SYSCFG_CMPCR_RANSRC, NMOSCode);
}

/**
  * @brief  Get I/O compensation cell code for NMOS transistors
  * @rmtoll SYSCFG_CMPCR   RANSRC   LL_SYSCFG_GetNMOSCompensationCode
  * @retval Returned value is the I/O compensation cell code for NMOS transistors
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetNMOSCompensationCode(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CMPCR, SYSCFG_CMPCR_RANSRC));
}

/**
  * @brief  Set connections to TIM1/8/15/16/17 Break inputs
  * @note this feature is available on STM32MP1 rev.B and above
  * @rmtoll SYSCFG_CBR   PVDL   LL_SYSCFG_SetTIMBreakInputs\n
  *         SYSCFG_CBR   CLL    LL_SYSCFG_SetTIMBreakInputs
  * @param  Break This parameter can be a combination of the following values:
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD
  *         @arg @ref LL_SYSCFG_TIMBREAK_CM4_LOCKUP (available for dual core lines only)
  * @retval None
  */
__STATIC_INLINE void LL_SYSCFG_SetTIMBreakInputs(uint32_t Break)
{
  MODIFY_REG(SYSCFG->CBR,  SYSCFG_CBR_PVDL | SYSCFG_CBR_CLL, Break);
}

/**
  * @brief  Get connections to TIM1/8/15/16/17 Break inputs
  * @note this feature is available on STM32MP1 rev.B and above
  * @rmtoll 
  *         SYSCFG_CBR   PVDL   LL_SYSCFG_GetTIMBreakInputs\n
  *         SYSCFG_CBR   CLL    LL_SYSCFG_GetTIMBreakInputs
  *         @arg @ref LL_SYSCFG_TIMBREAK_PVD
  *         @arg @ref LL_SYSCFG_TIMBREAK_CM4_LOCKUP (available for dual core lines only)
  */
__STATIC_INLINE uint32_t LL_SYSCFG_GetTIMBreakInputs(void)
{
  return (uint32_t)(READ_BIT(SYSCFG->CBR,  SYSCFG_CBR_PVDL | SYSCFG_CBR_CLL));
}


/**
  * @}
  */

/** @defgroup SYSTEM_LL_EF_DBGMCU DBGMCU
  * @{
  */

/**
  * @brief  Return the device identifier
  * @rmtoll DBGMCU_IDCODE DEV_ID        LL_DBGMCU_GetDeviceID
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFF
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetDeviceID(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_DEV_ID));
}

/**
  * @brief  Return the device revision identifier
  * @note This field indicates the revision of the device.
          For example, it is read as RevA -> 0x1000, Cat 2 revZ -> 0x1001
  * @rmtoll DBGMCU_IDCODE REV_ID        LL_DBGMCU_GetRevisionID
  * @retval Values between Min_Data=0x00 and Max_Data=0xFFFF
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetRevisionID(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->IDCODE, DBGMCU_IDCODE_REV_ID) >> DBGMCU_IDCODE_REV_ID_Pos);
}

/**
  * @brief  Enable D1 Domain debug during SLEEP mode
  * @rmtoll DBGMCU_CR   DBGSLEEP   LL_DBGMCU_EnableDebugInSleepMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_EnableDebugInSleepMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Disable D1 Domain debug during SLEEP mode
  * @rmtoll DBGMCU_CR   DBGSLEEP   LL_DBGMCU_DisableDebugInSleepMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableDebugInSleepMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_SLEEP);
}

/**
  * @brief  Enable D1 Domain debug during STOP mode
  * @rmtoll DBGMCU_CR   DBGSTOP   LL_DBGMCU_EnableDebugInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_EnableDebugInStopMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Disable D1 Domain debug during STOP mode
  * @rmtoll DBGMCU_CR   DBGSTOP   LL_DBGMCU_DisableDebugInStopMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableDebugInStopMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STOP);
}

/**
  * @brief  Enable D1 Domain debug during STANDBY mode
  * @rmtoll DBGMCU_CR   DBGSTBY   LL_DBGMCU_EnableDebugInStandbyMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_EnableDebugInStandbyMode(void)
{
  SET_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @brief  Disable D1 Domain debug during STANDBY mode
  * @rmtoll DBGMCU_CR   DBGSTBY   LL_DBGMCU_DisableDebugInStandbyMode
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_DisableDebugInStandbyMode(void)
{
  CLEAR_BIT(DBGMCU->CR, DBGMCU_CR_DBG_STANDBY);
}

/**
  * @brief  Set the direction of the bi-directional trigger pin TRGIO
  * @rmtoll DBGMCU_CR   TRGOEN   LL_DBGMCU_SetExternalTriggerPinDirection\n
  * @param  PinDirection This parameter can be one of the following values:
  *         @arg @ref LL_DBGMCU_TRGIO_INPUT_DIRECTION
  *         @arg @ref LL_DBGMCU_TRGIO_OUTPUT_DIRECTION
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_SetExternalTriggerPinDirection(uint32_t PinDirection)
{
  MODIFY_REG(DBGMCU->CR, DBGMCU_CR_DBG_TRGOEN, PinDirection);
}

/**
  * @brief  Get the direction of the bi-directional trigger pin TRGIO
  * @rmtoll DBGMCU_CR   TRGOEN   LL_DBGMCU_GetExternalTriggerPinDirection\n
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DBGMCU_TRGIO_INPUT_DIRECTION
  *         @arg @ref LL_DBGMCU_TRGIO_OUTPUT_DIRECTION
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetExternalTriggerPinDirection(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->CR, DBGMCU_CR_DBG_TRGOEN));
}

/**
  * @brief  Set the Watchdog Timer behaviour
  * @rmtoll DBGMCU_CR   WDFZCTL   LL_DBGMCU_SetWatchdogTimerBehaviour\n
  * @param  PinBehaviour This parameter can be one of the following values:
  *         @arg @ref LL_DBGMCU_IWDG_FREEZE_1CA7_HALT
  *         @arg @ref LL_DBGMCU_IWDG_FREEZE_2CA7_HALT
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_SetWatchdogTimerBehaviour(uint32_t PinBehaviour)
{
  MODIFY_REG(DBGMCU->CR, DBGMCU_CR_DBG_WDFZCTL, PinBehaviour);
}

/**
  * @brief  Get the Watchdog Timer behaviour
  * @rmtoll DBGMCU_CR   WDFZCTL   LL_DBGMCU_GetWatchdogTimerBehaviour\n
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_DBGMCU_IWDG_FREEZE_1CA7_HALT
  *         @arg @ref LL_DBGMCU_IWDG_FREEZE_2CA7_HALT
  */
__STATIC_INLINE uint32_t LL_DBGMCU_GetWatchdogTimerBehaviour(void)
{
  return (uint32_t)(READ_BIT(DBGMCU->CR, DBGMCU_CR_DBG_WDFZCTL));
}

/**
  * @brief  Freeze APB1 group1 peripherals
  * @rmtoll DBGMCU_APB1FZ2   TIM2      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM3      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM4      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM5      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM6      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM7      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM12     LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM13     LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM14     LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   LPTIM1    LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   WWDG1     LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C1      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C2      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C3      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C5      LL_DBGMCU_APB1_GRP1_FreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM12_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM13_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_LPTIM1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C5_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB1FZ2, Periphs);
}

/**
  * @brief  Unfreeze APB1 group1 peripherals
  * @rmtoll DBGMCU_APB1FZ2   TIM2      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM3      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM4      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM5      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM6      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM7      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM12     LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM13     LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   TIM14     LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   LPTIM1    LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   WWDG1     LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C1      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C2      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C3      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB1FZ2   I2C5      LL_DBGMCU_APB1_GRP1_UnFreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM4_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM5_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM6_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM7_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM12_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM13_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_TIM14_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_LPTIM1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_WWDG1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C1_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C2_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C3_STOP
  *         @arg @ref LL_DBGMCU_APB1_GRP1_I2C5_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB1_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB1FZ2, Periphs);
}

/**
  * @brief  Freeze APB2 group1 peripherals
  * @rmtoll DBGMCU_APB2FZ2   TIM1    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM8    LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM15   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM16   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM17   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  *         DBGMCU_APB2FZ2   FDCAN   LL_DBGMCU_APB2_GRP1_FreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM8_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM15_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM17_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_FDCAN_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB2FZ2, Periphs);
}

/**
  * @brief  Unfreeze APB2 group1 peripherals
  * @rmtoll DBGMCU_APB2FZ2   TIM1    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM8    LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM15   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM16   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2FZ2   TIM17   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB2FZ2   FDCAN   LL_DBGMCU_APB2_GRP1_UnFreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM1_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM8_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM15_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM16_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_TIM17_STOP
  *         @arg @ref LL_DBGMCU_APB2_GRP1_FDCAN_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB2_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB2FZ2, Periphs);
}

/**
  * @brief  Freeze APB3 peripherals
  * @rmtoll DBGMCU_APB3FZ2    LPTIM2    LL_DBGMCU_APB3_GRP1_FreezePeriph\n
  *         DBGMCU_APB3FZ2    LPTIM3    LL_DBGMCU_APB3_GRP1_FreezePeriph\n
  *         DBGMCU_APB3FZ2    LPTIM4    LL_DBGMCU_APB3_GRP1_FreezePeriph\n
  *         DBGMCU_APB3FZ2    LPTIM5    LL_DBGMCU_APB3_GRP1_FreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM2_STOP
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM3_STOP
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM4_STOP
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM5_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB3_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB3FZ2, Periphs);
}

/**
  * @brief  Unfreeze APB3 peripherals
  * @rmtoll DBGMCU_APB3FZ2    LPTIM2    LL_DBGMCU_APB3_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB3FZ2    LPTIM3    LL_DBGMCU_APB3_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB3FZ2    LPTIM4    LL_DBGMCU_APB3_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB3FZ2    LPTIM5    LL_DBGMCU_APB3_GRP1_UnFreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM2_STOP
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM3_STOP
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM4_STOP
  *         @arg @ref LL_DBGMCU_APB3_GRP1_LPTIM5_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB3_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB3FZ2, Periphs);
}

/**
  * @brief  Freeze APB5 peripherals
  * @rmtoll DBGMCU_APB5FZ2    I2C4      LL_DBGMCU_APB5_GRP1_FreezePeriph\n
  *         DBGMCU_APB5FZ2    RTC       LL_DBGMCU_APB5_GRP1_FreezePeriph\n
  *         DBGMCU_APB5FZ2    I2C6      LL_DBGMCU_APB5_GRP1_FreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB5_GRP1_I2C4_STOP
  *         @arg @ref LL_DBGMCU_APB5_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB5_GRP1_I2C6_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB5_GRP1_FreezePeriph(uint32_t Periphs)
{
  SET_BIT(DBGMCU->APB5FZ2, Periphs);
}

/**
  * @brief  Unfreeze APB5 peripherals
  * @rmtoll DBGMCU_APB5FZ2    I2C4      LL_DBGMCU_APB5_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB5FZ2    RTC       LL_DBGMCU_APB5_GRP1_UnFreezePeriph\n
  *         DBGMCU_APB5FZ2    I2C6      LL_DBGMCU_APB5_GRP1_UnFreezePeriph\n
  * @param  Periphs This parameter can be a combination of the following values:
  *         @arg @ref LL_DBGMCU_APB5_GRP1_I2C4_STOP
  *         @arg @ref LL_DBGMCU_APB5_GRP1_RTC_STOP
  *         @arg @ref LL_DBGMCU_APB5_GRP1_I2C6_STOP
  * @retval None
  */
__STATIC_INLINE void LL_DBGMCU_APB5_GRP1_UnFreezePeriph(uint32_t Periphs)
{
  CLEAR_BIT(DBGMCU->APB5FZ2, Periphs);
}
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif /* defined (SYSCFG) || defined (DBGMCU) */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32MP1xx_LL_SYSTEM_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
