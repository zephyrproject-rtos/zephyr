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
 * @file n32a455_bkp.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_BKP_H__
#define __N32A455_BKP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup BKP
 * @{
 */

/** @addtogroup BKP_Exported_Types
 * @{
 */

/**
 * @}
 */

/** @addtogroup BKP_Exported_Constants
 * @{
 */

/** @addtogroup Tamper_Pin_active_level
 * @{
 */

#define BKP_TP_HIGH            ((uint16_t)0x0000)
#define BKP_TP_LOW             ((uint16_t)0x0001)
#define IS_BKP_TP_LEVEL(LEVEL) (((LEVEL) == BKP_TP_HIGH) || ((LEVEL) == BKP_TP_LOW))
/**
 * @}
 */

/** @addtogroup Data_Backup_Register
 * @{
 */

#define BKP_DAT1  ((uint16_t)0x0004)
#define BKP_DAT2  ((uint16_t)0x0008)
#define BKP_DAT3  ((uint16_t)0x000C)
#define BKP_DAT4  ((uint16_t)0x0010)
#define BKP_DAT5  ((uint16_t)0x0014)
#define BKP_DAT6  ((uint16_t)0x0018)
#define BKP_DAT7  ((uint16_t)0x001C)
#define BKP_DAT8  ((uint16_t)0x0020)
#define BKP_DAT9  ((uint16_t)0x0024)
#define BKP_DAT10 ((uint16_t)0x0028)
#define BKP_DAT11 ((uint16_t)0x0040)
#define BKP_DAT12 ((uint16_t)0x0044)
#define BKP_DAT13 ((uint16_t)0x0048)
#define BKP_DAT14 ((uint16_t)0x004C)
#define BKP_DAT15 ((uint16_t)0x0050)
#define BKP_DAT16 ((uint16_t)0x0054)
#define BKP_DAT17 ((uint16_t)0x0058)
#define BKP_DAT18 ((uint16_t)0x005C)
#define BKP_DAT19 ((uint16_t)0x0060)
#define BKP_DAT20 ((uint16_t)0x0064)
#define BKP_DAT21 ((uint16_t)0x0068)
#define BKP_DAT22 ((uint16_t)0x006C)
#define BKP_DAT23 ((uint16_t)0x0070)
#define BKP_DAT24 ((uint16_t)0x0074)
#define BKP_DAT25 ((uint16_t)0x0078)
#define BKP_DAT26 ((uint16_t)0x007C)
#define BKP_DAT27 ((uint16_t)0x0080)
#define BKP_DAT28 ((uint16_t)0x0084)
#define BKP_DAT29 ((uint16_t)0x0088)
#define BKP_DAT30 ((uint16_t)0x008C)
#define BKP_DAT31 ((uint16_t)0x0090)
#define BKP_DAT32 ((uint16_t)0x0094)
#define BKP_DAT33 ((uint16_t)0x0098)
#define BKP_DAT34 ((uint16_t)0x009C)
#define BKP_DAT35 ((uint16_t)0x00A0)
#define BKP_DAT36 ((uint16_t)0x00A4)
#define BKP_DAT37 ((uint16_t)0x00A8)
#define BKP_DAT38 ((uint16_t)0x00AC)
#define BKP_DAT39 ((uint16_t)0x00B0)
#define BKP_DAT40 ((uint16_t)0x00B4)
#define BKP_DAT41 ((uint16_t)0x00B8)
#define BKP_DAT42 ((uint16_t)0x00BC)

#define IS_BKP_DAT(DAT)                                                                                                \
    (((DAT) == BKP_DAT1) || ((DAT) == BKP_DAT2) || ((DAT) == BKP_DAT3) || ((DAT) == BKP_DAT4) || ((DAT) == BKP_DAT5)   \
     || ((DAT) == BKP_DAT6) || ((DAT) == BKP_DAT7) || ((DAT) == BKP_DAT8) || ((DAT) == BKP_DAT9)                       \
     || ((DAT) == BKP_DAT10) || ((DAT) == BKP_DAT11) || ((DAT) == BKP_DAT12) || ((DAT) == BKP_DAT13)                   \
     || ((DAT) == BKP_DAT14) || ((DAT) == BKP_DAT15) || ((DAT) == BKP_DAT16) || ((DAT) == BKP_DAT17)                   \
     || ((DAT) == BKP_DAT18) || ((DAT) == BKP_DAT19) || ((DAT) == BKP_DAT20) || ((DAT) == BKP_DAT21)                   \
     || ((DAT) == BKP_DAT22) || ((DAT) == BKP_DAT23) || ((DAT) == BKP_DAT24) || ((DAT) == BKP_DAT25)                   \
     || ((DAT) == BKP_DAT26) || ((DAT) == BKP_DAT27) || ((DAT) == BKP_DAT28) || ((DAT) == BKP_DAT29)                   \
     || ((DAT) == BKP_DAT30) || ((DAT) == BKP_DAT31) || ((DAT) == BKP_DAT32) || ((DAT) == BKP_DAT33)                   \
     || ((DAT) == BKP_DAT34) || ((DAT) == BKP_DAT35) || ((DAT) == BKP_DAT36) || ((DAT) == BKP_DAT37)                   \
     || ((DAT) == BKP_DAT38) || ((DAT) == BKP_DAT39) || ((DAT) == BKP_DAT40) || ((DAT) == BKP_DAT41)                   \
     || ((DAT) == BKP_DAT42))


/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup BKP_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup BKP_Exported_Functions
 * @{
 */

void BKP_DeInit(void);
void BKP_ConfigTPLevel(uint16_t BKP_TamperPinLevel);
void BKP_TPEnable(FunctionalState Cmd);
void BKP_TPIntEnable(FunctionalState Cmd);
void BKP_WriteBkpData(uint16_t BKP_DAT, uint16_t Data);
uint16_t BKP_ReadBkpData(uint16_t BKP_DAT);
FlagStatus BKP_GetTEFlag(void);
void BKP_ClrTEFlag(void);
INTStatus BKP_GetTINTFlag(void);
void BKP_ClrTINTFlag(void);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_BKP_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
