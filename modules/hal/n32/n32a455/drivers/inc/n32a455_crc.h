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
 * @file n32a455_crc.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_CRC_H__
#define __N32A455_CRC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup CRC
 * @{
 */

/** @addtogroup CRC_Exported_Types
 * @{
 */

/**
 * @}
 */

/** @addtogroup CRC_Exported_Constants
 * @{
 */

/**
 * @}
 */

/** @addtogroup CRC_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup CRC_Exported_Functions
 * @{
 */

void CRC32_ResetCrc(void);
uint32_t CRC32_CalcCrc(uint32_t Data);
uint32_t CRC32_CalcBufCrc(uint32_t pBuffer[], uint32_t BufferLength);
uint32_t CRC32_GetCrc(void);
void CRC32_SetIDat(uint8_t IDValue);
uint8_t CRC32_GetIDat(void);

uint16_t CRC16_CalcBufCrc(uint8_t pBuffer[], uint32_t BufferLength);
uint16_t CRC16_CalcCRC(uint8_t Data);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_CRC_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
