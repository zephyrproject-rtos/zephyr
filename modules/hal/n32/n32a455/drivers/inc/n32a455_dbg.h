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
 * @file n32a455_dbg.h
 * @author Nations 
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_DBG_H__
#define __N32A455_DBG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup DBG
 * @{
 */

/** @addtogroup DBGMCU_Exported_Types
 * @{
 */

/**
 * @}
 */

/** @addtogroup DBGMCU_Exported_Constants
 * @{
 */

#define DBG_SLEEP             ((uint32_t)0x00000001)
#define DBG_STOP              ((uint32_t)0x00000002)
#define DBG_STDBY             ((uint32_t)0x00000004)
#define DBG_IWDG_STOP         ((uint32_t)0x00000100)
#define DBG_WWDG_STOP         ((uint32_t)0x00000200)
#define DBG_TIM1_STOP         ((uint32_t)0x00000400)
#define DBG_TIM2_STOP         ((uint32_t)0x00000800)
#define DBG_TIM3_STOP         ((uint32_t)0x00001000)
#define DBG_TIM4_STOP         ((uint32_t)0x00002000)
#define DBG_CAN1_STOP         ((uint32_t)0x00004000)
#define DBG_I2C1SMBUS_TIMEOUT ((uint32_t)0x00008000)
#define DBG_I2C2SMBUS_TIMEOUT ((uint32_t)0x00010000)
#define DBG_TIM8_STOP         ((uint32_t)0x00020000)
#define DBG_TIM5_STOP         ((uint32_t)0x00040000)
#define DBG_TIM6_STOP         ((uint32_t)0x00080000)
#define DBG_TIM7_STOP         ((uint32_t)0x00100000)
#define DBG_CAN2_STOP         ((uint32_t)0x00200000)

#define IS_DBGMCU_PERIPH(PERIPH) ((((PERIPH)&0xFFC000F8) == 0x00) && ((PERIPH) != 0x00))
/**
 * @}
 */

/** @addtogroup DBGMCU_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup DBGMCU_Exported_Functions
 * @{
 */

void GetUCID(uint8_t *UCIDbuf);
void GetUID(uint8_t *UIDbuf);
void GetDBGMCU_ID(uint8_t *DBGMCU_IDbuf);
uint32_t DBG_GetRevNum(void);
uint32_t DBG_GetDevNum(void);
void DBG_ConfigPeriph(uint32_t DBG_Periph, FunctionalState Cmd);

uint32_t DBG_GetFlashSize(void);
uint32_t DBG_GetSramSize(void);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_DBG_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
