/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_conj_f32.c
 * Description:  Floating-point complex conjugate
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
  @defgroup cmplx_conj Complex Conjugate

  Conjugates the elements of a complex data vector.

  The <code>pSrc</code> points to the source data and
  <code>pDst</code> points to the destination data where the result should be written.
  <code>numSamples</code> specifies the number of complex samples
  and the data in each array is stored in an interleaved fashion
  (real, imag, real, imag, ...).
  Each array has a total of <code>2*numSamples</code> values.

  The underlying algorithm is used:
  <pre>
  for (n = 0; n < numSamples; n++) {
      pDst[(2*n)  ] =  pSrc[(2*n)  ];    // real part
      pDst[(2*n)+1] = -pSrc[(2*n)+1];    // imag part
  }
  </pre>

  There are separate functions for floating-point, Q15, and Q31 data types.
 */

/**
  @addtogroup cmplx_conj
  @{
 */

/**
  @brief         Floating-point complex conjugate.
  @param[in]     pSrc        points to the input vector
  @param[out]    pDst        points to the output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

void arm_cmplx_conj_f32(
    const float32_t * pSrc,
    float32_t * pDst,
    uint32_t numSamples)
{
    static const float32_t cmplx_conj_sign[4] = { 1.0f, -1.0f, 1.0f, -1.0f };
    uint32_t blockSize = numSamples * CMPLX_DIM;   /* loop counters */
    uint32_t blkCnt;
    f32x4_t vecSrc;
    f32x4_t vecSign;

    /*
     * load sign vector
     */
    vecSign = *(f32x4_t *) cmplx_conj_sign;

    /* Compute 4 real samples at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {
        vecSrc = vld1q(pSrc);
        vst1q(pDst,vmulq(vecSrc, vecSign));
        /*
         * Decrement the blkCnt loop counter
         * Advance vector source and destination pointers
         */
        pSrc += 4;
        pDst += 4;
        blkCnt--;
    }

     /* Tail */
    blkCnt = (blockSize & 0x3) >> 1;

    while (blkCnt > 0U)
    {
      /* C[0] + jC[1] = A[0]+ j(-1)A[1] */
  
      /* Calculate Complex Conjugate and store result in destination buffer. */
      *pDst++ =  *pSrc++;
      *pDst++ = -*pSrc++;
  
      /* Decrement loop counter */
      blkCnt--;
    }

}

#else
void arm_cmplx_conj_f32(
  const float32_t * pSrc,
        float32_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)
   float32x4_t zero;
   float32x4x2_t vec;

   zero = vdupq_n_f32(0.0f);

   /* Compute 4 outputs at a time */
   blkCnt = numSamples >> 2U;

   while (blkCnt > 0U)
   {
     /* C[0]+jC[1] = A[0]+(-1)*jA[1] */
     /* Calculate Complex Conjugate and then store the results in the destination buffer. */
     vec = vld2q_f32(pSrc);
     vec.val[1] = vsubq_f32(zero,vec.val[1]);
     vst2q_f32(pDst,vec);

     /* Increment pointers */
     pSrc += 8;
     pDst += 8;
        
     /* Decrement the loop counter */
     blkCnt--;
   }

   /* Tail */
   blkCnt = numSamples & 0x3;

#else
#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] + jC[1] = A[0]+ j(-1)A[1] */

    /* Calculate Complex Conjugate and store result in destination buffer. */
    *pDst++ =  *pSrc++;
    *pDst++ = -*pSrc++;

    *pDst++ =  *pSrc++;
    *pDst++ = -*pSrc++;

    *pDst++ =  *pSrc++;
    *pDst++ = -*pSrc++;

    *pDst++ =  *pSrc++;
    *pDst++ = -*pSrc++;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = numSamples % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = numSamples;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */
#endif /* #if defined (ARM_MATH_NEON) */

  while (blkCnt > 0U)
  {
    /* C[0] + jC[1] = A[0]+ j(-1)A[1] */

    /* Calculate Complex Conjugate and store result in destination buffer. */
    *pDst++ =  *pSrc++;
    *pDst++ = -*pSrc++;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of cmplx_conj group
 */
