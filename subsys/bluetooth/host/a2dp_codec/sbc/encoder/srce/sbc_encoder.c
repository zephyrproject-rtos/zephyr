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
 *  contains code for encoder flow and initalization of encoder
 *
 ******************************************************************************/

#include "sbc_encoder.h"
#include <string.h>
#include "bt_target.h"
#include "sbc_enc_func_declare.h"

int16_t EncMaxShiftCounter;

#if (SBC_JOINT_STE_INCLUDED == TRUE)
int32_t s32LRDiff[SBC_MAX_NUM_OF_BLOCKS] = {0};
int32_t s32LRSum[SBC_MAX_NUM_OF_BLOCKS] = {0};
#endif

uint32_t SBC_Encode(SBC_ENC_PARAMS* pstrEncParams, int16_t* input,
                    uint8_t* output) {
  int32_t s32Ch;                 /* counter for ch*/
  int32_t s32Sb;                 /* counter for sub-band*/
  uint32_t u32Count, maxBit = 0; /* loop count*/
  int32_t s32MaxValue;           /* temp variable to store max value */

  int16_t* ps16ScfL;
  int32_t* SbBuffer;
  int32_t s32Blk; /* counter for block*/
  int32_t s32NumOfBlocks = pstrEncParams->s16NumOfBlocks;
#if (SBC_JOINT_STE_INCLUDED == TRUE)
  int32_t s32MaxValue2;
  uint32_t u32CountSum, u32CountDiff;
  int32_t *pSum, *pDiff;
#endif
  register int32_t s32NumOfSubBands = pstrEncParams->s16NumOfSubBands;

  /* SBC ananlysis filter*/
  if (s32NumOfSubBands == 4)
    SbcAnalysisFilter4(pstrEncParams, input);
  else
    SbcAnalysisFilter8(pstrEncParams, input);

  /* compute the scale factor, and save the max */
  ps16ScfL = pstrEncParams->as16ScaleFactor;
  s32Ch = pstrEncParams->s16NumOfChannels * s32NumOfSubBands;

  for (s32Sb = 0; s32Sb < s32Ch; s32Sb++) {
    SbBuffer = pstrEncParams->s32SbBuffer + s32Sb;
    s32MaxValue = 0;
    for (s32Blk = s32NumOfBlocks; s32Blk > 0; s32Blk--) {
      if (s32MaxValue < abs32(*SbBuffer)) s32MaxValue = abs32(*SbBuffer);
      SbBuffer += s32Ch;
    }

    u32Count = (s32MaxValue > 0x800000) ? 9 : 0;

    for (; u32Count < 15; u32Count++) {
      if (s32MaxValue <= (int32_t)(0x8000 << u32Count)) break;
    }
    *ps16ScfL++ = (int16_t)u32Count;

    if (u32Count > maxBit) maxBit = u32Count;
  }
/* In case of JS processing,check whether to use JS */
#if (SBC_JOINT_STE_INCLUDED == TRUE)
  if (pstrEncParams->s16ChannelMode == SBC_JOINT_STEREO) {
    /* Calculate sum and differance  scale factors for making JS decision   */
    ps16ScfL = pstrEncParams->as16ScaleFactor;
    /* calculate the scale factor of Joint stereo max sum and diff */
    for (s32Sb = 0; s32Sb < s32NumOfSubBands - 1; s32Sb++) {
      SbBuffer = pstrEncParams->s32SbBuffer + s32Sb;
      s32MaxValue2 = 0;
      s32MaxValue = 0;
      pSum = s32LRSum;
      pDiff = s32LRDiff;
      for (s32Blk = 0; s32Blk < s32NumOfBlocks; s32Blk++) {
        *pSum = (*SbBuffer + *(SbBuffer + s32NumOfSubBands)) >> 1;
        if (abs32(*pSum) > s32MaxValue) s32MaxValue = abs32(*pSum);
        pSum++;
        *pDiff = (*SbBuffer - *(SbBuffer + s32NumOfSubBands)) >> 1;
        if (abs32(*pDiff) > s32MaxValue2) s32MaxValue2 = abs32(*pDiff);
        pDiff++;
        SbBuffer += s32Ch;
      }
      u32Count = (s32MaxValue > 0x800000) ? 9 : 0;
      for (; u32Count < 15; u32Count++) {
        if (s32MaxValue <= (int32_t)(0x8000 << u32Count)) break;
      }
      u32CountSum = u32Count;
      u32Count = (s32MaxValue2 > 0x800000) ? 9 : 0;
      for (; u32Count < 15; u32Count++) {
        if (s32MaxValue2 <= (int32_t)(0x8000 << u32Count)) break;
      }
      u32CountDiff = u32Count;
      if ((*ps16ScfL + *(ps16ScfL + s32NumOfSubBands)) >
          (int16_t)(u32CountSum + u32CountDiff)) {
        if (u32CountSum > maxBit) maxBit = u32CountSum;

        if (u32CountDiff > maxBit) maxBit = u32CountDiff;

        *ps16ScfL = (int16_t)u32CountSum;
        *(ps16ScfL + s32NumOfSubBands) = (int16_t)u32CountDiff;

        SbBuffer = pstrEncParams->s32SbBuffer + s32Sb;
        pSum = s32LRSum;
        pDiff = s32LRDiff;

        for (s32Blk = 0; s32Blk < s32NumOfBlocks; s32Blk++) {
          *SbBuffer = *pSum;
          *(SbBuffer + s32NumOfSubBands) = *pDiff;

          SbBuffer += s32NumOfSubBands << 1;
          pSum++;
          pDiff++;
        }

        pstrEncParams->as16Join[s32Sb] = 1;
      } else {
        pstrEncParams->as16Join[s32Sb] = 0;
      }
      ps16ScfL++;
    }
    pstrEncParams->as16Join[s32Sb] = 0;
  }
#endif

  pstrEncParams->s16MaxBitNeed = (int16_t)maxBit;

  /* bit allocation */
  if ((pstrEncParams->s16ChannelMode == SBC_STEREO) ||
      (pstrEncParams->s16ChannelMode == SBC_JOINT_STEREO))
    sbc_enc_bit_alloc_ste(pstrEncParams);
  else
    sbc_enc_bit_alloc_mono(pstrEncParams);

  /* Quantize the encoded audio */
  return EncPacking(pstrEncParams, output);
}

/****************************************************************************
* InitSbcAnalysisFilt - Initalizes the input data to 0
*
* RETURNS : N/A
*/
void SBC_Encoder_Init(SBC_ENC_PARAMS* pstrEncParams) {
  uint16_t s16SamplingFreq; /*temp variable to store smpling freq*/
  int16_t s16Bitpool;       /*to store bit pool value*/
  int16_t s16BitRate;       /*to store bitrate*/
  int16_t s16FrameLen;      /*to store frame length*/
  uint16_t HeaderParams;

  /* Required number of channels */
  if (pstrEncParams->s16ChannelMode == SBC_MONO)
    pstrEncParams->s16NumOfChannels = 1;
  else
    pstrEncParams->s16NumOfChannels = 2;

  /* Bit pool calculation */
  if (pstrEncParams->s16SamplingFreq == SBC_sf16000)
    s16SamplingFreq = 16000;
  else if (pstrEncParams->s16SamplingFreq == SBC_sf32000)
    s16SamplingFreq = 32000;
  else if (pstrEncParams->s16SamplingFreq == SBC_sf44100)
    s16SamplingFreq = 44100;
  else
    s16SamplingFreq = 48000;

  if ((pstrEncParams->s16ChannelMode == SBC_JOINT_STEREO) ||
      (pstrEncParams->s16ChannelMode == SBC_STEREO)) {
    s16Bitpool =
        (int16_t)((pstrEncParams->u16BitRate * pstrEncParams->s16NumOfSubBands *
                   1000 / s16SamplingFreq) -
                  ((32 + (4 * pstrEncParams->s16NumOfSubBands *
                          pstrEncParams->s16NumOfChannels) +
                    ((pstrEncParams->s16ChannelMode - 2) *
                     pstrEncParams->s16NumOfSubBands)) /
                   pstrEncParams->s16NumOfBlocks));

    s16FrameLen = 4 +
                  (4 * pstrEncParams->s16NumOfSubBands *
                   pstrEncParams->s16NumOfChannels) /
                      8 +
                  (((pstrEncParams->s16ChannelMode - 2) *
                    pstrEncParams->s16NumOfSubBands) +
                   (pstrEncParams->s16NumOfBlocks * s16Bitpool)) /
                      8;

    s16BitRate = (8 * s16FrameLen * s16SamplingFreq) /
                 (pstrEncParams->s16NumOfSubBands *
                  pstrEncParams->s16NumOfBlocks * 1000);

    if (s16BitRate > pstrEncParams->u16BitRate) s16Bitpool--;

    if (pstrEncParams->s16NumOfSubBands == 8)
      pstrEncParams->s16BitPool = (s16Bitpool > 255) ? 255 : s16Bitpool;
    else
      pstrEncParams->s16BitPool = (s16Bitpool > 128) ? 128 : s16Bitpool;
  } else {
    s16Bitpool = (int16_t)(
        ((pstrEncParams->s16NumOfSubBands * pstrEncParams->u16BitRate * 1000) /
         (s16SamplingFreq * pstrEncParams->s16NumOfChannels)) -
        (((32 / pstrEncParams->s16NumOfChannels) +
          (4 * pstrEncParams->s16NumOfSubBands)) /
         pstrEncParams->s16NumOfBlocks));

    pstrEncParams->s16BitPool =
        (s16Bitpool > (16 * pstrEncParams->s16NumOfSubBands))
            ? (16 * pstrEncParams->s16NumOfSubBands)
            : s16Bitpool;
  }

  if (pstrEncParams->s16BitPool < 0) pstrEncParams->s16BitPool = 0;
  /* sampling freq */
  HeaderParams = ((pstrEncParams->s16SamplingFreq & 3) << 6);

  /* number of blocks*/
  HeaderParams |= (((pstrEncParams->s16NumOfBlocks - 4) & 12) << 2);

  /* channel mode: mono, dual...*/
  HeaderParams |= ((pstrEncParams->s16ChannelMode & 3) << 2);

  /* Loudness or SNR */
  HeaderParams |= ((pstrEncParams->s16AllocationMethod & 1) << 1);
  HeaderParams |= ((pstrEncParams->s16NumOfSubBands >> 3) & 1); /*4 or 8*/
  pstrEncParams->FrameHeader = HeaderParams;

  if (pstrEncParams->s16NumOfSubBands == 4) {
    if (pstrEncParams->s16NumOfChannels == 1)
      EncMaxShiftCounter = ((ENC_VX_BUFFER_SIZE - 4 * 10) >> 2) << 2;
    else
      EncMaxShiftCounter = ((ENC_VX_BUFFER_SIZE - 4 * 10 * 2) >> 3) << 2;
  } else {
    if (pstrEncParams->s16NumOfChannels == 1)
      EncMaxShiftCounter = ((ENC_VX_BUFFER_SIZE - 8 * 10) >> 3) << 3;
    else
      EncMaxShiftCounter = ((ENC_VX_BUFFER_SIZE - 8 * 10 * 2) >> 4) << 3;
  }

  SbcAnalysisInit();
}
