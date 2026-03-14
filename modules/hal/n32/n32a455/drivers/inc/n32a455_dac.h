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
 * @file n32a455_dac.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_DAC_H__
#define __N32A455_DAC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup DAC
 * @{
 */

/** @addtogroup DAC_Exported_Types
 * @{
 */

/**
 * @brief  DAC Init structure definition
 */

typedef struct
{
    uint32_t Trigger; /*!< Specifies the external trigger for the selected DAC channel.
                               This parameter can be a value of @ref DAC_trigger_selection */

    uint32_t WaveGen; /*!< Specifies whether DAC channel noise waves or triangle waves
                                      are generated, or whether no wave is generated.
                                      This parameter can be a value of @ref DAC_wave_generation */

    uint32_t
        LfsrUnMaskTriAmp; /*!< Specifies the LFSR mask for noise wave generation or
                                               the maximum amplitude triangle generation for the DAC channel.
                                               This parameter can be a value of @ref DAC_lfsrunmask_triangleamplitude */

    uint32_t BufferOutput; /*!< Specifies whether the DAC channel output buffer is enabled or disabled.
                                    This parameter can be a value of @ref DAC_output_buffer */
} DAC_InitType;

/**
 * @}
 */

/** @addtogroup DAC_Exported_Constants
 * @{
 */

/** @addtogroup DAC_trigger_selection
 * @{
 */

#define DAC_TRG_NONE                                                                                                   \
    ((uint32_t)0x00000000) /*!< Conversion is automatic once the DAC1_DHRxxxx register                                 \
                                    has been loaded, and not by external trigger */
#define DAC_TRG_T6_TRGO                                                                                                \
    ((uint32_t)0x00000004) /*!< TIM6 TRGO selected as external conversion trigger for DAC channel                      \
                            */
#define DAC_TRG_T8_TRGO                                                                                                \
    ((uint32_t)0x0000000C) /*!< TIM8 TRGO selected as external conversion trigger for DAC channel                      \
                                    only in High-density devices*/
#define DAC_TRG_T3_TRGO                                                                                                \
    ((uint32_t)0x0000000C) /*!< TIM8 TRGO selected as external conversion trigger for DAC channel                      \
                                    only in Connectivity line, Medium-density and Low-density Value Line devices */
#define DAC_TRG_T7_TRGO                                                                                                \
    ((uint32_t)0x00000014) /*!< TIM7 TRGO selected as external conversion trigger for DAC channel                      \
                            */
#define DAC_TRG_T5_TRGO                                                                                                \
    ((uint32_t)0x0000001C) /*!< TIM5 TRGO selected as external conversion trigger for DAC channel                      \
                            */
#define DAC_TRG_T15_TRGO                                                                                               \
    ((uint32_t)0x0000001C) /*!< TIM15 TRGO selected as external conversion trigger for DAC channel                     \
                                    only in Medium-density and Low-density Value Line devices*/
#define DAC_TRG_T2_TRGO                                                                                                \
    ((uint32_t)0x00000024) /*!< TIM2 TRGO selected as external conversion trigger for DAC channel                      \
                            */
#define DAC_TRG_T4_TRGO                                                                                                \
    ((uint32_t)0x0000002C) /*!< TIM4 TRGO selected as external conversion trigger for DAC channel                      \
                            */
#define DAC_TRG_EXT_IT9                                                                                                \
    ((uint32_t)0x00000034) /*!< EXTI Line9 event selected as external conversion trigger for DAC channel */
#define DAC_TRG_SOFTWARE ((uint32_t)0x0000003C) /*!< Conversion started by software trigger for DAC channel */

#define IS_DAC_TRIGGER(TRIGGER)                                                                                        \
    (((TRIGGER) == DAC_TRG_NONE) || ((TRIGGER) == DAC_TRG_T6_TRGO) || ((TRIGGER) == DAC_TRG_T8_TRGO)                   \
     || ((TRIGGER) == DAC_TRG_T7_TRGO) || ((TRIGGER) == DAC_TRG_T5_TRGO) || ((TRIGGER) == DAC_TRG_T2_TRGO)             \
     || ((TRIGGER) == DAC_TRG_T4_TRGO) || ((TRIGGER) == DAC_TRG_EXT_IT9) || ((TRIGGER) == DAC_TRG_SOFTWARE))

/**
 * @}
 */

/** @addtogroup DAC_wave_generation
 * @{
 */

#define DAC_WAVEGEN_NONE     ((uint32_t)0x00000000)
#define DAC_WAVEGEN_NOISE    ((uint32_t)0x00000040)
#define DAC_WAVEGEN_TRIANGLE ((uint32_t)0x00000080)
#define IS_DAC_GENERATE_WAVE(WAVE)                                                                                     \
    (((WAVE) == DAC_WAVEGEN_NONE) || ((WAVE) == DAC_WAVEGEN_NOISE) || ((WAVE) == DAC_WAVEGEN_TRIANGLE))
/**
 * @}
 */

/** @addtogroup DAC_lfsrunmask_triangleamplitude
 * @{
 */

#define DAC_UNMASK_LFSRBIT0 ((uint32_t)0x00000000) /*!< Unmask DAC channel LFSR bit0 for noise wave generation */
#define DAC_UNMASK_LFSRBITS1_0                                                                                         \
    ((uint32_t)0x00000100) /*!< Unmask DAC channel LFSR bit[1:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS2_0                                                                                         \
    ((uint32_t)0x00000200) /*!< Unmask DAC channel LFSR bit[2:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS3_0                                                                                         \
    ((uint32_t)0x00000300) /*!< Unmask DAC channel LFSR bit[3:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS4_0                                                                                         \
    ((uint32_t)0x00000400) /*!< Unmask DAC channel LFSR bit[4:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS5_0                                                                                         \
    ((uint32_t)0x00000500) /*!< Unmask DAC channel LFSR bit[5:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS6_0                                                                                         \
    ((uint32_t)0x00000600) /*!< Unmask DAC channel LFSR bit[6:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS7_0                                                                                         \
    ((uint32_t)0x00000700) /*!< Unmask DAC channel LFSR bit[7:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS8_0                                                                                         \
    ((uint32_t)0x00000800) /*!< Unmask DAC channel LFSR bit[8:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS9_0                                                                                         \
    ((uint32_t)0x00000900) /*!< Unmask DAC channel LFSR bit[9:0] for noise wave generation                             \
                            */
#define DAC_UNMASK_LFSRBITS10_0                                                                                        \
    ((uint32_t)0x00000A00) /*!< Unmask DAC channel LFSR bit[10:0] for noise wave generation */
#define DAC_UNMASK_LFSRBITS11_0                                                                                        \
    ((uint32_t)0x00000B00)                     /*!< Unmask DAC channel LFSR bit[11:0] for noise wave generation */
#define DAC_TRIAMP_1    ((uint32_t)0x00000000) /*!< Select max triangle amplitude of 1 */
#define DAC_TRIAMP_3    ((uint32_t)0x00000100) /*!< Select max triangle amplitude of 3 */
#define DAC_TRIAMP_7    ((uint32_t)0x00000200) /*!< Select max triangle amplitude of 7 */
#define DAC_TRIAMP_15   ((uint32_t)0x00000300) /*!< Select max triangle amplitude of 15 */
#define DAC_TRIAMP_31   ((uint32_t)0x00000400) /*!< Select max triangle amplitude of 31 */
#define DAC_TRIAMP_63   ((uint32_t)0x00000500) /*!< Select max triangle amplitude of 63 */
#define DAC_TRIAMP_127  ((uint32_t)0x00000600) /*!< Select max triangle amplitude of 127 */
#define DAC_TRIAMP_255  ((uint32_t)0x00000700) /*!< Select max triangle amplitude of 255 */
#define DAC_TRIAMP_511  ((uint32_t)0x00000800) /*!< Select max triangle amplitude of 511 */
#define DAC_TRIAMP_1023 ((uint32_t)0x00000900) /*!< Select max triangle amplitude of 1023 */
#define DAC_TRIAMP_2047 ((uint32_t)0x00000A00) /*!< Select max triangle amplitude of 2047 */
#define DAC_TRIAMP_4095 ((uint32_t)0x00000B00) /*!< Select max triangle amplitude of 4095 */

#define IS_DAC_LFSR_UNMASK_TRIANGLE_AMPLITUDE(VALUE)                                                                   \
    (((VALUE) == DAC_UNMASK_LFSRBIT0) || ((VALUE) == DAC_UNMASK_LFSRBITS1_0) || ((VALUE) == DAC_UNMASK_LFSRBITS2_0)    \
     || ((VALUE) == DAC_UNMASK_LFSRBITS3_0) || ((VALUE) == DAC_UNMASK_LFSRBITS4_0)                                     \
     || ((VALUE) == DAC_UNMASK_LFSRBITS5_0) || ((VALUE) == DAC_UNMASK_LFSRBITS6_0)                                     \
     || ((VALUE) == DAC_UNMASK_LFSRBITS7_0) || ((VALUE) == DAC_UNMASK_LFSRBITS8_0)                                     \
     || ((VALUE) == DAC_UNMASK_LFSRBITS9_0) || ((VALUE) == DAC_UNMASK_LFSRBITS10_0)                                    \
     || ((VALUE) == DAC_UNMASK_LFSRBITS11_0) || ((VALUE) == DAC_TRIAMP_1) || ((VALUE) == DAC_TRIAMP_3)                 \
     || ((VALUE) == DAC_TRIAMP_7) || ((VALUE) == DAC_TRIAMP_15) || ((VALUE) == DAC_TRIAMP_31)                          \
     || ((VALUE) == DAC_TRIAMP_63) || ((VALUE) == DAC_TRIAMP_127) || ((VALUE) == DAC_TRIAMP_255)                       \
     || ((VALUE) == DAC_TRIAMP_511) || ((VALUE) == DAC_TRIAMP_1023) || ((VALUE) == DAC_TRIAMP_2047)                    \
     || ((VALUE) == DAC_TRIAMP_4095))
/**
 * @}
 */

/** @addtogroup DAC_output_buffer
 * @{
 */

#define DAC_BUFFOUTPUT_ENABLE             ((uint32_t)0x00000002)
#define DAC_BUFFOUTPUT_DISABLE            ((uint32_t)0x00000000)
#define IS_DAC_OUTPUT_BUFFER_STATE(STATE) (((STATE) == DAC_BUFFOUTPUT_ENABLE) || ((STATE) == DAC_BUFFOUTPUT_DISABLE))
/**
 * @}
 */

/** @addtogroup DAC_Channel_selection
 * @{
 */

#define DAC_CHANNEL_1           ((uint32_t)0x00000000)
#define DAC_CHANNEL_2           ((uint32_t)0x00000010)
#define IS_DAC_CHANNEL(CHANNEL) (((CHANNEL) == DAC_CHANNEL_1) || ((CHANNEL) == DAC_CHANNEL_2))
/**
 * @}
 */

/** @addtogroup DAC_data_alignment
 * @{
 */

#define DAC_ALIGN_R_12BIT ((uint32_t)0x00000000)
#define DAC_ALIGN_L_12BIT ((uint32_t)0x00000004)
#define DAC_ALIGN_R_8BIT  ((uint32_t)0x00000008)
#define IS_DAC_ALIGN(ALIGN)                                                                                            \
    (((ALIGN) == DAC_ALIGN_R_12BIT) || ((ALIGN) == DAC_ALIGN_L_12BIT) || ((ALIGN) == DAC_ALIGN_R_8BIT))
/**
 * @}
 */

/** @addtogroup DAC_wave_generation
 * @{
 */

#define DAC_WAVE_NOISE    ((uint32_t)0x00000040)
#define DAC_WAVE_TRIANGLE ((uint32_t)0x00000080)
#define IS_DAC_WAVE(WAVE) (((WAVE) == DAC_WAVE_NOISE) || ((WAVE) == DAC_WAVE_TRIANGLE))
/**
 * @}
 */

/** @addtogroup DAC_data
 * @{
 */

#define IS_DAC_DATA(DATA) ((DATA) <= 0xFFF0)
/**
 * @}
 */

/**
 * @}
 */

/** @addtogroup DAC_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @addtogroup DAC_Exported_Functions
 * @{
 */

void DAC_DeInit(void);
void DAC_Init(uint32_t DAC_Channel, DAC_InitType* DAC_InitStruct);
void DAC_ClearStruct(DAC_InitType* DAC_InitStruct);
void DAC_Enable(uint32_t DAC_Channel, FunctionalState Cmd);

void DAC_DmaEnable(uint32_t DAC_Channel, FunctionalState Cmd);
void DAC_SoftTrgEnable(uint32_t DAC_Channel, FunctionalState Cmd);
void DAC_DualSoftwareTrgEnable(FunctionalState Cmd);
void DAC_WaveGenerationEnable(uint32_t DAC_Channel, uint32_t DAC_Wave, FunctionalState Cmd);
void DAC_SetCh1Data(uint32_t DAC_Align, uint16_t Data);
void DAC_SetCh2Data(uint32_t DAC_Align, uint16_t Data);
void DAC_SetDualChData(uint32_t DAC_Align, uint16_t Data2, uint16_t Data1);
uint16_t DAC_GetOutputDataVal(uint32_t DAC_Channel);

#ifdef __cplusplus
}
#endif

#endif /*__N32A455_DAC_H__ */
       /**
        * @}
        */

/**
 * @}
 */

/**
 * @}
 */
