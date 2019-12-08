/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_dot_prod_q15.c
 * Description:  Q15 dot product
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
  @ingroup groupMath
 */

/**
  @addtogroup BasicDotProd
  @{
 */

/**
  @brief         Dot product of Q15 vectors.
  @param[in]     pSrcA      points to the first input vector
  @param[in]     pSrcB      points to the second input vector
  @param[in]     blockSize  number of samples in each vector
  @param[out]    result     output result returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The intermediate multiplications are in 1.15 x 1.15 = 2.30 format and these
                   results are added to a 64-bit accumulator in 34.30 format.
                   Nonsaturating additions are used and given that there are 33 guard bits in the accumulator
                   there is no risk of overflow.
                   The return result is in 34.30 format.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_dot_prod_q15(
    const q15_t * pSrcA,
    const q15_t * pSrcB,
    uint32_t blockSize,
    q63_t * result)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecA;
    q15x8_t vecB;
    q63_t     sum = 0LL;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1]
         * Calculate dot product and then store the result in a temporary buffer.
         */
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        sum = vmlaldavaq(sum, vecA, vecB);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrcA += 8;
        pSrcB += 8;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        sum = vmlaldavaq_p(sum, vecA, vecB, p0);
    }

    *result = sum;
}

#else
void arm_dot_prod_q15(
  const q15_t * pSrcA,
  const q15_t * pSrcB,
        uint32_t blockSize,
        q63_t * result)
{
        uint32_t blkCnt;                               /* Loop counter */
        q63_t sum = 0;                                 /* Temporary return variable */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */

#if defined (ARM_MATH_DSP)
    /* Calculate dot product and store result in a temporary buffer. */
    sum = __SMLALD(read_q15x2_ia ((q15_t **) &pSrcA), read_q15x2_ia ((q15_t **) &pSrcB), sum);
    sum = __SMLALD(read_q15x2_ia ((q15_t **) &pSrcA), read_q15x2_ia ((q15_t **) &pSrcB), sum);
#else
    sum += (q63_t)((q31_t) *pSrcA++ * *pSrcB++);
    sum += (q63_t)((q31_t) *pSrcA++ * *pSrcB++);
    sum += (q63_t)((q31_t) *pSrcA++ * *pSrcB++);
    sum += (q63_t)((q31_t) *pSrcA++ * *pSrcB++);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Loop unrolling: Compute remaining outputs */
  blkCnt = blockSize % 0x4U;

#else

  /* Initialize blkCnt with number of samples */
  blkCnt = blockSize;

#endif /* #if defined (ARM_MATH_LOOPUNROLL) */

  while (blkCnt > 0U)
  {
    /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */

    /* Calculate dot product and store result in a temporary buffer. */
//#if defined (ARM_MATH_DSP)
//    sum  = __SMLALD(*pSrcA++, *pSrcB++, sum);
//#else
    sum += (q63_t)((q31_t) *pSrcA++ * *pSrcB++);
//#endif

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store result in destination buffer in 34.30 format */
  *result = sum;
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicDotProd group
 */
