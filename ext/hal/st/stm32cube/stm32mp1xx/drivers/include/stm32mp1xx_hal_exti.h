/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_exti.h
  * @author  MCD Application Team
  * @brief   Header file of EXTI HAL module.
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
#ifndef STM32MP1xx_HAL_EXTI_H
#define STM32MP1xx_HAL_EXTI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @defgroup EXTI EXTI
  * @brief EXTI HAL module driver
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup EXTI_Exported_Types EXTI Exported Types
  * @{
  */
typedef enum
{
  HAL_EXTI_COMMON_CB_ID          = 0x00U,
  HAL_EXTI_RISING_CB_ID          = 0x01U,
  HAL_EXTI_FALLING_CB_ID         = 0x02U,
} EXTI_CallbackIDTypeDef;


/**
  * @brief  EXTI Handle structure definition
  */
typedef struct
{
  uint32_t Line;                    /*!<  Exti line number */
  void (* RisingCallback)(void);    /*!<  Exti rising callback */
  void (* FallingCallback)(void);   /*!<  Exti falling callback */
} EXTI_HandleTypeDef;

/**
  * @brief  EXTI Configuration structure definition
  */
typedef struct
{
  uint32_t Line;      /*!< The Exti line to be configured. This parameter
                           can be a value of @ref EXTI_Line */
  uint32_t Mode;      /*!< The Exit Mode to be configured for a core.
                           This parameter can be a combination of @ref EXTI_Mode */
  uint32_t Trigger;   /*!< The Exti Trigger to be configured. This parameter
                           can be a value of @ref EXTI_Trigger */
  uint32_t GPIOSel;   /*!< The Exti GPIO multiplexer selection to be configured.
                           This parameter is only possible for line 0 to 15. It
                           can be a value of @ref EXTI_GPIOSel */
} EXTI_ConfigTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup EXTI_Exported_Constants EXTI Exported Constants
  * @{
  */

/** @defgroup EXTI_Line  EXTI Line
  * @{
  */
#define EXTI_LINE_0                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x00u)  /* EXTI_GPIO */
#define EXTI_LINE_1                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x01u)  /* EXTI_GPIO */
#define EXTI_LINE_2                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x02u)  /* EXTI_GPIO */
#define EXTI_LINE_3                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x03u)  /* EXTI_GPIO */
#define EXTI_LINE_4                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x04u)  /* EXTI_GPIO */
#define EXTI_LINE_5                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x05u)  /* EXTI_GPIO */
#define EXTI_LINE_6                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x06u)  /* EXTI_GPIO */
#define EXTI_LINE_7                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x07u)  /* EXTI_GPIO */
#define EXTI_LINE_8                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x08u)  /* EXTI_GPIO */
#define EXTI_LINE_9                         (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x09u)  /* EXTI_GPIO */
#define EXTI_LINE_10                        (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x0Au)  /* EXTI_GPIO */
#define EXTI_LINE_11                        (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x0Bu)  /* EXTI_GPIO */
#define EXTI_LINE_12                        (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x0Cu)  /* EXTI_GPIO */
#define EXTI_LINE_13                        (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x0Du)  /* EXTI_GPIO */
#define EXTI_LINE_14                        (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x0Eu)  /* EXTI_GPIO */
#define EXTI_LINE_15                        (EXTI_GPIO     | EXTI_EVENT | EXTI_REG1 | 0x0Fu)  /* EXTI_GPIO */
#define EXTI_LINE_16                        (EXTI_CONFIG   |              EXTI_REG1 | 0x10u)  /* PVD and AVD */
#define EXTI_LINE_17                        (EXTI_CONFIG   | EXTI_EVENT | EXTI_REG1 | 0x11u)  /* RTC timestamp and SecureError wakeup */
#define EXTI_LINE_18                        (EXTI_CONFIG   | EXTI_EVENT | EXTI_REG1 | 0x12u)  /* TAMP tamper and SecureError wakeup */
#define EXTI_LINE_19                        (EXTI_CONFIG   | EXTI_EVENT | EXTI_REG1 | 0x13u)  /* RTC Wakeup timer and Alarms (A and B) and SecureError wakeup */
#define EXTI_LINE_20                        (EXTI_RESERVED |            | EXTI_REG1 | 0x14u)  /* RESERVED */
#define EXTI_LINE_21                        (EXTI_DIRECT   |              EXTI_REG1 | 0x15u)  /* I2C1 wakeup */
#define EXTI_LINE_22                        (EXTI_DIRECT   |              EXTI_REG1 | 0x16u)  /* I2C2 wakeup */
#define EXTI_LINE_23                        (EXTI_DIRECT   |              EXTI_REG1 | 0x17u)  /* I2C3 wakeup */
#define EXTI_LINE_24                        (EXTI_DIRECT   |              EXTI_REG1 | 0x18u)  /* I2C4 wakeup */
#define EXTI_LINE_25                        (EXTI_DIRECT   |              EXTI_REG1 | 0x19u)  /* I2C5 wakeup */
#define EXTI_LINE_26                        (EXTI_DIRECT   |              EXTI_REG1 | 0x1Au)  /* USART1 wakeup */
#define EXTI_LINE_27                        (EXTI_DIRECT   |              EXTI_REG1 | 0x1Bu)  /* USART2 wakeup */
#define EXTI_LINE_28                        (EXTI_DIRECT   |              EXTI_REG1 | 0x1Cu)  /* USART3 wakeup */
#define EXTI_LINE_29                        (EXTI_DIRECT   |              EXTI_REG1 | 0x1Du)  /* USART6 wakeup */
#define EXTI_LINE_30                        (EXTI_DIRECT   |              EXTI_REG1 | 0x1Eu)  /* UART4 wakeup */
#define EXTI_LINE_31                        (EXTI_DIRECT   |              EXTI_REG1 | 0x1Fu)  /* UART5 wakeup */
#define EXTI_LINE_32                        (EXTI_DIRECT   |              EXTI_REG2 | 0x00u)  /* UART7 wakeup */
#define EXTI_LINE_33                        (EXTI_DIRECT   |              EXTI_REG2 | 0x01u)  /* UART8 wakeup */
#define EXTI_LINE_34                        (EXTI_RESERVED |              EXTI_REG2 | 0x02u)  /* RESERVED */
#define EXTI_LINE_35                        (EXTI_RESERVED |              EXTI_REG2 | 0x03u)  /* RESERVED */
#define EXTI_LINE_36                        (EXTI_DIRECT   |              EXTI_REG2 | 0x04u)  /* SPI1 wakeup */
#define EXTI_LINE_37                        (EXTI_DIRECT   |              EXTI_REG2 | 0x05u)  /* SPI2 wakeup */
#define EXTI_LINE_38                        (EXTI_DIRECT   |              EXTI_REG2 | 0x06u)  /* SPI3 wakeup */
#define EXTI_LINE_39                        (EXTI_DIRECT   |              EXTI_REG2 | 0x07u)  /* SPI4 wakeup */
#define EXTI_LINE_40                        (EXTI_DIRECT   |              EXTI_REG2 | 0x08u)  /* SPI5 wakeup */
#define EXTI_LINE_41                        (EXTI_DIRECT   |              EXTI_REG2 | 0x09u)  /* SPI6 wakeup */
#define EXTI_LINE_42                        (EXTI_DIRECT   |              EXTI_REG2 | 0x0Au)  /* MDIOS wakeup */
#define EXTI_LINE_43                        (EXTI_DIRECT   |              EXTI_REG2 | 0x0Bu)  /* USBH wakeup */
#define EXTI_LINE_44                        (EXTI_DIRECT   |              EXTI_REG2 | 0x0Cu)  /* OTG wakeup */
#define EXTI_LINE_45                        (EXTI_DIRECT   |              EXTI_REG2 | 0x0Du)  /* IWDG1 early wake */
#define EXTI_LINE_46                        (EXTI_DIRECT   |              EXTI_REG2 | 0x0Eu)  /* IWDG1 early wake */
#define EXTI_LINE_47                        (EXTI_DIRECT   |              EXTI_REG2 | 0x0Fu)  /* LPTIM1 wakeup */
#define EXTI_LINE_48                        (EXTI_DIRECT   |              EXTI_REG2 | 0x10u)  /* LPTIM2 wakeup */
#define EXTI_LINE_49                        (EXTI_RESERVED |              EXTI_REG2 | 0x11u)  /* RESERVED */
#define EXTI_LINE_50                        (EXTI_DIRECT   |              EXTI_REG2 | 0x12u)  /* LPTIM3 wakeup */
#define EXTI_LINE_51                        (EXTI_RESERVED |              EXTI_REG2 | 0x13u)  /* RESERVED */
#define EXTI_LINE_52                        (EXTI_DIRECT   |              EXTI_REG2 | 0x14u)  /* LPTIM4 wakeup */
#define EXTI_LINE_53                        (EXTI_DIRECT   |              EXTI_REG2 | 0x15u)  /* LPTIM5 wakeup */
#define EXTI_LINE_54                        (EXTI_DIRECT   |              EXTI_REG2 | 0x16u)  /* I2C6 wakeup */
#define EXTI_LINE_55                        (EXTI_DIRECT   |              EXTI_REG2 | 0x17u)  /* WKUP1 wakeup */
#define EXTI_LINE_56                        (EXTI_DIRECT   |              EXTI_REG2 | 0x18u)  /* WKUP2 wakeup */
#define EXTI_LINE_57                        (EXTI_DIRECT   |              EXTI_REG2 | 0x19u)  /* WKUP3 wakeup */
#define EXTI_LINE_58                        (EXTI_DIRECT   |              EXTI_REG2 | 0x1Au)  /* WKUP4 wakeup */
#define EXTI_LINE_59                        (EXTI_DIRECT   |              EXTI_REG2 | 0x1Bu)  /* WKUP5 wakeup */
#define EXTI_LINE_60                        (EXTI_DIRECT   |              EXTI_REG2 | 0x1Cu)  /* WKUP6 wakeup */
#define EXTI_LINE_61                        (EXTI_DIRECT   |              EXTI_REG2 | 0x1Du)  /* IPCC interrupt CPU1 */
#define EXTI_LINE_62                        (EXTI_DIRECT   |              EXTI_REG2 | 0x1Eu)  /* IPCC interrupt CPU2 */
#define EXTI_LINE_63                        (EXTI_DIRECT   |              EXTI_REG2 | 0x1Fu)  /* HSEM_IT1 interrupt */
#define EXTI_LINE_64                        (EXTI_DIRECT   |              EXTI_REG3 | 0x00u)  /* HSEM_IT2 interrupt */
#define EXTI_LINE_65                        (EXTI_CONFIG   |              EXTI_REG3 | 0x01u)  /* CPU2 SEV interrupt */
#define EXTI_LINE_66                        (EXTI_CONFIG   | EXTI_EVENT | EXTI_REG3 | 0x02u)  /* CPU1 SEV interrupt */
#define EXTI_LINE_67                        (EXTI_RESERVED |              EXTI_REG3 | 0x03u)  /* RESERVED */
#define EXTI_LINE_68                        (EXTI_CONFIG   |              EXTI_REG3 | 0x04u)  /* WWDG1 reset */
#define EXTI_LINE_69                        (EXTI_DIRECT   |              EXTI_REG3 | 0x05u)  /* HDMI CEC wakeup */
#define EXTI_LINE_70                        (EXTI_DIRECT   |              EXTI_REG3 | 0x06u)  /* ETH1 pmt_intr_o wakeup */
#define EXTI_LINE_71                        (EXTI_DIRECT   |              EXTI_REG3 | 0x07u)  /* ETH1 lpi_intr_o wakeup */
#define EXTI_LINE_72                        (EXTI_DIRECT   |              EXTI_REG3 | 0x08u)  /* DTS wakeup */
#define EXTI_LINE_73                        (EXTI_CONFIG   |              EXTI_REG3 | 0x09u)  /* CPU2 SYSRESETREQ local CPU2 reset */
#define EXTI_LINE_74                        (EXTI_RESERVED |              EXTI_REG3 | 0x0Au)  /* RESERVED */
#define EXTI_LINE_75                        (EXTI_DIRECT   |              EXTI_REG3 | 0x0Bu)  /* CDBGPWRUPREQ event */


/**
  * @}
  */

/** @defgroup EXTI_Mode  EXTI Mode
  * @{
  */
#define EXTI_MODE_C1_NONE                   0x00000010u
#define EXTI_MODE_C1_INTERRUPT              0x00000011u
#define EXTI_MODE_C1_EVENT                  0x00000012u
#define EXTI_MODE_C2_NONE                   0x00000020u
#define EXTI_MODE_C2_INTERRUPT              0x00000021u
#define EXTI_MODE_C2_EVENT                  0x00000022u
/**
  * @}
  */

/** @defgroup EXTI_Trigger  EXTI Trigger
  * @{
  */
#define EXTI_TRIGGER_NONE                   0x00000000u
#define EXTI_TRIGGER_RISING                 0x00000001u
#define EXTI_TRIGGER_FALLING                0x00000002u
#define EXTI_TRIGGER_RISING_FALLING         (EXTI_TRIGGER_RISING | EXTI_TRIGGER_FALLING)
/**
  * @}
  */

/** @defgroup EXTI_GPIOSel  EXTI GPIOSel
  * @brief
  * @{
  */
#define EXTI_GPIOA                          0x00000000u
#define EXTI_GPIOB                          0x00000001u
#define EXTI_GPIOC                          0x00000002u
#define EXTI_GPIOD                          0x00000003u
#define EXTI_GPIOE                          0x00000004u
#define EXTI_GPIOF                          0x00000005u
#define EXTI_GPIOG                          0x00000006u
#define EXTI_GPIOH                          0x00000007u
#define EXTI_GPIOI                          0x00000008u
#define EXTI_GPIOJ                          0x00000009u
#define EXTI_GPIOK                          0x0000000Au
#define EXTI_GPIOZ                          0x0000000Bu
/**
  * @}
  */

/**
  * @}
  */

/* Exported macro ------------------------------------------------------------*/
/** @defgroup EXTI_Exported_Macros EXTI Exported Macros
  * @{
  */

/**
  * @}
  */

/* Private constants --------------------------------------------------------*/
/** @defgroup EXTI_Private_Constants EXTI Private Constants
  * @{
  */
/**
  * @brief  EXTI Line property definition
  */
#define EXTI_PROPERTY_SHIFT                 24u
#define EXTI_DIRECT                         (0x01uL << EXTI_PROPERTY_SHIFT)
#define EXTI_CONFIG                         (0x02uL << EXTI_PROPERTY_SHIFT)
#define EXTI_GPIO                           ((0x04uL << EXTI_PROPERTY_SHIFT) | EXTI_CONFIG)
#define EXTI_RESERVED                       (0x08uL << EXTI_PROPERTY_SHIFT)
#define EXTI_PROPERTY_MASK                  (EXTI_DIRECT | EXTI_CONFIG | EXTI_GPIO)

/**
  * @brief  EXTI Event presence definition
  */
#define EXTI_EVENT_PRESENCE_SHIFT           28u
#define EXTI_EVENT                          (0x01uL << EXTI_EVENT_PRESENCE_SHIFT)
#define EXTI_EVENT_PRESENCE_MASK            (EXTI_EVENT)

/**
  * @brief  EXTI Register and bit usage
  */
#define EXTI_REG_SHIFT                      16u
#define EXTI_REG1                           (0x00uL << EXTI_REG_SHIFT)
#define EXTI_REG2                           (0x01uL << EXTI_REG_SHIFT)
#define EXTI_REG_MASK                       (EXTI_REG1 | EXTI_REG2)
#define EXTI_PIN_MASK                       0x0000001Fu

/**
  * @brief  EXTI Mask for interrupt & event mode
  */
#define EXTI_MODE_MASK                      (EXTI_MODE_C1 | EXTI_MODE_C2 | EXTI_MODE_EVENT | EXTI_MODE_INTERRUPT)

/**
  * @brief  EXTI Mask for trigger possibilities
  */
#define EXTI_TRIGGER_MASK                   (EXTI_TRIGGER_RISING | EXTI_TRIGGER_FALLING)

/**
  * @brief  EXTI Line number
  */
#define EXTI_LINE_NB                        76uL

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @defgroup EXTI_Private_Macros EXTI Private Macros
  * @{
  */
#define IS_EXTI_LINE(__LINE__)          ((((__LINE__) & ~(EXTI_PROPERTY_MASK | EXTI_EVENT_PRESENCE_MASK | EXTI_REG_MASK | EXTI_PIN_MASK)) == 0x00u) && \
                                        ((((__LINE__) & EXTI_PROPERTY_MASK) == EXTI_DIRECT)   || \
                                         (((__LINE__) & EXTI_PROPERTY_MASK) == EXTI_CONFIG)   || \
                                         (((__LINE__) & EXTI_PROPERTY_MASK) == EXTI_GPIO))    && \
                                         (((__LINE__) & (EXTI_REG_MASK | EXTI_PIN_MASK))      < \
                                         (((EXTI_LINE_NB / 32u) << EXTI_REG_SHIFT) | (EXTI_LINE_NB % 32u))))

#define IS_EXTI_MODE(__LINE__)          ((((__LINE__) & EXTI_MODE_MASK) != 0x00u) && \
                                         (((__LINE__) & ~EXTI_MODE_MASK) == 0x00u))

#define IS_EXTI_TRIGGER(__LINE__)       (((__LINE__) & ~EXTI_TRIGGER_MASK) == 0x00u)

#define IS_EXTI_PENDING_EDGE(__LINE__)  (((__LINE__) == EXTI_TRIGGER_RISING) || \
                                         ((__LINE__) == EXTI_TRIGGER_FALLING))

#define IS_EXTI_CONFIG_LINE(__LINE__)   (((__LINE__) & EXTI_CONFIG) != 0x00u)

#define IS_EXTI_GPIO_PORT(__PORT__)     (((__PORT__) == EXTI_GPIOA) || \
                                         ((__PORT__) == EXTI_GPIOB) || \
                                         ((__PORT__) == EXTI_GPIOC) || \
                                         ((__PORT__) == EXTI_GPIOD) || \
                                         ((__PORT__) == EXTI_GPIOE) || \
                                         ((__PORT__) == EXTI_GPIOF) || \
                                         ((__PORT__) == EXTI_GPIOG) || \
                                         ((__PORT__) == EXTI_GPIOH) || \
                                         ((__PORT__) == EXTI_GPIOI) || \
                                         ((__PORT__) == EXTI_GPIOK) || \
                                         ((__PORT__) == EXTI_GPIOZ))

#define IS_EXTI_GPIO_PIN(__PIN__)       ((__PIN__) < 16u)

/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/
/** @defgroup EXTI_Exported_Functions EXTI Exported Functions
  * @brief    EXTI Exported Functions
  * @{
  */

/** @defgroup EXTI_Exported_Functions_Group1 Configuration functions
  * @brief    Configuration functions
  * @{
  */
/* Configuration functions ****************************************************/
HAL_StatusTypeDef HAL_EXTI_SetConfigLine(EXTI_HandleTypeDef *hexti, EXTI_ConfigTypeDef *pExtiConfig);
HAL_StatusTypeDef HAL_EXTI_GetConfigLine(EXTI_HandleTypeDef *hexti, EXTI_ConfigTypeDef *pExtiConfig);
HAL_StatusTypeDef HAL_EXTI_ClearConfigLine(EXTI_HandleTypeDef *hexti);
HAL_StatusTypeDef HAL_EXTI_RegisterCallback(EXTI_HandleTypeDef *hexti, EXTI_CallbackIDTypeDef CallbackID, void (*pPendingCbfn)(void));
HAL_StatusTypeDef HAL_EXTI_GetHandle(EXTI_HandleTypeDef *hexti, uint32_t ExtiLine);
/**
  * @}
  */

/** @defgroup EXTI_Exported_Functions_Group2 IO operation functions
  * @brief    IO operation functions
  * @{
  */
/* IO operation functions *****************************************************/
void              HAL_EXTI_IRQHandler(EXTI_HandleTypeDef *hexti);
uint32_t          HAL_EXTI_GetPending(EXTI_HandleTypeDef *hexti, uint32_t Edge);
void              HAL_EXTI_ClearPending(EXTI_HandleTypeDef *hexti, uint32_t Edge);
void              HAL_EXTI_GenerateSWI(EXTI_HandleTypeDef *hexti);

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

#endif /* STM32MP1xx_HAL_EXTI_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
