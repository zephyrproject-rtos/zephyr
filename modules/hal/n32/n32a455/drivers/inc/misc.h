/*****************************************************************************
 * Copyright (c) 2022, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file misc.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __MISC_H__
#define __MISC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup MISC
 * @{
 */

/** @addtogroup MISC_Exported_Types
 * @{
 */

/**
 * @brief  NVIC Init Structure definition
 */

typedef struct
{
    uint8_t NVIC_IRQChannel; /*!< Specifies the IRQ channel to be enabled or disabled.
                                  This parameter can be a value of @ref IRQn_Type
                                  (For the complete N32A455 Devices IRQ Channels list, please
                                   refer to n32a455.h file) */

    uint8_t
        NVIC_IRQChannelPreemptionPriority; /*!< Specifies the pre-emption priority for the IRQ channel
                                                specified in NVIC_IRQChannel. This parameter can be a value
                                                between 0 and 15 as described in the table @ref NVIC_Priority_Table */

    uint8_t NVIC_IRQChannelSubPriority; /*!< Specifies the subpriority level for the IRQ channel specified
                                             in NVIC_IRQChannel. This parameter can be a value
                                             between 0 and 15 as described in the table @ref NVIC_Priority_Table */

    FunctionalState NVIC_IRQChannelCmd; /*!< Specifies whether the IRQ channel defined in NVIC_IRQChannel
                                             will be enabled or disabled.
                                             This parameter can be set either to ENABLE or DISABLE */
} NVIC_InitType;

/**
 * @}
 */

/** @addtogroup NVIC_Priority_Table
 * @{
 */

/**
@code
 The table below gives the allowed values of the pre-emption priority and subpriority according
 to the Priority Grouping configuration performed by NVIC_PriorityGroupConfig function
  ============================================================================================================================
    NVIC_PriorityGroup   | NVIC_IRQChannelPreemptionPriority | NVIC_IRQChannelSubPriority  | Description
  ============================================================================================================================
   NVIC_PriorityGroup_0  |                0                  |            0-15             |   0 bits for pre-emption
priority |                                   |                             |   4 bits for subpriority
  ----------------------------------------------------------------------------------------------------------------------------
   NVIC_PriorityGroup_1  |                0-1                |            0-7              |   1 bits for pre-emption
priority |                                   |                             |   3 bits for subpriority
  ----------------------------------------------------------------------------------------------------------------------------
   NVIC_PriorityGroup_2  |                0-3                |            0-3              |   2 bits for pre-emption
priority |                                   |                             |   2 bits for subpriority
  ----------------------------------------------------------------------------------------------------------------------------
   NVIC_PriorityGroup_3  |                0-7                |            0-1              |   3 bits for pre-emption
priority |                                   |                             |   1 bits for subpriority
  ----------------------------------------------------------------------------------------------------------------------------
   NVIC_PriorityGroup_4  |                0-15               |            0                |   4 bits for pre-emption
priority |                                   |                             |   0 bits for subpriority
  ============================================================================================================================
@endcode
*/

/**
 * @}
 */

/** @addtogroup MISC_Exported_Constants
 * @{
 */

/** @addtogroup Vector_Table_Base
 * @{
 */

#define NVIC_VectTab_RAM         ((uint32_t)0x20000000)
#define NVIC_VectTab_FLASH       ((uint32_t)0x08000000)
#define IS_NVIC_VECTTAB(VECTTAB) (((VECTTAB) == NVIC_VectTab_RAM) || ((VECTTAB) == NVIC_VectTab_FLASH))
/**
 * @}
 */

/** @addtogroup System_Low_Power
 * @{
 */

#define NVIC_LP_SEVONPEND   ((uint8_t)0x10)
#define NVIC_LP_SLEEPDEEP   ((uint8_t)0x04)
#define NVIC_LP_SLEEPONEXIT ((uint8_t)0x02)
#define IS_NVIC_LP(LP)      (((LP) == NVIC_LP_SEVONPEND) || ((LP) == NVIC_LP_SLEEPDEEP) || ((LP) == NVIC_LP_SLEEPONEXIT))
/**
 * @}
 */

/** @addtogroup Preemption_Priority_Group
 * @{
 */

#define NVIC_PriorityGroup_0                                                                                           \
    ((uint32_t)0x700) /*!< 0 bits for pre-emption priority                                                             \
                           4 bits for subpriority */
#define NVIC_PriorityGroup_1                                                                                           \
    ((uint32_t)0x600) /*!< 1 bits for pre-emption priority                                                             \
                           3 bits for subpriority */
#define NVIC_PriorityGroup_2                                                                                           \
    ((uint32_t)0x500) /*!< 2 bits for pre-emption priority                                                             \
                           2 bits for subpriority */
#define NVIC_PriorityGroup_3                                                                                           \
    ((uint32_t)0x400) /*!< 3 bits for pre-emption priority                                                             \
                           1 bits for subpriority */
#define NVIC_PriorityGroup_4                                                                                           \
    ((uint32_t)0x300) /*!< 4 bits for pre-emption priority                                                             \
                           0 bits for subpriority */

#define IS_NVIC_PRIORITY_GROUP(GROUP)                                                                                  \
    (((GROUP) == NVIC_PriorityGroup_0) || ((GROUP) == NVIC_PriorityGroup_1) || ((GROUP) == NVIC_PriorityGroup_2)       \
     || ((GROUP) == NVIC_PriorityGroup_3) || ((GROUP) == NVIC_PriorityGroup_4))

#define IS_NVIC_PREEMPTION_PRIORITY(PRIORITY) ((PRIORITY) < 0x10)

#define IS_NVIC_SUB_PRIORITY(PRIORITY) ((PRIORITY) < 0x10)

#define IS_NVIC_OFFSET(OFFSET) ((OFFSET) < 0x000FFFFF)

/**
 * @}
 */

/** @addtogroup SysTick_clock_source
 * @{
 */


#define SysTick_CLKSource_HCLK      ((uint32_t)0x00000004)
#define IS_SYSTICK_CLK_SOURCE(SOURCE)    ((SOURCE) == SysTick_CLKSource_HCLK)
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup MISC_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup MISC_Exported_Functions
 * @{
 */

void NVIC_PriorityGroupConfig(uint32_t NVIC_PriorityGroup);
void NVIC_Init(NVIC_InitType* NVIC_InitStruct);
void NVIC_SetVectorTable(uint32_t NVIC_VectTab, uint32_t Offset);
void NVIC_SystemLPConfig(uint8_t LowPowerMode, FunctionalState Cmd);
void SysTick_CLKSourceConfig(uint32_t SysTick_CLKSource);

#ifdef __cplusplus
}
#endif

#endif /* __MISC_H__ */

/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
