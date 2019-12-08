/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_dot_prod_q7.c
 * Description:  Q7 dot product
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
  @brief         Dot product of Q7 vectors.
  @param[in]     pSrcA      points to the first input vector
  @param[in]     pSrcB      points to the second input vector
  @param[in]     blockSize  number of samples in each vector
  @param[out]    result     output result returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The intermediate multiplications are in 1.7 x 1.7 = 2.14 format and these
                   results are added to an accumulator in 18.14 format.
                   Nonsaturating additions are used and there is no danger of wrap around as long as
                   the vectors are less than 2^18 elements long.
                   The return result is in 18.14 format.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_dot_prod_q7(
    const q7_t * pSrcA,
    const q7_t * pSrcB,
    uint32_t blockSize,
    q31_t * result)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t vecA;
    q7x16_t vecB;
    q31_t     sum = 0;

    /* Compute 16 outputs at a time */
    blkCnt = blockSize >> 4;
    while (blkCnt > 0U)
    {
        /*
         * C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1]
         * Calculate dot product and then store the result in a temporary buffer.
         */
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        sum = vmladavaq(sum, vecA, vecB);
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrcA += 16;
        pSrcB += 16;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 0xF;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp8q(blkCnt);
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        sum = vmladavaq_p(sum, vecA, vecB, p0);
    }

    *result = sum;
}
#else
void arm_dot_prod_q7(
  const q7_t * pSrcA,
  const q7_t * pSrcB,
        uint32_t blockSize,
        q31_t * result)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t sum = 0;                                 /* Temporary return variable */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q31_t input1, input2;                          /* Temporary variables */
  q31_t inA1, inA2, inB1, inB2;                  /* Temporary variables */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0]* B[0] + A[1]* B[1] + A[2]* B[2] + .....+ A[blockSize-1]* B[blockSize-1] */

#if defined (ARM_MATH_DSP)
    /* read 4 samples at a time from sourceA */
    input1 = read_q7x4_ia ((q7_t **) &pSrcA);
    /* read 4 samples at a time from sourceB */
    input2 = read_q7x4_ia ((q7_t **) &pSrcB);

    /* extract two q7_t samples to q15_t samples */
    inA1 = __SXTB16(__ROR(input1, 8));
    /* extract reminaing two samples */
    inA2 = __SXTB16(input1);
    /* extract two q7_t samples to q15_t samples */
    inB1 = __SXTB16(__ROR(input2, 8));
    /* extract reminaing two samples */
    inB2 = __SXTB16(input2);

    /* multiply and accumulate two samples at a time */
    sum = __SMLAD(inA1, inB1, sum);
    sum = __SMLAD(inA2, inB2, sum);
#else
    sum += (q31_t) ((q15_t) *pSrcA++ * *pSrcB++);
    sum += (q31_t) ((q15_t) *pSrcA++ * *pSrcB++);
    sum += (q31_t) ((q15_t) *pSrcA++ * *pSrcB++);
    sum += (q31_t) ((q15_t) *pSrcA++ * *pSrcB++);
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
//    sum  = __SMLAD(*pSrcA++, *pSrcB++, sum);
//#else
    sum += (q31_t) ((q15_t) *pSrcA++ * *pSrcB++);
//#endif

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store result in destination buffer in 18.14 format */
  *result = sum;
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicDotProd group
 */
