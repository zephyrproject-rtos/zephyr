/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_std_q31.c
 * Description:  Standard deviation of the elements of a Q31 vector
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
  @addtogroup STD
  @{
 */

/**
  @brief         Standard deviation of the elements of a Q31 vector.
  @param[in]     pSrc       points to the input vector.
  @param[in]     blockSize  number of samples in input vector.
  @param[out]    pResult    standard deviation value returned here.
  @return        none

  @par           Scaling and Overflow Behavior
                   The function is implemented using an internal 64-bit accumulator.
                   The input is represented in 1.31 format, which is then downshifted by 8 bits
                   which yields 1.23, and intermediate multiplication yields a 2.46 format.
                   The accumulator maintains full precision of the intermediate multiplication results,
                   but provides only a 16 guard bits.
                   There is no saturation on intermediate additions.
                   If the accumulator overflows it wraps around and distorts the result.
                   In order to avoid overflows completely the input signal must be scaled down by
                   log2(blockSize)-8 bits, as a total of blockSize additions are performed internally.
                   After division, internal variables should be Q18.46
                   Finally, the 18.46 accumulator is right shifted by 15 bits to yield a 1.31 format value.
 */
#if defined(ARM_MATH_MVEI)
void arm_std_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
    q31_t var=0;

    arm_var_q31(pSrc, blockSize, &var);
    arm_sqrt_q31(var, pResult);
}
#else
void arm_std_q31(
  const q31_t * pSrc,
        uint32_t blockSize,
        q31_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        q63_t sum = 0;                                 /* Accumulator */
        q63_t meanOfSquares, squareOfMean;             /* Square of mean and mean of square */
        q63_t sumOfSquares = 0;                        /* Sum of squares */
        q31_t in;                                      /* Temporary variable to store input value */

  if (blockSize <= 1U)
  {
    *pResult = 0;
    return;
  }

#if defined (ARM_MATH_LOOPUNROLL)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */
    /* C = A[0] + A[1] + ... + A[blockSize-1] */

    in = *pSrc++ >> 8U;
    /* Compute sum of squares and store result in a temporary variable, sumOfSquares. */
    sumOfSquares += ((q63_t) (in) * (in));
    /* Compute sum and store result in a temporary variable, sum. */
    sum += in;

    in = *pSrc++ >> 8U;
    sumOfSquares += ((q63_t) (in) * (in));
    sum += in;

    in = *pSrc++ >> 8U;
    sumOfSquares += ((q63_t) (in) * (in));
    sum += in;

    in = *pSrc++ >> 8U;
    sumOfSquares += ((q63_t) (in) * (in));
    sum += in;

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
    /* C = A[0] + A[1] + ... + A[blockSize-1] */

    in = *pSrc++ >> 8U;
    /* Compute sum of squares and store result in a temporary variable, sumOfSquares. */
    sumOfSquares += ((q63_t) (in) * (in));
    /* Compute sum and store result in a temporary variable, sum. */
    sum += in;

    /* Decrement loop counter */
    blkCnt--;
  }

  /* Compute Mean of squares and store result in a temporary variable, meanOfSquares. */
  meanOfSquares = (sumOfSquares / (q63_t)(blockSize - 1U));

  /* Compute square of mean */
  squareOfMean = ( sum * sum / (q63_t)(blockSize * (blockSize - 1U)));

  /* Compute standard deviation and store result in destination */
  arm_sqrt_q31((meanOfSquares - squareOfMean) >> 15U, pResult);
}
#endif /* defined(ARM_MATH_MVEI) */

/**
  @} end of STD group
 */
