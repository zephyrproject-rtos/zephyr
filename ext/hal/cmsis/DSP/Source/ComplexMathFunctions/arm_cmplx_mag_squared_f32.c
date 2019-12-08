/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mag_squared_f32.c
 * Description:  Floating-point complex magnitude squared
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
  @ingroup groupCmplxMath
 */

/**
  @defgroup cmplx_mag_squared Complex Magnitude Squared

  Computes the magnitude squared of the elements of a complex data vector.

  The <code>pSrc</code> points to the source data and
  <code>pDst</code> points to the where the result should be written.
  <code>numSamples</code> specifies the number of complex samples
  in the input array and the data is stored in an interleaved fashion
  (real, imag, real, imag, ...).
  The input array has a total of <code>2*numSamples</code> values;
  the output array has a total of <code>numSamples</code> values.

  The underlying algorithm is used:

  <pre>
  for (n = 0; n < numSamples; n++) {
      pDst[n] = pSrc[(2*n)+0]^2 + pSrc[(2*n)+1]^2;
  }
  </pre>

  There are separate functions for floating-point, Q15, and Q31 data types.
 */

/**
  @addtogroup cmplx_mag_squared
  @{
 */

/**
  @brief         Floating-point complex magnitude squared.
  @param[in]     pSrc        points to input vector
  @param[out]    pDst        points to output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

void arm_cmplx_mag_squared_f32(
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t numSamples)
{
    int32_t blockSize = numSamples;  /* loop counters */
    uint32_t  blkCnt;           /* loop counters */
    f32x4x2_t vecSrc;
    f32x4_t sum;
    float32_t real, imag;                          /* Temporary input variables */

    /* Compute 4 complex samples at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        vecSrc = vld2q(pSrc);
        sum = vmulq(vecSrc.val[0], vecSrc.val[0]);
        sum = vfmaq(sum, vecSrc.val[1], vecSrc.val[1]);
        vst1q(pDst, sum);

        pSrc += 8;
        pDst += 4;
        
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 3;
    while (blkCnt > 0U)
    {
      /* C[0] = (A[0] * A[0] + A[1] * A[1]) */
  
      real = *pSrc++;
      imag = *pSrc++;
  
      /* store result in destination buffer. */
      *pDst++ = (real * real) + (imag * imag);
  
      /* Decrement loop counter */
      blkCnt--;
    }

}

#else
void arm_cmplx_mag_squared_f32(
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        float32_t real, imag;                          /* Temporary input variables */

#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)
  float32x4x2_t vecA;
  float32x4_t vRealA;
  float32x4_t vImagA;
  float32x4_t vMagSqA;

  float32x4x2_t vecB;
  float32x4_t vRealB;
  float32x4_t vImagB;
  float32x4_t vMagSqB;

  /* Loop unrolling: Compute 8 outputs at a time */
  blkCnt = numSamples >> 3;

  while (blkCnt > 0U)
  {
    /* out = sqrt((real * real) + (imag * imag)) */

    vecA = vld2q_f32(pSrc);
    pSrc += 8;

    vRealA = vmulq_f32(vecA.val[0], vecA.val[0]);
    vImagA = vmulq_f32(vecA.val[1], vecA.val[1]);
    vMagSqA = vaddq_f32(vRealA, vImagA);

    vecB = vld2q_f32(pSrc);
    pSrc += 8;

    vRealB = vmulq_f32(vecB.val[0], vecB.val[0]);
    vImagB = vmulq_f32(vecB.val[1], vecB.val[1]);
    vMagSqB = vaddq_f32(vRealB, vImagB);

    /* Store the result in the destination buffer. */
    vst1q_f32(pDst, vMagSqA);
    pDst += 4;

    vst1q_f32(pDst, vMagSqB);
    pDst += 4;

    /* Decrement the loop counter */
    blkCnt--;
  }

  blkCnt = numSamples & 7;

#else
#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

    real = *pSrc++;
    imag = *pSrc++;
    *pDst++ = (real * real) + (imag * imag);

    real = *pSrc++;
    imag = *pSrc++;
    *pDst++ = (real * real) + (imag * imag);

    real = *pSrc++;
    imag = *pSrc++;
    *pDst++ = (real * real) + (imag * imag);

    real = *pSrc++;
    imag = *pSrc++;
    *pDst++ = (real * real) + (imag * imag);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = numSamples % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = numSamples;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */
#endif /* #if defined(ARM_MATH_NEON) */

  while (blkCnt > 0U)
  {
    /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

    real = *pSrc++;
    imag = *pSrc++;

    /* store result in destination buffer. */
    *pDst++ = (real * real) + (imag * imag);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of cmplx_mag_squared group
 */
