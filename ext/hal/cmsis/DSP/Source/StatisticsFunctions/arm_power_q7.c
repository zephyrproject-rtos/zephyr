/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_power_q7.c
 * Description:  Sum of the squares of the elements of a Q7 vector
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
  @brief         Sum of the squares of the elements of a Q7 vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    sum of the squares value returned here
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using a 32-bit internal accumulator.
                   The input is represented in 1.7 format.
                   Intermediate multiplication yields a 2.14 format, and this
                   result is added without saturation to an accumulator in 18.14 format.
                   With 17 guard bits in the accumulator, there is no risk of overflow, and the
                   full precision of the intermediate multiplication is preserved.
                   Finally, the return result is in 18.14 format.
 */
#if defined(ARM_MATH_MVEI)
void arm_power_q7(
  const q7_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
    uint32_t  blkCnt;           /* loop counters */
    q7x16_t vecSrc;
    q31_t   sum = 0LL;
    q7_t in;

   /* Compute 16 outputs at a time */
    blkCnt = blockSize >> 4U;
    while (blkCnt > 0U)
    {
        vecSrc = vldrbq_s8(pSrc);
        /*
         * sum lanes
         */
        sum = vmladavaq(sum, vecSrc, vecSrc);

        blkCnt--;
        pSrc += 16;
    }

    /*
     * tail
     */
    blkCnt = blockSize & 0xF;
    while (blkCnt > 0U)
    {
       /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

       /* Compute Power and store result in a temporary variable, sum. */
       in = *pSrc++;
       sum += ((q15_t) in * in);

       /* Decrement loop counter */
       blkCnt--;
    }

    *pResult = sum;
}
#else
void arm_power_q7(
  const q7_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        q31_t sum = 0;                                 /* Temporary result storage */
        q7_t in;                                       /* Temporary variable to store input value */

#if defined (ARM_MATH_LOOPUNROLL) && defined (ARM_MATH_DSP)
        q31_t in32;                                    /* Temporary variable to store packed input value */
        q31_t in1, in2;                                /* Temporary variables to store input value */
#endif

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */

    /* Compute Power and store result in a temporary variable, sum. */
#if defined (ARM_MATH_DSP)
    in32 = read_q7x4_ia ((q7_t **) &pSrc);

    in1 = __SXTB16(__ROR(in32, 8));
    in2 = __SXTB16(in32);

    /* calculate power and accumulate to accumulator */
    sum = __SMLAD(in1, in1, sum);
    sum = __SMLAD(in2, in2, sum);
#else
    in = *pSrc++;
    sum += ((q15_t) in * in);

    in = *pSrc++;
    sum += ((q15_t) in * in);

    in = *pSrc++;
    sum += ((q15_t) in * in);

    in = *pSrc++;
    sum += ((q15_t) in * in);
#endif /* #if defined (ARM_MATH_DSP) */

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
    sum += ((q15_t) in * in);

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Store result in 18.14 format */
  *pResult = sum;
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of power group
 */
