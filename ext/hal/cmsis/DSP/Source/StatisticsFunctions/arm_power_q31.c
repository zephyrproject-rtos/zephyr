/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_power_q31.c
 * Description:  Sum of the squares of the elements of a Q31 vector
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
  @addtogroup power
  @{
 */

/**
  @brief         Sum of the squares of the elements of a Q31 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    sum of the squares value returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 64-bit internal accumulator.
                   The input is represented in 1.31 format.
                   Intermediate multiplication yields a 2.62 format, and this
                   result is truncated to 2.48 format by discarding the lower 14 bits.
                   The 2.48 result is then added without saturation to a 64-bit accumulator in 16.48 format.
                   With 15 guard bits in the accumulator, there is no risk of overflow, and the
                   full precision of the intermediate multiplication is preserved.
                   Finally, the return result is in 16.48 format.
 */
#if defined(ARM_MATH_MVEI)
void arm_power_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q63_t * pResult)
{
    uint32_t     blkCnt;           /* loop counters */
    q31x4_t     vecSrc;
    q63_t       sum = 0LL;
    q31_t       in;

    /* Compute 4 outputs at a time */
    blkCnt = blockSize >> 2U;
    while (blkCnt > 0U)
    {
        vecSrc = vldrwq_s32(pSrc);
        /*
         * sum lanes
         */
        sum = vrmlaldavhaq(sum, vecSrc, vecSrc);

        blkCnt --;
        pSrc += 4;
    }

    /*
     * tail
     */
    blkCnt = blockSize & 0x3;
    while (blkCnt > 0U)
    {
       /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

       /* Compute Power and store result in a temporary variable, sum. */
       in = *pSrc++;
       sum += ((q63_t) in * in) >> 8;

       /* Decrement loop counter */
       blkCnt--;
    }

    *pResult = asrl(sum, 6);
}
#else
void arm_power_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q63_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        q63_t sum = 0;                                 /* Temporary result storage */
        q31_t in;                                      /* Temporary variable to store input value */

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

    /* Compute Power then shift intermediate results by 14 bits to maintain 16.48 format and store result in a temporary variable sum, providing 15 guard bits. */
    in = *pSrc++;
    sum += ((q63_t) in * in) >> 14U;

    in = *pSrc++;
    sum += ((q63_t) in * in) >> 14U;

    in = *pSrc++;
    sum += ((q63_t) in * in) >> 14U;

    in = *pSrc++;
    sum += ((q63_t) in * in) >> 14U;

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
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

    /* Compute Power and store result in a temporary variable, sum. */
    in = *pSrc++;
    sum += ((q63_t) in * in) >> 14U;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store results in 16.48 format */
  *pResult = sum;
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of power group
 */
