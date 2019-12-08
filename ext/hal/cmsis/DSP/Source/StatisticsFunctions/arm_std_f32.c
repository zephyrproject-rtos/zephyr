/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_std_f32.c
 * Description:  Standard deviation of the elements of a floating-point vector
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
  @defgroup STD Standard deviation

  Calculates the standard deviation of the elements in the input vector.
  The underlying algorithm is used:

  <pre>
      Result = sqrt((sumOfSquares - sum<sup>2</sup> / blockSize) / (blockSize - 1))

      sumOfSquares = pSrc[0] * pSrc[0] + pSrc[1] * pSrc[1] + ... + pSrc[blockSize-1] * pSrc[blockSize-1]
      sum = pSrc[0] + pSrc[1] + pSrc[2] + ... + pSrc[blockSize-1]
  </pre>

  There are separate functions for floating point, Q31, and Q15 data types.
 */

/**
  @addtogroup STD
  @{
 */

/**
  @brief         Standard deviation of the elements of a floating-point vector.
  @param[in]     pSrc       points to the input vector
  @param[in]     blockSize  number of samples in input vector
  @param[out]    pResult    standard deviation value returned here
  @return        none
 */
#if (defined(ARM_MATH_NEON_EXPERIMENTAL) || defined(ARM_MATH_MVEF)) && !defined(ARM_MATH_AUTOVECTORIZE)
void arm_std_f32(
  const float32_t * pSrc,
        uint32_t blockSize,
        float32_t * pResult)
{
  float32_t var;
  arm_var_f32(pSrc,blockSize,&var);
  arm_sqrt_f32(var, pResult);
}
#else
void arm_std_f32(
  const float32_t * pSrc,
        uint32_t blockSize,
        float32_t * pResult)
{
        uint32_t blkCnt;                               /* Loop counter */
        float32_t sum = 0.0f;                          /* Temporary result storage */
        float32_t sumOfSquares = 0.0f;                 /* Sum of squares */
        float32_t in;                                  /* Temporary variable to store input value */

#ifndef ARM_MATH_CM0_FAMILY
        float32_t meanOfSquares, mean, squareOfMean;   /* Temporary variables */
#else
        float32_t squareOfSum;                         /* Square of Sum */
        float32_t var;                                 /* Temporary varaince storage */
#endif

  if (blockSize <= 1U)
  {
    *pResult = 0;
    return;
  }

#if defined (ARM_MATH_LOOPUNROLL) && !defined(ARM_MATH_AUTOVECTORIZE)

  /* Loop unrolling: Compute 4 outputs at a time */
  blkCnt = blockSize >> 2U;

  while (blkCnt > 0U)
  {
    /* C = A[0] * A[0] + A[1] * A[1] + ... + A[blockSize-1] * A[blockSize-1] */
    /* C = A[0] + A[1] + ... + A[blockSize-1] */

    in = *pSrc++;
    /* Compute sum of squares and store result in a temporary variable, sumOfSquares. */
    sumOfSquares += in * in;
    /* Compute sum and store result in a temporary variable, sum. */
    sum += in;

    in = *pSrc++;
    sumOfSquares += in * in;
    sum += in;

    in = *pSrc++;
    sumOfSquares += in * in;
    sum += in;

    in = *pSrc++;
    sumOfSquares += in * in;
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

    in = *pSrc++;
    /* Compute sum of squares and store result in a temporary variable, sumOfSquares. */
    sumOfSquares += ( in * in);
    /* Compute sum and store result in a temporary variable, sum. */
    sum += in;

    /* Decrement loop counter */
    blkCnt--;
  }

#ifndef ARM_MATH_CM0_FAMILY

  /* Compute Mean of squares and store result in a temporary variable, meanOfSquares. */
  meanOfSquares = sumOfSquares / ((float32_t) blockSize - 1.0f);

  /* Compute mean of all input values */
  mean = sum / (float32_t) blockSize;

  /* Compute square of mean */
  squareOfMean = (mean * mean) * (((float32_t) blockSize) /
                                  ((float32_t) blockSize - 1.0f));

  /* Compute standard deviation and store result to destination */
  arm_sqrt_f32((meanOfSquares - squareOfMean), pResult);

#else
  /* Run the below code for Cortex-M0 */

  /* Compute square of sum */
  squareOfSum = ((sum * sum) / (float32_t) blockSize);

  /* Compute variance */
  var = ((sumOfSquares - squareOfSum) / (float32_t) (blockSize - 1.0f));

  /* Compute standard deviation and store result in destination */
  arm_sqrt_f32(var, pResult);

#endif /* #ifndef ARM_MATH_CM0_FAMILY */

}
#endif /* #if defined(ARM_MATH_NEON) || defined(ARM_MATH_MVEF)*/

/**
  @} end of STD group
 */
