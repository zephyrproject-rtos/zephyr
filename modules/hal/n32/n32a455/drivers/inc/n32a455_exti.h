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
 * @file n32a455_exti.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_EXTI_H__
#define __N32A455_EXTI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup EXTI
 * @{
 */

/** @addtogroup EXTI_Exported_Types
 * @{
 */

/**
 * @brief  EXTI mode enumeration
 */

typedef enum
{
    EXTI_Mode_Interrupt = 0x00,
    EXTI_Mode_Event     = 0x04
} EXTI_ModeType;

#define IS_EXTI_MODE(MODE) (((MODE) == EXTI_Mode_Interrupt) || ((MODE) == EXTI_Mode_Event))

/**
 * @brief  EXTI Trigger enumeration
 */

typedef enum
{
    EXTI_Trigger_Rising         = 0x08,
    EXTI_Trigger_Falling        = 0x0C,
    EXTI_Trigger_Rising_Falling = 0x10
} EXTI_TriggerType;

#define IS_EXTI_TRIGGER(TRIGGER)                                                                                       \
    (((TRIGGER) == EXTI_Trigger_Rising) || ((TRIGGER) == EXTI_Trigger_Falling)                                         \
     || ((TRIGGER) == EXTI_Trigger_Rising_Falling))
/**
 * @brief  EXTI Init Structure definition
 */

typedef struct
{
    uint32_t EXTI_Line; /*!< Specifies the EXTI lines to be enabled or disabled.
                             This parameter can be any combination of @ref EXTI_Lines */

    EXTI_ModeType EXTI_Mode; /*!< Specifies the mode for the EXTI lines.
                                     This parameter can be a value of @ref EXTI_ModeType */

    EXTI_TriggerType EXTI_Trigger; /*!< Specifies the trigger signal active edge for the EXTI lines.
                                           This parameter can be a value of @ref EXTI_ModeType */

    FunctionalState EXTI_LineCmd; /*!< Specifies the new state of the selected EXTI lines.
                                       This parameter can be set either to ENABLE or DISABLE */
} EXTI_InitType;

/**
 * @}
 */

/** @addtogroup EXTI_Exported_Constants
 * @{
 */

/** @addtogroup EXTI_Lines
 * @{
 */

#define EXTI_LINE0  ((uint32_t)0x00001) /*!< External interrupt line 0 */
#define EXTI_LINE1  ((uint32_t)0x00002) /*!< External interrupt line 1 */
#define EXTI_LINE2  ((uint32_t)0x00004) /*!< External interrupt line 2 */
#define EXTI_LINE3  ((uint32_t)0x00008) /*!< External interrupt line 3 */
#define EXTI_LINE4  ((uint32_t)0x00010) /*!< External interrupt line 4 */
#define EXTI_LINE5  ((uint32_t)0x00020) /*!< External interrupt line 5 */
#define EXTI_LINE6  ((uint32_t)0x00040) /*!< External interrupt line 6 */
#define EXTI_LINE7  ((uint32_t)0x00080) /*!< External interrupt line 7 */
#define EXTI_LINE8  ((uint32_t)0x00100) /*!< External interrupt line 8 */
#define EXTI_LINE9  ((uint32_t)0x00200) /*!< External interrupt line 9 */
#define EXTI_LINE10 ((uint32_t)0x00400) /*!< External interrupt line 10 */
#define EXTI_LINE11 ((uint32_t)0x00800) /*!< External interrupt line 11 */
#define EXTI_LINE12 ((uint32_t)0x01000) /*!< External interrupt line 12 */
#define EXTI_LINE13 ((uint32_t)0x02000) /*!< External interrupt line 13 */
#define EXTI_LINE14 ((uint32_t)0x04000) /*!< External interrupt line 14 */
#define EXTI_LINE15 ((uint32_t)0x08000) /*!< External interrupt line 15 */
#define EXTI_LINE16 ((uint32_t)0x10000) /*!< External interrupt line 16 Connected to the PVD Output */
#define EXTI_LINE17 ((uint32_t)0x20000) /*!< External interrupt line 17 Connected to the RTC Alarm event */
#define EXTI_LINE20 ((uint32_t)0x100000) /*!< External interrupt line 20 Connected to the RTC Wakeup event */

#define IS_EXTI_LINE(LINE) ((((LINE) & (uint32_t)0xFFC00000) == 0x00) && ((LINE) != (uint16_t)0x00))
#define IS_GET_EXTI_LINE(LINE)                                                                                         \
    (((LINE) == EXTI_LINE0) || ((LINE) == EXTI_LINE1) || ((LINE) == EXTI_LINE2) || ((LINE) == EXTI_LINE3)              \
     || ((LINE) == EXTI_LINE4) || ((LINE) == EXTI_LINE5) || ((LINE) == EXTI_LINE6) || ((LINE) == EXTI_LINE7)           \
     || ((LINE) == EXTI_LINE8) || ((LINE) == EXTI_LINE9) || ((LINE) == EXTI_LINE10) || ((LINE) == EXTI_LINE11)         \
     || ((LINE) == EXTI_LINE12) || ((LINE) == EXTI_LINE13) || ((LINE) == EXTI_LINE14) || ((LINE) == EXTI_LINE15)       \
     || ((LINE) == EXTI_LINE16) || ((LINE) == EXTI_LINE17) || ((LINE) == EXTI_LINE20))
   

/**
 * @}
 */

/** @addtogroup EXTI_TSSEL_Line
 * @{
 */


#define IS_EXTI_TSSEL_LINE(LINE)                                                                                       \
    (((LINE) == EXTI_TSSEL_LINE0) || ((LINE) == EXTI_TSSEL_LINE1) || ((LINE) == EXTI_TSSEL_LINE2)                      \
     || ((LINE) == EXTI_TSSEL_LINE3) || ((LINE) == EXTI_TSSEL_LINE4) || ((LINE) == EXTI_TSSEL_LINE5)                   \
     || ((LINE) == EXTI_TSSEL_LINE6) || ((LINE) == EXTI_TSSEL_LINE7) || ((LINE) == EXTI_TSSEL_LINE8)                   \
     || ((LINE) == EXTI_TSSEL_LINE9) || ((LINE) == EXTI_TSSEL_LINE10) || ((LINE) == EXTI_TSSEL_LINE11)                 \
     || ((LINE) == EXTI_TSSEL_LINE12) || ((LINE) == EXTI_TSSEL_LINE13) || ((LINE) == EXTI_TSSEL_LINE14)                \
     || ((LINE) == EXTI_TSSEL_LINE15))
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup EXTI_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup EXTI_Exported_Functions
 * @{
 */

void EXTI_DeInit(void);
void EXTI_InitPeripheral(EXTI_InitType* EXTI_InitStruct);
void EXTI_InitStruct(EXTI_InitType* EXTI_InitStruct);
void EXTI_TriggerSWInt(uint32_t EXTI_Line);
FlagStatus EXTI_GetStatusFlag(uint32_t EXTI_Line);
void EXTI_ClrStatusFlag(uint32_t EXTI_Line);
INTStatus EXTI_GetITStatus(uint32_t EXTI_Line);
void EXTI_ClrITPendBit(uint32_t EXTI_Line);
void EXTI_RTCTimeStampSel(uint32_t EXTI_TSSEL_Line);

#ifdef __cplusplus
}
#endif

#endif /* __N32A455_EXTI_H__ */
/**
 * @}
 */

/**
 * @}
 */

/**
 * @}
 */
