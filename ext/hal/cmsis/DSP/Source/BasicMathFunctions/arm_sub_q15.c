/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_sub_q15.c
 * Description:  Q15 vector subtraction
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
  @addtogroup BasicSub
  @{
 */

/**
  @brief         Q15 vector subtraction.
  @param[in]     pSrcA      points to the first input vector
  @param[in]     pSrcB      points to the second input vector
  @param[out]    pDst       points to the output vector
  @param[in]     blockSize  number of samples in each vector
  @return        none

  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   Results outside of the allowable Q15 range [0x8000 0x7FFF] are saturated.
 */

#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_sub_q15(
    const q15_t * pSrcA,
    const q15_t * pSrcB,
    q15_t * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecA;
    q15x8_t vecB;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C = A - B
         * Subtract and then store the results in the destination buffer.
         */
        vecA = vld1q(pSrcA);
        vecB = vld1q(pSrcB);
        vst1q(pDst, vqsubq(vecA, vecB));
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrcA  += 8;
        pSrcB  += 8;
        pDst   += 8;
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
        vstrhq_p(pDst, vqsubq(vecA, vecB), p0);
    }
}


#else
void arm_sub_q15(
  const q15_t * pSrcA,
  const q15_t * pSrcB,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q31_t inA1, inA2;
  q31_t inB1, inB2;
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A - B */

#if defined (ARM_MATH_DSP)
    /* read 2 times 2 samples at a time from sourceA */
    inA1 = read_q15x2_ia ((q15_t **) &pSrcA);
    inA2 = read_q15x2_ia ((q15_t **) &pSrcA);
    /* read 2 times 2 samples at a time from sourceB */
    inB1 = read_q15x2_ia ((q15_t **) &pSrcB);
    inB2 = read_q15x2_ia ((q15_t **) &pSrcB);

    /* Subtract and store 2 times 2 samples at a time */
    write_q15x2_ia (&pDst, __QSUB16(inA1, inB1));
    write_q15x2_ia (&pDst, __QSUB16(inA2, inB2));
#else
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrcA++ - *pSrcB++), 16);
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrcA++ - *pSrcB++), 16);
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrcA++ - *pSrcB++), 16);
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrcA++ - *pSrcB++), 16);
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
    /* C = A - B */

    /* Subtract and store result in destination buffer. */
#if defined (ARM_MATH_DSP)
    *pDst++ = (q15_t) __QSUB16(*pSrcA++, *pSrcB++);
#else
    *pDst++ = (q15_t) __SSAT(((q31_t) *pSrcA++ - *pSrcB++), 16);
#endif

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicSub group
 */
