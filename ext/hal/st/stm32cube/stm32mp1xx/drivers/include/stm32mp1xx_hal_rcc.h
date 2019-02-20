/**
  ******************************************************************************
  * @file    stm32mp1xx_hal_rcc.h
  * @author  MCD Application Team
  * @brief   Header file of RCC HAL module.
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
#ifndef __STM32MP1xx_HAL_RCC_H
#define __STM32MP1xx_HAL_RCC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32mp1xx_hal_def.h"

/** @addtogroup STM32MP1xx_HAL_Driver
  * @{
  */

/** @addtogroup RCC
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/** @defgroup RCC_Exported_Types RCC Exported Types
  * @{
  */

/**
  * @brief  RCC PLL configuration structure definition
  */
typedef struct
{
  uint32_t PLLState;   /*!< The new state of the PLL.
                            This parameter can be a value of @ref RCC_PLL_Config                      */

  uint32_t PLLSource;  /*!< RCC_PLLSource: PLL entry clock source.
                            This parameter must be a value of @ref RCC_PLL12_Clock_Source              */

  uint32_t PLLM;       /*!< PLLM: Division factor for PLL VCO input clock.
                            This parameter must be a number between 1 and 64 */

  uint32_t PLLN;       /*!< PLLN: Multiplication factor for PLL VCO output clock
                            This parameter must be a number between:
                            Integer mode:     25 and 100 for PLL1600 (PLL1 and PLL2), 25 and 200 for PLL800
                            Fractional mode:  4 and 512 (PLL1600 and PLL800) */

  uint32_t PLLP;       /*!< PLLP: Division factor ck_pll1_p
                            This parameter must be a number between 1 and 128 */

  uint32_t PLLQ;       /*!< PLLQ: Division factor ck_pll1_q
                            This parameter must be a number between 1 and 128 */

  uint32_t PLLR;       /*!< PLLR: Division for ck_pll1_r
                            This parameter must be a number between 1 and 128 */

  uint32_t PLLRGE;     /*!< PLLRGE: PLL input frequency range for PLL3 and PLL4
                            This parameter must be a value of @ref RCC_PLL3_IF_Range and RCC_PLL4_IF_Range */

  uint32_t PLLFRACV;   /*!< PLLFRACV: Fractional Part Of The Multiplication Factor for PLL1 VCO
                            It should be a value between 0 and 8191 ((2^13)-1) */

  uint32_t PLLMODE;   /*!< PLLMODE: PLL mode used
                             This parameter must be a value of @ref RCC_PLL_Mode */

  uint32_t MOD_PER;   /*!< MOD_PER:  Modulation Period Adjustment
                             This parameter must have a value contained on @ref RCC_MOD_PER */

  uint32_t RPDFN_DIS;  /*!< RPDFN_DIS: Dithering RPDF noise control
                            This parameter must be a value of @ref RCC_RPDFN_DIS*/

  uint32_t TPDFN_DIS;  /*!< TPDFN_DIS: Dithering TPDF noise control
                            This parameter must be a value of @ref RCC_TPDFN_DIS*/

  uint32_t SSCG_MODE;  /*!< SSCG_MODE: Spread spectrum clock generator mode
                            This parameter must be a value of @ref RCC_SSCG_MODE*/

  uint32_t INC_STEP;   /*!< INC_STEP: Modulation Depth Adjustment
                            This parameter must have a value contained on @ref RCC_INC_STEP */

} RCC_PLLInitTypeDef;




typedef struct
{
  uint32_t PLL1_P_Frequency;
  uint32_t PLL1_Q_Frequency;
  uint32_t PLL1_R_Frequency;
} PLL1_ClocksTypeDef;

/**
  * @brief  RCC PLL2 Clocks structure definition => [FCH] used in UART set_config use case
  */
typedef struct
{
  uint32_t PLL2_P_Frequency;
  uint32_t PLL2_Q_Frequency;
  uint32_t PLL2_R_Frequency;
} PLL2_ClocksTypeDef;

/**
  * @brief  RCC PLL3 Clocks structure definition => [FCH] used in UART set_config use case
  */
typedef struct
{
  uint32_t PLL3_P_Frequency;
  uint32_t PLL3_Q_Frequency;
  uint32_t PLL3_R_Frequency;
} PLL3_ClocksTypeDef;


typedef struct
{
  uint32_t PLL4_P_Frequency;
  uint32_t PLL4_Q_Frequency;
  uint32_t PLL4_R_Frequency;
} PLL4_ClocksTypeDef;

/**
  * @brief  RCC Internal/External Oscillator (HSE, HSI, CSI, LSE and LSI) configuration structure definition
  */
typedef struct
{

  uint32_t OscillatorType;       /*!< The oscillators to be configured.
                                      This parameter can be a value of @ref RCC_Oscillator_Type                   */

  uint32_t HSEState;             /*!< The new state of the HSE.
                                      This parameter can be a value of @ref RCC_HSE_Config                        */

  uint32_t LSEState;             /*!< The new state of the LSE.
                                      This parameter can be a value of @ref RCC_LSE_Config                        */

  uint32_t HSIState;             /*!< The new state of the HSI.
                                      This parameter can be a value of @ref RCC_HSI_Config                        */

  uint32_t HSICalibrationValue;  /*!< The calibration trimming value.
                                      This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x7F */

  uint32_t HSIDivValue;         /*!< The HSI Division value.
                                      This parameter must be a value of @ref RCC_HSI_Clock_Prescaler              */

  uint32_t LSIState;             /*!< The new state of the LSI.
                                      This parameter can be a value of @ref RCC_LSI_Config                        */

  uint32_t CSIState;             /*!< The new state of the CSI.
                                      This parameter can be a value of @ref RCC_CSI_Config */

  uint32_t CSICalibrationValue;  /*!< The calibration trimming value.
                                      This parameter must be a number between Min_Data = 0x00 and Max_Data = 0x1F */

  RCC_PLLInitTypeDef PLL;        /*!< PLL1 structure parameters                                                    */

  RCC_PLLInitTypeDef PLL2;      /*!< PLL2 structure parameters                                                    */

  RCC_PLLInitTypeDef PLL3;      /*!< PLL3 structure parameters                                                    */

  RCC_PLLInitTypeDef PLL4;      /*!< PLL4 structure parameters                                                    */

} RCC_OscInitTypeDef;

/**
  * @brief MPU configuration structure definition.
  */
typedef struct
{
  uint32_t MPU_Clock;
  uint32_t MPU_Div;
} RCC_MPUInitTypeDef;

/**
  * @brief  AXI configuration structure definition.
  */
typedef struct
{
  uint32_t AXI_Clock;
  uint32_t AXI_Div;
} RCC_AXISSInitTypeDef;


/**
  * @brief  MCU configuration structure definition.
  */
typedef struct
{
  uint32_t MCU_Clock;
  uint32_t MCU_Div;
} RCC_MCUInitTypeDef;

/**
  * @brief  RCC MPU, MCU, AXI, AHB and APB busses clock configuration structure definition.
  */
typedef struct
{
  uint32_t                  ClockType;              /*!< The clock to be configured.
                                                         This parameter can be a value of @ref RCC_System_Clock_Type */

  RCC_MPUInitTypeDef        MPUInit;
  RCC_AXISSInitTypeDef      AXISSInit;
  RCC_MCUInitTypeDef        MCUInit;
  uint32_t                  APB4_Div;
  uint32_t                  APB5_Div;
  uint32_t                  APB1_Div;
  uint32_t                  APB2_Div;
  uint32_t                  APB3_Div;
} RCC_ClkInitTypeDef;

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/

/** @defgroup RCC_Exported_Constants RCC Exported Constants
  * @{
  */
#define DBP_TIMEOUT_VALUE              ((uint32_t)100)
#define LSE_TIMEOUT_VALUE              ((uint32_t)5000)

/** @defgroup RCC_Oscillator_Type RCC_Oscillator_Type
  * @{
  */
#define RCC_OSCILLATORTYPE_NONE        ((uint32_t)0x00000000)
#define RCC_OSCILLATORTYPE_HSE         ((uint32_t)0x00000001)
#define RCC_OSCILLATORTYPE_HSI         ((uint32_t)0x00000002)
#define RCC_OSCILLATORTYPE_LSE         ((uint32_t)0x00000004)
#define RCC_OSCILLATORTYPE_LSI         ((uint32_t)0x00000008)
#define RCC_OSCILLATORTYPE_CSI         ((uint32_t)0x00000010)

#define IS_RCC_OSCILLATORTYPE(OSCILLATOR) (((OSCILLATOR) == RCC_OSCILLATORTYPE_NONE)                           || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSE) == RCC_OSCILLATORTYPE_HSE) || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_HSI) == RCC_OSCILLATORTYPE_HSI) || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_CSI) == RCC_OSCILLATORTYPE_CSI) || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_LSI) == RCC_OSCILLATORTYPE_LSI) || \
                                           (((OSCILLATOR) & RCC_OSCILLATORTYPE_LSE) == RCC_OSCILLATORTYPE_LSE))
/**
  * @}
  */

/** @defgroup RCC_System_Clock_Type RCC_System_Clock_Type
  * @{
  */
#define RCC_CLOCKTYPE_NONE             ((uint32_t)0x00000000)
#define RCC_CLOCKTYPE_MPU              ((uint32_t)0x00000001)
#define RCC_CLOCKTYPE_ACLK             ((uint32_t)0x00000002)
#define RCC_CLOCKTYPE_HCLK             ((uint32_t)0x00000004)
#define RCC_CLOCKTYPE_PCLK4            ((uint32_t)0x00000008)
#define RCC_CLOCKTYPE_PCLK5            ((uint32_t)0x00000010)
#define RCC_CLOCKTYPE_PCLK1            ((uint32_t)0x00000020)
#define RCC_CLOCKTYPE_PCLK2            ((uint32_t)0x00000040)
#define RCC_CLOCKTYPE_PCLK3            ((uint32_t)0x00000080)



#define IS_RCC_CLOCKTYPETYPE(CLOCK)    (((CLOCK) == RCC_CLOCKTYPE_NONE)                               || \
                                       (((CLOCK) & RCC_CLOCKTYPE_MPU)  == RCC_CLOCKTYPE_MPU)          || \
                                       (((CLOCK) & RCC_CLOCKTYPE_ACLK)== RCC_CLOCKTYPE_ACLK)          || \
                                       (((CLOCK) & RCC_CLOCKTYPE_HCLK)  == RCC_CLOCKTYPE_HCLK)        || \
                                       (((CLOCK) & RCC_CLOCKTYPE_PCLK4) == RCC_CLOCKTYPE_PCLK4)       || \
                                       (((CLOCK) & RCC_CLOCKTYPE_PCLK5) == RCC_CLOCKTYPE_PCLK5)       || \
                                       (((CLOCK) & RCC_CLOCKTYPE_PCLK1) == RCC_CLOCKTYPE_PCLK1)       || \
                                       (((CLOCK) & RCC_CLOCKTYPE_PCLK2) == RCC_CLOCKTYPE_PCLK2)       || \
                                       (((CLOCK) & RCC_CLOCKTYPE_PCLK3) == RCC_CLOCKTYPE_PCLK3))
/**
  * @}
  */

/** @defgroup RCC_HSE_Config RCC_HSE_Config
  * @{
  */
#define RCC_HSE_OFF                    ((uint32_t)0x00000000)
#define RCC_HSE_ON                     RCC_OCENSETR_HSEON
#define RCC_HSE_BYPASS                 (RCC_OCENSETR_HSEBYP | RCC_OCENSETR_HSEON)
#define RCC_HSE_BYPASS_DIG             (RCC_OCENSETR_HSEBYP | RCC_OCENSETR_HSEON | RCC_OCENSETR_DIGBYP)

#define IS_RCC_HSE(HSE) (((HSE) == RCC_HSE_OFF) || ((HSE) == RCC_HSE_ON) || \
                          ((HSE) == RCC_HSE_BYPASS) || ((HSE) == RCC_HSE_BYPASS_DIG))
/**
  * @}
  */

/** @defgroup RCC_LSE_Config RCC_LSE_Config
  * @{
  */
#define RCC_LSE_OFF                    ((uint32_t)0x00000000)
#define RCC_LSE_ON                     RCC_BDCR_LSEON
#define RCC_LSE_BYPASS                 (RCC_BDCR_LSEBYP | RCC_BDCR_LSEON)
#define RCC_LSE_BYPASS_DIG             (RCC_BDCR_LSEBYP | RCC_BDCR_LSEON | RCC_BDCR_DIGBYP)

#define IS_RCC_LSE(LSE) (((LSE) == RCC_LSE_OFF) || ((LSE) == RCC_LSE_ON) || \
                         ((LSE) == RCC_LSE_BYPASS) || ((LSE) == RCC_LSE_BYPASS_DIG))
/**
  * @}
  */

/** @defgroup RCC_HSI_Config RCC_HSI_Config
  * @{
  */
#define RCC_HSI_OFF                    ((uint32_t)0x00000000)
#define RCC_HSI_ON                     RCC_OCENSETR_HSION

#define IS_RCC_HSI(HSI) (((HSI) == RCC_HSI_OFF) || ((HSI) == RCC_HSI_ON))
/**
  * @}
  */

/** @defgroup RCC_HSI_Clock_Prescaler RCC_HSI_Clock_Prescaler
  * @{
  */
#define RCC_HSI_DIV1                  RCC_HSICFGR_HSIDIV_0  /* Division by 1, ck_hsi(_ker) = 64 MHz (default after reset)*/
#define RCC_HSI_DIV2                  RCC_HSICFGR_HSIDIV_1  /* Division by 2, ck_hsi(_ker) = 32 MHz*/
#define RCC_HSI_DIV4                  RCC_HSICFGR_HSIDIV_2  /* Division by 4, ck_hsi(_ker) = 16 MHz*/
#define RCC_HSI_DIV8                  RCC_HSICFGR_HSIDIV_3  /* Division by 8, ck_hsi(_ker) =  8 MHz*/

#define IS_RCC_HSIDIV(DIV)    (((DIV) == RCC_HSI_DIV1)  || ((DIV) == RCC_HSI_DIV2) || \
                                ((DIV) == RCC_HSI_DIV4) || ((DIV) == RCC_HSI_DIV8)    )
/**
  * @}
  */


/** @defgroup RCC_LSI_Config RCC_LSI_Config
  * @{
  */
#define RCC_LSI_OFF                    ((uint32_t)0x00000000)
#define RCC_LSI_ON                     RCC_RDLSICR_LSION

#define IS_RCC_LSI(LSI) (((LSI) == RCC_LSI_OFF) || ((LSI) == RCC_LSI_ON))
/**
  * @}
  */

/** @defgroup RCC_CSI_Config RCC_CSI_Config
  * @{
  */
#define RCC_CSI_OFF                    ((uint32_t)0x00000000)
#define RCC_CSI_ON                     RCC_OCENSETR_CSION

#define IS_RCC_CSI(CSI) (((CSI) == RCC_CSI_OFF) || ((CSI) == RCC_CSI_ON))
/**
  * @}
  */


/** @defgroup RCC_MCO_Index RCC_MCO_Index
  * @{
  */
#define RCC_MCO1                       ((uint32_t)0x00000000)
#define RCC_MCO2                       ((uint32_t)0x00000001)

#define IS_RCC_MCO(MCOx) (((MCOx) == RCC_MCO1) || ((MCOx) == RCC_MCO2))
/**
  * @}
  */

/** @defgroup RCC_MCO1_Clock_Source RCC_MCO1_Clock_Source
  * @{
  */
#define RCC_MCO1SOURCE_HSI             RCC_MCO1CFGR_MCO1SEL_0
#define RCC_MCO1SOURCE_HSE             RCC_MCO1CFGR_MCO1SEL_1
#define RCC_MCO1SOURCE_CSI             RCC_MCO1CFGR_MCO1SEL_2
#define RCC_MCO1SOURCE_LSI             RCC_MCO1CFGR_MCO1SEL_3
#define RCC_MCO1SOURCE_LSE             RCC_MCO1CFGR_MCO1SEL_4


#define IS_RCC_MCO1SOURCE(SOURCE) (((SOURCE) == RCC_MCO1SOURCE_HSI) || ((SOURCE) == RCC_MCO1SOURCE_HSE)     || \
                                   ((SOURCE) == RCC_MCO1SOURCE_CSI) || ((SOURCE) == RCC_MCO1SOURCE_LSI)  || \
                                   ((SOURCE) == RCC_MCO1SOURCE_LSE) )
/**
  * @}
  */

/** @defgroup RCC_MCO2_Clock_Source RCC_MCO2_Clock_Source
  * @{
  */
#define RCC_MCO2SOURCE_MPU             RCC_MCO2CFGR_MCO2SEL_0
#define RCC_MCO2SOURCE_AXI             RCC_MCO2CFGR_MCO2SEL_1
#define RCC_MCO2SOURCE_MCU             RCC_MCO2CFGR_MCO2SEL_2
#define RCC_MCO2SOURCE_PLL4            RCC_MCO2CFGR_MCO2SEL_3
#define RCC_MCO2SOURCE_HSE             RCC_MCO2CFGR_MCO2SEL_4
#define RCC_MCO2SOURCE_HSI             RCC_MCO2CFGR_MCO2SEL_5

#define IS_RCC_MCO2SOURCE(SOURCE) (((SOURCE) == RCC_MCO2SOURCE_MPU) || ((SOURCE) == RCC_MCO2SOURCE_AXI)   || \
                                   ((SOURCE) == RCC_MCO2SOURCE_MCU)    || ((SOURCE) == RCC_MCO2SOURCE_PLL4)   || \
                                   ((SOURCE) == RCC_MCO2SOURCE_HSE)    || ((SOURCE) == RCC_MCO2SOURCE_HSI))
/**
  * @}
  */

/** @defgroup RCC_MCOx_Clock_Prescaler RCC_MCOx_Clock_Prescaler
  * @{
  * @note: MCO1 division factors are used for MCODIV values as they are the same for MCO2
  */
#define RCC_MCODIV_1                   RCC_MCO1CFGR_MCO1DIV_0
#define RCC_MCODIV_2                   RCC_MCO1CFGR_MCO1DIV_1
#define RCC_MCODIV_3                   RCC_MCO1CFGR_MCO1DIV_2
#define RCC_MCODIV_4                   RCC_MCO1CFGR_MCO1DIV_3
#define RCC_MCODIV_5                   RCC_MCO1CFGR_MCO1DIV_4
#define RCC_MCODIV_6                   RCC_MCO1CFGR_MCO1DIV_5
#define RCC_MCODIV_7                   RCC_MCO1CFGR_MCO1DIV_6
#define RCC_MCODIV_8                   RCC_MCO1CFGR_MCO1DIV_7
#define RCC_MCODIV_9                   RCC_MCO1CFGR_MCO1DIV_8
#define RCC_MCODIV_10                  RCC_MCO1CFGR_MCO1DIV_9
#define RCC_MCODIV_11                  RCC_MCO1CFGR_MCO1DIV_10
#define RCC_MCODIV_12                  RCC_MCO1CFGR_MCO1DIV_11
#define RCC_MCODIV_13                  RCC_MCO1CFGR_MCO1DIV_12
#define RCC_MCODIV_14                  RCC_MCO1CFGR_MCO1DIV_13
#define RCC_MCODIV_15                  RCC_MCO1CFGR_MCO1DIV_14
#define RCC_MCODIV_16                  RCC_MCO1CFGR_MCO1DIV_15



#define IS_RCC_MCODIV(DIV)  (((DIV) == RCC_MCODIV_1)  || ((DIV) == RCC_MCODIV_2) || \
                            ((DIV) == RCC_MCODIV_3) || ((DIV) == RCC_MCODIV_4)   || \
                            ((DIV) == RCC_MCODIV_5) || ((DIV) == RCC_MCODIV_6)   || \
                            ((DIV) == RCC_MCODIV_7) || ((DIV) == RCC_MCODIV_8)   || \
                            ((DIV) == RCC_MCODIV_9) || ((DIV) == RCC_MCODIV_10)  || \
                            ((DIV) == RCC_MCODIV_11) || ((DIV) == RCC_MCODIV_12) || \
                            ((DIV) == RCC_MCODIV_13) || ((DIV) == RCC_MCODIV_14) || \
                            ((DIV) == RCC_MCODIV_15)  || ((DIV) == RCC_MCODIV_16)    )
/**
  * @}
  */


/** @defgroup RCC_MPU_Clock_Source RCC_MPU_Clock_Source
  * @{
  */
#define RCC_MPUSOURCE_HSI              RCC_MPCKSELR_MPUSRC_0
#define RCC_MPUSOURCE_HSE              RCC_MPCKSELR_MPUSRC_1
#define RCC_MPUSOURCE_PLL1             RCC_MPCKSELR_MPUSRC_2
#define RCC_MPUSOURCE_MPUDIV           RCC_MPCKSELR_MPUSRC_3

#define IS_RCC_MPUSOURCE(SOURCE) (((SOURCE) == RCC_MPUSOURCE_HSI)    || \
                                  ((SOURCE) == RCC_MPUSOURCE_HSE)     || \
                                  ((SOURCE) == RCC_MPUSOURCE_PLL1)    || \
                                  ((SOURCE) == RCC_MPUSOURCE_MPUDIV))
/**
  * @}
  */

/** @defgroup RCC_AXISS_Clock_Source RCC_AXISS_Clock_Source
  * @{
  */
#define RCC_AXISSOURCE_HSI            RCC_ASSCKSELR_AXISSRC_0
#define RCC_AXISSOURCE_HSE            RCC_ASSCKSELR_AXISSRC_1
#define RCC_AXISSOURCE_PLL2           RCC_ASSCKSELR_AXISSRC_2
#define RCC_AXISSOURCE_OFF            RCC_ASSCKSELR_AXISSRC_3

#define IS_RCC_AXISSOURCE(SOURCE) (((SOURCE) == RCC_AXISSOURCE_HSI)  || \
                                    ((SOURCE) == RCC_AXISSOURCE_HSE)  || \
                                    ((SOURCE) == RCC_AXISSOURCE_PLL2) || \
                                    ((SOURCE) == RCC_AXISSOURCE_OFF))
/**
  * @}
  */

/** @defgroup RCC_MCU_Clock_Source RCC_MCU_Clock_Source
  * @{
  */
#define RCC_MCUSSOURCE_HSI              RCC_MSSCKSELR_MCUSSRC_0
#define RCC_MCUSSOURCE_HSE              RCC_MSSCKSELR_MCUSSRC_1
#define RCC_MCUSSOURCE_CSI              RCC_MSSCKSELR_MCUSSRC_2
#define RCC_MCUSSOURCE_PLL3             RCC_MSSCKSELR_MCUSSRC_3

#define IS_RCC_MCUSSOURCE(SOURCE) (((SOURCE) == RCC_MCUSSOURCE_HSI)      || \
                                    ((SOURCE) == RCC_MCUSSOURCE_HSE)    || \
                                    ((SOURCE) == RCC_MCUSSOURCE_CSI)    || \
                                    ((SOURCE) == RCC_MCUSSOURCE_PLL3))
/**
  * @}
  */


/** @defgroup RCC_RTC_Division_Factor RCC_RTC_Division_Factor
  * @{
  */
#define RCC_RTCDIV(x)                             RCC_RTCDIVR_RTCDIV_(y)

#define IS_RCC_RTC_HSEDIV(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 64))
/**
  * @}
  */

/** @defgroup RCC_MPU_Clock_Divider RCC_MPU_Clock_Divider
  * @{
  */
#define RCC_MPU_DIV_OFF                RCC_MPCKDIVR_MPUDIV_0
#define RCC_MPU_DIV2                   RCC_MPCKDIVR_MPUDIV_1
#define RCC_MPU_DIV4                   RCC_MPCKDIVR_MPUDIV_2
#define RCC_MPU_DIV8                   RCC_MPCKDIVR_MPUDIV_3
#define RCC_MPU_DIV16                  RCC_MPCKDIVR_MPUDIV_4

#define IS_RCC_MPUDIV(DIVIDER) (  ((DIVIDER) == RCC_MPU_DIV2)  || \
                                  ((DIVIDER) == RCC_MPU_DIV4)  || \
                                  ((DIVIDER) == RCC_MPU_DIV8)  || \
                                  ((DIVIDER) == RCC_MPU_DIV16))
/**
  * @}
  */

/** @defgroup RCC_AXI_Clock_Divider RCC_AXI_Clock_Divider
  * @{
  */
#define RCC_AXI_DIV1                   RCC_AXIDIVR_AXIDIV_0
#define RCC_AXI_DIV2                   RCC_AXIDIVR_AXIDIV_1
#define RCC_AXI_DIV3                   RCC_AXIDIVR_AXIDIV_2
#define RCC_AXI_DIV4                   RCC_AXIDIVR_AXIDIV_3

#define IS_RCC_AXIDIV(DIVIDER) (((DIVIDER) == RCC_AXI_DIV1)   || \
                                 ((DIVIDER) == RCC_AXI_DIV2)  || \
                                 ((DIVIDER) == RCC_AXI_DIV3)  || \
                                 ((DIVIDER) == RCC_AXI_DIV4))
/**
  * @}
  */

/** @defgroup RCC_APB4_Clock_Divider RCC_APB4_Clock_Divider
  * @{
  */
#define RCC_APB4_DIV1                   RCC_APB4DIVR_APB4DIV_0
#define RCC_APB4_DIV2                   RCC_APB4DIVR_APB4DIV_1
#define RCC_APB4_DIV4                   RCC_APB4DIVR_APB4DIV_2
#define RCC_APB4_DIV8                   RCC_APB4DIVR_APB4DIV_3
#define RCC_APB4_DIV16                  RCC_APB4DIVR_APB4DIV_4

#define IS_RCC_APB4DIV(DIVIDER) (((DIVIDER) == RCC_APB4_DIV1)   || \
                                 ((DIVIDER) == RCC_APB4_DIV2)   || \
                                 ((DIVIDER) == RCC_APB4_DIV4)   || \
                                 ((DIVIDER) == RCC_APB4_DIV8)   || \
                                 ((DIVIDER) == RCC_APB4_DIV16))
/**
  * @}
  */

/** @defgroup RCC_APB5_Clock_Divider RCC_APB5_Clock_Divider
  * @{
  */
#define RCC_APB5_DIV1                   RCC_APB5DIVR_APB5DIV_0
#define RCC_APB5_DIV2                   RCC_APB5DIVR_APB5DIV_1
#define RCC_APB5_DIV4                   RCC_APB5DIVR_APB5DIV_2
#define RCC_APB5_DIV8                   RCC_APB5DIVR_APB5DIV_3
#define RCC_APB5_DIV16                  RCC_APB5DIVR_APB5DIV_4

#define IS_RCC_APB5DIV(DIVIDER) (((DIVIDER) == RCC_APB5_DIV1)   || \
                                 ((DIVIDER) == RCC_APB5_DIV2)   || \
                                 ((DIVIDER) == RCC_APB5_DIV4)   || \
                                 ((DIVIDER) == RCC_APB5_DIV8)   || \
                                 ((DIVIDER) == RCC_APB5_DIV16))
/**
  * @}
  */

/** @defgroup RCC_MCU_Clock_Divider RCC_MCU_Clock_Divider
  * @{
  */
#define RCC_MCU_DIV1                   RCC_MCUDIVR_MCUDIV_0
#define RCC_MCU_DIV2                   RCC_MCUDIVR_MCUDIV_1
#define RCC_MCU_DIV4                   RCC_MCUDIVR_MCUDIV_2
#define RCC_MCU_DIV8                   RCC_MCUDIVR_MCUDIV_3
#define RCC_MCU_DIV16                  RCC_MCUDIVR_MCUDIV_4
#define RCC_MCU_DIV32                  RCC_MCUDIVR_MCUDIV_5
#define RCC_MCU_DIV64                  RCC_MCUDIVR_MCUDIV_6
#define RCC_MCU_DIV128                 RCC_MCUDIVR_MCUDIV_7
#define RCC_MCU_DIV256                 RCC_MCUDIVR_MCUDIV_8
#define RCC_MCU_DIV512                 RCC_MCUDIVR_MCUDIV_9

#define IS_RCC_MCUDIV(DIVIDER) (((DIVIDER) == RCC_MCU_DIV1)   || \
                                 ((DIVIDER) == RCC_MCU_DIV2)   || \
                                 ((DIVIDER) == RCC_MCU_DIV4)   || \
                                 ((DIVIDER) == RCC_MCU_DIV8)   || \
                                 ((DIVIDER) == RCC_MCU_DIV16)  || \
                                 ((DIVIDER) == RCC_MCU_DIV32)  || \
                                 ((DIVIDER) == RCC_MCU_DIV64)  || \
                                 ((DIVIDER) == RCC_MCU_DIV128) || \
                                 ((DIVIDER) == RCC_MCU_DIV256) || \
                                 ((DIVIDER) == RCC_MCU_DIV512))
/**
  * @}
  */

/** @defgroup RCC_APB1_Clock_Divider RCC_APB1_Clock_Divider
  * @{
  */
#define RCC_APB1_DIV1                   RCC_APB1DIVR_APB1DIV_0
#define RCC_APB1_DIV2                   RCC_APB1DIVR_APB1DIV_1
#define RCC_APB1_DIV4                   RCC_APB1DIVR_APB1DIV_2
#define RCC_APB1_DIV8                   RCC_APB1DIVR_APB1DIV_3
#define RCC_APB1_DIV16                  RCC_APB1DIVR_APB1DIV_4

#define IS_RCC_APB1DIV(DIVIDER) (((DIVIDER) == RCC_APB1_DIV1)   || \
                                 ((DIVIDER) == RCC_APB1_DIV2)   || \
                                 ((DIVIDER) == RCC_APB1_DIV4)   || \
                                 ((DIVIDER) == RCC_APB1_DIV8)   || \
                                 ((DIVIDER) == RCC_APB1_DIV16))
/**
  * @}
  */

/** @defgroup RCC_APB2_Clock_Divider RCC_APB2_Clock_Divider
  * @{
  */
#define RCC_APB2_DIV1                   RCC_APB2DIVR_APB2DIV_0
#define RCC_APB2_DIV2                   RCC_APB2DIVR_APB2DIV_1
#define RCC_APB2_DIV4                   RCC_APB2DIVR_APB2DIV_2
#define RCC_APB2_DIV8                   RCC_APB2DIVR_APB2DIV_3
#define RCC_APB2_DIV16                  RCC_APB2DIVR_APB2DIV_4

#define IS_RCC_APB2DIV(DIVIDER) (((DIVIDER) == RCC_APB2_DIV1)   || \
                                 ((DIVIDER) == RCC_APB2_DIV2)   || \
                                 ((DIVIDER) == RCC_APB2_DIV4)   || \
                                 ((DIVIDER) == RCC_APB2_DIV8)   || \
                                 ((DIVIDER) == RCC_APB2_DIV16))
/**
  * @}
  */

/** @defgroup RCC_APB3_Clock_Divider RCC_APB3_Clock_Divider
  * @{
  */
#define RCC_APB3_DIV1                   RCC_APB3DIVR_APB3DIV_0
#define RCC_APB3_DIV2                   RCC_APB3DIVR_APB3DIV_1
#define RCC_APB3_DIV4                   RCC_APB3DIVR_APB3DIV_2
#define RCC_APB3_DIV8                   RCC_APB3DIVR_APB3DIV_3
#define RCC_APB3_DIV16                  RCC_APB3DIVR_APB3DIV_4

#define IS_RCC_APB3DIV(DIVIDER) (((DIVIDER) == RCC_APB3_DIV1)   || \
                                 ((DIVIDER) == RCC_APB3_DIV2)   || \
                                 ((DIVIDER) == RCC_APB3_DIV4)   || \
                                 ((DIVIDER) == RCC_APB3_DIV8)   || \
                                 ((DIVIDER) == RCC_APB3_DIV16))
/**
  * @}
  */

/** @defgroup RCC_PLL_Config RCC_PLL_Config
  * @{
  */
#define RCC_PLL_NONE                   ((uint32_t)0x00000000)
#define RCC_PLL_OFF                    ((uint32_t)0x00000001)
#define RCC_PLL_ON                     ((uint32_t)0x00000002)

#define IS_RCC_PLL(PLL) (((PLL) == RCC_PLL_NONE) ||((PLL) == RCC_PLL_OFF) || \
                         ((PLL) == RCC_PLL_ON))
/**
  * @}
  */

/** @defgroup RCC_PLL_Mode RCC_PLL_Mode
  * @{
  */
#define RCC_PLL_INTEGER                ((uint32_t)0x00000000)
#define RCC_PLL_FRACTIONAL              ((uint32_t)0x00000001)
#define RCC_PLL_SPREAD_SPECTRUM         ((uint32_t)0x00000002)

#define IS_RCC_PLLMODE(MODE) (((MODE) == RCC_PLL_FRACTIONAL) || \
                                     ((MODE) == RCC_PLL_INTEGER) || \
                                     ((MODE) == RCC_PLL_SPREAD_SPECTRUM))
/**
  * @}
  */


/** @defgroup RCC_SSCG_MODE RCC_SSCG_MODE
  * @{
  */
#define RCC_SSCG_CENTER_SPREAD          ((uint32_t)0x00000000)
#define RCC_SSCG_DOWN_SPREAD            ((uint32_t)RCC_PLL1CSGR_SSCG_MODE)

#define IS_RCC_SSCG_MODE(MODE) (((MODE) == RCC_SSCG_CENTER_SPREAD) || \
                                ((MODE) == RCC_SSCG_DOWN_SPREAD))
/**
  * @}
  */

/** @defgroup RCC_MOD_PER RCC_MOD_PER
  * @{
  */
#define RCC_MOD_PER_MIN          ((uint32_t)0x00000001)
#define RCC_MOD_PER_MAX           ((uint32_t)RCC_PLL1CSGR_MOD_PER) /* 8191 */

#define IS_RCC_MOD_PER(ADJ) ((RCC_MOD_PER_MIN <= (ADJ)) && \
                            ((ADJ) <= RCC_MOD_PER_MAX))
/**
  * @}
  */

/** @defgroup RCC_INC_STEP RCC_INC_STEP
  * @{
  */
#define RCC_INC_STEP_MIN          ((uint32_t)0x00000001)
#define RCC_INC_STEP_MAX           ((uint32_t)0x7FFF) /* 32767 */

#define IS_RCC_INC_STEP(ADJ) ((RCC_INC_STEP_MIN <= (ADJ)) && \
                            ((ADJ) <= RCC_INC_STEP_MAX))
/**
  * @}
  */

/** @defgroup RCC_RPDFN_DIS RCC_RPDFN_DIS
  * @{
  */
#define RCC_RPDFN_DIS_ENABLED          ((uint32_t)0x00000000)
#define RCC_RPDFN_DIS_DISABLED         ((uint32_t)RCC_PLL1CSGR_RPDFN_DIS)

#define IS_RCC_RPDFN_DIS(STATE) (((STATE) == RCC_RPDFN_DIS_DISABLED) || \
                             ((STATE) == RCC_RPDFN_DIS_ENABLED))
/**
  * @}
  */


/** @defgroup RCC_TPDFN_DIS RCC_TPDFN_DIS
  * @{
  */
#define RCC_TPDFN_DIS_ENABLED            ((uint32_t)0x00000000)
#define RCC_TPDFN_DIS_DISABLED           ((uint32_t)RCC_PLL1CSGR_TPDFN_DIS)

#define IS_RCC_TPDFN_DIS(STATE) (((STATE) == RCC_TPDFN_DIS_DISABLED) || \
                            ((STATE) == RCC_TPDFN_DIS_ENABLED))
/**
  * @}
  */


/** @defgroup RCC_PLL12_Clock_Source RCC_PLL12_Clock_Source
  * @{
  */
#define RCC_PLL12SOURCE_HSI              RCC_RCK12SELR_PLL12SRC_0
#define RCC_PLL12SOURCE_HSE              RCC_RCK12SELR_PLL12SRC_1
#define RCC_PLL12SOURCE_OFF              RCC_RCK12SELR_PLL12SRC_2

#define IS_RCC_PLL12SOURCE(SOURCE) (((SOURCE) == RCC_PLL12SOURCE_HSI) || \
                                      ((SOURCE) == RCC_PLL12SOURCE_HSE) || \
                                      ((SOURCE) == RCC_PLL12SOURCE_OFF))
/**
 * @}
 */


/** @defgroup RCC_PLL3_Clock_Source RCC_PLL3_Clock_Source
  * @{
  */
#define RCC_PLL3SOURCE_HSI              RCC_RCK3SELR_PLL3SRC_0
#define RCC_PLL3SOURCE_HSE              RCC_RCK3SELR_PLL3SRC_1
#define RCC_PLL3SOURCE_CSI              RCC_RCK3SELR_PLL3SRC_2
#define RCC_PLL3SOURCE_OFF              RCC_RCK3SELR_PLL3SRC_3


#define IS_RCC_PLL3SOURCE(SOURCE) (((SOURCE) == RCC_PLL3SOURCE_HSI)  || \
                                    ((SOURCE) == RCC_PLL3SOURCE_HSE)  || \
                                    ((SOURCE) == RCC_PLL3SOURCE_CSI)  || \
                                    ((SOURCE) == RCC_PLL3SOURCE_OFF))

/**
  * @}
  */


/** @defgroup RCC_PLL4_Clock_Source RCC_PLL4_Clock_Source
  * @{
  */
#define RCC_PLL4SOURCE_HSI              RCC_RCK4SELR_PLL4SRC_0
#define RCC_PLL4SOURCE_HSE              RCC_RCK4SELR_PLL4SRC_1
#define RCC_PLL4SOURCE_CSI              RCC_RCK4SELR_PLL4SRC_2
#define RCC_PLL4SOURCE_I2S_CKIN         RCC_RCK4SELR_PLL4SRC_3


#define IS_RCC_PLL4SOURCE(SOURCE) (((SOURCE) == RCC_PLL4SOURCE_HSI)  || \
                                    ((SOURCE) == RCC_PLL4SOURCE_HSE)  || \
                                    ((SOURCE) == RCC_PLL4SOURCE_CSI)  || \
                                    ((SOURCE) == RCC_PLL4SOURCE_I2S_CKIN))
/**
  * @}
  */

/** @defgroup RCC_PLL1_MUL_DIV_Factors RCC_PLL1_MUL_DIV_Factors
  * @{
  */
#define IS_RCC_PLLM1_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 64))
/* @note DIVN limits are different depending if using integer or fractional mode */
#define IS_RCC_PLLN1_INT_VALUE(VALUE)  ((25 <= (VALUE)) && ((VALUE) <= 100))
#define IS_RCC_PLLN1_FRAC_VALUE(VALUE) ((4 <= (VALUE)) && ((VALUE) <= 512))
#define IS_RCC_PLLP1_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLQ1_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLR1_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
/**
  * @}
  */

/** @defgroup RCC_PLL2_MUL_DIV_Factors RCC_PLL2_MUL_DIV_Factors
  * @{
  */
#define IS_RCC_PLLM2_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 64))
/* @note DIVN limits are different depending if using integer or fractional mode */
#define IS_RCC_PLLN2_INT_VALUE(VALUE)  ((25 <= (VALUE)) && ((VALUE) <= 100))
#define IS_RCC_PLLN2_FRAC_VALUE(VALUE) ((4 <= (VALUE)) && ((VALUE) <= 512))
#define IS_RCC_PLLP2_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLQ2_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLR2_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
/**
  * @}
  */

/** @defgroup RCC_PLL3_MUL_DIV_Factors RCC_PLL3_MUL_DIV_Factors
  * @{
  */
#define IS_RCC_PLLM3_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 64))
/* @note DIVN limits are different depending if using integer or fractional mode */
#define IS_RCC_PLLN3_INT_VALUE(VALUE)  ((25 <= (VALUE)) && ((VALUE) <= 200))
#define IS_RCC_PLLN3_FRAC_VALUE(VALUE) ((4 <= (VALUE)) && ((VALUE) <= 512))
#define IS_RCC_PLLP3_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLQ3_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLR3_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
/**
  * @}
  */

/** @defgroup RCC_PLL4_MUL_DIV_Factors RCC_PLL4_MUL_DIV_Factors
  * @{
  */
#define IS_RCC_PLLM4_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 64))
/* @note DIVN limits are different depending if using integer or fractional mode */
#define IS_RCC_PLLN4_INT_VALUE(VALUE)  ((25 <= (VALUE)) && ((VALUE) <= 200))
#define IS_RCC_PLLN4_FRAC_VALUE(VALUE) ((4 <= (VALUE)) && ((VALUE) <= 512))
#define IS_RCC_PLLP4_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLQ4_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
#define IS_RCC_PLLR4_VALUE(VALUE) ((1 <= (VALUE)) && ((VALUE) <= 128))
/**
  * @}
  */

/** @defgroup RCC_PLL1_Clock_Output RCC_PLL1_Clock_Output
  * @{
  */
#define RCC_PLL1_DIVP                RCC_PLL1CR_DIVPEN
#define RCC_PLL1_DIVQ                RCC_PLL1CR_DIVQEN
#define RCC_PLL1_DIVR                RCC_PLL1CR_DIVREN

#define IS_RCC_PLL1CLOCKOUT_VALUE(VALUE) (((VALUE) == RCC_PLL1_DIVP) || \
                                         ((VALUE) == RCC_PLL1_DIVQ) || \
                                         ((VALUE) == RCC_PLL1_DIVR))
/**
  * @}
  */


/** @defgroup RCC_PLL2_Clock_Output RCC_PLL2_Clock_Output
  * @{
  */
#define RCC_PLL2_DIVP                RCC_PLL2CR_DIVPEN
#define RCC_PLL2_DIVQ                RCC_PLL2CR_DIVQEN
#define RCC_PLL2_DIVR                RCC_PLL2CR_DIVREN

#define IS_RCC_PLL2CLOCKOUT_VALUE(VALUE) (((VALUE) == RCC_PLL2CR_DIVPEN) || \
                                         ((VALUE) == RCC_PLL2CR_DIVQEN) || \
                                         ((VALUE) == RCC_PLL2CR_DIVREN))
/**
  * @}
  */

/** @defgroup RCC_PLL3_Clock_Output RCC_PLL3_Clock_Output
  * @{
  */

#define RCC_PLL3_DIVP                RCC_PLL3CR_DIVPEN
#define RCC_PLL3_DIVQ                RCC_PLL3CR_DIVQEN
#define RCC_PLL3_DIVR                RCC_PLL3CR_DIVREN

#define IS_RCC_PLL3CLOCKOUT_VALUE(VALUE) (((VALUE) == RCC_PLL3_DIVP) || \
                                         ((VALUE) == RCC_PLL3_DIVQ) || \
                                         ((VALUE) == RCC_PLL3_DIVR))
/**
  * @}
  */

/** @defgroup RCC_PLL4_Clock_Output RCC_PLL4_Clock_Output
  * @{
  */
#define RCC_PLL4_DIVP                RCC_PLL4CR_DIVPEN
#define RCC_PLL4_DIVQ                RCC_PLL4CR_DIVQEN
#define RCC_PLL4_DIVR                RCC_PLL4CR_DIVREN

#define IS_RCC_PLL4CLOCKOUT_VALUE(VALUE) (((VALUE) == RCC_PLL4_DIVP) || \
                                         ((VALUE) == RCC_PLL4_DIVQ) || \
                                         ((VALUE) == RCC_PLL4_DIVR))


/**
  * @}
  */

/** @defgroup RCC_PLL3_IF_Range RCC_PLL3_IF_Range
  * @{
  */
#define RCC_PLL3IFRANGE_0              RCC_PLL3CFGR1_IFRGE_0
#define RCC_PLL3IFRANGE_1              RCC_PLL3CFGR1_IFRGE_1
/**
  * @}
  */

/** @defgroup RCC_PLL4_IF_Range RCC_PLL4_IF_Range
  * @{
  */
#define RCC_PLL4IFRANGE_0              RCC_PLL4CFGR1_IFRGE_0
#define RCC_PLL4IFRANGE_1              RCC_PLL4CFGR1_IFRGE_1
/**
  * @}
  */


/** @defgroup RCC_RTC_Clock_Source RCC_RTC_Clock_Source
  * @{
  */
#define RCC_RTCCLKSOURCE_OFF           RCC_BDCR_RTCSRC_0 /* No clock (default after backup domain reset)*/
#define RCC_RTCCLKSOURCE_LSE           RCC_BDCR_RTCSRC_1
#define RCC_RTCCLKSOURCE_LSI           RCC_BDCR_RTCSRC_2
#define RCC_RTCCLKSOURCE_HSE_DIV       RCC_BDCR_RTCSRC_3 /* HSE clock divided by RTCDIV value is used as RTC clock*/

#define IS_RCC_RTCCLKSOURCE(SOURCE) (((SOURCE) == RCC_RTCCLKSOURCE_OFF) || ((SOURCE) == RCC_RTCCLKSOURCE_LSE) || \
                                      ((SOURCE) == RCC_RTCCLKSOURCE_LSI) || ((SOURCE) == RCC_RTCCLKSOURCE_HSE_DIV))
/**
  * @}
  */

/** @defgroup RCC_Flag RCC_Flag
  * @{
  */
/* Flags in the OCRDYR register */
#define RCC_FLAG_HSIRDY           ( (RCC->OCRDYR & RCC_OCRDYR_HSIRDY)     == RCC_OCRDYR_HSIRDY )
#define RCC_FLAG_HSIDIVRDY        ( (RCC->OCRDYR & RCC_OCRDYR_HSIDIVRDY)  == RCC_OCRDYR_HSIDIVRDY )
#define RCC_FLAG_CSIRDY           ( (RCC->OCRDYR & RCC_OCRDYR_CSIRDY)     == RCC_OCRDYR_CSIRDY )
#define RCC_FLAG_HSERDY           ( (RCC->OCRDYR & RCC_OCRDYR_HSERDY)     == RCC_OCRDYR_HSERDY )
#define RCC_FLAG_AXICKRDY         ( (RCC->OCRDYR & RCC_OCRDYR_AXICKRDY)   == RCC_OCRDYR_AXICKRDY )
#define RCC_FLAG_CKREST           ( (RCC->OCRDYR & RCC_OCRDYR_CKREST)     == RCC_OCRDYR_CKREST )

/* Flags in the MPCKSELR register */
#define RCC_FLAG_MPUSRCRDY        ( (RCC->MPCKSELR & RCC_MPCKSELR_MPUSRCRDY)     == RCC_MPCKSELR_MPUSRCRDY )

/* Flags in the ASSCKSELR register */
#define RCC_FLAG_AXISSRCRDY       ( (RCC->ASSCKSELR & RCC_ASSCKSELR_AXISSRCRDY)  == RCC_ASSCKSELR_AXISSRCRDY )

/* Flags in the MSSCKSELR register */
#define RCC_FLAG_MCUSSRCRDY       ( (RCC->MSSCKSELR & RCC_MSSCKSELR_MCUSSRCRDY)  == RCC_MSSCKSELR_MCUSSRCRDY )

/* Flags in the RCK12SELR register */
#define RCC_FLAG_PLL12SRCRDY      ( (RCC->RCK12SELR & RCC_RCK12SELR_PLL12SRCRDY) == RCC_RCK12SELR_PLL12SRCRDY )

/* Flags in the RCK3SELR */
#define RCC_FLAG_PLL3SRCRDY       ( (RCC->RCK3SELR & RCC_RCK3SELR_PLL3SRCRDY)    == RCC_RCK3SELR_PLL3SRCRDY )

/* Flags in the RCK4SELR register */
#define RCC_FLAG_PLL4SRCRDY       ( (RCC->RCK4SELR & RCC_RCK4SELR_PLL4SRCRDY)    == RCC_RCK4SELR_PLL4SRCRDY )

/* Flags in the TIMG1PRER register */
#define RCC_FLAG_TIMG1PRERDY      ( (RCC->TIMG1PRER & RCC_TIMG1PRER_TIMG1PRERDY) == RCC_TIMG1PRER_TIMG1PRERDY )

/* Flags in the TIMG2PRER register */
#define RCC_FLAG_TIMG2PRERDY      ( (RCC->TIMG2PRER & RCC_TIMG2PRER_TIMG2PRERDY) == RCC_TIMG2PRER_TIMG2PRERDY )

/* Flags in the MPCKDIVR register */
#define RCC_FLAG_MPUDIVRDY        ( (RCC->MPCKDIVR & RCC_MPCKDIVR_MPUDIVRDY)     == RCC_MPCKDIVR_MPUDIVRDY )

/* Flags in the AXIDIVR register */
#define RCC_FLAG_AXIDIVRDY        ( (RCC->AXIDIVR & RCC_AXIDIVR_AXIDIVRDY)       == RCC_AXIDIVR_AXIDIVRDY )

/* Flags in the APB4DIVR register */
#define RCC_FLAG_APB4DIVRDY       ( (RCC->APB4DIVR & RCC_APB4DIVR_APB4DIVRDY)    == RCC_APB4DIVR_APB4DIVRDY )

/* Flags in the APB5DIVR register */
#define RCC_FLAG_APB5DIVRDY       ( (RCC->APB5DIVR & RCC_APB5DIVR_APB5DIVRDY)    == RCC_APB5DIVR_APB5DIVRDY )

/* Flags in the MCUDIVR register */
#define RCC_FLAG_MCUDIVRDY        ( (RCC->MCUDIVR & RCC_MCUDIVR_MCUDIVRDY)       ==  RCC_MCUDIVR_MCUDIVRDY )

/* Flags in the APB1DIVR register */
#define RCC_FLAG_APB1DIVRDY       ((RCC->APB1DIVR & RCC_APB1DIVR_APB1DIVRDY)     ==  RCC_APB1DIVR_APB1DIVRDY )

/* Flags in the APB2DIVR register */
#define RCC_FLAG_APB2DIVRDY       ((RCC->APB2DIVR & RCC_APB2DIVR_APB2DIVRDY)     ==  RCC_APB2DIVR_APB2DIVRDY )

/* Flags in the APB3DIVR register */
#define RCC_FLAG_APB3DIVRDY       ((RCC->APB3DIVR & RCC_APB3DIVR_APB3DIVRDY)     ==  RCC_APB3DIVR_APB3DIVRDY )

/* Flags in the PLL1CR register */
#define RCC_FLAG_PLL1RDY          ((RCC->PLL1CR & RCC_PLL1CR_PLL1RDY)            ==  RCC_PLL1CR_PLL1RDY )

/* Flags in the PLL2CR register */
#define RCC_FLAG_PLL2RDY          ((RCC->PLL2CR & RCC_PLL2CR_PLL2RDY)            ==  RCC_PLL2CR_PLL2RDY )

/* Flags in the PLL3CR register */
#define RCC_FLAG_PLL3RDY          ((RCC->PLL3CR & RCC_PLL3CR_PLL3RDY)            ==  RCC_PLL3CR_PLL3RDY )

/* Flags in the PLL4CR register */
#define RCC_FLAG_PLL4RDY          ((RCC->PLL4CR & RCC_PLL4CR_PLL4RDY)            ==  RCC_PLL4CR_PLL4RDY )

/* Flags in the DDRCKSELR register */
#define RCC_FLAG_DDRPHYCSRCRDY    ((RCC->DDRCKSELR & RCC_DDRCKSELR_DDRPHYCSRCRDY) == RCC_DDRCKSELR_DDRPHYCSRCRDY )

/* Flags in the BDCR register */
#define RCC_FLAG_LSERDY          ((RCC->BDCR & RCC_BDCR_LSERDY)                 ==  RCC_BDCR_LSERDY )

/* Flags in the RDLSICR register */
#define RCC_FLAG_LSIRDY           ((RCC->RDLSICR & RCC_RDLSICR_LSIRDY)           ==  RCC_RDLSICR_LSIRDY )

#if defined(CORE_CA7)
/* Flags in the RCC_MP_RSTSCLRR */
#define RCC_MASK_ALL_RESET_FLAGS  (0x00001BDFU)
#define RCC_FLAG_PORRST           ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_PORRSTF)  == RCC_MP_RSTSCLRR_PORRSTF)
#define RCC_FLAG_BORRST           ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_BORRSTF)  == RCC_MP_RSTSCLRR_BORRSTF)
#define RCC_FLAG_PADRST           ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_PADRSTF)  == RCC_MP_RSTSCLRR_PADRSTF)
#define RCC_FLAG_HCSSRST          ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_HCSSRSTF) == RCC_MP_RSTSCLRR_HCSSRSTF)
#define RCC_FLAG_VCORERST         ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_VCORERSTF)  == RCC_MP_RSTSCLRR_VCORERSTF)
#define RCC_FLAG_MPSYSRSTF        ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_MPSYSRSTF)  == RCC_MP_RSTSCLRR_MPSYSRSTF)
#define RCC_FLAG_MCSYSRSTF        ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_MCSYSRSTF)  == RCC_MP_RSTSCLRR_MCSYSRSTF)
#define RCC_FLAG_IWDG1RST         ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_IWDG1RSTF)  == RCC_MP_RSTSCLRR_IWDG1RSTF)
#define RCC_FLAG_IWDG2RST         ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_IWDG2RSTF)  == RCC_MP_RSTSCLRR_IWDG2RSTF)
#define RCC_FLAG_STDBYRSTF        ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_STDBYRSTF)  == RCC_MP_RSTSCLRR_STDBYRSTF)
#define RCC_FLAG_CSTDBYRSTF       ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_CSTDBYRSTF) == RCC_MP_RSTSCLRR_CSTDBYRSTF)
#define RCC_FLAG_MPUP0RSTF        ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_MPUP0RSTF)  == RCC_MP_RSTSCLRR_MPUP0RSTF)
#define RCC_FLAG_MPUP1RSTF        ((RCC->MP_RSTSCLRR & RCC_MP_RSTSCLRR_MPUP1RSTF)  == RCC_MP_RSTSCLRR_MPUP1RSTF)
#elif defined(CORE_CM4)
/* Flags in the RCC_MC_RSTSCLRR */
#define RCC_MASK_ALL_RESET_FLAGS  (0x000007FFU)
#define RCC_FLAG_PORRST           ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_PORRSTF)  == RCC_MC_RSTSCLRR_PORRSTF)
#define RCC_FLAG_BORRST           ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_BORRSTF)  == RCC_MC_RSTSCLRR_BORRSTF)
#define RCC_FLAG_PADRST           ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_PADRSTF)  == RCC_MC_RSTSCLRR_PADRSTF)
#define RCC_FLAG_HCSSRST          ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_HCSSRSTF)  == RCC_MC_RSTSCLRR_HCSSRSTF)
#define RCC_FLAG_VCORERST         ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_VCORERSTF)  == RCC_MC_RSTSCLRR_VCORERSTF)
#define RCC_FLAG_MCURST           ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_MCURSTF)  == RCC_MC_RSTSCLRR_MCURSTF)
#define RCC_FLAG_MPSYSRST         ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_MPSYSRSTF)  == RCC_MC_RSTSCLRR_MPSYSRSTF)
#define RCC_FLAG_MCSYSRST         ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_MCSYSRSTF)  == RCC_MC_RSTSCLRR_MCSYSRSTF)
#define RCC_FLAG_IWDG1RST         ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_IWDG1RSTF)  == RCC_MC_RSTSCLRR_IWDG1RSTF)
#define RCC_FLAG_IWDG2RST         ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_IWDG2RSTF)  == RCC_MC_RSTSCLRR_IWDG2RSTF)
#define RCC_FLAG_WWDG1RST         ((RCC->MC_RSTSCLRR & RCC_MC_RSTSCLRR_WWDG1RSTF)  == RCC_MC_RSTSCLRR_WWDG1RSTF)
#endif

/** @brief  Clear all the reset flags
  * @note   Writing a '1' clears the corresponding bit to '0'
  */
#if defined(CORE_CA7)
#define __HAL_RCC_CLEAR_RESET_FLAGS() WRITE_REG(RCC->MP_RSTSCLRR, RCC_MASK_ALL_RESET_FLAGS)
#elif defined(CORE_CM4)
#define __HAL_RCC_CLEAR_RESET_FLAGS() WRITE_REG(RCC->MC_RSTSCLRR, RCC_MASK_ALL_RESET_FLAGS)
#endif /* CORE_CA7 */

#if defined(CORE_CA7)
/** @brief  Check RCC flag is set or not.
  * @param  __FLAG__: specifies the flag to check.
  *         This parameter can be one of the following values:
  *         @arg RCC_FLAG_PORRST
  *         @arg RCC_FLAG_BORRST
  *         @arg RCC_FLAG_PADRST
  *         @arg RCC_FLAG_HCSSRST
  *         @arg RCC_FLAG_VCORERST
  *         @arg RCC_FLAG_MPSYSRSTF
  *         @arg RCC_FLAG_MCSYSRSTF
  *         @arg RCC_FLAG_IWDG1RST
  *         @arg RCC_FLAG_IWDG2RST
  *         @arg RCC_FLAG_STDBYRSTF
  *         @arg RCC_FLAG_CSTDBYRSTF
  *         @arg RCC_FLAG_MPUP0RSTF
  *         @arg RCC_FLAG_MPUP1RSTF
  * @retval The state of __FLAG__ (TRUE or FALSE).
  */
#elif defined(CORE_CM4)
/** @brief  Check RCC flag is set or not.
  * @param  __FLAG__: specifies the flag to check.
  *         This parameter can be one of the following values:
  *         @arg RCC_FLAG_PORRST
  *         @arg RCC_FLAG_BORRST
  *         @arg RCC_FLAG_PADRST
  *         @arg RCC_FLAG_HCSSRST
  *         @arg RCC_FLAG_VCORERST
  *         @arg RCC_FLAG_MCURST
  *         @arg RCC_FLAG_MPSYSRST
  *         @arg RCC_FLAG_MCSYSRST
  *         @arg RCC_FLAG_IWDG1RST
  *         @arg RCC_FLAG_IWDG2RST
  *         @arg RCC_FLAG_WWDG1RST
  * @retval The state of __FLAG__ (TRUE or FALSE).
  */
#endif /* CORE_CA7 */
#define __HAL_RCC_GET_FLAG(__FLAG__) ( __FLAG__ ? 1:0 )
/**
  * @}
  */

/** @defgroup RCC_Calibration RCC_Calibration limits
  * @{
  */
/** RCC_Calibration limits*/
#define IS_RCC_HSICALIBRATION_VALUE(VALUE) ((VALUE) <= 0x7F)
#define IS_RCC_CSICALIBRATION_VALUE(VALUE) ((VALUE) <= 0x1F)
/**
  * @}
  */

/** @defgroup RCC_LSEDrive_Config RCC_LSEDrive_Config
  * @{
  */
#define RCC_LSEDRIVE_LOW                 RCC_BDCR_LSEDRV_0      /*!< LSE low drive capability */
#define RCC_LSEDRIVE_MEDIUMLOW           RCC_BDCR_LSEDRV_1      /*!< LSE medium low drive capability */
#define RCC_LSEDRIVE_MEDIUMHIGH          RCC_BDCR_LSEDRV_2      /*!< LSE medium high drive capability */
#define RCC_LSEDRIVE_HIGH                RCC_BDCR_LSEDRV_3      /*!< LSE high drive capability */

#define IS_RCC_LSEDRIVE(VALUE) (((VALUE) == RCC_LSEDRIVE_LOW) || ((VALUE) == RCC_LSEDRIVE_MEDIUMLOW) || \
                                      ((VALUE) == RCC_LSEDRIVE_MEDIUMHIGH) || ((VALUE) == RCC_LSEDRIVE_HIGH))
/**
  * @}
  */


/** @defgroup RCC_Interrupt RCC_Interrupt
  * @{
  */
#ifdef CORE_CM4
#define RCC_IT_LSIRDY                  RCC_MC_CIFR_LSIRDYF
#define RCC_IT_LSERDY                  RCC_MC_CIFR_LSERDYF
#define RCC_IT_HSIRDY                  RCC_MC_CIFR_HSIRDYF
#define RCC_IT_HSERDY                  RCC_MC_CIFR_HSERDYF
#define RCC_IT_CSIRDY                  RCC_MC_CIFR_CSIRDYF
#define RCC_IT_PLLRDY                  RCC_MC_CIFR_PLL1DYF
#define RCC_IT_PLL2RDY                 RCC_MC_CIFR_PLL2DYF
#define RCC_IT_PLL3RDY                 RCC_MC_CIFR_PLL3DYF
#define RCC_IT_PLL4RDY                 RCC_MC_CIFR_PLL4DYF
#define RCC_IT_LSECSS                  RCC_MC_CIFR_LSECSSF
#define RCC_IT_WKUP                    RCC_MC_CIFR_WKUPF
#endif /* CORE_CM4*/

#ifdef CORE_CA7
#define RCC_IT_LSIRDY                  RCC_MP_CIFR_LSIRDYF
#define RCC_IT_LSERDY                  RCC_MP_CIFR_LSERDYF
#define RCC_IT_HSIRDY                  RCC_MP_CIFR_HSIRDYF
#define RCC_IT_HSERDY                  RCC_MP_CIFR_HSERDYF
#define RCC_IT_CSIRDY                  RCC_MP_CIFR_CSIRDYF
#define RCC_IT_PLLRDY                  RCC_MP_CIFR_PLL1DYF
#define RCC_IT_PLL2RDY                 RCC_MP_CIFR_PLL2DYF
#define RCC_IT_PLL3RDY                 RCC_MP_CIFR_PLL3DYF
#define RCC_IT_PLL4RDY                 RCC_MP_CIFR_PLL4DYF
#define RCC_IT_LSECSS                  RCC_MP_CIFR_LSECSSF
#define RCC_IT_WKUP                    RCC_MP_CIFR_WKUPF
#endif /* CORE_CA7 */

#define RCC_IT_ALL  (RCC_IT_LSIRDY | RCC_IT_LSERDY | RCC_IT_HSIRDY | \
                     RCC_IT_HSERDY | RCC_IT_CSIRDY | RCC_IT_PLLRDY | \
                     RCC_IT_PLL2RDY | RCC_IT_PLL3RDY | RCC_IT_PLL4RDY | \
                     RCC_IT_LSECSS | RCC_IT_WKUP)

#define IS_RCC_IT(VALUE) (((VALUE) == RCC_IT_LSIRDY) || ((VALUE) == RCC_IT_LSERDY) || \
                          ((VALUE) == RCC_IT_HSIRDY) || ((VALUE) == RCC_IT_HSERDY) || \
                          ((VALUE) == RCC_IT_CSIRDY) || ((VALUE) == RCC_IT_PLLRDY) || \
                          ((VALUE) == RCC_IT_PLL2RDY) || ((VALUE) == RCC_IT_PLL3RDY) || \
                          ((VALUE) == RCC_IT_PLL4RDY) || ((VALUE) == RCC_IT_LSECSS) || \
                          ((VALUE) == RCC_IT_WKUP))

/**
  * @}
  */

/**
  * @}
  */

/* Exported macros -----------------------------------------------------------*/

/** @defgroup RCC_Exported_Macros RCC_Exported_Macros
  * @{
  */
/** @brief  Force or release the APB1 peripheral reset */
#define __HAL_RCC_APB1_FORCE_RESET()      (RCC->APB1RSTSETR = 0xADEFDBFFU)
#define __HAL_RCC_TIM2_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM2RST)
#define __HAL_RCC_TIM3_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM3RST)
#define __HAL_RCC_TIM4_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM4RST)
#define __HAL_RCC_TIM5_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM5RST)
#define __HAL_RCC_TIM6_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM6RST)
#define __HAL_RCC_TIM7_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM7RST)
#define __HAL_RCC_TIM12_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM12RST)
#define __HAL_RCC_TIM13_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM13RST)
#define __HAL_RCC_TIM14_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_TIM14RST)
#define __HAL_RCC_LPTIM1_FORCE_RESET()    (RCC->APB1RSTSETR = RCC_APB1RSTSETR_LPTIM1RST)
#define __HAL_RCC_SPI2_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_SPI2RST)
#define __HAL_RCC_SPI3_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_SPI3RST)
#define __HAL_RCC_USART2_FORCE_RESET()    (RCC->APB1RSTSETR = RCC_APB1RSTSETR_USART2RST)
#define __HAL_RCC_USART3_FORCE_RESET()    (RCC->APB1RSTSETR = RCC_APB1RSTSETR_USART3RST)
#define __HAL_RCC_UART4_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_UART4RST)
#define __HAL_RCC_UART5_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_UART5RST)
#define __HAL_RCC_UART7_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_UART7RST)
#define __HAL_RCC_UART8_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_UART8RST)
#define __HAL_RCC_I2C1_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_I2C1RST)
#define __HAL_RCC_I2C2_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_I2C2RST)
#define __HAL_RCC_I2C3_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_I2C3RST)
#define __HAL_RCC_I2C5_FORCE_RESET()      (RCC->APB1RSTSETR = RCC_APB1RSTSETR_I2C5RST)
#define __HAL_RCC_SPDIFRX_FORCE_RESET()   (RCC->APB1RSTSETR = RCC_APB1RSTSETR_SPDIFRST)
#define __HAL_RCC_CEC_FORCE_RESET()       (RCC->APB1RSTSETR = RCC_APB1RSTSETR_CECRST)
#define __HAL_RCC_DAC12_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_DAC12RST)
#define __HAL_RCC_MDIOS_FORCE_RESET()     (RCC->APB1RSTSETR = RCC_APB1RSTSETR_MDIOSRST)

#define __HAL_RCC_APB1_RELEASE_RESET()    (RCC->APB1RSTCLRR = 0xADEFDBFFU)
#define __HAL_RCC_TIM2_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM2RST)
#define __HAL_RCC_TIM3_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM3RST)
#define __HAL_RCC_TIM4_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM4RST)
#define __HAL_RCC_TIM5_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM5RST)
#define __HAL_RCC_TIM6_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM6RST)
#define __HAL_RCC_TIM7_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM7RST)
#define __HAL_RCC_TIM12_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM12RST)
#define __HAL_RCC_TIM13_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM13RST)
#define __HAL_RCC_TIM14_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_TIM14RST)
#define __HAL_RCC_LPTIM1_RELEASE_RESET()  (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_LPTIM1RST)
#define __HAL_RCC_SPI2_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_SPI2RST)
#define __HAL_RCC_SPI3_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_SPI3RST)
#define __HAL_RCC_USART2_RELEASE_RESET()  (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_USART2RST)
#define __HAL_RCC_USART3_RELEASE_RESET()  (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_USART3RST)
#define __HAL_RCC_UART4_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_UART4RST)
#define __HAL_RCC_UART5_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_UART5RST)
#define __HAL_RCC_UART7_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_UART7RST)
#define __HAL_RCC_UART8_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_UART8RST)
#define __HAL_RCC_I2C1_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_I2C1RST)
#define __HAL_RCC_I2C2_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_I2C2RST)
#define __HAL_RCC_I2C3_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_I2C3RST)
#define __HAL_RCC_I2C5_RELEASE_RESET()    (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_I2C5RST)
#define __HAL_RCC_SPDIFRX_RELEASE_RESET() (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_SPDIFRST)
#define __HAL_RCC_CEC_RELEASE_RESET()     (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_CECRST)
#define __HAL_RCC_DAC12_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_DAC12RST)
#define __HAL_RCC_MDIOS_RELEASE_RESET()   (RCC->APB1RSTCLRR = RCC_APB1RSTCLRR_MDIOSRST)

/** @brief  Force or release the APB2 peripheral reset. */
#define __HAL_RCC_APB2_FORCE_RESET()      (RCC->APB2RSTSETR = 0x117271FU)
#define __HAL_RCC_TIM1_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_TIM1RST)
#define __HAL_RCC_TIM8_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_TIM8RST)
#define __HAL_RCC_TIM15_FORCE_RESET()     (RCC->APB2RSTSETR = RCC_APB2RSTSETR_TIM15RST)
#define __HAL_RCC_TIM16_FORCE_RESET()     (RCC->APB2RSTSETR = RCC_APB2RSTSETR_TIM16RST)
#define __HAL_RCC_TIM17_FORCE_RESET()     (RCC->APB2RSTSETR = RCC_APB2RSTSETR_TIM17RST)
#define __HAL_RCC_SPI1_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_SPI1RST)
#define __HAL_RCC_SPI4_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_SPI4RST)
#define __HAL_RCC_SPI5_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_SPI5RST)
#define __HAL_RCC_USART6_FORCE_RESET()    (RCC->APB2RSTSETR = RCC_APB2RSTSETR_USART6RST)
#define __HAL_RCC_SAI1_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_SAI1RST)
#define __HAL_RCC_SAI2_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_SAI2RST)
#define __HAL_RCC_SAI3_FORCE_RESET()      (RCC->APB2RSTSETR = RCC_APB2RSTSETR_SAI3RST)
#define __HAL_RCC_DFSDM1_FORCE_RESET()    (RCC->APB2RSTSETR = RCC_APB2RSTSETR_DFSDMRST)
#define __HAL_RCC_FDCAN_FORCE_RESET()     (RCC->APB2RSTSETR = RCC_APB2RSTSETR_FDCANRST)

#define __HAL_RCC_APB2_RELEASE_RESET()    (RCC->APB2RSTCLRR = 0x117271FU)
#define __HAL_RCC_TIM1_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_TIM1RST)
#define __HAL_RCC_TIM8_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_TIM8RST)
#define __HAL_RCC_TIM15_RELEASE_RESET()   (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_TIM15RST)
#define __HAL_RCC_TIM16_RELEASE_RESET()   (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_TIM16RST)
#define __HAL_RCC_TIM17_RELEASE_RESET()   (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_TIM17RST)
#define __HAL_RCC_SPI1_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_SPI1RST)
#define __HAL_RCC_SPI4_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_SPI4RST)
#define __HAL_RCC_SPI5_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_SPI5RST)
#define __HAL_RCC_USART6_RELEASE_RESET()  (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_USART6RST)
#define __HAL_RCC_SAI1_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_SAI1RST)
#define __HAL_RCC_SAI2_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_SAI2RST)
#define __HAL_RCC_SAI3_RELEASE_RESET()    (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_SAI3RST)
#define __HAL_RCC_DFSDM1_RELEASE_RESET()  (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_DFSDMRST)
#define __HAL_RCC_FDCAN_RELEASE_RESET()   (RCC->APB2RSTCLRR = RCC_APB2RSTCLRR_FDCANRST)

/** @brief  Force or release the APB3 peripheral reset. */
#define __HAL_RCC_APB3_FORCE_RESET()      (RCC->APB3RSTSETR = 0x0003290FU)
#define __HAL_RCC_LPTIM2_FORCE_RESET()    (RCC->APB3RSTSETR = RCC_APB3RSTSETR_LPTIM2RST)
#define __HAL_RCC_LPTIM3_FORCE_RESET()    (RCC->APB3RSTSETR = RCC_APB3RSTSETR_LPTIM3RST)
#define __HAL_RCC_LPTIM4_FORCE_RESET()    (RCC->APB3RSTSETR = RCC_APB3RSTSETR_LPTIM4RST)
#define __HAL_RCC_LPTIM5_FORCE_RESET()    (RCC->APB3RSTSETR = RCC_APB3RSTSETR_LPTIM5RST)
#define __HAL_RCC_SAI4_FORCE_RESET()      (RCC->APB3RSTSETR = RCC_APB3RSTSETR_SAI4RST)
#define __HAL_RCC_SYSCFG_FORCE_RESET()    (RCC->APB3RSTSETR = RCC_APB3RSTSETR_SYSCFGRST)
#define __HAL_RCC_VREF_FORCE_RESET()      (RCC->APB3RSTSETR = RCC_APB3RSTSETR_VREFRST)
#define __HAL_RCC_TMPSENS_FORCE_RESET()   (RCC->APB3RSTSETR = RCC_APB3RSTSETR_TMPSENSRST)
#define __HAL_RCC_PMBCTRL_FORCE_RESET()   (RCC->APB3RSTSETR = RCC_APB3RSTSETR_PMBCTRLRST)

#define __HAL_RCC_APB3_RELEASE_RESET()    (RCC->APB3RSTCLRR = 0x0003290FU)
#define __HAL_RCC_LPTIM2_RELEASE_RESET()  (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_LPTIM2RST)
#define __HAL_RCC_LPTIM3_RELEASE_RESET()  (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_LPTIM3RST)
#define __HAL_RCC_LPTIM4_RELEASE_RESET()  (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_LPTIM4RST)
#define __HAL_RCC_LPTIM5_RELEASE_RESET()  (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_LPTIM5RST)
#define __HAL_RCC_SAI4_RELEASE_RESET()    (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_SAI4RST)
#define __HAL_RCC_SYSCFG_RELEASE_RESET()  (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_SYSCFGRST)
#define __HAL_RCC_VREF_RELEASE_RESET()    (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_VREFRST)
#define __HAL_RCC_TMPSENS_RELEASE_RESET() (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_TMPSENSRST)
#define __HAL_RCC_PMBCTRL_RELEASE_RESET() (RCC->APB3RSTCLRR = RCC_APB3RSTCLRR_PMBCTRLRST)

/** @brief  Force or release the AHB2 peripheral reset. */
#define __HAL_RCC_AHB2_FORCE_RESET()      (RCC->AHB2RSTSETR = 0x00010127U)
#define __HAL_RCC_DMA1_FORCE_RESET()      (RCC->AHB2RSTSETR = RCC_AHB2RSTSETR_DMA1RST)
#define __HAL_RCC_DMA2_FORCE_RESET()      (RCC->AHB2RSTSETR = RCC_AHB2RSTSETR_DMA2RST)
#define __HAL_RCC_DMAMUX_FORCE_RESET()    (RCC->AHB2RSTSETR = RCC_AHB2RSTSETR_DMAMUXRST)
#define __HAL_RCC_ADC12_FORCE_RESET()     (RCC->AHB2RSTSETR = RCC_AHB2RSTSETR_ADC12RST)
#define __HAL_RCC_USBO_FORCE_RESET()      (RCC->AHB2RSTSETR = RCC_AHB2RSTSETR_USBORST)
#define __HAL_RCC_SDMMC3_FORCE_RESET()    (RCC->AHB2RSTSETR = RCC_AHB2RSTSETR_SDMMC3RST)

#define __HAL_RCC_AHB2_RELEASE_RESET()    (RCC->AHB2RSTCLRR = 0x00010127U)
#define __HAL_RCC_DMA1_RELEASE_RESET()    (RCC->AHB2RSTCLRR = RCC_AHB2RSTCLRR_DMA1RST)
#define __HAL_RCC_DMA2_RELEASE_RESET()    (RCC->AHB2RSTCLRR = RCC_AHB2RSTCLRR_DMA2RST)
#define __HAL_RCC_DMAMUX_RELEASE_RESET()  (RCC->AHB2RSTCLRR = RCC_AHB2RSTCLRR_DMAMUXRST)
#define __HAL_RCC_ADC12_RELEASE_RESET()   (RCC->AHB2RSTCLRR = RCC_AHB2RSTCLRR_ADC12RST)
#define __HAL_RCC_USBO_RELEASE_RESET()    (RCC->AHB2RSTCLRR = RCC_AHB2RSTCLRR_USBORST)
#define __HAL_RCC_SDMMC3_RELEASE_RESET()  (RCC->AHB2RSTCLRR = RCC_AHB2RSTCLRR_SDMMC3RST)

/** @brief  Force or release the AHB3 peripheral reset. */
#define __HAL_RCC_DCMI_FORCE_RESET()      (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_DCMIRST)
#if defined(CRYP2)
#define __HAL_RCC_AHB3_FORCE_RESET()      (RCC->AHB3RSTSETR = 0x000018F1U)
#define __HAL_RCC_CRYP2_FORCE_RESET()     (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_CRYP2RST)
#else
#define __HAL_RCC_AHB3_FORCE_RESET()      (RCC->AHB3RSTSETR = 0x000018E1U)
#endif  /* CRYP2 */
#define __HAL_RCC_HASH2_FORCE_RESET()     (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_HASH2RST)
#define __HAL_RCC_RNG2_FORCE_RESET()      (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_RNG2RST)
#define __HAL_RCC_CRC2_FORCE_RESET()      (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_CRC2RST)
#define __HAL_RCC_HSEM_FORCE_RESET()      (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_HSEMRST)
#define __HAL_RCC_IPCC_FORCE_RESET()      (RCC->AHB3RSTSETR = RCC_AHB3RSTSETR_IPCCRST)

#define __HAL_RCC_DCMI_RELEASE_RESET()    (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_DCMIRST)
#if defined(CRYP2)
#define __HAL_RCC_AHB3_RELEASE_RESET()    (RCC->AHB3RSTCLRR = 0x000018F1U)
#define __HAL_RCC_CRYP2_RELEASE_RESET()   (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_CRYP2RST)
#else
#define __HAL_RCC_AHB3_REALEASE_RESET()   (RCC->AHB3RSTCLRR = 0x000018E1U)
#endif  /* CRYP2 */
#define __HAL_RCC_HASH2_RELEASE_RESET()   (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_HASH2RST)
#define __HAL_RCC_RNG2_RELEASE_RESET()    (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_RNG2RST)
#define __HAL_RCC_CRC2_RELEASE_RESET()    (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_CRC2RST)
#define __HAL_RCC_HSEM_RELEASE_RESET()    (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_HSEMRST)
#define __HAL_RCC_IPCC_RELEASE_RESET()    (RCC->AHB3RSTCLRR = RCC_AHB3RSTCLRR_IPCCRST)

/** @brief  Force or release the AHB4 peripheral reset. */
#define __HAL_RCC_AHB4_FORCE_RESET()      (RCC->AHB4RSTSETR = 0x000007FFU)
#define __HAL_RCC_GPIOA_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOARST)
#define __HAL_RCC_GPIOB_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOBRST)
#define __HAL_RCC_GPIOC_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOCRST)
#define __HAL_RCC_GPIOD_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIODRST)
#define __HAL_RCC_GPIOE_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOERST)
#define __HAL_RCC_GPIOF_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOFRST)
#define __HAL_RCC_GPIOG_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOGRST)
#define __HAL_RCC_GPIOH_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOHRST)
#define __HAL_RCC_GPIOI_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOIRST)
#define __HAL_RCC_GPIOJ_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOJRST)
#define __HAL_RCC_GPIOK_FORCE_RESET()     (RCC->AHB4RSTSETR = RCC_AHB4RSTSETR_GPIOKRST)

#define __HAL_RCC_AHB4_RELEASE_RESET()    (RCC->AHB4RSTCLRR = 0x000007FFU)
#define __HAL_RCC_GPIOA_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOARST)
#define __HAL_RCC_GPIOB_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOBRST)
#define __HAL_RCC_GPIOC_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOCRST)
#define __HAL_RCC_GPIOD_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIODRST)
#define __HAL_RCC_GPIOE_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOERST)
#define __HAL_RCC_GPIOF_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOFRST)
#define __HAL_RCC_GPIOG_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOGRST)
#define __HAL_RCC_GPIOH_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOHRST)
#define __HAL_RCC_GPIOI_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOIRST)
#define __HAL_RCC_GPIOJ_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOJRST)
#define __HAL_RCC_GPIOK_RELEASE_RESET()   (RCC->AHB4RSTCLRR = RCC_AHB4RSTCLRR_GPIOKRST)

/** @brief  Force or release the APB4 peripheral reset. */
#define __HAL_RCC_APB4_FORCE_RESET()      (RCC->APB4RSTSETR = 0x00010111U)
#define __HAL_RCC_LTDC_FORCE_RESET()      (RCC->APB4RSTSETR = RCC_APB4RSTSETR_LTDCRST)
#define __HAL_RCC_DSI_FORCE_RESET()       (RCC->APB4RSTSETR = RCC_APB4RSTSETR_DSIRST)
#define __HAL_RCC_DDRPERFM_FORCE_RESET()  (RCC->APB4RSTSETR = RCC_APB4RSTSETR_DDRPERFMRST)
#define __HAL_RCC_USBPHY_FORCE_RESET()    (RCC->APB4RSTSETR = RCC_APB4RSTSETR_USBPHYRST)

#define __HAL_RCC_APB4_RELEASE_RESET()    (RCC->APB4RSTCLRR = 0x00010111U)
#define __HAL_RCC_LTDC_RELEASE_RESET()    (RCC->APB4RSTCLRR = RCC_APB4RSTCLRR_LTDCRST)
#define __HAL_RCC_DSI_RELEASE_RESET()     (RCC->APB4RSTCLRR = RCC_APB4RSTCLRR_DSIRST)
#define __HAL_RCC_DDRPERFM_RELEASE_RESET() (RCC->APB4RSTCLRR = RCC_APB4RSTCLRR_DDRPERFMRST)
#define __HAL_RCC_USBPHY_RELEASE_RESET()  (RCC->APB4RSTCLRR = RCC_APB4RSTCLRR_USBPHYRST)

/** @brief  Force or release the APB5 peripheral reset. */
/** @note   Note that when the secure mode is enabled, only secure accesses can
 *          reset the peripherals connected to APB5.
 */
#define __HAL_RCC_APB5_FORCE_RESET()      (RCC->APB5RSTSETR = 0x0010001DU)
#define __HAL_RCC_SPI6_FORCE_RESET()      (RCC->APB5RSTSETR = RCC_APB5RSTSETR_SPI6RST)
#define __HAL_RCC_I2C4_FORCE_RESET()      (RCC->APB5RSTSETR = RCC_APB5RSTSETR_I2C4RST)
#define __HAL_RCC_I2C6_FORCE_RESET()      (RCC->APB5RSTSETR = RCC_APB5RSTSETR_I2C6RST)
#define __HAL_RCC_USART1_FORCE_RESET()    (RCC->APB5RSTSETR = RCC_APB5RSTSETR_USART1RST)
#define __HAL_RCC_STGEN_FORCE_RESET()     (RCC->APB5RSTSETR = RCC_APB5RSTSETR_STGENRST)

#define __HAL_RCC_APB5_RELEASE_RESET()    (RCC->APB5RSTCLRR = 0x0010001DU)
#define __HAL_RCC_SPI6_RELEASE_RESET()    (RCC->APB5RSTCLRR = RCC_APB5RSTCLRR_SPI6RST)
#define __HAL_RCC_I2C4_RELEASE_RESET()    (RCC->APB5RSTCLRR = RCC_APB5RSTCLRR_I2C4RST)
#define __HAL_RCC_I2C6_RELEASE_RESET()    (RCC->APB5RSTCLRR = RCC_APB5RSTCLRR_I2C6RST)
#define __HAL_RCC_USART1_RELEASE_RESET()  (RCC->APB5RSTCLRR = RCC_APB5RSTCLRR_USART1RST)
#define __HAL_RCC_STGEN_RELEASE_RESET()   (RCC->APB5RSTCLRR = RCC_APB5RSTCLRR_STGENRST)

/** @brief  Force or release the AHB5 peripheral reset. */
#define __HAL_RCC_GPIOZ_FORCE_RESET()     (RCC->AHB5RSTSETR = RCC_AHB5RSTSETR_GPIOZRST)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_FORCE_RESET()     (RCC->AHB5RSTSETR = RCC_AHB5RSTSETR_CRYP1RST)
#define __HAL_RCC_AHB5_FORCE_RESET()      (RCC->AHB5RSTSETR = 0x00010071U)
#else
#define __HAL_RCC_AHB5_FORCE_RESET()      (RCC->AHB5RSTSETR = 0x00010061U)
#endif /* CRYP1 */
#define __HAL_RCC_HASH1_FORCE_RESET()     (RCC->AHB5RSTSETR = RCC_AHB5RSTSETR_HASH1RST)
#define __HAL_RCC_RNG1_FORCE_RESET()      (RCC->AHB5RSTSETR = RCC_AHB5RSTSETR_RNG1RST)
#define __HAL_RCC_AXIMC_FORCE_RESET()     (RCC->AHB5RSTSETR = RCC_AHB5RSTSETR_AXIMCRST)

#define __HAL_RCC_GPIOZ_RELEASE_RESET()   (RCC->AHB5RSTCLRR = RCC_AHB5RSTCLRR_GPIOZRST)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_RELEASE_RESET()   (RCC->AHB5RSTCLRR = RCC_AHB5RSTCLRR_CRYP1RST)
#define __HAL_RCC_AHB5_RELEASE_RESET()    (RCC->AHB5RSTCLRR = 0x00010071U)
#else
#define __HAL_RCC_AHB5_RELEASE_RESET()    (RCC->AHB5RSTCLRR = 0x00010061U)
#endif /* CRYP1 */
#define __HAL_RCC_HASH1_RELEASE_RESET()   (RCC->AHB5RSTCLRR = RCC_AHB5RSTCLRR_HASH1RST)
#define __HAL_RCC_RNG1_RELEASE_RESET()    (RCC->AHB5RSTCLRR = RCC_AHB5RSTCLRR_RNG1RST)
#define __HAL_RCC_AXIMC_RELEASE_RESET()   (RCC->AHB5RSTCLRR = RCC_AHB5RSTCLRR_AXIMCRST)

/** @brief  Force or release the AHB6 peripheral reset. */
#define __HAL_RCC_AHB6_FORCE_RESET()      (RCC->AHB6RSTSETR = 0x01135420U)
#define __HAL_RCC_GPU_FORCE_RESET()       (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_GPURST)
#define __HAL_RCC_ETH1MAC_FORCE_RESET()   (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_ETHMACRST)
#define __HAL_RCC_FMC_FORCE_RESET()       (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_FMCRST)
#define __HAL_RCC_QSPI_FORCE_RESET()      (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_QSPIRST)
#define __HAL_RCC_SDMMC1_FORCE_RESET()    (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_SDMMC1RST)
#define __HAL_RCC_SDMMC2_FORCE_RESET()    (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_SDMMC2RST)
#define __HAL_RCC_CRC1_FORCE_RESET()      (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_CRC1RST)
#define __HAL_RCC_USBH_FORCE_RESET()      (RCC->AHB6RSTSETR = RCC_AHB6RSTSETR_USBHRST)

/** @note __HAL_RCC_GPU_RELEASE_RESET does not exist as GPU reset process may
 *        take several clock cycles, for that purpose, the application can
 *        read-back the reset bit RCC_AHB6RSTSETR_GPURST in order to check if
 *        the reset process is on-going or completed after setting
 *        RCC_AHB6RSTSETR_GPURST
 */
#define __HAL_RCC_AHB6_RELEASE_RESET()    (RCC->AHB6RSTCLRR = 0x01135400U)
#define __HAL_RCC_ETH1MAC_RELEASE_RESET() (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_ETHMACRST)
#define __HAL_RCC_FMC_RELEASE_RESET()     (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_FMCRST)
#define __HAL_RCC_QSPI_RELEASE_RESET()    (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_QSPIRST)
#define __HAL_RCC_SDMMC1_RELEASE_RESET()  (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_SDMMC1RST)
#define __HAL_RCC_SDMMC2_RELEASE_RESET()  (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_SDMMC2RST)
#define __HAL_RCC_CRC1_RELEASE_RESET()    (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_CRC1RST)
#define __HAL_RCC_USBH_RELEASE_RESET()    (RCC->AHB6RSTCLRR = RCC_AHB6RSTCLRR_USBHRST)

/** @brief  Force or release the TZAHB6 peripheral reset. */
#define __HAL_RCC_TZAHB6_FORCE_RESET()    (RCC->TZAHB6RSTSETR = 0x00000001U)
#define __HAL_RCC_MDMA_FORCE_RESET()      (RCC->TZAHB6RSTSETR = RCC_TZAHB6RSTSETR_MDMARST)

#define __HAL_RCC_TZAHB6_RELEASE_RESET()  (RCC->TZAHB6RSTCLRR = 0x00000001U)
#define __HAL_RCC_MDMA_RELEASE_RESET()    (RCC->TZAHB6RSTCLRR = RCC_TZAHB6RSTCLRR_MDMARST)

#ifdef CORE_CA7
/** @brief  Enable or disable the APB1 peripheral clock.
 * @note   After reset, the peripheral clock (used for registers read/write access)
 *         is disabled and the application software has to enable this clock before
 *         using it. It shall be used to allocate a peripheral to the MCU.
 */
#define __HAL_RCC_TIM2_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM2EN)
#define __HAL_RCC_TIM3_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM3EN)
#define __HAL_RCC_TIM4_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM4EN)
#define __HAL_RCC_TIM5_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM5EN)
#define __HAL_RCC_TIM6_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM6EN)
#define __HAL_RCC_TIM7_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM7EN)
#define __HAL_RCC_TIM12_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM12EN)
#define __HAL_RCC_TIM13_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM13EN)
#define __HAL_RCC_TIM14_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_TIM14EN)
#define __HAL_RCC_LPTIM1_CLK_ENABLE()     (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_LPTIM1EN)
#define __HAL_RCC_SPI2_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_SPI2EN)
#define __HAL_RCC_SPI3_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_SPI3EN)
#define __HAL_RCC_USART2_CLK_ENABLE()     (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_USART2EN)
#define __HAL_RCC_USART3_CLK_ENABLE()     (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_USART3EN)
#define __HAL_RCC_UART4_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_UART4EN)
#define __HAL_RCC_UART5_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_UART5EN)
#define __HAL_RCC_UART7_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_UART7EN)
#define __HAL_RCC_UART8_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_UART8EN)
#define __HAL_RCC_I2C1_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_I2C1EN)
#define __HAL_RCC_I2C2_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_I2C2EN)
#define __HAL_RCC_I2C3_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_I2C3EN)
#define __HAL_RCC_I2C5_CLK_ENABLE()       (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_I2C5EN)
#define __HAL_RCC_SPDIFRX_CLK_ENABLE()    (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_SPDIFEN)
#define __HAL_RCC_CEC_CLK_ENABLE()        (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_CECEN)
#define __HAL_RCC_DAC12_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_DAC12EN)
#define __HAL_RCC_MDIOS_CLK_ENABLE()      (RCC->MP_APB1ENSETR = RCC_MC_APB1ENSETR_MDIOSEN)

#define __HAL_RCC_TIM2_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM2EN)
#define __HAL_RCC_TIM3_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM3EN)
#define __HAL_RCC_TIM4_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM4EN)
#define __HAL_RCC_TIM5_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM5EN)
#define __HAL_RCC_TIM6_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM6EN)
#define __HAL_RCC_TIM7_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM7EN)
#define __HAL_RCC_TIM12_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM12EN)
#define __HAL_RCC_TIM13_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM13EN)
#define __HAL_RCC_TIM14_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM14EN)
#define __HAL_RCC_LPTIM1_CLK_DISABLE()    (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_LPTIM1EN)
#define __HAL_RCC_SPI2_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_SPI2EN)
#define __HAL_RCC_SPI3_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_SPI3EN)
#define __HAL_RCC_USART2_CLK_DISABLE()    (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_USART2EN)
#define __HAL_RCC_USART3_CLK_DISABLE()    (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_USART3EN)
#define __HAL_RCC_UART4_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART4EN)
#define __HAL_RCC_UART4_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART4EN)
#define __HAL_RCC_UART5_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART5EN)
#define __HAL_RCC_UART7_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART7EN)
#define __HAL_RCC_UART8_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART8EN)
#define __HAL_RCC_I2C1_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C1EN)
#define __HAL_RCC_I2C2_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C2EN)
#define __HAL_RCC_I2C3_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C3EN)
#define __HAL_RCC_I2C5_CLK_DISABLE()      (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C5EN)
#define __HAL_RCC_SPDIFRX_CLK_DISABLE()   (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_SPDIFEN)
#define __HAL_RCC_CEC_CLK_DISABLE()       (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_CECEN)
#define __HAL_RCC_DAC12_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_DAC12EN)
#define __HAL_RCC_MDIOS_CLK_DISABLE()     (RCC->MP_APB1ENCLRR = RCC_MC_APB1ENCLRR_MDIOSEN)

/** @brief  Enable or disable the APB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_TIM1_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_TIM1EN)
#define __HAL_RCC_TIM8_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_TIM8EN)
#define __HAL_RCC_TIM15_CLK_ENABLE()      (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_TIM15EN)
#define __HAL_RCC_TIM16_CLK_ENABLE()      (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_TIM16EN)
#define __HAL_RCC_TIM17_CLK_ENABLE()      (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_TIM17EN)
#define __HAL_RCC_SPI1_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_SPI1EN)
#define __HAL_RCC_SPI4_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_SPI4EN)
#define __HAL_RCC_SPI5_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_SPI5EN)
#define __HAL_RCC_USART6_CLK_ENABLE()     (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_USART6EN)
#define __HAL_RCC_SAI1_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_SAI1EN)
#define __HAL_RCC_SAI2_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_SAI2EN)
#define __HAL_RCC_SAI3_CLK_ENABLE()       (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_SAI3EN)
#define __HAL_RCC_DFSDM1_CLK_ENABLE()     (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_DFSDMEN)
#define __HAL_RCC_ADFSDM1_CLK_ENABLE()    (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_ADFSDMEN)
#define __HAL_RCC_FDCAN_CLK_ENABLE()      (RCC->MP_APB2ENSETR = RCC_MC_APB2ENSETR_FDCANEN)

#define __HAL_RCC_TIM1_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM1EN)
#define __HAL_RCC_TIM8_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM8EN)
#define __HAL_RCC_TIM15_CLK_DISABLE()     (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM15EN)
#define __HAL_RCC_TIM16_CLK_DISABLE()     (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM16EN)
#define __HAL_RCC_TIM17_CLK_DISABLE()     (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM17EN)
#define __HAL_RCC_SPI1_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_SPI1EN)
#define __HAL_RCC_SPI4_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_SPI4EN)
#define __HAL_RCC_SPI5_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_SPI5EN)
#define __HAL_RCC_USART6_CLK_DISABLE()    (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_USART6EN)
#define __HAL_RCC_SAI1_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_SAI1EN)
#define __HAL_RCC_SAI2_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_SAI2EN)
#define __HAL_RCC_SAI3_CLK_DISABLE()      (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_SAI3EN)
#define __HAL_RCC_DFSDM1_CLK_DISABLE()    (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_DFSDMEN)
#define __HAL_RCC_ADFSDM1_CLK_DISABLE()   (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_ADFSDMEN)
#define __HAL_RCC_FDCAN_CLK_DISABLE()     (RCC->MP_APB2ENCLRR = RCC_MC_APB2ENCLRR_FDCANEN)

/** @brief  Enable or disable the APB3 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_LPTIM2_CLK_ENABLE()     (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM2EN)
#define __HAL_RCC_LPTIM3_CLK_ENABLE()     (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM3EN)
#define __HAL_RCC_LPTIM4_CLK_ENABLE()     (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM4EN)
#define __HAL_RCC_LPTIM5_CLK_ENABLE()     (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM5EN)
#define __HAL_RCC_SAI4_CLK_ENABLE()       (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_SAI4EN)
#define __HAL_RCC_SYSCFG_CLK_ENABLE()     (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_SYSCFGEN)
#define __HAL_RCC_VREF_CLK_ENABLE()       (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_VREFEN)
#define __HAL_RCC_TMPSENS_CLK_ENABLE()    (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_TMPSENSEN)
#define __HAL_RCC_PMBCTRL_CLK_ENABLE()    (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_PMBCTRLEN)
#define __HAL_RCC_HDP_CLK_ENABLE()        (RCC->MP_APB3ENSETR = RCC_MC_APB3ENSETR_HDPEN)

#define __HAL_RCC_LPTIM2_CLK_DISABLE()    (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM2EN)
#define __HAL_RCC_LPTIM3_CLK_DISABLE()    (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM3EN)
#define __HAL_RCC_LPTIM4_CLK_DISABLE()    (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM4EN)
#define __HAL_RCC_LPTIM5_CLK_DISABLE()    (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM5EN)
#define __HAL_RCC_SAI4_CLK_DISABLE()      (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_SAI4EN)
#define __HAL_RCC_SYSCFG_CLK_DISABLE()    (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_SYSCFGEN)
#define __HAL_RCC_VREF_CLK_DISABLE()      (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_VREFEN)
#define __HAL_RCC_TMPSENS_CLK_DISABLE()   (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_TMPSENSEN)
#define __HAL_RCC_PMBCTRL_CLK_DISABLE()   (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_PMBCTRLEN)
#define __HAL_RCC_HDP_CLK_DISABLE()       (RCC->MP_APB3ENCLRR = RCC_MC_APB3ENCLRR_HDPEN)

/** @brief  Enable or disable the APB4 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_LTDC_CLK_ENABLE()       (RCC->MP_APB4ENSETR = RCC_MC_APB4ENSETR_LTDCEN)
#define __HAL_RCC_DSI_CLK_ENABLE()        (RCC->MP_APB4ENSETR = RCC_MC_APB4ENSETR_DSIEN)
#define __HAL_RCC_DDRPERFM_CLK_ENABLE()   (RCC->MP_APB4ENSETR = RCC_MC_APB4ENSETR_DDRPERFMEN)
#define __HAL_RCC_IWDG2APB_CLK_ENABLE()   (RCC->MP_APB4ENSETR = RCC_MP_APB4ENSETR_IWDG2APBEN)
#define __HAL_RCC_USBPHY_CLK_ENABLE()     (RCC->MP_APB4ENSETR = RCC_MC_APB4ENSETR_USBPHYEN)
#define __HAL_RCC_STGENRO_CLK_ENABLE()    (RCC->MP_APB4ENSETR = RCC_MC_APB4ENSETR_STGENROEN)

#define __HAL_RCC_LTDC_CLK_DISABLE()      (RCC->MP_APB4ENCLRR = RCC_MC_APB4ENCLRR_LTDCEN)
#define __HAL_RCC_DSI_CLK_DISABLE()       (RCC->MP_APB4ENCLRR = RCC_MC_APB4ENCLRR_DSIEN)
#define __HAL_RCC_DDRPERFM_CLK_DISABLE()  (RCC->MP_APB4ENCLRR = RCC_MC_APB4ENCLRR_DDRPERFMEN)
#define __HAL_RCC_IWDG2APB_CLK_DISABLE()  (RCC->MP_APB4ENCLRR = RCC_MP_APB4ENCLRR_IWDG2APBEN)
#define __HAL_RCC_USBPHY_CLK_DISABLE()    (RCC->MP_APB4ENCLRR = RCC_MC_APB4ENCLRR_USBPHYEN)
#define __HAL_RCC_STGENRO_CLK_DISABLE()   (RCC->MP_APB4ENCLRR = RCC_MC_APB4ENCLRR_STGENROEN)

/** @brief  Enable or disable the APB5 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_SPI6_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_SPI6EN)
#define __HAL_RCC_I2C4_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_I2C4EN)
#define __HAL_RCC_I2C6_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_I2C6EN)
#define __HAL_RCC_USART1_CLK_ENABLE()     (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_USART1EN)
#define __HAL_RCC_RTCAPB_CLK_ENABLE()     (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_RTCAPBEN)
#define __HAL_RCC_TZC1_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_TZC1EN)
#define __HAL_RCC_TZC2_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_TZC2EN)
#define __HAL_RCC_TZPC_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_TZPCEN)
#define __HAL_RCC_IWDG1APB_CLK_ENABLE()   (RCC->MP_APB5ENSETR = RCC_MP_APB5ENSETR_IWDG1APBEN)
#define __HAL_RCC_BSEC_CLK_ENABLE()       (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_BSECEN)
#define __HAL_RCC_STGEN_CLK_ENABLE()      (RCC->MP_APB5ENSETR = RCC_MC_APB5ENSETR_STGENEN)

#define __HAL_RCC_SPI6_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_SPI6EN)
#define __HAL_RCC_I2C4_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_I2C4EN)
#define __HAL_RCC_I2C6_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_I2C6EN)
#define __HAL_RCC_USART1_CLK_DISABLE()    (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_USART1EN)
#define __HAL_RCC_RTCAPB_CLK_DISABLE()    (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_RTCAPBEN)
#define __HAL_RCC_TZC1_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_TZC1EN)
#define __HAL_RCC_TZC2_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_TZC2EN)
#define __HAL_RCC_TZPC_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENCLRR_TZPCEN)
#define __HAL_RCC_IWDG1APB_CLK_DISABLE()  (RCC->MP_APB5ENCLRR = RCC_MP_APB5ENCLRR_IWDG1APBEN)
#define __HAL_RCC_BSEC_CLK_DISABLE()      (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENSETR_BSECEN)
#define __HAL_RCC_STGEN_CLK_DISABLE()     (RCC->MP_APB5ENCLRR = RCC_MC_APB5ENSETR_STGENEN)

/** @brief  Enable or disable the AHB5 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_GPIOZ_CLK_ENABLE()      (RCC->MP_AHB5ENSETR = RCC_MC_AHB5ENSETR_GPIOZEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_ENABLE()      (RCC->MP_AHB5ENSETR = RCC_MC_AHB5ENSETR_CRYP1EN)
#endif
#define __HAL_RCC_HASH1_CLK_ENABLE()      (RCC->MP_AHB5ENSETR = RCC_MC_AHB5ENSETR_HASH1EN)
#define __HAL_RCC_RNG1_CLK_ENABLE()       (RCC->MP_AHB5ENSETR = RCC_MC_AHB5ENSETR_RNG1EN)
#define __HAL_RCC_BKPSRAM_CLK_ENABLE()    (RCC->MP_AHB5ENSETR = RCC_MC_AHB5ENSETR_BKPSRAMEN)

#define __HAL_RCC_GPIOZ_CLK_DISABLE()     (RCC->MP_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_GPIOZEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_DISABLE()     (RCC->MP_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_CRYP1EN)
#endif
#define __HAL_RCC_HASH1_CLK_DISABLE()     (RCC->MP_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_HASH1EN)
#define __HAL_RCC_RNG1_CLK_DISABLE()      (RCC->MP_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_RNG1EN)
#define __HAL_RCC_BKPSRAM_CLK_DISABLE()   (RCC->MP_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_BKPSRAMEN)

/** @brief  Enable or disable the AHB6 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_MDMA_CLK_ENABLE()       (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_MDMAEN)
#define __HAL_RCC_GPU_CLK_ENABLE()        (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_GPUEN)
#define __HAL_RCC_ETH1CK_CLK_ENABLE()     (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHCKEN)
#define __HAL_RCC_ETH1TX_CLK_ENABLE()     (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHTXEN)
#define __HAL_RCC_ETH1RX_CLK_ENABLE()     (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHRXEN)
#define __HAL_RCC_ETH1MAC_CLK_ENABLE()    (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHMACEN)
#define __HAL_RCC_FMC_CLK_ENABLE()        (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_FMCEN)
#define __HAL_RCC_QSPI_CLK_ENABLE()       (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_QSPIEN)
#define __HAL_RCC_SDMMC1_CLK_ENABLE()     (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_SDMMC1EN)
#define __HAL_RCC_SDMMC2_CLK_ENABLE()     (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_SDMMC2EN)
#define __HAL_RCC_CRC1_CLK_ENABLE()       (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_CRC1EN)
#define __HAL_RCC_USBH_CLK_ENABLE()       (RCC->MP_AHB6ENSETR = RCC_MC_AHB6ENSETR_USBHEN)

#define __HAL_RCC_MDMA_CLK_DISABLE()      (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_MDMAEN)
#define __HAL_RCC_GPU_CLK_DISABLE()       (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_GPUEN)
#define __HAL_RCC_ETH1CK_CLK_DISABLE()    (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHCKEN)
#define __HAL_RCC_ETH1TX_CLK_DISABLE()    (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHTXEN)
#define __HAL_RCC_ETH1RX_CLK_DISABLE()    (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHRXEN)
#define __HAL_RCC_ETH1MAC_CLK_DISABLE()   (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHMACEN)
#define __HAL_RCC_FMC_CLK_DISABLE()       (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_FMCEN)
#define __HAL_RCC_QSPI_CLK_DISABLE()      (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_QSPIEN)
#define __HAL_RCC_SDMMC1_CLK_DISABLE()    (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_SDMMC1EN)
#define __HAL_RCC_SDMMC2_CLK_DISABLE()    (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_SDMMC2EN)
#define __HAL_RCC_CRC1_CLK_DISABLE()      (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_CRC1EN)
#define __HAL_RCC_USBH_CLK_DISABLE()      (RCC->MP_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_USBHEN)

/** @brief  Enable or disable the AHB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_DMA1_CLK_ENABLE()       (RCC->MP_AHB2ENSETR = RCC_MC_AHB2ENSETR_DMA1EN)
#define __HAL_RCC_DMA2_CLK_ENABLE()       (RCC->MP_AHB2ENSETR = RCC_MC_AHB2ENSETR_DMA2EN)
#define __HAL_RCC_DMAMUX_CLK_ENABLE()     (RCC->MP_AHB2ENSETR = RCC_MC_AHB2ENSETR_DMAMUXEN)
#define __HAL_RCC_ADC12_CLK_ENABLE()      (RCC->MP_AHB2ENSETR = RCC_MC_AHB2ENSETR_ADC12EN)
#define __HAL_RCC_USBO_CLK_ENABLE()       (RCC->MP_AHB2ENSETR = RCC_MC_AHB2ENSETR_USBOEN)
#define __HAL_RCC_SDMMC3_CLK_ENABLE()     (RCC->MP_AHB2ENSETR = RCC_MC_AHB2ENSETR_SDMMC3EN)

#define __HAL_RCC_DMA1_CLK_DISABLE()      (RCC->MP_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_DMA1EN)
#define __HAL_RCC_DMA2_CLK_DISABLE()      (RCC->MP_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_DMA2EN)
#define __HAL_RCC_DMAMUX_CLK_DISABLE()    (RCC->MP_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_DMAMUXEN)
#define __HAL_RCC_ADC12_CLK_DISABLE()     (RCC->MP_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_ADC12EN)
#define __HAL_RCC_USBO_CLK_DISABLE()      (RCC->MP_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_USBOEN)
#define __HAL_RCC_SDMMC3_CLK_DISABLE()    (RCC->MP_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_SDMMC3EN)

/** @brief  Enable or disable the AHB3 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_DCMI_CLK_ENABLE()       (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_DCMIEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_ENABLE()      (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_CRYP2EN)
#endif
#define __HAL_RCC_HASH2_CLK_ENABLE()      (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_HASH2EN)
#define __HAL_RCC_RNG2_CLK_ENABLE()       (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_RNG2EN)
#define __HAL_RCC_CRC2_CLK_ENABLE()       (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_CRC2EN)
#define __HAL_RCC_HSEM_CLK_ENABLE()       (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_HSEMEN)
#define __HAL_RCC_IPCC_CLK_ENABLE()       (RCC->MP_AHB3ENSETR = RCC_MC_AHB3ENSETR_IPCCEN)

#define __HAL_RCC_DCMI_CLK_DISABLE()      (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_DCMIEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_DISABLE()     (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_CRYP2EN)
#endif
#define __HAL_RCC_HASH2_CLK_DISABLE()     (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_HASH2EN)
#define __HAL_RCC_RNG2_CLK_DISABLE()      (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_RNG2EN)
#define __HAL_RCC_CRC2_CLK_DISABLE()      (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_CRC2EN)
#define __HAL_RCC_HSEM_CLK_DISABLE()      (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_HSEMEN)
#define __HAL_RCC_IPCC_CLK_DISABLE()      (RCC->MP_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_IPCCEN)

/** @brief  Enable or disable the AHB4 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_GPIOA_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOAEN)
#define __HAL_RCC_GPIOB_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOBEN)
#define __HAL_RCC_GPIOC_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOCEN)
#define __HAL_RCC_GPIOD_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIODEN)
#define __HAL_RCC_GPIOE_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOEEN)
#define __HAL_RCC_GPIOF_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOFEN)
#define __HAL_RCC_GPIOG_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOGEN)
#define __HAL_RCC_GPIOH_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOHEN)
#define __HAL_RCC_GPIOI_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOIEN)
#define __HAL_RCC_GPIOJ_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOJEN)
#define __HAL_RCC_GPIOK_CLK_ENABLE()      (RCC->MP_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOKEN)

#define __HAL_RCC_GPIOA_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOAEN)
#define __HAL_RCC_GPIOB_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOBEN)
#define __HAL_RCC_GPIOC_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOCEN)
#define __HAL_RCC_GPIOD_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIODEN)
#define __HAL_RCC_GPIOE_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOEEN)
#define __HAL_RCC_GPIOF_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOFEN)
#define __HAL_RCC_GPIOG_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOGEN)
#define __HAL_RCC_GPIOH_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOHEN)
#define __HAL_RCC_GPIOI_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOIEN)
#define __HAL_RCC_GPIOJ_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOJEN)
#define __HAL_RCC_GPIOK_CLK_DISABLE()     (RCC->MP_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOKEN)


/** @brief  Enable or disable the MLAHB peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_RETRAM_CLK_ENABLE()     (RCC->MP_MLAHBENSETR = RCC_MC_MLAHBENSETR_RETRAMEN)

#define __HAL_RCC_RETRAM_CLK_DISABLE()    (RCC->MP_MLAHBENCLRR = RCC_MC_MLAHBENCLRR_RETRAMEN)

/** @brief  MCU reset
  * @note   It generates a reset of the MCU core
  *         MCURST bit is set by software, cleared by hardware when the reset
  *         command is executed.
  */
#define __HAL_RCC_MCU_RESET()       (RCC->MP_GRSTCSETR = RCC_MP_GRSTCSETR_MCURST)

/** @brief  System reset
  * @note   It generates a system reset
  *         MPSYSRST bit is set by software, cleared by hardware.
  */
#define __HAL_RCC_SYS_RESET()       (RCC->MP_GRSTCSETR = RCC_MP_GRSTCSETR_MPSYSRST)




/** @brief  Enable or disable the APB1 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM2LPEN)
#define __HAL_RCC_TIM3_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM3LPEN)
#define __HAL_RCC_TIM4_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM4LPEN)
#define __HAL_RCC_TIM5_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM5LPEN)
#define __HAL_RCC_TIM6_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM6LPEN)
#define __HAL_RCC_TIM7_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM7LPEN)
#define __HAL_RCC_TIM12_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM12LPEN)
#define __HAL_RCC_TIM13_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM13LPEN)
#define __HAL_RCC_TIM14_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM14LPEN)
#define __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE()       (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_LPTIM1LPEN)
#define __HAL_RCC_SPI2_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_SPI2LPEN)
#define __HAL_RCC_SPI3_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_SPI3LPEN)
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()       (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_USART2LPEN)
#define __HAL_RCC_USART3_CLK_SLEEP_ENABLE()       (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_USART3LPEN)
#define __HAL_RCC_UART4_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART4LPEN)
#define __HAL_RCC_UART5_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART5LPEN)
#define __HAL_RCC_UART7_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART7LPEN)
#define __HAL_RCC_UART8_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART8LPEN)
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C1LPEN)
#define __HAL_RCC_I2C2_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C2LPEN)
#define __HAL_RCC_I2C3_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C3LPEN)
#define __HAL_RCC_I2C5_CLK_SLEEP_ENABLE()         (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C5LPEN)
#define __HAL_RCC_SPDIFRX_CLK_SLEEP_ENABLE()      (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_SPDIFLPEN)
#define __HAL_RCC_CEC_CLK_SLEEP_ENABLE()          (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_CECLPEN)
#define __HAL_RCC_DAC12_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_DAC12LPEN)
#define __HAL_RCC_MDIOS_CLK_SLEEP_ENABLE()        (RCC->MP_APB1LPENSETR = RCC_MC_APB1LPENSETR_MDIOSLPEN)

#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM2LPEN)
#define __HAL_RCC_TIM3_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM3LPEN)
#define __HAL_RCC_TIM4_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM4LPEN)
#define __HAL_RCC_TIM5_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM5LPEN)
#define __HAL_RCC_TIM6_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM6LPEN)
#define __HAL_RCC_TIM7_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM7LPEN)
#define __HAL_RCC_TIM12_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM12LPEN)
#define __HAL_RCC_TIM13_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM13LPEN)
#define __HAL_RCC_TIM14_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM14LPEN)
#define __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE()      (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_LPTIM1LPEN)
#define __HAL_RCC_SPI2_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_SPI2LPEN)
#define __HAL_RCC_SPI3_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_SPI3LPEN)
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()      (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_USART2LPEN)
#define __HAL_RCC_USART3_CLK_SLEEP_DISABLE()      (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_USART3LPEN)
#define __HAL_RCC_UART4_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART4LPEN)
#define __HAL_RCC_UART5_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART5LPEN)
#define __HAL_RCC_UART7_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART7LPEN)
#define __HAL_RCC_UART8_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART8LPEN)
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C1LPEN)
#define __HAL_RCC_I2C2_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C2LPEN)
#define __HAL_RCC_I2C3_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C3LPEN)
#define __HAL_RCC_I2C5_CLK_SLEEP_DISABLE()        (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C5LPEN)
#define __HAL_RCC_SPDIFRX_CLK_SLEEP_DISABLE()     (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_SPDIFLPEN)
#define __HAL_RCC_CEC_CLK_SLEEP_DISABLE()         (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_CECLPEN)
#define __HAL_RCC_DAC12_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_DAC12LPEN)
#define __HAL_RCC_MDIOS_CLK_SLEEP_DISABLE()       (RCC->MP_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_MDIOSLPEN)


/** @brief  Enable or disable the APB2 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_TIM1_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM1LPEN)
#define __HAL_RCC_TIM8_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM8LPEN)
#define __HAL_RCC_TIM15_CLK_SLEEP_ENABLE()        (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM15LPEN)
#define __HAL_RCC_TIM16_CLK_SLEEP_ENABLE()        (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM16LPEN)
#define __HAL_RCC_TIM17_CLK_SLEEP_ENABLE()        (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM17LPEN)
#define __HAL_RCC_SPI1_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_SPI1LPEN)
#define __HAL_RCC_SPI4_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_SPI4LPEN)
#define __HAL_RCC_SPI5_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_SPI5LPEN)
#define __HAL_RCC_USART6_CLK_SLEEP_ENABLE()       (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_USART6LPEN)
#define __HAL_RCC_SAI1_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_SAI1LPEN)
#define __HAL_RCC_SAI2_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_SAI2LPEN)
#define __HAL_RCC_SAI3_CLK_SLEEP_ENABLE()         (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_SAI3LPEN)
#define __HAL_RCC_DFSDM1_CLK_SLEEP_ENABLE()       (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_DFSDMLPEN)
#define __HAL_RCC_ADFSDM1_CLK_SLEEP_ENABLE()      (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_ADFSDMLPEN)
#define __HAL_RCC_FDCAN_CLK_SLEEP_ENABLE()        (RCC->MP_APB2LPENSETR = RCC_MC_APB2LPENSETR_FDCANLPEN)

#define __HAL_RCC_TIM1_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM1LPEN)
#define __HAL_RCC_TIM8_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM8LPEN)
#define __HAL_RCC_TIM15_CLK_SLEEP_DISABLE()       (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM15LPEN)
#define __HAL_RCC_TIM16_CLK_SLEEP_DISABLE()       (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM16LPEN)
#define __HAL_RCC_TIM17_CLK_SLEEP_DISABLE()       (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM17LPEN)
#define __HAL_RCC_SPI1_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SPI1LPEN)
#define __HAL_RCC_SPI4_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SPI4LPEN)
#define __HAL_RCC_SPI5_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SPI5LPEN)
#define __HAL_RCC_USART6_CLK_SLEEP_DISABLE()      (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_USART6LPEN)
#define __HAL_RCC_SAI1_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SAI1LPEN)
#define __HAL_RCC_SAI2_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SAI2LPEN)
#define __HAL_RCC_SAI3_CLK_SLEEP_DISABLE()        (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SAI3LPEN)
#define __HAL_RCC_DFSDM1_CLK_SLEEP_DISABLE()      (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_DFSDMLPEN)
#define __HAL_RCC_ADFSDM1_CLK_SLEEP_DISABLE()     (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_ADFSDMLPEN)
#define __HAL_RCC_FDCAN_CLK_SLEEP_DISABLE()       (RCC->MP_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_FDCANLPEN)


/** @brief  Enable or disable the APB3 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_LPTIM2_CLK_SLEEP_ENABLE()       (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM2LPEN)
#define __HAL_RCC_LPTIM3_CLK_SLEEP_ENABLE()       (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM3LPEN)
#define __HAL_RCC_LPTIM4_CLK_SLEEP_ENABLE()       (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM4LPEN)
#define __HAL_RCC_LPTIM5_CLK_SLEEP_ENABLE()       (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM5LPEN)
#define __HAL_RCC_SAI4_CLK_SLEEP_ENABLE()         (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_SAI4LPEN)
#define __HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE()       (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_SYSCFGLPEN)
#define __HAL_RCC_VREF_CLK_SLEEP_ENABLE()         (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_VREFLPEN)
#define __HAL_RCC_TMPSENS_CLK_SLEEP_ENABLE()      (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_TMPSENSLPEN)
#define __HAL_RCC_PMBCTRL_CLK_SLEEP_ENABLE()      (RCC->MP_APB3LPENSETR = RCC_MC_APB3LPENSETR_PMBCTRLLPEN)

#define __HAL_RCC_LPTIM2_CLK_SLEEP_DISABLE()      (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM2LPEN)
#define __HAL_RCC_LPTIM3_CLK_SLEEP_DISABLE()      (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM3LPEN)
#define __HAL_RCC_LPTIM4_CLK_SLEEP_DISABLE()      (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM4LPEN)
#define __HAL_RCC_LPTIM5_CLK_SLEEP_DISABLE()      (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM5LPEN)
#define __HAL_RCC_SAI4_CLK_SLEEP_DISABLE()        (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_SAI4LPEN)
#define __HAL_RCC_SYSCFG_CLK_SLEEP_DISABLE()      (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_SYSCFGLPEN)
#define __HAL_RCC_VREF_CLK_SLEEP_DISABLE()        (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_VREFLPEN)
#define __HAL_RCC_TMPSENS_CLK_SLEEP_DISABLE()     (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_TMPSENSLPEN)
#define __HAL_RCC_PMBCTRL_CLK_SLEEP_DISABLE()     (RCC->MP_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_PMBCTRLLPEN)

/** @brief  Enable or disable the APB4 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_LTDC_CLK_SLEEP_ENABLE()         (RCC->MP_APB4LPENSETR = RCC_MC_APB4LPENSETR_LTDCLPEN)
#define __HAL_RCC_DSI_CLK_SLEEP_ENABLE()          (RCC->MP_APB4LPENSETR = RCC_MC_APB4LPENSETR_DSILPEN)
#define __HAL_RCC_DDRPERFM_CLK_SLEEP_ENABLE()     (RCC->MP_APB4LPENSETR = RCC_MC_APB4LPENSETR_DDRPERFMLPEN)
#define __HAL_RCC_IWDG2APB_CLK_SLEEP_ENABLE()     (RCC->MP_APB4LPENSETR = RCC_MP_APB4LPENSETR_IWDG2APBLPEN)
#define __HAL_RCC_USBPHY_CLK_SLEEP_ENABLE()       (RCC->MP_APB4LPENSETR = RCC_MC_APB4LPENSETR_USBPHYLPEN)
#define __HAL_RCC_STGENRO_CLK_SLEEP_ENABLE()      (RCC->MP_APB4LPENSETR = RCC_MC_APB4LPENSETR_STGENROLPEN)
#define __HAL_RCC_STGENRO_CLK_STOP_ENABLE()       (RCC->MP_APB4LPENSETR = RCC_MC_APB4LPENSETR_STGENROSTPEN)

#define __HAL_RCC_LTDC_CLK_SLEEP_DISABLE()        (RCC->MP_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_LTDCLPEN)
#define __HAL_RCC_DSI_CLK_SLEEP_DISABLE()         (RCC->MP_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_DSILPEN)
#define __HAL_RCC_DDRPERFM_CLK_SLEEP_DISABLE()    (RCC->MP_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_DDRPERFMLPEN)
#define __HAL_RCC_IWDG2APB_CLK_SLEEP_DISABLE()    (RCC->MP_APB4LPENCLRR = RCC_MP_APB4LPENCLRR_IWDG2APBLPEN)
#define __HAL_RCC_USBPHY_CLK_SLEEP_DISABLE()      (RCC->MP_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_USBPHYLPEN)
#define __HAL_RCC_STGENRO_CLK_SLEEP_DISABLE()     (RCC->MP_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_STGENROLPEN)
#define __HAL_RCC_STGENRO_CLK_STOP_DISABLE()      (RCC->MP_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_STGENROSTPEN)

/** @brief  Enable or disable the APB5 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_SPI6_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_SPI6LPEN)
#define __HAL_RCC_I2C4_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_I2C4LPEN)
#define __HAL_RCC_I2C6_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_I2C6LPEN)
#define __HAL_RCC_USART1_CLK_SLEEP_ENABLE()       (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_USART1LPEN)
#define __HAL_RCC_RTCAPB_CLK_SLEEP_ENABLE()       (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_RTCAPBLPEN)
#define __HAL_RCC_TZC1_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_TZC1LPEN)
#define __HAL_RCC_TZC2_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_TZC2LPEN)
#define __HAL_RCC_TZPC_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_TZPCLPEN)
#define __HAL_RCC_BSEC_CLK_SLEEP_ENABLE()         (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_BSECLPEN)
#define __HAL_RCC_STGEN_CLK_SLEEP_ENABLE()        (RCC->MP_APB5LPENSETR = RCC_MC_APB5LPENSETR_STGENLPEN)

#define __HAL_RCC_SPI6_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_SPI6LPEN)
#define __HAL_RCC_I2C4_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_I2C4LPEN)
#define __HAL_RCC_I2C6_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_I2C6LPEN)
#define __HAL_RCC_USART1_CLK_SLEEP_DISABLE()      (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_USART1LPEN)
#define __HAL_RCC_RTCAPB_CLK_SLEEP_DISABLE()      (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_RTCAPBLPEN)
#define __HAL_RCC_TZC1_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_TZC1LPEN)
#define __HAL_RCC_TZC2_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_TZC2LPEN)
#define __HAL_RCC_TZPC_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_TZPCLPEN)
#define __HAL_RCC_BSEC_CLK_SLEEP_DISABLE()        (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENSETR_BSECLPEN)
#define __HAL_RCC_STGEN_CLK_SLEEP_DISABLE()       (RCC->MP_APB5LPENCLRR = RCC_MC_APB5LPENSETR_STGENLPEN)


/** @brief  Enable or disable the AHB5 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_GPIOZ_CLK_SLEEP_ENABLE()        (RCC->MP_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_GPIOZLPEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_SLEEP_ENABLE()        (RCC->MP_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_CRYP1LPEN)
#endif
#define __HAL_RCC_HASH1_CLK_SLEEP_ENABLE()        (RCC->MP_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_HASH1LPEN)
#define __HAL_RCC_RNG1_CLK_SLEEP_ENABLE()         (RCC->MP_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_RNG1LPEN)
#define __HAL_RCC_BKPSRAM_CLK_SLEEP_ENABLE()      (RCC->MP_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_BKPSRAMLPEN)

#define __HAL_RCC_GPIOZ_CLK_SLEEP_DISABLE()       (RCC->MP_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_GPIOZLPEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_SLEEP_DISABLE()       (RCC->MP_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_CRYP1LPEN)
#endif
#define __HAL_RCC_HASH1_CLK_SLEEP_DISABLE()       (RCC->MP_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_HASH1LPEN)
#define __HAL_RCC_RNG1_CLK_SLEEP_DISABLE()        (RCC->MP_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_RNG1LPEN)
#define __HAL_RCC_BKPSRAM_CLK_SLEEP_DISABLE()     (RCC->MP_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_BKPSRAMLPEN)


/** @brief  Enable or disable the AHB6 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_MDMA_CLK_SLEEP_ENABLE()         (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_MDMALPEN)
#define __HAL_RCC_GPU_CLK_SLEEP_ENABLE()          (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_GPULPEN)
#define __HAL_RCC_ETH1CK_CLK_SLEEP_ENABLE()       (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHCKLPEN)
#define __HAL_RCC_ETH1TX_CLK_SLEEP_ENABLE()       (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHTXLPEN)
#define __HAL_RCC_ETH1RX_CLK_SLEEP_ENABLE()       (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHRXLPEN)
#define __HAL_RCC_ETH1MAC_CLK_SLEEP_ENABLE()      (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHMACLPEN)
#define __HAL_RCC_FMC_CLK_SLEEP_ENABLE()          (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_FMCLPEN)
#define __HAL_RCC_QSPI_CLK_SLEEP_ENABLE()         (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_QSPILPEN)
#define __HAL_RCC_SDMMC1_CLK_SLEEP_ENABLE()       (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_SDMMC1LPEN)
#define __HAL_RCC_SDMMC2_CLK_SLEEP_ENABLE()       (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_SDMMC2LPEN)
#define __HAL_RCC_CRC1_CLK_SLEEP_ENABLE()         (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_CRC1LPEN)
#define __HAL_RCC_USBH_CLK_SLEEP_ENABLE()         (RCC->MP_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_USBHLPEN)

#define __HAL_RCC_MDMA_CLK_SLEEP_DISABLE()        (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_MDMALPEN)
#define __HAL_RCC_GPU_CLK_SLEEP_DISABLE()         (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_GPULPEN)
#define __HAL_RCC_ETH1CK_CLK_SLEEP_DISABLE()      (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHCKLPEN)
#define __HAL_RCC_ETH1TX_CLK_SLEEP_DISABLE()      (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHTXLPEN)
#define __HAL_RCC_ETH1RX_CLK_SLEEP_DISABLE()      (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHRXLPEN)
#define __HAL_RCC_ETH1MAC_CLK_SLEEP_DISABLE()     (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHMACLPEN)
#define __HAL_RCC_FMC_CLK_SLEEP_DISABLE()         (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_FMCLPEN)
#define __HAL_RCC_QSPI_CLK_SLEEP_DISABLE()        (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_QSPILPEN)
#define __HAL_RCC_SDMMC1_CLK_SLEEP_DISABLE()      (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_SDMMC1LPEN)
#define __HAL_RCC_SDMMC2_CLK_SLEEP_DISABLE()      (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_SDMMC2LPEN)
#define __HAL_RCC_CRC1_CLK_SLEEP_DISABLE()        (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_CRC1LPEN)
#define __HAL_RCC_USBH_CLK_SLEEP_DISABLE()        (RCC->MP_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_USBHLPEN)


/** @brief  Enable or disable the AHB2 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_DMA1_CLK_SLEEP_ENABLE()         (RCC->MP_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_DMA1LPEN)
#define __HAL_RCC_DMA2_CLK_SLEEP_ENABLE()         (RCC->MP_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_DMA2LPEN)
#define __HAL_RCC_DMAMUX_CLK_SLEEP_ENABLE()       (RCC->MP_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_DMAMUXLPEN)
#define __HAL_RCC_ADC12_CLK_SLEEP_ENABLE()        (RCC->MP_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_ADC12LPEN)
#define __HAL_RCC_USBO_CLK_SLEEP_ENABLE()         (RCC->MP_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_USBOLPEN)
#define __HAL_RCC_SDMMC3_CLK_SLEEP_ENABLE()       (RCC->MP_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_SDMMC3LPEN)

#define __HAL_RCC_DMA1_CLK_SLEEP_DISABLE()        (RCC->MP_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_DMA1LPEN)
#define __HAL_RCC_DMA2_CLK_SLEEP_DISABLE()        (RCC->MP_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_DMA2LPEN)
#define __HAL_RCC_DMAMUX_CLK_SLEEP_DISABLE()      (RCC->MP_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_DMAMUXLPEN)
#define __HAL_RCC_ADC12_CLK_SLEEP_DISABLE()       (RCC->MP_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_ADC12LPEN)
#define __HAL_RCC_USBO_CLK_SLEEP_DISABLE()        (RCC->MP_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_USBOLPEN)
#define __HAL_RCC_SDMMC3_CLK_SLEEP_DISABLE()      (RCC->MP_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_SDMMC3LPEN)

/** @brief  Enable or disable the AHB3 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_DCMI_CLK_SLEEP_ENABLE()         (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_DCMILPEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_SLEEP_ENABLE()        (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_CRYP2LPEN)
#endif
#define __HAL_RCC_HASH2_CLK_SLEEP_ENABLE()        (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_HASH2LPEN)
#define __HAL_RCC_RNG2_CLK_SLEEP_ENABLE()         (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_RNG2LPEN)
#define __HAL_RCC_CRC2_CLK_SLEEP_ENABLE()         (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_CRC2LPEN)
#define __HAL_RCC_HSEM_CLK_SLEEP_ENABLE()         (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_HSEMLPEN)
#define __HAL_RCC_IPCC_CLK_SLEEP_ENABLE()         (RCC->MP_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_IPCCLPEN)

#define __HAL_RCC_DCMI_CLK_SLEEP_DISABLE()        (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_DCMILPEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_SLEEP_DISABLE()       (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_CRYP2LPEN)
#endif
#define __HAL_RCC_HASH2_CLK_SLEEP_DISABLE()       (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_HASH2LPEN)
#define __HAL_RCC_RNG2_CLK_SLEEP_DISABLE()        (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_RNG2LPEN)
#define __HAL_RCC_CRC2_CLK_SLEEP_DISABLE()        (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_CRC2LPEN)
#define __HAL_RCC_HSEM_CLK_SLEEP_DISABLE()        (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_HSEMLPEN)
#define __HAL_RCC_IPCC_CLK_SLEEP_DISABLE()        (RCC->MP_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_IPCCLPEN)


/** @brief  Enable or disable the AHB4 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_GPIOA_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOALPEN)
#define __HAL_RCC_GPIOB_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOBLPEN)
#define __HAL_RCC_GPIOC_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOCLPEN)
#define __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIODLPEN)
#define __HAL_RCC_GPIOE_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOELPEN)
#define __HAL_RCC_GPIOF_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOFLPEN)
#define __HAL_RCC_GPIOG_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOGLPEN)
#define __HAL_RCC_GPIOH_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOHLPEN)
#define __HAL_RCC_GPIOI_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOILPEN)
#define __HAL_RCC_GPIOJ_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOJLPEN)
#define __HAL_RCC_GPIOK_CLK_SLEEP_ENABLE()        (RCC->MP_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOKLPEN)

#define __HAL_RCC_GPIOA_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOALPEN)
#define __HAL_RCC_GPIOB_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOBLPEN)
#define __HAL_RCC_GPIOC_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOCLPEN)
#define __HAL_RCC_GPIOD_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIODLPEN)
#define __HAL_RCC_GPIOE_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOELPEN)
#define __HAL_RCC_GPIOF_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOFLPEN)
#define __HAL_RCC_GPIOG_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOGLPEN)
#define __HAL_RCC_GPIOH_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOHLPEN)
#define __HAL_RCC_GPIOI_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOILPEN)
#define __HAL_RCC_GPIOJ_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOJLPEN)
#define __HAL_RCC_GPIOK_CLK_SLEEP_DISABLE()       (RCC->MP_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOKLPEN)

/** @brief  Enable or disable the AXI peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_SYSRAM_CLK_SLEEP_ENABLE()       (RCC->MP_AXIMLPENSETR = RCC_MC_AXIMLPENSETR_SYSRAMLPEN)

#define __HAL_RCC_SYSRAM_CLK_SLEEP_DISABLE()      (RCC->MP_AXIMLPENCLRR = RCC_MC_AXIMLPENCLRR_SYSRAMLPEN)


/** @brief  Enable or disable the MLAHB peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_RETRAM_CLK_SLEEP_ENABLE()       (RCC->MP_MLAHBLPENSETR = RCC_MC_MLAHBLPENSETR_RETRAMLPEN)

#define __HAL_RCC_RETRAM_CLK_SLEEP_DISABLE()      (RCC->MP_MLAHBLPENCLRR = RCC_MC_MLAHBLPENCLRR_RETRAMLPEN)





#else /* CORE_CA7 */




/** @brief  Enable or disable the APB1 peripheral clock.
 * @note   After reset, the peripheral clock (used for registers read/write access)
 *         is disabled and the application software has to enable this clock before
 *         using it. It shall be used to allocate a peripheral to the MCU.
 */
#define __HAL_RCC_TIM2_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM2EN)
#define __HAL_RCC_TIM3_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM3EN)
#define __HAL_RCC_TIM4_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM4EN)
#define __HAL_RCC_TIM5_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM5EN)
#define __HAL_RCC_TIM6_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM6EN)
#define __HAL_RCC_TIM7_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM7EN)
#define __HAL_RCC_TIM12_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM12EN)
#define __HAL_RCC_TIM13_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM13EN)
#define __HAL_RCC_TIM14_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_TIM14EN)
#define __HAL_RCC_LPTIM1_CLK_ENABLE()       (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_LPTIM1EN)
#define __HAL_RCC_SPI2_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_SPI2EN)
#define __HAL_RCC_SPI3_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_SPI3EN)
#define __HAL_RCC_USART2_CLK_ENABLE()       (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_USART2EN)
#define __HAL_RCC_USART3_CLK_ENABLE()       (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_USART3EN)
#define __HAL_RCC_UART4_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_UART4EN)
#define __HAL_RCC_UART5_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_UART5EN)
#define __HAL_RCC_UART7_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_UART7EN)
#define __HAL_RCC_UART8_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_UART8EN)
#define __HAL_RCC_I2C1_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_I2C1EN)
#define __HAL_RCC_I2C2_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_I2C2EN)
#define __HAL_RCC_I2C3_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_I2C3EN)
#define __HAL_RCC_I2C5_CLK_ENABLE()         (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_I2C5EN)
#define __HAL_RCC_SPDIFRX_CLK_ENABLE()      (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_SPDIFEN)
#define __HAL_RCC_CEC_CLK_ENABLE()          (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_CECEN)
#define __HAL_RCC_WWDG1_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_WWDG1EN)
#define __HAL_RCC_DAC12_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_DAC12EN)
#define __HAL_RCC_MDIOS_CLK_ENABLE()        (RCC->MC_APB1ENSETR = RCC_MC_APB1ENSETR_MDIOSEN)

#define __HAL_RCC_TIM2_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM2EN)
#define __HAL_RCC_TIM3_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM3EN)
#define __HAL_RCC_TIM4_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM4EN)
#define __HAL_RCC_TIM5_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM5EN)
#define __HAL_RCC_TIM6_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM6EN)
#define __HAL_RCC_TIM7_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM7EN)
#define __HAL_RCC_TIM12_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM12EN)
#define __HAL_RCC_TIM13_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM13EN)
#define __HAL_RCC_TIM14_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_TIM14EN)
#define __HAL_RCC_LPTIM1_CLK_DISABLE()      (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_LPTIM1EN)
#define __HAL_RCC_SPI2_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_SPI2EN)
#define __HAL_RCC_SPI3_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_SPI3EN)
#define __HAL_RCC_USART2_CLK_DISABLE()      (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_USART2EN)
#define __HAL_RCC_USART3_CLK_DISABLE()      (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_USART3EN)
#define __HAL_RCC_UART4_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART4EN)
#define __HAL_RCC_UART5_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART5EN)
#define __HAL_RCC_UART7_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART7EN)
#define __HAL_RCC_UART8_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_UART8EN)
#define __HAL_RCC_I2C1_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C1EN)
#define __HAL_RCC_I2C2_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C2EN)
#define __HAL_RCC_I2C3_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C3EN)
#define __HAL_RCC_I2C5_CLK_DISABLE()        (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_I2C5EN)
#define __HAL_RCC_SPDIFRX_CLK_DISABLE()     (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_SPDIFEN)
#define __HAL_RCC_CEC_CLK_DISABLE()         (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_CECEN)
#define __HAL_RCC_WWDG1_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_WWDG1EN)
#define __HAL_RCC_DAC12_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_DAC12EN)
#define __HAL_RCC_MDIOS_CLK_DISABLE()       (RCC->MC_APB1ENCLRR = RCC_MC_APB1ENCLRR_MDIOSEN)

/** @brief  Enable or disable the APB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_TIM1_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_TIM1EN)
#define __HAL_RCC_TIM8_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_TIM8EN)
#define __HAL_RCC_TIM15_CLK_ENABLE()        (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_TIM15EN)
#define __HAL_RCC_TIM16_CLK_ENABLE()        (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_TIM16EN)
#define __HAL_RCC_TIM17_CLK_ENABLE()        (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_TIM17EN)
#define __HAL_RCC_SPI1_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_SPI1EN)
#define __HAL_RCC_SPI4_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_SPI4EN)
#define __HAL_RCC_SPI5_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_SPI5EN)
#define __HAL_RCC_USART6_CLK_ENABLE()       (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_USART6EN)
#define __HAL_RCC_SAI1_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_SAI1EN)
#define __HAL_RCC_SAI2_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_SAI2EN)
#define __HAL_RCC_SAI3_CLK_ENABLE()         (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_SAI3EN)
#define __HAL_RCC_DFSDM1_CLK_ENABLE()       (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_DFSDMEN)
#define __HAL_RCC_ADFSDM1_CLK_ENABLE()      (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_ADFSDMEN)
#define __HAL_RCC_FDCAN_CLK_ENABLE()        (RCC->MC_APB2ENSETR = RCC_MC_APB2ENSETR_FDCANEN)

#define __HAL_RCC_TIM1_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM1EN)
#define __HAL_RCC_TIM8_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM8EN)
#define __HAL_RCC_TIM15_CLK_DISABLE()       (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM15EN)
#define __HAL_RCC_TIM16_CLK_DISABLE()       (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM16EN)
#define __HAL_RCC_TIM17_CLK_DISABLE()       (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_TIM17EN)
#define __HAL_RCC_SPI1_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_SPI1EN)
#define __HAL_RCC_SPI4_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_SPI4EN)
#define __HAL_RCC_SPI5_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_SPI5EN)
#define __HAL_RCC_USART6_CLK_DISABLE()      (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_USART6EN)
#define __HAL_RCC_SAI1_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_SAI1EN)
#define __HAL_RCC_SAI2_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_SAI2EN)
#define __HAL_RCC_SAI3_CLK_DISABLE()        (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_SAI3EN)
#define __HAL_RCC_DFSDM1_CLK_DISABLE()      (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_DFSDMEN)
#define __HAL_RCC_ADFSDM1_CLK_DISABLE()     (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_ADFSDMEN)
#define __HAL_RCC_FDCAN_CLK_DISABLE()       (RCC->MC_APB2ENCLRR = RCC_MC_APB2ENCLRR_FDCANEN)

/** @brief  Enable or disable the APB3 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_LPTIM2_CLK_ENABLE()       (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM2EN)
#define __HAL_RCC_LPTIM3_CLK_ENABLE()       (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM3EN)
#define __HAL_RCC_LPTIM4_CLK_ENABLE()       (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM4EN)
#define __HAL_RCC_LPTIM5_CLK_ENABLE()       (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_LPTIM5EN)
#define __HAL_RCC_SAI4_CLK_ENABLE()         (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_SAI4EN)
#define __HAL_RCC_SYSCFG_CLK_ENABLE()       (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_SYSCFGEN)
#define __HAL_RCC_VREF_CLK_ENABLE()         (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_VREFEN)
#define __HAL_RCC_TMPSENS_CLK_ENABLE()      (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_TMPSENSEN)
#define __HAL_RCC_PMBCTRL_CLK_ENABLE()      (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_PMBCTRLEN)
#define __HAL_RCC_HDP_CLK_ENABLE()          (RCC->MC_APB3ENSETR = RCC_MC_APB3ENSETR_HDPEN)

#define __HAL_RCC_LPTIM2_CLK_DISABLE()      (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM2EN)
#define __HAL_RCC_LPTIM3_CLK_DISABLE()      (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM3EN)
#define __HAL_RCC_LPTIM4_CLK_DISABLE()      (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM4EN)
#define __HAL_RCC_LPTIM5_CLK_DISABLE()      (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_LPTIM5EN)
#define __HAL_RCC_SAI4_CLK_DISABLE()        (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_SAI4EN)
#define __HAL_RCC_SYSCFG_CLK_DISABLE()      (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_SYSCFGEN)
#define __HAL_RCC_VREF_CLK_DISABLE()        (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_VREFEN)
#define __HAL_RCC_TMPSENS_CLK_DISABLE()     (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_TMPSENSEN)
#define __HAL_RCC_PMBCTRL_CLK_DISABLE()     (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_PMBCTRLEN)
#define __HAL_RCC_HDP_CLK_DISABLE()         (RCC->MC_APB3ENCLRR = RCC_MC_APB3ENCLRR_HDPEN)

/** @brief  Enable or disable the APB4 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_LTDC_CLK_ENABLE()         (RCC->MC_APB4ENSETR = RCC_MC_APB4ENSETR_LTDCEN)
#define __HAL_RCC_DSI_CLK_ENABLE()          (RCC->MC_APB4ENSETR = RCC_MC_APB4ENSETR_DSIEN)
#define __HAL_RCC_DDRPERFM_CLK_ENABLE()     (RCC->MC_APB4ENSETR = RCC_MC_APB4ENSETR_DDRPERFMEN)
#define __HAL_RCC_USBPHY_CLK_ENABLE()       (RCC->MC_APB4ENSETR = RCC_MC_APB4ENSETR_USBPHYEN)
#define __HAL_RCC_STGENRO_CLK_ENABLE()      (RCC->MC_APB4ENSETR = RCC_MC_APB4ENSETR_STGENROEN)

#define __HAL_RCC_LTDC_CLK_DISABLE()        (RCC->MC_APB4ENCLRR = RCC_MC_APB4ENCLRR_LTDCEN)
#define __HAL_RCC_DSI_CLK_DISABLE()         (RCC->MC_APB4ENCLRR = RCC_MC_APB4ENCLRR_DSIEN)
#define __HAL_RCC_DDRPERFM_CLK_DISABLE()    (RCC->MC_APB4ENCLRR = RCC_MC_APB4ENCLRR_DDRPERFMEN)
#define __HAL_RCC_USBPHY_CLK_DISABLE()      (RCC->MC_APB4ENCLRR = RCC_MC_APB4ENCLRR_USBPHYEN)
#define __HAL_RCC_STGENRO_CLK_DISABLE()     (RCC->MC_APB4ENCLRR = RCC_MC_APB4ENCLRR_STGENROEN)

/** @brief  Enable or disable the APB5 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_SPI6_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_SPI6EN)
#define __HAL_RCC_I2C4_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_I2C4EN)
#define __HAL_RCC_I2C6_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_I2C6EN)
#define __HAL_RCC_USART1_CLK_ENABLE()       (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_USART1EN)
#define __HAL_RCC_RTCAPB_CLK_ENABLE()       (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_RTCAPBEN)
#define __HAL_RCC_TZC1_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_TZC1EN)
#define __HAL_RCC_TZC2_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_TZC2EN)
#define __HAL_RCC_TZPC_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_TZPCEN)
#define __HAL_RCC_BSEC_CLK_ENABLE()         (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_BSECEN)
#define __HAL_RCC_STGEN_CLK_ENABLE()        (RCC->MC_APB5ENSETR = RCC_MC_APB5ENSETR_STGENEN)

#define __HAL_RCC_SPI6_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_SPI6EN)
#define __HAL_RCC_I2C4_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_I2C4EN)
#define __HAL_RCC_I2C6_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_I2C6EN)
#define __HAL_RCC_USART1_CLK_DISABLE()      (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_USART1EN)
#define __HAL_RCC_RTCAPB_CLK_DISABLE()      (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_RTCAPBEN)
#define __HAL_RCC_TZC1_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_TZC1EN)
#define __HAL_RCC_TZC2_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_TZC2EN)
#define __HAL_RCC_TZPC_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENCLRR_TZPCEN)
#define __HAL_RCC_BSEC_CLK_DISABLE()        (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENSETR_BSECEN)
#define __HAL_RCC_STGEN_CLK_DISABLE()       (RCC->MC_APB5ENCLRR = RCC_MC_APB5ENSETR_STGENEN)

/** @brief  Enable or disable the AHB5 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_GPIOZ_CLK_ENABLE()        (RCC->MC_AHB5ENSETR = RCC_MC_AHB5ENSETR_GPIOZEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_ENABLE()        (RCC->MC_AHB5ENSETR = RCC_MC_AHB5ENSETR_CRYP1EN)
#endif
#define __HAL_RCC_HASH1_CLK_ENABLE()        (RCC->MC_AHB5ENSETR = RCC_MC_AHB5ENSETR_HASH1EN)
#define __HAL_RCC_RNG1_CLK_ENABLE()         (RCC->MC_AHB5ENSETR = RCC_MC_AHB5ENSETR_RNG1EN)
#define __HAL_RCC_BKPSRAM_CLK_ENABLE()      (RCC->MC_AHB5ENSETR = RCC_MC_AHB5ENSETR_BKPSRAMEN)

#define __HAL_RCC_GPIOZ_CLK_DISABLE()       (RCC->MC_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_GPIOZEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_DISABLE()       (RCC->MC_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_CRYP1EN)
#endif
#define __HAL_RCC_HASH1_CLK_DISABLE()       (RCC->MC_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_HASH1EN)
#define __HAL_RCC_RNG1_CLK_DISABLE()        (RCC->MC_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_RNG1EN)
#define __HAL_RCC_BKPSRAM_CLK_DISABLE()     (RCC->MC_AHB5ENCLRR = RCC_MC_AHB5ENCLRR_BKPSRAMEN)

/** @brief  Enable or disable the AHB6 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_MDMA_CLK_ENABLE()         (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_MDMAEN)
#define __HAL_RCC_GPU_CLK_ENABLE()          (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_GPUEN)
#define __HAL_RCC_ETH1CK_CLK_ENABLE()       (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHCKEN)
#define __HAL_RCC_ETH1TX_CLK_ENABLE()       (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHTXEN)
#define __HAL_RCC_ETH1RX_CLK_ENABLE()       (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHRXEN)
#define __HAL_RCC_ETH1MAC_CLK_ENABLE()      (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_ETHMACEN)
#define __HAL_RCC_FMC_CLK_ENABLE()          (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_FMCEN)
#define __HAL_RCC_QSPI_CLK_ENABLE()         (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_QSPIEN)
#define __HAL_RCC_SDMMC1_CLK_ENABLE()       (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_SDMMC1EN)
#define __HAL_RCC_SDMMC2_CLK_ENABLE()       (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_SDMMC2EN)
#define __HAL_RCC_CRC1_CLK_ENABLE()         (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_CRC1EN)
#define __HAL_RCC_USBH_CLK_ENABLE()         (RCC->MC_AHB6ENSETR = RCC_MC_AHB6ENSETR_USBHEN)

#define __HAL_RCC_MDMA_CLK_DISABLE()        (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_MDMAEN)
#define __HAL_RCC_GPU_CLK_DISABLE()         (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_GPUEN)
#define __HAL_RCC_ETH1CK_CLK_DISABLE()      (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHCKEN)
#define __HAL_RCC_ETH1TX_CLK_DISABLE()      (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHTXEN)
#define __HAL_RCC_ETH1RX_CLK_DISABLE()      (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHRXEN)
#define __HAL_RCC_ETH1MAC_CLK_DISABLE()     (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_ETHMACEN)
#define __HAL_RCC_FMC_CLK_DISABLE()         (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_FMCEN)
#define __HAL_RCC_QSPI_CLK_DISABLE()        (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_QSPIEN)
#define __HAL_RCC_SDMMC1_CLK_DISABLE()      (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_SDMMC1EN)
#define __HAL_RCC_SDMMC2_CLK_DISABLE()      (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_SDMMC2EN)
#define __HAL_RCC_CRC1_CLK_DISABLE()        (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_CRC1EN)
#define __HAL_RCC_USBH_CLK_DISABLE()        (RCC->MC_AHB6ENCLRR = RCC_MC_AHB6ENCLRR_USBHEN)

/** @brief  Enable or disable the AHB2 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_DMA1_CLK_ENABLE()         (RCC->MC_AHB2ENSETR = RCC_MC_AHB2ENSETR_DMA1EN)
#define __HAL_RCC_DMA2_CLK_ENABLE()         (RCC->MC_AHB2ENSETR = RCC_MC_AHB2ENSETR_DMA2EN)
#define __HAL_RCC_DMAMUX_CLK_ENABLE()       (RCC->MC_AHB2ENSETR = RCC_MC_AHB2ENSETR_DMAMUXEN)
#define __HAL_RCC_ADC12_CLK_ENABLE()        (RCC->MC_AHB2ENSETR = RCC_MC_AHB2ENSETR_ADC12EN)
#define __HAL_RCC_USBO_CLK_ENABLE()         (RCC->MC_AHB2ENSETR = RCC_MC_AHB2ENSETR_USBOEN)
#define __HAL_RCC_SDMMC3_CLK_ENABLE()       (RCC->MC_AHB2ENSETR = RCC_MC_AHB2ENSETR_SDMMC3EN)

#define __HAL_RCC_DMA1_CLK_DISABLE()        (RCC->MC_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_DMA1EN)
#define __HAL_RCC_DMA2_CLK_DISABLE()        (RCC->MC_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_DMA2EN)
#define __HAL_RCC_DMAMUX_CLK_DISABLE()      (RCC->MC_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_DMAMUXEN)
#define __HAL_RCC_ADC12_CLK_DISABLE()       (RCC->MC_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_ADC12EN)
#define __HAL_RCC_USBO_CLK_DISABLE()        (RCC->MC_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_USBOEN)
#define __HAL_RCC_SDMMC3_CLK_DISABLE()      (RCC->MC_AHB2ENCLRR = RCC_MC_AHB2ENCLRR_SDMMC3EN)

/** @brief  Enable or disable the AHB3 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_DCMI_CLK_ENABLE()         (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_DCMIEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_ENABLE()        (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_CRYP2EN)
#endif
#define __HAL_RCC_HASH2_CLK_ENABLE()        (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_HASH2EN)
#define __HAL_RCC_RNG2_CLK_ENABLE()         (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_RNG2EN)
#define __HAL_RCC_CRC2_CLK_ENABLE()         (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_CRC2EN)
#define __HAL_RCC_HSEM_CLK_ENABLE()         (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_HSEMEN)
#define __HAL_RCC_IPCC_CLK_ENABLE()         (RCC->MC_AHB3ENSETR = RCC_MC_AHB3ENSETR_IPCCEN)

#define __HAL_RCC_DCMI_CLK_DISABLE()        (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_DCMIEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_DISABLE()       (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_CRYP2EN)
#endif
#define __HAL_RCC_HASH2_CLK_DISABLE()       (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_HASH2EN)
#define __HAL_RCC_RNG2_CLK_DISABLE()        (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_RNG2EN)
#define __HAL_RCC_CRC2_CLK_DISABLE()        (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_CRC2EN)
#define __HAL_RCC_HSEM_CLK_DISABLE()        (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_HSEMEN)
#define __HAL_RCC_IPCC_CLK_DISABLE()        (RCC->MC_AHB3ENCLRR = RCC_MC_AHB3ENCLRR_IPCCEN)

/** @brief  Enable or disable the AHB4 peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_GPIOA_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOAEN)
#define __HAL_RCC_GPIOB_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOBEN)
#define __HAL_RCC_GPIOC_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOCEN)
#define __HAL_RCC_GPIOD_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIODEN)
#define __HAL_RCC_GPIOE_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOEEN)
#define __HAL_RCC_GPIOF_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOFEN)
#define __HAL_RCC_GPIOG_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOGEN)
#define __HAL_RCC_GPIOH_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOHEN)
#define __HAL_RCC_GPIOI_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOIEN)
#define __HAL_RCC_GPIOJ_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOJEN)
#define __HAL_RCC_GPIOK_CLK_ENABLE()        (RCC->MC_AHB4ENSETR = RCC_MC_AHB4ENSETR_GPIOKEN)

#define __HAL_RCC_GPIOA_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOAEN)
#define __HAL_RCC_GPIOB_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOBEN)
#define __HAL_RCC_GPIOC_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOCEN)
#define __HAL_RCC_GPIOD_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIODEN)
#define __HAL_RCC_GPIOE_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOEEN)
#define __HAL_RCC_GPIOF_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOFEN)
#define __HAL_RCC_GPIOG_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOGEN)
#define __HAL_RCC_GPIOH_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOHEN)
#define __HAL_RCC_GPIOI_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOIEN)
#define __HAL_RCC_GPIOJ_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOJEN)
#define __HAL_RCC_GPIOK_CLK_DISABLE()       (RCC->MC_AHB4ENCLRR = RCC_MC_AHB4ENCLRR_GPIOKEN)

/** @brief  Enable or disable the AXI peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_SYSRAM_CLK_ENABLE()       (RCC->MC_AXIMENSETR = RCC_MC_AXIMENSETR_SYSRAMEN)

#define __HAL_RCC_SYSRAM_CLK_DISABLE()      (RCC->MC_AXIMENCLRR = RCC_MC_AXIMENCLRR_SYSRAMEN)

/** @brief  Enable or disable the MLAHB peripheral clock.
  * @note   After reset, the peripheral clock (used for registers read/write access)
  *         is disabled and the application software has to enable this clock before
  *         using it. It shall be used to allocate a peripheral to the MCU.
  */
#define __HAL_RCC_RETRAM_CLK_ENABLE()       (RCC->MC_MLAHBENSETR = RCC_MC_MLAHBENSETR_RETRAMEN)

#define __HAL_RCC_RETRAM_CLK_DISABLE()      (RCC->MC_MLAHBENCLRR = RCC_MC_MLAHBENCLRR_RETRAMEN)







/** @brief  Enable or disable the APB1 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_TIM2_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM2LPEN)
#define __HAL_RCC_TIM3_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM3LPEN)
#define __HAL_RCC_TIM4_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM4LPEN)
#define __HAL_RCC_TIM5_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM5LPEN)
#define __HAL_RCC_TIM6_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM6LPEN)
#define __HAL_RCC_TIM7_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM7LPEN)
#define __HAL_RCC_TIM12_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM12LPEN)
#define __HAL_RCC_TIM13_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM13LPEN)
#define __HAL_RCC_TIM14_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_TIM14LPEN)
#define __HAL_RCC_LPTIM1_CLK_SLEEP_ENABLE()       (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_LPTIM1LPEN)
#define __HAL_RCC_SPI2_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_SPI2LPEN)
#define __HAL_RCC_SPI3_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_SPI3LPEN)
#define __HAL_RCC_USART2_CLK_SLEEP_ENABLE()       (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_USART2LPEN)
#define __HAL_RCC_USART3_CLK_SLEEP_ENABLE()       (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_USART3LPEN)
#define __HAL_RCC_UART4_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART4LPEN)
#define __HAL_RCC_UART5_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART5LPEN)
#define __HAL_RCC_UART7_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART7LPEN)
#define __HAL_RCC_UART8_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_UART8LPEN)
#define __HAL_RCC_I2C1_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C1LPEN)
#define __HAL_RCC_I2C2_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C2LPEN)
#define __HAL_RCC_I2C3_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C3LPEN)
#define __HAL_RCC_I2C5_CLK_SLEEP_ENABLE()         (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_I2C5LPEN)
#define __HAL_RCC_SPDIFRX_CLK_SLEEP_ENABLE()      (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_SPDIFLPEN)
#define __HAL_RCC_CEC_CLK_SLEEP_ENABLE()          (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_CECLPEN)
#define __HAL_RCC_WWDG1_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_WWDG1LPEN)
#define __HAL_RCC_DAC12_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_DAC12LPEN)
#define __HAL_RCC_MDIOS_CLK_SLEEP_ENABLE()        (RCC->MC_APB1LPENSETR = RCC_MC_APB1LPENSETR_MDIOSLPEN)

#define __HAL_RCC_TIM2_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM2LPEN)
#define __HAL_RCC_TIM3_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM3LPEN)
#define __HAL_RCC_TIM4_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM4LPEN)
#define __HAL_RCC_TIM5_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM5LPEN)
#define __HAL_RCC_TIM6_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM6LPEN)
#define __HAL_RCC_TIM7_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM7LPEN)
#define __HAL_RCC_TIM12_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM12LPEN)
#define __HAL_RCC_TIM13_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM13LPEN)
#define __HAL_RCC_TIM14_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_TIM14LPEN)
#define __HAL_RCC_LPTIM1_CLK_SLEEP_DISABLE()      (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_LPTIM1LPEN)
#define __HAL_RCC_SPI2_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_SPI2LPEN)
#define __HAL_RCC_SPI3_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_SPI3LPEN)
#define __HAL_RCC_USART2_CLK_SLEEP_DISABLE()      (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_USART2LPEN)
#define __HAL_RCC_USART3_CLK_SLEEP_DISABLE()      (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_USART3LPEN)
#define __HAL_RCC_UART4_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART4LPEN)
#define __HAL_RCC_UART5_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART5LPEN)
#define __HAL_RCC_UART7_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART7LPEN)
#define __HAL_RCC_UART8_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_UART8LPEN)
#define __HAL_RCC_I2C1_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C1LPEN)
#define __HAL_RCC_I2C2_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C2LPEN)
#define __HAL_RCC_I2C3_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C3LPEN)
#define __HAL_RCC_I2C5_CLK_SLEEP_DISABLE()        (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_I2C5LPEN)
#define __HAL_RCC_SPDIFRX_CLK_SLEEP_DISABLE()     (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_SPDIFLPEN)
#define __HAL_RCC_CEC_CLK_SLEEP_DISABLE()         (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_CECLPEN)
#define __HAL_RCC_WWDG1_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_WWDG1LPEN)
#define __HAL_RCC_DAC12_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_DAC12LPEN)
#define __HAL_RCC_MDIOS_CLK_SLEEP_DISABLE()       (RCC->MC_APB1LPENCLRR = RCC_MC_APB1LPENCLRR_MDIOSLPEN)


/** @brief  Enable or disable the APB2 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_TIM1_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM1LPEN)
#define __HAL_RCC_TIM8_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM8LPEN)
#define __HAL_RCC_TIM15_CLK_SLEEP_ENABLE()        (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM15LPEN)
#define __HAL_RCC_TIM16_CLK_SLEEP_ENABLE()        (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM16LPEN)
#define __HAL_RCC_TIM17_CLK_SLEEP_ENABLE()        (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_TIM17LPEN)
#define __HAL_RCC_SPI1_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_SPI1LPEN)
#define __HAL_RCC_SPI4_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_SPI4LPEN)
#define __HAL_RCC_SPI5_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_SPI5LPEN)
#define __HAL_RCC_USART6_CLK_SLEEP_ENABLE()       (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_USART6LPEN)
#define __HAL_RCC_SAI1_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_SAI1LPEN)
#define __HAL_RCC_SAI2_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_SAI2LPEN)
#define __HAL_RCC_SAI3_CLK_SLEEP_ENABLE()         (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_SAI3LPEN)
#define __HAL_RCC_DFSDM1_CLK_SLEEP_ENABLE()       (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_DFSDMLPEN)
#define __HAL_RCC_ADFSDM1_CLK_SLEEP_ENABLE()      (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_ADFSDMLPEN)
#define __HAL_RCC_FDCAN_CLK_SLEEP_ENABLE()        (RCC->MC_APB2LPENSETR = RCC_MC_APB2LPENSETR_FDCANLPEN)

#define __HAL_RCC_TIM1_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM1LPEN)
#define __HAL_RCC_TIM8_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM8LPEN)
#define __HAL_RCC_TIM15_CLK_SLEEP_DISABLE()       (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM15LPEN)
#define __HAL_RCC_TIM16_CLK_SLEEP_DISABLE()       (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM16LPEN)
#define __HAL_RCC_TIM17_CLK_SLEEP_DISABLE()       (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_TIM17LPEN)
#define __HAL_RCC_SPI1_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SPI1LPEN)
#define __HAL_RCC_SPI4_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SPI4LPEN)
#define __HAL_RCC_SPI5_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SPI5LPEN)
#define __HAL_RCC_USART6_CLK_SLEEP_DISABLE()      (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_USART6LPEN)
#define __HAL_RCC_SAI1_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SAI1LPEN)
#define __HAL_RCC_SAI2_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SAI2LPEN)
#define __HAL_RCC_SAI3_CLK_SLEEP_DISABLE()        (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_SAI3LPEN)
#define __HAL_RCC_DFSDM1_CLK_SLEEP_DISABLE()      (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_DFSDMLPEN)
#define __HAL_RCC_ADFSDM1_CLK_SLEEP_DISABLE()     (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_ADFSDMLPEN)
#define __HAL_RCC_FDCAN_CLK_SLEEP_DISABLE()       (RCC->MC_APB2LPENCLRR = RCC_MC_APB2LPENCLRR_FDCANLPEN)


/** @brief  Enable or disable the APB3 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_LPTIM2_CLK_SLEEP_ENABLE()       (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM2LPEN)
#define __HAL_RCC_LPTIM3_CLK_SLEEP_ENABLE()       (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM3LPEN)
#define __HAL_RCC_LPTIM4_CLK_SLEEP_ENABLE()       (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM4LPEN)
#define __HAL_RCC_LPTIM5_CLK_SLEEP_ENABLE()       (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_LPTIM5LPEN)
#define __HAL_RCC_SAI4_CLK_SLEEP_ENABLE()         (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_SAI4LPEN)
#define __HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE()       (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_SYSCFGLPEN)
#define __HAL_RCC_VREF_CLK_SLEEP_ENABLE()         (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_VREFLPEN)
#define __HAL_RCC_TMPSENS_CLK_SLEEP_ENABLE()      (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_TMPSENSLPEN)
#define __HAL_RCC_PMBCTRL_CLK_SLEEP_ENABLE()      (RCC->MC_APB3LPENSETR = RCC_MC_APB3LPENSETR_PMBCTRLLPEN)

#define __HAL_RCC_LPTIM2_CLK_SLEEP_DISABLE()      (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM2LPEN)
#define __HAL_RCC_LPTIM3_CLK_SLEEP_DISABLE()      (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM3LPEN)
#define __HAL_RCC_LPTIM4_CLK_SLEEP_DISABLE()      (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM4LPEN)
#define __HAL_RCC_LPTIM5_CLK_SLEEP_DISABLE()      (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_LPTIM5LPEN)
#define __HAL_RCC_SAI4_CLK_SLEEP_DISABLE()        (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_SAI4LPEN)
#define __HAL_RCC_SYSCFG_CLK_SLEEP_DISABLE()      (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_SYSCFGLPEN)
#define __HAL_RCC_VREF_CLK_SLEEP_DISABLE()        (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_VREFLPEN)
#define __HAL_RCC_TMPSENS_CLK_SLEEP_DISABLE()     (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_TMPSENSLPEN)
#define __HAL_RCC_PMBCTRL_CLK_SLEEP_DISABLE()     (RCC->MC_APB3LPENCLRR = RCC_MC_APB3LPENCLRR_PMBCTRLLPEN)

/** @brief  Enable or disable the APB4 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_LTDC_CLK_SLEEP_ENABLE()         (RCC->MC_APB4LPENSETR = RCC_MC_APB4LPENSETR_LTDCLPEN)
#define __HAL_RCC_DSI_CLK_SLEEP_ENABLE()          (RCC->MC_APB4LPENSETR = RCC_MC_APB4LPENSETR_DSILPEN)
#define __HAL_RCC_DDRPERFM_CLK_SLEEP_ENABLE()     (RCC->MC_APB4LPENSETR = RCC_MC_APB4LPENSETR_DDRPERFMLPEN)
#define __HAL_RCC_USBPHY_CLK_SLEEP_ENABLE()       (RCC->MC_APB4LPENSETR = RCC_MC_APB4LPENSETR_USBPHYLPEN)
#define __HAL_RCC_STGENRO_CLK_SLEEP_ENABLE()      (RCC->MC_APB4LPENSETR = RCC_MC_APB4LPENSETR_STGENROLPEN)
#define __HAL_RCC_STGENRO_CLK_STOP_ENABLE()       (RCC->MC_APB4LPENSETR = RCC_MC_APB4LPENSETR_STGENROSTPEN)

#define __HAL_RCC_LTDC_CLK_SLEEP_DISABLE()        (RCC->MC_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_LTDCLPEN)
#define __HAL_RCC_DSI_CLK_SLEEP_DISABLE()         (RCC->MC_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_DSILPEN)
#define __HAL_RCC_DDRPERFM_CLK_SLEEP_DISABLE()    (RCC->MC_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_DDRPERFMLPEN)
#define __HAL_RCC_USBPHY_CLK_SLEEP_DISABLE()      (RCC->MC_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_USBPHYLPEN)
#define __HAL_RCC_STGENRO_CLK_SLEEP_DISABLE()     (RCC->MC_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_STGENROLPEN)
#define __HAL_RCC_STGENRO_CLK_STOP_DISABLE()      (RCC->MC_APB4LPENCLRR = RCC_MC_APB4LPENCLRR_STGENROSTPEN)

/** @brief  Enable or disable the APB5 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_SPI6_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_SPI6LPEN)
#define __HAL_RCC_I2C4_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_I2C4LPEN)
#define __HAL_RCC_I2C6_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_I2C6LPEN)
#define __HAL_RCC_USART1_CLK_SLEEP_ENABLE()       (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_USART1LPEN)
#define __HAL_RCC_RTCAPB_CLK_SLEEP_ENABLE()       (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_RTCAPBLPEN)
#define __HAL_RCC_TZC1_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_TZC1LPEN)
#define __HAL_RCC_TZC2_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_TZC2LPEN)
#define __HAL_RCC_TZPC_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_TZPCLPEN)
#define __HAL_RCC_BSEC_CLK_SLEEP_ENABLE()         (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_BSECLPEN)
#define __HAL_RCC_STGEN_CLK_SLEEP_ENABLE()        (RCC->MC_APB5LPENSETR = RCC_MC_APB5LPENSETR_STGENLPEN)

#define __HAL_RCC_SPI6_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_SPI6LPEN)
#define __HAL_RCC_I2C4_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_I2C4LPEN)
#define __HAL_RCC_I2C6_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_I2C6LPEN)
#define __HAL_RCC_USART1_CLK_SLEEP_DISABLE()      (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_USART1LPEN)
#define __HAL_RCC_RTCAPB_CLK_SLEEP_DISABLE()      (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_RTCAPBLPEN)
#define __HAL_RCC_TZC1_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_TZC1LPEN)
#define __HAL_RCC_TZC2_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_TZC2LPEN)
#define __HAL_RCC_TZPC_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENCLRR_TZPCLPEN)
#define __HAL_RCC_BSEC_CLK_SLEEP_DISABLE()        (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENSETR_BSECLPEN)
#define __HAL_RCC_STGEN_CLK_SLEEP_DISABLE()       (RCC->MC_APB5LPENCLRR = RCC_MC_APB5LPENSETR_STGENLPEN)


/** @brief  Enable or disable the AHB5 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_GPIOZ_CLK_SLEEP_ENABLE()        (RCC->MC_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_GPIOZLPEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_SLEEP_ENABLE()        (RCC->MC_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_CRYP1LPEN)
#endif
#define __HAL_RCC_HASH1_CLK_SLEEP_ENABLE()        (RCC->MC_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_HASH1LPEN)
#define __HAL_RCC_RNG1_CLK_SLEEP_ENABLE()         (RCC->MC_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_RNG1LPEN)
#define __HAL_RCC_BKPSRAM_CLK_SLEEP_ENABLE()      (RCC->MC_AHB5LPENSETR = RCC_MC_AHB5LPENSETR_BKPSRAMLPEN)

#define __HAL_RCC_GPIOZ_CLK_SLEEP_DISABLE()       (RCC->MC_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_GPIOZLPEN)
#if defined(CRYP1)
#define __HAL_RCC_CRYP1_CLK_SLEEP_DISABLE()       (RCC->MC_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_CRYP1LPEN)
#endif
#define __HAL_RCC_HASH1_CLK_SLEEP_DISABLE()       (RCC->MC_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_HASH1LPEN)
#define __HAL_RCC_RNG1_CLK_SLEEP_DISABLE()        (RCC->MC_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_RNG1LPEN)
#define __HAL_RCC_BKPSRAM_CLK_SLEEP_DISABLE()     (RCC->MC_AHB5LPENCLRR = RCC_MC_AHB5LPENCLRR_BKPSRAMLPEN)


/** @brief  Enable or disable the AHB6 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_MDMA_CLK_SLEEP_ENABLE()         (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_MDMALPEN)
#define __HAL_RCC_GPU_CLK_SLEEP_ENABLE()          (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_GPULPEN)
#define __HAL_RCC_ETH1CK_CLK_SLEEP_ENABLE()       (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHCKLPEN)
#define __HAL_RCC_ETH1TX_CLK_SLEEP_ENABLE()       (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHTXLPEN)
#define __HAL_RCC_ETH1RX_CLK_SLEEP_ENABLE()       (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHRXLPEN)
#define __HAL_RCC_ETH1MAC_CLK_SLEEP_ENABLE()      (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHMACLPEN)
#define __HAL_RCC_FMC_CLK_SLEEP_ENABLE()          (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_FMCLPEN)
#define __HAL_RCC_QSPI_CLK_SLEEP_ENABLE()         (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_QSPILPEN)
#define __HAL_RCC_SDMMC1_CLK_SLEEP_ENABLE()       (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_SDMMC1LPEN)
#define __HAL_RCC_SDMMC2_CLK_SLEEP_ENABLE()       (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_SDMMC2LPEN)
#define __HAL_RCC_CRC1_CLK_SLEEP_ENABLE()         (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_CRC1LPEN)
#define __HAL_RCC_USBH_CLK_SLEEP_ENABLE()         (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_USBHLPEN)

#define __HAL_RCC_MDMA_CLK_SLEEP_DISABLE()        (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_MDMALPEN)
#define __HAL_RCC_GPU_CLK_SLEEP_DISABLE()         (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_GPULPEN)
#define __HAL_RCC_ETH1CK_CLK_SLEEP_DISABLE()      (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHCKLPEN)
#define __HAL_RCC_ETH1TX_CLK_SLEEP_DISABLE()      (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHTXLPEN)
#define __HAL_RCC_ETH1RX_CLK_SLEEP_DISABLE()      (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHRXLPEN)
#define __HAL_RCC_ETH1MAC_CLK_SLEEP_DISABLE()     (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_ETHMACLPEN)
#define __HAL_RCC_FMC_CLK_SLEEP_DISABLE()         (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_FMCLPEN)
#define __HAL_RCC_QSPI_CLK_SLEEP_DISABLE()        (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_QSPILPEN)
#define __HAL_RCC_SDMMC1_CLK_SLEEP_DISABLE()      (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_SDMMC1LPEN)
#define __HAL_RCC_SDMMC2_CLK_SLEEP_DISABLE()      (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_SDMMC2LPEN)
#define __HAL_RCC_CRC1_CLK_SLEEP_DISABLE()        (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_CRC1LPEN)
#define __HAL_RCC_USBH_CLK_SLEEP_DISABLE()        (RCC->MC_AHB6LPENCLRR = RCC_MC_AHB6LPENCLRR_USBHLPEN)


/** @brief  Enable or disable the AHB2 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_DMA1_CLK_SLEEP_ENABLE()         (RCC->MC_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_DMA1LPEN)
#define __HAL_RCC_DMA2_CLK_SLEEP_ENABLE()         (RCC->MC_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_DMA2LPEN)
#define __HAL_RCC_DMAMUX_CLK_SLEEP_ENABLE()       (RCC->MC_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_DMAMUXLPEN)
#define __HAL_RCC_ADC12_CLK_SLEEP_ENABLE()        (RCC->MC_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_ADC12LPEN)
#define __HAL_RCC_USBO_CLK_SLEEP_ENABLE()         (RCC->MC_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_USBOLPEN)
#define __HAL_RCC_SDMMC3_CLK_SLEEP_ENABLE()       (RCC->MC_AHB2LPENSETR = RCC_MC_AHB2LPENSETR_SDMMC3LPEN)

#define __HAL_RCC_DMA1_CLK_SLEEP_DISABLE()        (RCC->MC_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_DMA1LPEN)
#define __HAL_RCC_DMA2_CLK_SLEEP_DISABLE()        (RCC->MC_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_DMA2LPEN)
#define __HAL_RCC_DMAMUX_CLK_SLEEP_DISABLE()      (RCC->MC_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_DMAMUXLPEN)
#define __HAL_RCC_ADC12_CLK_SLEEP_DISABLE()       (RCC->MC_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_ADC12LPEN)
#define __HAL_RCC_USBO_CLK_SLEEP_DISABLE()        (RCC->MC_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_USBOLPEN)
#define __HAL_RCC_SDMMC3_CLK_SLEEP_DISABLE()      (RCC->MC_AHB2LPENCLRR = RCC_MC_AHB2LPENCLRR_SDMMC3LPEN)

/** @brief  Enable or disable the AHB3 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_DCMI_CLK_SLEEP_ENABLE()         (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_DCMILPEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_SLEEP_ENABLE()        (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_CRYP2LPEN)
#endif
#define __HAL_RCC_HASH2_CLK_SLEEP_ENABLE()        (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_HASH2LPEN)
#define __HAL_RCC_RNG2_CLK_SLEEP_ENABLE()         (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_RNG2LPEN)
#define __HAL_RCC_CRC2_CLK_SLEEP_ENABLE()         (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_CRC2LPEN)
#define __HAL_RCC_HSEM_CLK_SLEEP_ENABLE()         (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_HSEMLPEN)
#define __HAL_RCC_IPCC_CLK_SLEEP_ENABLE()         (RCC->MC_AHB3LPENSETR = RCC_MC_AHB3LPENSETR_IPCCLPEN)

#define __HAL_RCC_DCMI_CLK_SLEEP_DISABLE()        (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_DCMILPEN)
#if defined(CRYP2)
#define __HAL_RCC_CRYP2_CLK_SLEEP_DISABLE()       (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_CRYP2LPEN)
#endif
#define __HAL_RCC_HASH2_CLK_SLEEP_DISABLE()       (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_HASH2LPEN)
#define __HAL_RCC_RNG2_CLK_SLEEP_DISABLE()        (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_RNG2LPEN)
#define __HAL_RCC_CRC2_CLK_SLEEP_DISABLE()        (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_CRC2LPEN)
#define __HAL_RCC_HSEM_CLK_SLEEP_DISABLE()        (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_HSEMLPEN)
#define __HAL_RCC_IPCC_CLK_SLEEP_DISABLE()        (RCC->MC_AHB3LPENCLRR = RCC_MC_AHB3LPENCLRR_IPCCLPEN)


/** @brief  Enable or disable the AHB4 peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_GPIOA_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOALPEN)
#define __HAL_RCC_GPIOB_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOBLPEN)
#define __HAL_RCC_GPIOC_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOCLPEN)
#define __HAL_RCC_GPIOD_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIODLPEN)
#define __HAL_RCC_GPIOE_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOELPEN)
#define __HAL_RCC_GPIOF_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOFLPEN)
#define __HAL_RCC_GPIOG_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOGLPEN)
#define __HAL_RCC_GPIOH_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOHLPEN)
#define __HAL_RCC_GPIOI_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOILPEN)
#define __HAL_RCC_GPIOJ_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOJLPEN)
#define __HAL_RCC_GPIOK_CLK_SLEEP_ENABLE()        (RCC->MC_AHB4LPENSETR = RCC_MC_AHB4LPENSETR_GPIOKLPEN)

#define __HAL_RCC_GPIOA_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOALPEN)
#define __HAL_RCC_GPIOB_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOBLPEN)
#define __HAL_RCC_GPIOC_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOCLPEN)
#define __HAL_RCC_GPIOD_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIODLPEN)
#define __HAL_RCC_GPIOE_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOELPEN)
#define __HAL_RCC_GPIOF_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOFLPEN)
#define __HAL_RCC_GPIOG_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOGLPEN)
#define __HAL_RCC_GPIOH_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOHLPEN)
#define __HAL_RCC_GPIOI_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOILPEN)
#define __HAL_RCC_GPIOJ_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOJLPEN)
#define __HAL_RCC_GPIOK_CLK_SLEEP_DISABLE()       (RCC->MC_AHB4LPENCLRR = RCC_MC_AHB4LPENCLRR_GPIOKLPEN)

/** @brief  Enable or disable the AXI peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_SYSRAM_CLK_SLEEP_ENABLE()       (RCC->MC_AXIMLPENSETR = RCC_MC_AXIMLPENSETR_SYSRAMLPEN)

#define __HAL_RCC_SYSRAM_CLK_SLEEP_DISABLE()      (RCC->MC_AXIMLPENCLRR = RCC_MC_AXIMLPENCLRR_SYSRAMLPEN)


/** @brief  Enable or disable the MLAHB peripheral clock during  CSLEEP mode.
  * @note   Peripheral clock gating in SLEEP mode can be used to further reduce
  *         power consumption.
  * @note   After wakeup from SLEEP mode, the peripheral clock is enabled again.
  * @note   By default, all peripheral clocks are enabled during CSLEEP mode.
  */
#define __HAL_RCC_RETRAM_CLK_SLEEP_ENABLE()       (RCC->MC_MLAHBLPENSETR = RCC_MC_MLAHBLPENSETR_RETRAMLPEN)
#define __HAL_RCC_SRAM1_CLK_SLEEP_ENABLE()        (RCC->MC_MLAHBLPENSETR = RCC_MC_MLAHBLPENSETR_SRAM1LPEN)
#define __HAL_RCC_SRAM2_CLK_SLEEP_ENABLE()        (RCC->MC_MLAHBLPENSETR = RCC_MC_MLAHBLPENSETR_SRAM2LPEN)
#define __HAL_RCC_SRAM34_CLK_SLEEP_ENABLE()       (RCC->MC_MLAHBLPENSETR = RCC_MC_MLAHBLPENSETR_SRAM34LPEN)

#define __HAL_RCC_RETRAM_CLK_SLEEP_DISABLE()      (RCC->MC_MLAHBLPENCLRR = RCC_MC_MLAHBLPENCLRR_RETRAMLPEN)
#define __HAL_RCC_SRAM1_CLK_SLEEP_DISABLE()       (RCC->MC_MLAHBLPENCLRR = RCC_MC_MLAHBLPENCLRR_SRAM1LPEN)
#define __HAL_RCC_SRAM2_CLK_SLEEP_DISABLE()       (RCC->MC_MLAHBLPENCLRR = RCC_MC_MLAHBLPENCLRR_SRAM2LPEN)
#define __HAL_RCC_SRAM34_CLK_SLEEP_DISABLE()      (RCC->MC_MLAHBLPENCLRR = RCC_MC_MLAHBLPENCLRR_SRAM3LPEN)

/** @brief  Enable or disable the AHB6 peripheral clock during  (C)STOP mode.
  * @note   Peripheral clock gating in (C)STOP mode can be used to further reduce
  *         power consumption.
  * @note   It is possible to control the gating of clocks coming from PADs ETH_RX_CLK and ETH_TX_CLK
  *         in CSTOP and STOP modes via the ETHSTPEN bit
  */
#define __HAL_RCC_ETH1CK_CLK_STOP_ENABLE()         (RCC->MC_AHB6LPENSETR = RCC_MC_AHB6LPENSETR_ETHSTPEN)

#define __HAL_RCC_ETH1CK_CLK_STOP_DISABLE()        (RCC->MC_AHB6LPENSETR) =  (RCC_MC_AHB6LPENCLRR_ETHSTPEN)

#endif /* !CORE_CA7 */

/** @brief Macro to test if HSE oscillator is used somewhere in the core system
 *  @note  HAL driver does not ensure these clocks are used by peripherals, it's
 *         up to user code
 */
#define IS_HSE_IN_USE() \
  (( ((__HAL_RCC_GET_MPU_SOURCE()    == RCC_MPUSOURCE_HSE   ) && __HAL_RCC_GET_FLAG(RCC_FLAG_MPUSRCRDY) ) || \
     ((__HAL_RCC_GET_AXIS_SOURCE()   == RCC_AXISSOURCE_HSE  ) && __HAL_RCC_GET_FLAG(RCC_FLAG_AXISSRCRDY)) || \
     ((__HAL_RCC_GET_MCU_SOURCE()    == RCC_MCUSSOURCE_HSE  ) && __HAL_RCC_GET_FLAG(RCC_FLAG_MCUSSRCRDY)) || \
                                                                                                             \
     ((__HAL_RCC_GET_PLL12_SOURCE()  == RCC_PLL12SOURCE_HSE)   &&                                            \
      ( __HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY)    || __HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY) ))                || \
                                                                                                             \
     ((__HAL_RCC_GET_PLL3_SOURCE()   == RCC_PLL3SOURCE_HSE)    && __HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) )  || \
                                                                                                             \
     ((__HAL_RCC_GET_PLL4_SOURCE()   == RCC_PLL4SOURCE_HSE)    && __HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY) ))? 1 : 0)

/** @brief Macro to test if HSI oscillator is used somewhere in the core system
 *  @note  HAL driver does not ensure these clocks are used by peripherals, it's
 *         up to user code
 */
#define IS_HSI_IN_USE() \
  (( ((__HAL_RCC_GET_MPU_SOURCE()    == RCC_MPUSOURCE_HSI   ) && __HAL_RCC_GET_FLAG(RCC_FLAG_MPUSRCRDY)  ) || \
     ((__HAL_RCC_GET_AXIS_SOURCE()   == RCC_AXISSOURCE_HSI  ) && __HAL_RCC_GET_FLAG(RCC_FLAG_AXISSRCRDY) ) || \
     ((__HAL_RCC_GET_MCU_SOURCE()    == RCC_MCUSSOURCE_HSI  ) && __HAL_RCC_GET_FLAG(RCC_FLAG_MCUSSRCRDY) ) || \
                                                                                                              \
     ((__HAL_RCC_GET_PLL12_SOURCE()  == RCC_PLL12SOURCE_HSI)   &&                                             \
         ( __HAL_RCC_GET_FLAG(RCC_FLAG_PLL1RDY)    || __HAL_RCC_GET_FLAG(RCC_FLAG_PLL2RDY) ))              || \
                                                                                                              \
     ((__HAL_RCC_GET_PLL3_SOURCE()   == RCC_PLL3SOURCE_HSI)    && __HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) )   || \
                                                                                                              \
     ((__HAL_RCC_GET_PLL4_SOURCE()   == RCC_PLL4SOURCE_HSI)    && __HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY) ))? 1 : 0)


/** @brief Macro to test if CSI oscillator is used somewhere in the core system
 *  @note  HAL driver does not ensure these clocks are used by peripherals, it's
 *         up to user code
 */
#define IS_CSI_IN_USE() \
  (( ((__HAL_RCC_GET_PLL3_SOURCE()   == RCC_PLL3SOURCE_CSI)  && __HAL_RCC_GET_FLAG(RCC_FLAG_PLL3RDY) ) || \
     ((__HAL_RCC_GET_MCU_SOURCE()    == RCC_MCUSSOURCE_CSI)  && __HAL_RCC_GET_FLAG(RCC_FLAG_MCUSSRCRDY)) || \
                                                                                                            \
     ((__HAL_RCC_GET_PLL4_SOURCE()   == RCC_PLL4SOURCE_CSI)  && __HAL_RCC_GET_FLAG(RCC_FLAG_PLL4RDY)))? 1 : 0)

/** @brief Macros to test if PLLx are used on the Core and buses clock generation system (MPU, MCU or AXIS blocks)
  * @note  HAL driver does not ensure these clocks are used by peripherals, it's up to user code  */
#define __IS_PLL1_IN_USE() \
  (  ( ((__HAL_RCC_GET_MPU_SOURCE() == RCC_MPUSOURCE_PLL1)   &&  __HAL_RCC_GET_FLAG(RCC_FLAG_MPUSRCRDY)) || \
       ((__HAL_RCC_GET_MPU_SOURCE() == RCC_MPUSOURCE_MPUDIV) &&  __HAL_RCC_GET_FLAG(RCC_FLAG_MPUSRCRDY)) )   ? 1:0)

#define __IS_PLL2_IN_USE() \
  (  ( (__HAL_RCC_GET_AXIS_SOURCE() == RCC_AXISSOURCE_PLL2) &&  __HAL_RCC_GET_FLAG(RCC_FLAG_AXISSRCRDY)  )? 1:0)

#define __IS_PLL3_IN_USE() \
  (  ( (__HAL_RCC_GET_MCU_SOURCE()  == RCC_MCUSSOURCE_PLL3)  &&  __HAL_RCC_GET_FLAG(RCC_FLAG_MCUSSRCRDY) )? 1:0)

/** @brief  Macros to enable or disable the Internal High Speed oscillator (HSI).
  * @note   The HSI is stopped by hardware when entering STOP and STANDBY modes.
  *         It is used (enabled by hardware) as system clock source after startup
  *         from Reset, wakeup from STOP and STANDBY mode, or in case of failure
  *         of the HSE used directly or indirectly as system clock (if the Clock
  *         Security System CSS is enabled).
  * @note   HSI can not be stopped if it is used as system clock source. In this case,
  *         you have to select another source of the system clock then stop the HSI.
  * @note   After enabling the HSI, the application software should wait on HSIRDY
  *         flag to be set indicating that HSI clock is stable and can be used as
  *         system clock source.
  *         This parameter can be: ENABLE or DISABLE.
  * @note   When the HSI is stopped, HSIRDY flag goes low after 6 HSI oscillator
  *         clock cycles.
  */
#define __HAL_RCC_HSI_ENABLE()   SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSION)
#define __HAL_RCC_HSI_DISABLE()  WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSION)

/** @brief  Macro to adjust the Internal High Speed oscillator (HSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal HSI RC.
  * @param  __HSICalibrationValue__: specifies the calibration trimming value
  *         HSITRIM. HSITRIM represents a signed value. HSITRIM field is added
  *         to the engineering Option Bytes loaded during reset phase
  *         (bsec_hsi_cal[11:0]) in order to form the calibration trimming value.
  *         HSICAL[11:0] = bsec_hsi_cal[11:0] + HSITRIM[6:0]
  *         This parameter must be a number between 0 and 0x7F:
  *         0x40: bsec_hsi_cal[11:0] - 64
  *         0x41: bsec_hsi_cal[11:0] - 63
  *         ...
  *         0x0: bsec_hsi_cal[11:0] (default after reset)
  *         ...
  *         0x3E: bsec_hsi_cal[11:0] + 62
  *         0x3F: bsec_hsi_cal[11:0] + 63
  */
#define __HAL_RCC_HSI_CALIBRATIONVALUE_ADJUST(__HSICalibrationValue__) \
                  MODIFY_REG(RCC->HSICFGR, RCC_HSICFGR_HSITRIM, \
                            (uint32_t)(__HSICalibrationValue__) << RCC_HSICFGR_HSITRIM_Pos)


/**
  * @brief  Macro to configure HSI clock divider
  * @note   Set and reset by software to control the HSI clock division factor.
  * @note   It is not allowed to change HSIDIV, if the HSI is currently used as
  *         reference clock for a PLL.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode.
  * @note   Write access to this register is not allowed during the clock restore sequence
  * @note   The application can check if the new division factor is taken into
  *         account by reading back the HSIDIVRDY flag
  *
  * @param  __RCC_HSIDIV__: specifies the HSI division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_HSI_DIV1:  Division by 1, ck_hsi(_ker) = 64 MHz (default after reset)
  *              @arg RCC_HSI_DIV2:  Division by 2, ck_hsi(_ker) = 32 MHz
  *              @arg RCC_HSI_DIV4:  Division by 4, ck_hsi(_ker) = 16 MHz
  *              @arg RCC_HSI_DIV8:  Division by 8, ck_hsi(_ker) =  8 MHz
  */
#define __HAL_RCC_HSI_DIV(__RCC_HSIDIV__) \
                 do{  MODIFY_REG( RCC->HSICFGR, RCC_HSICFGR_HSIDIV , __RCC_HSIDIV__ );\
                 } while(0)

/** @brief  Macro to get the HSI clock division factor.
  * @retval The HSI clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_HSI_DIV1:  Division by 1, ck_hsi(_ker) = 64 MHz (default after reset)
  *              - RCC_HSI_DIV2:  Division by 2, ck_hsi(_ker) = 32 MHz
  *              - RCC_HSI_DIV4:  Division by 4, ck_hsi(_ker) = 16 MHz
  *              - RCC_HSI_DIV8:  Division by 8, ck_hsi(_ker) =  8 MHz
  */
#define __HAL_RCC_GET_HSI_DIV() ((uint32_t)(RCC->HSICFGR & RCC_HSICFGR_HSIDIV ))


/**
  * @brief    Macros to enable or disable the force of the Internal High Speed oscillator (HSI)
  *           in STOP mode to be quickly available as kernel clock.
  * @note     Keeping the HSI ON in STOP mode allows to avoid slowing down the communication
  *           speed because of the HSI startup time.
  * @note     The enable of this function has not effect on the HSION bit.
  *           This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
#define __HAL_RCC_HSISTOP_ENABLE()           SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSIKERON)
#define __HAL_RCC_HSISTOP_DISABLE()          WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSIKERON)


/**
  * @brief  Macros to enable or disable the Internal  oscillator (CSI).
  * @note     The CSI is stopped by hardware when entering STOP and STANDBY modes.
  *           It is used (enabled by hardware) as system clock source after
  *           startup from Reset, wakeup from STOP and STANDBY mode, or in case
  *           of failure of the HSE used directly or indirectly as system clock
  *           (if the Clock Security System CSS is enabled).
  * @note     CSI can not be stopped if it is used as system clock source.
  *           In this case, you have to select another source of the system
  *           clock then stop the CSI.
  * @note     After enabling the CSI, the application software should wait on
  *           CSIRDY flag to be set indicating that CSI clock is stable and can
  *           be used as system clock source.
  * @note   When the CSI is stopped, CSIRDY flag goes low after 6 CSI oscillator
  *         clock cycles.
  */
#define __HAL_RCC_CSI_ENABLE()  SET_BIT(RCC->OCENSETR, RCC_OCENSETR_CSION)
#define __HAL_RCC_CSI_DISABLE() WRITE_REG(RCC->OCENCLRR, RCC_OCENSETR_CSION)

/** @brief  Macro Adjusts the Internal  oscillator (CSI) calibration value.
  * @note   The calibration is used to compensate for the variations in voltage
  *         and temperature that influence the frequency of the internal CSI RC.
  *         Please refer to the datasheet of the product for for more details on how to
  *         calibrate the CSI.
  * @param  __CSICalibrationValue__: specifies the calibration trimming value.
  *         This parameter must be a number between 0 and 0x1F.
  */
#define __HAL_RCC_CSI_CALIBRATIONVALUE_ADJUST(__CSICalibrationValue__) \
                  MODIFY_REG(RCC->CSICFGR, RCC_CSICFGR_CSITRIM, \
                            (uint32_t)(__CSICalibrationValue__) << RCC_CSICFGR_CSITRIM_Pos)

/**
  * @brief    Macros to enable or disable the force of the Internal High Speed oscillator (CSI)
  *           in STOP mode to be quickly available as kernel clock.
  * @note     Keeping the CSI ON in STOP mode allows to avoid slowing down the communication
  *           speed because of the CSI startup time.
  * @note     The enable of this function has not effect on the CSION bit.
  *           This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
#define __HAL_RCC_CSISTOP_ENABLE()            SET_BIT(RCC->OCENSETR, RCC_OCENSETR_CSIKERON)
#define __HAL_RCC_CSISTOP_DISABLE()           WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_CSIKERON)


/** @brief  Macros to enable or disable the Internal Low Speed oscillator (LSI).
  * @note   After enabling the LSI, the application software should wait on
  *         LSIRDY flag to be set indicating that LSI clock is stable and can
  *         be used to clock the IWDG and/or the RTC.
  * @note   LSI can not be disabled if the IWDG is running.
  * @note   When the LSI is stopped, LSIRDY flag goes low after 6 LSI oscillator
  *         clock cycles.
  */
#define __HAL_RCC_LSI_ENABLE()  SET_BIT(RCC->RDLSICR, RCC_RDLSICR_LSION)
#define __HAL_RCC_LSI_DISABLE() CLEAR_BIT(RCC->RDLSICR, RCC_RDLSICR_LSION)




/** @brief  Macro to configure the Minimum Reset Duration
  * @note   Set and reset by software. They define the minimum guaranteed
  *         duration of the NRST low pulse. The LSI oscillator is automatically
  *         enabled when needed by the RPCTL.
  * @note   00000: NRST low pulse duration is guaranteed by the pulse stretcher
  *                of the PAD. The RPCTL is bypassed (default after reset)
  *         00010: The guaranteed NRST low pulse duration is about 2 ms (2 x 32 ck_lsi cycles),
  *         ...
  *         11111: The guaranteed NRST low pulse duration is about 31 ms (31 x 32 ck_lsi cycles).
  */
#define __HAL_RCC_MRD_CONFIG(__DURATION__) \
    do { MODIFY_REG(RCC->RDLSICR, RCC_RDLSICR_MRD, \
                    (uint32_t)(__DURATION__) << RCC_RDLSICR_MRD_Pos) \
         HAL_Delay(1); \
    }  while(0)



/**
  * @brief  Macro (depricated) to configure the External High Speed oscillator (HSE).
  * @note   Macro name is kept to mantain compatibility and it is mapped on
  *         HAL_RCC_HSEConfig function which replaces macro and should be used
  *         instead.
  * @note   HSE state can not be changed if it is used directly or through the
  *         PLL as system clock. In this case, you have to select another source
  *         of the system clock then change the HSE state (ex. disable it).
  * @note   The HSE is stopped by hardware when entering STOP and STANDBY modes.
  * @note   This function reset the CSSON bit, so if the clock security system(CSS)
  *         was previously enabled you have to enable it again after calling this
  *         function.
  * @param  __STATE__: specifies the new state of the HSE.
  *         This parameter can be one of the following values:
  *            @arg RCC_HSE_OFF: turn OFF the HSE oscillator
  *            @arg RCC_HSE_ON: turn ON the HSE oscillator using an external
  *                 crystal/ceramic resonator
  *            @arg RCC_HSE_BYPASS: HSE oscillator bypassed with external clock
  *                 using a low-swing analog signal provided to OSC_IN
  *            @arg RCC_HSE_BYPASS_DIG: HSE oscillator bypassed with external
  *                 clock using a full-swing digital signal provided to OSC_IN
  */
#define __HAL_RCC_HSE_CONFIG(__STATE__) HAL_RCC_HSEConfig(__STATE__)


/**
  * @brief    Macros to enable or disable the force of the External High Speed oscillator (HSE)
  *           in STOP mode to be quickly available as kernel clock.
  * @note     Keeping the HSE ON in STOP mode allows to avoid slowing down the communication
  *           speed because of the HSE startup time.
  * @note     The enable of this function has not effect on the HSION bit.
  *           This parameter can be: ENABLE or DISABLE.
  * @retval None
  */
#define __HAL_RCC_HSESTOP_ENABLE()           SET_BIT(RCC->OCENSETR, RCC_OCENSETR_HSEKERON)
#define __HAL_RCC_HSESTOP_DISABLE()          WRITE_REG(RCC->OCENCLRR, RCC_OCENCLRR_HSEKERON)


/**
  * @brief  Macro (depricated) to configure the External Low Speed oscillator (LSE).
  * @note   Macro name is kept to mantain compatibility and it is mapped on
  *         HAL_RCC_LSEConfig function which replaces macro and should be used
  *         instead.
  * @note   As the LSE is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         HAL_PWR_EnableBkUpAccess() function before to configure the LSE
  *         (to be done once after reset).
  * @param  __STATE__: specifies the new state of the LSE.
  *         This parameter can be a value of @ref RCC_LSE_Config
  *            @arg RCC_LSE_OFF: turn OFF the LSE oscillator
  *            @arg RCC_LSE_ON: turn ON the LSE oscillator using an external
  *                 crystal/ceramic resonator
  *            @arg RCC_LSE_BYPASS: LSE oscillator bypassed with external clock
  *                 using a low-swing analog signal provided to OSC32_IN
  *            @arg RCC_LSE_BYPASS_DIG: LSE oscillator bypassed with external clock using
  *                 a full-swing digital signal provided to OSC32_IN
  */
#define __HAL_RCC_LSE_CONFIG(__STATE__) HAL_RCC_LSEConfig(__STATE__)


/**
  * @brief  Macro to configures the External Low Speed oscillator (LSE) drive capability.
  * @note   As the LSE is in the Backup domain and write access is denied to
  *         this domain after reset, you have to enable write access using
  *         HAL_PWR_EnableBkUpAccess() function before to configure the LSE
  *         (to be done once after reset).
  * @param  __LSEDRIVE__: specifies the new state of the LSE drive capability.
  *          This parameter can be one of the following values:
  *            @arg RCC_LSEDRIVE_LOW: LSE oscillator low drive capability (default after backup domain reset)
  *            @arg RCC_LSEDRIVE_MEDIUMLOW: LSE oscillator medium low drive capability.
  *            @arg RCC_LSEDRIVE_MEDIUMHIGH: LSE oscillator medium high drive capability.
  *            @arg RCC_LSEDRIVE_HIGH: LSE oscillator high drive capability.
  * @retval None
  */
#define __HAL_RCC_LSEDRIVE_CONFIG(__LSEDRIVE__) \
                  MODIFY_REG(RCC->BDCR, RCC_BDCR_LSEDRV, (uint32_t)(__LSEDRIVE__))


/** @brief  Macro to get the LSE driving capability.
  * @retval The LSE Driving capability . The returned value can be one
  *         of the following:
  *             - RCC_LSEDRIVE_LOW: LSE oscillator low drive capability (default after backup domain reset)
  *             - RCC_LSEDRIVE_MEDIUMLOW: LSE oscillator medium low drive capability.
  *             - RCC_LSEDRIVE_MEDIUMHIGH: LSE oscillator medium high drive capability.
  *             - RCC_LSEDRIVE_HIGH: LSE oscillator high drive capability.
  */
#define  __HAL_RCC_GET_LSEDRIVE() ((uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_LSEDRV)))

/** @brief  Macros to enable or disable the the RTC clock.
  * @note   These macros must be used only after the RTC clock source was selected.
  */
#define __HAL_RCC_RTC_ENABLE()         SET_BIT(RCC->BDCR, RCC_BDCR_RTCCKEN)
#define __HAL_RCC_RTC_DISABLE()        CLEAR_BIT(RCC->BDCR, RCC_BDCR_RTCCKEN)

/** @brief  Macros to configure the RTC clock (RTCCLK).
  * @note   As the RTC clock configuration bits are in the Backup domain and write
  *         access is protected to this domain after reset, you have to enable write
  *         access using the Power Backup Access macro ( DBP bit in the PWR control
  *         register 1 PWR_CR1 has to be set to 1) before to configure
  *         the RTC clock source (to be done once after reset).
  * @note   Once the RTC clock is configured it can't be changed unless the
  *         Backup domain is reset using __HAL_RCC_BACKUPRESET_RELEASE() macro, or by
  *         a Power On Reset (POR).
  * @param  __RTCCLKSource__: specifies the RTC clock source.
  *         This parameter can be one of the following values:
  *            @arg RCC_RTCCLKSOURCE_OFF: No clock (default after backup domain reset)
  *            @arg RCC_RTCCLKSOURCE_LSE: LSE selected as RTC clock.
  *            @arg RCC_RTCCLKSOURCE_LSI: LSI selected as RTC clock.
  *            @arg RCC_RTCCLKSOURCE_HSE_DIV: HSE clock divided by RTCDIV value is used as RTC clock
  *
  * @note   If the LSE or LSI is used as RTC clock source, the RTC continues to
  *         work in STOP and STANDBY modes, and can be used as wakeup source.
  *         However, when the HSE clock is used as RTC clock source, the RTC
  *         cannot be used in STOP and STANDBY modes.
  * @note   The maximum input clock frequency for RTC is 1MHz (when using HSE as
  *         RTC clock source).
  */
#define __HAL_RCC_RTC_CONFIG(__RTCCLKSource__)  \
                 MODIFY_REG( RCC->BDCR, RCC_BDCR_RTCSRC, (__RTCCLKSource__))


/** @brief  Macro to get the clock source used as RTC clock.
  * @retval The clock source used as RTC clock. The returned value can be one
  *         of the following:
  *             - RCC_RTCCLKSOURCE_OFF:  No clock (default after backup domain reset)
  *             - RCC_RTCCLKSOURCE_LSE:  LSE used as RTC clock.
  *             - RCC_RTCCLKSOURCE_LSI:  LSI used as RTC clock.
  *             - RCC_RTCCLKSOURCE_HSE_DIV: HSE clock divided by RTCDIV value is used as RTC clock
  */
#define  __HAL_RCC_GET_RTC_SOURCE() ((uint32_t)(READ_BIT(RCC->BDCR, RCC_BDCR_RTCSRC)))


/** @brief  Macros to force or release the Backup domain reset.
  * @note   This function resets the RTC peripheral (including the backup registers)
  *
  * @note   The BKPSRAM and RETRAM are not affected
  */
#define __HAL_RCC_BACKUPRESET_FORCE()   SET_BIT(RCC->BDCR, RCC_BDCR_VSWRST)
#define __HAL_RCC_BACKUPRESET_RELEASE() CLEAR_BIT(RCC->BDCR, RCC_BDCR_VSWRST)


/** @brief  Macros to enable or disable the PLL1.
  * @note   After enabling the main PLL, the application software should wait on
  *         PLLRDY flag to be set indicating that PLL clock is stable and can
  *         be used as system clock source.
  * @note   The PLL1 can not be disabled if it is used as MPU clock source
  * @note   The PLL1 is disabled by hardware when entering STOP and STANDBY modes.
  */

#define __HAL_RCC_PLL1_ENABLE()     SET_BIT(RCC->PLL1CR, RCC_PLL1CR_PLLON )
#define __HAL_RCC_PLL1_DISABLE()    CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_PLLON)

/**
  * @brief  Enables or disables each clock output
  * @note   Enabling/disabling  Those Clocks outputs can be disabled anytime  without the need to stop the PLL,
  *         with the exception of ck_pll_p  cannot be stopped if used as MPU Clock
  * @param  __RCC_PLL1ClockOut__: specifies the PLL clock to be outputed
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL1_DIVP
  *            @arg RCC_PLL1_DIVQ
  *            @arg RCC_PLL1_DIVR
  * @retval None
  */

#define __HAL_RCC_PLL1CLKOUT_ENABLE(__RCC_PLL1ClockOut__)       SET_BIT(RCC->PLL1CR, __RCC_PLL1ClockOut__ )

#define __HAL_RCC_PLL1CLKOUT_DISABLE(__RCC_PLL1ClockOut__)      CLEAR_BIT(RCC->PLL1CR, __RCC_PLL1ClockOut__ )


/**
  * @brief  Enables or disables Fractional Part Of The Multiplication Factor of PLL1 VCO
  * @note   Enabling/disabling  Fractional Part can be done anytime  without the need to stop the PLL1
  * @retval None
  */

#define __HAL_RCC_PLL1FRACV_ENABLE()        SET_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACLE)

#define __HAL_RCC_PLL1FRACV_DISABLE()       CLEAR_BIT(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACLE)



/**
  * @brief  Macro to configure PLL1 and PLL2 clock source
  * @note   This function must be used only when the PLLs are disabled.
  *
  * @param  __RCC_PLL12SOURCE__: specifies the PLL1 and PLL2 entry clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL12SOURCE_HSI: HSI oscillator clock selected as PLL1 and PLL2 clock entry
  *            @arg RCC_PLL12SOURCE_HSE: HSE oscillator clock selected as PLL1 and PLL2 clock entry
  *            @arg RCC_PLL12SOURCE_OFF: No clock send to DIVMx divider and PLLs
  * @note   This clock source (__RCC_PLL12Source__) is common for the PLL1 and PLL2 .
  * @retval None
  */
#define __HAL_RCC_PLL12_SOURCE(__RCC_PLL12SOURCE__ ) \
                  do{ MODIFY_REG( RCC->RCK12SELR, RCC_RCK12SELR_PLL12SRC, __RCC_PLL12SOURCE__ );  \
                  } while(0)

/** @brief  Macro to get the clock source used as PLL1 and PLL2 clocks.
  * @retval The clock source used as PLL1 and PLL1 clocks. The returned value can be one
  *         of the following:
  *              - RCC_PLL12SOURCE_HSI: HSI used as PLL12 clock.
  *              - RCC_PLL12SOURCE_HSE: HSE used as PLL12 clock.
  *              - RCC_PLL12SOURCE_OFF: PLL1 and PLL2 clock is gated.
  */
#define __HAL_RCC_GET_PLL12_SOURCE() ((uint32_t)(RCC->RCK12SELR & RCC_RCK12SELR_PLL12SRC))

/**
  * @brief  Macro to configure PLL1 multiplication and division factors.
  * @note   This function must be used only when the PLL1 is disabled.
  *
  * @param  __PLLM1__: specifies the division factor for PLL VCO input clock
  *          This parameter must be a number between 1 and 64
  * @note   You have to set the PLLM1 parameter correctly to ensure that the VCO input
  *         frequency ranges from 8 to 16 MHz
  * @param  __PLLN1__: specifies the multiplication factor for PLL VCO output clock
  *          This parameter must be a number between 25 and 100
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 800 to 1600 MHz
  * @param  __PLLP1__: specifies the division factor for the PLL1 P output
  *          This parameter must be a number in the range 1 to 128
  * @param  __PLLQ1__: specifies the division factor for for the PLL1 Q output
  *          This parameter must be in the range 1 to 128
  * @param  __PLLR1__: specifies the division factor for for the PLL1 R output
  *          This parameter must be in the range 1 to 128
  * @retval None
  */
#define __HAL_RCC_PLL1_CONFIG(__PLLM1__, __PLLN1__, __PLLP1__, __PLLQ1__,__PLLR1__ ) \
                  do{  MODIFY_REG( RCC->PLL1CFGR1, (RCC_PLL1CFGR1_DIVN | RCC_PLL1CFGR1_DIVM1) , \
                                   ( (__PLLN1__ - 1U) | ( (__PLLM1__ - 1U) << 16U) ) );  \
                       MODIFY_REG( RCC->PLL1CFGR2, (RCC_PLL1CFGR2_DIVP | RCC_PLL1CFGR2_DIVQ | RCC_PLL1CFGR2_DIVR), \
                                   ( (__PLLP1__ - 1U) | ( (__PLLQ1__ - 1U) <<8U ) | ( (__PLLR1__ - 1U) <<16U) )); \
                    } while(0)



/**
  * @brief  Macro to configure the PLL1 clock Fractional Part Of The Multiplication Factor
  *
  * @note   These bits can be written at any time, allowing dynamic fine-tuning of the PLL1 VCO
  *
  * @param  __RCC_PLL1FRACV__: specifies Fractional Part Of The Multiplication Factorfor PLL1 VCO
  *            It should be a value between 0 and 8191 ((2^13)-1)
  * @note   Warning: the software has to set correctly these bits to insure that the VCO
  *               output frequency is between its valid frequency range, which is: 800 to 1600 MHz
  * @retval None
  */
#define  __HAL_RCC_PLL1FRACV_CONFIG(__RCC_PLL1FRACV__) \
                   MODIFY_REG(RCC->PLL1FRACR, RCC_PLL1FRACR_FRACV,\
                             (uint32_t)(__RCC_PLL1FRACV__) << RCC_PLL1FRACR_FRACV_Pos)


/** @brief  Macros to enable or disable the Spread Spectrum Clock Generator of PLL1
  */
#define  __HAL_RCC_PLL1_SSMODE_ENABLE()    SET_BIT(RCC->PLL1CR, RCC_PLL1CR_SSCG_CTRL)

#define  __HAL_RCC_PLL1_SSMODE_DISABLE()   CLEAR_BIT(RCC->PLL1CR, RCC_PLL1CR_SSCG_CTRL)


/** @brief  Macro to config the PLL1 Clock Spreading Generator
  * @param  __RCC_PLL1_MOD_PER__:       Modulation Period Adjustment for PLL1
  *          This parameter must have a value between 1 and 8191
  *
  * @param  __RCC_PLL1_TPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_TPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_TPDFN_DIS_DISABLED:  Dithering noise injection disabled

  * @param  __RCC_PLL1_RPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_RPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_RPDFN_DIS_DISABLED: Dithering noise injection disabled
  *
  *
  * @param  __RCC_PLL1_SSCG_MODE__:     Spread spectrum clock generator mode
  *          This parameter can be one of the following values:
  *            @arg RCC_SSCG_CENTER_SPREAD:  Center-spread modulation selected (default after reset)
  *            @arg RCC_SSCG_DOWN_SPREAD:    Down-spread modulation selected
  *
  * @param  __RCC_PLL1_INC_STEP__:   Modulation Depth Adjustment for PLL1
  *          This parameter must have a value between 1 and 32767
  * @note   MOD_PER x INC_STEP shall not exceed (2^15)-1
  * @retval None
  */
#define __HAL_RCC_PLL1CSGCONFIG(__RCC_PLL1_MOD_PER__, __RCC_PLL1_TPDFN_DIS__, __RCC_PLL1_RPDFN_DIS__, \
                                __RCC_PLL1_SSCG_MODE__, __RCC_PLL1_INC_STEP__ ) \
                  do{  MODIFY_REG( RCC->PLL1CSGR, (RCC_PLL1CSGR_MOD_PER | RCC_PLL1CSGR_TPDFN_DIS | RCC_PLL1CSGR_RPDFN_DIS | \
                                                   RCC_PLL1CSGR_SSCG_MODE | RCC_PLL1CSGR_INC_STEP) , \
                                   (__RCC_PLL1_MOD_PER__ | __RCC_PLL1_TPDFN_DIS__ | __RCC_PLL1_RPDFN_DIS__ | \
                                    __RCC_PLL1_SSCG_MODE__ | (__RCC_PLL1_INC_STEP__ << RCC_PLL1CSGR_INC_STEP_Pos)) ) ;  \
                     } while(0)



/** @brief  Macros to enable or disable the PLL2.
  * @note   After enabling the main PLL, the application software should wait on
  *         PLLRDY flag to be set indicating that PLL clock is stable and can
  *         be used as system clock source.
  * @note   The PLL2 can not be disabled if it is used as MPU clock source
  * @note   The PLL2 is disabled by hardware when entering STOP and STANDBY modes.
  */
#define __HAL_RCC_PLL2_ENABLE()     SET_BIT(RCC->PLL2CR, RCC_PLL2CR_PLLON )
#define __HAL_RCC_PLL2_DISABLE()    CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_PLLON)

/**
  * @brief  Enables or disables each clock output
  * @note   Enabling/disabling  Those Clocks outputs can be disabled anytime  without the need to stop the PLL
  * @param  __RCC_PLL2ClockOut__: specifies the PLL clock to be outputed
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL2_DIVP
  *            @arg RCC_PLL2_DIVQ
  *            @arg RCC_PLL2_DIVR
  * @retval None
  */

#define __HAL_RCC_PLL2CLKOUT_ENABLE(__RCC_PLL2ClockOut__)       SET_BIT(RCC->PLL2CR, __RCC_PLL2ClockOut__ )

#define __HAL_RCC_PLL2CLKOUT_DISABLE(__RCC_PLL2ClockOut__)      CLEAR_BIT(RCC->PLL2CR, __RCC_PLL2ClockOut__ )

/**
  * @brief  Enables or disables Fractional Part Of The Multiplication Factor of PLL2 VCO
  * @note   Enabling/disabling  Fractional Part can be done anytime  without the need to stop the PLL2
  * @retval None
  */

#define __HAL_RCC_PLL2FRACV_ENABLE()        SET_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACLE)

#define __HAL_RCC_PLL2FRACV_DISABLE()       CLEAR_BIT(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACLE)

/**
  * @brief  Macro to configure PLL2 multiplication and division factors.
  * @note   This function must be used only when the PLL2 is disabled.
  *
  * @param  __PLLM2__: specifies the division factor for PLL VCO input clock
  *          This parameter must be a number between 1 and 64
  * @note   You have to set the PLLM2 parameter correctly to ensure that the VCO input
  *         frequency ranges from 8 to 16 MHz
  * @param  __PLLN2__: specifies the multiplication factor for PLL VCO output clock
  *          This parameter must be a number between 25 and 100
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 800 to 1600 MHz
  * @param  __PLLP2__: specifies the division factor for the PLL2 P output
  *          This parameter must be a number in the range 1 to 128
  * @param  __PLLQ2__: specifies the division factor for for the PLL2 Q output
  *          This parameter must be in the range 1 to 128
  * @param  __PLLR2__: specifies the division factor for for the PLL2 R output
  *          This parameter must be in the range 1 to 128
  * @retval None
  */
#define __HAL_RCC_PLL2_CONFIG(__PLLM2__, __PLLN2__, __PLLP2__, __PLLQ2__,__PLLR2__ ) \
                  do{  MODIFY_REG( RCC->PLL2CFGR1, (RCC_PLL2CFGR1_DIVN | RCC_PLL2CFGR1_DIVM2) , \
                                   ( (__PLLN2__ - 1U) | ( (__PLLM2__ - 1U) << 16U) ) );  \
                       MODIFY_REG( RCC->PLL2CFGR2, (RCC_PLL2CFGR2_DIVP | RCC_PLL2CFGR2_DIVQ | RCC_PLL2CFGR2_DIVR), \
                                   ( (__PLLP2__ - 1U) | ( (__PLLQ2__ - 1U) <<8U ) | ( (__PLLR2__ - 1U) <<16U) )); \
                    } while(0)

/**
  * @brief  Macro to configure the PLL2 clock Fractional Part Of The Multiplication Factor
  *
  * @note   These bits can be written at any time, allowing dynamic fine-tuning of the PLL2 VCO
  *
  * @param  __RCC_PLL2FRACV__: specifies Fractional Part Of The Multiplication Factorfor PLL2 VCO
  *           It should be a value between 0 and 8191 ((2^13)-1)
  * @note   Warning: the software has to set correctly these bits to insure that the VCO
  *         output frequency is between its valid frequency range, which is: 800 to 1600 MHz
  * @retval None
  */
#define  __HAL_RCC_PLL2FRACV_CONFIG(__RCC_PLL2FRACV__) \
                   MODIFY_REG(RCC->PLL2FRACR, RCC_PLL2FRACR_FRACV, \
                              (uint32_t)(__RCC_PLL2FRACV__) << RCC_PLL2FRACR_FRACV_Pos)


/** @brief  Macros to enable or disable the Spread Spectrum Clock Generator of PLL2
  */
#define  __HAL_RCC_PLL2_SSMODE_ENABLE()    SET_BIT(RCC->PLL2CR, RCC_PLL2CR_SSCG_CTRL)

#define  __HAL_RCC_PLL2_SSMODE_DISABLE()   CLEAR_BIT(RCC->PLL2CR, RCC_PLL2CR_SSCG_CTRL)


/** @brief  Macro to config the PLL2 Clock Spreading Generator
  * @param  __RCC_PLL2_MOD_PER__:       Modulation Period Adjustment for PLL2
  *          This parameter must have a value between 1 and 8191
  *
  * @param  __RCC_PLL2_TPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_TPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_TPDFN_DIS_DISABLED:  Dithering noise injection disabled

  * @param  __RCC_PLL2_RPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_RPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_RPDFN_DIS_DISABLED: Dithering noise injection disabled
  *
  *
  * @param  __RCC_PLL2_SSCG_MODE__:     Spread spectrum clock generator mode
  *          This parameter can be one of the following values:
  *            @arg RCC_SSCG_CENTER_SPREAD:  Center-spread modulation selected (default after reset)
  *            @arg RCC_SSCG_DOWN_SPREAD:    Down-spread modulation selected
  *
  * @param  __RCC_PLL2_INC_STEP__:   Modulation Depth Adjustment for PLL2
  *          This parameter must have a value between 1 and 32767
  *
  * @note   MOD_PER x INC_STEP shall not exceed (2^15)-1
  * @retval None
  */
#define __HAL_RCC_PLL2CSGCONFIG(__RCC_PLL2_MOD_PER__, __RCC_PLL2_TPDFN_DIS__, __RCC_PLL2_RPDFN_DIS__, \
                                __RCC_PLL2_SSCG_MODE__, __RCC_PLL2_INC_STEP__ ) \
                  do{  MODIFY_REG( RCC->PLL2CSGR, (RCC_PLL2CSGR_MOD_PER | RCC_PLL2CSGR_TPDFN_DIS | RCC_PLL2CSGR_RPDFN_DIS | \
                                                   RCC_PLL2CSGR_SSCG_MODE | RCC_PLL2CSGR_INC_STEP) , \
                                   (__RCC_PLL2_MOD_PER__ | __RCC_PLL2_TPDFN_DIS__ | __RCC_PLL2_RPDFN_DIS__ | \
                                    __RCC_PLL2_SSCG_MODE__ | (__RCC_PLL2_INC_STEP__ << RCC_PLL2CSGR_INC_STEP_Pos)) ) ;  \
                     } while(0)

/** @brief  Macros to enable or disable the PLL3.
  * @note   After enabling the main PLL, the application software should wait on
  *         PLLRDY flag to be set indicating that PLL clock is stable and can
  *         be used as system clock source.
  * @note   The PLL3 can not be disabled if it is used as MPU clock source
  * @note   The PLL3 is disabled by hardware when entering STOP and STANDBY modes.
  */
#define __HAL_RCC_PLL3_ENABLE()     SET_BIT(RCC->PLL3CR, RCC_PLL3CR_PLLON )
#define __HAL_RCC_PLL3_DISABLE()    CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_PLLON)

/**
  * @brief  Enables or disables each clock output
  * @note   Enabling/disabling  Those Clocks outputs can be disabled anytime  without the need to stop the PLL,
  *         with the exception of ck_pll_p  cannot be stopped if used as MPU Clock
  * @param  __RCC_PLL3ClockOut__: specifies the PLL clock to be outputed
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL3_DIVP
  *            @arg RCC_PLL3_DIVQ
  *            @arg RCC_PLL3_DIVR
  * @retval None
  */
#define __HAL_RCC_PLL3CLKOUT_ENABLE(__RCC_PLL3ClockOut__)       SET_BIT(RCC->PLL3CR, __RCC_PLL3ClockOut__ )

#define __HAL_RCC_PLL3CLKOUT_DISABLE(__RCC_PLL3ClockOut__)      CLEAR_BIT(RCC->PLL3CR, __RCC_PLL3ClockOut__ )


/**
  * @brief  Enables or disables Fractional Part Of The Multiplication Factor of PLL3 VCO
  * @note   Enabling/disabling  Fractional Part can be done anytime  without the need to stop the PLL3
  * @retval None
  */

#define __HAL_RCC_PLL3FRACV_ENABLE()        SET_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACLE)

#define __HAL_RCC_PLL3FRACV_DISABLE()       CLEAR_BIT(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACLE)


/** @brief  Macros to enable or disable the Spread Spectrum Clock Generator of PLL3
  */
#define  __HAL_RCC_PLL3_SSMODE_ENABLE()    SET_BIT(RCC->PLL3CR, RCC_PLL3CR_SSCG_CTRL)

#define  __HAL_RCC_PLL3_SSMODE_DISABLE()   CLEAR_BIT(RCC->PLL3CR, RCC_PLL3CR_SSCG_CTRL)


/** @brief  Macro to config the PLL3 Clock Spreading Generator
  * @param  __RCC_PLL3_MOD_PER__:       Modulation Period Adjustment for PLL3
  *          This parameter must have a value between 1 and 8191
  *
  * @param  __RCC_PLL3_TPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_TPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_TPDFN_DIS_DISABLED:  Dithering noise injection disabled

  * @param  __RCC_PLL3_RPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_RPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_RPDFN_DIS_DISABLED: Dithering noise injection disabled
  *
  *
  * @param  __RCC_PLL3_SSCG_MODE__:     Spread spectrum clock generator mode
  *          This parameter can be one of the following values:
  *            @arg RCC_SSCG_CENTER_SPREAD:  Center-spread modulation selected (default after reset)
  *            @arg RCC_SSCG_DOWN_SPREAD:    Down-spread modulation selected
  *
  * @param  __RCC_PLL3_INC_STEP__:   Modulation Depth Adjustment for PLL3
  *          This parameter must have a value between 1 and 32767
  *
  * @note   MOD_PER x INC_STEP shall not exceed (2^15)-1
  * @retval None
  */
#define __HAL_RCC_PLL3CSGCONFIG(__RCC_PLL3_MOD_PER__, __RCC_PLL3_TPDFN_DIS__, __RCC_PLL3_RPDFN_DIS__, \
                                __RCC_PLL3_SSCG_MODE__, __RCC_PLL3_INC_STEP__ ) \
                  do{  MODIFY_REG( RCC->PLL3CSGR, (RCC_PLL3CSGR_MOD_PER | RCC_PLL3CSGR_TPDFN_DIS | RCC_PLL3CSGR_RPDFN_DIS | \
                                                   RCC_PLL3CSGR_SSCG_MODE | RCC_PLL3CSGR_INC_STEP) , \
                                   (__RCC_PLL3_MOD_PER__ | __RCC_PLL3_TPDFN_DIS__ | __RCC_PLL3_RPDFN_DIS__ | \
                                    __RCC_PLL3_SSCG_MODE__ | (__RCC_PLL3_INC_STEP__ << RCC_PLL3CSGR_INC_STEP_Pos)) ) ;  \
                     } while(0)


/**
  * @brief  Macro to configure PLL3 clock source
  * @note   This function must be used only when the PLL is disabled.
  *
  * @param  __RCC_PLL3SOURCE__: specifies the PLL3 entry clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL3SOURCE_HSI: HSI oscillator clock selected as PLL3 clock entry
  *            @arg RCC_PLL3SOURCE_HSE: HSE oscillator clock selected as PLL3 clock entry
  *            @arg RCC_PLL3SOURCE_CSI: HSE oscillator clock selected as PLL3 clock entry
  *            @arg RCC_PLL3SOURCE_OFF: No clock send to DIVMx divider and PLL
  * @retval None
  */
#define __HAL_RCC_PLL3_SOURCE(__RCC_PLL3SOURCE__ ) \
                  do{ MODIFY_REG( RCC->RCK3SELR, RCC_RCK3SELR_PLL3SRC, __RCC_PLL3SOURCE__ );  \
                  } while(0)

/** @brief  Macro to get the clock source used as PLL3 clock.
  * @retval The clock source used as PLL3 clock. The returned value can be one
  *         of the following:
  *             - RCC_PLL3SOURCE_HSI:  HSI used as PLL3 clock.
  *             - RCC_PLL3SOURCE_HSE:  HSE used as PLL3 clock.
  *             - RCC_PLL3SOURCE_CSI:  CSI used as PLL3 clock.
  *             - RCC_PLL3SOURCE_OFF:  PLL3 clock is gated.
  */
#define __HAL_RCC_GET_PLL3_SOURCE() ((uint32_t)(RCC->RCK3SELR & RCC_RCK3SELR_PLL3SRC))

/**
  * @brief  Macro to configure PLL3 multiplication and division factors.
  * @note   This function must be used only when the PLL3 is disabled.
  *
  * @param  __PLLM3__: specifies the division factor for PLL VCO input clock
  *          This parameter must be a number between 1 and 64
  * @note   You have to set the PLLM1 parameter correctly to ensure that the VCO input
  *         frequency ranges from 4 to 16 MHz
  * @param  __PLLN3__: specifies the multiplication factor for PLL VCO output clock
  *          This parameter must be a number between 25 and 200
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 400 to 800 MHz
  * @param  __PLLP3__: specifies the division factor for the PLL3 P output
  *          This parameter must be a number in the range 1 to 128
  * @param  __PLLQ3__: specifies the division factor for for the PLL3 Q output
  *          This parameter must be in the range 1 to 128
  * @param  __PLLR3__: specifies the division factor for for the PLL3 R output
  *          This parameter must be in the range 1 to 128
  * @retval None
  */
#define __HAL_RCC_PLL3_CONFIG(__PLLM3__, __PLLN3__, __PLLP3__, __PLLQ3__,__PLLR3__ ) \
                  do{  MODIFY_REG( RCC->PLL3CFGR1, (RCC_PLL3CFGR1_DIVN | RCC_PLL3CFGR1_DIVM3) , \
                                   ( (__PLLN3__ - 1U) | ( (__PLLM3__ - 1U) << 16U) ) );  \
                       MODIFY_REG( RCC->PLL3CFGR2, (RCC_PLL3CFGR2_DIVP | RCC_PLL3CFGR2_DIVQ | RCC_PLL3CFGR2_DIVR), \
                                   ( (__PLLP3__ - 1U) | ( (__PLLQ3__ - 1U) <<8U ) | ( (__PLLR3__ - 1U) <<16U) )); \
                    } while(0)

/**
  * @brief  Macro to configure the PLL3 clock Fractional Part Of The Multiplication Factor
  *
  * @note   These bits can be written at any time, allowing dynamic fine-tuning of the PLL3 VCO
  *
  * @param  __RCC_PLL3FRACV__: specifies Fractional Part Of The Multiplication Factorfor PLL3 VCO
  *           It should be a value between 0 and 8191 ((2^13)-1)
  * @note   Warning: the software has to set correctly these bits to insure that the VCO
  *         output frequency is between its valid frequency range, which is: 400 to 800 MHz
  * @retval None
  */
#define  __HAL_RCC_PLL3FRACV_CONFIG(__RCC_PLL3FRACV__) \
                   MODIFY_REG(RCC->PLL3FRACR, RCC_PLL3FRACR_FRACV, \
                             (uint32_t)(__RCC_PLL3FRACV__) << RCC_PLL3FRACR_FRACV_Pos)


/** @brief  Macro to select  the PLL3  input frequency range.
  * @param  __RCC_PLL3IFRange__: specifies the PLL3 input frequency range
  *         This parameter can be one of the following values:
  *            @arg RCC_PLL3IFRANGE_0: Range frequency is between 4 and 8 MHz (default after reset)
  *            @arg RCC_PLL3IFRANGE_1: Range frequency is between 8 and 16 MHz
  * @retval None
  */
#define __HAL_RCC_PLL3_IFRANGE(__RCC_PLL3IFRange__) \
                     MODIFY_REG(RCC->PLL3CFGR1, RCC_PLL3CFGR1_IFRGE, (__RCC_PLL3IFRange__))


/** @brief  Macros to enable or disable the PLL4.
  * @note   After enabling the main PLL, the application software should wait on
  *         PLLRDY flag to be set indicating that PLL clock is stable and can
  *         be used as system clock source.
  * @note   The PLL4 can not be disabled if it is used as MPU clock source
  * @note   The PLL4 is disabled by hardware when entering STOP and STANDBY modes.
  */
#define __HAL_RCC_PLL4_ENABLE()     SET_BIT(RCC->PLL4CR, RCC_PLL4CR_PLLON )
#define __HAL_RCC_PLL4_DISABLE()    CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_PLLON)

/**
  * @brief  Enables or disables each clock output
  * @note   Enabling/disabling  Those Clocks outputs can be disabled anytime  without the need to stop the PLL,
  *         with the exception of ck_pll_p  cannot be stopped if used as MPU Clock
  * @param  __RCC_PLL4ClockOut__: specifies the PLL clock to be outputed
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL4_DIVP
  *            @arg RCC_PLL4_DIVQ
  *            @arg RCC_PLL4_DIVR
  * @retval None
  */
#define __HAL_RCC_PLL4CLKOUT_ENABLE(__RCC_PLL4ClockOut__)       SET_BIT(RCC->PLL4CR, __RCC_PLL4ClockOut__ )

#define __HAL_RCC_PLL4CLKOUT_DISABLE(__RCC_PLL4ClockOut__)      CLEAR_BIT(RCC->PLL4CR, __RCC_PLL4ClockOut__ )


/**
  * @brief  Enables or disables Fractional Part Of The Multiplication Factor of PLL4 VCO
  * @note   Enabling/disabling  Fractional Part can be done anytime  without the need to stop the PLL4
  * @retval None
  */

#define __HAL_RCC_PLL4FRACV_ENABLE()        SET_BIT(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACLE)

#define __HAL_RCC_PLL4FRACV_DISABLE()       CLEAR_BIT(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACLE)


/** @brief  Macros to enable or disable the Spread Spectrum Clock Generator of PLL4
  */
#define  __HAL_RCC_PLL4_SSMODE_ENABLE()    SET_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL)

#define  __HAL_RCC_PLL4_SSMODE_DISABLE()   CLEAR_BIT(RCC->PLL4CR, RCC_PLL4CR_SSCG_CTRL)


/** @brief  Macro to config the PLL4 Clock Spreading Generator
  * @param  __RCC_PLL4_MOD_PER__:       Modulation Period Adjustment for PLL4
  *          This parameter must have a value between 1 and 8191
  *
  * @param  __RCC_PLL4_TPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_TPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_TPDFN_DIS_DISABLED:  Dithering noise injection disabled

  * @param  __RCC_PLL4_RPDFN_DIS__:      Dithering TPDF noise control
  *          This parameter can be one of the following values:
  *            @arg RCC_RPDFN_DIS_ENABLED: Dithering noise injection enabled (default after reset)
  *            @arg RCC_RPDFN_DIS_DISABLED: Dithering noise injection disabled
  *
  *
  * @param  __RCC_PLL4_SSCG_MODE__:     Spread spectrum clock generator mode
  *          This parameter can be one of the following values:
  *            @arg RCC_SSCG_CENTER_SPREAD:  Center-spread modulation selected (default after reset)
  *            @arg RCC_SSCG_DOWN_SPREAD:    Down-spread modulation selected
  *
  * @param  __RCC_PLL4_INC_STEP__:   Modulation Depth Adjustment for PLL4
  *          This parameter must have a value between 1 and 32767
  *
  * @note   MOD_PER x INC_STEP shall not exceed (2^15)-1
  * @retval None
  */
#define __HAL_RCC_PLL4CSGCONFIG(__RCC_PLL4_MOD_PER__, __RCC_PLL4_TPDFN_DIS__, __RCC_PLL4_RPDFN_DIS__, \
                                __RCC_PLL4_SSCG_MODE__, __RCC_PLL4_INC_STEP__ ) \
                  do{  MODIFY_REG( RCC->PLL4CSGR, (RCC_PLL4CSGR_MOD_PER | RCC_PLL4CSGR_TPDFN_DIS | RCC_PLL4CSGR_RPDFN_DIS | \
                                                   RCC_PLL4CSGR_SSCG_MODE | RCC_PLL4CSGR_INC_STEP) , \
                                   (__RCC_PLL4_MOD_PER__ | __RCC_PLL4_TPDFN_DIS__ | __RCC_PLL4_RPDFN_DIS__ | \
                                    __RCC_PLL4_SSCG_MODE__ | (__RCC_PLL4_INC_STEP__ << RCC_PLL4CSGR_INC_STEP_Pos)) ) ;  \
                     } while(0)


/**
  * @brief  Macro to configure PLL4 clock source
  * @note   This function must be used only when the PLL is disabled.
  *
  * @param  __RCC_PLL4SOURCE__: specifies the PLL4 entry clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_PLL4SOURCE_HSI: HSI oscillator clock selected as PLL4 clock entry
  *            @arg RCC_PLL4SOURCE_HSE: HSE oscillator clock selected as PLL4 clock entry
  *            @arg RCC_PLL4SOURCE_CSI: HSE oscillator clock selected as PLL4 clock entry
  *            @arg RCC_PLL4SOURCE_I2S_CKIN: Signal I2S_CKIN used as reference clock
  * @retval None
  */
#define __HAL_RCC_PLL4_SOURCE(__RCC_PLL4SOURCE__ ) \
                  do{ MODIFY_REG( RCC->RCK4SELR, RCC_RCK4SELR_PLL4SRC, __RCC_PLL4SOURCE__ );  \
                  } while(0)

/** @brief  Macro to get the clock source used as PLL4 clock.
  * @retval The clock source used as PLL4 clock. The returned value can be one
  *         of the following:
  *             - RCC_PLL4SOURCE_HSI:  HSI used as PLL4 clock.
  *             - RCC_PLL4SOURCE_HSE:  HSE used as PLL4 clock.
  *             - RCC_PLL4SOURCE_CSI:  CSI used as PLL4 clock.
  *             - RCC_PLL4SOURCE_I2S_CKIN:  I2S_CKIN used as PLL4 clock.
  */
#define __HAL_RCC_GET_PLL4_SOURCE() ((uint32_t)(RCC->RCK4SELR & RCC_RCK4SELR_PLL4SRC))


/**
  * @brief  Macro to configure PLL4 multiplication and division factors.
  * @note   This function must be used only when the PLL4 is disabled.
  *
  * @param  __PLLM4__: specifies the division factor for PLL VCO input clock
  *          This parameter must be a number between 1 and 64
  * @note   You have to set the PLLM1 parameter correctly to ensure that the VCO input
  *         frequency ranges from 4 to 16 MHz
  * @param  __PLLN4__: specifies the multiplication factor for PLL VCO output clock
  *          This parameter must be a number between 25 and 200
  * @note   You have to set the PLLN parameter correctly to ensure that the VCO
  *         output frequency is between 400 to 800 MHz
  * @param  __PLLP4__: specifies the division factor for the PLL4 P output
  *          This parameter must be a number in the range 1 to 128
  * @param  __PLLQ4__: specifies the division factor for for the PLL4 Q output
  *          This parameter must be in the range 1 to 128
  * @param  __PLLR4__: specifies the division factor for for the PLL4 R output
  *          This parameter must be in the range 1 to 128
  * @retval None
  */
#define __HAL_RCC_PLL4_CONFIG(__PLLM4__, __PLLN4__, __PLLP4__, __PLLQ4__,__PLLR4__ ) \
                  do{  MODIFY_REG( RCC->PLL4CFGR1, (RCC_PLL4CFGR1_DIVN | RCC_PLL4CFGR1_DIVM4) , \
                                   ( (__PLLN4__ - 1U) | ( (__PLLM4__ - 1U) << 16U) ) );  \
                       MODIFY_REG( RCC->PLL4CFGR2, (RCC_PLL4CFGR2_DIVP | RCC_PLL4CFGR2_DIVQ | RCC_PLL4CFGR2_DIVR), \
                                   ( (__PLLP4__ - 1U) | ( (__PLLQ4__ - 1U) <<8U ) | ( (__PLLR4__ - 1U) <<16U) )); \
                    } while(0)


/**
  * @brief  Macro to configure the PLL4 clock Fractional Part Of The Multiplication Factor
  *
  * @note   These bits can be written at any time, allowing dynamic fine-tuning of the PLL4 VCO
  *
  * @param  __RCC_PLL4FRACV__: specifies Fractional Part Of The Multiplication Factorfor PLL4 VCO
  *           It should be a value between 0 and 8191 ((2^13)-1)
  * @note   Warning: the software has to set correctly these bits to insure that the VCO
  *         output frequency is between its valid frequency range, which is: 400 to 800 MHz
  * @retval None
  */
#define  __HAL_RCC_PLL4FRACV_CONFIG(__RCC_PLL4FRACV__) \
                    MODIFY_REG(RCC->PLL4FRACR, RCC_PLL4FRACR_FRACV, \
                              (uint32_t)(__RCC_PLL4FRACV__) << RCC_PLL4FRACR_FRACV_Pos)


/** @brief  Macro to select  the PLL4  input frequency range.
  * @param  __RCC_PLL4IFRange__: specifies the PLL4 input frequency range
  *         This parameter can be one of the following values:
  *            @arg RCC_PLL4IFRANGE_0: Range frequency is between 4 and 8 MHz (default after reset)
  *            @arg RCC_PLL4IFRANGE_1: Range frequency is between 8 and 16 MHz
  * @retval None
  */
#define __HAL_RCC_PLL4_IFRANGE(__RCC_PLL4IFRange__) \
                      MODIFY_REG(RCC->PLL4CFGR1, RCC_PLL4CFGR1_IFRGE, (__RCC_PLL4IFRange__))


/** @brief  Macros to enable or disable the MCO1 output.
 *
 */
#define __HAL_RCC_MCO1_ENABLE()   SET_BIT(RCC->MCO1CFGR, RCC_MCO1CFGR_MCO1ON)
#define __HAL_RCC_MCO1_DISABLE()  CLEAR_BIT(RCC->MCO1CFGR, RCC_MCO1CFGR_MCO1ON)

/**
  * @brief  Macro to configure MCO1 prescaler
  * @note   Set and cleared by software to configure the prescaler of the MCO1.
  * @note   Modification of this prescaler may generate glitches on MCO1.
  * @note   It is highly recommended to change this prescaler only after reset,
  *         before enabling the external oscillators and the PLLs.
  *
  * @param  __RCC_MCO1SOURCE__: specifies the MCO1 entry clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCO1SOURCE_HSI: HSI clock selected as MCO1 clock entry (default after reset)
  *            @arg RCC_MCO1SOURCE_HSE: HSE clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_CSI: CSI clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_LSI: LSI clock selected as MCO1 clock entry
  *            @arg RCC_MCO1SOURCE_LSE: LSE clock selected as MCO1 clock entry
  *
  * @param  __RCC_MCO1PRESCALER__: specifies the MCO1 prescaler value.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCODIV_1:  bypass (default after reset)
  *            @arg RCC_MCODIV_2:  division by 2
  *            @arg RCC_MCODIV_3:  division by 3
  *            @arg RCC_MCODIV_4:  division by 4
  *            @arg RCC_MCODIV_5:  division by 5
  *            @arg RCC_MCODIV_6:  division by 6
  *            @arg RCC_MCODIV_7:  division by 7
  *            @arg RCC_MCODIV_8:  division by 8
  *            @arg RCC_MCODIV_9:  division by 9
  *            @arg RCC_MCODIV_10: division by 10
  *            @arg RCC_MCODIV_11: division by 11
  *            @arg RCC_MCODIV_12: division by 12
  *            @arg RCC_MCODIV_13: division by 13
  *            @arg RCC_MCODIV_14: division by 14
  *            @arg RCC_MCODIV_15: division by 15
  *            @arg RCC_MCODIV_16: division by 16
  * @retval None
  */
#define __HAL_RCC_MCO1_CONFIGURE(__RCC_MCO1SOURCE__ , __RCC_MCO1PRESCALER__ ) \
                do{  MODIFY_REG( RCC->MCO1CFGR, (RCC_MCO1CFGR_MCO1SEL | RCC_MCO1CFGR_MCO1DIV) , \
                                 (__RCC_MCO1SOURCE__ | __RCC_MCO1PRESCALER__) );\
                } while(0)


/** @brief  Macro to get the clock source used as MCO1 clock.
  * @retval The clock source used as MCO1 clock. The returned value can be one
  *         of the following:
  *              - RCC_MCO1SOURCE_HSI:  HSI used as MCO1 clock (default after reset).
  *              - RCC_MCO1SOURCE_HSE:  HSE used as MCO1 clock.
  *              - RCC_MCO1SOURCE_CSI:  CSI used as MCO1 clock.
  *              - RCC_MCO1SOURCE_LSI:  LSI used as MCO1 clock.
  *              - RCC_MCO1SOURCE_LSE:  LSE used as MCO1 clock.
  */
#define __HAL_RCC_GET_MCO1_SOURCE() ((uint32_t)(RCC->MCO1CFGR & RCC_MCO1CFGR_MCO1SEL))

/** @brief  Macro to get the MCO1 prescaler value
  * @retval The MCO1 prescaler value. The returned value can be one
  *         of the following:
  *            - RCC_MCODIV_1:  bypass (default after reset)
  *            - RCC_MCODIV_2:  division by 2
  *            - RCC_MCODIV_3:  division by 3
  *            - RCC_MCODIV_4:  division by 4
  *            - RCC_MCODIV_5:  division by 5
  *            - RCC_MCODIV_6:  division by 6
  *            - RCC_MCODIV_7:  division by 7
  *            - RCC_MCODIV_8:  division by 8
  *            - RCC_MCODIV_9:  division by 9
  *            - RCC_MCODIV_10: division by 10
  *            - RCC_MCODIV_11: division by 11
  *            - RCC_MCODIV_12: division by 12
  *            - RCC_MCODIV_13: division by 13
  *            - RCC_MCODIV_14: division by 14
  *            - RCC_MCODIV_15: division by 15
  *            - RCC_MCODIV_16: division by 16
  */
#define __HAL_RCC_GET_MCO1_DIV() ((uint32_t)(RCC->MCO1CFGR & RCC_MCO1CFGR_MCO1DIV))


/** @brief  Macros to enable or disable the MCO2 output.
  *
  */
#define __HAL_RCC_MCO2_ENABLE()   SET_BIT(RCC->MCO2CFGR, RCC_MCO2CFGR_MCO2ON)
#define __HAL_RCC_MCO2_DISABLE()  CLEAR_BIT(RCC->MCO2CFGR, RCC_MCO2CFGR_MCO2ON)

/**
  * @brief  Macro to configure MCO2 prescaler
  * @note   Set and cleared by software to configure the prescaler of the MCO2.
  * @note   Modification of this prescaler may generate glitches on MCO2.
  * @note   It is highly recommended to change this prescaler only after reset,
  *         before enabling the external oscillators and the PLLs.
  *
  * @param  __RCC_MCO2SOURCE__: specifies the MCO2 entry clock source.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCO2SOURCE_MPU: MPU clock selected as MCO2 clock entry (default after reset)
  *            @arg RCC_MCO2SOURCE_AXI: AXI clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_MCU: MCU clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_PLL4: PLL4 clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_HSE: HSE clock selected as MCO2 clock entry
  *            @arg RCC_MCO2SOURCE_HSI: HSI clock selected as MCO2 clock entry
  *
  * @param  __RCC_MCO2PRESCALER__: specifies the MCO2 prescaler value.
  *          This parameter can be one of the following values:
  *            @arg RCC_MCODIV_1:  bypass (default after reset)
  *            @arg RCC_MCODIV_2:  division by 2
  *            @arg RCC_MCODIV_3:  division by 3
  *            @arg RCC_MCODIV_4:  division by 4
  *            @arg RCC_MCODIV_5:  division by 5
  *            @arg RCC_MCODIV_6:  division by 6
  *            @arg RCC_MCODIV_7:  division by 7
  *            @arg RCC_MCODIV_8:  division by 8
  *            @arg RCC_MCODIV_9:  division by 9
  *            @arg RCC_MCODIV_10: division by 10
  *            @arg RCC_MCODIV_11: division by 11
  *            @arg RCC_MCODIV_12: division by 12
  *            @arg RCC_MCODIV_13: division by 13
  *            @arg RCC_MCODIV_14: division by 14
  *            @arg RCC_MCODIV_15: division by 15
  *            @arg RCC_MCODIV_16: division by 16
  * @retval None
  */
#define __HAL_RCC_MCO2_CONFIGURE(__RCC_MCO2SOURCE__ , __RCC_MCO2PRESCALER__ ) \
                do{  MODIFY_REG( RCC->MCO2CFGR, (RCC_MCO2CFGR_MCO2SEL | RCC_MCO2CFGR_MCO2DIV) , \
                                 (__RCC_MCO2SOURCE__ | __RCC_MCO2PRESCALER__) );\
                } while(0)


/** @brief  Macro to get the clock source used as MCO2 clock.
  * @retval The clock source used as MCO2 clock. The returned value can be one
  *         of the following:
  *              - RCC_MCO2SOURCE_MPU:  MPU used as MCO2 clock (default after reset).
  *              - RCC_MCO2SOURCE_AXI:  AXI used as MCO2 clock.
  *              - RCC_MCO2SOURCE_MCU:  MCU used as MCO2 clock.
  *              - RCC_MCO2SOURCE_PLL4: PLL4 used as MCO2 clock.
  *              - RCC_MCO2SOURCE_HSE:  HSE used as MCO2 clock.
  *              - RCC_MCO2SOURCE_HSI:  HSI used as MCO2 clock.
  */
#define __HAL_RCC_GET_MCO2_SOURCE() ((uint32_t)(RCC->MCO2CFGR & RCC_MCO2CFGR_MCO2SEL))

/** @brief  Macro to get the MCO2 prescaler value
  * @retval The MCO2 prescaler value. The returned value can be one
  *         of the following:
  *            - RCC_MCODIV_1:  bypass (default after reset)
  *            - RCC_MCODIV_2:  division by 2
  *            - RCC_MCODIV_3:  division by 3
  *            - RCC_MCODIV_4:  division by 4
  *            - RCC_MCODIV_5:  division by 5
  *            - RCC_MCODIV_6:  division by 6
  *            - RCC_MCODIV_7:  division by 7
  *            - RCC_MCODIV_8:  division by 8
  *            - RCC_MCODIV_9:  division by 9
  *            - RCC_MCODIV_10: division by 10
  *            - RCC_MCODIV_11: division by 11
  *            - RCC_MCODIV_12: division by 12
  *            - RCC_MCODIV_13: division by 13
  *            - RCC_MCODIV_14: division by 14
  *            - RCC_MCODIV_15: division by 15
  *            - RCC_MCODIV_16: division by 16
  */
#define __HAL_RCC_GET_MCO2_DIV() ((uint32_t)(RCC->MCO2CFGR & RCC_MCO2CFGR_MCO2DIV))


/**
  * @brief  Macro to configure MPU sub-system clock source
  * @note   Set and reset by software to select the MPU sub-system clock source (ck_mpuss)
  * @note   If TZEN = 1, it is only possible to modify this parameter in secure mode
  * @note   Write access is not allowed during the clock restore sequence
  *
  * @param  __RCC_MPUSOURCE__: specifies the MPU entry clock source.
  *          This parameter can be one of the following values:
  *              @arg RCC_MPUSOURCE_HSI:   HSI used as MPU clock (default after reset).
  *              @arg RCC_MPUSOURCE_HSE:   HSE used as MPU clock.
  *              @arg RCC_MPUSOURCE_PLL1:  PLL1 used as MPU clock.
  *              @arg RCC_MPUSOURCE_MPUDIV: MPUDIV used as MPU clock.
  */
#define __HAL_RCC_MPU_SOURCE(__RCC_MPUSOURCE__) \
                do{  MODIFY_REG( RCC->MPCKSELR, RCC_MPCKSELR_MPUSRC , __RCC_MPUSOURCE__ );\
                } while(0)

/** @brief  Macro to get the clock source used as MPU clock.
  * @retval The clock source used as MPU clock. The returned value can be one
  *         of the following:
  *              - RCC_MPUSOURCE_HSI:   HSI used as MPU clock (default after reset).
  *              - RCC_MPUSOURCE_HSE:   HSE used as MPU clock.
  *              - RCC_MPUSOURCE_PLL1:  PLL1 used as MPU clock.
  *              - RCC_MPUSOURCE_MPUDIV: MPU used as MPU clock.
  */
#define __HAL_RCC_GET_MPU_SOURCE() ((uint32_t)(RCC->MPCKSELR & RCC_MPCKSELR_MPUSRC))


/**
  * @brief  Macro to configure AXISS clock source
  * @note   Set and reset by software to select the AXI sub-system clock source (ck_axiss)
  * @note   If TZEN = 1, it is only possible to modify this parameter in secure mode
  * @note   Write access is not allowed during the clock restore sequence
  *
  * @param  __RCC_AXISSOURCE__: specifies the AXISS entry clock source.
  *          This parameter can be one of the following values:
  *              @arg RCC_AXISSOURCE_HSI:   HSI used as AXISS clock (default after reset).
  *              @arg RCC_AXISSOURCE_HSE:   HSE used as AXISS clock.
  *              @arg RCC_AXISSOURCE_PLL2:  PLL2 used as AXISS clock.
  *              @arg RCC_AXISSOURCE_OFF:   AXISS clock is gated.
  */
#define __HAL_RCC_AXISS_SOURCE(__RCC_AXISSOURCE__) \
                do{  MODIFY_REG( RCC->ASSCKSELR, RCC_ASSCKSELR_AXISSRC , __RCC_AXISSOURCE__ );\
                } while(0)

/** @brief  Macro to get the clock source used as AXIS clock.
  * @retval The clock source used as AXIS clock. The returned value can be one
  *         of the following:
  *              - RCC_AXISSOURCE_HSI:  HSI used as AXIS clock (default after reset).
  *              - RCC_AXISSOURCE_HSE:  HSE used as AXIS clock.
  *              - RCC_AXISSOURCE_PLL2: PLL2 used as AXIS clock.
  *              - RCC_AXISSOURCE_OFF:  AXIS clock is gated.
  */
#define __HAL_RCC_GET_AXIS_SOURCE() ((uint32_t)(RCC->ASSCKSELR & RCC_ASSCKSELR_AXISSRC))


/**
  * @brief  Macro to configure MCU sub-system clock source
  * @note   Set and reset by software to select the mcu sub-system clock source (ck_mcuss)
  * @note   If TZEN = 1, it is only possible to modify this parameter in secure mode
  * @note   Write access is not allowed during the clock restore sequence
  *
  * @param  __RCC_MCUSSOURCE__: specifies the MCU entry clock source.
  *          This parameter can be one of the following values:
  *              @arg RCC_MCUSSOURCE_HSI:   HSI used as MCU clock (default after reset).
  *              @arg RCC_MCUSSOURCE_HSE:   HSE used as MCU clock.
  *              @arg RCC_MCUSSOURCE_PLL3:  PLL3 used as MCU clock.
  */
#define __HAL_RCC_MCU_SOURCE(__RCC_MCUSSOURCE__) \
                 do{  MODIFY_REG( RCC->MSSCKSELR, RCC_MSSCKSELR_MCUSSRC , __RCC_MCUSSOURCE__ );\
                 } while(0)

/** @brief  Macro to get the clock source used as MCU clock.
  * @retval The clock source used as MCU clock. The returned value can be one
  *         of the following:
  *             - RCC_MCUSSOURCE_HSI:  HSI used as MCU clock (default after reset).
  *             - RCC_MCUSSOURCE_HSE:  HSE used as MCU clock.
  *             - RCC_MCUSSOURCE_CSI:  CSI used as MCU clock.
  *             - RCC_MCUSSOURCE_PLL3: PLL3 used as MCU clock.
  */
#define __HAL_RCC_GET_MCU_SOURCE() ((uint32_t)(RCC->MSSCKSELR & RCC_MSSCKSELR_MCUSSRC))



/**
  * @brief  Macro to set the HSE division factor for RTC clock
  * @note   Set and cleared by software to divide the HSE to generate a clock for RTC.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode
  * @note   Caution: The software has to set these bits correctly to ensure that the clock supplied to the RTC is lower than 4 MHz.
  * @note   These bits must be configured if needed before selecting the RTC clock source
  *
  * @param  __HSEDIV__: Specifies the HSE division factor for RTC clock
  *               This parameter must be a number between 1 and 64. Eg.:
  *               1:  HSE (default after reset)
  *               2:  HSE/2
  *               64: HSE/64
  * @retval None
  */
#define __HAL_RCC_RTC_HSEDIV(__HSEDIV__) \
                  do{  MODIFY_REG( RCC->RTCDIVR, RCC_RTCDIVR_RTCDIV ,  (__HSEDIV__ -1U ));  \
                    } while(0)

/** @brief  Macro to get the HSE division factor for RTC clock.
  * @retval The HSE division factor for RTC clock. The returned value can be a number between 1 and 64 Eg.:
  *               1:  HSE (default after reset)
  *               2:  HSE/2
  *               64: HSE/64
  */
#define __HAL_RCC_GET_RTC_HSEDIV() ((uint32_t)((RCC->RTCDIVR & RCC_RTCDIVR_RTCDIV) + 1U ))

/**
  * @brief  Macro to configure MPU Core clock divider
  * @note   Set and reset by software to control the MPU clock division factor.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode.
  * @note   It is possible to change this division ratio on-the-fly. It impacts only the frequency of MPU clock.
  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_mpuss cycles after MPUDIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the MPUDIVRDY flag
  *
  * @param  __RCC_MPUDIV__: specifies the MPU division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_MPU_DIV_OFF:  The MPUDIV is disabled (default after reset)
  *              @arg RCC_MPU_DIV2:  ck_mpuss divided by 2
  *              @arg RCC_MPU_DIV4:  ck_mpuss divided by 4
  *              @arg RCC_MPU_DIV8:  ck_mpuss divided by 8
  *              @arg RCC_MPU_DIV16: ck_mpuss divided by 16
  *
  * @note   Warning: Do not set RCC_MPU_DIV_OFF if the MPUSRC = RCC_MPUSOURCE_MPUDIV,
  *         otherwise the processor will no longer be clocked
  */
#define __HAL_RCC_MPU_DIV(__RCC_MPUDIV__) \
                 do{  MODIFY_REG( RCC->MPCKDIVR, RCC_MPCKDIVR_MPUDIV , __RCC_MPUDIV__ );\
                 } while(0)

/** @brief  Macro to get the MPU clock division factor.
  * @retval The MPU clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_MPU_DIV_OFF:  The MPUDIV is disabled (default after reset)
  *              - RCC_MPU_DIV2:  ck_mpuss divided by 2
  *              - RCC_MPU_DIV4:  ck_mpuss divided by 4
  *              - RCC_MPU_DIV8:  ck_mpuss divided by 8
  *              - RCC_MPU_DIV16: ck_mpuss divided by 16
  */
#define __HAL_RCC_GET_MPU_DIV() ((uint32_t)(RCC->MPCKDIVR & RCC_MPCKDIVR_MPUDIV ))


/**
  * @brief  Macro to configure AXI Core clock divider
  * @note   Set and reset by software to control the  AXI, AHB5 and AHB6 clock division factor.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode.
  * @note   It is possible to change this division ratio on-the-fly. It impacts the frequency
  *         of AXI, APB4, APB5 AHB5 and AHB6 clocks.
  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_axiss cycles after AXIDIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the AXIDIVRDY flag
  *
  * @param  __RCC_AXIDIV__: specifies the AXI division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_AXI_DIV1:  ck_axiss not divided (default after reset)
  *              @arg RCC_AXI_DIV2:  ck_axiss divided by 2
  *              @arg RCC_AXI_DIV3:  ck_axiss divided by 3
  *              @arg RCC_AXI_DIV4:  ck_axiss divided by 4
  */
#define __HAL_RCC_AXI_DIV(__RCC_AXIDIV__) \
                 do{  MODIFY_REG( RCC->AXIDIVR, RCC_AXIDIVR_AXIDIV , __RCC_AXIDIV__ );\
                 } while(0)

/** @brief  Macro to get the AXI, AHB5 and AHB6 clock division factor.
  * @retval The AXI clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_AXI_DIV1:  ck_axiss not divided (default after reset)
  *              - RCC_AXI_DIV2:  ck_axiss divided by 2
  *              - RCC_AXI_DIV3:  ck_axiss divided by 3
  *              - RCC_AXI_DIV4:  ck_axiss divided by 4
  */
#define __HAL_RCC_GET_AXI_DIV() ((uint32_t)(RCC->AXIDIVR & RCC_AXIDIVR_AXIDIV ))


/**
  * @brief  Macro to configure APB4 clock divider
  * @note   Set and reset by software to control the  APB4 clock division factor.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode.
  * @note   It is possible to change this division ratio on-the-fly. It impacts only the APB4 clock.
  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_aclk cycles after APB4DIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the APB4DIVRDY flag
  *
  * @param  __RCC_APB4DIV__: specifies the APB4 division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_APB4_DIV1:  ck_aclk not divided (default after reset)
  *              @arg RCC_APB4_DIV2:  ck_aclk divided by 2
  *              @arg RCC_APB4_DIV4:  ck_aclk divided by 4
  *              @arg RCC_APB4_DIV8:  ck_aclk divided by 8
  *              @arg RCC_APB4_DIV16: ck_aclk divided by 16
  */
#define __HAL_RCC_APB4_DIV(__RCC_APB4DIV__) \
                 do{  MODIFY_REG( RCC->APB4DIVR, RCC_APB4DIVR_APB4DIV , __RCC_APB4DIV__ );\
                 } while(0)

/** @brief  Macro to get the APB4 clock division factor.
  * @retval The APB4 clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_APB4_DIV1:  ck_aclk not divided (default after reset)
  *              - RCC_APB4_DIV2:  ck_aclk divided by 2
  *              - RCC_APB4_DIV4:  ck_aclk divided by 4
  *              - RCC_APB4_DIV8:  ck_aclk divided by 8
  *              - RCC_APB4_DIV16: ck_aclk divided by 16
  */
#define __HAL_RCC_GET_APB4_DIV() ((uint32_t)(RCC->APB4DIVR & RCC_APB4DIVR_APB4DIV ))


/**
  * @brief  Macro to configure APB5 clock divider
  * @note   Set and reset by software to control the  APB5 clock division factor.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode.
  * @note   It is possible to change this division ratio on-the-fly. It impacts only the APB5 clock.
  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_aclk cycles after APB5DIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the APB5DIVRDY flag
  *
  * @param  __RCC_APB5DIV__: specifies the APB5 division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_APB5_DIV1:  ck_aclk not divided (default after reset)
  *              @arg RCC_APB5_DIV2:  ck_aclk divided by 2
  *              @arg RCC_APB5_DIV4:  ck_aclk divided by 4
  *              @arg RCC_APB5_DIV8:  ck_aclk divided by 8
  *              @arg RCC_APB5_DIV16: ck_aclk divided by 16
  */
#define __HAL_RCC_APB5_DIV(__RCC_APB5DIV__) \
                 do{  MODIFY_REG( RCC->APB5DIVR, RCC_APB5DIVR_APB5DIV , __RCC_APB5DIV__ );\
                 } while(0)

/** @brief  Macro to get the APB5 clock division factor.
  * @retval The APB5 clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_APB5_DIV1:  ck_aclk not divided (default after reset)
  *              - RCC_APB5_DIV2:  ck_aclk divided by 2
  *              - RCC_APB5_DIV4:  ck_aclk divided by 4
  *              - RCC_APB5_DIV8:  ck_aclk divided by 8
  *              - RCC_APB5_DIV16: ck_aclk divided by 16
  */
#define __HAL_RCC_GET_APB5_DIV() ((uint32_t)(RCC->APB5DIVR & RCC_APB5DIVR_APB5DIV ))


/**
  * @brief  Macro to configure MCU clock divider
  * @note   Set and reset by software to control the  MCU clock division factor.
  * @note   If TZEN = 1, this parameter can only be modified in secure mode.
  * @note   Changing this division ratio has an impact on the frequency of MCU clock, and all bus matrix clocks.
  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_mcuss cycles after MCUDIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the MCUDIVRDY flag
  *
  * @param  __RCC_MCUDIV__: specifies the MCU division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_MCU_DIV1:  ck_mcuss not divided (default after reset)
  *              @arg RCC_MCU_DIV2:  ck_mcuss divided by 2
  *              @arg RCC_MCU_DIV4:  ck_mcuss divided by 4
  *              @arg RCC_MCU_DIV8:  ck_mcuss divided by 8
  *              @arg RCC_MCU_DIV16: ck_mcuss divided by 16
  *              @arg RCC_MCU_DIV32: ck_mcuss divided by 32
  *              @arg RCC_MCU_DIV64: ck_mcuss divided by 64
  *              @arg RCC_MCU_DIV128: ck_mcuss divided by 128
  *              @arg RCC_MCU_DIV256: ck_mcuss divided by 256
  *              @arg RCC_MCU_DIV512: ck_mcuss divided by 512
  */
#define __HAL_RCC_MCU_DIV(__RCC_MCUDIV__) \
                 do{  MODIFY_REG( RCC->MCUDIVR, RCC_MCUDIVR_MCUDIV , __RCC_MCUDIV__ );\
                 } while(0)

/** @brief  Macro to get the MCU clock division factor.
  * @retval The MCU clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_MCU_DIV1:  ck_mcuss not divided (default after reset)
  *              - RCC_MCU_DIV2:  ck_mcuss divided by 2
  *              - RCC_MCU_DIV4:  ck_mcuss divided by 4
  *              - RCC_MCU_DIV8:  ck_mcuss divided by 8
  *              - RCC_MCU_DIV16: ck_mcuss divided by 16
  *              - RCC_MCU_DIV32: ck_mcuss divided by 32
  *              - RCC_MCU_DIV64: ck_mcuss divided by 64
  *              - RCC_MCU_DIV128: ck_mcuss divided by 128
  *              - RCC_MCU_DIV256: ck_mcuss divided by 256
  *              - RCC_MCU_DIV512: ck_mcuss divided by 512
  */
#define __HAL_RCC_GET_MCU_DIV() ((uint32_t)(RCC->MCUDIVR & RCC_MCUDIVR_MCUDIV ))

/**
  * @brief  Macro to configure APB1 clock divider
  * @note   Set and reset by software to control the  APB1 clock division factor.
  * @note   It is possible to change this division ratio on-the-fly. It impacts only the APB1 clock.
  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_hclk cycles after APB1DIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the APB1DIVRDY flag
  *
  * @param  __RCC_APB1DIV__: specifies the APB1 division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_APB1_DIV1:  ck_hclk not divided (default after reset)
  *              @arg RCC_APB1_DIV2:  ck_hclk divided by 2
  *              @arg RCC_APB1_DIV4:  ck_hclk divided by 4
  *              @arg RCC_APB1_DIV8:  ck_hclk divided by 8
  *              @arg RCC_APB1_DIV16: ck_hclk divided by 16
  */
#define __HAL_RCC_APB1_DIV(__RCC_APB1DIV__) \
                 do{  MODIFY_REG( RCC->APB1DIVR, RCC_APB1DIVR_APB1DIV , __RCC_APB1DIV__ );\
                 } while(0)

/** @brief  Macro to get the APB1 clock division factor.
  * @retval The APB1 clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_APB1_DIV1:  ck_hclk not divided (default after reset)
  *              - RCC_APB1_DIV2:  ck_hclk divided by 2
  *              - RCC_APB1_DIV4:  ck_hclk divided by 4
  *              - RCC_APB1_DIV8:  ck_hclk divided by 8
  *              - RCC_APB1_DIV16: ck_hclk divided by 16
  */
#define __HAL_RCC_GET_APB1_DIV() ((uint32_t)(RCC->APB1DIVR & RCC_APB1DIVR_APB1DIV ))


/**
  * @brief  Macro to configure APB2 clock divider
  * @note   Set and reset by software to control the  APB2 clock division factor.
  * @note   It is possible to change this division ratio on-the-fly. It impacts only the APB2 clock.

  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_hclk cycles after APB2DIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the APB2DIVRDY flag
  *
  * @param  __RCC_APB2DIV__: specifies the APB2 division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_APB2_DIV1:  ck_hclk not divided (default after reset)
  *              @arg RCC_APB2_DIV2:  ck_hclk divided by 2
  *              @arg RCC_APB2_DIV4:  ck_hclk divided by 4
  *              @arg RCC_APB2_DIV8:  ck_hclk divided by 8
  *              @arg RCC_APB2_DIV16: ck_hclk divided by 16
  */
#define __HAL_RCC_APB2_DIV(__RCC_APB2DIV__) \
                 do{  MODIFY_REG( RCC->APB2DIVR, RCC_APB2DIVR_APB2DIV , __RCC_APB2DIV__ );\
                 } while(0)

/** @brief  Macro to get the APB2 clock division factor.
  * @retval The APB2 clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_APB2_DIV1:  ck_hclk not divided (default after reset)
  *              - RCC_APB2_DIV2:  ck_hclk divided by 2
  *              - RCC_APB2_DIV4:  ck_hclk divided by 4
  *              - RCC_APB2_DIV8:  ck_hclk divided by 8
  *              - RCC_APB2_DIV16: ck_hclk divided by 16
  */
#define __HAL_RCC_GET_APB2_DIV() ((uint32_t)(RCC->APB2DIVR & RCC_APB2DIVR_APB2DIV ))


/**
  * @brief  Macro to configure APB3 clock divider
  * @note   Set and reset by software to control the  APB3 clock division factor.
  * @note   It is possible to change this division ratio on-the-fly. It impacts only the APB3 clock.

  * @note   The clocks are divided with the new prescaler factor, from 1 to 16 ck_hclk cycles after APB3DIV update.
  * @note   The application can check if the new division factor is taken into account by reading back the APB3DIVRDY flag
  *
  * @param  __RCC_APB3DIV__: specifies the APB3 division factor
  *          This parameter can be one of the following values:
  *              @arg RCC_APB3_DIV1:  ck_hclk not divided (default after reset)
  *              @arg RCC_APB3_DIV2:  ck_hclk divided by 2
  *              @arg RCC_APB3_DIV4:  ck_hclk divided by 4
  *              @arg RCC_APB3_DIV8:  ck_hclk divided by 8
  *              @arg RCC_APB3_DIV16: ck_hclk divided by 16
  */
#define __HAL_RCC_APB3_DIV(__RCC_APB3DIV__) \
                 do{  MODIFY_REG( RCC->APB3DIVR, RCC_APB3DIVR_APB3DIV , __RCC_APB3DIV__ );\
                 } while(0)

/** @brief  Macro to get the APB3 clock division factor.
  * @retval The APB3 clock division factor. The returned value can be one
  *         of the following:
  *              - RCC_APB3_DIV1:  ck_hclk not divided (default after reset)
  *              - RCC_APB3_DIV2:  ck_hclk divided by 2
  *              - RCC_APB3_DIV4:  ck_hclk divided by 4
  *              - RCC_APB3_DIV8:  ck_hclk divided by 8
  *              - RCC_APB3_DIV16: ck_hclk divided by 16
  */
#define __HAL_RCC_GET_APB3_DIV() ((uint32_t)(RCC->APB3DIVR & RCC_APB3DIVR_APB3DIV ))


/** @brief  Enable RCC interrupt (Perform Byte access to RCC_CIR bits to enable
  *         the selected interrupts).
  * @param  __INTERRUPT__: specifies the RCC interrupt sources to be enabled.
  *         This parameter can be any combination of the following values:
  *            @arg RCC_IT_LSIRDY:  LSI ready interrupt
  *            @arg RCC_IT_LSERDY:  LSE ready interrupt
  *            @arg RCC_IT_HSIRDY:  HSI ready interrupt
  *            @arg RCC_IT_HSERDY:  HSE ready interrupt
  *            @arg RCC_IT_CSIRDY:  CSI ready interrupt
  *            @arg RCC_IT_PLLRDY:  Main PLL ready interrupt
  *            @arg RCC_IT_PLL2RDY: PLL2RDY ready interrupt
  *            @arg RCC_IT_PLL3RDY: PLL3RDY ready interrupt
  *            @arg RCC_IT_PLL4RDY: PLL4RDY ready interrupt
  *            @arg RCC_IT_LSECSS:  LSE Clock security system interrupt
  *            @arg RCC_IT_WKUP:    Wake-up from CSTOP Interrupt
  *            @arg RCC_IT_ALL:     All RCC interrupts
  */
#ifdef CORE_CM4
#define __HAL_RCC_ENABLE_IT(__INTERRUPT__) SET_BIT(RCC->MC_CIER, (__INTERRUPT__))
#endif /* CORE_CM4 */
#ifdef CORE_CA7
#define __HAL_RCC_ENABLE_IT(__INTERRUPT__) SET_BIT(RCC->MP_CIER, (__INTERRUPT__))
#endif /* CORE_CA7 */

/** @brief Disable RCC interrupt (Perform Byte access to RCC_CIR[14:8] bits to disable
  *        the selected interrupts).
  * @param  __INTERRUPT__: specifies the RCC interrupt sources to be disabled.
  *         This parameter can be any combination of the following values:
  *            @arg RCC_IT_LSIRDY:  LSI ready interrupt
  *            @arg RCC_IT_LSERDY:  LSE ready interrupt
  *            @arg RCC_IT_HSIRDY:  HSI ready interrupt
  *            @arg RCC_IT_HSERDY:  HSE ready interrupt
  *            @arg RCC_IT_CSIRDY:  CSI ready interrupt
  *            @arg RCC_IT_PLLRDY:  Main PLL ready interrupt
  *            @arg RCC_IT_PLL2RDY: PLL2RDY ready interrupt
  *            @arg RCC_IT_PLL3RDY: PLL3RDY ready interrupt
  *            @arg RCC_IT_PLL4RDY: PLL4RDY ready interrupt
  *            @arg RCC_IT_LSECSS:  LSE Clock security system interrupt
  *            @arg RCC_IT_WKUP:    Wake-up from CSTOP Interrupt
  *            @arg RCC_IT_ALL:     All RCC interrupts
  */
#ifdef CORE_CM4
#define __HAL_RCC_DISABLE_IT(__INTERRUPT__) CLEAR_BIT(RCC->MC_CIER, (__INTERRUPT__))
#endif /* CORE_CM4 */
#ifdef CORE_CA7
#define __HAL_RCC_DISABLE_IT(__INTERRUPT__) CLEAR_BIT(RCC->MP_CIER, (__INTERRUPT__))
#endif /* CORE_CA7 */


/** @brief  Clear the RCC's interrupt pending bits (Perform Byte access to RCC_CIR[23:16]
  *         bits to clear the selected interrupt pending bits.
  * @param  __INTERRUPT__: specifies the interrupt pending bit to clear.
  *         This parameter can be any combination of the following values:
  *            @arg RCC_IT_LSIRDY:  LSI ready interrupt
  *            @arg RCC_IT_LSERDY:  LSE ready interrupt
  *            @arg RCC_IT_HSIRDY:  HSI ready interrupt
  *            @arg RCC_IT_HSERDY:  HSE ready interrupt
  *            @arg RCC_IT_CSIRDY:  CSI ready interrupt
  *            @arg RCC_IT_PLLRDY:  Main PLL ready interrupt
  *            @arg RCC_IT_PLL2RDY: PLL2RDY ready interrupt
  *            @arg RCC_IT_PLL3RDY: PLL3RDY ready interrupt
  *            @arg RCC_IT_PLL4RDY: PLL4RDY ready interrupt
  *            @arg RCC_IT_LSECSS:  LSE Clock security system interrupt
  *            @arg RCC_IT_WKUP:    Wake-up from CSTOP Interrupt
  *            @arg RCC_IT_ALL:     All RCC interrupts
  */
#ifdef CORE_CM4
#define __HAL_RCC_CLEAR_IT(__INTERRUPT__) (RCC->MC_CIFR = (__INTERRUPT__))
#endif /* CORE_CM4 */
#ifdef CORE_CA7
#define __HAL_RCC_CLEAR_IT(__INTERRUPT__) (RCC->MP_CIFR = (__INTERRUPT__))
#endif /* CORE_CA7 */

/** @brief  Check the RCC's interrupt has occurred or not.
  * @param  __INTERRUPT__: specifies the RCC interrupt source to check.
  *         This parameter can be one of the following values:
  *            @arg RCC_IT_LSIRDY:  LSI ready interrupt
  *            @arg RCC_IT_LSERDY:  LSE ready interrupt
  *            @arg RCC_IT_HSIRDY:  HSI ready interrupt
  *            @arg RCC_IT_HSERDY:  HSE ready interrupt
  *            @arg RCC_IT_CSIRDY:  CSI ready interrupt
  *            @arg RCC_IT_PLLRDY:  Main PLL ready interrupt
  *            @arg RCC_IT_PLL2RDY: PLL2RDY ready interrupt
  *            @arg RCC_IT_PLL3RDY: PLL3RDY ready interrupt
  *            @arg RCC_IT_PLL4RDY: PLL4RDY ready interrupt
  *            @arg RCC_IT_LSECSS:  LSE Clock security system interrupt
  *            @arg RCC_IT_WKUP:    Wake-up from CSTOP Interrupt
  *            @arg RCC_IT_ALL:     All RCC interrupts
  * @retval The new state of __INTERRUPT__ (TRUE or FALSE).
  */
#ifdef CORE_CM4
#define __HAL_RCC_GET_IT(__INTERRUPT__) ((RCC->MC_CIFR & (__INTERRUPT__)) == (__INTERRUPT__))
#endif /* CORE_CM4 */
#ifdef CORE_CA7
#define __HAL_RCC_GET_IT(__INTERRUPT__) ((RCC->MP_CIFR & (__INTERRUPT__)) == (__INTERRUPT__))
#endif /* CORE_CA7 */
/**
  * @}
  */
/* Include RCC HAL Extension module */
#include "stm32mp1xx_hal_rcc_ex.h"

/* Exported functions --------------------------------------------------------*/
/** @addtogroup RCC_Exported_Functions RCC Exported Functions
 * @{
 */

/** @addtogroup RCC_Exported_Functions_Group1 Initialization and de-initialization functions
  * @{
  */
/* Initialization and de-initialization functions  ******************************/
HAL_StatusTypeDef HAL_RCC_DeInit(void);
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_HSEConfig(uint32_t State);
HAL_StatusTypeDef HAL_RCC_LSEConfig(uint32_t State);

HAL_StatusTypeDef RCC_PLL1_Config(RCC_PLLInitTypeDef *pll1);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct);
HAL_StatusTypeDef HAL_RCC_LSEDriveConfig(uint32_t LseDriveValue);
void     HAL_RCC_GetOscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
void     HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t *pFLatency);

void     HAL_RCC_WAKEUP_IRQHandler(void);
void     HAL_RCC_WAKEUP_Callback(void);
void     HAL_RCC_IRQHandler(void);
void     HAL_RCC_Callback(uint32_t Flags);
void     HAL_RCC_EnableHSECSS(void);


/**
  * @}
  */

/** @addtogroup RCC_Exported_Functions_Group2 Peripheral Control functions
  * @{
  */
/* Peripheral Control functions  ************************************************/
void     HAL_RCC_MCOConfig(uint32_t RCC_MCOx, uint32_t RCC_MCOSource, uint32_t RCC_MCODiv);
/* Get frequency functions  *****************************************************/
void     HAL_RCC_GetPLL1ClockFreq(PLL1_ClocksTypeDef *PLL1_Clocks);
void     HAL_RCC_GetPLL2ClockFreq(PLL2_ClocksTypeDef *PLL2_Clocks);
void     HAL_RCC_GetPLL3ClockFreq(PLL3_ClocksTypeDef *PLL3_Clocks);
void     HAL_RCC_GetPLL4ClockFreq(PLL4_ClocksTypeDef *PLL4_Clocks);

uint32_t HAL_RCC_GetMPUSSFreq(void);
uint32_t HAL_RCC_GetAXISSFreq(void);
uint32_t HAL_RCC_GetMCUSSFreq(void);
uint32_t HAL_RCC_GetACLKFreq(void);
uint32_t HAL_RCC_GetHCLK1Freq(void);
uint32_t HAL_RCC_GetHCLK2Freq(void);
uint32_t HAL_RCC_GetHCLK3Freq(void);
uint32_t HAL_RCC_GetHCLK4Freq(void);
uint32_t HAL_RCC_GetHCLK5Freq(void);
uint32_t HAL_RCC_GetHCLK6Freq(void);
uint32_t HAL_RCC_GetMCUFreq(void);
uint32_t HAL_RCC_GetFCLKFreq(void);
uint32_t HAL_RCC_GetMLHCLKFreq(void);
uint32_t HAL_RCC_GetPCLK1Freq(void);
uint32_t HAL_RCC_GetPCLK2Freq(void);
uint32_t HAL_RCC_GetPCLK3Freq(void);
uint32_t HAL_RCC_GetPCLK4Freq(void);
uint32_t HAL_RCC_GetPCLK5Freq(void);

uint32_t HAL_RCC_GetSystemCoreClockFreq(void);

uint32_t RCC_GetCKPERFreq(void);
/**
  * @}
  */

/**
  * @}
  */

/* Private types -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private constants ---------------------------------------------------------*/
/** @defgroup RCC_Private_Constants RCC Private Constants
  * @{
  */

#define HSE_TIMEOUT_VALUE          HSE_STARTUP_TIMEOUT
#define HSI_TIMEOUT_VALUE          (100U)  /* 100 ms */
#define CSI_TIMEOUT_VALUE          (100U)  /* 100 ms */
#define LSI_TIMEOUT_VALUE          (100U)  /* 100 ms */
#define PLL_TIMEOUT_VALUE          (100U)  /* 100 ms */
#define CLOCKSWITCH_TIMEOUT_VALUE  (1000U) /* 1 s    */

/**
  * @}
  */

/* Private macros ------------------------------------------------------------*/
/** @addtogroup RCC_Private_Macros RCC Private Macros
  * @{
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

#endif /* __STM32MP1xx_HAL_RCC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
