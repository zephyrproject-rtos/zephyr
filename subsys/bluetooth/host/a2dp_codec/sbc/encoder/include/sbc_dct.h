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
 *  Definitions for the fast DCT.
 *
 ******************************************************************************/

#ifndef SBC_DCT_H
#define SBC_DCT_H

#include "sbc_enc_func_declare.h"

#if (SBC_ARM_ASM_OPT == TRUE)
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1, s32OutLow) \
  {                                                          \
    __asm {																				\
    MUL s32OutLow,(int32_t)s16In2, (s32In1>>15) } \
  }
#else
#if (SBC_DSP_OPT == TRUE)
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1, s32OutLow) \
  s32OutLow = SBC_Multiply_32_16_Simplified((int32_t)s16In2, s32In1);
#else
#if (SBC_IPAQ_OPT == TRUE)
/*
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1 , s32OutLow)
s32OutLow=(int32_t)((int32_t)(s16In2)*(int32_t)(s32In1>>15));
*/
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1, s32OutLow) \
  s32OutLow = (int32_t)(((int64_t)(s16In2) * (int64_t)(s32In1)) >> 15);
#if (SBC_IS_64_MULT_IN_IDCT == TRUE)
#define SBC_MULT_32_32(s32In2, s32In1, s32OutLow)          \
  {                                                        \
    s64Temp = ((int64_t)s32In2) * ((int64_t)s32In1) >> 31; \
    s32OutLow = (int32_t)s64Temp;                          \
  }
#endif
#else
#define SBC_MULT_32_16_SIMPLIFIED(s16In2, s32In1, s32OutLow)     \
  {                                                              \
    s32In1Temp = s32In1;                                         \
    s32In2Temp = (int32_t)s16In2;                                \
                                                                 \
    /* Multiply one +ve and the other -ve number */              \
    if (s32In1Temp < 0) {                                        \
      s32In1Temp ^= 0xFFFFFFFF;                                  \
      s32In1Temp++;                                              \
      s32OutLow = (s32In2Temp * (s32In1Temp >> 16));             \
      s32OutLow += ((s32In2Temp * (s32In1Temp & 0xFFFF)) >> 16); \
      s32OutLow ^= 0xFFFFFFFF;                                   \
      s32OutLow++;                                               \
    } else {                                                     \
      s32OutLow = (s32In2Temp * (s32In1Temp >> 16));             \
      s32OutLow += ((s32In2Temp * (s32In1Temp & 0xFFFF)) >> 16); \
    }                                                            \
    s32OutLow <<= 1;                                             \
  }
#if (SBC_IS_64_MULT_IN_IDCT == TRUE)
#define SBC_MULT_64(s32In1, s32In2, s32OutLow, s32OutHi)                     \
  {                                                                          \
    s32OutLow =                                                              \
        (int32_t)(((int64_t)s32In1 * (int64_t)s32In2) & 0x00000000FFFFFFFF); \
    s32OutHi = (int32_t)(((int64_t)s32In1 * (int64_t)s32In2) >> 32);         \
  }
#define SBC_MULT_32_32(s32In2, s32In1, s32OutLow)                    \
  {                                                                  \
    s32HiTemp = 0;                                                   \
    SBC_MULT_64(s32In2, s32In1, s32OutLow, s32HiTemp);               \
    s32OutLow = (((s32OutLow >> 15) & 0x1FFFF) | (s32HiTemp << 17)); \
  }
#endif

#endif
#endif
#endif

#endif
