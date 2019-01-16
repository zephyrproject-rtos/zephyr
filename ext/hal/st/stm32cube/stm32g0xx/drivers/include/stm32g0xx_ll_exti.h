/**
  ******************************************************************************
  * @file    stm32g0xx_ll_exti.h
  * @author  MCD Application Team
  * @brief   Header file of EXTI LL module.
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
#ifndef STM32G0xx_LL_EXTI_H
#define STM32G0xx_LL_EXTI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx.h"

/** @addtogroup STM32G0xx_LL_Driver
  * @{
  */

#if defined (EXTI)

/** @defgroup EXTI_LL EXTI
  * @{
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
#define LL_EXTI_REGISTER_PINPOS_SHFT        16u   /*!< Define used to shift pin position in EXTICR register */

/* Private Macros ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup EXTI_LL_Private_Macros EXTI Private Macros
  * @{
  */
/**
  * @}
  */
#endif /*USE_FULL_LL_DRIVER*/
/* Exported types ------------------------------------------------------------*/
#if defined(USE_FULL_LL_DRIVER)
/** @defgroup EXTI_LL_ES_INIT EXTI Exported Init structure
  * @{
  */
typedef struct
{

  uint32_t Line_0_31;           /*!< Specifies the EXTI lines to be enabled or disabled for Lines in range 0 to 31
                                     This parameter can be any combination of @ref EXTI_LL_EC_LINE */
#if defined(STM32G081xx) || defined(STM32G071xx)
  uint32_t Line_32_63;          /*!< Specifies the EXTI lines to be enabled or disabled for Lines in range 32 to 63
                                     This parameter can be any combination of @ref EXTI_LL_EC_LINE */
#endif
  FunctionalState LineCommand;  /*!< Specifies the new state of the selected EXTI lines.
                                     This parameter can be set either to ENABLE or DISABLE */

  uint8_t Mode;                 /*!< Specifies the mode for the EXTI lines.
                                     This parameter can be a value of @ref EXTI_LL_EC_MODE. */

  uint8_t Trigger;              /*!< Specifies the trigger signal active edge for the EXTI lines.
                                     This parameter can be a value of @ref EXTI_LL_EC_TRIGGER. */
} LL_EXTI_InitTypeDef;

/**
  * @}
  */
#endif /*USE_FULL_LL_DRIVER*/

/* Exported constants --------------------------------------------------------*/
/** @defgroup EXTI_LL_Exported_Constants EXTI Exported Constants
  * @{
  */

/** @defgroup EXTI_LL_EC_LINE LINE
  * @{
  */
#define LL_EXTI_LINE_0                 EXTI_IMR1_IM0           /*!< Extended line 0 */
#define LL_EXTI_LINE_1                 EXTI_IMR1_IM1           /*!< Extended line 1 */
#define LL_EXTI_LINE_2                 EXTI_IMR1_IM2           /*!< Extended line 2 */
#define LL_EXTI_LINE_3                 EXTI_IMR1_IM3           /*!< Extended line 3 */
#define LL_EXTI_LINE_4                 EXTI_IMR1_IM4           /*!< Extended line 4 */
#define LL_EXTI_LINE_5                 EXTI_IMR1_IM5           /*!< Extended line 5 */
#define LL_EXTI_LINE_6                 EXTI_IMR1_IM6           /*!< Extended line 6 */
#define LL_EXTI_LINE_7                 EXTI_IMR1_IM7           /*!< Extended line 7 */
#define LL_EXTI_LINE_8                 EXTI_IMR1_IM8           /*!< Extended line 8 */
#define LL_EXTI_LINE_9                 EXTI_IMR1_IM9           /*!< Extended line 9 */
#define LL_EXTI_LINE_10                EXTI_IMR1_IM10          /*!< Extended line 10 */
#define LL_EXTI_LINE_11                EXTI_IMR1_IM11          /*!< Extended line 11 */
#define LL_EXTI_LINE_12                EXTI_IMR1_IM12          /*!< Extended line 12 */
#define LL_EXTI_LINE_13                EXTI_IMR1_IM13          /*!< Extended line 13 */
#define LL_EXTI_LINE_14                EXTI_IMR1_IM14          /*!< Extended line 14 */
#define LL_EXTI_LINE_15                EXTI_IMR1_IM15          /*!< Extended line 15 */
#if defined(EXTI_IMR1_IM16)
#define LL_EXTI_LINE_16                EXTI_IMR1_IM16          /*!< Extended line 16 */
#endif
#define LL_EXTI_LINE_17                EXTI_IMR1_IM17          /*!< Extended line 17 */
#if defined(EXTI_IMR1_IM18)
#define LL_EXTI_LINE_18                EXTI_IMR1_IM18          /*!< Extended line 18 */
#endif
#define LL_EXTI_LINE_19                EXTI_IMR1_IM19          /*!< Extended line 19 */
#if defined(EXTI_IMR1_IM20)
#define LL_EXTI_LINE_20                EXTI_IMR1_IM20          /*!< Extended line 20 */
#endif
#if defined(EXTI_IMR1_IM21)
#define LL_EXTI_LINE_21                EXTI_IMR1_IM21          /*!< Extended line 21 */
#endif
#if defined(EXTI_IMR1_IM22)
#define LL_EXTI_LINE_22                EXTI_IMR1_IM22          /*!< Extended line 22 */
#endif
#define LL_EXTI_LINE_23                EXTI_IMR1_IM23          /*!< Extended line 23 */
#if defined(EXTI_IMR1_IM24)
#define LL_EXTI_LINE_24                EXTI_IMR1_IM24          /*!< Extended line 24 */
#endif
#if defined(EXTI_IMR1_IM25)
#define LL_EXTI_LINE_25                EXTI_IMR1_IM25          /*!< Extended line 25 */
#endif
#if defined(EXTI_IMR1_IM26)
#define LL_EXTI_LINE_26                EXTI_IMR1_IM26          /*!< Extended line 26 */
#endif
#if defined(EXTI_IMR1_IM27)
#define LL_EXTI_LINE_27                EXTI_IMR1_IM27          /*!< Extended line 27 */
#endif
#if defined(EXTI_IMR1_IM28)
#define LL_EXTI_LINE_28                EXTI_IMR1_IM28          /*!< Extended line 28 */
#endif
#if defined(EXTI_IMR1_IM29)
#define LL_EXTI_LINE_29                EXTI_IMR1_IM29          /*!< Extended line 29 */
#endif
#if defined(EXTI_IMR1_IM30)
#define LL_EXTI_LINE_30                EXTI_IMR1_IM30          /*!< Extended line 30 */
#endif
#if defined(EXTI_IMR1_IM31)
#define LL_EXTI_LINE_31                EXTI_IMR1_IM31          /*!< Extended line 31 */
#endif
#define LL_EXTI_LINE_ALL_0_31          EXTI_IMR1_IM            /*!< All Extended line not reserved*/

#if defined(EXTI_IMR2_IM32)
#define LL_EXTI_LINE_32                EXTI_IMR2_IM32          /*!< Extended line 32 */
#endif
#if defined(EXTI_IMR2_IM33)
#define LL_EXTI_LINE_33                EXTI_IMR2_IM33          /*!< Extended line 33 */
#endif
#if defined(EXTI_IMR2_IM32) || defined(EXTI_IMR2_IM33)
#define LL_EXTI_LINE_ALL_32_63         EXTI_IMR2_IM            /*!< All Extended line not reserved*/
#endif

#define LL_EXTI_LINE_ALL               0xFFFFFFFFU             /*!< All Extended line */

#if defined(USE_FULL_LL_DRIVER)
#define LL_EXTI_LINE_NONE              0x00000000U             /*!< None Extended line */
#endif /*USE_FULL_LL_DRIVER*/

/** @defgroup EXTI_LL_EC_CONFIG_PORT EXTI CONFIG PORT
  * @{
  */
#define LL_EXTI_CONFIG_PORTA               0U                                          /*!< EXTI PORT A */
#define LL_EXTI_CONFIG_PORTB               EXTI_EXTICR1_EXTI0_0                        /*!< EXTI PORT B */
#define LL_EXTI_CONFIG_PORTC               EXTI_EXTICR1_EXTI0_1                        /*!< EXTI PORT C */
#if defined(GPIOD_BASE)
#define LL_EXTI_CONFIG_PORTD               (EXTI_EXTICR1_EXTI0_1|EXTI_EXTICR1_EXTI0_0) /*!< EXTI PORT D */
#endif /*GPIOD_BASE*/
#if defined(GPIOE_BASE)
#define LL_EXTI_CONFIG_PORTE               EXTI_EXTICR1_EXTI0_2                        /*!< EXTI PORT E */
#endif /*GPIOE_BASE*/
#define LL_EXTI_CONFIG_PORTF               (EXTI_EXTICR1_EXTI0_2|EXTI_EXTICR1_EXTI0_0) /*!< EXTI PORT F */
/**
  * @}
  */

/** @defgroup EXTI_LL_EC_CONFIG_LINE EXTI CONFIG LINE
  * @{
  */
#define LL_EXTI_CONFIG_LINE0               ((0uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 0U)  /*!< EXTI_POSITION_0  | EXTICR[0] */
#define LL_EXTI_CONFIG_LINE1               ((8uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 0U)  /*!< EXTI_POSITION_8  | EXTICR[0] */
#define LL_EXTI_CONFIG_LINE2               ((16uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 0U)  /*!< EXTI_POSITION_16 | EXTICR[0] */
#define LL_EXTI_CONFIG_LINE3               ((24uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 0U)  /*!< EXTI_POSITION_24 | EXTICR[0] */
#define LL_EXTI_CONFIG_LINE4               ((0uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 1U)  /*!< EXTI_POSITION_0  | EXTICR[1] */
#define LL_EXTI_CONFIG_LINE5               ((8uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 1U)  /*!< EXTI_POSITION_8  | EXTICR[1] */
#define LL_EXTI_CONFIG_LINE6               ((16uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 1U)  /*!< EXTI_POSITION_16 | EXTICR[1] */
#define LL_EXTI_CONFIG_LINE7               ((24uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 1U)  /*!< EXTI_POSITION_24 | EXTICR[1] */
#define LL_EXTI_CONFIG_LINE8               ((0uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 2U)  /*!< EXTI_POSITION_0  | EXTICR[2] */
#define LL_EXTI_CONFIG_LINE9               ((8uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 2U)  /*!< EXTI_POSITION_8  | EXTICR[2] */
#define LL_EXTI_CONFIG_LINE10              ((16uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 2U)  /*!< EXTI_POSITION_16 | EXTICR[2] */
#define LL_EXTI_CONFIG_LINE11              ((24uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 2U)  /*!< EXTI_POSITION_24 | EXTICR[2] */
#define LL_EXTI_CONFIG_LINE12              ((0uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 3U)  /*!< EXTI_POSITION_0  | EXTICR[3] */
#define LL_EXTI_CONFIG_LINE13              ((8uL << LL_EXTI_REGISTER_PINPOS_SHFT)  | 3U)  /*!< EXTI_POSITION_8  | EXTICR[3] */
#define LL_EXTI_CONFIG_LINE14              ((16uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 3U)  /*!< EXTI_POSITION_16 | EXTICR[3] */
#define LL_EXTI_CONFIG_LINE15              ((24uL << LL_EXTI_REGISTER_PINPOS_SHFT) | 3U)  /*!< EXTI_POSITION_24 | EXTICR[3] */
/**
  * @}
  */
/**
  * @}
  */
#if defined(USE_FULL_LL_DRIVER)

/** @defgroup EXTI_LL_EC_MODE Mode
  * @{
  */
#define LL_EXTI_MODE_IT                 ((uint8_t)0x00U) /*!< Interrupt Mode */
#define LL_EXTI_MODE_EVENT              ((uint8_t)0x01U) /*!< Event Mode */
#define LL_EXTI_MODE_IT_EVENT           ((uint8_t)0x02U) /*!< Interrupt & Event Mode */
/**
  * @}
  */

/** @defgroup EXTI_LL_EC_TRIGGER Edge Trigger
  * @{
  */
#define LL_EXTI_TRIGGER_NONE            ((uint8_t)0x00U) /*!< No Trigger Mode */
#define LL_EXTI_TRIGGER_RISING          ((uint8_t)0x01U) /*!< Trigger Rising Mode */
#define LL_EXTI_TRIGGER_FALLING         ((uint8_t)0x02U) /*!< Trigger Falling Mode */
#define LL_EXTI_TRIGGER_RISING_FALLING  ((uint8_t)0x03U) /*!< Trigger Rising & Falling Mode */

/**
  * @}
  */


#endif /*USE_FULL_LL_DRIVER*/


/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup EXTI_LL_Exported_Macros EXTI Exported Macros
  * @{
  */

/** @defgroup EXTI_LL_EM_WRITE_READ Common Write and read registers Macros
  * @{
  */

/**
  * @brief  Write a value in EXTI register
  * @param  __REG__ Register to be written
  * @param  __VALUE__ Value to be written in the register
  * @retval None
  */
#define LL_EXTI_WriteReg(__REG__, __VALUE__) WRITE_REG(EXTI->__REG__, (__VALUE__))

/**
  * @brief  Read a value in EXTI register
  * @param  __REG__ Register to be read
  * @retval Register value
  */
#define LL_EXTI_ReadReg(__REG__) READ_REG(EXTI->__REG__)
/**
  * @}
  */


/**
  * @}
  */



/* Exported functions --------------------------------------------------------*/
/** @defgroup EXTI_LL_Exported_Functions EXTI Exported Functions
 * @{
 */
/** @defgroup EXTI_LL_EF_IT_Management IT_Management
  * @{
  */

/**
  * @brief  Enable ExtiLine Interrupt request for Lines in range 0 to 31
  * @note The reset value for the direct or internal lines (see RM)
  *       is set to 1 in order to enable the interrupt by default.
  *       Bits are set automatically at Power on.
  * @rmtoll IMR1         IMx           LL_EXTI_EnableIT_0_31
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  *         @arg @ref LL_EXTI_LINE_19
  *         @arg @ref LL_EXTI_LINE_21
  *         @arg @ref LL_EXTI_LINE_23
  *         @arg @ref LL_EXTI_LINE_25
  *         @arg @ref LL_EXTI_LINE_26
  *         @arg @ref LL_EXTI_LINE_27
  *         @arg @ref LL_EXTI_LINE_28
  *         @arg @ref LL_EXTI_LINE_29
  *         @arg @ref LL_EXTI_LINE_30
  *         @arg @ref LL_EXTI_LINE_31
  *         @arg @ref LL_EXTI_LINE_ALL_0_31
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_EnableIT_0_31(uint32_t ExtiLine)
{
  SET_BIT(EXTI->IMR1, ExtiLine);
}
#if defined(STM32G081xx) || defined(STM32G071xx)
/**
  * @brief  Enable ExtiLine Interrupt request for Lines in range 32 to 63
  * @note The reset value for the direct lines (lines 32 & 33)
  *       is set to 1 in order to enable the interrupt by default.
  *       Bits are set automatically at Power on.
  * @rmtoll IMR2         IMx           LL_EXTI_EnableIT_32_63
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_32
  *         @arg @ref LL_EXTI_LINE_33
  *         @arg @ref LL_EXTI_LINE_ALL_32_63
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_EnableIT_32_63(uint32_t ExtiLine)
{
  SET_BIT(EXTI->IMR2, ExtiLine);
}
#endif
/**
  * @brief  Disable ExtiLine Interrupt request for Lines in range 0 to 31
  * @note The reset value for the direct or internal lines (see RM)
  *       is set to 1 in order to enable the interrupt by default.
  *       Bits are set automatically at Power on.
  * @rmtoll IMR1         IMx           LL_EXTI_DisableIT_0_31
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  *         @arg @ref LL_EXTI_LINE_19
  *         @arg @ref LL_EXTI_LINE_21
  *         @arg @ref LL_EXTI_LINE_23
  *         @arg @ref LL_EXTI_LINE_25
  *         @arg @ref LL_EXTI_LINE_26
  *         @arg @ref LL_EXTI_LINE_27
  *         @arg @ref LL_EXTI_LINE_28
  *         @arg @ref LL_EXTI_LINE_29
  *         @arg @ref LL_EXTI_LINE_30
  *         @arg @ref LL_EXTI_LINE_31
  *         @arg @ref LL_EXTI_LINE_ALL_0_31
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_DisableIT_0_31(uint32_t ExtiLine)
{
  CLEAR_BIT(EXTI->IMR1, ExtiLine);
}

#if defined(STM32G081xx) || defined(STM32G071xx)
/**
  * @brief  Disable ExtiLine Interrupt request for Lines in range 32 to 63
  * @note The reset value for the direct lines (lines 32 & 33)
  *       is set to 1 in order to enable the interrupt by default.
  *       Bits are set automatically at Power on.
  * @rmtoll IMR2         IMx           LL_EXTI_DisableIT_32_63
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_32
  *         @arg @ref LL_EXTI_LINE_33
  *         @arg @ref LL_EXTI_LINE_ALL_32_63
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_DisableIT_32_63(uint32_t ExtiLine)
{
  CLEAR_BIT(EXTI->IMR2, ExtiLine);
}
#endif

/**
  * @brief  Indicate if ExtiLine Interrupt request is enabled for Lines in range 0 to 31
  * @note The reset value for the direct or internal lines (see RM)
  *       is set to 1 in order to enable the interrupt by default.
  *       Bits are set automatically at Power on.
  * @rmtoll IMR1         IMx           LL_EXTI_IsEnabledIT_0_31
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  *         @arg @ref LL_EXTI_LINE_19
  *         @arg @ref LL_EXTI_LINE_21
  *         @arg @ref LL_EXTI_LINE_23
  *         @arg @ref LL_EXTI_LINE_25
  *         @arg @ref LL_EXTI_LINE_26
  *         @arg @ref LL_EXTI_LINE_27
  *         @arg @ref LL_EXTI_LINE_28
  *         @arg @ref LL_EXTI_LINE_29
  *         @arg @ref LL_EXTI_LINE_30
  *         @arg @ref LL_EXTI_LINE_31
  *         @arg @ref LL_EXTI_LINE_ALL_0_31
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsEnabledIT_0_31(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->IMR1, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}

#if defined(STM32G081xx) || defined(STM32G071xx)
/**
  * @brief  Indicate if ExtiLine Interrupt request is enabled for Lines in range 32 to 63
  * @note The reset value for the direct lines (lines 32 & 33)
  *       is set to 1 in order to enable the interrupt by default.
  * @rmtoll IMR2         IMx           LL_EXTI_IsEnabledIT_32_63
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_32
  *         @arg @ref LL_EXTI_LINE_33
  *         @arg @ref LL_EXTI_LINE_ALL_32_63
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsEnabledIT_32_63(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->IMR2, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}
#endif

/**
  * @}
  */

/** @defgroup EXTI_LL_EF_Event_Management Event_Management
  * @{
  */

/**
  * @brief  Enable ExtiLine Event request for Lines in range 0 to 31
  * @rmtoll EMR1         EMx           LL_EXTI_EnableEvent_0_31
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  *         @arg @ref LL_EXTI_LINE_19
  *         @arg @ref LL_EXTI_LINE_21
  *         @arg @ref LL_EXTI_LINE_23
  *         @arg @ref LL_EXTI_LINE_25
  *         @arg @ref LL_EXTI_LINE_26
  *         @arg @ref LL_EXTI_LINE_27
  *         @arg @ref LL_EXTI_LINE_28
  *         @arg @ref LL_EXTI_LINE_29
  *         @arg @ref LL_EXTI_LINE_30
  *         @arg @ref LL_EXTI_LINE_31
  *         @arg @ref LL_EXTI_LINE_ALL_0_31
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_EnableEvent_0_31(uint32_t ExtiLine)
{
  SET_BIT(EXTI->EMR1, ExtiLine);

}

#if defined(STM32G081xx) || defined(STM32G071xx)
/**
  * @brief  Enable ExtiLine Event request for Lines in range 32 to 63
  * @rmtoll EMR2         EMx           LL_EXTI_EnableEvent_32_63
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_32
  *         @arg @ref LL_EXTI_LINE_33
  *         @arg @ref LL_EXTI_LINE_ALL_32_63
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_EnableEvent_32_63(uint32_t ExtiLine)
{
  SET_BIT(EXTI->EMR2, ExtiLine);
}
#endif
/**
  * @brief  Disable ExtiLine Event request for Lines in range 0 to 31
  * @rmtoll EMR1         EMx           LL_EXTI_DisableEvent_0_31
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  *         @arg @ref LL_EXTI_LINE_19
  *         @arg @ref LL_EXTI_LINE_21
  *         @arg @ref LL_EXTI_LINE_23
  *         @arg @ref LL_EXTI_LINE_25
  *         @arg @ref LL_EXTI_LINE_26
  *         @arg @ref LL_EXTI_LINE_27
  *         @arg @ref LL_EXTI_LINE_28
  *         @arg @ref LL_EXTI_LINE_29
  *         @arg @ref LL_EXTI_LINE_30
  *         @arg @ref LL_EXTI_LINE_31
  *         @arg @ref LL_EXTI_LINE_ALL_0_31
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_DisableEvent_0_31(uint32_t ExtiLine)
{
  CLEAR_BIT(EXTI->EMR1, ExtiLine);
}

#if defined(STM32G081xx) || defined(STM32G071xx)
/**
  * @brief  Disable ExtiLine Event request for Lines in range 32 to 63
  * @rmtoll EMR2         EMx           LL_EXTI_DisableEvent_32_63
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_32
  *         @arg @ref LL_EXTI_LINE_33
  *         @arg @ref LL_EXTI_LINE_ALL_32_63
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_DisableEvent_32_63(uint32_t ExtiLine)
{
  CLEAR_BIT(EXTI->EMR2, ExtiLine);
}
#endif
/**
  * @brief  Indicate if ExtiLine Event request is enabled for Lines in range 0 to 31
  * @rmtoll EMR1         EMx           LL_EXTI_IsEnabledEvent_0_31
  * @param  ExtiLine This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  *         @arg @ref LL_EXTI_LINE_19
  *         @arg @ref LL_EXTI_LINE_21
  *         @arg @ref LL_EXTI_LINE_23
  *         @arg @ref LL_EXTI_LINE_25
  *         @arg @ref LL_EXTI_LINE_26
  *         @arg @ref LL_EXTI_LINE_27
  *         @arg @ref LL_EXTI_LINE_28
  *         @arg @ref LL_EXTI_LINE_29
  *         @arg @ref LL_EXTI_LINE_30
  *         @arg @ref LL_EXTI_LINE_31
  *         @arg @ref LL_EXTI_LINE_ALL_0_31
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsEnabledEvent_0_31(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->EMR1, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}

#if defined(STM32G081xx) || defined(STM32G071xx)
/**
  * @brief  Indicate if ExtiLine Event request is enabled for Lines in range 32 to 63
  * @rmtoll EMR2         EMx           LL_EXTI_IsEnabledEvent_32_63
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_32
  *         @arg @ref LL_EXTI_LINE_33
  *         @arg @ref LL_EXTI_LINE_ALL_32_63
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsEnabledEvent_32_63(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->EMR2, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}
#endif

/**
  * @}
  */

/** @defgroup EXTI_LL_EF_Rising_Trigger_Management Rising_Trigger_Management
  * @{
  */

/**
  * @brief  Enable ExtiLine Rising Edge Trigger for Lines in range 0 to 31
  * @note The configurable wakeup lines are edge-triggered. No glitch must be
  *       generated on these lines. If a rising edge on a configurable interrupt
  *       line occurs during a write operation in the EXTI_RTSR register, the
  *       pending bit is not set.
  *       Rising and falling edge triggers can be set for
  *       the same interrupt line. In this case, both generate a trigger
  *       condition.
  * @rmtoll RTSR1        RTx           LL_EXTI_EnableRisingTrig_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_EnableRisingTrig_0_31(uint32_t ExtiLine)
{
  SET_BIT(EXTI->RTSR1, ExtiLine);

}

/**
  * @brief  Disable ExtiLine Rising Edge Trigger for Lines in range 0 to 31
  * @note The configurable wakeup lines are edge-triggered. No glitch must be
  *       generated on these lines. If a rising edge on a configurable interrupt
  *       line occurs during a write operation in the EXTI_RTSR register, the
  *       pending bit is not set.
  *       Rising and falling edge triggers can be set for
  *       the same interrupt line. In this case, both generate a trigger
  *       condition.
  * @rmtoll RTSR1        RTx           LL_EXTI_DisableRisingTrig_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_DisableRisingTrig_0_31(uint32_t ExtiLine)
{
  CLEAR_BIT(EXTI->RTSR1, ExtiLine);

}

/**
  * @brief  Check if rising edge trigger is enabled for Lines in range 0 to 31
  * @rmtoll RTSR1        RTx           LL_EXTI_IsEnabledRisingTrig_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsEnabledRisingTrig_0_31(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->RTSR1, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup EXTI_LL_EF_Falling_Trigger_Management Falling_Trigger_Management
  * @{
  */

/**
  * @brief  Enable ExtiLine Falling Edge Trigger for Lines in range 0 to 31
  * @note The configurable wakeup lines are edge-triggered. No glitch must be
  *       generated on these lines. If a falling edge on a configurable interrupt
  *       line occurs during a write operation in the EXTI_FTSR register, the
  *       pending bit is not set.
  *       Rising and falling edge triggers can be set for
  *       the same interrupt line. In this case, both generate a trigger
  *       condition.
  * @rmtoll FTSR1        FTx           LL_EXTI_EnableFallingTrig_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_EnableFallingTrig_0_31(uint32_t ExtiLine)
{
  SET_BIT(EXTI->FTSR1, ExtiLine);
}

/**
  * @brief  Disable ExtiLine Falling Edge Trigger for Lines in range 0 to 31
  * @note The configurable wakeup lines are edge-triggered. No glitch must be
  *       generated on these lines. If a Falling edge on a configurable interrupt
  *       line occurs during a write operation in the EXTI_FTSR register, the
  *       pending bit is not set.
  *       Rising and falling edge triggers can be set for the same interrupt line.
  *       In this case, both generate a trigger condition.
  * @rmtoll FTSR1        FTx           LL_EXTI_DisableFallingTrig_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_DisableFallingTrig_0_31(uint32_t ExtiLine)
{
  CLEAR_BIT(EXTI->FTSR1, ExtiLine);
}

/**
  * @brief  Check if falling edge trigger is enabled for Lines in range 0 to 31
  * @rmtoll FTSR1        FTx           LL_EXTI_IsEnabledFallingTrig_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsEnabledFallingTrig_0_31(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->FTSR1, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}

/**
  * @}
  */

/** @defgroup EXTI_LL_EF_Software_Interrupt_Management Software_Interrupt_Management
  * @{
  */

/**
  * @brief  Generate a software Interrupt Event for Lines in range 0 to 31
  * @note If the interrupt is enabled on this line in the EXTI_IMR, writing a 1 to
  *       this bit when it is at '0' sets the corresponding pending bit in EXTI_PR
  *       resulting in an interrupt request generation.
  *       This bit is cleared by clearing the corresponding bit in the EXTI_PR
  *       register (by writing a 1 into the bit)
  * @rmtoll SWIER1       SWIx          LL_EXTI_GenerateSWI_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_GenerateSWI_0_31(uint32_t ExtiLine)
{
  SET_BIT(EXTI->SWIER1, ExtiLine);
}

/**
  * @}
  */

/** @defgroup EXTI_LL_EF_Flag_Management Flag_Management
  * @{
  */

/**
  * @brief  Check if the ExtLine Falling Flag is set or not for Lines in range 0 to 31
  * @note This bit is set when the falling edge event arrives on the interrupt
  *       line. This bit is cleared by writing a 1 to the bit.
  * @rmtoll FPR1          FPIFx           LL_EXTI_IsActiveFallingFlag_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsActiveFallingFlag_0_31(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->FPR1, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}

/**
  * @brief  Read ExtLine Combination Falling Flag for Lines in range 0 to 31
  * @note This bit is set when the falling edge event arrives on the interrupt
  *       line. This bit is cleared by writing a 1 to the bit.
  * @rmtoll FPR1          FPIFx           LL_EXTI_ReadFallingFlag_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval @note This bit is set when the selected edge event arrives on the interrupt
  */
__STATIC_INLINE uint32_t LL_EXTI_ReadFallingFlag_0_31(uint32_t ExtiLine)
{
  return (READ_BIT(EXTI->FPR1, ExtiLine));
}

/**
  * @brief  Clear ExtLine Falling Flags  for Lines in range 0 to 31
  * @note This bit is set when the falling edge event arrives on the interrupt
  *       line. This bit is cleared by writing a 1 to the bit.
  * @rmtoll FPR1          FPIFx           LL_EXTI_ClearFallingFlag_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_ClearFallingFlag_0_31(uint32_t ExtiLine)
{
  WRITE_REG(EXTI->FPR1, ExtiLine);
}

/**
  * @brief  Check if the ExtLine Rising Flag is set or not for Lines in range 0 to 31
  * @note This bit is set when the Rising edge event arrives on the interrupt
  *       line. This bit is cleared by writing a 1 to the bit.
  * @rmtoll RPR1          RPIFx           LL_EXTI_IsActiveRisingFlag_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval State of bit (1 or 0).
  */
__STATIC_INLINE uint32_t LL_EXTI_IsActiveRisingFlag_0_31(uint32_t ExtiLine)
{
  return ((READ_BIT(EXTI->RPR1, ExtiLine) == (ExtiLine)) ? 1UL : 0UL);
}

/**
  * @brief  Read ExtLine Combination Rising Flag for Lines in range 0 to 31
  * @note This bit is set when the Rising edge event arrives on the interrupt
  *       line. This bit is cleared by writing a 1 to the bit.
  * @rmtoll RPR1          RPIFx           LL_EXTI_ReadRisingFlag_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval @note This bit is set when the selected edge event arrives on the interrupt
  */
__STATIC_INLINE uint32_t LL_EXTI_ReadRisingFlag_0_31(uint32_t ExtiLine)
{
  return (READ_BIT(EXTI->RPR1, ExtiLine));
}

/**
  * @brief  Clear ExtLine Rising Flags  for Lines in range 0 to 31
  * @note This bit is set when the Rising edge event arrives on the interrupt
  *       line. This bit is cleared by writing a 1 to the bit.
  * @rmtoll RPR1          RPIFx           LL_EXTI_ClearRisingFlag_0_31
  * @param  ExtiLine This parameter can be a combination of the following values:
  *         @arg @ref LL_EXTI_LINE_0
  *         @arg @ref LL_EXTI_LINE_1
  *         @arg @ref LL_EXTI_LINE_2
  *         @arg @ref LL_EXTI_LINE_3
  *         @arg @ref LL_EXTI_LINE_4
  *         @arg @ref LL_EXTI_LINE_5
  *         @arg @ref LL_EXTI_LINE_6
  *         @arg @ref LL_EXTI_LINE_7
  *         @arg @ref LL_EXTI_LINE_8
  *         @arg @ref LL_EXTI_LINE_9
  *         @arg @ref LL_EXTI_LINE_10
  *         @arg @ref LL_EXTI_LINE_11
  *         @arg @ref LL_EXTI_LINE_12
  *         @arg @ref LL_EXTI_LINE_13
  *         @arg @ref LL_EXTI_LINE_14
  *         @arg @ref LL_EXTI_LINE_15
  *         @arg @ref LL_EXTI_LINE_16
  *         @arg @ref LL_EXTI_LINE_17
  *         @arg @ref LL_EXTI_LINE_18
  * @note   Please check each device line mapping for EXTI Line availability
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_ClearRisingFlag_0_31(uint32_t ExtiLine)
{
  WRITE_REG(EXTI->RPR1, ExtiLine);
}

/**
  * @}
  */
/** @defgroup EXTI_LL_EF_Config EF configuration functions
  * @{
  */

/**
  * @brief  Configure source input for the EXTI external interrupt.
  * @rmtoll EXTI_EXTICR1 EXTI0         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR1 EXTI1         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR1 EXTI2         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR1 EXTI3         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI4         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI5         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI6         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI7         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI8         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI9         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI10        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI11        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI12        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI13        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI14        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI15        LL_EXTI_SetEXTISource
  * @param  Port This parameter can be one of the following values:
  *         @arg @ref EXTI_LL_EC_CONFIG_PORT
  *
  *         (*) value not defined in all devices
  * @param  Line This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_CONFIG_LINE0
  *         @arg @ref LL_EXTI_CONFIG_LINE1
  *         @arg @ref LL_EXTI_CONFIG_LINE2
  *         @arg @ref LL_EXTI_CONFIG_LINE3
  *         @arg @ref LL_EXTI_CONFIG_LINE4
  *         @arg @ref LL_EXTI_CONFIG_LINE5
  *         @arg @ref LL_EXTI_CONFIG_LINE6
  *         @arg @ref LL_EXTI_CONFIG_LINE7
  *         @arg @ref LL_EXTI_CONFIG_LINE8
  *         @arg @ref LL_EXTI_CONFIG_LINE9
  *         @arg @ref LL_EXTI_CONFIG_LINE10
  *         @arg @ref LL_EXTI_CONFIG_LINE11
  *         @arg @ref LL_EXTI_CONFIG_LINE12
  *         @arg @ref LL_EXTI_CONFIG_LINE13
  *         @arg @ref LL_EXTI_CONFIG_LINE14
  *         @arg @ref LL_EXTI_CONFIG_LINE15
  * @retval None
  */
__STATIC_INLINE void LL_EXTI_SetEXTISource(uint32_t Port, uint32_t Line)
{
  MODIFY_REG(EXTI->EXTICR[Line & 0x03u], EXTI_EXTICR1_EXTI0 << (Line >> LL_EXTI_REGISTER_PINPOS_SHFT), Port << (Line >> LL_EXTI_REGISTER_PINPOS_SHFT));
}

/**
  * @brief  Get the configured defined for specific EXTI Line
  * @rmtoll EXTI_EXTICR1 EXTI0         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR1 EXTI1         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR1 EXTI2         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR1 EXTI3         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI4         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI5         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI6         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR2 EXTI7         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI8         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI9         LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI10        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR3 EXTI11        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI12        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI13        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI14        LL_EXTI_SetEXTISource\n
  *         EXTI_EXTICR4 EXTI15        LL_EXTI_SetEXTISource
  * @param  Line This parameter can be one of the following values:
  *         @arg @ref LL_EXTI_CONFIG_LINE0
  *         @arg @ref LL_EXTI_CONFIG_LINE1
  *         @arg @ref LL_EXTI_CONFIG_LINE2
  *         @arg @ref LL_EXTI_CONFIG_LINE3
  *         @arg @ref LL_EXTI_CONFIG_LINE4
  *         @arg @ref LL_EXTI_CONFIG_LINE5
  *         @arg @ref LL_EXTI_CONFIG_LINE6
  *         @arg @ref LL_EXTI_CONFIG_LINE7
  *         @arg @ref LL_EXTI_CONFIG_LINE8
  *         @arg @ref LL_EXTI_CONFIG_LINE9
  *         @arg @ref LL_EXTI_CONFIG_LINE10
  *         @arg @ref LL_EXTI_CONFIG_LINE11
  *         @arg @ref LL_EXTI_CONFIG_LINE12
  *         @arg @ref LL_EXTI_CONFIG_LINE13
  *         @arg @ref LL_EXTI_CONFIG_LINE14
  *         @arg @ref LL_EXTI_CONFIG_LINE15
  * @retval Returned value can be one of the following values:
  *         @arg @ref EXTI_LL_EC_CONFIG_PORT
  *
  *         (*) value not defined in all devices
  */
__STATIC_INLINE uint32_t LL_EXTI_GetEXTISource(uint32_t Line)
{
  return (READ_BIT(EXTI->EXTICR[Line & 0x03u], (EXTI_EXTICR1_EXTI0 << (Line >> LL_EXTI_REGISTER_PINPOS_SHFT))) >> (Line >> LL_EXTI_REGISTER_PINPOS_SHFT));
}
/**
  * @}
  */

#if defined(USE_FULL_LL_DRIVER)
/** @defgroup EXTI_LL_EF_Init Initialization and de-initialization functions
  * @{
  */

uint32_t LL_EXTI_Init(LL_EXTI_InitTypeDef *EXTI_InitStruct);
uint32_t LL_EXTI_DeInit(void);
void LL_EXTI_StructInit(LL_EXTI_InitTypeDef *EXTI_InitStruct);


/**
  * @}
  */
#endif /* USE_FULL_LL_DRIVER */

/**
  * @}
  */

/**
  * @}
  */

#endif /* EXTI */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* STM32G0xx_LL_EXTI_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
