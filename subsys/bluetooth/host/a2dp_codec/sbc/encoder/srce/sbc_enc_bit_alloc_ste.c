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
 *  This file contains the code for bit allocation algorithm. It calculates
 *  the number of bits required for the encoded stream of data.
 *
 ******************************************************************************/

/*Includes*/
#include "sbc_enc_func_declare.h"
#include "sbc_encoder.h"

/*global arrays*/
extern const int16_t sbc_enc_as16Offset4[4][4];
extern const int16_t sbc_enc_as16Offset8[4][8];

/****************************************************************************
* BitAlloc - Calculates the required number of bits for the given scale factor
* and the number of subbands.
*
* RETURNS : N/A
*/

void sbc_enc_bit_alloc_ste(SBC_ENC_PARAMS* pstrCodecParams) {
  /* CAUTIOM -> mips optim for arm 32 require to use int32_t instead of int16_t
   */
  /* Do not change variable type or name */
  int32_t s32MaxBitNeed; /*to store the max bits needed per sb*/
  int32_t s32BitCount;   /*the used number of bits*/
  int32_t s32SliceCount; /*to store hwo many slices can be put in bitpool*/
  int32_t s32BitSlice;   /*number of bitslices in bitpool*/
  int32_t s32Sb;         /*counter for sub-band*/
  int32_t s32Ch;         /*counter for channel*/
  int16_t* ps16BitNeed;  /*temp memory to store required number of bits*/
  int32_t s32Loudness;   /*used in Loudness calculation*/
  int16_t *ps16GenBufPtr, *pas16ScaleFactor;
  int16_t* ps16GenArrPtr;
  int16_t* ps16GenTabPtr;
  int32_t s32NumOfSubBands = pstrCodecParams->s16NumOfSubBands;
  int32_t s32BitPool = pstrCodecParams->s16BitPool;

  /* bitneed values are derived from scale factor */
  if (pstrCodecParams->s16AllocationMethod == SBC_SNR) {
    ps16BitNeed = pstrCodecParams->as16ScaleFactor;
    s32MaxBitNeed = pstrCodecParams->s16MaxBitNeed;
  } else {
    ps16BitNeed = pstrCodecParams->s16ScartchMemForBitAlloc;
    pas16ScaleFactor = pstrCodecParams->as16ScaleFactor;
    s32MaxBitNeed = 0;
    ps16GenBufPtr = ps16BitNeed;
    for (s32Ch = 0; s32Ch < 2; s32Ch++) {
      if (s32NumOfSubBands == 4) {
        ps16GenTabPtr =
            (int16_t*)sbc_enc_as16Offset4[pstrCodecParams->s16SamplingFreq];
      } else {
        ps16GenTabPtr =
            (int16_t*)sbc_enc_as16Offset8[pstrCodecParams->s16SamplingFreq];
      }

      for (s32Sb = 0; s32Sb < s32NumOfSubBands; s32Sb++) {
        if (*pas16ScaleFactor == 0)
          *ps16GenBufPtr = -5;
        else {
          s32Loudness = (int32_t)(*pas16ScaleFactor - *ps16GenTabPtr);

          if (s32Loudness > 0)
            *ps16GenBufPtr = (int16_t)(s32Loudness >> 1);
          else
            *ps16GenBufPtr = (int16_t)s32Loudness;
        }

        if (*ps16GenBufPtr > s32MaxBitNeed) s32MaxBitNeed = *ps16GenBufPtr;
        pas16ScaleFactor++;
        ps16GenBufPtr++;
        ps16GenTabPtr++;
      }
    }
  }

  /* iterative process to find out hwo many bitslices fit into the bitpool */
  s32BitSlice = s32MaxBitNeed + 1;
  s32BitCount = s32BitPool;
  s32SliceCount = 0;
  do {
    s32BitSlice--;
    s32BitCount -= s32SliceCount;
    s32SliceCount = 0;
    ps16GenBufPtr = ps16BitNeed;

    for (s32Sb = 0; s32Sb < 2 * s32NumOfSubBands; s32Sb++) {
      if ((*ps16GenBufPtr >= s32BitSlice + 1) &&
          (*ps16GenBufPtr < s32BitSlice + 16)) {
        if (*(ps16GenBufPtr) == s32BitSlice + 1)
          s32SliceCount += 2;
        else
          s32SliceCount++;
      }
      ps16GenBufPtr++;
    }
  } while (s32BitCount - s32SliceCount > 0);

  if (s32BitCount - s32SliceCount == 0) {
    s32BitCount -= s32SliceCount;
    s32BitSlice--;
  }

  /* Bits are distributed until the last bitslice is reached */
  ps16GenBufPtr = ps16BitNeed;
  ps16GenArrPtr = pstrCodecParams->as16Bits;
  for (s32Ch = 0; s32Ch < 2; s32Ch++) {
    for (s32Sb = 0; s32Sb < s32NumOfSubBands; s32Sb++) {
      if (*ps16GenBufPtr < s32BitSlice + 2)
        *ps16GenArrPtr = 0;
      else
        *ps16GenArrPtr = ((*(ps16GenBufPtr)-s32BitSlice) < 16)
                             ? (int16_t)(*(ps16GenBufPtr)-s32BitSlice)
                             : 16;
      ps16GenBufPtr++;
      ps16GenArrPtr++;
    }
  }

  /* the remaining bits are allocated starting at subband 0 */
  s32Ch = 0;
  s32Sb = 0;
  ps16GenBufPtr = ps16BitNeed;
  ps16GenArrPtr -= 2 * s32NumOfSubBands;

  while ((s32BitCount > 0) && (s32Sb < s32NumOfSubBands)) {
    if ((*(ps16GenArrPtr) >= 2) && (*(ps16GenArrPtr) < 16)) {
      (*(ps16GenArrPtr))++;
      s32BitCount--;
    } else if ((*ps16GenBufPtr == s32BitSlice + 1) && (s32BitCount > 1)) {
      *(ps16GenArrPtr) = 2;
      s32BitCount -= 2;
    }
    if (s32Ch == 1) {
      s32Ch = 0;
      s32Sb++;
      ps16GenBufPtr = ps16BitNeed + s32Sb;
      ps16GenArrPtr = pstrCodecParams->as16Bits + s32Sb;

    } else {
      s32Ch = 1;
      ps16GenBufPtr = ps16BitNeed + s32NumOfSubBands + s32Sb;
      ps16GenArrPtr = pstrCodecParams->as16Bits + s32NumOfSubBands + s32Sb;
    }
  }

  s32Ch = 0;
  s32Sb = 0;
  ps16GenArrPtr = pstrCodecParams->as16Bits;

  while ((s32BitCount > 0) && (s32Sb < s32NumOfSubBands)) {
    if (*(ps16GenArrPtr) < 16) {
      (*(ps16GenArrPtr))++;
      s32BitCount--;
    }
    if (s32Ch == 1) {
      s32Ch = 0;
      s32Sb++;
      ps16GenArrPtr = pstrCodecParams->as16Bits + s32Sb;
    } else {
      s32Ch = 1;
      ps16GenArrPtr = pstrCodecParams->as16Bits + s32NumOfSubBands + s32Sb;
    }
  }
}

/*End of BitAlloc() function*/
