/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_var_f32.c
 * Description:  Variance of the elements of a floating-point vector
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
  @defgroup variance  Variance

  Calculates the variance of the elements in the input vector.
  The underlying algorithm used is the direct method sometimes referred to as the two-pass method:

  <pre>
      Result = sum(element - meanOfElements)^2) / numElement - 1

      meanOfElements = ( pSrc[0] * pSrc[0] + pSrc[1] * pSrc[1] + ... + pSrc[blockSize-1] ) / blockSize
  </pre>

  There are separate functions for floating point, Q31, and Q15 data types.
 */

/**
  @addtogroup variance
  @{
 */

/**
  @brief         Variance of the elements of a floating-point vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    variance value returned here
  @return        none
 */
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

#include "arm_helium_utils.h"

void arm_var_f32(
           const float32_t * pSrc,
                 uint32_t blockSize,
                 float32_t * pResult)
{
    uint32_t         blkCnt;     /* loop counters */
    f32x4_t         vecSrc;
    f32x4_t         sumVec = vdupq_n_f32(0.0f);
    float32_t       fMean;
    float32_t sum = 0.0f;                          /* accumulator */
    float32_t in;                                  /* Temporary variable to store input value */

    if (blockSize <= 1U) {
        *pResult = 0;
        return;
    }

    arm_mean_f32(pSrc, blockSize, &fMean);

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {

        vecSrc = vldrwq_f32(pSrc);
        /*
         * sum lanes
         */
        vecSrc = vsubq(vecSrc, fMean);
        sumVec = vfmaq(sumVec, vecSrc, vecSrc);

        blkCnt --;
        pSrc += 4;
    }

    sum = vecAddAcrossF32Mve(sumVec);

    /*
     * tail
     */
    blkCnt = blockSize & 0x3;
    while (blkCnt > 0U)
    {
       in = *pSrc++ - fMean;
       sum += in * in;

       /* Decrement loop counter */
       blkCnt--;
    }
   
    /* Variance */
    *pResult = sum / (float32_t) (blockSize - 1);
}
#else
#if defined(ARM_MATH_NEON_EXPERIMENTAL) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_var_f32(
           const float32_t * pSrc,
                 uint32_t blockSize,
                 float32_t * pResult)
{
  float32_t mean;

  float32_t sum = 0.0f;                          /* accumulator */
  float32_t in;                                  /* Temporary variable to store input value */
  uint32_t blkCnt;                               /* loop counter */

  float32x4_t sumV = vdupq_n_f32(0.0f);                          /* Temporary result storage */
  float32x2_t sumV2;
  float32x4_t inV;
  float32x4_t avg;

  arm_mean_f32(pSrc,blockSize,&mean);
  avg = vdupq_n_f32(mean);

  blkCnt = blockSize >> 2U;

  /* Compute 4 outputs at a time.
   ** a second loop below computes the remaining 1 to 3 samples. */
  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + A[2] * A[2] + ... + A[blockSize-1] * A[blockSize-1] */
    /* Compute Power and then store the result in a temporary variable, sum. */
    inV = vld1q_f32(pSrc);
    inV = vsubq_f32(inV, avg);
    sumV = vmlaq_f32(sumV, inV, inV);
    pSrc += 4;

    /* Decrement the loop counter */
    blkCnt--;
  }

  sumV2 = vpadd_f32(vget_low_f32(sumV),vget_high_f32(sumV));
  sum = sumV2[0] + sumV2[1];

  /* If the blockSize is not a multiple of 4, compute any remaining output samples here.
   ** No loop unrolling is used. */
  blkCnt = blockSize % 0x4U;

  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + A[2] * A[2] + ... + A[blockSize-1] * A[blockSize-1] */
    /* compute power and then store the result in a temporary variable, sum. */
    in = *pSrc++;
    in = in - mean;
    sum += in * in;

    /* Decrement the loop counter */
    blkCnt--;
  }

  /* Variance */
  *pResult = sum / (float32_t)(blockSize - 1.0f);

}

#else
void arm_var_f32(
  const float32_t * pSrc,
        uint32_t blockSize,
        float32_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        float32_t sum = 0.0f;                          /* Temporary result storage */
        float32_t fSum = 0.0f;
        float32_t fMean, fValue;
  const float32_t * pInput = pSrc;

  if (blockSize <= 1U)
  {
    *pResult = 0;
    return;
  }

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */

    sum += *pInput++;
    sum += *pInput++;
    sum += *pInput++;
    sum += *pInput++;


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
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */

    sum += *pInput++;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) / blockSize  */
  fMean = sum / (float32_t) blockSize;

  pInput = pSrc;

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    fValue = *pInput++ - fMean;
    fSum += fValue * fValue;

    fValue = *pInput++ - fMean;
    fSum += fValue * fValue;

    fValue = *pInput++ - fMean;
    fSum += fValue * fValue;

    fValue = *pInput++ - fMean;
    fSum += fValue * fValue;

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
    fValue = *pInput++ - fMean;
    fSum += fValue * fValue;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Variance */
  *pResult = fSum / (float32_t)(blockSize - 1.0f);
}
#endif /* #if defined(ARM_MATH_NEON) */
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of variance group
 */
