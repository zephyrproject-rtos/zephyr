/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_mean_q31.c
 * Description:  Mean value of a Q31 vector
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
  @ingroup groupStats
 */

/**
  @addtogroup mean
  @{
 */

/**
  @brief         Mean value of a Q31 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    mean value returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 64-bit internal accumulator.
                   The input is represented in 1.31 format and is accumulated in a 64-bit
                   accumulator in 33.31 format.
                   There is no risk of internal overflow with this approach, and the
                   full precision of intermediate result is preserved.
                   Finally, the accumulator is truncated to yield a result of 1.31 format.
 */
#if defined(ARM_MATH_MVEI)
void arm_mean_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
    uint32_t  blkCnt;           /* loop counters */
    q31x4_t vecSrc;
    q63_t     sum = 0LL;


    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {

        vecSrc = vldrwq_s32(pSrc);
        /*
         * sum lanes
         */
        sum = vaddlvaq(sum, vecSrc);

        blkCnt --;
        pSrc += 4;
    }

    /* Tail */
    blkCnt = blockSize & 0x3;

    while (blkCnt > 0U)
    {
      /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
      sum += *pSrc++;
      blkCnt --;
    }

    *pResult = arm_div_q63_to_q31(sum, blockSize);
}
#else
void arm_mean_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        q63_t sum = 0;                                 /* Temporary result storage */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
    sum += *pSrc++;

    sum += *pSrc++;

    sum += *pSrc++;

    sum += *pSrc++;

    /* Decrement the loop counter */
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
    /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) */
    sum += *pSrc++;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* C = (A[0] + A[1] + A[2] + ... + A[blockSize-1]) / blockSize  */
  /* Store result to destination */
  *pResult = (q31_t) (sum / blockSize);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of mean group
 */
