/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_negate_q15.c
 * Description:  Negates Q15 vectors
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
  @addtogroup BasicNegate
  @{
 */

/**
  @brief         Negates the elements of a Q15 vector.
  @param[in]     pSrc       points to the input vector.
  @param[out]    pDst       points to the output vector.
  @param[in]     blockSize  number of samples in each vector.
  @return        none

  @par           Conditions for optimum performance
                   Input and output buffers should be aligned by 32-bit
  @par           Scaling and Overflow Behavior
                   The function uses saturating arithmetic.
                   The Q15 value -1 (0x8000) is saturated to the maximum allowable positive value 0x7FFF.
 */
#if defined(ARM_MATH_MVEI)

#include "arm_helium_utils.h"

void arm_negate_q15(
    const q15_t  * pSrc,
    q15_t  * pDst,
    uint32_t blockSize)
{
    uint32_t  blkCnt;           /* loop counters */
    q15x8_t vecSrc;

    /* Compute 8 outputs at a time */
    blkCnt = blockSize >> 3;
    while (blkCnt > 0U)
    {
        /*
         * C = -A
         * Negate and then store the results in the destination buffer.
         */
        vecSrc = vld1q(pSrc);
        vst1q(pDst, vqnegq(vecSrc));
        /*
         * Decrement the blockSize loop counter
         */
        blkCnt--;
        /*
         * advance vector source and destination pointers
         */
        pSrc += 8;
        pDst += 8;
    }
    /*
     * tail
     */
    blkCnt = blockSize & 7;
    if (blkCnt > 0U)
    {
        mve_pred16_t p0 = vctp16q(blkCnt);
        vecSrc = vld1q(pSrc);
        vstrhq_p(pDst, vqnegq(vecSrc), p0);
    }
}

#else
void arm_negate_q15(
  const q15_t * pSrc,
        q15_t * pDst,
        uint32_t blockSize)
{
        uint32_t blkCnt;                               /* Loop counter */
        q15_t in;                                      /* Temporary input variable */

#if defined (ARM_MATH_LOOPUNROLL)

#if defined (ARM_MATH_DSP)
  q31_t in1;                                    /* Temporary input variables */
#endif

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = -A */

#if defined (ARM_MATH_DSP)
    /* Negate and store result in destination buffer (2 samples at a time). */
    in1 = read_q15x2_ia ((q15_t **) &pSrc);
    write_q15x2_ia (&pDst, __QSUB16(0, in1));

    in1 = read_q15x2_ia ((q15_t **) &pSrc);
    write_q15x2_ia (&pDst, __QSUB16(0, in1));
#else
    in = *pSrc++;
    *pDst++ = (in == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in;

    in = *pSrc++;
    *pDst++ = (in == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in;

    in = *pSrc++;
    *pDst++ = (in == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in;

    in = *pSrc++;
    *pDst++ = (in == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in;
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
    /* C = -A */

    /* Negate and store result in destination buffer. */
    in = *pSrc++;
    *pDst++ = (in == (q15_t) 0x8000) ? (q15_t) 0x7fff : -in;

    /* Decrement loop counter */
    blkCnt--;
  }

}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of BasicNegate group
 */
