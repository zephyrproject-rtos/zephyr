/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mag_squared_q15.c
 * Description:  Q15 complex magnitude squared
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
  @addtogroup cmplx_mag_squared
  @{
 */

/**
  @brief         Q15 complex magnitude squared.
  @param[in]     pSrc        points to input vector
  @param[out]    pDst        points to output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function implements 1.15 by 1.15 multiplications and finally output is converted into 3.13 format.
 */

#if defined(ARM_MATH_MVEI)

void arm_cmplx_mag_squared_q15(
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t numSamples)
{
  int32_t blockSize = numSamples;  /* loop counters */
  uint32_t  blkCnt;           /* loop counters */
  q31_t in;
  q31_t acc0;                                    /* Accumulators */
  q15x8x2_t vecSrc;
  q15x8_t vReal, vImag;
  q15x8_t vMagSq;

  
  blkCnt = blockSize >> 3;
  while (blkCnt > 0U)
  {
    vecSrc = vld2q(pSrc);
    vReal = vmulhq(vecSrc.val[0], vecSrc.val[0]);
    vImag = vmulhq(vecSrc.val[1], vecSrc.val[1]);
    vMagSq = vqaddq(vReal, vImag);
    vMagSq = vshrq(vMagSq, 1);

    vst1q(pDst, vMagSq);

    pSrc += 16;
    pDst += 8;
    /*
     * Decrement the blkCnt loop counter
     * Advance vector source and destination pointers
     */
    blkCnt --;
  }

  /*
   * tail
   */
  blkCnt = blockSize & 7;
  while (blkCnt > 0U)
  {
    /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

    in = read_q15x2_ia ((q15_t **) &pSrc);
    acc0 = __SMUAD(in, in);

    /* store result in 3.13 format in destination buffer. */
    *pDst++ = (q15_t) (acc0 >> 17);


    /* Decrement loop counter */
    blkCnt--;
  }

}

#else
void arm_cmplx_mag_squared_q15(
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_DSP)
        q31_t in;
        q31_t acc0;                                    /* Accumulators */
#else
        q15_t real, imag;                              /* Temporary input variables */
        q31_t acc0, acc1;                              /* Accumulators */
#endif

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

#if defined (ARM_MATH_DSP)
    in = read_q15x2_ia ((q15_t **) &pSrc);
    acc0 = __SMUAD(in, in);
    /* store result in 3.13 format in destination buffer. */
    *pDst++ = (q15_t) (acc0 >> 17);

    in = read_q15x2_ia ((q15_t **) &pSrc);
    acc0 = __SMUAD(in, in);
    *pDst++ = (q15_t) (acc0 >> 17);

    in = read_q15x2_ia ((q15_t **) &pSrc);
    acc0 = __SMUAD(in, in);
    *pDst++ = (q15_t) (acc0 >> 17);

    in = read_q15x2_ia ((q15_t **) &pSrc);
    acc0 = __SMUAD(in, in);
    *pDst++ = (q15_t) (acc0 >> 17);
#else
    real = *pSrc++;
    imag = *pSrc++;
    acc0 = ((q31_t) real * real);
    acc1 = ((q31_t) imag * imag);
    /* store result in 3.13 format in destination buffer. */
    *pDst++ = (q15_t) (((q63_t) acc0 + acc1) >> 17);

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = ((q31_t) real * real);
    acc1 = ((q31_t) imag * imag);
    *pDst++ = (q15_t) (((q63_t) acc0 + acc1) >> 17);

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = ((q31_t) real * real);
    acc1 = ((q31_t) imag * imag);
    *pDst++ = (q15_t) (((q63_t) acc0 + acc1) >> 17);

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = ((q31_t) real * real);
    acc1 = ((q31_t) imag * imag);
    *pDst++ = (q15_t) (((q63_t) acc0 + acc1) >> 17);
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
    /* C[0] = (A[0] * A[0] + A[1] * A[1]) */

#if defined (ARM_MATH_DSP)
    in = read_q15x2_ia ((q15_t **) &pSrc);
    acc0 = __SMUAD(in, in);

    /* store result in 3.13 format in destination buffer. */
    *pDst++ = (q15_t) (acc0 >> 17);
#else
    real = *pSrc++;
    imag = *pSrc++;
    acc0 = ((q31_t) real * real);
    acc1 = ((q31_t) imag * imag);

    /* store result in 3.13 format in destination buffer. */
    *pDst++ = (q15_t) (((q63_t) acc0 + acc1) >> 17);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

}

#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of cmplx_mag_squared group
 */
