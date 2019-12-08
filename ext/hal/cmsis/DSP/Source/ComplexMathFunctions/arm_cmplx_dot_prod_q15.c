/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_dot_prod_q15.c
 * Description:  Processing function for the Q15 Complex Dot product
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
  @addtogroup cmplx_dot_prod
  @{
 */

/**
  @brief         Q15 complex dot product.
  @param[in]     pSrcA       points to the first input vector
  @param[in]     pSrcB       points to the second input vector
  @param[in]     numSamples  number of samples in each vector
  @param[out]    realResult  real part of the result returned here
  @param[out]    imagResult  imaginary part of the result returned her
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The intermediate 1.15 by 1.15 multiplications are performed with full precision and yield a 2.30 result.
                   These are accumulated in a 64-bit accumulator with 34.30 precision.
                   As a final step, the accumulators are converted to 8.24 format.
                   The return results <code>realResult</code> and <code>imagResult</code> are in 8.24 format.
 */

#if defined(ARM_MATH_MVEI)
void arm_cmplx_dot_prod_q15(
  const q15_t * pSrcA,
  const q15_t * pSrcB,
        uint32_t numSamples,
        q31_t * realResult,
        q31_t * imagResult)
{

  uint32_t blockSize = numSamples * CMPLX_DIM;  /* loop counters */
  uint32_t blkCnt;
  q15_t a0,b0,c0,d0;

  q63_t accReal = 0LL; q63_t accImag = 0LL;
  q15x8_t vecSrcA, vecSrcB;



  /* should give more freedom to generate stall free code */
  vecSrcA = vld1q(pSrcA);
  vecSrcB = vld1q(pSrcB);
  pSrcA += 8;
  pSrcB += 8;

  /* Compute 4 complex samples at a time */
  blkCnt = blockSize >> 3;
  while (blkCnt > 0U) 
  {
      q15x8_t vecSrcC, vecSrcD;

      accReal = vmlsldavaq(accReal, vecSrcA, vecSrcB);
      vecSrcC = vld1q(pSrcA);
      pSrcA += 8;

      accImag = vmlaldavaxq(accImag, vecSrcA, vecSrcB);
      vecSrcD = vld1q(pSrcB);
      pSrcB += 8;

      accReal = vmlsldavaq(accReal, vecSrcC, vecSrcD);
      vecSrcA = vld1q(pSrcA);
      pSrcA += 8;

      accImag = vmlaldavaxq(accImag, vecSrcC, vecSrcD);
      vecSrcB = vld1q(pSrcB);
      pSrcB += 8;
      /*
       * Decrement the blockSize loop counter
       */
      blkCnt--;
  }

  /* Tail */
  pSrcA -= 8;
  pSrcB -= 8; 

  blkCnt = (blockSize & 7) >> 1;
  
  while (blkCnt > 0U)
  {
    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    accReal += (q31_t)a0 * c0;
    accImag += (q31_t)a0 * d0;
    accReal -= (q31_t)b0 * d0;
    accImag += (q31_t)b0 * c0;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store real and imaginary result in 8.24 format  */
  /* Convert real data in 34.30 to 8.24 by 6 right shifts */
  *realResult = (q31_t) (accReal >> 6);
  /* Convert imaginary data in 34.30 to 8.24 by 6 right shifts */
  *imagResult = (q31_t) (accImag >> 6);
}
#else
void arm_cmplx_dot_prod_q15(
  const q15_t * pSrcA,
  const q15_t * pSrcB,
        uint32_t numSamples,
        q31_t * realResult,
        q31_t * imagResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        q63_t real_sum = 0, imag_sum = 0;              /* Temporary result variables */
        q15_t a0,b0,c0,d0;

#if defined (ARM_MATH_LOOPUNROLL)
  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += (q31_t)a0 * c0;
    imag_sum += (q31_t)a0 * d0;
    real_sum -= (q31_t)b0 * d0;
    imag_sum += (q31_t)b0 * c0;

    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += (q31_t)a0 * c0;
    imag_sum += (q31_t)a0 * d0;
    real_sum -= (q31_t)b0 * d0;
    imag_sum += (q31_t)b0 * c0;

    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += (q31_t)a0 * c0;
    imag_sum += (q31_t)a0 * d0;
    real_sum -= (q31_t)b0 * d0;
    imag_sum += (q31_t)b0 * c0;

    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += (q31_t)a0 * c0;
    imag_sum += (q31_t)a0 * d0;
    real_sum -= (q31_t)b0 * d0;
    imag_sum += (q31_t)b0 * c0;

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
    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += (q31_t)a0 * c0;
    imag_sum += (q31_t)a0 * d0;
    real_sum -= (q31_t)b0 * d0;
    imag_sum += (q31_t)b0 * c0;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store real and imaginary result in 8.24 format  */
  /* Convert real data in 34.30 to 8.24 by 6 right shifts */
  *realResult = (q31_t) (real_sum >> 6);
  /* Convert imaginary data in 34.30 to 8.24 by 6 right shifts */
  *imagResult = (q31_t) (imag_sum >> 6);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of cmplx_dot_prod group
 */
