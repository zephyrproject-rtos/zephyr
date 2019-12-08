/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mult_cmplx_q31.c
 * Description:  Q31 complex-by-complex multiplication
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
  @addtogroup CmplxByCmplxMult
  @{
 */

/**
  @brief         Q31 complex-by-complex multiplication.
  @param[in]     pSrcA       points to first input vector
  @param[in]     pSrcB       points to second input vector
  @param[out]    pDst        points to output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function implements 1.31 by 1.31 multiplications and finally output is converted into 3.29 format.
                   Input down scaling is not required.
 */

#if defined(ARM_MATH_MVEI)
void arm_cmplx_mult_cmplx_q31(
  const q31_t * pSrcA,
  const q31_t * pSrcB,
        q31_t * pDst,
        uint32_t numSamples)
{

    uint32_t blkCnt;           /* loop counters */
    uint32_t blockSize = numSamples * CMPLX_DIM;  /* loop counters */
    q31x4_t vecA;
    q31x4_t vecB;
    q31x4_t vecDst;
    q31_t a, b, c, d;                              /* Temporary variables */

    /* Compute 2 complex outputs at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {

        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        /* C[2 * i] = A[2 * i] * B[2 * i] - A[2 * i + 1] * B[2 * i + 1].  */
        vecDst = vqdmlsdhq(vuninitializedq_s32(),vecA, vecB);
        /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i].  */
        vecDst = vqdmladhxq(vecDst, vecA, vecB);

        vecDst = vshrq(vecDst, 2);
        vst1q(pDst, vecDst);

        blkCnt --;
        pSrcA += 4;
        pSrcB += 4;
        pDst += 4;
    };

    blkCnt = (blockSize & 3) >> 1;
    while (blkCnt > 0U)
    {
      /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
      /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */
  
      a = *pSrcA++;
      b = *pSrcA++;
      c = *pSrcB++;
      d = *pSrcB++;
  
      /* store result in 3.29 format in destination buffer. */
      *pDst++ = (q31_t) ( (((q63_t) a * c) >> 33) - (((q63_t) b * d) >> 33) );
      *pDst++ = (q31_t) ( (((q63_t) a * d) >> 33) + (((q63_t) b * c) >> 33) );
  
      /* Decrement loop counter */
      blkCnt--;
    }
}
#else
void arm_cmplx_mult_cmplx_q31(
  const q31_t * pSrcA,
  const q31_t * pSrcB,
        q31_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t a, b, c, d;                              /* Temporary variables */

#if defined (ARM_MATH_LOOPUNROLL)

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
    /* store result in 3.29 format in destination buffer. */
    *pDst++ = (q31_t) ( (((q63_t) a * c) >> 33) - (((q63_t) b * d) >> 33) );
    *pDst++ = (q31_t) ( (((q63_t) a * d) >> 33) + (((q63_t) b * c) >> 33) );

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    *pDst++ = (q31_t) ( (((q63_t) a * c) >> 33) - (((q63_t) b * d) >> 33) );
    *pDst++ = (q31_t) ( (((q63_t) a * d) >> 33) + (((q63_t) b * c) >> 33) );

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    *pDst++ = (q31_t) ( (((q63_t) a * c) >> 33) - (((q63_t) b * d) >> 33) );
    *pDst++ = (q31_t) ( (((q63_t) a * d) >> 33) + (((q63_t) b * c) >> 33) );

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;
    *pDst++ = (q31_t) ( (((q63_t) a * c) >> 33) - (((q63_t) b * d) >> 33) );
    *pDst++ = (q31_t) ( (((q63_t) a * d) >> 33) + (((q63_t) b * c) >> 33) );

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = numSamples % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = numSamples;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C[2 * i    ] = A[2 * i] * B[2 * i    ] - A[2 * i + 1] * B[2 * i + 1]. */
    /* C[2 * i + 1] = A[2 * i] * B[2 * i + 1] + A[2 * i + 1] * B[2 * i    ]. */

    a = *pSrcA++;
    b = *pSrcA++;
    c = *pSrcB++;
    d = *pSrcB++;

    /* store result in 3.29 format in destination buffer. */
    *pDst++ = (q31_t) ( (((q63_t) a * c) >> 33) - (((q63_t) b * d) >> 33) );
    *pDst++ = (q31_t) ( (((q63_t) a * d) >> 33) + (((q63_t) b * c) >> 33) );

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of CmplxByCmplxMult group
 */
