/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_q7_to_float.c
 * Description:  Converts the elements of the Q7 vector to floating-point vector
 *
 * $Date:        18. March 2019
 * $Revision:    V1.6.0
 *
 * Target Processor: Cortex-M cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2019 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "arm_math.h"

/**
  @ingroup groupSupport
 */

/**
 * @defgroup q7_to_x  Convert 8-bit Integer value
 */

/**
  @addtogroup q7_to_x
  @{
 */

/**
  @brief         Converts the elements of the Q7 vector to floating-point vector.
  @param[in]     pSrc       points to the Q7 input vector
  @param[out]    pDst       points to the floating-point output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

 @par            Details
                   The equation used for the conversion process is:
  <pre>
      pDst[n] = (float32_t) pSrc[n] / 128;   0 <= n < blockSize.
  </pre>
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_q7_to_float(
  const q7_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t vecDst;
    q7_t const *pSrcVec;

    pSrcVec = (q7_t const *) pSrc;
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        /* C = (float32_t) A / 32768 */
        /* convert from q7 to float and then store the results in the destination buffer */
        vecDst = vldrbq_s32(pSrcVec);    
        pSrcVec += 4;
        vstrwq(pDst, vcvtq_n_f32_s32(vecDst, 7));   
        pDst += 4;
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }

  blkCnt = blockSize & 3;
  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 128 */

    /* Convert from q7 to float and store result in destination buffer */
    *pDst++ = ((float32_t) * pSrcVec++ / 128.0f);

    /* Decrement loop counter */
    blkCnt--;
  }
}
#else
#if defined(ARM_MATH_NEON)
void arm_q7_to_float(
  const q7_t * pSrc,
  float32_t * pDst,
  uint32_t blockSize)
{
  const q7_t *pIn = pSrc;                              /* Src pointer */
  uint32_t blkCnt;                               /* loop counter */

  int8x16_t inV;
  int16x8_t inVLO, inVHI;
  int32x4_t inVLL, inVLH, inVHL, inVHH;
  float32x4_t outV;

  blkCnt = blockSize >> 4U;

  /* Compute 16 outputs at a time.
   ** a second loop below computes the remaining 1 to 15 samples. */
  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 128 */
    /* Convert from q7 to float and then store the results in the destination buffer */
    inV = vld1q_s8(pIn);
    pIn += 16;

    inVLO = vmovl_s8(vget_low_s8(inV));
    inVHI = vmovl_s8(vget_high_s8(inV));

    inVLL = vmovl_s16(vget_low_s16(inVLO));
    inVLH = vmovl_s16(vget_high_s16(inVLO));
    inVHL = vmovl_s16(vget_low_s16(inVHI));
    inVHH = vmovl_s16(vget_high_s16(inVHI));

    outV = vcvtq_n_f32_s32(inVLL,7);
    vst1q_f32(pDst, outV);
    pDst += 4;

    outV = vcvtq_n_f32_s32(inVLH,7);
    vst1q_f32(pDst, outV);
    pDst += 4;

    outV = vcvtq_n_f32_s32(inVHL,7);
    vst1q_f32(pDst, outV);
    pDst += 4;

    outV = vcvtq_n_f32_s32(inVHH,7);
    vst1q_f32(pDst, outV);
    pDst += 4;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* If the blockSize is not a multiple of 16, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 0xF;

  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 128 */
    /* Convert from q7 to float and then store the results in the destination buffer */
    *pDst++ = ((float32_t) * pIn++ / 128.0f);

    /* Decrement the loop counter */
    blkCnt--;
  }
}
#else
void arm_q7_to_float(
  const q7_t * pSrc,
        float32_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
  const q7_t *pIn = pSrc;                              /* Source pointer */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 128 */

    /* Convert from q7 to float and store result in destination buffer */
    *pDst++ = ((float32_t) * pIn++ / 128.0f);
    *pDst++ = ((float32_t) * pIn++ / 128.0f);
    *pDst++ = ((float32_t) * pIn++ / 128.0f);
    *pDst++ = ((float32_t) * pIn++ / 128.0f);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C = (float32_t) A / 128 */

    /* Convert from q7 to float and store result in destination buffer */
    *pDst++ = ((float32_t) * pIn++ / 128.0f);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of q7_to_x group
 */
