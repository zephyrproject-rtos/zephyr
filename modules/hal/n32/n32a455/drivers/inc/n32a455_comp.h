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
 * @file n32a455_comp.h
 * @author Nations
 * @version v1.0.0
 *
 * @copyright Copyright (c) 2022, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32A455_COMP_H__
#define __N32A455_COMP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "n32a455.h"
#include <stdbool.h>

/** @addtogroup N32A455_StdPeriph_Driver
 * @{
 */

/** @addtogroup COMP
 * @{
 */

/** @addtogroup COMP_Exported_Constants
 * @{
 */
typedef enum
{
    COMP1 = 0,
    COMP2 = 1,
    COMP3 = 2,
    COMP4 = 3,
    COMP5 = 4,
    COMP6 = 5,
    COMP7 = 6
} COMPX;

// COMPx_CTRL
#define COMP1_CTRL_INPDAC_MASK (0x01L << 18)
#define COMP_CTRL_OUT_MASK     (0x01L << 17)
#define COMP_CTRL_BLKING_MASK  (0x07L << 14)
typedef enum
{
    COMP_CTRL_BLKING_NO       = (0x0L << 14),
    COMP_CTRL_BLKING_TIM1_OC5 = (0x1L << 14),
    COMP_CTRL_BLKING_TIM8_OC5 = (0x2L << 14),
} COMP_CTRL_BLKING;
#define COMPx_CTRL_HYST_MASK (0x03L << 12)
typedef enum
{
    COMP_CTRL_HYST_NO   = (0x0L << 12),
    COMP_CTRL_HYST_LOW  = (0x1L << 12),
    COMP_CTRL_HYST_MID  = (0x2L << 12),
    COMP_CTRL_HYST_HIGH = (0x3L << 12),
} COMP_CTRL_HYST;

#define COMP_POL_MASK         (0x01L << 11)
#define COMP_CTRL_OUTSEL_MASK (0x0FL << 7)
typedef enum
{
    COMPX_CTRL_OUTSEL_NC = (0x0L << 7),
    // comp1 out trig
    COMP1_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP1_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP1_CTRL_OUTSEL_TIM1_IC1            = (0x2L << 7),
    COMP1_CTRL_OUTSEL_TIM1_OCrefclear     = (0x3L << 7),
    COMP1_CTRL_OUTSEL_TIM2_IC1            = (0x4L << 7),
    COMP1_CTRL_OUTSEL_TIM2_OCrefclear     = (0x5L << 7),
    COMP1_CTRL_OUTSEL_TIM3_IC1            = (0x6L << 7),
    COMP1_CTRL_OUTSEL_TIM3_OCrefclear     = (0x7L << 7),
    COMP1_CTRL_OUTSEL_TIM4_IC1            = (0x8L << 7),
    COMP1_CTRL_OUTSEL_TIM4_OCrefclear     = (0x9L << 7),
    COMP1_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP1_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
    // comp2 out trig
    COMP2_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP2_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP2_CTRL_OUTSEL_TIM1_IC1            = (0x2L << 7),
    COMP2_CTRL_OUTSEL_TIM1_OCrefclear     = (0x3L << 7),
    COMP2_CTRL_OUTSEL_TIM2_IC2            = (0x4L << 7),
    COMP2_CTRL_OUTSEL_TIM2_OCrefclear     = (0x5L << 7),
    COMP2_CTRL_OUTSEL_TIM3_IC2            = (0x6L << 7),
    COMP2_CTRL_OUTSEL_TIM3_OCrefclear     = (0x7L << 7),
    COMP2_CTRL_OUTSEL_TIM5_IC1            = (0x8L << 7), ////(0x9L << 7)
    COMP2_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP2_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
    // comp3 out trig
    COMP3_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP3_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP3_CTRL_OUTSEL_TIM1_IC1            = (0x2L << 7),
    COMP3_CTRL_OUTSEL_TIM1_OCrefclear     = (0x3L << 7),
    COMP3_CTRL_OUTSEL_TIM2_IC3            = (0x4L << 7),
    COMP3_CTRL_OUTSEL_TIM2_OCrefclear     = (0x5L << 7),
    COMP3_CTRL_OUTSEL_TIM4_IC2            = (0x6L << 7),
    COMP3_CTRL_OUTSEL_TIM4_OCrefclear     = (0x7L << 7),
    COMP3_CTRL_OUTSEL_TIM5_IC2            = (0x8L << 7), //(0x9L << 7)
    COMP3_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP3_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
    // comp4 out trig
    COMP4_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP4_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP4_CTRL_OUTSEL_TIM3_IC3            = (0x2L << 7),
    COMP4_CTRL_OUTSEL_TIM3_OCrefclear     = (0x3L << 7),
    COMP4_CTRL_OUTSEL_TIM4_IC3            = (0x4L << 7),
    COMP4_CTRL_OUTSEL_TIM4_OCrefclear     = (0x5L << 7),
    COMP4_CTRL_OUTSEL_TIM5_IC3            = (0x6L << 7), //(0x7L << 7)
    COMP4_CTRL_OUTSEL_TIM8_IC1            = (0x8L << 7),
    COMP4_CTRL_OUTSEL_TIM8_OCrefclear     = (0x9L << 7),
    COMP4_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP4_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
    // comp5 out trig
    COMP5_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP5_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP5_CTRL_OUTSEL_TIM2_IC4            = (0x2L << 7),
    COMP5_CTRL_OUTSEL_TIM2_OCrefclear     = (0x3L << 7),
    COMP5_CTRL_OUTSEL_TIM3_IC4            = (0x4L << 7),
    COMP5_CTRL_OUTSEL_TIM3_OCrefclear     = (0x5L << 7),
    COMP5_CTRL_OUTSEL_TIM4_IC4            = (0x6L << 7),
    COMP5_CTRL_OUTSEL_TIM4_OCrefclear     = (0x7L << 7),
    COMP5_CTRL_OUTSEL_TIM8_IC1            = (0x8L << 7),
    COMP5_CTRL_OUTSEL_TIM8_OCrefclear     = (0x9L << 7),
    COMP5_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP5_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
    // comp6 out trig
    COMP6_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP6_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP6_CTRL_OUTSEL_TIM2_IC1            = (0x2L << 7),
    COMP6_CTRL_OUTSEL_TIM2_OCrefclear     = (0x3L << 7),
    COMP6_CTRL_OUTSEL_TIM3_IC1            = (0x4L << 7),
    COMP6_CTRL_OUTSEL_TIM3_OCrefclear     = (0x5L << 7),
    COMP6_CTRL_OUTSEL_TIM5_IC1            = (0x6L << 7), //(0x7L << 7)
    COMP6_CTRL_OUTSEL_TIM8_IC1            = (0x8L << 7),
    COMP6_CTRL_OUTSEL_TIM8_OCrefclear     = (0x9L << 7),
    COMP6_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP6_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
    // comp7 out trig
    COMP7_CTRL_OUTSEL_NC                  = (0x0L << 7),
    COMP7_CTRL_OUTSEL_TIM1_BKIN           = (0x1L << 7),
    COMP7_CTRL_OUTSEL_TIM2_IC1            = (0x2L << 7),
    COMP7_CTRL_OUTSEL_TIM2_OCrefclear     = (0x3L << 7),
    COMP7_CTRL_OUTSEL_TIM3_IC1            = (0x4L << 7),
    COMP7_CTRL_OUTSEL_TIM3_OCrefclear     = (0x5L << 7),
    COMP7_CTRL_OUTSEL_TIM5_IC1            = (0x6L << 7), //(0x7L << 7)
    COMP7_CTRL_OUTSEL_TIM8_IC1            = (0x8L << 7),
    COMP7_CTRL_OUTSEL_TIM8_OCrefclear     = (0x9L << 7),
    COMP7_CTRL_OUTSEL_TIM8_BKIN           = (0xAL << 7),
    COMP7_CTRL_OUTSEL_TIM1_BKIN_TIM8_BKIN = (0xBL << 7),
} COMP_CTRL_OUTTRIG;

#define COMP_CTRL_INPSEL_MASK                       (0x07L<<4)
typedef enum {
  COMPX_CTRL_INPSEL_RES = (0x7L << 4),
  //comp1 inp sel
  COMP1_CTRL_INPSEL_PA1 =   (0x0L << 4),
  COMP1_CTRL_INPSEL_PB10 =  (0x1L << 4),
  //comp2 inp sel, need recheck maybe wrong
  COMP2_CTRL_INPSEL_PA1 =   (0x0L << 4),
  COMP2_CTRL_INPSEL_PB11 =  (0x1L << 4),
  COMP2_CTRL_INPSEL_PA7 =   (0x2L << 4),
  //comp3 inp sel
  COMP3_CTRL_INPSEL_PB14 =  (0x0L << 4),
  COMP3_CTRL_INPSEL_PB0 =   (0x1L << 4),
  //comp4 inp sel, need recheck maybe wrong
  COMP4_CTRL_INPSEL_PB14 =  (0x0L << 4),
  COMP4_CTRL_INPSEL_PB0 =   (0x1L << 4),
  COMP4_CTRL_INPSEL_PC9 =   (0x2L << 4),
  COMP4_CTRL_INPSEL_PB15 =  (0x3L << 4),
  //comp5 inp sel
  COMP5_CTRL_INPSEL_PC4 =  (0x0L << 4),
  COMP5_CTRL_INPSEL_PC3 =  (0x1L << 4),
  COMP5_CTRL_INPSEL_PA3 =  (0x2L << 4),
  //comp6 inp sel, need recheck maybe wrong
  COMP6_CTRL_INPSEL_PC4 =  (0x0L << 4),
  COMP6_CTRL_INPSEL_PC3 =  (0x1L << 4),
  COMP6_CTRL_INPSEL_PC5 =  (0x2L << 4),
  COMP6_CTRL_INPSEL_PD9 =  (0x3L << 4),
  //comp7 inp sel
  COMP7_CTRL_INPSEL_PC1 =  (0x0L << 4),
}COMP_CTRL_INPSEL;

#define COMP_CTRL_INMSEL_MASK                       (0x07L<<1)
typedef enum {
  COMPX_CTRL_INMSEL_RES = (0x7L << 1),
  //comp1 inm sel
  COMP1_CTRL_INMSEL_PA0          = (0x0L << 1),
  COMP1_CTRL_INMSEL_DAC1_PA4     = (0x1L << 1),
  COMP1_CTRL_INMSEL_DAC2_PA5     = (0x2L << 1),
  COMP1_CTRL_INMSEL_VERF1        = (0x3L << 1),
  COMP1_CTRL_INMSEL_VERF2        = (0x4L << 1),
  //comp2 inm sel
  COMP2_CTRL_INMSEL_PB1          = (0x0L << 1),
  COMP2_CTRL_INMSEL_PE8          = (0x1L << 1),
  COMP2_CTRL_INMSEL_DAC1_PA4     = (0x2L << 1),
  COMP2_CTRL_INMSEL_DAC2_PA5     = (0x3L << 1),
  COMP2_CTRL_INMSEL_VERF1        = (0x4L << 1),
  COMP2_CTRL_INMSEL_VERF2        = (0x5L << 1),
  //comp3 inm sel
  COMP3_CTRL_INMSEL_PB12         = (0x0L << 1),
  COMP3_CTRL_INMSEL_PE7          = (0x1L << 1),
  COMP3_CTRL_INMSEL_DAC1_PA4     = (0x2L << 1),
  COMP3_CTRL_INMSEL_DAC2_PA5     = (0x3L << 1),
  COMP3_CTRL_INMSEL_VERF1        = (0x4L << 1),
  COMP3_CTRL_INMSEL_VERF2        = (0x5L << 1),
  //comp4 inm sel
  COMP4_CTRL_INMSEL_PC4          = (0x0L << 1),
  COMP4_CTRL_INMSEL_PB13         = (0x1L << 1),
  COMP4_CTRL_INMSEL_DAC1_PA4     = (0x2L << 1),
  COMP4_CTRL_INMSEL_DAC2_PA5     = (0x3L << 1),
  COMP4_CTRL_INMSEL_VERF1        = (0x4L << 1),
  COMP4_CTRL_INMSEL_VERF2        = (0x5L << 1),
  //comp5 inm sel
  COMP5_CTRL_INMSEL_PB10         = (0x0L << 1),
  COMP5_CTRL_INMSEL_PD10         = (0x1L << 1),
  COMP5_CTRL_INMSEL_DAC1_PA4     = (0x2L << 1),
  COMP5_CTRL_INMSEL_DAC2_PA5     = (0x3L << 1),
  COMP5_CTRL_INMSEL_VERF1        = (0x4L << 1),
  COMP5_CTRL_INMSEL_VERF2        = (0x5L << 1),
  //comp6 inm sel
  COMP6_CTRL_INMSEL_PA7         = (0x0L << 1),
  COMP6_CTRL_INMSEL_PD8         = (0x1L << 1),
  COMP6_CTRL_INMSEL_DAC1_PA4    = (0x2L << 1),
  COMP6_CTRL_INMSEL_DAC2_PA5    = (0x3L << 1),
  COMP6_CTRL_INMSEL_VERF1       = (0x4L << 1),
  COMP6_CTRL_INMSEL_VERF2       = (0x5L << 1),
  //comp7 inm sel
  COMP7_CTRL_INMSEL_PC0        = (0x0L << 1),
  COMP7_CTRL_INMSEL_DAC1_PA4   = (0x1L << 1),
  COMP7_CTRL_INMSEL_DAC2_PA5   = (0x2L << 1),
  COMP7_CTRL_INMSEL_VERF1      = (0x3L << 1),
  COMP7_CTRL_INMSEL_VERF2      = (0x4L << 1),
}COMP_CTRL_INMSEL;

#define COMP_CTRL_EN_MASK (0x01L << 0)

//COMPx_FILC
#define COMP_FILC_SAMPW_MASK                        (0x1FL<<6)//Low filter sample window size. Number of samples to monitor is SAMPWIN+1.
#define COMP_FILC_THRESH_MASK                       (0x1FL<<1)//For proper operation, the value of THRESH must be greater than SAMPWIN / 2.
#define COMP_FILC_FILEN_MASK                        (0x01L<<0)//Filter enable.

//COMPx_FILCLKCR
#define COMP_FILCLKCR_CLKPSC_MASK                        (0xFFFFL<<0)//Low filter sample clock prescale. Number of system clocks between samples = CLK_PRE_CYCLE + 1, e.g.

//COMP_WINMODE @addtogroup COMP_WINMODE_CMPMD
#define COMP_WINMODE_CMPMD_MSK                       (0x07L <<0)
#define COMP_WINMODE_CMP56MD                         (0x01L <<2)//1: Comparators 5 and 6 can be used in window mode.
#define COMP_WINMODE_CMP34MD                         (0x01L <<1)//1: Comparators 3 and 4 can be used in window mode.
#define COMP_WINMODE_CMP12MD                         (0x01L <<0)//1: Comparators 1 and 2 can be used in window mode.

//COMPx_LOCK
#define COMP_LOCK_CMPLK_MSK                          (0x7FL <<0)
#define COMP_LOCK_CMP1LK_MSK                         (0x01L <<0)//1: COMx Lock bit
#define COMP_LOCK_CMP2LK_MSK                         (0x01L <<1)//1: COMx Lock bit
#define COMP_LOCK_CMP3LK_MSK                         (0x01L <<2)//1: COMx Lock bit
#define COMP_LOCK_CMP4LK_MSK                         (0x01L <<3)//1: COMx Lock bit
#define COMP_LOCK_CMP5LK_MSK                         (0x01L <<4)//1: COMx Lock bit
#define COMP_LOCK_CMP6LK_MSK                         (0x01L <<5)//1: COMx Lock bit
#define COMP_LOCK_CMP7LK_MSK                         (0x01L <<6)//1: COMx Lock bit

// COMP_INTEN @addtogroup COMP_INTEN_CMPIEN
#define COMP_INTEN_CMPIEN_MSK (0x7FL << 0)
#define COMP_INTEN_CMP7IEN    (0x01L << 6) // This bit control Interrput enable of COMP.
#define COMP_INTEN_CMP6IEN    (0x01L << 5)
#define COMP_INTEN_CMP5IEN    (0x01L << 4)
#define COMP_INTEN_CMP4IEN    (0x01L << 3)
#define COMP_INTEN_CMP3IEN    (0x01L << 2)
#define COMP_INTEN_CMP2IEN    (0x01L << 1)
#define COMP_INTEN_CMP1IEN    (0x01L << 0)

// COMP_INTSTS @addtogroup COMP_INTSTS_CMPIS
#define COMP_INTSTS_INTSTS_MSK (0x7FL << 0)
#define COMP_INTSTS_CMP7IS     (0x01L << 6) // This bit control Interrput enable of COMP.
#define COMP_INTSTS_CMP6IS     (0x01L << 5)
#define COMP_INTSTS_CMP5IS     (0x01L << 4)
#define COMP_INTSTS_CMP4IS     (0x01L << 3)
#define COMP_INTSTS_CMP3IS     (0x01L << 2)
#define COMP_INTSTS_CMP2IS     (0x01L << 1)
#define COMP_INTSTS_CMP1IS     (0x01L << 0)

// COMP_VREFSCL @addtogroup COMP_VREFSCL
#define COMP_VREFSCL_VV2TRM_MSK (0x3FL << 8) // Vref2 Voltage scaler triming value.
#define COMP_VREFSCL_VV2EN_MSK  (0x01L << 7)
#define COMP_VREFSCL_VV1TRM_MSK (0x3FL << 1) // Vref1 Voltage scaler triming value.
#define COMP_VREFSCL_VV1EN_MSK  (0x01L << 0)
/**
 * @}
 */

/**
 * @brief  COMP Init structure definition
 */

typedef struct
{
    // ctrl
    bool InpDacConnect; // only COMP1 have this bit

    COMP_CTRL_BLKING Blking; /*see @ref COMP_CTRL_BLKING */

    COMP_CTRL_HYST Hyst;

    bool PolRev; // out polarity reverse

    COMP_CTRL_OUTTRIG OutSel;
    COMP_CTRL_INPSEL InpSel;
    COMP_CTRL_INMSEL InmSel;

    bool En;

    // filter
    uint8_t SampWindow; // 5bit
    uint8_t Thresh;     // 5bit ,need > SampWindow/2
    bool FilterEn;

    // filter psc
    uint16_t ClkPsc;
} COMP_InitType;

/** @addtogroup COMP_Exported_Functions
 * @{
 */

void COMP_DeInit(void);
void COMP_StructInit(COMP_InitType* COMP_InitStruct);
void COMP_Init(COMPX COMPx, COMP_InitType* COMP_InitStruct);
void COMP_Enable(COMPX COMPx, FunctionalState en);
void COMP_SetInpSel(COMPX COMPx, COMP_CTRL_INPSEL VpSel);
void COMP_SetInmSel(COMPX COMPx, COMP_CTRL_INMSEL VmSel);
void COMP_SetOutTrig(COMPX COMPx, COMP_CTRL_OUTTRIG OutTrig);
void COMP_SetLock(uint32_t Lock);                                              // see @COMP_LOCK_CMPLK
void COMP_SetIntEn(uint32_t IntEn);                                            // see @COMP_INTEN_CMPIEN
uint32_t COMP_GetIntSts(void);                                                 // return see @COMP_INTSTS_CMPIS
void COMP_SetRefScl(uint8_t Vv2Trim, bool Vv2En, uint8_t Vv1Trim, bool Vv1En); // parma range see @COMP_VREFSCL
FlagStatus COMP_GetOutStatus(COMPX COMPx);
FlagStatus COMP_GetIntStsOneComp(COMPX COMPx);
void COMP_SetFilterPrescaler(COMPX COMPx , uint16_t FilPreVal);
void COMP_SetFilterControl(COMPX COMPx , uint8_t FilEn, uint8_t TheresNum , uint8_t SampPW);
void COMP_SetHyst(COMPX COMPx , COMP_CTRL_HYST HYST);
void COMP_SetBlanking(COMPX COMPx , COMP_CTRL_BLKING BLK);



/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*__N32A455_ADC_H */
/**
 * @}
 */
/**
 * @}
 */
