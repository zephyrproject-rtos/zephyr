/**
  ******************************************************************************
  * @file    stm32g0xx_ll_ucpd.h
  * @author  MCD Application Team
  * @brief   Header file of UCPD LL module.
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
#ifndef STM32G0xx_LL_UCPD_H
#define STM32G0xx_LL_UCPD_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
  * @{
  */

#if defined (UCPD1) || defined (UCPD2)

/** @defgroup UCPD_LL UCPD
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup UCPD_LL_ES_INIT UCPD Exported Init structure
  * @{
  */

/**
  * @brief  UCPD Init structures definition
  */
typedef struct
{
  uint32_t psc_ucpdclk;         /*!< Specifies the prescaler for the ucpd clock.
                                     This parameter can be a value of @ref UCPD_LL_EC_PSC.
                                     This feature can be modified afterwards using unitary function @ref LL_UCPD_SetPSCClk().*/

  uint32_t transwin;            /*!< Specifies the number of cycles (minus 1) of the half bit clock (see HBITCLKDIV) to achieve a legal
                                    tTransitionWindow (set according to IP clock to define an interval of between 12 and 20 us).
                                    This parameter can be a value between 0x1 and 0x3FF
                                    This value can be modified afterwards using unitary function @ref LL_UCPD_SetTransWin().*/

  uint32_t IfrGap;              /*!< Specifies the definition of the clock divider (minus 1) in order to generate tInterframeGap
                                    from the IP clock.
                                    This parameter can be a value between 0x1 and 0x3FF
                                    This feature can be modified afterwards using unitary function @ref LL_UCPD_SetIfrGap().*/

  uint32_t HbitClockDiv;        /*!< Specifies  the number of cycles (minus one) at the IP for a half bit clock e.g. program 3
                                     for a bit clock that takes 8 cycles of the IP clock "UCPD1_CLK"..
                                     This parameter can be a value between 0x0 and 0x1F.
                                     This feature can be modified afterwards using unitary function @ref LL_UCPD_SetHbitClockDiv().*/

} LL_UCPD_InitTypeDef;

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/* Exported constants --------------------------------------------------------*/
/** @defgroup UCPD_LL_Exported_Constants UCPD Exported Constants
  * @{
  */

/** @defgroup UCPD_LL_EC_GET_FLAG Get Flags Defines
  * @brief    Flags defines which can be used with LL_ucpd_ReadReg function
  * @{
  */
#define LL_UCPD_SR_TXIS             UCPD_SR_TXIS               /*!< Transmit interrupt status            */
#define LL_UCPD_SR_TXMSGDISC        UCPD_SR_TXMSGDISC          /*!< Transmit message discarded interrupt */
#define LL_UCPD_SR_TXMSGSENT        UCPD_SR_TXMSGSENT          /*!< Transmit message sent interrupt      */
#define LL_UCPD_SR_TXMSGABT         UCPD_SR_TXMSGABT           /*!< Transmit message abort interrupt     */
#define LL_UCPD_SR_HRSTDISC         UCPD_SR_HRSTDISC           /*!< HRST discarded interrupt             */
#define LL_UCPD_SR_HRSTSENT         UCPD_SR_HRSTSENT           /*!< HRST sent interrupt                  */
#define LL_UCPD_SR_TXUND            UCPD_SR_TXUND              /*!< Tx data underrun condition interrupt */
#define LL_UCPD_SR_RXNE             UCPD_SR_RXNE               /*!< Receive data register not empty interrupt  */
#define LL_UCPD_SR_RXORDDET         UCPD_SR_RXORDDET           /*!< Rx ordered set (4 K-codes) detected interrupt  */
#define LL_UCPD_SR_RXHRSTDET        UCPD_SR_RXHRSTDET          /*!< Rx Hard Reset detect interrupt  */
#define LL_UCPD_SR_RXOVR            UCPD_SR_RXOVR              /*!< Rx data overflow interrupt  */
#define LL_UCPD_SR_RXMSGEND         UCPD_SR_RXMSGEND           /*!< Rx message received  */
#define LL_UCPD_SR_RXERR            UCPD_SR_RXERR              /*!< Rx error  */
#define LL_UCPD_SR_TYPECEVT1        UCPD_SR_TYPECEVT1          /*!< Type C voltage level event on CC1  */
#define LL_UCPD_SR_TYPECEVT2        UCPD_SR_TYPECEVT2          /*!< Type C voltage level event on CC2  */
#define LL_UCPD_SR_TYPEC_VSTATE_CC1 UCPD_SR_TYPEC_VSTATE_CC1   /*!<Status of DC level on CC1 pin  */
#define LL_UCPD_SR_TYPEC_VSTATE_CC2 UCPD_SR_TYPEC_VSTATE_CC2   /*!<Status of DC level on CC2 pin  */
#define LL_UCPD_SR_FRSEVT           UCPD_SR_FRSEVT             /*!<Fast Role Swap detection event  */

/**
  * @}
  */

/** @defgroup UCPD_LL_EC_IT IT Defines
  * @brief    IT defines which can be used with LL_UCPD_ReadReg and  LL_UCPD_WriteReg functions
  * @{
  */
#define LL_UCPD_IMR_TXIS             UCPD_IMR_TXISIE               /*!< Enable transmit interrupt status            */
#define LL_UCPD_IMR_TXMSGDISC        UCPD_IMR_TXMSGDISCIE          /*!< Enable transmit message discarded interrupt */
#define LL_UCPD_IMR_TXMSGSENT        UCPD_IMR_TXMSGSENTIE          /*!< Enable transmit message sent interrupt      */
#define LL_UCPD_IMR_TXMSGABT         UCPD_IMR_TXMSGABTIE           /*!< Enable transmit message abort interrupt     */
#define LL_UCPD_IMR_HRSTDISC         UCPD_IMR_HRSTDISCIE           /*!< Enable HRST discarded interrupt             */
#define LL_UCPD_IMR_HRSTSENT         UCPD_IMR_HRSTSENTIE           /*!< Enable HRST sent interrupt                  */
#define LL_UCPD_IMR_TXUND            UCPD_IMR_TXUNDIE              /*!< Enable tx data underrun condition interrupt */
#define LL_UCPD_IMR_RXNE             UCPD_IMR_RXNEIE               /*!< Enable Receive data register not empty interrupt  */
#define LL_UCPD_IMR_RXORDDET         UCPD_IMR_RXORDDETIE           /*!< Enable Rx ordered set (4 K-codes) detected interrupt  */
#define LL_UCPD_IMR_RXHRSTDET        UCPD_IMR_RXHRSTDETIE          /*!< Enable Rx Hard Reset detect interrupt  */
#define LL_UCPD_IMR_RXOVR            UCPD_IMR_RXOVRIE              /*!< Enable Rx data overflow interrupt  */
#define LL_UCPD_IMR_RXMSGEND         UCPD_IMR_RXMSGEND             /*!< Enable Rx message received  */
#define LL_UCPD_IMR_RXERR            UCPD_IMR_RXMSGENDIE           /*!< Enable Rx error  */
#define LL_UCPD_IMR_TYPECEVT1        UCPD_IMR_TYPECEVT1IE          /*!< Enable Type C voltage level event on CC1  */
#define LL_UCPD_IMR_TYPECEVT2        UCPD_IMR_TYPECEVT2IE          /*!< Enable Type C voltage level event on CC2  */
#define LL_UCPD_IMR_FRSEVT           UCPD_IMR_FRSEVTIE             /*!< Enable fast Role Swap detection event  */
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_ORDERSET orderset value
  * @brief    definition of the usual orderset
  * @{
  */
#define LL_UCPD_SYNC1 0x18u
#define LL_UCPD_SYNC2 0x11u
#define LL_UCPD_SYNC3 0x06u
#define LL_UCPD_RST1  0x07u
#define LL_UCPD_RST2  0x19u
#define LL_UCPD_EOP   0x0Du

#define LL_UCPD_ORDERED_SET_SOP         (LL_UCPD_SYNC1 | (LL_UCPD_SYNC1<<5u) | (LL_UCPD_SYNC1<<10u) | (LL_UCPD_SYNC2<<15u))
#define LL_UCPD_ORDERED_SET_SOP1        (LL_UCPD_SYNC1 | (LL_UCPD_SYNC1<<5u) | (LL_UCPD_SYNC3<<10u) | (LL_UCPD_SYNC3<<15u))
#define LL_UCPD_ORDERED_SET_SOP2        (LL_UCPD_SYNC1 | (LL_UCPD_SYNC3<<5u) | (LL_UCPD_SYNC1<<10u) | (LL_UCPD_SYNC3<<15u))
#define LL_UCPD_ORDERED_SET_HARD_RESET  (LL_UCPD_RST1  | (LL_UCPD_RST1<<5u)  | (LL_UCPD_RST1<<10u)  | (LL_UCPD_RST2<<15u ))
#define LL_UCPD_ORDERED_SET_CABLE_RESET (LL_UCPD_RST1  | (LL_UCPD_SYNC1<<5u) | (LL_UCPD_RST1<<10u)  | (LL_UCPD_SYNC3<<15u))
#define LL_UCPD_ORDERED_SET_SOP1_DEBUG  (LL_UCPD_SYNC1 | (LL_UCPD_RST2<<5u)  | (LL_UCPD_RST2<<10u)  | (LL_UCPD_SYNC3<<15u))
#define LL_UCPD_ORDERED_SET_SOP2_DEBUG  (LL_UCPD_SYNC1 | (LL_UCPD_RST2<<5u)  | (LL_UCPD_SYNC3<<10u) | (LL_UCPD_SYNC2<<15u))
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_MODE Role Mode
  * @{
  */
#define LL_UCPD_ROLE_SNK             UCPD_CR_ANAMODE        /*!< Mode SNK Rd         */
#define LL_UCPD_ROLE_SRC             0x0U
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_RESISTOR resistor value
  * @{
  */
#define LL_UCPD_RESISTOR_DEFAULT    UCPD_CR_ANASUBMODE_0   /*!< Rp default  */
#define LL_UCPD_RESISTOR_1_5A       UCPD_CR_ANASUBMODE_1   /*!< Rp 1.5 A    */
#define LL_UCPD_RESISTOR_3_0A       UCPD_CR_ANASUBMODE     /*!< Rp 3.0 A    */
#define LL_UCPD_RESISTOR_NONE       0x0U                    /*!< No resistor */
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_CFG1_ORDERSET orderset configuration
  * @{
  */
#define LL_UCPD_ORDERSET_SOP         UCPD_CFG1_RXORDSETEN_0
#define LL_UCPD_ORDERSET_SOP1        UCPD_CFG1_RXORDSETEN_1
#define LL_UCPD_ORDERSET_SOP2        UCPD_CFG1_RXORDSETEN_2
#define LL_UCPD_ORDERSET_HARDRST     UCPD_CFG1_RXORDSETEN_3
#define LL_UCPD_ORDERSET_CABLERST    UCPD_CFG1_RXORDSETEN_4
#define LL_UCPD_ORDERSET_SOP1_DEBUG  UCPD_CFG1_RXORDSETEN_5
#define LL_UCPD_ORDERSET_SOP2_DEBUG  UCPD_CFG1_RXORDSETEN_6
#define LL_UCPD_ORDERSET_SOP_EXT1    UCPD_CFG1_RXORDSETEN_7
#define LL_UCPD_ORDERSET_SOP_EXT2    UCPD_CFG1_RXORDSETEN_8
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_CCxEVT  ccxevt
  * @{
  */
#define LL_UCPD_SNK_CC1_VOPEN      0x00u
#define LL_UCPD_SNK_CC1_VRP        UCPD_SR_TYPEC_VSTATE_CC1_0
#define LL_UCPD_SNK_CC1_VRP15A     UCPD_SR_TYPEC_VSTATE_CC1_1
#define LL_UCPD_SNK_CC1_VRP30A     (UCPD_SR_TYPEC_VSTATE_CC1_0 | UCPD_SR_TYPEC_VSTATE_CC1_1)

#define LL_UCPD_SNK_CC2_VOPEN      0x00u
#define LL_UCPD_SNK_CC2_VRP        UCPD_SR_TYPEC_VSTATE_CC2_0
#define LL_UCPD_SNK_CC2_VRP15A     UCPD_SR_TYPEC_VSTATE_CC2_1
#define LL_UCPD_SNK_CC2_VRP30A     (UCPD_SR_TYPEC_VSTATE_CC2_0 | UCPD_SR_TYPEC_VSTATE_CC2_1)

#define LL_UCPD_SRC_CC1_VRA        0x0U
#define LL_UCPD_SRC_CC1_VRD        UCPD_SR_TYPEC_VSTATE_CC1_0
#define LL_UCPD_SRC_CC1_OPEN       UCPD_SR_TYPEC_VSTATE_CC1_1

#define LL_UCPD_SRC_CC2_VRA        0x0U
#define LL_UCPD_SRC_CC2_VRD        UCPD_SR_TYPEC_VSTATE_CC2_0
#define LL_UCPD_SRC_CC2_OPEN       UCPD_SR_TYPEC_VSTATE_CC2_1
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_PSC prescaler for UCPDCLK
  * @{
  */
#define LL_UCPD_PSC_DIV1            0x0u
#define LL_UCPD_PSC_DIV2            UCPD_CFG1_PSC_UCPDCLK_0
#define LL_UCPD_PSC_DIV4            UCPD_CFG1_PSC_UCPDCLK_1
#define LL_UCPD_PSC_DIV8            (UCPD_CFG1_PSC_UCPDCLK_1 | UCPD_CFG1_PSC_UCPDCLK_0)
#define LL_UCPD_PSC_DIV16           UCPD_CFG1_PSC_UCPDCLK_2
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_CCENABLE cc pin enable
  * @{
  */
#define LL_UCPD_CCENABLE_NONE       0x0U
#define LL_UCPD_CCENABLE_CC1        UCPD_CR_CCENABLE_0
#define LL_UCPD_CCENABLE_CC2        UCPD_CR_CCENABLE_1
#define LL_UCPD_CCENABLE_CC1CC2     (UCPD_CR_CCENABLE_0 | UCPD_CR_CCENABLE_1)
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_CCPIN cc pin selection
  * @{
  */
#define LL_UCPD_CCPIN_CC1           0x0U
#define LL_UCPD_CCPIN_CC2           UCPD_CR_PHYCCSEL
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_RXMODE rx mode
  * @{
  */
#define LL_UCPD_RXMODE_NORMAL           0x0U
#define LL_UCPD_RXMODE_BIST_TEST_DATA   UCPD_CR_RXMODE
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_TXMODE tx mode
  * @{
  */
#define LL_UCPD_TXMODE_NORMAL           0x0U
#define LL_UCPD_TXMODE_CABLE_RESET      UCPD_CR_TXMODE_0
#define LL_UCPD_TXMODE_BIST_CARRIER2    UCPD_CR_TXMODE_1
/**
  * @}
  */

/** @defgroup UCPD_LL_EC_RXORDSET rx orderset
  * @{
  */
#define LL_UCPD_RXORDSET_SOP             0x0U
#define LL_UCPD_RXORDSET_SOP1            UCPD_RX_ORDSET_RXORDSET_0
#define LL_UCPD_RXORDSET_SOP2            UCPD_RX_ORDSET_RXORDSET_1
#define LL_UCPD_RXORDSET_SOP1_DEBUG      (UCPD_RX_ORDSET_RXORDSET_0 | UCPD_RX_ORDSET_RXORDSET_1)
#define LL_UCPD_RXORDSET_SOP2_DEBUG      UCPD_RX_ORDSET_RXORDSET_2
#define LL_UCPD_RXORDSET_CABLE_RESET     (UCPD_RX_ORDSET_RXORDSET_2 | UCPD_RX_ORDSET_RXORDSET_0)
#define LL_UCPD_RXORDSET_SOPEXT1         (UCPD_RX_ORDSET_RXORDSET_2 | UCPD_RX_ORDSET_RXORDSET_1)
#define LL_UCPD_RXORDSET_SOPEXT2         (UCPD_RX_ORDSET_RXORDSET_2 | UCPD_RX_ORDSET_RXORDSET_1 | UCPD_RX_ORDSET_RXORDSET_0)
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup UCPD_LL_Exported_Macros UCPD Exported Macros
  * @{
  */

/** @defgroup UCPD_LL_EM_WRITE_READ Common Write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in UCPD register
  * @param  __INSTANCE__ UCPD Instance
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_UCPD_WriteReg(__INSTANCE__, __REG__, __VALUE__) WRITE_REG((__INSTANCE__)->__REG__, (__VALUE__))

/**
  * @brief  Read a value in UCPD register
  * @param  __INSTANCE__ UCPD Instance
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_UCPD_ReadReg(__INSTANCE__, __REG__) READ_REG((__INSTANCE__)->__REG__)
/**
  * @}
  */

/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @defgroup UCPD_LL_Exported_Functions UCPD Exported Functions
  * @{
  */

/** @defgroup UCPD_LL_EF_Configuration Configuration
  * @{
  */

/** @defgroup UCPD_LL_EF_CFG1 CFG1 register
  * @{
  */
/**
  * @brief  Enable UCPD peripheral
  * @rmtoll CFG1          UCPDEN           LL_UCPD_Enable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_Enable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CFG1, UCPD_CFG1_UCPDEN);
}

/**
  * @brief  Disable UCPD peripheral
  * @note   When disabling the UCPD, follow the procedure described in the Reference Manual.
  * @rmtoll CFG1          UCPDEN           LL_UCPD_Disable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_Disable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CFG1, UCPD_CFG1_UCPDEN);
}

/**
  * @brief  Check if UCPD peripheral is enabled
  * @rmtoll CFG1          UCPDEN           LL_UCPD_IsEnabled
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnabled(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->CFG1, UCPD_CFG1_UCPDEN) == (UCPD_CFG1_UCPDEN)) ? 1UL : 0UL);
}

/**
  * @brief  Set the receiver ordered set detection enable
  * @rmtoll CFG1          RXORDSETEN          LL_UCPD_SetRxOrderSet
  * @param  UCPDx UCPD Instance
  * @param  OrderSet This parameter can be combinaison of the following values:
  *         @arg @ref LL_UCPD_ORDERSET_SOP
  *         @arg @ref LL_UCPD_ORDERSET_SOP1
  *         @arg @ref LL_UCPD_ORDERSET_SOP2
  *         @arg @ref LL_UCPD_ORDERSET_HARDRST
  *         @arg @ref LL_UCPD_ORDERSET_CABLERST
  *         @arg @ref LL_UCPD_ORDERSET_SOP1_DEBUG
  *         @arg @ref LL_UCPD_ORDERSET_SOP2_DEBUG
  *         @arg @ref LL_UCPD_ORDERSET_SOP_EXT1
  *         @arg @ref LL_UCPD_ORDERSET_SOP_EXT2
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetRxOrderSet(UCPD_TypeDef *UCPDx, uint32_t OrderSet)
{
  MODIFY_REG(UCPDx->CFG1, UCPD_CFG1_RXORDSETEN, OrderSet);
}

/**
  * @brief  Set the prescaler for ucpd clock
  * @rmtoll CFG1          UCPDCLK          LL_UCPD_SetPSCClk
  * @param  UCPDx UCPD Instance
  * @param  Psc This parameter can be one of the following values:
  *         @arg @ref LL_UCPD_PSC_DIV1
  *         @arg @ref LL_UCPD_PSC_DIV2
  *         @arg @ref LL_UCPD_PSC_DIV4
  *         @arg @ref LL_UCPD_PSC_DIV8
  *         @arg @ref LL_UCPD_PSC_DIV16
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetPSCClk(UCPD_TypeDef *UCPDx, uint32_t Psc)
{
  MODIFY_REG(UCPDx->CFG1, UCPD_CFG1_PSC_UCPDCLK, Psc);
}

/**
  * @brief  Set the number of cycles (minus 1) of the half bit clock
  * @rmtoll CFG1          TRANSWIN          LL_UCPD_SetTransWin
  * @param  UCPDx UCPD Instance
  * @param  TransWin a value between 1 and 0x3FF
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetTransWin(UCPD_TypeDef *UCPDx, uint32_t TransWin)
{
  MODIFY_REG(UCPDx->CFG1, UCPD_CFG1_TRANSWIN, TransWin << UCPD_CFG1_TRANSWIN_Pos);
}

/**
  * @brief  Set the clock divider value to generate an interframe gap
  * @rmtoll CFG1          IFRGAP          LL_UCPD_SetIfrGap
  * @param  UCPDx UCPD Instance
  * @param  IfrGap a value between 1 and 0x3FF
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetIfrGap(UCPD_TypeDef *UCPDx, uint32_t IfrGap)
{
  MODIFY_REG(UCPDx->CFG1, UCPD_CFG1_IFRGAP, IfrGap << UCPD_CFG1_IFRGAP_Pos);
}

/**
  * @brief  Set the clock divider value to generate an interframe gap
  * @rmtoll CFG1          HBITCLKDIV          LL_UCPD_SetHbitClockDiv
  * @param  UCPDx UCPD Instance
  * @param  HbitClock a value between 0 and 31
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetHbitClockDiv(UCPD_TypeDef *UCPDx, uint32_t HbitClock)
{
  MODIFY_REG(UCPDx->CFG1, UCPD_CFG1_HBITCLKDIV, HbitClock << UCPD_CFG1_HBITCLKDIV_Pos);
}

/**
  * @}
  */

/** @defgroup UCPD_LL_EF_CFG2 CFG2 register
  * @{
  */

/**
  * @brief  Enable the wakeup mode
  * @rmtoll CFG2          WUPEN          LL_UCPD_WakeUpEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_WakeUpEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CFG2, UCPD_CFG2_WUPEN);
}

/**
  * @brief  Disable the wakeup mode
  * @rmtoll CFG2          WUPEN          LL_UCPD_WakeUpDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_WakeUpDisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CFG2, UCPD_CFG2_WUPEN);
}

/**
  * @brief  Force clock enable
  * @rmtoll CFG2          FORCECLK          LL_UCPD_ForceClockEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ForceClockEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CFG2, UCPD_CFG2_FORCECLK);
}

/**
  * @brief  Force clock disable
  * @rmtoll CFG2          FORCECLK          LL_UCPD_ForceClockDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ForceClockDisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CFG2, UCPD_CFG2_FORCECLK);
}

/**
  * @brief  RxFilter enable
  * @rmtoll CFG2          RXFILTDIS          LL_UCPD_RxFilterEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_RxFilterEnable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CFG2, UCPD_CFG2_RXFILTDIS);
}

/**
  * @brief  RxFilter disable
  * @rmtoll CFG2          RXFILTDIS          LL_UCPD_RxFilterDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_RxFilterDisable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CFG2, UCPD_CFG2_RXFILTDIS);
}

/**
  * @}
  */

/**
  * @}
  */

/** @defgroup UCPD_LL_EF_CR CR register
  * @{
  */
/**
  * @brief  Type C detector for CC2 enable
  * @rmtoll CR          CC2TCDIS          LL_UCPD_TypeCDetectionCC2Enable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_TypeCDetectionCC2Enable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CR, UCPD_CR_CC2TCDIS);
}

/**
  * @brief  Type C detector for CC2 disable
  * @rmtoll CR          CC2TCDIS          LL_UCPD_TypeCDetectionCC2Disable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_TypeCDetectionCC2Disable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_CC2TCDIS);
}

/**
  * @brief  Type C detector for CC1 enable
  * @rmtoll CR          CC1TCDIS          LL_UCPD_TypeCDetectionCC1Enable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_TypeCDetectionCC1Enable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CR, UCPD_CR_CC1TCDIS);
}

/**
  * @brief  Type C detector for CC1 disable
  * @rmtoll CR          CC1TCDIS          LL_UCPD_TypeCDetectionCC1Disable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_TypeCDetectionCC1Disable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_CC1TCDIS);
}

/**
  * @brief  Source Vconn discharge enable
  * @rmtoll CR          RDCH          LL_UCPD_VconnDischargeEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_VconnDischargeEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_RDCH);
}

/**
  * @brief  Source Vconn discharge disable
  * @rmtoll CR          RDCH          LL_UCPD_VconnDischargeDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_VconnDischargeDisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CR, UCPD_CR_RDCH);
}

/**
  * @brief  Signal Fast Role Swap request
  * @rmtoll CR          FRSTX          LL_UCPD_VconnDischargeDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SignalFRSTX(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_FRSTX);
}

/**
  * @brief  Fast Role swap RX detection enable
  * @rmtoll CR          FRSRXEN          LL_UCPD_FRSDetectionEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_FRSDetectionEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_FRSRXEN);
}

/**
  * @brief  Fast Role swap RX detection disable
  * @rmtoll CR          FRSRXEN          LL_UCPD_FRSDetectionDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_FRSDetectionDisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CR, UCPD_CR_FRSRXEN);
}

/**
  * @brief  Set cc enable
  * @rmtoll CR          CC1VCONNEN          LL_UCPD_SetccEnable
  * @param  UCPDx UCPD Instance
  * @param  CCEnable This parameter can be one of the following values:
  *         @arg @ref LL_UCPD_CCENABLE_NONE
  *         @arg @ref LL_UCPD_CCENABLE_CC1
  *         @arg @ref LL_UCPD_CCENABLE_CC2
  *         @arg @ref LL_UCPD_CCENABLE_CC1CC2
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetccEnable(UCPD_TypeDef *UCPDx, uint32_t CCEnable)
{
  MODIFY_REG(UCPDx->CR, UCPD_CR_CCENABLE, CCEnable);
}

/**
  * @brief  Set UCPD SNK role
  * @rmtoll CR        ANAMODE          LL_UCPD_SetSNKRole
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetSNKRole(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_ANAMODE);
}

/**
  * @brief  Set UCPD SRC role
  * @rmtoll CR        ANAMODE          LL_UCPD_SetSRCRole
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetSRCRole(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CR, UCPD_CR_ANAMODE);
}

/**
  * @brief  Get UCPD Role
  * @rmtoll CR          ANAMODE          LL_UCPD_GetRole
  * @param  UCPDx UCPD Instance
  * @retval Returned value can be one of the following values:
  *         @arg @ref LL_UCPD_ROLE_SNK
  *         @arg @ref LL_UCPD_ROLE_SRC
  */
__STATIC_INLINE uint32_t LL_UCPD_GetRole(UCPD_TypeDef const * const UCPDx)
{
  return (uint32_t)(READ_BIT(UCPDx->CR, UCPD_CR_ANAMODE));
}

/**
  * @brief  Set Rp resistor
  * @rmtoll CR        ANASUBMODE          LL_UCPD_SetRpResistor
  * @param  UCPDx UCPD Instance
  * @param  Resistor This parameter can be one of the following values:
  *         @arg @ref LL_UCPD_RESISTOR_DEFAULT
  *         @arg @ref LL_UCPD_RESISTOR_1_5A
  *         @arg @ref LL_UCPD_RESISTOR_3_0A
  *         @arg @ref LL_UCPD_RESISTOR_NONE
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetRpResistor(UCPD_TypeDef *UCPDx, uint32_t Resistor)
{
  MODIFY_REG(UCPDx->CR, UCPD_CR_ANASUBMODE,  Resistor);
}

/**
  * @brief  Set CC pin
  * @rmtoll CR        PHYCCSEL          LL_UCPD_SetCCPin
  * @param  UCPDx UCPD Instance
  * @param  CCPin This parameter can be one of the following values:
  *         @arg @ref LL_UCPD_CCPIN_CC1
  *         @arg @ref LL_UCPD_CCPIN_CC2
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetCCPin(UCPD_TypeDef *UCPDx, uint32_t CCPin)
{
  MODIFY_REG(UCPDx->CR, UCPD_CR_PHYCCSEL,  CCPin);
}

/**
  * @brief  Rx enable
  * @rmtoll CR        PHYRXEN          LL_UCPD_RxEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_RxEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_PHYRXEN);
}

/**
  * @brief  Rx disable
  * @rmtoll CR        PHYRXEN          LL_UCPD_RxDisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_RxDisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CR, UCPD_CR_PHYRXEN);
}

/**
  * @brief  Set Rx mode
  * @rmtoll CR        RXMODE          LL_UCPD_SetRxMode
  * @param  UCPDx UCPD Instance
  * @param  RxMode This parameter can be one of the following values:
  *         @arg @ref LL_UCPD_RXMODE_NORMAL
  *         @arg @ref LL_UCPD_RXMODE_BIST_TEST_DATA
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetRxMode(UCPD_TypeDef *UCPDx, uint32_t RxMode)
{
  MODIFY_REG(UCPDx->CR, UCPD_CR_RXMODE, RxMode);
}

/**
  * @brief  Send Hard Reset
  * @rmtoll CR        TXHRST          LL_UCPD_SendHardReset
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SendHardReset(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_TXHRST);
}

/**
  * @brief  Send message
  * @rmtoll CR        TXSEND          LL_UCPD_SendMessage
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SendMessage(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CR, UCPD_CR_TXSEND);
}

/**
  * @brief  Set Tx mode
  * @rmtoll CR        TXMODE          LL_UCPD_SetTxMode
  * @param  UCPDx UCPD Instance
  * @param  TxMode This parameter can be one of the following values:
  *         @arg @ref LL_UCPD_TXMODE_NORMAL
  *         @arg @ref LL_UCPD_TXMODE_CABLE_RESET
  *         @arg @ref LL_UCPD_TXMODE_BIST_CARRIER2
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetTxMode(UCPD_TypeDef *UCPDx, uint32_t TxMode)
{
  MODIFY_REG(UCPDx->CR, UCPD_CR_TXMODE, TxMode);
}

/**
  * @}
  */

/** @defgroup UCPD_LL_EF_IT_Management Interrupt Management
  * @{
  */

/**
  * @brief  Enable FRS interrupt
  * @rmtoll IMR          FRSEVTIE         LL_UCPD_EnableIT_FRS
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_FRS(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_FRSEVTIE);
}

/**
  * @brief  Enable type c event on CC2
  * @rmtoll IMR          TYPECEVT2IE        LL_UCPD_EnableIT_TypeCEventCC2
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TypeCEventCC2(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TYPECEVT2IE);
}

/**
  * @brief  Enable type c event on CC1
  * @rmtoll IMR          TYPECEVT1IE        LL_UCPD_EnableIT_TypeCEventCC1
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TypeCEventCC1(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TYPECEVT1IE);
}

/**
  * @brief  Enable Rx message end interrupt
  * @rmtoll IMR          RXMSGENDIE         LL_UCPD_EnableIT_RxMsgEnd
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_RxMsgEnd(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_RXMSGENDIE);
}

/**
  * @brief  Enable Rx overrun interrupt
  * @rmtoll IMR          RXOVRIE         LL_UCPD_EnableIT_RxOvr
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_RxOvr(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_RXOVRIE);
}

/**
  * @brief  Enable Rx hard resrt interrupt
  * @rmtoll IMR          RXHRSTDETIE         LL_UCPD_EnableIT_RxHRST
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_RxHRST(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_RXHRSTDETIE);
}

/**
  * @brief  Enable Rx orderset interrupt
  * @rmtoll IMR          RXORDDETIE         LL_UCPD_EnableIT_RxOrderSet
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_RxOrderSet(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_RXORDDETIE);
}

/**
  * @brief  Enable Rx non empty interrupt
  * @rmtoll IMR          RXNEIE         LL_UCPD_EnableIT_RxNE
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_RxNE(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_RXNEIE);
}

/**
  * @brief  Enable TX underrun interrupt
  * @rmtoll IMR          TXUNDIE         LL_UCPD_EnableIT_TxUND
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxUND(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TXUNDIE);
}

/**
  * @brief  Enable hard reset sent interrupt
  * @rmtoll IMR          HRSTSENTIE         LL_UCPD_EnableIT_TxHRSTSENT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxHRSTSENT(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_HRSTSENTIE);
}

/**
  * @brief  Enable hard reset discard interrupt
  * @rmtoll IMR          HRSTDISCIE         LL_UCPD_EnableIT_TxHRSTDISC
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxHRSTDISC(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_HRSTDISCIE);
}

/**
  * @brief  Enable Tx message abort interrupt
  * @rmtoll IMR          TXMSGABTIE         LL_UCPD_EnableIT_TxMSGABT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxMSGABT(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TXMSGABTIE);
}

/**
  * @brief  Enable Tx message sent interrupt
  * @rmtoll IMR          TXMSGSENTIE         LL_UCPD_EnableIT_TxMSGSENT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxMSGSENT(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TXMSGSENTIE);
}

/**
  * @brief  Enable Tx message discarded interrupt
  * @rmtoll IMR          TXMSGDISCIE         LL_UCPD_EnableIT_TxMSGDISC
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxMSGDISC(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TXMSGDISCIE);
}

/**
  * @brief  Enable Tx data receive interrupt
  * @rmtoll IMR          TXISIE         LL_UCPD_EnableIT_TxIS
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_EnableIT_TxIS(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->IMR, UCPD_IMR_TXISIE);
}

/**
  * @brief  Disable FRS interrupt
  * @rmtoll IMR          FRSEVTIE         LL_UCPD_DisableIT_FRS
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_FRS(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_FRSEVTIE);
}

/**
  * @brief  Disable type c event on CC2
  * @rmtoll IMR          TYPECEVT2IE        LL_UCPD_DisableIT_TypeCEventCC2
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TypeCEventCC2(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TYPECEVT2IE);
}

/**
  * @brief  Disable type c event on CC1
  * @rmtoll IMR          TYPECEVT1IE        LL_UCPD_DisableIT_TypeCEventCC1
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TypeCEventCC1(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TYPECEVT1IE);
}

/**
  * @brief  Disable Rx message end interrupt
  * @rmtoll IMR          RXMSGENDIE         LL_UCPD_DisableIT_RxMsgEnd
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_RxMsgEnd(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_RXMSGENDIE);
}

/**
  * @brief  Disable Rx overrun interrupt
  * @rmtoll IMR          RXOVRIE         LL_UCPD_DisableIT_RxOvr
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_RxOvr(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_RXOVRIE);
}

/**
  * @brief  Disable Rx hard resrt interrupt
  * @rmtoll IMR          RXHRSTDETIE         LL_UCPD_DisableIT_RxHRST
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_RxHRST(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_RXHRSTDETIE);
}

/**
  * @brief  Disable Rx orderset interrupt
  * @rmtoll IMR          RXORDDETIE         LL_UCPD_DisableIT_RxOrderSet
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_RxOrderSet(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_RXORDDETIE);
}

/**
  * @brief  Disable Rx non empty interrupt
  * @rmtoll IMR          RXNEIE         LL_UCPD_DisableIT_RxNE
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_RxNE(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_RXNEIE);
}

/**
  * @brief  Disable TX underrun interrupt
  * @rmtoll IMR          TXUNDIE         LL_UCPD_DisableIT_TxUND
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxUND(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TXUNDIE);
}

/**
  * @brief  Disable hard reset sent interrupt
  * @rmtoll IMR          HRSTSENTIE         LL_UCPD_DisableIT_TxHRSTSENT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxHRSTSENT(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_HRSTSENTIE);
}

/**
  * @brief  Disable hard reset discard interrupt
  * @rmtoll IMR          HRSTDISCIE         LL_UCPD_DisableIT_TxHRSTDISC
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxHRSTDISC(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_HRSTDISCIE);
}

/**
  * @brief  Disable Tx message abort interrupt
  * @rmtoll IMR          TXMSGABTIE         LL_UCPD_DisableIT_TxMSGABT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxMSGABT(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TXMSGABTIE);
}

/**
  * @brief  Disable Tx message sent interrupt
  * @rmtoll IMR          TXMSGSENTIE         LL_UCPD_DisableIT_TxMSGSENT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxMSGSENT(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TXMSGSENTIE);
}

/**
  * @brief  Disable Tx message discarded interrupt
  * @rmtoll IMR          TXMSGDISCIE         LL_UCPD_DisableIT_TxMSGDISC
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxMSGDISC(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TXMSGDISCIE);
}

/**
  * @brief  Disable Tx data receive interrupt
  * @rmtoll IMR          TXISIE         LL_UCPD_DisableIT_TxIS
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_DisableIT_TxIS(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->IMR, UCPD_IMR_TXISIE);
}

/**
  * @brief  Check if FRS interrupt enabled
  * @rmtoll IMR          FRSEVTIE         LL_UCPD_DisableIT_FRS
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_FRS(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_FRSEVTIE) == UCPD_IMR_FRSEVTIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if type c event on CC2 enabled
  * @rmtoll IMR          TYPECEVT2IE        LL_UCPD_DisableIT_TypeCEventCC2
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TypeCEventCC2(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TYPECEVT2IE) == UCPD_IMR_TYPECEVT2IE) ? 1UL : 0UL);
}

/**
  * @brief  Check if type c event on CC1 enabled
  * @rmtoll IMR2          TYPECEVT1IE        LL_UCPD_IsEnableIT_TypeCEventCC1
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TypeCEventCC1(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TYPECEVT1IE) == UCPD_IMR_TYPECEVT1IE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx message end interrupt enabled
  * @rmtoll IMR          RXMSGENDIE         LL_UCPD_IsEnableIT_RxMsgEnd
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_RxMsgEnd(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_RXMSGENDIE) == UCPD_IMR_RXMSGENDIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx overrun interrupt enabled
  * @rmtoll IMR          RXOVRIE         LL_UCPD_IsEnableIT_RxOvr
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_RxOvr(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_RXOVRIE) == UCPD_IMR_RXOVRIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx hard resrt interrupt enabled
  * @rmtoll IMR          RXHRSTDETIE         LL_UCPD_IsEnableIT_RxHRST
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_RxHRST(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_RXHRSTDETIE) == UCPD_IMR_RXHRSTDETIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx orderset interrupt enabled
  * @rmtoll IMR          RXORDDETIE         LL_UCPD_IsEnableIT_RxOrderSet
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_RxOrderSet(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_RXORDDETIE) == UCPD_IMR_RXORDDETIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx non empty interrupt enabled
  * @rmtoll IMR          RXNEIE         LL_UCPD_IsEnableIT_RxNE
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_RxNE(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_RXNEIE) == UCPD_IMR_RXNEIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if TX underrun interrupt enabled
  * @rmtoll IMR          TXUNDIE         LL_UCPD_IsEnableIT_TxUND
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxUND(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TXUNDIE) == UCPD_IMR_TXUNDIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if hard reset sent interrupt enabled
  * @rmtoll IMR          HRSTSENTIE         LL_UCPD_IsEnableIT_TxHRSTSENT
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxHRSTSENT(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_HRSTSENTIE) == UCPD_IMR_HRSTSENTIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if hard reset discard interrupt enabled
  * @rmtoll IMR          HRSTDISCIE         LL_UCPD_IsEnableIT_TxHRSTDISC
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxHRSTDISC(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_HRSTDISCIE) == UCPD_IMR_HRSTDISCIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx message abort interrupt enabled
  * @rmtoll IMR          TXMSGABTIE         LL_UCPD_IsEnableIT_TxMSGABT
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxMSGABT(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TXMSGABTIE) == UCPD_IMR_TXMSGABTIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx message sent interrupt enabled
  * @rmtoll IMR          TXMSGSENTIE         LL_UCPD_IsEnableIT_TxMSGSENT
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxMSGSENT(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TXMSGSENTIE) == UCPD_IMR_TXMSGSENTIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx message discarded interrupt enabled
  * @rmtoll IMR          TXMSGDISCIE         LL_UCPD_IsEnableIT_TxMSGDISC
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxMSGDISC(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TXMSGDISCIE) == UCPD_IMR_TXMSGDISCIE) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx data receive interrupt enabled
  * @rmtoll IMR          TXISIE         LL_UCPD_IsEnableIT_TxIS
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnableIT_TxIS(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->IMR, UCPD_IMR_TXISIE) == UCPD_IMR_TXISIE) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup UCPD_LL_EF_IT_Clear Interrupt Clear
  * @{
  */
/**
  * @brief  Clear FRS interrupt
  * @rmtoll ICR          FRSEVTIE         LL_UCPD_ClearFlag_FRS
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_FRS(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_FRSEVTCF);
}

/**
  * @brief  Clear type c event on CC2
  * @rmtoll IIMR          TYPECEVT2IE        LL_UCPD_ClearFlag_TypeCEventCC2
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TypeCEventCC2(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_TYPECEVT2CF);
}

/**
  * @brief  Clear type c event on CC1
  * @rmtoll IIMR          TYPECEVT1IE        LL_UCPD_ClearFlag_TypeCEventCC1
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TypeCEventCC1(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_TYPECEVT1CF);
}

/**
  * @brief  Clear Rx message end interrupt
  * @rmtoll ICR          RXMSGENDIE         LL_UCPD_ClearFlag_RxMsgEnd
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_RxMsgEnd(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_RXMSGENDCF);
}

/**
  * @brief  Clear Rx overrun interrupt
  * @rmtoll ICR          RXOVRIE         LL_UCPD_ClearFlag_RxOvr
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_RxOvr(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_RXOVRCF);
}

/**
  * @brief  Clear Rx hard resrt interrupt
  * @rmtoll ICR          RXHRSTDETIE         LL_UCPD_ClearFlag_RxHRST
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_RxHRST(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_RXHRSTDETCF);
}

/**
  * @brief  Clear Rx orderset interrupt
  * @rmtoll ICR          RXORDDETIE         LL_UCPD_ClearFlag_RxOrderSet
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_RxOrderSet(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_RXORDDETCF);
}

/**
  * @brief  Clear TX underrun interrupt
  * @rmtoll ICR          TXUNDIE         LL_UCPD_ClearFlag_TxUND
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TxUND(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_TXUNDCF);
}

/**
  * @brief  Clear hard reset sent interrupt
  * @rmtoll ICR          HRSTSENTIE         LL_UCPD_ClearFlag_TxHRSTSENT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TxHRSTSENT(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_HRSTSENTCF);
}

/**
  * @brief  Clear hard reset discard interrupt
  * @rmtoll ICR          HRSTDISCIE         LL_UCPD_ClearFlag_TxHRSTDISC
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TxHRSTDISC(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_HRSTDISCCF);
}

/**
  * @brief  Clear Tx message abort interrupt
  * @rmtoll ICR          TXMSGABTIE         LL_UCPD_ClearFlag_TxMSGABT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TxMSGABT(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_TXMSGABTCF);
}

/**
  * @brief  Clear Tx message sent interrupt
  * @rmtoll ICR          TXMSGSENTIE         LL_UCPD_ClearFlag_TxMSGSENT
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TxMSGSENT(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_TXMSGSENTCF);
}

/**
  * @brief  Clear Tx message discarded interrupt
  * @rmtoll ICR          TXMSGDISCIE         LL_UCPD_ClearFlag_TxMSGDISC
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_ClearFlag_TxMSGDISC(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->ICR, UCPD_ICR_TXMSGDISCCF);
}

/**
  * @}
  */

/** @defgroup UCPD_LL_EF_FLAG_Management FLAG Management
  * @{
  */

/**
  * @brief  Check if FRS interrupt
  * @rmtoll SR          FRSEVT         LL_UCPD_IsActiveFlag_FRS
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_FRS(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_FRSEVT) == UCPD_SR_FRSEVT) ? 1UL : 0UL);
}

/**
  * @brief  Check if type c event on CC2
  * @rmtoll SR          TYPECEVT2        LL_UCPD_IsActiveFlag_TypeCEventCC2
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TypeCEventCC2(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TYPECEVT2) == UCPD_SR_TYPECEVT2) ? 1UL : 0UL);
}

/**
  * @brief  Check if type c event on CC1
  * @rmtoll SR          TYPECEVT1        LL_UCPD_IsActiveFlag_TypeCEventCC1
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TypeCEventCC1(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TYPECEVT1) == UCPD_SR_TYPECEVT1) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx message end interrupt
  * @rmtoll SR          RXMSGEND         LL_UCPD_IsActiveFlag_RxMsgEnd
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_RxMsgEnd(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_RXMSGEND) == UCPD_SR_RXMSGEND) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx overrun interrupt
  * @rmtoll SR          RXOVR         LL_UCPD_IsActiveFlag_RxOvr
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_RxOvr(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_RXOVR) == UCPD_SR_RXOVR) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx hard resrt interrupt
  * @rmtoll SR          RXHRSTDET         LL_UCPD_IsActiveFlag_RxHRST
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_RxHRST(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_RXHRSTDET) == UCPD_SR_RXHRSTDET) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx orderset interrupt
  * @rmtoll SR          RXORDDET         LL_UCPD_IsActiveFlag_RxOrderSet
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_RxOrderSet(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_RXORDDET) == UCPD_SR_RXORDDET) ? 1UL : 0UL);
}

/**
  * @brief  Check if Rx non empty interrupt
  * @rmtoll SR          RXNE         LL_UCPD_IsActiveFlag_RxNE
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_RxNE(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_RXNE) == UCPD_SR_RXNE) ? 1UL : 0UL);
}

/**
  * @brief  Check if TX underrun interrupt
  * @rmtoll SR          TXUND         LL_UCPD_IsActiveFlag_TxUND
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxUND(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TXUND) == UCPD_SR_TXUND) ? 1UL : 0UL);
}

/**
  * @brief  Check if hard reset sent interrupt
  * @rmtoll SR          HRSTSENT         LL_UCPD_IsActiveFlag_TxHRSTSENT
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxHRSTSENT(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_HRSTSENT) == UCPD_SR_HRSTSENT) ? 1UL : 0UL);
}

/**
  * @brief  Check if hard reset discard interrupt
  * @rmtoll SR          HRSTDISC         LL_UCPD_IsActiveFlag_TxHRSTDISC
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxHRSTDISC(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_HRSTDISC) == UCPD_SR_HRSTDISC) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx message abort interrupt
  * @rmtoll SR          TXMSGABT         LL_UCPD_IsActiveFlag_TxMSGABT
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxMSGABT(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TXMSGABT) == UCPD_SR_TXMSGABT) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx message sent interrupt
  * @rmtoll SR          TXMSGSENT         LL_UCPD_IsActiveFlag_TxMSGSENT
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxMSGSENT(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TXMSGSENT) == UCPD_SR_TXMSGSENT) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx message discarded interrupt
  * @rmtoll SR         TXMSGDISC         LL_UCPD_IsActiveFlag_TxMSGDISC
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxMSGDISC(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TXMSGDISC) == UCPD_SR_TXMSGDISC) ? 1UL : 0UL);
}

/**
  * @brief  Check if Tx data receive interrupt
  * @rmtoll SR          TXIS         LL_UCPD_IsActiveFlag_TxIS
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsActiveFlag_TxIS(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->SR, UCPD_SR_TXIS) == UCPD_SR_TXIS) ? 1UL : 0UL);
}

/**
  * @brief  return the vstate value for CC2
  * @rmtoll SR          TXIS         LL_UCPD_GetTypeCVstateCC2
  * @param  UCPDx UCPD Instance
  * @retval val
  */
__STATIC_INLINE uint32_t LL_UCPD_GetTypeCVstateCC2(UCPD_TypeDef const * const UCPDx)
{
  return UCPDx->SR & UCPD_SR_TYPEC_VSTATE_CC2;
}

/**
  * @brief  return the vstate value for CC1
  * @rmtoll SR          TXIS         LL_UCPD_GetTypeCVstateCC1
  * @param  UCPDx UCPD Instance
  * @retval val
  */
__STATIC_INLINE uint32_t LL_UCPD_GetTypeCVstateCC1(UCPD_TypeDef const * const UCPDx)
{
  return UCPDx->SR & UCPD_SR_TYPEC_VSTATE_CC1;
}

/**
  * @}
  */


/** @defgroup UCPD_LL_EF_DMA_Management DMA Management
  * @{
  */

/**
  * @brief  Rx DMA Enable
  * @rmtoll CFG1          RXDMAEN          LL_UCPD_RxDMAEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_RxDMAEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CFG1, UCPD_CFG1_RXDMAEN);
}

/**
  * @brief  Rx DMA Disable
  * @rmtoll CFG1          RXDMAEN          LL_UCPD_RxDMADisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_RxDMADisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CFG1, UCPD_CFG1_RXDMAEN);
}

/**
  * @brief  Tx DMA Enable
  * @rmtoll CFG1          TXDMAEN          LL_UCPD_TxDMAEnable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_TxDMAEnable(UCPD_TypeDef *UCPDx)
{
  SET_BIT(UCPDx->CFG1, UCPD_CFG1_TXDMAEN);
}

/**
  * @brief  Tx DMA Disable
  * @rmtoll CFG1          TXDMAEN          LL_UCPD_TxDMADisable
  * @param  UCPDx UCPD Instance
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_TxDMADisable(UCPD_TypeDef *UCPDx)
{
  CLEAR_BIT(UCPDx->CFG1, UCPD_CFG1_TXDMAEN);
}

/**
  * @brief  Check if DMA Tx is enabled
  * @rmtoll CR2          TXDMAEN       LL_UCPD_IsEnabledTxDMA
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnabledTxDMA(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->CFG1, UCPD_CFG1_TXDMAEN) == (UCPD_CFG1_TXDMAEN)) ? 1UL : 0UL);
}

/**
  * @brief  Check if DMA Rx is enabled
  * @rmtoll CR2          RXDMAEN       LL_UCPD_IsEnabledRxDMA
  * @param  UCPDx UCPD Instance
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_UCPD_IsEnabledRxDMA(UCPD_TypeDef const * const UCPDx)
{
  return ((READ_BIT(UCPDx->CFG1, UCPD_CFG1_RXDMAEN) == (UCPD_CFG1_RXDMAEN)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup UCPD_LL_EF_DATA_Management DATA Management
  * @{
  */

/**
  * @brief  write the orderset for Tx message
  * @rmtoll TX_ORDSET           TXORDSET            LL_UCPD_WriteTxOrderSet
  * @param  UCPDx UCPD Instance
  * @param  TxOrderSet one of the following value
  *         @arg @ref LL_UCPD_ORDERED_SET_SOP
  *         @arg @ref LL_UCPD_ORDERED_SET_SOP1
  *         @arg @ref LL_UCPD_ORDERED_SET_SOP2
  *         @arg @ref LL_UCPD_ORDERED_SET_HARD_RESET
  *         @arg @ref LL_UCPD_ORDERED_SET_CABLE_RESET
  *         @arg @ref LL_UCPD_ORDERED_SET_SOP1_DEBUG
  *         @arg @ref LL_UCPD_ORDERED_SET_SOP2_DEBUG
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_WriteTxOrderSet(UCPD_TypeDef *UCPDx, uint32_t TxOrderSet)
{
  WRITE_REG(UCPDx->TX_ORDSET, TxOrderSet);
}

/**
  * @brief  write the Tx paysize
  * @rmtoll TX_PAYSZ          TXPAYSZ            LL_UCPD_WriteTxPaySize
  * @param  UCPDx UCPD Instance
  * @param  TxPaySize
  * @retval None.
  */
__STATIC_INLINE void LL_UCPD_WriteTxPaySize(UCPD_TypeDef *UCPDx, uint32_t TxPaySize)
{
  WRITE_REG(UCPDx->TX_PAYSZ, TxPaySize);
}

/**
  * @brief  Write data
  * @rmtoll TXDR           DR            LL_UCPD_WriteData
  * @param  UCPDx UCPD Instance
  * @param  Data Value between Min_Data=0x00 and Max_Data=0xFF
  * @retval None.
  */
__STATIC_INLINE void LL_UCPD_WriteData(UCPD_TypeDef *UCPDx, uint8_t Data)
{
  WRITE_REG(UCPDx->TXDR, Data);
}

/**
  * @brief  read RX the orderset
  * @rmtoll RX_ORDSET           RXORDSET            LL_UCPD_ReadRxOrderSet
  * @param  UCPDx UCPD Instance
  * @retval RxOrderSet one of the following value
  *         @arg @ref LL_UCPD_RXORDSET_SOP
  *         @arg @ref LL_UCPD_RXORDSET_SOP1
  *         @arg @ref LL_UCPD_RXORDSET_SOP2
  *         @arg @ref LL_UCPD_RXORDSET_SOP1_DEBUG
  *         @arg @ref LL_UCPD_RXORDSET_SOP2_DEBUG
  *         @arg @ref LL_UCPD_RXORDSET_CABLE_RESET
  *         @arg @ref LL_UCPD_RXORDSET_SOPEXT1
  *         @arg @ref LL_UCPD_RXORDSET_SOPEXT2
  */
__STATIC_INLINE uint32_t LL_UCPD_ReadRxOrderSet(UCPD_TypeDef const * const UCPDx)
{
  return READ_BIT(UCPDx->RX_ORDSET, UCPD_RX_ORDSET_RXORDSET);
}

/**
  * @brief  Read the Rx paysize
  * @rmtoll TX_PAYSZ          TXPAYSZ            LL_UCPD_ReadRxPaySize
  * @param  UCPDx UCPD Instance
  * @retval RXPaysize.
  */
__STATIC_INLINE uint32_t LL_UCPD_ReadRxPaySize(UCPD_TypeDef const * const UCPDx)
{
  return READ_BIT(UCPDx->TX_PAYSZ, UCPD_RX_PAYSZ_RXPAYSZ);
}

/**
  * @brief  Read data
  * @rmtoll TXDR           RXDATA            LL_UCPD_ReadData
  * @param  UCPDx UCPD Instance
  * @retval RxData Value between Min_Data=0x00 and Max_Data=0xFF
  */
__STATIC_INLINE uint32_t LL_UCPD_ReadData(UCPD_TypeDef const * const UCPDx)
{
  return READ_REG(UCPDx->RXDR);
}

/**
  * @brief  Set Rx OrderSet Ext1
  * @rmtoll RX_ORDEXT1           RXSOPX1            LL_UCPD_SetRxOrdExt1
  * @param  UCPDx UCPD Instance
  * @param  SOPExt Value between Min_Data=0x00000 and Max_Data=0xFFFFF
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetRxOrdExt1(UCPD_TypeDef *UCPDx, uint32_t SOPExt)
{
  WRITE_REG(UCPDx->RX_ORDEXT1, SOPExt);
}

/**
  * @brief  Set Rx OrderSet Ext2
  * @rmtoll RX_ORDEXT2           RXSOPX2            LL_UCPD_SetRxOrdExt2
  * @param  UCPDx UCPD Instance
  * @param  SOPExt Value between Min_Data=0x00000 and Max_Data=0xFFFFF
  * @retval None
  */
__STATIC_INLINE void LL_UCPD_SetRxOrdExt2(UCPD_TypeDef *UCPDx, uint32_t SOPExt)
{
  WRITE_REG(UCPDx->RX_ORDEXT2, SOPExt);
}

/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup UCPD_LL_EF_Init Initialization and de-initialization functions
  * @{
  */

ErrorStatus LL_UCPD_DeInit(UCPD_TypeDef *UCPDx);
ErrorStatus LL_UCPD_Init(UCPD_TypeDef *UCPDx, LL_UCPD_InitTypeDef *UCPD_InitStruct);
void        LL_UCPD_StructInit(LL_UCPD_InitTypeDef *UCPD_InitStruct);

/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/**
  * @}
  */

#endif /* defined (UCPD1) || defined (UCPD2) */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_LL_UCPD_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
