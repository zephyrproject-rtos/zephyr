/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mult_q31.c
 * Description:  Q31 vector multiplication
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
  @addtogroup BasicMult
  @{
 */

/**
  @brief         Q31 vector multiplication.
  @param[in]     pSrcA      points to the first input vector.
  @param[in]     pSrcB      points to the second input vector.
  @param[out]    pDst       points to the output vector.
  @param[in]     blockSize  number of samples in each vector.
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q31 range[0x80000000 0x7FFFFFFF] are saturated.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_mult_q31(
    const q31_t * pSrcA,
    const q31_t * pSrcB,
    q31_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4_t vecA, vecB;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2;
    while (blkCnt > 0U)
    {
        /*
         * C = A * B
         * Multiply the inputs and then store the results in the destination buffer.
         */
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vst1q(pDst, vqdmulhq(vecA, vecB));
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrcA  += 4;
        pSrcB  += 4;
        pDst   += 4;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 3;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp32q(blkCnt);
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vstrwq_p(pDst, vqdmulhq(vecA, vecB), p0);
    }
}

#else
void arm_mult_q31(
  const q31_t * pSrcA,
  const q31_t * pSrcB,
        q31_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t out;                                     /* Temporary output variable */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A * B */

    /* Multiply inputs and store result in destination buffer. */
    out = ((q63_t) *pSrcA++ * *pSrcB++) >> 32;
    out = __SSAT(out, 31);
    *pDst++ = out << 1U;

    out = ((q63_t) *pSrcA++ * *pSrcB++) >> 32;
    out = __SSAT(out, 31);
    *pDst++ = out << 1U;

    out = ((q63_t) *pSrcA++ * *pSrcB++) >> 32;
    out = __SSAT(out, 31);
    *pDst++ = out << 1U;

    out = ((q63_t) *pSrcA++ * *pSrcB++) >> 32;
    out = __SSAT(out, 31);
    *pDst++ = out << 1U;

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
    /* C = A * B */

    /* Multiply inputs and store result in destination buffer. */
    out = ((q63_t) *pSrcA++ * *pSrcB++) >> 32;
    out = __SSAT(out, 31);
    *pDst++ = out << 1U;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicMult group
 */
