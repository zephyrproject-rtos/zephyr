/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_mag_q31.c
 * Description:  Q31 complex magnitude
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
  @addtogroup cmplx_mag
  @{
 */

/**
  @brief         Q31 complex magnitude.
  @param[in]     pSrc        points to input vector
  @param[out]    pDst        points to output vector
  @param[in]     numSamples  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function implements 1.31 by 1.31 multiplications and finally output is converted into 2.30 format.
                   Input down scaling is not required.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_cmplx_mag_q31(
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t numSamples)
{
    int32_t blockSize = numSamples;  /* loop counters */
    uint32_t  blkCnt;           /* loop counters */

    q31x4x2_t vecSrc;
    q31x4_t sum;

    q31_t real, imag;                              /* Temporary input variables */
    q31_t acc0, acc1;                              /* Accumulators */

    /* Compute 4 complex samples at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        vecSrc = vld2q(pSrc);

        sum = vqaddq(vmulhq(vecSrc.val[0], vecSrc.val[0]),
                     vmulhq(vecSrc.val[1], vecSrc.val[1]));

        sum = vshrq(sum, 1);

        /*

        This function is using a table. There are compilations flags to avoid
        including this table (and in this case, arm_cmplx_maq_q31 must not
        be built and linked.)

        */
        sum = FAST_VSQRT_Q31(sum);

        vst1q(pDst, sum);

        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        pSrc += 8;
        pDst += 4;
    }

    /*
     * tail
     */
    blkCnt = blockSize & 3;
    while (blkCnt > 0U)
    {
      /* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */
  
      real = *pSrc++;
      imag = *pSrc++;
      acc0 = (q31_t) (((q63_t) real * real) >> 33);
      acc1 = (q31_t) (((q63_t) imag * imag) >> 33);
  
      /* store result in 2.30 format in destination buffer. */
      arm_sqrt_q31(acc0 + acc1, pDst++);
  
      /* Decrement loop counter */
      blkCnt--;
    }
}

#else
void arm_cmplx_mag_q31(
  const q31_t * pSrc,
        q31_t * pDst,
        uint32_t numSamples)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t real, imag;                              /* Temporary input variables */
        q31_t acc0, acc1;                              /* Accumulators */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    /* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = (q31_t) (((q63_t) real * real) >> 33);
    acc1 = (q31_t) (((q63_t) imag * imag) >> 33);

    /* store result in 2.30 format in destination buffer. */
    arm_sqrt_q31(acc0 + acc1, pDst++);

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = (q31_t) (((q63_t) real * real) >> 33);
    acc1 = (q31_t) (((q63_t) imag * imag) >> 33);
    arm_sqrt_q31(acc0 + acc1, pDst++);

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = (q31_t) (((q63_t) real * real) >> 33);
    acc1 = (q31_t) (((q63_t) imag * imag) >> 33);
    arm_sqrt_q31(acc0 + acc1, pDst++);

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = (q31_t) (((q63_t) real * real) >> 33);
    acc1 = (q31_t) (((q63_t) imag * imag) >> 33);
    arm_sqrt_q31(acc0 + acc1, pDst++);

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
    /* C[0] = sqrt(A[0] * A[0] + A[1] * A[1]) */

    real = *pSrc++;
    imag = *pSrc++;
    acc0 = (q31_t) (((q63_t) real * real) >> 33);
    acc1 = (q31_t) (((q63_t) imag * imag) >> 33);

    /* store result in 2.30 format in destination buffer. */
    arm_sqrt_q31(acc0 + acc1, pDst++);

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of cmplx_mag group
 */
