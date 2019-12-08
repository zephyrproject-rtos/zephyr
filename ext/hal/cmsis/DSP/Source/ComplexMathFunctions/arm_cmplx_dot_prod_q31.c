/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cmplx_dot_prod_q31.c
 * Description:  Q31 complex dot product
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
  @brief         Q31 complex dot product.
  @param[in]     pSrcA       points to the first input vector
  @param[in]     pSrcB       points to the second input vector
  @param[in]     numSamples  number of samples in each vector
  @param[out]    realResult  real part of the result returned here
  @param[out]    imagResult  imaginary part of the result returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The intermediate 1.31 by 1.31 multiplications are performed with 64-bit precision and then shifted to 16.48 format.
                   The internal real and imaginary accumulators are in 16.48 format and provide 15 guard bits.
                   Additions are nonsaturating and no overflow will occur as long as <code>numSamples</code> is less than 32768.
                   The return results <code>realResult</code> and <code>imagResult</code> are in 16.48 format.
                   Input down scaling is not required.
 */

#if defined(ARM_MATH_MVEI)

void arm_cmplx_dot_prod_q31(
  const q31_t * pSrcA,
  const q31_t * pSrcB,
        uint32_t numSamples,
        q63_t * realResult,
        q63_t * imagResult)
{
    uint32_t blockSize = numSamples * CMPLX_DIM;  /* loop counters */
    uint32_t blkCnt;
    q31x4_t vecSrcA, vecSrcB;
    q63_t accReal = 0LL; 
    q63_t accImag = 0LL;

    q31_t a0,b0,c0,d0;

     /* Compute 2 complex samples at a time */
    blkCnt = blockSize >> 2U;

    while (blkCnt > 0U)
    {        

        vecSrcA = vld1q(pSrcA);
        vecSrcB = vld1q(pSrcB);

        accReal = vrmlsldavhaq(accReal, vecSrcA, vecSrcB);
        accImag = vrmlaldavhaxq(accImag, vecSrcA, vecSrcB);

        /*
         * Decrement the blkCnt loop counter
         * Advance vector source and destination pointers
         */
        pSrcA += 4;
        pSrcB += 4;
        blkCnt --;
    }

    accReal = asrl(accReal, (14 - 8));
    accImag = asrl(accImag, (14 - 8));

    /* Tail */
    blkCnt = (blockSize & 3) >> 1;

    while (blkCnt > 0U)
    {
      a0 = *pSrcA++;
      b0 = *pSrcA++;
      c0 = *pSrcB++;
      d0 = *pSrcB++;
  
      accReal += ((q63_t)a0 * c0) >> 14;
      accImag += ((q63_t)a0 * d0) >> 14;
      accReal -= ((q63_t)b0 * d0) >> 14;
      accImag += ((q63_t)b0 * c0) >> 14;
  
      /* Decrement loop counter */
      blkCnt--;
    }
  
    /* Store real and imaginary result in destination buffer. */
    *realResult = accReal;
    *imagResult = accImag;
}

#else
void arm_cmplx_dot_prod_q31(
  const q31_t * pSrcA,
  const q31_t * pSrcB,
        uint32_t numSamples,
        q63_t * realResult,
        q63_t * imagResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        q63_t real_sum = 0, imag_sum = 0;              /* Temporary result variables */
        q31_t a0,b0,c0,d0;

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = numSamples >> 2U;

  while (blkCnt > 0U)
  {
    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += ((q63_t)a0 * c0) >> 14;
    imag_sum += ((q63_t)a0 * d0) >> 14;
    real_sum -= ((q63_t)b0 * d0) >> 14;
    imag_sum += ((q63_t)b0 * c0) >> 14;

    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += ((q63_t)a0 * c0) >> 14;
    imag_sum += ((q63_t)a0 * d0) >> 14;
    real_sum -= ((q63_t)b0 * d0) >> 14;
    imag_sum += ((q63_t)b0 * c0) >> 14;

    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += ((q63_t)a0 * c0) >> 14;
    imag_sum += ((q63_t)a0 * d0) >> 14;
    real_sum -= ((q63_t)b0 * d0) >> 14;
    imag_sum += ((q63_t)b0 * c0) >> 14;

    a0 = *pSrcA++;
    b0 = *pSrcA++;
    c0 = *pSrcB++;
    d0 = *pSrcB++;

    real_sum += ((q63_t)a0 * c0) >> 14;
    imag_sum += ((q63_t)a0 * d0) >> 14;
    real_sum -= ((q63_t)b0 * d0) >> 14;
    imag_sum += ((q63_t)b0 * c0) >> 14;

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

    real_sum += ((q63_t)a0 * c0) >> 14;
    imag_sum += ((q63_t)a0 * d0) >> 14;
    real_sum -= ((q63_t)b0 * d0) >> 14;
    imag_sum += ((q63_t)b0 * c0) >> 14;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store real and imaginary result in 16.48 format  */
  *realResult = real_sum;
  *imagResult = imag_sum;
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of cmplx_dot_prod group
 */
