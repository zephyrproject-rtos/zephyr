/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_conj_q31.c
 * Description:  Q31 complex conjugate
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
  @addtogroup cmplx_conj
  @{
 */

/**
  @brief         Q31 complex conjugate.
  @param[in]     pSrc        points to the input vector
  @param[out]    pDst        points to the output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   The Q31 value -1 (0x80000000) is saturated to the maximum allowable positive value 0x7FFFFFFF.
 */

#if defined(ARM_MATH_MVEI)

void arm_cmplx_conj_q31(
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t numSamples)
{

    uint32_t blockSize = numSamples * CMPLX_DIM;   /* loop counters */
    uint32_t blkCnt;
    q31x4_t vecSrc;
    q31x4_t vecSign;
    q31_t in;                                      /* Temporary input variable */

    /*
     * {2, 0, 2, 0} - {1, 1, 1, 1}
     */
    vecSign = vsubq(vdwdupq_u32(2, 4, 2), vdupq_n_u32(1));

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
        blkCnt --;
    }

     /* Tail */
    blkCnt = (blockSize & 0x3) >> 1;

    while (blkCnt > 0U)
    {
      /* C[0] + jC[1] = A[0]+ j(-1)A[1] */
  
      /* Calculate Complex Conjugate and store result in destination buffer. */
      *pDst++ =  *pSrc++;
      in = *pSrc++;
      *pDst++ = __QSUB(0, in);
  
      /* Decrement loop counter */
      blkCnt--;
    }


}
#else

void arm_cmplx_conj_q31(
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t in;                                      /* Temporary input variable */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] + jC[1] = A[0]+ j(-1)A[1] */

    /* Calculate Complex Conjugate and store result in destination buffer. */
    *pDst++ =  *pSrc++;
    in = *pSrc++;
#if defined (ARM_MATH_DSP)
    *pDst++ = __QSUB(0, in);
#else
    *pDst++ = (in == INT32_MIN) ? INT32_MAX : -in;
#endif

    *pDst++ =  *pSrc++;
    in =  *pSrc++;
#if defined (ARM_MATH_DSP)
    *pDst++ = __QSUB(0, in);
#else
    *pDst++ = (in == INT32_MIN) ? INT32_MAX : -in;
#endif

    *pDst++ =  *pSrc++;
    in = *pSrc++;
#if defined (ARM_MATH_DSP)
    *pDst++ = __QSUB(0, in);
#else
    *pDst++ = (in == INT32_MIN) ? INT32_MAX : -in;
#endif

    *pDst++ =  *pSrc++;
    in = *pSrc++;
#if defined (ARM_MATH_DSP)
    *pDst++ = __QSUB(0, in);
#else
    *pDst++ = (in == INT32_MIN) ? INT32_MAX : -in;
#endif

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
    /* C[0] + jC[1] = A[0]+ j(-1)A[1] */

    /* Calculate Complex Conjugate and store result in destination buffer. */
    *pDst++ =  *pSrc++;
    in = *pSrc++;
#if defined (ARM_MATH_DSP)
    *pDst++ = __QSUB(0, in);
#else
    *pDst++ = (in == INT32_MIN) ? INT32_MAX : -in;
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of cmplx_conj group
 */
