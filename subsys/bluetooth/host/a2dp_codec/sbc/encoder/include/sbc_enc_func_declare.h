/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Function declarations.
 *
 ******************************************************************************/

#ifndef SBC_FUNCDECLARE_H
#define SBC_FUNCDECLARE_H

#include "sbc_encoder.h"
/* Global data */
#if (SBC_IS_64_MULT_IN_WINDOW_ACCU == FALSE)
extern const int16_t gas32CoeffFor4SBs[];
extern const int16_t gas32CoeffFor8SBs[];
#else
extern const int32_t gas32CoeffFor4SBs[];
extern const int32_t gas32CoeffFor8SBs[];
#endif

/* Global functions*/

extern void sbc_enc_bit_alloc_mono(SBC_ENC_PARAMS* CodecParams);
extern void sbc_enc_bit_alloc_ste(SBC_ENC_PARAMS* CodecParams);

extern void SbcAnalysisInit(void);

extern void SbcAnalysisFilter4(SBC_ENC_PARAMS* strEncParams, int16_t* input);
extern void SbcAnalysisFilter8(SBC_ENC_PARAMS* strEncParams, int16_t* input);

extern void SBC_FastIDCT8(int32_t* pInVect, int32_t* pOutVect);
extern void SBC_FastIDCT4(int32_t* x0, int32_t* pOutVect);

extern uint32_t EncPacking(SBC_ENC_PARAMS* strEncParams, uint8_t* output);
extern void EncQuantizer(SBC_ENC_PARAMS*);
#if (SBC_DSP_OPT == TRUE)
int32_t SBC_Multiply_32_16_Simplified(int32_t s32In2Temp, int32_t s32In1Temp);
#endif
#endif
