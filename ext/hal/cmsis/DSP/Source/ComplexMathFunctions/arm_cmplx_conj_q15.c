/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_conj_q15.c
 * Description:  Q15 complex conjugate
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
  @brief         Q15 complex conjugate.
  @param[in]     pSrc        points to the input vector
  @param[out]    pDst        points to the output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   The Q15 value -1 (0x8000) is saturated to the maximum allowable positive value 0x7FFF.
 */


#if defined(ARM_MATH_MVEI)
void arm_cmplx_conj_q15(
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t numSamples)
{
    uint32_t blockSize = numSamples * CMPLX_DIM;   /* loop counters */
    uint32_t blkCnt;
    q31_t in1; 

    q15x8_t vecSrc;
    q15x8_t vecSign;

    /*
     * {2, 0, 2, 0, 2, 0, 2, 0} - {1, 1, 1, 1, 1, 1, 1, 1}
     */
    vecSign = vsubq(vdwdupq_u16(2, 4, 2), vdupq_n_u16(1));


    /* Compute 8 real samples at a time */
    blkCnt = blockSize >> 3U;
    while (blkCnt > 0U)
    {
        vecSrc = vld1q(pSrc);
        vst1q(pDst,vmulq(vecSrc, vecSign));
        /*
         * Decrement the blkCnt loop counter
         * Advance vector source and destination pointers
         */
        pSrc += 8;
        pDst += 8;
        blkCnt --;
    }
    
     /* Tail */
    blkCnt = (blockSize & 0x7) >> 1;

    while (blkCnt > 0U)
    {
      /* C[0] + jC[1] = A[0]+ j(-1)A[1] */
  
      /* Calculate Complex Conjugate and store result in destination buffer. */
      *pDst++ =  *pSrc++;
      in1 = *pSrc++;
      *pDst++ = __SSAT(-in1, 16);
  
      /* Decrement loop counter */
      blkCnt--;
    }
}
#else
void arm_cmplx_conj_q15(
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t in1;                                     /* Temporary input variable */

#if defined (ARM_MATH_LOOPUNROLL) && defined (ARM_MATH_DSP)
        q31_t in2, in3, in4;                           /* Temporary input variables */
#endif


#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] + jC[1] = A[0]+ j(-1)A[1] */

    /* Calculate Complex Conjugate and store result in destination buffer. */

    #if defined (ARM_MATH_DSP)
    in1 = read_q15x2_ia ((q15_t **) &pSrc);
    in2 = read_q15x2_ia ((q15_t **) &pSrc);
    in3 = read_q15x2_ia ((q15_t **) &pSrc);
    in4 = read_q15x2_ia ((q15_t **) &pSrc);

#ifndef ARM_MATH_BIG_ENDIAN
    in1 = __QASX(0, in1);
    in2 = __QASX(0, in2);
    in3 = __QASX(0, in3);
    in4 = __QASX(0, in4);
#else
    in1 = __QSAX(0, in1);
    in2 = __QSAX(0, in2);
    in3 = __QSAX(0, in3);
    in4 = __QSAX(0, in4);
#endif /* #ifndef ARM_MATH_BIG_ENDIAN */

    in1 = ((uint32_t) in1 >> 16) | ((uint32_t) in1 << 16);
    in2 = ((uint32_t) in2 >> 16) | ((uint32_t) in2 << 16);
    in3 = ((uint32_t) in3 >> 16) | ((uint32_t) in3 << 16);
    in4 = ((uint32_t) in4 >> 16) | ((uint32_t) in4 << 16);

    write_q15x2_ia (&pDst, in1);
    write_q15x2_ia (&pDst, in2);
    write_q15x2_ia (&pDst, in3);
    write_q15x2_ia (&pDst, in4);
#else
    *pDst++ =  *pSrc++;
    in1 = *pSrc++;
    *pDst++ = (in1 == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in1;

    *pDst++ =  *pSrc++;
    in1 = *pSrc++;
    *pDst++ = (in1 == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in1;

    *pDst++ =  *pSrc++;
    in1 = *pSrc++;
    *pDst++ = (in1 == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in1;

    *pDst++ =  *pSrc++;
    in1 = *pSrc++;
    *pDst++ = (in1 == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in1;

#endif /* #if defined (ARM_MATH_DSP) */

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
    in1 = *pSrc++;
#if defined (ARM_MATH_DSP)
    *pDst++ = __SSAT(-in1, 16);
#else
    *pDst++ = (in1 == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in1;
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of cmplx_conj group
 */
