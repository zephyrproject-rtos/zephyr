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
 * @file n32a455_iwdg.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_IWDG_H__
#define __N32A455_IWDG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup IWDG
 * @{
 */

/** @addtogroup IWDG_Exported_Types
 * @{
 */

/**
 * @}
 */

/** @addtogroup IWDG_Exported_Constants
 * @{
 */

/** @addtogroup IWDG_WriteAccess
 * @{
 */

#define IWDG_WRITE_ENABLE     ((uint16_t)0x5555)
#define IWDG_WRITE_DISABLE    ((uint16_t)0x0000)
#define IS_IWDG_WRITE(ACCESS) (((ACCESS) == IWDG_WRITE_ENABLE) || ((ACCESS) == IWDG_WRITE_DISABLE))
/**
 * @}
 */

/** @addtogroup IWDG_prescaler
 * @{
 */

#define IWDG_PRESCALER_DIV4   ((uint8_t)0x00)
#define IWDG_PRESCALER_DIV8   ((uint8_t)0x01)
#define IWDG_PRESCALER_DIV16  ((uint8_t)0x02)
#define IWDG_PRESCALER_DIV32  ((uint8_t)0x03)
#define IWDG_PRESCALER_DIV64  ((uint8_t)0x04)
#define IWDG_PRESCALER_DIV128 ((uint8_t)0x05)
#define IWDG_PRESCALER_DIV256 ((uint8_t)0x06)
#define IS_IWDG_PRESCALER_DIV(PRESCALER)                                                                               \
    (((PRESCALER) == IWDG_PRESCALER_DIV4) || ((PRESCALER) == IWDG_PRESCALER_DIV8)                                      \
     || ((PRESCALER) == IWDG_PRESCALER_DIV16) || ((PRESCALER) == IWDG_PRESCALER_DIV32)                                 \
     || ((PRESCALER) == IWDG_PRESCALER_DIV64) || ((PRESCALER) == IWDG_PRESCALER_DIV128)                                \
     || ((PRESCALER) == IWDG_PRESCALER_DIV256))
/**
 * @}
 */

/** @addtogroup IWDG_Flag
 * @{
 */

#define IWDG_PVU_FLAG          ((uint16_t)0x0001)
#define IWDG_CRVU_FLAG         ((uint16_t)0x0002)
#define IS_IWDG_FLAG(FLAG)     (((FLAG) == IWDG_PVU_FLAG) || ((FLAG) == IWDG_CRVU_FLAG))
#define IS_IWDG_RELOAD(RELOAD) ((RELOAD) <= 0xFFF)
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup IWDG_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup IWDG_Exported_Functions
 * @{
 */

void IWDG_WriteConfig(uint16_t IWDG_WriteAccess);
void IWDG_SetPrescalerDiv(uint8_t IWDG_Prescaler);
void IWDG_CntReload(uint16_t Reload);
void IWDG_ReloadKey(void);
void IWDG_Enable(void);
FlagStatus IWDG_GetStatus(uint16_t IWDG_FLAG);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_IWDG_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
