/**
  ******************************************************************************
  * @file    stm32g0xx_hal.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the HAL
  *          module driver.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32G0xx_HAL_H
#define STM32G0xx_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal_conf.h"

/** @addtogroup STM32G0xx_HAL_Driver
  * @{
  */

/** @defgroup HAL HAL
  * @{
  */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** @defgroup HAL_Exported_Constants HAL Exported Constants
  * @{
  */

/** @defgroup HAL_TICK_FREQ Tick Frequency
  * @{
  */
#define  HAL_TICK_FREQ_10HZ         100U
#define  HAL_TICK_FREQ_100HZ        10U
#define  HAL_TICK_FREQ_1KHZ         1U
#define  HAL_TICK_FREQ_DEFAULT      HAL_TICK_FREQ_1KHZ

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup SYSCFG_Exported_Constants SYSCFG Exported Constants
  * @{
  */

/** @defgroup SYSCFG_BootMode Boot Mode
  * @{
  */
#define SYSCFG_BOOT_MAINFLASH          0x00000000U                      /*!< Main Flash memory mapped at 0x0000 0000   */
#define SYSCFG_BOOT_SYSTEMFLASH        SYSCFG_CFGR1_MEM_MODE_0          /*!< System Flash memory mapped at 0x0000 0000 */
#define SYSCFG_BOOT_SRAM               (SYSCFG_CFGR1_MEM_MODE_1 | SYSCFG_CFGR1_MEM_MODE_0)  /*!< Embedded SRAM mapped at 0x0000 0000 */

/**
  * @}
  */

/** @defgroup SYSCFG_Break Break
  * @{
  */
#define SYSCFG_BREAK_SP                SYSCFG_CFGR2_SPL    /*!< Enables and locks the SRAM Parity error signal with Break Input of TIM1/15/16/17 */
#if defined(SYSCFG_CFGR2_PVDL)
#define SYSCFG_BREAK_PVD               SYSCFG_CFGR2_PVDL   /*!< Enables and locks the PVD connection with TIM1/15/16/17 Break Input and also the PVDE and PLS bits of the Power Control Interface */
#endif
#define SYSCFG_BREAK_LOCKUP            SYSCFG_CFGR2_CLL    /*!< Enables and locks the LOCKUP output of CortexM0+ with Break Input of TIM1/15/16/17 */
#define SYSCFG_BREAK_ECC               SYSCFG_CFGR2_ECCL   /*!< Enables and locks the ECC of CortexM0+ with Break Input of TIM1/15/16/17 */
/**
  * @}
  */


/** @defgroup HAL_Pin_remapping Pin remapping
  * @{
  */
/* Only available on cut2.0 */
#define SYSCFG_REMAP_PA11                   SYSCFG_CFGR1_PA11_RMP       /*!< PA11 pad behaves digitally as PA9 GPIO pin */
#define SYSCFG_REMAP_PA12                   SYSCFG_CFGR1_PA12_RMP       /*!< PA12 pad behaves digitally as PA10 GPIO pin */
/**
  * @}
  */

/** @defgroup HAL_IR_ENV_SEL IR Modulation Envelope signal selection
  * @{
  */
#define HAL_SYSCFG_IRDA_ENV_SEL_TIM16     (SYSCFG_CFGR1_IR_MOD_0 & SYSCFG_CFGR1_IR_MOD_1)    /*!< 00: Timer16 is selected as IR Modulation envelope source */
#define HAL_SYSCFG_IRDA_ENV_SEL_USART1    (SYSCFG_CFGR1_IR_MOD_0)                            /*!< 01: USART1 is selected as IR Modulation envelope source */
#if defined (STM32G081xx) || defined (STM32G071xx) || defined (STM32G070xx)
#define HAL_SYSCFG_IRDA_ENV_SEL_USART4    (SYSCFG_CFGR1_IR_MOD_1)                            /*!< 10: USART4 is selected as IR Modulation envelope source */
#endif

/**
  * @}
  */

/** @defgroup HAL_IR_POL_SEL IR output polarity selection
  * @{
  */
#define HAL_SYSCFG_IRDA_POLARITY_NOT_INVERTED     0x00000000U                                /*!< 00: IR output polarity not inverted */
#define HAL_SYSCFG_IRDA_POLARITY_INVERTED         SYSCFG_CFGR1_IR_POL                        /*!< 01: IR output polarity inverted */

/**
  * @}
  */

#if defined(VREFBUF)
/** @defgroup SYSCFG_VREFBUF_VoltageScale VREFBUF Voltage Scale
  * @{
  */
#define SYSCFG_VREFBUF_VOLTAGE_SCALE0  0x00000000U            /*!< Voltage reference scale 0: VREF_OUT1 around 2.048 V.
                                                                   This requires VDDA equal to or higher than 2.4 V.   */
#define SYSCFG_VREFBUF_VOLTAGE_SCALE1  VREFBUF_CSR_VRS        /*!< Voltage reference scale 1: VREF_OUT1 around 2.5 V.
                                                                   This requires VDDA equal to or higher than 2.8 V.   */

/**
  * @}
  */

/** @defgroup SYSCFG_VREFBUF_HighImpedance VREFBUF High Impedance
  * @{
  */
#define SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE  0x00000000U        /*!< VREF_plus pin is internally connected to Voltage reference buffer output */
#define SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE   VREFBUF_CSR_HIZ    /*!< VREF_plus pin is high impedance */

/**
  * @}
  */
#endif /* VREFBUF */

/** @defgroup SYSCFG_FastModePlus_GPIO Fast mode Plus on GPIO
  * @{
  */

/** @brief  Fast mode Plus driving capability on a specific GPIO
  */
#define SYSCFG_FASTMODEPLUS_PB6        SYSCFG_CFGR1_I2C_PB6_FMP  /*!< Enable Fast mode Plus on PB6 */
#define SYSCFG_FASTMODEPLUS_PB7        SYSCFG_CFGR1_I2C_PB7_FMP  /*!< Enable Fast mode Plus on PB7 */
#define SYSCFG_FASTMODEPLUS_PB8        SYSCFG_CFGR1_I2C_PB8_FMP  /*!< Enable Fast mode Plus on PB8 */
#define SYSCFG_FASTMODEPLUS_PB9        SYSCFG_CFGR1_I2C_PB9_FMP  /*!< Enable Fast mode Plus on PB9 */
#define SYSCFG_FASTMODEPLUS_PA9        SYSCFG_CFGR1_I2C_PA9_FMP  /*!< Enable Fast mode Plus on PA9 */
#define SYSCFG_FASTMODEPLUS_PA10       SYSCFG_CFGR1_I2C_PA10_FMP /*!< Enable Fast mode Plus on PA10 */

/**
 * @}
 */

/** @defgroup SYSCFG_FastModePlus_I2Cx Fast mode Plus driving capability activation for I2Cx
  * @{
  */

/** @brief  Fast mode Plus driving capability on a specific GPIO
  */
#define SYSCFG_FASTMODEPLUS_I2C1       SYSCFG_CFGR1_I2C1_FMP /*!< Enable Fast mode Plus on I2C1 */
#define SYSCFG_FASTMODEPLUS_I2C2       SYSCFG_CFGR1_I2C2_FMP /*!< Enable Fast mode Plus on I2C2 */

/**
 * @}
 */
#if defined (STM32G081xx) || defined (STM32G071xx) || defined (STM32G070xx)
/** @defgroup SYSCFG_UCPDx_STROBE SYSCFG Dead Battery feature configuration
  * @{
  */
#define SYSCFG_UCPD1_STROBE          SYSCFG_CFGR1_UCPD1_STROBE /*!< UCPD1 Dead battery sw configuration */
#define SYSCFG_UCPD2_STROBE          SYSCFG_CFGR1_UCPD2_STROBE /*!< UCPD2 Dead battery sw configuration */
#endif
/**
  * @}
  */

/** @defgroup HAL_ISR_Wrapper HAL ISR Wrapper
  * @brief ISR Wrapper
  * @{
  */
#define HAL_SYSCFG_ITLINE0                           0x00000000U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE1                           0x00000001U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE2                           0x00000002U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE3                           0x00000003U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE4                           0x00000004U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE5                           0x00000005U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE6                           0x00000006U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE7                           0x00000007U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE8                           0x00000008U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE9                           0x00000009U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE10                          0x0000000AU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE11                          0x0000000BU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE12                          0x0000000CU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE13                          0x0000000DU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE14                          0x0000000EU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE15                          0x0000000FU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE16                          0x00000010U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE17                          0x00000011U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE18                          0x00000012U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE19                          0x00000013U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE20                          0x00000014U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE21                          0x00000015U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE22                          0x00000016U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE23                          0x00000017U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE24                          0x00000018U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE25                          0x00000019U /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE26                          0x0000001AU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE27                          0x0000001BU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE28                          0x0000001CU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE29                          0x0000001DU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE30                          0x0000001EU /*!< Internal define for macro handling */
#define HAL_SYSCFG_ITLINE31                          0x0000001FU /*!< Internal define for macro handling */

#define HAL_ITLINE_WWDG           ((HAL_SYSCFG_ITLINE0 << 0x18U) | SYSCFG_ITLINE0_SR_WWDG)          /*!< WWDG has expired .... */
#if defined (SYSCFG_ITLINE1_SR_PVDOUT)
#define HAL_ITLINE_PVDOUT         ((HAL_SYSCFG_ITLINE1 << 0x18U) | SYSCFG_ITLINE1_SR_PVDOUT)        /*!< Power voltage detection Interrupt .... */
#endif
#define HAL_ITLINE_RTC            ((HAL_SYSCFG_ITLINE2 << 0x18U) | SYSCFG_ITLINE2_SR_RTC)           /*!< RTC -> exti[19] Interrupt */
#define HAL_ITLINE_TAMPER         ((HAL_SYSCFG_ITLINE2 << 0x18U) | SYSCFG_ITLINE2_SR_TAMPER)        /*!< TAMPER -> exti[21] interrupt .... */
#define HAL_ITLINE_FLASH_ECC      ((HAL_SYSCFG_ITLINE3 << 0x18U) | SYSCFG_ITLINE3_SR_FLASH_ECC)     /*!< Flash ECC Interrupt */
#define HAL_ITLINE_FLASH_ITF      ((HAL_SYSCFG_ITLINE3 << 0x18U) | SYSCFG_ITLINE3_SR_FLASH_ITF)     /*!< Flash ITF Interrupt */
#define HAL_ITLINE_CLK_CTRL       ((HAL_SYSCFG_ITLINE4 << 0x18U) | SYSCFG_ITLINE4_SR_CLK_CTRL)      /*!< CLK Control Interrupt */
#define HAL_ITLINE_EXTI0          ((HAL_SYSCFG_ITLINE5 << 0x18U) | SYSCFG_ITLINE5_SR_EXTI0)         /*!< External Interrupt 0 */
#define HAL_ITLINE_EXTI1          ((HAL_SYSCFG_ITLINE5 << 0x18U) | SYSCFG_ITLINE5_SR_EXTI1)         /*!< External Interrupt 1 */
#define HAL_ITLINE_EXTI2          ((HAL_SYSCFG_ITLINE6 << 0x18U) | SYSCFG_ITLINE6_SR_EXTI2)         /*!< External Interrupt 2 */
#define HAL_ITLINE_EXTI3          ((HAL_SYSCFG_ITLINE6 << 0x18U) | SYSCFG_ITLINE6_SR_EXTI3)         /*!< External Interrupt 3 */
#define HAL_ITLINE_EXTI4          ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI4)         /*!< EXTI4 Interrupt */
#define HAL_ITLINE_EXTI5          ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI5)         /*!< EXTI5 Interrupt */
#define HAL_ITLINE_EXTI6          ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI6)         /*!< EXTI6 Interrupt */
#define HAL_ITLINE_EXTI7          ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI7)         /*!< EXTI7 Interrupt */
#define HAL_ITLINE_EXTI8          ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI8)         /*!< EXTI8 Interrupt */
#define HAL_ITLINE_EXTI9          ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI9)         /*!< EXTI9 Interrupt */
#define HAL_ITLINE_EXTI10         ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI10)        /*!< EXTI10 Interrupt */
#define HAL_ITLINE_EXTI11         ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI11)        /*!< EXTI11 Interrupt */
#define HAL_ITLINE_EXTI12         ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI12)        /*!< EXTI12 Interrupt */
#define HAL_ITLINE_EXTI13         ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI13)        /*!< EXTI13 Interrupt */
#define HAL_ITLINE_EXTI14         ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI14)        /*!< EXTI14 Interrupt */
#define HAL_ITLINE_EXTI15         ((HAL_SYSCFG_ITLINE7 << 0x18U) | SYSCFG_ITLINE7_SR_EXTI15)        /*!< EXTI15 Interrupt */
#if defined (UCPD1)
#define HAL_ITLINE_UCPD1          ((HAL_SYSCFG_ITLINE8 << 0x18U) | SYSCFG_ITLINE8_SR_UCPD1)         /*!< UCPD1 Interrupt */
#endif
#if defined (UCPD2)
#define HAL_ITLINE_UCPD2          ((HAL_SYSCFG_ITLINE8 << 0x18U) | SYSCFG_ITLINE8_SR_UCPD2)         /*!< UCPD2 Interrupt */
#endif
#define HAL_ITLINE_DMA1_CH1       ((HAL_SYSCFG_ITLINE9 << 0x18U) | SYSCFG_ITLINE9_SR_DMA1_CH1)      /*!< DMA1 Channel 1 Interrupt */
#define HAL_ITLINE_DMA1_CH2       ((HAL_SYSCFG_ITLINE10 << 0x18U) | SYSCFG_ITLINE10_SR_DMA1_CH2)    /*!< DMA1 Channel 2 Interrupt */
#define HAL_ITLINE_DMA1_CH3       ((HAL_SYSCFG_ITLINE10 << 0x18U) | SYSCFG_ITLINE10_SR_DMA1_CH3)    /*!< DMA1 Channel 3 Interrupt */
#define HAL_ITLINE_DMAMUX         ((HAL_SYSCFG_ITLINE11 << 0x18U) | SYSCFG_ITLINE11_SR_DMAMUX)      /*!< DMAMUX Interrupt */
#define HAL_ITLINE_DMA1_CH4       ((HAL_SYSCFG_ITLINE11 << 0x18U) | SYSCFG_ITLINE11_SR_DMA1_CH4)    /*!< DMA1 Channel 4 Interrupt */
#define HAL_ITLINE_DMA1_CH5       ((HAL_SYSCFG_ITLINE11 << 0x18U) | SYSCFG_ITLINE11_SR_DMA1_CH5)    /*!< DMA1 Channel 5 Interrupt */
#if defined (STM32G081xx) || defined (STM32G071xx) || defined (STM32G070xx)
#define HAL_ITLINE_DMA1_CH6       ((HAL_SYSCFG_ITLINE11 << 0x18U) | SYSCFG_ITLINE11_SR_DMA1_CH6)    /*!< DMA1 Channel 6 Interrupt */
#define HAL_ITLINE_DMA1_CH7       ((HAL_SYSCFG_ITLINE11 << 0x18U) | SYSCFG_ITLINE11_SR_DMA1_CH7)    /*!< DMA1 Channel 7 Interrupt */
#endif
#define HAL_ITLINE_ADC            ((HAL_SYSCFG_ITLINE12 << 0x18U) | SYSCFG_ITLINE12_SR_ADC)         /*!< ADC Interrupt */
#if defined (COMP1)
#define HAL_ITLINE_COMP1          ((HAL_SYSCFG_ITLINE12 << 0x18U) | SYSCFG_ITLINE12_SR_COMP1)       /*!< COMP1 Interrupt -> exti[17] */
#endif
#if defined (COMP2)
#define HAL_ITLINE_COMP2          ((HAL_SYSCFG_ITLINE12 << 0x18U) | SYSCFG_ITLINE12_SR_COMP2)       /*!< COMP2 Interrupt -> exti[18] */
#endif
#define HAL_ITLINE_TIM1_BRK       ((HAL_SYSCFG_ITLINE13 << 0x18U) | SYSCFG_ITLINE13_SR_TIM1_BRK)    /*!< TIM1 BRK Interrupt */
#define HAL_ITLINE_TIM1_UPD       ((HAL_SYSCFG_ITLINE13 << 0x18U) | SYSCFG_ITLINE13_SR_TIM1_UPD)    /*!< TIM1 UPD Interrupt */
#define HAL_ITLINE_TIM1_TRG       ((HAL_SYSCFG_ITLINE13 << 0x18U) | SYSCFG_ITLINE13_SR_TIM1_TRG)    /*!< TIM1 TRG Interrupt */
#define HAL_ITLINE_TIM1_CCU       ((HAL_SYSCFG_ITLINE13 << 0x18U) | SYSCFG_ITLINE13_SR_TIM1_CCU)    /*!< TIM1 CCU Interrupt */
#define HAL_ITLINE_TIM1_CC        ((HAL_SYSCFG_ITLINE14 << 0x18U) | SYSCFG_ITLINE14_SR_TIM1_CC)     /*!< TIM1 CC Interrupt */
#if defined (TIM2)
#define HAL_ITLINE_TIM2           ((HAL_SYSCFG_ITLINE15 << 0x18U) | SYSCFG_ITLINE15_SR_TIM2_GLB)    /*!< TIM2 Interrupt */
#endif
#define HAL_ITLINE_TIM3           ((HAL_SYSCFG_ITLINE16 << 0x18U) | SYSCFG_ITLINE16_SR_TIM3_GLB)    /*!< TIM3 Interrupt */
#if defined(TIM6)
#define HAL_ITLINE_TIM6           ((HAL_SYSCFG_ITLINE17 << 0x18U) | SYSCFG_ITLINE17_SR_TIM6_GLB)    /*!< TIM6 Interrupt */
#endif
#if defined(DAC1)
#define HAL_ITLINE_DAC            ((HAL_SYSCFG_ITLINE17 << 0x18U) | SYSCFG_ITLINE17_SR_DAC_GLB)     /*!< DAC Interrupt */
#endif
#if defined(LPTIM1)
#define HAL_ITLINE_LPTIM1         ((HAL_SYSCFG_ITLINE17 << 0x18U) | SYSCFG_ITLINE17_SR_LPTIM1_GLB)  /*!< LPTIM1 Interrupt -> exti[29] */
#endif
#if defined(TIM6)
#define HAL_ITLINE_TIM7           ((HAL_SYSCFG_ITLINE18 << 0x18U) | SYSCFG_ITLINE18_SR_TIM7_GLB)    /*!< TIM7 Interrupt */
#endif
#if defined(LPTIM2)
#define HAL_ITLINE_LPTIM2         ((HAL_SYSCFG_ITLINE18 << 0x18U) | SYSCFG_ITLINE18_SR_LPTIM2_GLB)  /*!< LPTIM2 Interrupt -> exti[30] */
#endif
#define HAL_ITLINE_TIM14          ((HAL_SYSCFG_ITLINE19 << 0x18U) | SYSCFG_ITLINE19_SR_TIM14_GLB)   /*!< TIM14 Interrupt */
#if defined(TIM15)
#define HAL_ITLINE_TIM15          ((HAL_SYSCFG_ITLINE20 << 0x18U) | SYSCFG_ITLINE20_SR_TIM15_GLB)   /*!< TIM15 Interrupt */
#endif
#define HAL_ITLINE_TIM16          ((HAL_SYSCFG_ITLINE21 << 0x18U) | SYSCFG_ITLINE21_SR_TIM16_GLB)   /*!< TIM16 Interrupt */
#define HAL_ITLINE_TIM17          ((HAL_SYSCFG_ITLINE22 << 0x18U) | SYSCFG_ITLINE22_SR_TIM17_GLB)   /*!< TIM17 Interrupt */
#define HAL_ITLINE_I2C1           ((HAL_SYSCFG_ITLINE23 << 0x18U) | SYSCFG_ITLINE23_SR_I2C1_GLB)    /*!< I2C1 Interrupt -> exti[23] */
#define HAL_ITLINE_I2C2           ((HAL_SYSCFG_ITLINE24 << 0x18U) | SYSCFG_ITLINE24_SR_I2C2_GLB)    /*!< I2C2 Interrupt */
#define HAL_ITLINE_SPI1           ((HAL_SYSCFG_ITLINE25 << 0x18U) | SYSCFG_ITLINE25_SR_SPI1)        /*!< SPI1 Interrupt  */
#define HAL_ITLINE_SPI2           ((HAL_SYSCFG_ITLINE26 << 0x18U) | SYSCFG_ITLINE26_SR_SPI2)        /*!< SPI2 Interrupt */
#define HAL_ITLINE_USART1         ((HAL_SYSCFG_ITLINE27 << 0x18U) | SYSCFG_ITLINE27_SR_USART1_GLB)  /*!< USART1 GLB Interrupt -> exti[25] */
#define HAL_ITLINE_USART2         ((HAL_SYSCFG_ITLINE28 << 0x18U) | SYSCFG_ITLINE28_SR_USART2_GLB)  /*!< USART2 GLB Interrupt -> exti[26] */
#if defined(USART3)
#define HAL_ITLINE_USART3         ((HAL_SYSCFG_ITLINE29 << 0x18U) | SYSCFG_ITLINE29_SR_USART3_GLB)  /*!< USART3 Interrupt .... */
#endif
#if defined(USART4)
#define HAL_ITLINE_USART4         ((HAL_SYSCFG_ITLINE29 << 0x18U) | SYSCFG_ITLINE29_SR_USART4_GLB)  /*!< USART4 Interrupt .... */
#endif
#if defined (LPUART1)
#define HAL_ITLINE_LPUART1        ((HAL_SYSCFG_ITLINE29 << 0x18U) | SYSCFG_ITLINE29_SR_LPUART1_GLB) /*!< LPUART1 Interrupt -> exti[28]*/
#endif
#if defined (CEC)
#define HAL_ITLINE_CEC            ((HAL_SYSCFG_ITLINE30 << 0x18U) | SYSCFG_ITLINE30_SR_CEC)         /*!< CEC Interrupt -> exti[27] */
#endif
#if defined (RNG)
#define HAL_ITLINE_RNG            ((HAL_SYSCFG_ITLINE31 << 0x18U) | SYSCFG_ITLINE31_SR_RNG)         /*!< RNG Interrupt */
#endif
#if defined (AES)
#define HAL_ITLINE_AES            ((HAL_SYSCFG_ITLINE31 << 0x18U) | SYSCFG_ITLINE31_SR_AES)         /*!< AES Interrupt */
#endif
/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/
/** @defgroup HAL_Exported_Macros HAL Exported Macros
  * @{
  */

/** @defgroup DBG_Exported_Macros DBG Exported Macros
  * @{
  */

/** @brief  Freeze and Unfreeze Peripherals in Debug mode
  */
#if defined(DBG_APB_FZ1_DBG_TIM2_STOP)
#define __HAL_DBGMCU_FREEZE_TIM2()           SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM2_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM2()         CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM2_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_TIM3_STOP)
#define __HAL_DBGMCU_FREEZE_TIM3()           SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM3_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM3()         CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM3_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_TIM6_STOP)
#define __HAL_DBGMCU_FREEZE_TIM6()           SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM6_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM6()         CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM6_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_TIM7_STOP)
#define __HAL_DBGMCU_FREEZE_TIM7()           SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM7_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM7()         CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_TIM7_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_RTC_STOP)
#define __HAL_DBGMCU_FREEZE_RTC()            SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_RTC_STOP)
#define __HAL_DBGMCU_UNFREEZE_RTC()          CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_RTC_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_WWDG_STOP)
#define __HAL_DBGMCU_FREEZE_WWDG()           SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_WWDG_STOP)
#define __HAL_DBGMCU_UNFREEZE_WWDG()         CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_WWDG_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_IWDG_STOP)
#define __HAL_DBGMCU_FREEZE_IWDG()           SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_IWDG_STOP)
#define __HAL_DBGMCU_UNFREEZE_IWDG()         CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_IWDG_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP)
#define __HAL_DBGMCU_FREEZE_I2C1_TIMEOUT()   SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP)
#define __HAL_DBGMCU_UNFREEZE_I2C1_TIMEOUT() CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_LPTIM1_STOP)
#define __HAL_DBGMCU_FREEZE_LPTIM1()         SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_LPTIM1_STOP)
#define __HAL_DBGMCU_UNFREEZE_LPTIM1()       CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_LPTIM1_STOP)
#endif

#if defined(DBG_APB_FZ1_DBG_LPTIM2_STOP)
#define __HAL_DBGMCU_FREEZE_LPTIM2()         SET_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_LPTIM2_STOP)
#define __HAL_DBGMCU_UNFREEZE_LPTIM2()       CLEAR_BIT(DBG->APBFZ1, DBG_APB_FZ1_DBG_LPTIM2_STOP)
#endif

#if defined(DBG_APB_FZ2_DBG_TIM1_STOP)
#define __HAL_DBGMCU_FREEZE_TIM1()           SET_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM1_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM1()         CLEAR_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM1_STOP)
#endif

#if defined(DBG_APB_FZ2_DBG_TIM14_STOP)
#define __HAL_DBGMCU_FREEZE_TIM14()          SET_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM14_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM14()        CLEAR_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM14_STOP)
#endif

#if defined(DBG_APB_FZ2_DBG_TIM15_STOP)
#define __HAL_DBGMCU_FREEZE_TIM15()          SET_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM15_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM15()        CLEAR_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM15_STOP)
#endif

#if defined(DBG_APB_FZ2_DBG_TIM16_STOP)
#define __HAL_DBGMCU_FREEZE_TIM16()          SET_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM16_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM16()        CLEAR_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM16_STOP)
#endif

#if defined(DBG_APB_FZ2_DBG_TIM17_STOP)
#define __HAL_DBGMCU_FREEZE_TIM17()          SET_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM17_STOP)
#define __HAL_DBGMCU_UNFREEZE_TIM17()        CLEAR_BIT(DBG->APBFZ2, DBG_APB_FZ2_DBG_TIM17_STOP)
#endif

/**
  * @}
  */

/** @defgroup SYSCFG_Exported_Macros SYSCFG Exported Macros
  * @{
  */

/** @brief  Main Flash memory mapped at 0x00000000
  */
#define __HAL_SYSCFG_REMAPMEMORY_FLASH()       CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE)

/** @brief  System Flash memory mapped at 0x00000000
  */
#define __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH() MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE, SYSCFG_CFGR1_MEM_MODE_0)

/** @brief  Embedded SRAM mapped at 0x00000000
  */
#define __HAL_SYSCFG_REMAPMEMORY_SRAM()        MODIFY_REG(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE, (SYSCFG_CFGR1_MEM_MODE_1|SYSCFG_CFGR1_MEM_MODE_0))

/**
  * @brief  Return the boot mode as configured by user.
  * @retval The boot mode as configured by user. The returned value can be one
  *         of the following values @ref SYSCFG_BootMode
  */
#define __HAL_SYSCFG_GET_BOOT_MODE()           READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_MEM_MODE)

/** @brief  SYSCFG Break ECC lock.
  *         Enable and lock the connection of Flash ECC error connection to TIM1 Break input.
  * @note   The selected configuration is locked and can be unlocked only by system reset.
  */
#define __HAL_SYSCFG_BREAK_ECC_LOCK()           SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_ECCL)


/** @brief  SYSCFG Break Cortex-M0+ Lockup lock.
  *         Enables and locks the connection of Cortex-M0+ LOCKUP (Hardfault) output to TIM1/15/16/17 Break input
  * @note   The selected configuration is locked and can be unlocked only by system reset.
  */
#define __HAL_SYSCFG_BREAK_LOCKUP_LOCK()        SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_CLL)

#if defined(SYSCFG_CFGR2_PVDL)
/** @brief  SYSCFG Break PVD lock.
  *         Enables and locks the PVD connection with Timer1/15/16/17 Break input, as well as the PVDE and PLS[2:0] in the PWR_CR register
  * @note   The selected configuration is locked and can be unlocked only by system reset
  */
#define __HAL_SYSCFG_BREAK_PVD_LOCK()           SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_PVDL)
#endif

/** @brief  SYSCFG Break SRAM PARITY lock
  *         Enables and locks the SRAM_PARITY error signal with Break Input of TIMER1/15/16/17
  * @note   The selected configuration is locked and can only be unlocked by system reset
  */
#define __HAL_SYSCFG_BREAK_SRAMPARITY_LOCK()    SET_BIT(SYSCFG->CFGR2,SYSCFG_CFGR2_SPL)

/** @brief  Parity check on RAM disable macro
  * @note   Disabling the parity check on RAM locks the configuration bit.
  *         To re-enable the parity check on RAM perform a system reset.
  */
#define __HAL_SYSCFG_RAM_PARITYCHECK_DISABLE()  (SYSCFG->CFGR2 |= SYSCFG_CFGR2_SPF)

/** @brief  Set the PEF bit to clear the SRAM Parity Error Flag.
  */
#define __HAL_SYSCFG_CLEAR_FLAG()               SET_BIT(SYSCFG->CFGR2, SYSCFG_CFGR2_SPF)

/** @brief  Fast-mode Plus driving capability enable/disable macros
  * @param __FASTMODEPLUS__ This parameter can be a value of @ref SYSCFG_FastModePlus_GPIO
  */
#define __HAL_SYSCFG_FASTMODEPLUS_ENABLE(__FASTMODEPLUS__)  do {assert_param(IS_SYSCFG_FASTMODEPLUS((__FASTMODEPLUS__)));\
                                                                SET_BIT(SYSCFG->CFGR1, (__FASTMODEPLUS__));\
                                                               }while(0U)

#define __HAL_SYSCFG_FASTMODEPLUS_DISABLE(__FASTMODEPLUS__) do {assert_param(IS_SYSCFG_FASTMODEPLUS((__FASTMODEPLUS__)));\
                                                                CLEAR_BIT(SYSCFG->CFGR1, (__FASTMODEPLUS__));\
                                                               }while(0U)


/** @brief  ISR wrapper check
  * @note Allow to determine interrupt source per line.
  */
#define __HAL_SYSCFG_GET_PENDING_IT(__SOURCE__)       (SYSCFG->IT_LINE_SR[((__SOURCE__) >> 0x18U)] & ((__SOURCE__) & 0x00FFFFFFU))

/** @brief  selection of the modulation envelope signal macro, using bits [7:6] of SYSCFG_CFGR1 register
  * @param __SOURCE__ This parameter can be a value of @ref HAL_IR_ENV_SEL
  */
#define __HAL_SYSCFG_IRDA_ENV_SELECTION(__SOURCE__)  do {assert_param(IS_HAL_SYSCFG_IRDA_ENV_SEL((__SOURCE__)));\
                                                         CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_IR_MOD);\
                                                         SET_BIT(SYSCFG->CFGR1, (__SOURCE__));\
                                                        }while(0U)

#define __HAL_SYSCFG_GET_IRDA_ENV_SELECTION()  ((SYSCFG->CFGR1) & 0x000000C0U)

/** @brief  IROut Polarity Selection, using bit[5] of SYSCFG_CFGR1 register
  * @param __SEL__ This parameter can be a value of @ref HAL_IR_POL_SEL
  */
#define __HAL_SYSCFG_IRDA_OUT_POLARITY_SELECTION(__SEL__)  do { assert_param(IS_HAL_SYSCFG_IRDA_POL_SEL((__SEL__)));\
                                                                CLEAR_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_IR_POL);\
                                                                SET_BIT(SYSCFG->CFGR1,(__SEL__));\
                                                              }while(0U)

/**
  * @brief  Return the IROut Polarity mode as configured by user.
  * @retval The IROut polarity as configured by user. The returned value can be one
  *         of @ref HAL_IR_POL_SEL
  */
#define __HAL_SYSCFG_GET_POLARITY()           READ_BIT(SYSCFG->CFGR1, SYSCFG_CFGR1_IR_POL)

/** @brief  Break input to TIM1/15/16/17 capability enable/disable macros
  * @param __BREAK__ This parameter can be a value of @ref SYSCFG_Break
  */
#define __HAL_SYSCFG_BREAK_ENABLE(__BREAK__)     do {assert_param(IS_SYSCFG_BREAK_CONFIG((__BREAK__)));\
                                                     SET_BIT(SYSCFG->CFGR2, (__BREAK__));\
                                                    }while(0U)

#define __HAL_SYSCFG_BREAK_DISABLE(__BREAK__)    do {assert_param(IS_SYSCFG_BREAK_CONFIG((__BREAK__)));\
                                                     CLEAR_BIT(SYSCFG->CFGR2, (__BREAK__));\
                                                    }while(0U)


/**
  * @}
  */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup SYSCFG_Private_Macros SYSCFG Private Macros
  * @{
  */
#if defined (PWR_PVD_SUPPORT)
#define IS_SYSCFG_BREAK_CONFIG(__CONFIG__) (((__CONFIG__) == SYSCFG_BREAK_SP)        || \
                                            ((__CONFIG__) == SYSCFG_BREAK_PVD)       || \
                                            ((__CONFIG__) == SYSCFG_BREAK_ECC)       || \
                                            ((__CONFIG__) == SYSCFG_BREAK_LOCKUP))
#else
#define IS_SYSCFG_BREAK_CONFIG(__CONFIG__) (((__CONFIG__) == SYSCFG_BREAK_SP)        || \
                                            ((__CONFIG__) == SYSCFG_BREAK_ECC)       || \
                                            ((__CONFIG__) == SYSCFG_BREAK_LOCKUP))
#endif


#if defined (STM32G081xx) || defined (STM32G071xx) || defined (STM32G070xx)
#define IS_HAL_SYSCFG_IRDA_ENV_SEL(SEL)   (((SEL) == HAL_SYSCFG_IRDA_ENV_SEL_TIM16)   || \
                                           ((SEL) == HAL_SYSCFG_IRDA_ENV_SEL_USART1)  || \
                                           ((SEL) == HAL_SYSCFG_IRDA_ENV_SEL_USART4))
#endif
#define IS_HAL_SYSCFG_IRDA_POL_SEL(SEL)   (((SEL) == HAL_SYSCFG_IRDA_POLARITY_NOT_INVERTED)   || \
                                           ((SEL) == HAL_SYSCFG_IRDA_POLARITY_INVERTED))

#if defined (SYSCFG_CFGR1_UCPD1_STROBE) || defined (SYSCFG_CFGR1_UCPD2_STROBE)
#define IS_SYSCFG_DBATT_CONFIG(__CONFIG__) (((__CONFIG__) == SYSCFG_UCPD1_STROBE) || \
                                            ((__CONFIG__) == SYSCFG_UCPD2_STROBE) || \
                                            ((__CONFIG__) == (SYSCFG_UCPD1_STROBE | SYSCFG_UCPD2_STROBE)))
#endif
#if defined(VREFBUF)
#define IS_SYSCFG_VREFBUF_VOLTAGE_SCALE(__SCALE__)  (((__SCALE__) == SYSCFG_VREFBUF_VOLTAGE_SCALE0) || \
                                                     ((__SCALE__) == SYSCFG_VREFBUF_VOLTAGE_SCALE1))

#define IS_SYSCFG_VREFBUF_HIGH_IMPEDANCE(__VALUE__)  (((__VALUE__) == SYSCFG_VREFBUF_HIGH_IMPEDANCE_DISABLE) || \
                                                      ((__VALUE__) == SYSCFG_VREFBUF_HIGH_IMPEDANCE_ENABLE))

#define IS_SYSCFG_VREFBUF_TRIMMING(__VALUE__)  (((__VALUE__) > 0U) && ((__VALUE__) <= VREFBUF_CCR_TRIM))
#endif /* VREFBUF */

#define IS_SYSCFG_FASTMODEPLUS(__PIN__) ((((__PIN__) & SYSCFG_FASTMODEPLUS_PA9)  == SYSCFG_FASTMODEPLUS_PA9)  || \
                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PA10) == SYSCFG_FASTMODEPLUS_PA10) || \
                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB6)  == SYSCFG_FASTMODEPLUS_PB6)  || \
                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB7)  == SYSCFG_FASTMODEPLUS_PB7)  || \
                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB8)  == SYSCFG_FASTMODEPLUS_PB8)  || \
                                         (((__PIN__) & SYSCFG_FASTMODEPLUS_PB9)  == SYSCFG_FASTMODEPLUS_PB9))

#define IS_HAL_REMAP_PIN(RMP)               (((RMP) == SYSCFG_REMAP_PA11) || \
                                             ((RMP) == SYSCFG_REMAP_PA12) || \
                                             ((RMP) == (SYSCFG_REMAP_PA11 | SYSCFG_REMAP_PA12)))
/**
  * @}
  */

/** @defgroup HAL_Private_Macros HAL Private Macros
  * @{
  */
#define IS_TICKFREQ(FREQ) (((FREQ) == HAL_TICK_FREQ_10HZ)  || \
                           ((FREQ) == HAL_TICK_FREQ_100HZ) || \
                           ((FREQ) == HAL_TICK_FREQ_1KHZ))
/**
  * @}
  */
/* Exported functions --------------------------------------------------------*/

/** @defgroup HAL_Exported_Functions HAL Exported Functions
  * @{
  */

/** @defgroup HAL_Exported_Functions_Group1 HAL Initialization and Configuration functions
  * @{
  */

/* Initialization and Configuration functions  ******************************/
HAL_StatusTypeDef HAL_Init(void);
HAL_StatusTypeDef HAL_DeInit(void);
void HAL_MspInit(void);
void HAL_MspDeInit(void);
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority);

/**
  * @}
  */

/** @defgroup HAL_Exported_Functions_Group2 HAL Control functions
  * @{
  */

/* Peripheral Control functions  ************************************************/
void HAL_IncTick(void);
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
uint32_t HAL_GetTickPrio(void);
HAL_StatusTypeDef HAL_SetTickFreq(uint32_t Freq);
uint32_t HAL_GetTickFreq(void);
void HAL_SuspendTick(void);
void HAL_ResumeTick(void);
uint32_t HAL_GetHalVersion(void);
uint32_t HAL_GetREVID(void);
uint32_t HAL_GetDEVID(void);
uint32_t HAL_GetUIDw0(void);
uint32_t HAL_GetUIDw1(void);
uint32_t HAL_GetUIDw2(void);

/**
  * @}
  */

/** @defgroup HAL_Exported_Functions_Group3 DBGMCU Control functions
  * @{
  */

/* DBGMCU Peripheral Control functions  *****************************************/
void HAL_DBGMCU_EnableDBGStopMode(void);
void HAL_DBGMCU_DisableDBGStopMode(void);
void HAL_DBGMCU_EnableDBGStandbyMode(void);
void HAL_DBGMCU_DisableDBGStandbyMode(void);

/**
  * @}
  */

/* Exported variables ---------------------------------------------------------*/
/** @addtogroup HAL_Exported_Variables
  * @{
  */
extern __IO uint32_t uwTick;
extern uint32_t uwTickPrio;
extern uint32_t uwTickFreq;
/**
  * @}
  */

/** @defgroup HAL_Exported_Functions_Group4 SYSCFG configuration functions
  * @{
  */

/* SYSCFG Control functions  ****************************************************/

#if defined(VREFBUF)
void HAL_SYSCFG_VREFBUF_VoltageScalingConfig(uint32_t VoltageScaling);
void HAL_SYSCFG_VREFBUF_HighImpedanceConfig(uint32_t Mode);
void HAL_SYSCFG_VREFBUF_TrimmingConfig(uint32_t TrimmingValue);
HAL_StatusTypeDef HAL_SYSCFG_EnableVREFBUF(void);
void HAL_SYSCFG_DisableVREFBUF(void);
#endif /* VREFBUF */

void HAL_SYSCFG_EnableIOAnalogSwitchBooster(void);
void HAL_SYSCFG_DisableIOAnalogSwitchBooster(void);
void HAL_SYSCFG_EnableRemap(uint32_t PinRemap);
void HAL_SYSCFG_DisableRemap(uint32_t PinRemap);
#if defined (SYSCFG_CFGR1_UCPD1_STROBE) || defined (SYSCFG_CFGR1_UCPD2_STROBE)
void HAL_SYSCFG_StrobeDBattpinsConfig(uint32_t ConfigDeadBattery);
#endif
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

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_HAL_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
