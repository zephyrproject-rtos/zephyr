/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mean_f32.c
 * Description:  Mean value of a floating-point vector
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
  @ingroup groupStats
 */

/**
  @defgroup mean Mean

  Calculates the mean of the input vector. Mean is defined as the average of the elements in the vector.
  The underlying algorithm is used:

  <pre>
      Result = (pSrc[0] + pSrc[1] + pSrc[2] + ... + pSrc[blockSize-1]) / blockSize;
  </pre>

  There are separate functions for floating-point, Q31, Q15, and Q7 data types.
 */

/**
  @addtogroup mean
  @{
 */

/**
  @brief         Mean value of a floating-point vector.
  @param[in]     pSrc       points to the input vector.
  @param[in]     blockSize  number of samples in input vector.
  @param[out]    pResult    mean value returned here.
  @return        none
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

void arm_mean_f32(
  const float32_t * pSrc,
  uint32_t blockSize,
  float32_t * pResult)
{
    uint32_t  blkCnt;           /* loop counters */
    f32x4_t vecSrc;
    f32x4_t sumVec = vdupq_n_f32(0.0f);
    float32_t sum = 0.0f; 

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {
        vecSrc = vldrwq_f32(pSrc);
        sumVec = vaddq_f32(sumVec, vecSrc);

        blkCnt --;
        pSrc += 4;
    }

    sum = vecAddAcrossF32Mve(sumVec);

    /* Tail */
    blkCnt = blockSize & 0x3;

    while (blkCnt > 0U)
    {
      /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
      sum += *pSrc++;
  
      /* Decrement loop counter */
      blkCnt--;
    }

    *pResult = sum / (float32_t) blockSize;
}


#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_mean_f32(
  const float32_t * pSrc,
  uint32_t blockSize,
  float32_t * pResult)
{
  float32_t sum = 0.0f;                          /* Temporary result storage */
  float32x4_t sumV = vdupq_n_f32(0.0f);                          /* Temporary result storage */
  float32x2_t sumV2;

  uint32_t blkCnt;                               /* Loop counter */

  float32_t in1, in2, in3, in4;
  float32x4_t inV;

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
    inV = vld1q_f32(pSrc);
    sumV = vaddq_f32(sumV, inV);
    
    pSrc += 4;
    /* Decrement the loop counter */
    blkCnt--;
  }

  sumV2 = vpadd_f32(vget_low_f32(sumV),vget_high_f32(sumV));
  sum = sumV2[0] + sumV2[1];

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize & 3;

  while (blkCnt > 0U)
  {
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
    sum += *pSrc++;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) / blockSize  */
  /* Store the result to the destination */
  *pResult = sum / (float32_t) blockSize;
}
#else
void arm_mean_f32(
  const float32_t * pSrc,
        uint32_t blockSize,
        float32_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        float32_t sum = 0.0f;                          /* Temporary result storage */

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
    sum += *pSrc++;

    sum += *pSrc++;

    sum += *pSrc++;

    sum += *pSrc++;

    /* Decrement the loop counter */
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
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
    sum += *pSrc++;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) / blockSize  */
  /* Store result to destination */
  *pResult = (sum / blockSize);
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of mean group
 */
