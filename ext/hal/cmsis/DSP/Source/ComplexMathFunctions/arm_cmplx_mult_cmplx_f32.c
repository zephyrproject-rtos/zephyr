/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mult_cmplx_f32.c
 * Description:  Floating-point complex-by-complex multiplication
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
  @defgroup CmplxByCmplxMult Complex-by-Complex Multiplication

  Multiplies a complex vector by another complex vector and generates a complex result.
  The data in the complex arrays is stored in an interleaved fashion
  (real, imag, real, imag, ...).
  The parameter <code>numSamples</code> represents the number of complex
  samples processed.  The complex arrays have a total of <code>2*numSamples</code>
  real values.

  The underlying algorithm is used:

  <pre>
  for (n = 0; n < numSamples; n++) {
      pDst[(2*n)+0] = pSrcA[(2*n)+0] * pSrcB[(2*n)+0] - pSrcA[(2*n)+1] * pSrcB[(2*n)+1];
      pDst[(2*n)+1] = pSrcA[(2*n)+0] * pSrcB[(2*n)+1] + pSrcA[(2*n)+1] * pSrcB[(2*n)+0];
  }
  </pre>

  There are separate functions for floating-point, Q15, and Q31 data types.
 */

/**
  @addtogroup CmplxByCmplxMult
  @{
 */

/**
  @brief         Floating-point complex-by-complex multiplication.
  @param[in]     pSrcA       points to first input vector
  @param[in]     pSrcB       points to second input vector
  @param[out]    pDst        points to output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none
 */

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)

void arm_cmplx_mult_cmplx_f32(
  const float32_t * pSrcA,
  const float32_t * pSrcB,
        float32_t * pDst,
        uint32_t numSamples)
{
    uint32_t blkCnt;           /* loop counters */
    uint32_t blockSize = numSamples;  /* loop counters */
    float32_t a, b, c, d;  /* Temporary variables to store real and imaginary values */

    f32x4x2_t vecA;
    f32x4x2_t vecB;
    f32x4x2_t vecDst;

    /* Compute 4 complex outputs at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
            vecA = vld2q(pSrcA);  // load & separate real/imag pSrcA (de-interleave 2)
            vecB = vld2q(pSrcB);  // load & separate real/imag pSrcB
            pSrcA += 8;
            pSrcB += 8;

            /* C[2 * i] = A[2 * i] * B[2 * i] - A[2 * i + 1] * B[2 * i + 1].  */
            vecDst.val[0] = vmulq(vecA.val[0], vecB.val[0]);
            vecDst.val[0] = vfmsq(vecDst.val[0],vecA.val[1], vecB.val[1]);
            /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i].  */
            vecDst.val[1] = vmulq(vecA.val[0], vecB.val[1]);
            vecDst.val[1] = vfmaq(vecDst.val[1], vecA.val[1], vecB.val[0]);

            vst2q(pDst, vecDst);
            pDst += 8;

            blkCnt--;
    }

    /* Tail */
    blkCnt = blockSize & 3;
    while (blkCnt > 0U)
    {
      /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
      /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
  
      a = *pSrcA++;
      b = *pSrcA++;
      c = *pSrcB++;
      d = *pSrcB++;
  
      /* store result in destination buffer. */
      *pDst++ = (a * c) - (b * d);
      *pDst++ = (a * d) + (b * c);
  
      /* Decrement loop counter */
      blkCnt--;
    }

}

#else
void arm_cmplx_mult_cmplx_f32(
  const float32_t * pSrcA,
  const float32_t * pSrcB,
        float32_t * pDst,
        uint32_t numSamples)
{
    uint32_t blkCnt;                               /* Loop counter */
    float32_t a, b, c, d;  /* Temporary variables to store real and imaginary values */

#if defined(ARM_MATH_NEON) && !defined(ARM_MATH_AUTOVECTORIZE)
    float32x4x2_t va, vb;
    float32x4x2_t outCplx;

    /* Compute 4 outputs at a time */
    blkCnt = numSamples >> 2U;

    while (blkCnt > 0U)
    {
        va = vld2q_f32(pSrcA);  // load & separate real/imag pSrcA (de-interleave 2)
        vb = vld2q_f32(pSrcB);  // load & separate real/imag pSrcB

	/* Increment pointers */
        pSrcA += 8;
        pSrcB += 8;
	
	/* Re{C} = Re{A}*Re{B} - Im{A}*Im{B} */
        outCplx.val[0] = vmulq_f32(va.val[0], vb.val[0]);
        outCplx.val[0] = vmlsq_f32(outCplx.val[0], va.val[1], vb.val[1]);

	/* Im{C} = Re{A}*Im{B} + Im{A}*Re{B} */
        outCplx.val[1] = vmulq_f32(va.val[0], vb.val[1]);
        outCplx.val[1] = vmlaq_f32(outCplx.val[1], va.val[1], vb.val[0]);

        vst2q_f32(pDst, outCplx);

	/* Increment pointer */
        pDst += 8;

	/* Decrement the loop counter */
        blkCnt--;
    }

    /* Tail */
    blkCnt = numSamples & 3;

#else
#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
    /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    /* store result in destination buffer. */
    *pDst++ = (a * c) - (b * d);
    *pDst++ = (a * d) + (b * c);

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    *pDst++ = (a * c) - (b * d);
    *pDst++ = (a * d) + (b * c);

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    *pDst++ = (a * c) - (b * d);
    *pDst++ = (a * d) + (b * c);

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    *pDst++ = (a * c) - (b * d);
    *pDst++ = (a * d) + (b * c);

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
    /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
    /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;

    /* store result in destination buffer. */
    *pDst++ = (a * c) - (b * d);
    *pDst++ = (a * d) + (b * c);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE) */

/**
  @} end of CmplxByCmplxMult group
 */
